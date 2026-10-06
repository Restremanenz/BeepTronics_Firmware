#pragma once
#include <cstddef>
#include "core/ClimbTone.h"
#include "Config.h"

enum class Sound { Startup, Shutdown, VolumeUp, VolumeDown };

class AudioOutput {
public:
    void begin();
    void play(Sound sound, uint32_t nowMs);
    void update(float climbMps, bool measurementValid, uint32_t nowMs);
    void adjustVolume(int delta);
    bool busy() const { return notes_ != nullptr; }
    void mute();
    uint16_t volume() const { return volume_; }
    struct Note { uint16_t frequency; uint16_t durationMs; };
private:
    void write(uint16_t frequency);
    ClimbTone tone_;
    uint16_t volume_ = config::volumeDefault;
    uint16_t frequency_ = 0;
    uint16_t appliedVolume_ = 0;
    const Note* notes_ = nullptr;
    size_t noteCount_ = 0;
    size_t noteIndex_ = 0;
    uint32_t noteStartedMs_ = 0;
};
