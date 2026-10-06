#include "Arduino.h"

#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define SAMPLE_RATE 50.0 // per second

#define LOG_MSG(fmt, ...) (Serial.printf(fmt "\r\n", ##__VA_ARGS__)/*(void)fmt*/)
#define CLAMP(value, min, max) ((value) > (max) ? (max) : ((value) < (min) ? (min) : (value)))
#define BEEP(freq) do {                     \
        ledcWriteTone(LEDC_SPEAKER, freq);  \
        ledcWrite(LEDC_SPEAKER, volume);    \
    } while(0)
#define MUTE() (ledcWrite(LEDC_SPEAKER, false))

uint8_t get_battery_percentage(void);
float get_battery_voltage(void);
void shift_float_array(float *arr, size_t size, float val);
float pressure_to_altitude(float pressure, float temperature);
uint16_t lift_to_beep_freq(float lift);
uint16_t lift_to_beep_length(float lift);

#ifdef __cplusplus
}
#endif

#endif // UTIL_H