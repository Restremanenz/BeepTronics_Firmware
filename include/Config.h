#pragma once
#include <cstddef>
#include <cstdint>

#ifndef BEEPTRONICS_SIMULATION
#define BEEPTRONICS_SIMULATION 0
#endif

namespace config {
constexpr bool simulation = BEEPTRONICS_SIMULATION != 0;
constexpr uint8_t ledBrightness = 20;
constexpr uint32_t buttonDebounceMs = 25;
constexpr uint32_t audioWarmupMs = 10000;
constexpr uint32_t telemetryIntervalMs = 100;
constexpr uint32_t staleMeasurementUs = 250000;
constexpr float seaLevelPressureHpa = 1013.25f;
// Original 0.08 IIR weight at 20 ms, scaled to actual sample intervals.
constexpr float altitudeWeight = 0.08f;
constexpr float referenceSampleSeconds = 0.02f;
constexpr size_t slopeHistorySize = 35;
constexpr float slopeNoiseMps = 0.1f;
constexpr float climbOnMps = 0.25f;
constexpr float climbOffMps = 0.1f;
constexpr uint16_t volumeDefault = 20;
constexpr uint16_t volumeMax = 100;
constexpr int volumeStep = 5;
constexpr uint8_t speakerChannel = 0;
constexpr uint8_t speakerResolution = 8;
constexpr uint32_t batteryIntervalMs = 50000;
constexpr uint16_t batterySamples = 400;
constexpr uint16_t batteryEmptyMv = 3500;
constexpr uint16_t batteryFullMv = 3950;
static_assert(climbOnMps > climbOffMps, "Invalid climb hysteresis");
static_assert(slopeHistorySize > 0, "Empty averaging window");
}
