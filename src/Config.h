#pragma once

#define LED_BRIGHTNESS 20

// Baro Settings
#define LIFT_THRESHOLD 0.25
#define LIFT_OFF_THRESHOLD 0.1
#define SINK_THRESHOLD -0.5
#define SINK_OFF_THRESHOLD -0.1

#define LEDC_SPEAKER 0 // PWM Channnel
#define RESOLUTION 8   // Volume Resultion --> 8 = 256
#define VOLUME_MIN 0   // minimum volume
#define VOLUME_MAX 100 // maximum volume
#define VOLUME_STEP 5  // volume increment

#define ALTITUDE_IIR_WEIGHT 0.08 // how much the new value alters the result
#define SLOPE_HISTORY_SIZE  35
#define SLOPE_NOISE_MAX 0.1f

#define BAT_SAMPLE_TIME 50000















//Sinnlos

// #define D4 293
// #define FIS4 364
// #define GIS4 415
// #define B4 494
// #define A4 440
// #define B3 247
// #define GIS3 207
// #define A3 220
// #define C4 262

// #define FULL_NOTE 1500
// #define HALF_NOTE (FULL_NOTE / 2)
// #define QUARTER_NOTE (HALF_NOTE / 2)
// #define EIGHT_NOTE (QUARTER_NOTE / 2)
// #define PLAY_NOTE(NOTE, LENGTH) do {    \
//   ledcWriteTone(LEDC_SPEAKER, NOTE);    \
//   ledcWrite(LEDC_SPEAKER, VOLUME);      \
//   delay(LENGTH);                        \
//   ledcWrite(LEDC_SPEAKER, false);       \
//   delay(10);                            \
// } while(0)
// #define NO_TONE(LENGTH) do {            \
//   ledcWrite(LEDC_SPEAKER, false);       \
//   delay(LENGTH);                        \
// } while(0)