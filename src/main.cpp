#include "Arduino.h"
#include "Wire.h"
#include "Bounce2.h"
#include "FastLED.h"
#include "MS5611.h"
#include "Pinout.h"
#include "config.h"
#include "util.h"
#include "BLE.h"


// variables
unsigned int aktmillis, prevmillis;

// barometer
MS5611 baro(CS_BARO, MISO, MOSI, CLOCK);

// RGB LED
CRGB led;

// UI Buttons
#define BTNS_COUNT    4
#define BTNS_INTERVAL 25 // ms
const uint8_t btn_pins[BTNS_COUNT] = {BTN_ON, BTN_DOWN, BTN_UP, BTN_OK};
Bounce *btns = new Bounce[BTNS_COUNT];
#define UPDATE_BTNS() for (uint8_t i = 0; i < BTNS_COUNT; i++) { btns[i].update(); }

// remaining battery voltage in mV
uint8_t bat_per = 0;
float bat_voltage = 0;

// buzzer volume
uint16_t volume = 20;

float prev_altitude = 0.f;

#define SIMULATION_INTERVAL 1000
bool simulation = true;
float simulated_lift = 0.f;

void update_simulation(void);
void vario_beep(float lift);

void setup()
{
    pinMode(PWR,     OUTPUT);
    pinMode(EN_GPS,  OUTPUT);
    pinMode(SPEAKER, OUTPUT);
    pinMode(BAT_EN,  OUTPUT);
    pinMode(BAT,     INPUT );
  
    // Initialize serial
    Serial.begin(115200);

    // Enable PCB
    digitalWrite(PWR, HIGH);
    // Enable GPS
    digitalWrite(EN_GPS, LOW);

    // Start I2C-Bus
    Wire.begin(SDA, SCL);

    // Initialize RGB LED
    FastLED.addLeds<WS2812, LED, GRB>(&led, 1);
    FastLED.setBrightness(LED_BRIGHTNESS);

    // set up buzzer
    ledcSetup(LEDC_SPEAKER, 1000, RESOLUTION);
    ledcAttachPin(SPEAKER, LEDC_SPEAKER);
    delay(10);

    // set up ui buttons
    for (uint8_t i = 0; i < BTNS_COUNT; i++) {
        btns[i].attach(btn_pins[i], INPUT_PULLUP);
        btns[i].interval(BTNS_INTERVAL);
    }

    // set up barometer
    if (!baro.begin()) {
        LOG_MSG("ERROR: MS5611 not found! Aborting...");
        delay(500);
        abort();
    }
    
    // play startup sequence
    BEEP(523);  // C5
    delay(80);
    BEEP(659);  // E5
    delay(80);
    BEEP(784);  // G5
    delay(80);
    BEEP(1046); // C6 
    delay(200);
    MUTE();

    bat_per = get_battery_percentage();

    baro.startRead();
    while (!baro.update()) { delay(10); }
    prev_altitude = pressure_to_altitude(baro.getPressure(), baro.getTemperature());

    setup_BLE();
}

