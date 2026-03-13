#include <Arduino.h>
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

const char *SSID = "WIFI_SSID";
const char *PASS = "WIFI_PASS";
const char *IP = "TCP_SERVER_IP";
const int PORT = 5050;

Timer prodTimer;
Timer pauseTimer;

enum State
{
    IDLE,
    RUNNING,
    PAUSED
};
State currentState = IDLE;

long currentOrderId = 0;
String currentOrderCode = "";
int pauseCount = 0;

bool isLongBeeping = false;
unsigned long longBeepStartTime = 0;

unsigned long lastAutoSave = 0;
const unsigned long AUTO_SAVE_INTERVAL = 180000;

bool lastConnectionState = false;

void forceStateSave(bool activeStatus)
{
    MachineState ms = {
        activeStatus,
        currentOrderId,
        currentOrderCode,
        (double)prodTimer.getSeconds(),
        (double)pauseTimer.getSeconds(),
        pauseCount,
        (currentState == PAUSED)};
    saveState(ms);
}

void setup()
{
    Serial.begin(115200);
    pinMode(PIN_SWITCH_PROD, INPUT_PULLUP);
    pinMode(PIN_BTN_PAUSE, INPUT_PULLUP);
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    ledsInit(PIN_LED_R, PIN_LED_G, PIN_LED_B);
    displayInit();
    persistenceInit();

    updateStatus("Conectando WiFi", SSID);
    networkInit(SSID, PASS, IP, PORT);

    unsigned long startAttempt = millis();
    while (!isConnected() && millis() - startAttempt < 15000)
    {
        networkLoop();
        delay(100);
    }

    lastConnectionState = isConnected();

    MachineState saved = loadState();
    bool switchOn = !digitalRead(PIN_SWITCH_PROD);

    if (saved.active && switchOn)
    {
        currentOrderId = saved.orderId;
        currentOrderCode = saved.orderCode.length() > 0 ? saved.orderCode : "OFFLINE-REC";
        pauseCount = saved.pauseCount;

        prodTimer.set((unsigned long)saved.prodTime);
        pauseTimer.set((unsigned long)saved.pauseTime);

        bool isPauseHeld = !digitalRead(PIN_BTN_PAUSE);

        if (isPauseHeld)
        {
            currentState = PAUSED;
            pauseTimer.start();
        }
        else
        {
            currentState = RUNNING;
            prodTimer.start();
        }

        if (isConnected())
        {
            sendStatus(currentState == RUNNING ? "RUNNING" : "PAUSED");
        }
    }
    else
    {
        clearState();
        currentState = IDLE;
    }
}

void finalizeProduction()
{
    if (currentState == RUNNING)
        prodTimer.pause();
    if (currentState == PAUSED)
        pauseTimer.pause();

    if (prodTimer.getSeconds() < 10)
    {
        updateStatus("CANCELADO", "Tempo curto (<10s)");
        clearState();
        delay(2000);
        
        if (isConnected())
            sendStatus("IDLE");
        currentState = IDLE;
        return;
    }

    updateStatus("Finalizando...", "Enviando Log");

    if (isConnected())
    {
        bool sent = sendJsonLog(currentOrderId,
                                (double)prodTimer.getSeconds(),
                                (double)pauseTimer.getSeconds(),
                                1, pauseCount);

        if (sent)
        {
            updateStatus("SUCESSO", "Log Salvo");
            clearState();
            delay(1000);
        }
        else
        {
            displayError("Erro Ack Server");
            enqueueOfflineLog(currentOrderId, (double)prodTimer.getSeconds(), (double)pauseTimer.getSeconds(), pauseCount);
            clearState();
            delay(2000);
        }
    }
    else
    {
        displayError("Offline - Na Fila");
        enqueueOfflineLog(currentOrderId, (double)prodTimer.getSeconds(), (double)pauseTimer.getSeconds(), pauseCount);
        clearState();
        delay(2000);
    }

    if (isConnected())
        sendStatus("IDLE");
    currentState = IDLE;
}

