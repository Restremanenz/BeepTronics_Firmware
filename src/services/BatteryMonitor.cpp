#include "BatteryMonitor.h"
#include "BoardPins.h"
#include "Config.h"
#include <Arduino.h>
#include <algorithm>

void BatteryMonitor::begin() {
    digitalWrite(pins::batteryEnable, HIGH);
    pinMode(pins::batteryEnable, OUTPUT);
    pinMode(pins::batteryAdc, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(pins::batteryAdc, ADC_11db);
}

void BatteryMonitor::update(uint32_t nowMs) {
    if (!sampling_) {
        if (!requested_ && nowMs - previousMs_ < config::batteryIntervalMs) return;
        requested_ = false;
        sampling_ = true;
        samples_ = 0;
        sumMv_ = 0;
        startedMs_ = nowMs;
        digitalWrite(pins::batteryEnable, LOW);
        return;
    }
    if (nowMs - startedMs_ < 2) return;
    // The capacitor-coupled enable is a finite measurement window, not a latch.
    if (nowMs - startedMs_ > 100) {
        reading_.valid = false;
        digitalWrite(pins::batteryEnable, HIGH);
        sampling_ = false;
        previousMs_ = nowMs;
        return;
    }
    // Bound work per loop so the barometer and audio continue to run.
    for (uint8_t i = 0; i < 16 && samples_ < config::batterySamples; ++i, ++samples_) {
        sumMv_ += analogReadMilliVolts(pins::batteryAdc);
    }
    if (samples_ < config::batterySamples) return;
    digitalWrite(pins::batteryEnable, HIGH);
    sampling_ = false;
    previousMs_ = nowMs;
    const float millivolts = 2.0f * sumMv_ / samples_;
    reading_.voltageV = millivolts / 1000;
    reading_.valid = millivolts >= 2500 && millivolts <= 4500;
    const float percent = 100 * (millivolts - config::batteryEmptyMv) /
                          (config::batteryFullMv - config::batteryEmptyMv);
    reading_.percent = static_cast<uint8_t>(std::max(0.0f, std::min(100.0f, percent)));
}
