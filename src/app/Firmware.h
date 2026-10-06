#pragma once
#include <Bounce2.h>
#include <FastLED.h>
#include "BoardPins.h"
#include "core/VarioFilter.h"
#include "drivers/MS5611.h"
#include "services/AudioOutput.h"
#include "services/BatteryMonitor.h"
#include "services/BleTelemetry.h"

class Firmware {
public:
    void begin();
    void update();
private:
    void updateButtons(uint32_t nowMs);
    void updateMeasurement(uint32_t nowUs);
    void setLed(const CRGB& color);
    MS5611 barometer_{pins::barometerCs, pins::barometerMiso, pins::barometerMosi, pins::spiClock};
    VarioFilter filter_;
    Measurement measurement_;
    AudioOutput audio_;
    BatteryMonitor battery_;
    BleTelemetry ble_;
    Bounce buttons_[4];
    CRGB led_ = CRGB::Black;
    bool barometerReady_ = false;
    bool shuttingDown_ = false;
    bool switchedOff_ = false;
    bool faultReported_ = false;
    uint32_t startedMs_ = 0;
    uint32_t simulationUs_ = 0;
    uint32_t simulationStartMs_ = 0;
    float simulationAltitude_ = 0;
};
