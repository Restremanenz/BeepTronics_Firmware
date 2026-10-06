#pragma once
#include <cstdint>

// Controller-side signal directions, verified against Schmatic.pdf (Vario V2).
namespace pins {
constexpr uint8_t statusLed = 4;
constexpr uint8_t sensorPower = 7; // Shared GPS + IMU LDO, active HIGH.
constexpr uint8_t powerLatch = 45;
constexpr uint8_t speaker = 3;
constexpr uint8_t i2cSda = 6;
constexpr uint8_t i2cScl = 5;
constexpr uint8_t buttonOn = 17;
constexpr uint8_t buttonDown = 40;
constexpr uint8_t buttonUp = 41;
constexpr uint8_t buttonOk = 42;
constexpr uint8_t batteryAdc = 1;
constexpr uint8_t batteryEnable = 2;
constexpr uint8_t barometerCs = 14;
constexpr uint8_t spiClock = 11;
constexpr uint8_t barometerMosi = 12; // Old net name MISO; goes to sensor SDI.
constexpr uint8_t barometerMiso = 13; // Old net name MOSI; comes from sensor SDO.
constexpr uint8_t displayCs = 10;
constexpr uint8_t displayData = 12;
constexpr uint8_t displayDc = 13; // Shared with barometer MISO; not a normal shared SPI bus!
constexpr uint8_t displayBusy = 18;
constexpr uint8_t displayReset = 8;
constexpr uint8_t gpsExternalInterrupt = 21; // Input of the GNSS module.
constexpr uint8_t gpsReset = 47;
constexpr uint8_t imuInterrupt = 48;
}
