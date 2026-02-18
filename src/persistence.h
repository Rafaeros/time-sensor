#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <Arduino.h>

struct MachineState {
    bool active;
    long orderId;          
    double prodTime;       
    double pauseTime;       
    int pauseCount;        
    bool isPaused;
};

void persistenceInit();
void saveState(MachineState state);
MachineState loadState();
void clearState();

#endif