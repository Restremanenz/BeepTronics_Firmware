#include "ClimbTone.h"
#include "Config.h"
#include <algorithm>
#include <cmath>

void ClimbTone::reset() {
    climbing_ = false;
    sounding_ = false;
    phaseDurationMs_ = 0;
}

uint16_t ClimbTone::update(float climbMps, uint32_t nowMs) {
    if (!std::isfinite(climbMps) || climbMps <= config::climbOffMps) {
        reset();
        return 0;
    }
    const float bounded = std::max(config::climbOnMps, std::min(climbMps, 15.0f));
    const auto duration = static_cast<uint32_t>(std::max(60.0f, std::min(1000.0f,
        200.0f / std::sqrt(bounded - 0.1f) - 10.0f)));
    if (!climbing_) {
        if (climbMps < config::climbOnMps) return 0;
        climbing_ = true;
        sounding_ = true;
        phaseStartMs_ = nowMs;
        phaseDurationMs_ = duration;
    } else if (nowMs - phaseStartMs_ >= phaseDurationMs_) {
        sounding_ = !sounding_;
        phaseStartMs_ = nowMs;
        phaseDurationMs_ = duration;
    }
    return sounding_ ? static_cast<uint16_t>(bounded * bounded * 10.24f + 565.0f) : 0;
}