void loop()
{
    networkLoop();
    ledsLoop();

    bool currentConnection = isConnected();

    if (currentConnection && !lastConnectionState)
    {
        if (currentState == RUNNING)
            sendStatus("RUNNING");
        else if (currentState == PAUSED)
            sendStatus("PAUSED");
        else if (currentState == IDLE)
            sendStatus("IDLE");
    }
    lastConnectionState = currentConnection;

    if (isLongBeeping)
    {
        if (millis() - longBeepStartTime < 2000)
        {
            digitalWrite(PIN_BUZZER, HIGH);
        }
        else
        {
            digitalWrite(PIN_BUZZER, LOW);
            isLongBeeping = false;
        }
    }
    else
    {
        digitalWrite(PIN_BUZZER, LOW);
    }

    bool switchOn = !digitalRead(PIN_SWITCH_PROD);
    bool isPauseHeld = !digitalRead(PIN_BTN_PAUSE);

    int pendingLogs = getOfflineLogCount();
    bool isFlushing = (currentState == IDLE && currentConnection && pendingLogs > 0);

    if (isFlushing)
    {
        setLedColor(LED_BLUE);
        setBlink(true, 200);
    }
    else
    {
        if (currentState == IDLE)
        {
            setLedColor(currentConnection ? LED_GREEN : LED_RED);
            setBlink(false);
        }
        else if (currentState == RUNNING)
        {
            setLedColor(LED_BLUE);
            setBlink(true, 500);
        }
        else if (currentState == PAUSED)
        {
            setLedColor(LED_YELLOW);
            setBlink(true, 500);
        }
    }

    if (isFlushing)
    {
        updateStatus("Fazendo Flush", String(pendingLogs) + " pendentes");

        OfflineLog nextLog = peekOfflineLog();
        if (nextLog.valid)
        {
            bool sent = sendJsonLog(nextLog.orderId, nextLog.prodTime, nextLog.pauseTime, 1, nextLog.pauseCount);
            if (sent)
            {
                popOfflineLog();
                delay(500);
            }
            else
            {
                displayError("Erro no Flush");
                delay(2000);
            }
        }
        else
        {
            popOfflineLog();
        }
    }

    switch (currentState)
    {
    case IDLE:
        if (switchOn)
        {
            if (isFlushing)
            {
                displayError("Aguarde Flush!");
                delay(1500);
                break;
            }

            updateStatus("Buscando OP...", "");

            if (currentConnection)
            {
                OrderInfo info = requestOrderInfo();
                if (info.id > 0)
                {
                    currentOrderId = info.id;
                    currentOrderCode = info.code;
                }
                else
                {
                    displayError("Sem OP Ativa");
                    delay(2000);
                    break;
                }
            }
            else
            {
                MachineState lastKnown = loadState();
                if (lastKnown.orderId > 0)
                {
                    currentOrderId = lastKnown.orderId;
                    currentOrderCode = lastKnown.orderCode;
                }
                else
                {
                    displayError("Sem Rede e Sem OP");
                    delay(2000);
                    break;
                }
            }

            prodTimer.reset();
            pauseTimer.reset();
            isLongBeeping = true;
            longBeepStartTime = millis();

            if (isPauseHeld)
            {
                currentState = PAUSED;
                pauseCount = 1;
                pauseTimer.start();
                if (currentConnection)
                    sendStatus("PAUSED");
            }
            else
            {
                currentState = RUNNING;
                pauseCount = 0;
                prodTimer.start();
                if (currentConnection)
                    sendStatus("RUNNING");
            }

            forceStateSave(true);
            lastAutoSave = millis();
        }
        break;

    case RUNNING:
        if (!switchOn)
        {
            finalizeProduction();
        }
        else if (isPauseHeld)
        {
            prodTimer.pause();
            pauseTimer.start();
            currentState = PAUSED;
            pauseCount++;

            if (currentConnection)
                sendStatus("PAUSED");

            forceStateSave(true);
            lastAutoSave = millis();
        }
        break;

    case PAUSED:
        if (!switchOn)
        {
            finalizeProduction();
        }
        else if (!isPauseHeld)
        {
            pauseTimer.pause();
            prodTimer.start();
            currentState = RUNNING;

            if (currentConnection)
                sendStatus("RUNNING");

            forceStateSave(true);
            lastAutoSave = millis();
        }
        break;
    }

    if ((currentState == RUNNING || currentState == PAUSED) && (millis() - lastAutoSave > AUTO_SAVE_INTERVAL))
    {
        forceStateSave(true);
        lastAutoSave = millis();
    }

    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate > 500)
    {
        lastDisplayUpdate = millis();

        if (currentState == RUNNING)
        {
            displayProduction(currentOrderCode, prodTimer.getSeconds(), 1);
        }
        else if (currentState == PAUSED)
        {
            displayPaused(pauseTimer.getSeconds());
        }
        else if (currentState == IDLE && !isFlushing)
        {
            if (currentConnection)
            {
                updateStatus("PRONTO", "Aguardando...");
            }
            else
            {
                if (pendingLogs > 0)
                {
                    updateStatus("OFFLINE", String(pendingLogs) + " na fila");
                }
                else
                {
                    updateStatus("OFFLINE", "Aguardando...");
                }
            }
        }
    }

    delay(50);
}