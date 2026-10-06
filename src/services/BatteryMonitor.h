#pragma once
#include "core/Measurement.h"

class BatteryMonitor {
public:
    void begin();
    void request() { requested_ = true; }
    void update(uint32_t nowMs);
    const BatteryReading& reading() const { return reading_; }
private:
    BatteryReading reading_;
    bool requested_ = true;
    bool sampling_ = false;
    uint32_t startedMs_ = 0;
    uint32_t previousMs_ = 0;
    uint32_t sumMv_ = 0;
    uint16_t samples_ = 0;
};
