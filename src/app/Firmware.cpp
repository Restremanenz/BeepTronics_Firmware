#include "Firmware.h"
#include "Config.h"
#include <Arduino.h>
#include <cmath>

namespace {
const uint8_t buttonPins[] = {pins::buttonOn, pins::buttonDown, pins::buttonUp, pins::buttonOk};
}

void Firmware::begin() {
    digitalWrite(pins::powerLatch, HIGH);
    pinMode(pins::powerLatch, OUTPUT);
    digitalWrite(pins::sensorPower, LOW);
    pinMode(pins::sensorPower, OUTPUT); // GPS and IMU are not integrated yet.
    digitalWrite(pins::displayCs, HIGH);
    pinMode(pins::displayCs, OUTPUT);
    Serial.begin(115200); // Never wait for USB: the instrument must boot standalone.
    FastLED.addLeds<WS2812, pins::statusLed, GRB>(&led_, 1);
    FastLED.setBrightness(config::ledBrightness);
    setLed(CRGB::Orange);
    for (size_t i = 0; i < 4; ++i) {
        // PCB has active-HIGH buttons with external pulldowns.
        buttons_[i].attach(buttonPins[i], INPUT);
        buttons_[i].interval(config::buttonDebounceMs);
    }
    battery_.begin();
    audio_.begin();
    if (!config::simulation) barometerReady_ = barometer_.begin();
    ble_.begin();
    startedMs_ = millis();
    simulationStartMs_ = startedMs_;
    simulationUs_ = micros();
    Serial.printf("BeepTronics | mode=%s | flash=%u bytes | PSRAM=%u bytes\n",
                  config::simulation ? "SIMULATION" : "SENSOR",
                  ESP.getFlashChipSize(), ESP.getPsramSize());
    if (config::simulation || barometerReady_) {
        audio_.play(Sound::Startup, startedMs_);
    } else {
        setLed(CRGB::Red);
        Serial.println("MS5611 initialization/PROM CRC failed. Measurement disabled; buttons remain usable.");
        faultReported_ = true;
    }
}

void Firmware::setLed(const CRGB& color) {
    if (led_ == color) return;
    led_ = color;
    FastLED.show();
}

void Firmware::updateButtons(uint32_t nowMs) {
    for (auto& button : buttons_) button.update();
    if (shuttingDown_) return;
    // Keep actions on release, matching the existing hardware behavior.
    if (buttons_[0].fell()) {
        battery_.request();
        if (config::simulation) {
            simulationStartMs_ = nowMs;
            simulationUs_ = micros();
            simulationAltitude_ = 0;
            measurement_ = Measurement{};
            audio_.mute();
        }
    }
    if (buttons_[1].fell()) {
        audio_.adjustVolume(-config::volumeStep);
        audio_.play(Sound::VolumeDown, nowMs);
    }
    if (buttons_[2].fell()) {
        audio_.adjustVolume(config::volumeStep);
        audio_.play(Sound::VolumeUp, nowMs);
    }
    if (buttons_[3].fell()) {
        shuttingDown_ = true;
        audio_.play(Sound::Shutdown, nowMs);
    }
}

void Firmware::updateMeasurement(uint32_t nowUs) {
    if (config::simulation) {
        const uint32_t elapsed = nowUs - simulationUs_;
        if (elapsed < 20000) return;
        // Bounded 0..5 m/s demo, repeated every 26 seconds.
        const float climb = ((millis() - simulationStartMs_) / 1000 % 26) * 0.2f;
        simulationAltitude_ += climb * (elapsed * 1e-6f);
        if (simulationAltitude_ > 5000) simulationAltitude_ = 0;
        simulationUs_ = nowUs;
        measurement_.altitudeM = simulationAltitude_;
        measurement_.pressureHpa = config::seaLevelPressureHpa * std::pow(
            1.0f - simulationAltitude_ / 44330.0f, 1.0f / 0.1902949571836346f);
        measurement_.temperatureC = 20;
        measurement_.climbMps = climb;
        measurement_.timestampUs = nowUs;
        measurement_.valid = true;
    } else if (barometerReady_) {
        barometer_.startRead();
        if (barometer_.update()) {
            measurement_ = filter_.update(barometer_.getPressure(), barometer_.getTemperature(), micros());
        }
    }
}

void Firmware::update() {
    if (switchedOff_) return; // Also stay silent if USB/external hardware keeps power on.
    uint32_t nowMs = millis();
    updateButtons(nowMs);
    if (shuttingDown_) {
        audio_.update(0, false, nowMs);
        if (!audio_.busy()) {
            audio_.mute();
            setLed(CRGB::Black);
            digitalWrite(pins::sensorPower, LOW);
            digitalWrite(pins::batteryEnable, HIGH);
            digitalWrite(pins::powerLatch, LOW);
            switchedOff_ = true;
        }
        return;
    }
    updateMeasurement(micros());
    battery_.update(nowMs);
    nowMs = millis();
    if (measurement_.valid && uint32_t(micros() - measurement_.timestampUs) > config::staleMeasurementUs) {
        measurement_.valid = false;
        filter_.reset();
    }
    const bool warmedUp = uint32_t(nowMs - startedMs_) >= config::audioWarmupMs;
    const bool fault = !measurement_.valid && (warmedUp || (!config::simulation && !barometerReady_));
    if (fault && !faultReported_) Serial.println("Invalid/stale barometer data; vario sound and telemetry paused.");
    if (!fault && faultReported_ && measurement_.valid) Serial.println("Barometer measurements recovered.");
    faultReported_ = fault;
    audio_.update(measurement_.climbMps, measurement_.valid && (config::simulation || warmedUp), nowMs);
    ble_.update(measurement_, battery_.reading(), nowMs);
    setLed(fault ? CRGB::Red : (config::simulation ? CRGB::Purple :
           (warmedUp ? CRGB::Black : CRGB::Orange)));
}
