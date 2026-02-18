#include "timer.h"

Timer::Timer() {
    startTime = 0;
    accumulated = 0;
    active = false;
}

void Timer::start() {
    if (!active) {
        startTime = millis();
        active = true;
    }
}

void Timer::pause() {
    if (active) {
        accumulated += (millis() - startTime);
        active = false;
    }
}

void Timer::reset() {
    active = false;
    accumulated = 0;
    startTime = 0;
}

void Timer::set(unsigned long seconds) {
    accumulated = seconds * 1000;
    active = false;
}

unsigned long Timer::getSeconds() {
    if (active) {
        return (accumulated + (millis() - startTime)) / 1000;
    }
    return accumulated / 1000;
}