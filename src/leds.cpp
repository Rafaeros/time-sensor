#include <Arduino.h>
#include "leds.h"

int _pinR, _pinG, _pinB;
bool _blinking = false;
int _blinkInterval = 500;
unsigned long _lastBlink = 0;
bool _ledOn = true;
LedState _currentColor = LED_OFF;

void ledsInit(int pinR, int pinG, int pinB) {
    _pinR = pinR; _pinG = pinG; _pinB = pinB;
    pinMode(_pinR, OUTPUT); pinMode(_pinG, OUTPUT); pinMode(_pinB, OUTPUT);
}

void applyColor(LedState color) {
    int r = 0, g = 0, b = 0;

    switch(color) {
        case LED_RED:
            r = 255; g = 0; b = 0;
            break;
        case LED_GREEN:
            r = 0; g = 255; b = 0;
            break;
        case LED_BLUE:
            r = 0; g = 0; b = 255;
            break;
        case LED_YELLOW:
            r = 255;
            g = 25;
            b = 0;
            break;
        case LED_OFF:
        default:
            r = 0; g = 0; b = 0;
            break;
    }

    analogWrite(_pinR, r);
    analogWrite(_pinG, g);
    analogWrite(_pinB, b);
}

void setLedColor(LedState color) {
    _currentColor = color;
    if (!_blinking) applyColor(_currentColor);
}

void setBlink(bool enable, int intervalMs) {
    _blinking = enable;
    _blinkInterval = intervalMs;
    if (!enable) applyColor(_currentColor);
}

void ledsLoop() {
    if (_blinking && millis() - _lastBlink > _blinkInterval) {
        _lastBlink = millis();
        _ledOn = !_ledOn;
        if (_ledOn) applyColor(_currentColor);
        else applyColor(LED_OFF);
    }
}