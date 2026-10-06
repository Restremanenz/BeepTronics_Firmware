#include "VarioFilter.h"
#include <cmath>

void VarioFilter::reset() {
    initialized_ = false;
    history_.fill(0);
    next_ = 0;
    sum_ = 0;
}

Measurement VarioFilter::update(float pressureHpa, float temperatureC, uint32_t timestampUs) {
    Measurement result;
    if (!std::isfinite(pressureHpa) || pressureHpa < 10 || pressureHpa > 1200 ||
        !std::isfinite(temperatureC) || temperatureC < -40 || temperatureC > 85) {
        reset();
        return result;
    }
    result.pressureHpa = pressureHpa;
    result.temperatureC = temperatureC;
    result.timestampUs = timestampUs;
    const float rawAltitude = 44330.0f * (1.0f - std::pow(
        pressureHpa / config::seaLevelPressureHpa, 0.1902949571836346f));
    const uint32_t elapsed = timestampUs - previousUs_;
    if (!initialized_ || elapsed > config::staleMeasurementUs) {
        reset();
        altitude_ = rawAltitude;
        previousUs_ = timestampUs;
        initialized_ = true;
        result.altitudeM = altitude_;
        return result;
    }
    if (elapsed == 0) return result;
    const float dt = elapsed * 1e-6f;
    const float weight = 1.0f - std::pow(1.0f - config::altitudeWeight,
                                       dt / config::referenceSampleSeconds);
    const float filtered = altitude_ + weight * (rawAltitude - altitude_);
    float slope = (filtered - altitude_) / dt;
    if (std::fabs(slope) < config::slopeNoiseMps) slope = 0;
    sum_ += slope - history_[next_];
    history_[next_] = slope;
    next_ = (next_ + 1) % history_.size();
    altitude_ = filtered;
    previousUs_ = timestampUs;
    result.altitudeM = altitude_;
    result.climbMps = sum_ / history_.size();
    result.valid = true;
    return result;
}
