#pragma once
#include "core/Measurement.h"

class NimBLEServer;
class NimBLECharacteristic;

class BleTelemetry {
public:
    void begin();
    void update(const Measurement& measurement, const BatteryReading& battery, uint32_t nowMs);
private:
    NimBLEServer* server_ = nullptr;
    NimBLECharacteristic* tx_ = nullptr;
    uint32_t previousMs_ = 0;
};
