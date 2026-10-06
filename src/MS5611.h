#pragma once

// Author: Franz Hölzl
// Usage: SPI Library for MS5611 Pressure Sensor without a software delay (delayMicroseconds())
// Credits: Rob Tillaart https://github.com/RobTillaart/MS5611_SPI
// Version: 1               22.03.2023

#include "Arduino.h"
#include "SPI.h"

#define MS5611_SAMPLE_RATE 50

class MS5611
{
public:
    explicit MS5611(uint8_t select, uint8_t dataOut = 255, uint8_t dataIn = 255, uint8_t clock = 255);

    bool begin();

    //reset command + get constants
    //returns false if ROM constants == 0;
    bool reset();

    //  the actual reading of the sensor;
    //  returns MS5611_READ_OK upon success
    void startRead();

    bool update();

    //  temperature is in ²C
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
    uint8_t _startRead;
    unsigned long _convertStart;
    const uint16_t _CONVERT_DELAY = 9100;

    uint8_t _select;
    uint8_t _dataIn;
    uint8_t _dataOut;
    uint8_t _clock;
    uint32_t _SPIspeed = 1000000;

    SPIClass *mySPI;
    SPISettings _spi_settings;
};
