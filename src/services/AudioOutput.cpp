#include "AudioOutput.h"
#include "BoardPins.h"
#include <Arduino.h>
#include <algorithm>

namespace {
const AudioOutput::Note startup[] = {{523,80}, {659,80}, {784,80}, {1046,200}};
const AudioOutput::Note shutdown[] = {{1046,80}, {784,80}, {659,80}, {523,200}};
const AudioOutput::Note up[] = {{659,80}};
const AudioOutput::Note down[] = {{523,80}};
}

void AudioOutput::begin() {
    ledcSetup(config::speakerChannel, 1000, config::speakerResolution);
    ledcAttachPin(pins::speaker, config::speakerChannel);
    ledcWrite(config::speakerChannel, 0);
}

void AudioOutput::write(uint16_t frequency) {
    if (frequency == frequency_ && volume_ == appliedVolume_) return;
    frequency_ = frequency;
    appliedVolume_ = volume_;
    if (frequency && volume_) {
        ledcWriteTone(config::speakerChannel, frequency);
        ledcWrite(config::speakerChannel, volume_);
    } else {
        ledcWrite(config::speakerChannel, 0);
    }
}

void AudioOutput::adjustVolume(int delta) {
    volume_ = static_cast<uint16_t>(std::max(0, std::min(int(config::volumeMax), int(volume_) + delta)));
}

void AudioOutput::play(Sound sound, uint32_t nowMs) {
    tone_.reset();
    switch (sound) {
        case Sound::Startup: notes_ = startup; noteCount_ = 4; break;
        case Sound::Shutdown: notes_ = shutdown; noteCount_ = 4; break;
        case Sound::VolumeUp: notes_ = up; noteCount_ = 1; break;
        case Sound::VolumeDown: notes_ = down; noteCount_ = 1; break;
    }
    noteIndex_ = 0;
    noteStartedMs_ = nowMs;
    write(notes_[0].frequency);
}

void AudioOutput::update(float climbMps, bool measurementValid, uint32_t nowMs) {
    if (notes_) {
        if (nowMs - noteStartedMs_ < notes_[noteIndex_].durationMs) return;
        if (++noteIndex_ < noteCount_) {
            noteStartedMs_ = nowMs;
            write(notes_[noteIndex_].frequency);
            return;
        }
        notes_ = nullptr;
        write(0);
    }
    if (!measurementValid) {
        tone_.reset();
        write(0);
        return;
    }
    write(tone_.update(climbMps, nowMs));
}

void AudioOutput::mute() {
    notes_ = nullptr;
    tone_.reset();
    write(0);
}
