#include <Arduino.h>
#include <WiFi.h>
#include "network.h"
#include "leds.h"
#include "persistence.h"
#include "display.h"
#include "timer.h"

#define PIN_SWITCH_PROD 25
#define PIN_BTN_PAUSE 26
#define PIN_LED_R 14
#define PIN_LED_G 13
#define PIN_LED_B 12
#define PIN_BUZZER 27

// Use real Gateway MAC address here
uint8_t gatewayMacAddress[] = {0x30, 0xAE, 0xA4, 0xDB, 0x29, 0x74};

Timer prodTimer;
Timer pauseTimer;

enum State { IDLE, RUNNING, PAUSED };
State currentState = IDLE;
bool serverOnline = false; // Backend connection status

long currentOrderId = 0;
String currentOrderCode = "";
int pauseCount = 0;

bool isLongBeeping = false;
unsigned long longBeepStartTime = 0;

unsigned long lastAutoSave = 0;
const unsigned long AUTO_SAVE_INTERVAL = 180000;

void forceStateSave(bool activeStatus) {
    MachineState ms = {
        activeStatus,
        currentOrderId,
        currentOrderCode,
        (double)prodTimer.getSeconds(),
        (double)pauseTimer.getSeconds(),
        pauseCount,
        (currentState == PAUSED)
    };
    saveState(ms);
}

void setup() {
    Serial.begin(115200);
    pinMode(PIN_SWITCH_PROD, INPUT_PULLUP);
    pinMode(PIN_BTN_PAUSE, INPUT_PULLUP);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    ledsInit(PIN_LED_R, PIN_LED_G, PIN_LED_B);
    displayInit();
    persistenceInit();

    updateStatus("Starting Radio", "ESP-NOW");
    networkInit(gatewayMacAddress);

    updateStatus("Connecting...", "Java Server");
    
    // Test server connectivity (Java backend)
    serverOnline = pingServer();

    if (serverOnline) {
        setLedColor(LED_GREEN);
        setBlink(true, 100); // Fast green: connection successful!
        delay(1500); 
    } else {
        setLedColor(LED_RED);
        setBlink(true, 500); // Red: radio or server error
        delay(1500);
    }
    setBlink(false, 0);

    MachineState saved = loadState();
    bool switchOn = (digitalRead(PIN_SWITCH_PROD) == LOW); 

    if (saved.active && switchOn) {
        currentOrderId = saved.orderId;
        currentOrderCode = saved.orderCode.length() > 0 ? saved.orderCode : "OFFLINE-REC";
        pauseCount = saved.pauseCount;

        prodTimer.set((unsigned long)saved.prodTime);
        pauseTimer.set((unsigned long)saved.pauseTime);

        if (digitalRead(PIN_BTN_PAUSE) == LOW) {
            currentState = PAUSED;
            pauseTimer.start();
        } else {
            currentState = RUNNING;
            prodTimer.start();
        }
    } else {
        clearState();
        currentState = IDLE;
    }
    
    // Send initial status on startup to update Web dashboard
    delay(500); // Wait for radio stabilization
    if (serverOnline) {
        if (currentState == RUNNING) sendStatus("RUNNING");
        else if (currentState == PAUSED) sendStatus("PAUSED");
        else sendStatus("IDLE");
    }
}

void finalizeProduction() {
    if (currentState == RUNNING) prodTimer.pause();
    if (currentState == PAUSED) pauseTimer.pause();

    if (prodTimer.getSeconds() < 10) {
        updateStatus("CANCELLED", "Short time (<10s)");
        clearState();
        delay(2000);
        currentState = IDLE;
        if (serverOnline) sendStatus("IDLE");
        return;
    }

    updateStatus("Finalizing...", "Sending Log");

    bool sent = sendLog(currentOrderId, (double)prodTimer.getSeconds(), (double)pauseTimer.getSeconds(), 1, pauseCount);
    
    if (sent) {
        updateStatus("SUCCESS", "Log Saved");
        clearState();
        delay(1000);
    } else {
        displayError("Offline - Queued");
        enqueueOfflineLog(currentOrderId, (double)prodTimer.getSeconds(), (double)pauseTimer.getSeconds(), pauseCount);
        clearState();
        delay(2000);
    }

    currentState = IDLE;
    if (serverOnline) sendStatus("IDLE");
}

