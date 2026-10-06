#include "core/VarioFilter.h"
#include "core/ClimbTone.h"
#include "core/Lk8ex1.h"
#include "drivers/MS5611.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

namespace fake {
uint32_t nowUs = 0;
// MS5611 datasheet example coefficients; PROM fixture has CRC nibble 0.
uint16_t prom[8] = {0, 40127, 36924, 23317, 23282, 33464, 28312, 0};
uint32_t d1 = 9085466, d2 = 8569150;
int miso = -1, mosi = -1;
}

int assertions = 0;
void check(bool condition, const char* message) {
    ++assertions;
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}
bool near(float a, float b, float tolerance) { return std::fabs(a-b) <= tolerance; }
float pressureAt(float altitude) {
    return 1013.25f * std::pow(1-altitude/44330.0f, 1/0.1902949571836346f);
}

void testFilter() {
    VarioFilter filter;
    check(!filter.update(1013.25f,20,0).valid, "first sample cannot provide velocity");
    auto m = filter.update(1013.25f,20,20000);
    check(m.valid && near(m.climbMps,0,0.001f), "stationary instrument");
    check(!filter.update(1013.25f,20,20000).valid, "duplicate timestamp");
    check(!filter.update(0,20,40000).valid, "zero pressure rejected");
    check(!filter.update(std::numeric_limits<float>::quiet_NaN(),20,60000).valid, "NaN rejected");
    check(!filter.update(1013.25f,20,80000).valid, "invalid sample resets history");
    for (uint32_t stepUs : {10000u, 20000u, 35000u}) {
        filter.reset();
        uint32_t time = 0;
        for (int i=0; i<1000; ++i) {
            m = filter.update(pressureAt(time*1e-6f*2),20,time);
            time += stepUs;
        }
        check(m.valid && near(m.climbMps,2,0.04f), "2 m/s independent of sample rate");
    }
    filter.reset();
    uint32_t time = 0;
    for (int i=0; i<1000; ++i) {
        m = filter.update(pressureAt(-time*1e-6f*1.5f),20,time);
        time += i%2 ? 17000 : 29000;
    }
    check(near(m.climbMps,-1.5f,0.05f), "sinking with irregular timing");
    check(!filter.update(900,20,time+1000000).valid, "long gap reinitializes altitude");
    m = filter.update(900,20,time+1020000);
    check(m.valid && near(m.climbMps,0,0.001f), "no spurious lift after gap");
    filter.reset();
    filter.update(1013.25f,20,0xFFFFFF00u);
    m = filter.update(1013.25f,20,uint32_t(0xFFFFFF00u+20000u));
    check(m.valid && m.climbMps==0, "micros rollover");
}

void testTone() {
    ClimbTone tone;
    check(tone.update(0.2f,0)==0, "below entry threshold is silent");
    check(tone.update(0.3f,0)>0, "climb starts tone");
    check(tone.update(0.2f,600)==0, "hysteresis zone still advances to pause");
    check(tone.update(0.2f,1200)>0, "hysteresis zone still advances to tone");
    check(tone.update(0.1f,1201)==0, "off threshold silences immediately");
    check(tone.update(0.2f,1202)==0, "must cross entry threshold again");
    check(tone.update(-2,1300)==0, "no sink tone in this version");
    check(tone.update(10000,1400)<3000, "simulation/outlier frequency bounded");
    check(tone.update(std::numeric_limits<float>::quiet_NaN(),1401)==0, "invalid lift silences");
    check(tone.update(1,0xFFFFFF00u)>0, "tone before millis rollover");
    check(tone.update(1,uint32_t(0xFFFFFF00u+300u))==0, "tone phase across rollover");
}

void testProtocol() {
    Measurement m;
    m.valid=true; m.pressureHpa=1013.25f; m.climbMps=-1.25f; m.temperatureC=20;
    BatteryReading battery;
    battery.valid=true; battery.percent=75;
    char output[96];
    const size_t length=formatLk8ex1(output,sizeof(output),m,battery);
    check(length==std::strlen(output), "sentence length");
    check(std::strncmp(output,"$LK8EX1,101325,99999,-125,20,1075,*",33)==0, "units, sign and battery encoding");
    const char* star=std::strchr(output,'*');
    unsigned checksum=0;
    for (const char* p=output+1; p<star; ++p) checksum ^= static_cast<unsigned char>(*p);
    check(std::strtoul(star+1,nullptr,16)==checksum, "NMEA checksum");
    check(std::strcmp(output+length-2,"\r\n")==0, "CRLF terminator");
    char small[4]={'x','x','x','x'};
    check(formatLk8ex1(small,sizeof(small),m,battery)==0 && small[0]==0, "short buffer rejected");
    battery.valid=false;
    formatLk8ex1(output,sizeof(output),m,battery);
    check(std::strstr(output,",20,999,*")!=nullptr, "unknown battery sentinel");
    m.pressureHpa=101325;
    check(formatLk8ex1(output,sizeof(output),m,battery)==0, "old Pa/hPa initialization bug rejected");
    m.pressureHpa=1013.25f; m.valid=false;
    check(formatLk8ex1(output,sizeof(output),m,battery)==0, "no invalid telemetry");
}

void testDriver() {
    MS5611 sensor(14,13,12,11); // Local object also needs initialized state.
    check(sensor.begin(), "datasheet PROM fixture accepted");
    check(fake::miso==13 && fake::mosi==12, "actual controller SPI directions");
    sensor.startRead();
    check(!sensor.update(), "conversion does not complete immediately");
    fake::nowUs += 9100;
    check(!sensor.update(), "pressure conversion followed by temperature");
    fake::nowUs += 9100;
    check(sensor.update(), "both conversions ready");
    check(near(sensor.getTemperature(),20.07f,0.02f), "datasheet temperature example");
    check(near(sensor.getPressure(),1000.09f,0.05f), "datasheet pressure example");
    sensor.startRead(); fake::d1=0;
    fake::nowUs += 9100; sensor.update();
    fake::nowUs += 9100;
    check(!sensor.update(), "disconnected ADC does not become a valid sample");
    fake::d1=9085466;
    fake::prom[1] ^= 1;
    check(!sensor.reset(), "corrupt calibration fails CRC");
    fake::prom[1] ^= 1;
    check(sensor.reset(), "reset after restored calibration");
    fake::nowUs=0xFFFFFF00u;
    sensor.startRead(); fake::nowUs += 9100; sensor.update();
    fake::nowUs += 9100;
    check(sensor.update(), "conversion works across micros rollover");
}

int main() {
    testFilter(); testTone(); testProtocol(); testDriver();
    std::cout << "PASS: " << assertions << " assertions (filter, audio, LK8EX1, MS5611)\n";
}
