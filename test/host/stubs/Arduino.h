#pragma once
#include <cstdint>
constexpr int OUTPUT = 1, HIGH = 1, LOW = 0;
namespace fake {
extern uint32_t nowUs;
}
inline uint32_t micros() { return fake::nowUs; }
inline uint32_t millis() { return fake::nowUs / 1000; }
inline void yield() { ++fake::nowUs; }
inline void delayMicroseconds(uint32_t us) { fake::nowUs += us; }
inline void pinMode(uint8_t, int) {}
inline void digitalWrite(uint8_t, int) {}
