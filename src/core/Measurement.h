#pragma once
#include <cstdint>

struct Measurement {
    float pressureHpa = 0;
    float temperatureC = 0;
    float altitudeM = 0;
    float climbMps = 0;
    uint32_t timestampUs = 0;
    bool valid = false;
};

struct BatteryReading {
    float voltageV = 0;
    uint8_t percent = 0;
    bool valid = false;
};
