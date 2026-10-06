#include <Arduino.h>

#ifndef BLE_H
#define BLE_H

bool BLE_connected(void);
void setup_BLE(void);
void send_Data_LK8EX1(float pressure, float lift, int temperature, uint8_t battery);

#endif // BLE_H