#include <Arduino.h>
#include "leds.h"

static int _r, _g;

void ledsInit(int pinR, int pinG) {
    _r = pinR;
    _g = pinG;
    pinMode(_r, OUTPUT);
    pinMode(_g, OUTPUT);
}

void setRGB(bool r, bool g) {
    digitalWrite(_r, r);
    digitalWrite(_g, g);
}
