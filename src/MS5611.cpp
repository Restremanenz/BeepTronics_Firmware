// Author: Franz Hölzl
// Usage: SPI Library for MS5611 Pressure Sensor without a software delay (delayMicroseconds())
// Credits: Rob Tillaart https://github.com/RobTillaart/MS5611_SPI
// Version: 1               22.03.2023
// Datasheet: https://www.te.com/commerce/DocumentDelivery/DDEController?Action=showdoc&DocId=Data+Sheet%7FMS5611-01BA03%7FB3%7Fpdf%7FEnglish%7FENG_DS_MS5611-01BA03_B3.pdf%7FCAT-BLPS0036

#include "MS5611.h"

// datasheet page 10
#define MS5611_CMD_READ_ADC 0x00
#define MS5611_CMD_READ_PROM 0xA0
#define MS5611_CMD_RESET 0x1E
#define MS5611_CMD_CONVERT_D1 0x48
#define MS5611_CMD_CONVERT_D2 0x58

/////////////////////////////////////////////////////
//
//  PUBLIC
//
MS5611::MS5611(uint8_t select, uint8_t dataOut, uint8_t dataIn, uint8_t clock)
{

    _temperature = -999;
    _pressure = -999;
    _result = -999;
    _lastRead = 0;
    _pressureOffset = 0;
    _temperatureOffset = 0;

    //  SPI
    _select = select;
    _dataIn = dataOut;
    _dataOut = dataIn;
    _clock = clock;
}

bool MS5611::begin()
{
    // Start SPI
    pinMode(_select, OUTPUT);
    digitalWrite(_select, HIGH);

    setSPIspeed(_SPIspeed);
    mySPI = new SPIClass(FSPI);
    mySPI->end();
    mySPI->begin(_clock, _dataOut, _dataIn, _select);

    // Reset device and return connection Status
    return reset();
}

bool MS5611::reset()
{
    sendCommand(MS5611_CMD_RESET);
    uint32_t start = micros();
    // waiting at least 2.8ms after Reset
    //  yield loop prevents blocking RTOS
    while (micros() - start < 3000) //  increased as first ROM values were missed.
    {
        yield();
        delayMicroseconds(10);
    }

    //  initialize the C[] array
    initConstants();

    // read factory calibrations from EEPROM.
    bool ROM_OK = true;
    for (uint8_t reg = 0; reg < 7; reg++)
    {
        //  used indices match datasheet.
        //  C[0] == manufacturer - read but not used;
        //  C[7] == CRC - skipped.
        uint16_t tmp = readProm(reg);
        C[reg] *= tmp;

        // check if data was recived
        if (reg > 0)
        {
            ROM_OK = ROM_OK && (tmp != 0);
        }
    }
    return ROM_OK;
}

bool MS5611::update()
{
    uint32_t start = micros();
    static uint32_t D1 = 0, D2 = 0;

    if (_startRead && start - _convertStart >= _CONVERT_DELAY)
    {
        _startRead = _startRead < 2 ? _startRead + 1 : 0;

        if (_startRead == 2)
        {
            D1 = readADC();
            sendCommand(MS5611_CMD_CONVERT_D2);
            _convertStart = start;
            return false;
        }
        else if (_startRead == 0)
        {
            D2 = readADC();
            calculateValues(D1, D2);
            return true;
        }
    }

    return false;
}

void MS5611::startRead()
{
    if (_startRead != 0) return;
    _startRead = 1;
    sendCommand(MS5611_CMD_CONVERT_D1);
    _convertStart = micros();
}

float MS5611::getTemperature() const
{
    if (_temperatureOffset == 0)
        return _temperature * 0.01;
    return _temperature * 0.01 + _temperatureOffset;
};

float MS5611::getPressure() const
{
    if (_pressureOffset == 0)
        return _pressure * 0.01;
    return _pressure * 0.01 + _pressureOffset;
};

