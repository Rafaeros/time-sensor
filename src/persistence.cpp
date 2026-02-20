#include "persistence.h"
#include <Preferences.h>

Preferences prefs;

void persistenceInit() {
    prefs.begin("smp_data", false); 
}

void saveState(MachineState state) {
    prefs.putBool("active", state.active);
    prefs.putLong("orderId", state.orderId);
    prefs.putString("orderCode", state.orderCode);
    prefs.putDouble("prodTime", state.prodTime);
    prefs.putDouble("pauseTime", state.pauseTime);
    prefs.putInt("pauseCount", state.pauseCount);
    prefs.putBool("isPaused", state.isPaused);
}

MachineState loadState() {
    MachineState state;
    state.active = prefs.getBool("active", false);
    state.orderId = prefs.getLong("orderId", 0);
    state.orderCode = prefs.getString("orderCode", "");
    state.prodTime = prefs.getDouble("prodTime", 0.0);
    state.pauseTime = prefs.getDouble("pauseTime", 0.0);
    state.pauseCount = prefs.getInt("pauseCount", 0);
    state.isPaused = prefs.getBool("isPaused", false);
    return state;
}

void clearState() {
    prefs.putBool("active", false);
    prefs.putDouble("prodTime", 0.0);
    prefs.putDouble("pauseTime", 0.0);
    prefs.putInt("pauseCount", 0);
    prefs.putBool("isPaused", false);
}


void enqueueOfflineLog(long orderId, double prodTime, double pauseTime, int pauseCount) {
    int tail = prefs.getInt("q_tail", 0);
    String data = String(orderId) + "," + String(prodTime) + "," + String(pauseTime) + "," + String(pauseCount);
    
    prefs.putString(("q_" + String(tail)).c_str(), data);
    prefs.putInt("q_tail", tail + 1);
}

int getOfflineLogCount() {
    int head = prefs.getInt("q_head", 0);
    int tail = prefs.getInt("q_tail", 0);
    
    if (head >= tail) {
        if (tail > 0) {
            prefs.putInt("q_head", 0);
            prefs.putInt("q_tail", 0);
        }
        return 0;
    }
    return tail - head;
}

OfflineLog peekOfflineLog() {
    OfflineLog log = {false, 0, 0.0, 0.0, 0};
    int head = prefs.getInt("q_head", 0);
    
    String data = prefs.getString(("q_" + String(head)).c_str(), "");
    if (data.length() > 0) {
        int comma1 = data.indexOf(',');
        int comma2 = data.indexOf(',', comma1 + 1);
        int comma3 = data.indexOf(',', comma2 + 1);

        if (comma1 > 0 && comma2 > 0 && comma3 > 0) {
            log.valid = true;
            log.orderId = data.substring(0, comma1).toInt();
            log.prodTime = data.substring(comma1 + 1, comma2).toDouble();
            log.pauseTime = data.substring(comma2 + 1, comma3).toDouble();
            log.pauseCount = data.substring(comma3 + 1).toInt();
        }
    }
    return log;
}

void popOfflineLog() {
    int head = prefs.getInt("q_head", 0);
    prefs.remove(("q_" + String(head)).c_str());
    prefs.putInt("q_head", head + 1);
}