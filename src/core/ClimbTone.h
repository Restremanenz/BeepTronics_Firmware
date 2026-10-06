#pragma once
#include <cstdint>

// Pure state machine: returns frequency in Hz; 0 means silence.
class ClimbTone {
public:
    uint16_t update(float climbMps, uint32_t nowMs);
    void reset();
private:
    bool climbing_ = false;
    bool sounding_ = false;
    uint32_t phaseStartMs_ = 0;
    uint32_t phaseDurationMs_ = 0;
};