void loop()
{
    static float lift = 0.f;
    static float pressure = 101325;
    static int temperature = 0;

    aktmillis = millis();
    UPDATE_BTNS();

    if (BLE_connected() && aktmillis - prevmillis >= 100) {
        prevmillis = aktmillis;
        send_Data_LK8EX1(pressure, lift, temperature, bat_per);
    }

    // evaluate buttons
    for (uint8_t i = 0; i < BTNS_COUNT; i++) {
        if (btns[i].fell()) {
            switch(btns[i].getPin()) {
                case BTN_ON:
                   simulation = !simulation;
                   simulated_lift = 0.f;
                    led = CRGB(255, 255, 255);
                    FastLED.show();
                break;
                case BTN_DOWN:
                    // decrease volume
                    volume = CLAMP(volume - VOLUME_STEP, VOLUME_MIN, VOLUME_MAX);
                    led = CRGB(255, 0, 0);
                    FastLED.show();
                    BEEP(523);  // C5
                    delay(80);
                    MUTE();
                break;
                case BTN_UP:
                    // increase volume
                    bat_per = get_battery_percentage();
                    //bat_voltage = get_battery_voltage();
                    led = CRGB(0, 0, 255);
                    FastLED.show();
                    BEEP(659);  // C5
                    delay(80);
                    MUTE();
                    volume = CLAMP(volume + VOLUME_STEP, VOLUME_MIN, VOLUME_MAX);
                break;
                case BTN_OK:
                    // shut off device
                    BEEP(1046); // C6
                    delay(80);
                    BEEP(784);  // G5
                    delay(80);
                    BEEP(659);  // E5
                    delay(80);
                    BEEP(523);  // C5 
                    delay(200);
                    MUTE();

                    digitalWrite(PWR,LOW);
                break;
            }
        }
    }
    
    if (simulation) {
        update_simulation();
        // don't read out baro if simulating
        return;
    }

    // start read operation if ready
    baro.startRead();

    // check if values are ready
    if (baro.update()) {
        float altitude, altitude_iir, slope;
        static float slope_history[SLOPE_HISTORY_SIZE] = {0};

        // altitude      = pressure_to_altitude(baro.getPressure(), baro.getTemperature());
        pressure      = baro.getPressure();
        temperature   = baro.getTemperature();
        altitude      = pressure_to_altitude(pressure, temperature);
        altitude_iir  = (1.0 - ALTITUDE_IIR_WEIGHT) * prev_altitude + ALTITUDE_IIR_WEIGHT * altitude;
        float altitude_iir2 = (1.0 - 0.01) * prev_altitude + 0.01 * altitude;

        //LOG_MSG("H2: %f", altitude_iir2);
        //LOG_MSG("H1: %f", altitude_iir);
        //LOG_MSG("H0: %f", altitude);

        // calculate change in altitude over 1/50th of a second
        slope  = altitude_iir - prev_altitude;
        slope *= (float)MS5611_SAMPLE_RATE; // m/s
        if (abs(slope) < SLOPE_NOISE_MAX) slope = 0.f;

        //LOG_MSG("S: %f", slope);

        memmove(&slope_history[1], &slope_history[0], (SLOPE_HISTORY_SIZE - 1) * sizeof(slope_history[0]));
        slope_history[0] = slope;

        lift = 0.f;
        for (uint8_t i = 0; i < SLOPE_HISTORY_SIZE; i++) lift += slope_history[i];
        lift /= (float)SLOPE_HISTORY_SIZE;

        //LOG_MSG("V: %f", lift);

        prev_altitude = altitude_iir;
    }
    if (millis() < 10000) return;
    vario_beep(lift);
}

void update_simulation(void)
{
    unsigned long mills = millis();
    static unsigned long prev_mills = millis();

    if (mills - prev_mills >= SIMULATION_INTERVAL) {
        prev_mills = mills;
        simulated_lift += 0.2;
    }
    vario_beep(simulated_lift);
}

void vario_beep(float lift)
{
    static unsigned long prev_beep = millis(), prev_batt_update = millis();
    static uint32_t beep_time = 0;
    static bool is_beeping = false;
    
    uint32_t beep_freq = 0;
    unsigned long mills = millis();

    if (mills - prev_beep >= beep_time) {
        if (!is_beeping && lift >= LIFT_THRESHOLD) {
            prev_beep = mills;

            is_beeping = true;
            beep_time = lift_to_beep_length(lift);
            beep_freq = lift_to_beep_freq(lift);
            LOG_MSG("Beep Time: %u", beep_time);

            // led = CRGB(255, 0, 0);
            // FastLED.show();

            BEEP(beep_freq);
        } else if (is_beeping && lift >= LIFT_THRESHOLD) {
            prev_beep = mills;

            beep_time = lift_to_beep_length(lift);
            is_beeping = false;       

            // led = CRGB(0, 0, 0);
            // FastLED.show();    

            MUTE();
        }

        if (lift <= LIFT_OFF_THRESHOLD) {
            prev_beep = mills;

            if (led) {
                led = CRGB(0, 0, 0);
                FastLED.show();
            }

            is_beeping = false;
            MUTE();
        }

        if (mills - prev_batt_update >= BAT_SAMPLE_TIME) {
            bat_per = get_battery_percentage();
            prev_batt_update = mills;
        } 
    }
}
