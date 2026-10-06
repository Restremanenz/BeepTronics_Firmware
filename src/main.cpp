#include <Arduino.h>
#include "app/Firmware.h"

namespace {
Firmware firmware;
}

void setup() { firmware.begin(); }
void loop() { firmware.update(); }
