#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

void displayInit();
void updateStatus(String status, String subInfo);
void displayProduction(String opCode, unsigned long seconds, int qty);
void displayPaused(unsigned long seconds);
void displayError(String msg);

#endif