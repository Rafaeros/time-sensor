#include "network.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h> // Low-level Wi-Fi control

// Gateway MAC address (Receiver ESP32)
uint8_t gatewayAddress[6];

// ESP-NOW supports up to 250 bytes. Defining 2 message types:
#define MSG_TYPE_CMD 0  // Text and Commands (GET_ORDER, STATUS, OK_LOG)
#define MSG_TYPE_LOG 1  // Production data (Binary structs)

typedef struct {
    uint8_t type;
    char text[100]; 
} CommandMessage;

typedef struct {
    uint8_t type;
    long orderId;
    float cycleTime;
    float pausedTime;
    int qtyProd;
    int qtyPaused;
} LogMessage;

// Volatile variables for radio interrupt handling
volatile bool _radioReady = false;
volatile bool _ackReceived = false;
volatile bool _orderReceived = false;
volatile bool _pongReceived = false;
OrderInfo _lastReceivedOrder = {0, ""};

// Callback for data sent (Antenna delivery confirmation)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    // Real confirmation comes from server response message
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    uint8_t msgType = incomingData[0]; // First byte denotes message type

    if (msgType == MSG_TYPE_CMD) {
        CommandMessage cmd;
        memcpy(&cmd, incomingData, sizeof(cmd));
        
        // Convert to String and trim line endings from backend
        String text = String(cmd.text);
        text.trim(); 
        
        if (text == "OK_LOG") {
            _ackReceived = true; // Servidor confirmou que gravou no Banco!
        }
        else if (text == "OK_PONG") {
            _pongReceived = true; // Server responded to PING
        }
        else if (text.startsWith("ORDER:")) {
            int firstColon = text.indexOf(':');
            int secondColon = text.indexOf(':', firstColon + 1);
            
            if (secondColon > 0) {
                _lastReceivedOrder.id = text.substring(firstColon + 1, secondColon).toInt();
                _lastReceivedOrder.code = text.substring(secondColon + 1);
            } else {
                _lastReceivedOrder.id = text.substring(6).toInt();
                _lastReceivedOrder.code = "OP-" + String(_lastReceivedOrder.id);
            }
            _orderReceived = true; // Order received successfully
        }
    }
}

void networkInit(uint8_t *gatewayMac) {
    Serial.println("--- STARTING ESP-NOW RADIO ---");
    
    // Copy Gateway MAC to memory
    memcpy(gatewayAddress, gatewayMac, 6);

    // Set Wi-Fi to Station mode without connecting to any AP
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    // --- FORCE CHANNEL 1 TO AVOID CONFLICTS ---
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    // ---------------------------------------------------------------------

    if (esp_now_init() == ESP_OK) {
        Serial.println("✅ ESP-NOW Radio Initialized!");
        _radioReady = true;
    } else {
        Serial.println("❌ Fatal error initializing radio.");
        return;
    }

    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

    // Register Gateway as peer
    // Zero peerInfo to avoid memory junk
    esp_now_peer_info_t peerInfo = {}; 
    memcpy(peerInfo.peer_addr, gatewayAddress, 6);
    peerInfo.channel = 1; // Force Channel 1 for transmission
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK){
        Serial.println("❌ Failed to add Gateway peer");
        _radioReady = false;
    }
}

void networkLoop() {
    // ESP-NOW does not require manual keep-alive or reconnection.
    // Radio stays active listening for packets.
}

bool isConnected() {
    return _radioReady;
}

OrderInfo requestOrderInfo() {
    OrderInfo info = {0, ""};
    if (!isConnected()) return info;

    _orderReceived = false;

    // Monta o pacote e joga no ar
    CommandMessage msg;
    msg.type = MSG_TYPE_CMD;
    strcpy(msg.text, "GET_ORDER");
    esp_now_send(gatewayAddress, (uint8_t *) &msg, sizeof(msg));

    // Wait for asynchronous Gateway response (max 2s)
    unsigned long start = millis();
    while (!_orderReceived) {
        if (millis() - start > 2000) {
            Serial.println("⚠️ Timeout waiting for Order.");
            return info; 
        }
        delay(10); // Prevent Watchdog Reset
    }
    
    return _lastReceivedOrder;
}

void sendStatus(String status) {
    if (!isConnected()) return;

    CommandMessage msg;
    msg.type = MSG_TYPE_CMD;
    String payload = "STATUS:" + status;
    payload.toCharArray(msg.text, 100);
    
    esp_now_send(gatewayAddress, (uint8_t *) &msg, sizeof(msg));
}

bool sendLog(long orderId, double cycleTime, double pausedTime, int qtyProd, int qtyPaused) {
    if (!isConnected()) return false;
    
    _ackReceived = false;

    // Fast binary struct assembly (no heavy JSON)
    LogMessage logMsg;
    logMsg.type = MSG_TYPE_LOG;
    logMsg.orderId = orderId;
    logMsg.cycleTime = cycleTime;
    logMsg.pausedTime = pausedTime;
    logMsg.qtyProd = qtyProd;
    logMsg.qtyPaused = qtyPaused;

    // Broadcast data
    esp_now_send(gatewayAddress, (uint8_t *) &logMsg, sizeof(logMsg));

    // Wait for database write confirmation (max 4s)
    unsigned long start = millis();
    while (!_ackReceived) { 
        if (millis() - start > 4000) {
            Serial.println("❌ Timeout waiting for OK_LOG confirmation");
            return false;
        }
        delay(10);
    }

    return true; // Se chegou aqui, _ackReceived ficou true!
}

bool pingServer() {
    if (!isConnected()) return false;
    
    _pongReceived = false;

    CommandMessage msg;
    msg.type = MSG_TYPE_CMD;
    strcpy(msg.text, "PING");
    esp_now_send(gatewayAddress, (uint8_t *) &msg, sizeof(msg));

    // Wait up to 2.5s for server response
    unsigned long start = millis();
    while (!_pongReceived) {
        if (millis() - start > 2500) {
            return false; 
        }
        delay(10);
    }
    
    return true; // Success: OK_PONG received!
}