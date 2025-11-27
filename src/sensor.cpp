#include <Arduino.h>
#include "sensor.h"

static int _trig, _echo;

void sensorInit(int trigPin, int echoPin) {
    _trig = trigPin;
    _echo = echoPin;
    pinMode(_trig, OUTPUT);
    pinMode(_echo, INPUT);
}

long measureDistanceCM() {
    digitalWrite(_trig, LOW);
    delayMicroseconds(2);
    digitalWrite(_trig, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trig, LOW);

    long duration = pulseIn(_echo, HIGH);
    return duration * 0.034 / 2;
}
