#include <Arduino.h>
#include "network.h"
#include "leds.h"
#include "persistence.h"
#include "display.h"
#include "timer.h" 

#define PIN_SWITCH_PROD 46
#define PIN_BTN_PAUSE   45
#define PIN_LED_R       36  
#define PIN_LED_G       37  
#define PIN_BUZZER      48
const char* SSID = "WIFI_SSID";
const char* PASS = "WIFI_PASS";
const char* IP   = "TCP_SERVER_IP"; 
const int PORT   = 5050;

Timer prodTimer;  
Timer pauseTimer; 

enum State { IDLE, RUNNING, PAUSED };
State currentState = IDLE;

long currentOrderId = 0;
String currentOrderCode = "";
int pauseCount = 0;

bool isLongBeeping = false;
unsigned long longBeepStartTime = 0;
unsigned long rapidBeepTimer = 0;
bool rapidBeepState = false;

void setup() {
    Serial.begin(115200);
    pinMode(PIN_SWITCH_PROD, INPUT_PULLUP); 
    pinMode(PIN_BTN_PAUSE, INPUT_PULLUP);   
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW); 
    
    ledsInit(PIN_LED_R, PIN_LED_G);
    displayInit();
    persistenceInit();
    
    MachineState saved = loadState();
    bool switchOn = !digitalRead(PIN_SWITCH_PROD);

    if (saved.active && switchOn) {
        Serial.println("Recuperando...");
        currentOrderId = saved.orderId;
        currentOrderCode = "REC-" + String(currentOrderId);
        pauseCount = saved.pauseCount;
        
        prodTimer.set((unsigned long)saved.prodTime);
        pauseTimer.set((unsigned long)saved.pauseTime);

        bool isPauseHeld = !digitalRead(PIN_BTN_PAUSE);
        
        if (isPauseHeld) {
            currentState = PAUSED;
            pauseTimer.start();
            setLedColor(LED_YELLOW);
        } else {
            currentState = RUNNING;
            prodTimer.start();
            setLedColor(LED_GREEN);
            setBlink(true);
        }
    } else {
        clearState();
        currentState = IDLE;
        setLedColor(LED_RED);
    }
    
    updateStatus("Conectando WiFi", SSID);
    networkInit(SSID, PASS, IP, PORT);
}

void finalizeProduction() {
    if (currentState == RUNNING) prodTimer.pause();
    if (currentState == PAUSED) pauseTimer.pause();

    updateStatus("Finalizando...", "Enviando Log");
    
    bool sent = sendJsonLog(currentOrderId, 
                          (double)prodTimer.getSeconds(), 
                          (double)pauseTimer.getSeconds(), 
                          1, pauseCount);
    
    sendStatus("IDLE"); 
    
    clearState();
    currentState = IDLE;
    setBlink(false);
    
    if (sent) {
        updateStatus("SUCESSO", "Log Salvo");
        delay(1000);
    } else {
        displayError("Erro Ack Server"); 
        delay(2000);
    }
}

void loop() {
    networkLoop();
    ledsLoop();
    if (isLongBeeping) {
        if (millis() - longBeepStartTime < 2000) {
            digitalWrite(PIN_BUZZER, HIGH);
        } else {
            digitalWrite(PIN_BUZZER, LOW);
            isLongBeeping = false;
        }
    } 
    else if (!isConnected()) {
        if (millis() - rapidBeepTimer > 150) {
            rapidBeepTimer = millis();
            rapidBeepState = !rapidBeepState;
            digitalWrite(PIN_BUZZER, rapidBeepState ? HIGH : LOW);
        }
    } 
    else {
        digitalWrite(PIN_BUZZER, LOW);
    }

    bool switchOn = !digitalRead(PIN_SWITCH_PROD); 
    bool isPauseHeld = !digitalRead(PIN_BTN_PAUSE);

    if (currentState == IDLE) {
        if (isConnected()) setLedColor(LED_GREEN);
        else setLedColor(LED_RED);
    }

    switch (currentState) {
        case IDLE:
            if (switchOn) { 
                if (!isConnected()) {
                    displayError("Sem Rede!");
                    delay(1000);
                    break;
                }

                updateStatus("Buscando OP...", "");
                OrderInfo info = requestOrderInfo();
                
                if (info.id > 0) {
                    currentOrderId = info.id;
                    currentOrderCode = info.code; 
                    
                    currentState = RUNNING;
                    prodTimer.reset(); pauseTimer.reset(); pauseCount = 0;
                    isLongBeeping = true;
                    longBeepStartTime = millis();

                    if (isPauseHeld) {
                        currentState = PAUSED;
                        pauseTimer.start();
                        sendStatus("PAUSED");
                        setLedColor(LED_YELLOW);
                        setBlink(false);
                    } else {
                        prodTimer.start(); 
                        sendStatus("RUNNING");
                        setLedColor(LED_GREEN);
                        setBlink(true, 500);
                    }
                    MachineState ms = {true, currentOrderId, 0, 0, 0, (currentState == PAUSED)};
                    saveState(ms);
                } else {
                     displayError("Sem OP Ativa");
                     delay(2000);
                }
            }
            break;

        case RUNNING:
            if (!switchOn) {
                finalizeProduction();
            }
            else if (isPauseHeld) {
                prodTimer.pause(); 
                pauseTimer.start(); 
                
                currentState = PAUSED;
                pauseCount++;
                
                sendStatus("PAUSED");
                setBlink(false);
                setLedColor(LED_YELLOW);
                
                MachineState ms = {true, currentOrderId, 
                                   (double)prodTimer.getSeconds(), (double)pauseTimer.getSeconds(), 
                                   pauseCount, true};
                saveState(ms);
            }
            break;

        case PAUSED:
            if (!switchOn) {
                finalizeProduction();
            }
            else if (!isPauseHeld) {
                pauseTimer.pause(); 
                prodTimer.start(); 
                
                currentState = RUNNING;
                
                sendStatus("RUNNING");
                setLedColor(LED_GREEN);
                setBlink(true);
                
                MachineState ms = {true, currentOrderId, 
                                   (double)prodTimer.getSeconds(), (double)pauseTimer.getSeconds(), 
                                   pauseCount, false};
                saveState(ms);
            }
            break;
    }

    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 500) {
        lastDisplayUpdate = millis();
        if (currentState == RUNNING) {
            displayProduction(currentOrderCode, prodTimer.getSeconds(), 1); 
        } else if (currentState == PAUSED) {
            displayPaused(pauseTimer.getSeconds());
        } else if (currentState == IDLE && isConnected()) {
            updateStatus("PRONTO", "Aguardando...");
        } else if (currentState == IDLE && !isConnected()) {
            updateStatus("OFFLINE", "Verifique Rede");
        }
    }
    
    delay(50);
}