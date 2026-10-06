#include "util.h"
#include "Pinout.h"

uint8_t get_battery_percentage(void)
{
    const uint16_t BATTERY_SAMPLES = 400;
    const uint16_t BATTERY_MAX  = 3950; 
    const uint16_t BATTERY_MIN  = 3500;

    uint64_t sum = 0;

    digitalWrite(BAT_EN, LOW); 
    delay(2);

    for (uint32_t i = 0; i < BATTERY_SAMPLES; i++) {
        uint16_t v = analogReadMilliVolts(BAT);
        sum += (uint64_t)v;
    }
    float percentage = 2.f * (float)sum / BATTERY_SAMPLES;
    percentage = (percentage - BATTERY_MIN) * 100.f / (BATTERY_MAX - BATTERY_MIN);
    percentage = CLAMP(percentage, 0, 100);
    digitalWrite(BAT_EN, HIGH);

    return (uint8_t)percentage;
}

float get_battery_voltage(void)
{
    const uint16_t BATTERY_SAMPLES = 400;
    const uint16_t BATTERY_MAX  = 4000; 
    const uint16_t BATTERY_MIN  = 3500;

    uint64_t sum = 0;

    digitalWrite(BAT_EN, LOW); 
    delay(2);

    for (uint32_t i = 0; i < BATTERY_SAMPLES; i++) {
        uint16_t v = analogReadMilliVolts(BAT);
        sum += (uint64_t)v;
    }
    
    digitalWrite(BAT_EN, HIGH);

    return 2.f * (float)sum / BATTERY_SAMPLES/1000.f;
}

void shift_float_array(float *arr, size_t size, float val)
{
    for (size_t i = 1; i < size; i++) {
        arr[i] = arr[i - 1];
    }
    arr[0] = val;
}

float pressure_to_altitude(float pressure, float temperature)
{
// from https://en.wikipedia.org/wiki/Barometric_formula
// and  https://www.mide.com/air-pressure-at-altitude-calculator
#define Pb      1013.25    // static pressure in mBar
#define Tb      288.15     // standard temperature in Kelvin
#define Lb      0.0065     // standard temperature lapse in K/m
#define hb      0.0        // standard height in m
#define R       8.3144598  // universal gas constant in J/molK
#define g0      9.80665    // gravitational acceleration in m/s²
#define M       0.0289644  // molar mass of earth's air in kg/mol 
    // return (1.0 - pow((pressure / 1013.25), 0.190284)) * (temperature + 273.15) / 0.0065;
    return (44330.0 * (1.0 - pow(pressure / 1013.25, 0.1902949571836346)));
    // return (hb + Tb / Lb * (pow(pressure / Pb, -(R * Lb) / (g0 * M)) - 1));
}

uint16_t lift_to_beep_freq(float lift)
{
    return (uint16_t)((lift * 3.2f) * (lift * 3.2f) + 565.f);
}

uint16_t lift_to_beep_length(float lift)
{
    return (uint16_t)((200.f / sqrtf(lift - 0.1)) - 10.f);
}