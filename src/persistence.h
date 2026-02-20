#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <Arduino.h>

struct MachineState {
    bool active;
    long orderId;
    String orderCode;
    double prodTime;
    double pauseTime;
    int pauseCount;
    bool isPaused;
};

struct OfflineLog {
    bool valid;
    long orderId;
    double prodTime;
    double pauseTime;
    int pauseCount;
};

void persistenceInit();

void saveState(MachineState state);
MachineState loadState();
void clearState();

void enqueueOfflineLog(long orderId, double prodTime, double pauseTime, int pauseCount);
int getOfflineLogCount();
OfflineLog peekOfflineLog();
void popOfflineLog();

#endif