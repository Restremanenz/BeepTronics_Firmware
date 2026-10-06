#pragma once

// Author: Franz Hölzl
// Usage: SPI Library for MS5611 Pressure Sensor without a software delay (delayMicroseconds())
// Credits: Rob Tillaart https://github.com/RobTillaart/MS5611_SPI
// Version: 1               22.03.2023

#include "Arduino.h"
#include "SPI.h"

class MS5611
{
public:
    explicit MS5611(uint8_t select, uint8_t miso, uint8_t mosi, uint8_t clock);
    MS5611(const MS5611&) = delete;
    MS5611& operator=(const MS5611&) = delete;

    bool begin();

    //reset command + get constants
    // Returns false for invalid calibration words or a PROM CRC mismatch.
    bool reset();

    //  the actual reading of the sensor;
    //  Starts a conversion if the driver is idle.
    void startRead();

    bool update();

    // Temperature in degrees Celsius.
    float getTemperature() const;

    //  pressure is in mBar
    float getPressure() const;

    //  OFFSET
    void setPressureOffset(float offset = 0);
    void setTemperatureOffset(float offset = 0);

    //  last time in millis() when the sensor has been read.
    uint32_t lastRead() const;

    //       speed in Hz
    void setSPIspeed(uint32_t speed);

protected:
    uint32_t readADC();
    uint16_t readProm(uint8_t reg);
    int sendCommand(const uint8_t command);
    void initConstants();
    void calculateValues(uint32_t D1, uint32_t D2);

    int32_t _temperature;
    int32_t _pressure;
    float _pressureOffset;
    float _temperatureOffset;
    int _result;
    float C[7];
    uint32_t _lastRead;
    uint8_t _startRead = 0;
    uint32_t _convertStart = 0;
    uint32_t _rawPressure = 0;
    const uint16_t _CONVERT_DELAY = 9100;

    uint8_t _select;
    uint8_t _dataIn;
    uint8_t _dataOut;
    uint8_t _clock;
    uint32_t _SPIspeed = 1000000;

    SPIClass _spi{FSPI};
    SPIClass *mySPI = &_spi;
    SPISettings _spi_settings;
};