void MS5611::setPressureOffset(float offset)
{
    _pressureOffset = offset;
};

void MS5611::setTemperatureOffset(float offset)
{
    _temperatureOffset = offset;
};

uint32_t MS5611::lastRead() const
{
    return _lastRead;
};

void MS5611::setSPIspeed(uint32_t speed)
{
    _SPIspeed = speed;
    _spi_settings = SPISettings(_SPIspeed, MSBFIRST, SPI_MODE0);
};

/////////////////////////////////////////////////////
//
//  PRIVATE
//
uint16_t MS5611::readProm(uint8_t reg)
{
    //  last EEPROM register is CRC - Page 13 datasheet.
    uint8_t promCRCRegister = 7;
    if (reg > promCRCRegister)
        return 0;

    uint16_t value = 0;
    digitalWrite(_select, LOW);

    mySPI->beginTransaction(_spi_settings);
    mySPI->transfer(MS5611_CMD_READ_PROM + reg * 2);
    value += mySPI->transfer(0x00);
    value <<= 8;
    value += mySPI->transfer(0x00);
    mySPI->endTransaction();

    digitalWrite(_select, HIGH);
    return value;
}

uint32_t MS5611::readADC()
{
    uint32_t value = 0;

    digitalWrite(_select, LOW);
    mySPI->beginTransaction(_spi_settings);
    mySPI->transfer(0x00);
    value += mySPI->transfer(0x00);
    value <<= 8;
    value += mySPI->transfer(0x00);
    value <<= 8;
    value += mySPI->transfer(0x00);
    mySPI->endTransaction();
    digitalWrite(_select, HIGH);

    return value;
}

int MS5611::sendCommand(const uint8_t command)
{
    digitalWrite(_select, LOW);
    mySPI->beginTransaction(_spi_settings);
    mySPI->transfer(command);
    mySPI->endTransaction();
    digitalWrite(_select, HIGH);

    return 0;
}

void MS5611::initConstants()
{
    //  constants that were multiplied in read() - datasheet page 8
    //  do this once and you save CPU cycles
    //
    //                               datasheet ms5611     |    appNote
    //                                mode = 0;           |    mode = 1
    C[0] = 1;
    C[1] = 32768L;          //  SENSt1   = C[1] * 2^15    |    * 2^16
    C[2] = 65536L;          //  OFFt1    = C[2] * 2^16    |    * 2^17
    C[3] = 3.90625E-3;      //  TCS      = C[3] / 2^8     |    / 2^7
    C[4] = 7.8125E-3;       //  TCO      = C[4] / 2^7     |    / 2^6
    C[5] = 256;             //  Tref     = C[5] * 2^8     |    * 2^8
    C[6] = 1.1920928955E-7; //  TEMPSENS = C[6] / 2^23    |    / 2^23
}

void MS5611::calculateValues(uint32_t D1, uint32_t D2)
{
    //  TEMP & PRESS MATH - PAGE 7/20
    float dT = D2 - C[5];
    _temperature = 2000 + dT * C[6];

    float offset = C[2] + dT * C[4];
    float sens = C[1] + dT * C[3];

    if (_temperature < 2000)
    {
        float T2 = dT * dT * 4.6566128731E-10;
        float t = (_temperature - 2000) * (_temperature - 2000);
        float offset2 = 2.5 * t;
        float sens2 = 1.25 * t;
        //  COMMENT OUT < -1500 CORRECTION IF NOT NEEDED
        if (_temperature < -1500)
        {
            t = (_temperature + 1500) * (_temperature + 1500);
            offset2 += 7 * t;
            sens2 += 5.5 * t;
        }
        _temperature -= T2;
        offset -= offset2;
        sens -= sens2;
    }
    //  END SECOND ORDER COMPENSATION

    _pressure = (D1 * sens * 4.76837158205E-7 - offset) * 3.051757813E-5;

    _lastRead = millis();
}