#pragma once
#include <array>
#include "Config.h"
#include "Measurement.h"

class VarioFilter {
public:
    Measurement update(float pressureHpa, float temperatureC, uint32_t timestampUs);
    void reset();
private:
    bool initialized_ = false;
    float altitude_ = 0;
    uint32_t previousUs_ = 0;
    std::array<float, config::slopeHistorySize> history_{};
    size_t next_ = 0;
    float sum_ = 0;
};
