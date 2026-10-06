#pragma once
#include <cstddef>
#include "Measurement.h"

// Returns complete sentence length, or 0 for invalid data / insufficient space.
size_t formatLk8ex1(char* output, size_t capacity, const Measurement& measurement,
                   const BatteryReading& battery);
