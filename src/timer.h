#ifndef TIMER_H
#define TIMER_H

#include <Arduino.h>

class Timer {
private:
    unsigned long startTime;
    unsigned long accumulated;
    bool active;

public:
    Timer();
    void start();
    void pause();
    void reset();
    void set(unsigned long seconds); 
    unsigned long getSeconds();
};

#endif