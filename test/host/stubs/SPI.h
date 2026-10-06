#pragma once
#include <cstdint>
constexpr int FSPI = 1, MSBFIRST = 1, SPI_MODE0 = 0;
namespace fake {
extern uint16_t prom[8];
extern uint32_t d1, d2;
extern int miso, mosi;
}
struct SPISettings {
    SPISettings(uint32_t = 0, int = 0, int = 0) {}
};
class SPIClass {
public:
    explicit SPIClass(int) {}
    void end() {}
    void begin(int, int miso, int mosi, int) { fake::miso = miso; fake::mosi = mosi; }
    void beginTransaction(SPISettings) { index_ = 0; }
    void endTransaction() {}
    uint8_t transfer(uint8_t value) {
        if (index_++ == 0) {
            command_ = value;
            if (value == 0x48 || value == 0x58) conversion_ = value;
            return 0;
        }
        if (command_ >= 0xA0 && command_ <= 0xAE) {
            const auto word = fake::prom[(command_ - 0xA0) / 2];
            return index_ == 2 ? word >> 8 : word & 255;
        }
        if (command_ == 0) {
            const auto adc = conversion_ == 0x48 ? fake::d1 : fake::d2;
            return (adc >> (8 * (4 - index_))) & 255;
        }
        return 0;
    }
private:
    uint8_t command_ = 0, conversion_ = 0, index_ = 0;
};