void loop() {
    networkLoop();
    ledsLoop();

    // --- SMART HEARTBEAT ---
    // Every 15 seconds notifies backend to replace TCP PING
    static unsigned long lastHeartbeat = 0;
    if (millis() - lastHeartbeat > 15000) {
        lastHeartbeat = millis();
        
        // Check if server is reachable
        serverOnline = pingServer(); 
        
        // If online, update current state in database
        if (serverOnline) {
            if (currentState == IDLE) sendStatus("IDLE");
            else if (currentState == RUNNING) sendStatus("RUNNING");
            else if (currentState == PAUSED) sendStatus("PAUSED");
        }
    }

    if (isLongBeeping) {
        if (millis() - longBeepStartTime < 2000) digitalWrite(PIN_BUZZER, HIGH);
        else {
            digitalWrite(PIN_BUZZER, LOW);
            isLongBeeping = false;
        }
    } else {
        digitalWrite(PIN_BUZZER, LOW);
    }

    // --- PHYSICAL INPUTS AND MOTION DETECTION ---
    bool switchOn = (digitalRead(PIN_SWITCH_PROD) == LOW);
    bool isPauseHeld = (digitalRead(PIN_BTN_PAUSE) == LOW);

    // Detect only the moment the switch is turned on (edge detection)
    static bool lastSwitchState = false;
    bool switchJustTurnedOn = (switchOn && !lastSwitchState);
    lastSwitchState = switchOn;

    int pendingLogs = getOfflineLogCount();
    bool isFlushing = (currentState == IDLE && pendingLogs > 0);

    // --- LED STATUS SYSTEM ---
    if (isFlushing) {
        setLedColor(LED_BLUE);
        setBlink(true, 200);
    } else {
        if (currentState == IDLE) {
            // Green only if both radio and server are connected
            if (isConnected() && serverOnline) {
                setLedColor(LED_GREEN);
                setBlink(false); 
            } else {
                setLedColor(LED_RED);
                setBlink(false); 
            }
        } else if (currentState == RUNNING) {
            setLedColor(LED_BLUE);
            setBlink(true, 500);
        } else if (currentState == PAUSED) {
            setLedColor(LED_YELLOW);
            setBlink(true, 500);
        }
    }

    if (isFlushing) {
        static unsigned long lastFlushAttempt = 0;
        if (millis() - lastFlushAttempt > 1500) { 
            lastFlushAttempt = millis();
            updateStatus("Flushing", String(pendingLogs) + " pending");
            
            OfflineLog nextLog = peekOfflineLog();
            if (nextLog.valid) {
                bool sent = sendLog(nextLog.orderId, nextLog.prodTime, nextLog.pauseTime, 1, nextLog.pauseCount);
                if (sent) popOfflineLog(); 
                else displayError("Flush Error");
            } else {
                popOfflineLog();
            }
        }
    }

    switch (currentState) {
        case IDLE:
            // Triggered exactly when the switch is turned ON
            if (switchJustTurnedOn) {
                if (isFlushing) {
                    displayError("Wait for Flush!");
                    delay(1500);
                    break;
                }
                updateStatus("Fetching Order...", "");

                OrderInfo info = requestOrderInfo();
                if (info.id > 0) {
                    currentOrderId = info.id;
                    currentOrderCode = info.code;
                } else {
                    displayError("No Active OP");
                    delay(2000);
                    // Loop won't repeat! Operator must toggle switch OFF and ON again.
                    break;
                }

                prodTimer.reset();
                pauseTimer.reset();
                isLongBeeping = true;
                longBeepStartTime = millis();

                if (isPauseHeld) {
                    currentState = PAUSED;
                    pauseCount = 1;
                    pauseTimer.start();
                    if (serverOnline) sendStatus("PAUSED");
                } else {
                    currentState = RUNNING;
                    pauseCount = 0;
                    prodTimer.start();
                    if (serverOnline) sendStatus("RUNNING");
                }
                forceStateSave(true);
                lastAutoSave = millis();
            }
            break;

        case RUNNING:
            if (!switchOn) finalizeProduction();
            else if (isPauseHeld) {
                prodTimer.pause();
                pauseTimer.start();
                currentState = PAUSED;
                pauseCount++;
                if (serverOnline) sendStatus("PAUSED");
                forceStateSave(true);
                lastAutoSave = millis();
            }
            break;

        case PAUSED:
            if (!switchOn) finalizeProduction();
            else if (!isPauseHeld) {
                pauseTimer.pause();
                prodTimer.start();
                currentState = RUNNING;
                if (serverOnline) sendStatus("RUNNING");
                forceStateSave(true);
                lastAutoSave = millis();
            }
            break;
    }

    if ((currentState == RUNNING || currentState == PAUSED) && (millis() - lastAutoSave > AUTO_SAVE_INTERVAL)) {
        forceStateSave(true);
        lastAutoSave = millis();
    }

    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 500) {
        lastDisplayUpdate = millis();
        if (currentState == RUNNING) displayProduction(currentOrderCode, prodTimer.getSeconds(), 1);
        else if (currentState == PAUSED) displayPaused(pauseTimer.getSeconds());
        else if (currentState == IDLE && !isFlushing) {
            if (!serverOnline) {
                 updateStatus("OFFLINE", "Server Error");
            }
            else if (pendingLogs > 0) {
                 updateStatus("READY", String(pendingLogs) + " in queue");
            }
            else {
                 updateStatus("READY", "Waiting...");
            }
        }
    }
    delay(50);
}