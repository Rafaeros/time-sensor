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
    // Lógica para CATODO COMUM (HIGH acende)
    digitalWrite(_pinR, (color == LED_RED || color == LED_YELLOW) ? HIGH : LOW);
    digitalWrite(_pinG, (color == LED_GREEN || color == LED_YELLOW) ? HIGH : LOW);
    digitalWrite(_pinB, (color == LED_BLUE) ? HIGH : LOW);
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