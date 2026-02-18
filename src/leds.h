#ifndef LEDS_H
#define LEDS_H

enum LedState { LED_OFF, LED_RED, LED_GREEN, LED_YELLOW };

void ledsInit(int pinR, int pinG);
void setLedColor(LedState color);
void setBlink(bool enable, int intervalMs = 500);
void ledsLoop();

#endif