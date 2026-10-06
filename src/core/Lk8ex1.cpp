#include "Lk8ex1.h"
#include <cmath>
#include <cstdio>

size_t formatLk8ex1(char* output, size_t capacity, const Measurement& m,
                   const BatteryReading& battery) {
    if (!output || capacity == 0) return 0;
    output[0] = '\0';
    if (!m.valid || !std::isfinite(m.pressureHpa) || m.pressureHpa < 10 ||
        m.pressureHpa > 1200 || !std::isfinite(m.climbMps) || std::fabs(m.climbMps) > 100 ||
        !std::isfinite(m.temperatureC) || m.temperatureC < -40 || m.temperatureC > 85) return 0;
    char body[80];
    const unsigned batteryField = battery.valid && battery.percent <= 100 ? 1000 + battery.percent : 999;
    const int length = std::snprintf(body, sizeof(body), "LK8EX1,%ld,99999,%ld,%ld,%u,",
        std::lround(m.pressureHpa * 100), std::lround(m.climbMps * 100),
        std::lround(m.temperatureC), batteryField);
    if (length < 0 || static_cast<size_t>(length) >= sizeof(body)) return 0;
    uint8_t checksum = 0;
    for (int i = 0; i < length; ++i) checksum ^= static_cast<uint8_t>(body[i]);
    if (capacity < static_cast<size_t>(length) + 7) return 0;
    const int written = std::snprintf(output, capacity, "$%s*%02X\r\n", body, checksum);
    return written > 0 ? static_cast<size_t>(written) : 0;
}
