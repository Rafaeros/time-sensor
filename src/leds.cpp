#include <Arduino.h>
#include "leds.h"

int _pinR, _pinG;
bool _blinking = false;
int _blinkInterval = 500;
unsigned long _lastBlink = 0;
bool _ledOn = true;
LedState _currentColor = LED_OFF;

void ledsInit(int pinR, int pinG) {
    _pinR = pinR; _pinG = pinG;
    pinMode(_pinR, OUTPUT); pinMode(_pinG, OUTPUT);
}

void applyColor(LedState color) {
    // Ajuste HIGH/LOW dependendo se for Catodo ou Anodo Comum
    // Assumindo CATODO COMUM (GND comum, HIGH acende)
    digitalWrite(_pinR, (color == LED_RED || color == LED_YELLOW) ? HIGH : LOW);
    digitalWrite(_pinG, (color == LED_GREEN || color == LED_YELLOW) ? HIGH : LOW);
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