#include "persistence.h"
#include <Preferences.h>

Preferences prefs;

void persistenceInit() {
    prefs.begin("smp_data", false); 
}

void saveState(MachineState state) {
    prefs.putBool("active", state.active);
    prefs.putLong("orderId", state.orderId);
    prefs.putDouble("prodTime", state.prodTime);
    prefs.putDouble("pauseTime", state.pauseTime);
    prefs.putInt("pauseCount", state.pauseCount);
    prefs.putBool("isPaused", state.isPaused);
}

MachineState loadState() {
    MachineState state;
    state.active = prefs.getBool("active", false);
    state.orderId = prefs.getLong("orderId", 0);
    state.prodTime = prefs.getDouble("prodTime", 0.0);
    state.pauseTime = prefs.getDouble("pauseTime", 0.0);
    state.pauseCount = prefs.getInt("pauseCount", 0);
    state.isPaused = prefs.getBool("isPaused", false);
    return state;
}

void clearState() {
    prefs.clear();
}