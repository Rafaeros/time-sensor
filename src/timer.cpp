#include <Arduino.h>
#include "timer.h"

static unsigned long startTime = 0;
static unsigned long accumulated = 0;
static bool active = false;

void timerStart() {
    if (!active) {
        startTime = millis() - accumulated;
        active = true;
    }
}

void timerPause() {
    if (active) {
        accumulated = millis() - startTime;
        active = false;
    }
}

void timerReset() {
    active = false;
    accumulated = 0;
}

unsigned long timerGetSeconds() {
    if (active)
        return (millis() - startTime) / 1000;
    return accumulated / 1000;
}
