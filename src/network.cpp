#include <WiFi.h>
#include <ArduinoJson.h>
#include "network.h"

WiFiClient client;
String _ssid, _pass, _serverIP;
int _serverPort;
unsigned long _lastPing = 0;
unsigned long _lastReconnectAttempt = 0;

void networkInit(const char* ssid, const char* pass, const char* serverIP, int port) {
    _ssid = ssid;
    _pass = pass;
    _serverIP = serverIP;
    _serverPort = port;
    
    Serial.println("--- INICIANDO REDE ---");
    Serial.print("SSID Alvo: "); Serial.println(_ssid);
    Serial.print("Server Alvo: "); Serial.print(_serverIP); Serial.print(":"); Serial.println(_serverPort);

    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid.c_str(), _pass.c_str());
}

void networkLoop() {
    if (WiFi.status() != WL_CONNECTED) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > 5000) {
            _lastReconnectAttempt = now;
            Serial.print("[WiFi] Tentando reconectar ao SSID: ");
            Serial.println(_ssid);
            
            WiFi.disconnect();
            WiFi.reconnect();
            WiFi.begin(_ssid.c_str(), _pass.c_str());
        }
        return;
    }
    if (!client.connected()) {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt > 3000) {
            _lastReconnectAttempt = now;
            
            Serial.print("[WiFi] Conectado! IP Local: ");
            Serial.println(WiFi.localIP());
            Serial.print("[TCP] Tentando conectar ao Server: ");
            Serial.println(_serverIP);

            if (client.connect(_serverIP.c_str(), _serverPort)) {
                Serial.println("✅ [TCP] CONECTADO! Enviando Handshake...");
                client.println("ID:" + WiFi.macAddress());
                unsigned long start = millis();
                while (!client.available() && millis() - start < 1000) delay(10);
                while (client.available()) client.read();
            } else {
                Serial.println("❌ [TCP] Falha na conexão. Verifique IP e Firewall.");
            }
        }
    } else {
        if (millis() - _lastPing > 5000) {
            _lastPing = millis();
            client.println("PING");
            while(client.available()) client.read();
        }
    }
}

bool isConnected() {
    return WiFi.status() == WL_CONNECTED && client.connected();
}

OrderInfo requestOrderInfo() {
    OrderInfo info = {0, ""};
    
    if (!isConnected()) return info;

    while(client.available()) client.read(); 
    
    client.println("GET_ORDER");

    unsigned long start = millis();
    while (!client.available()) {
        if (millis() - start > 2000) return info;
        delay(10);
    }
    
    String resp = client.readStringUntil('\n');
    resp.trim(); 
    if (resp.startsWith("ORDER:")) {
        int firstColon = resp.indexOf(':');
        int secondColon = resp.indexOf(':', firstColon + 1);
        
        if (secondColon > 0) {
            String idStr = resp.substring(firstColon + 1, secondColon);
            String codeStr = resp.substring(secondColon + 1);
            
            info.id = idStr.toInt();
            info.code = codeStr;
        } else {
            info.id = resp.substring(6).toInt();
            info.code = "OP-" + String(info.id);
        }
    }
    return info;
}

void sendStatus(String status) {
    if (isConnected()) client.println("STATUS:" + status);
}

bool sendJsonLog(long orderId, double cycleTime, double pausedTime, int qtyProd, int qtyPaused) {
    if (!isConnected()) return false;
    while(client.available()) client.read();

    JsonDocument doc;
    doc["orderId"] = orderId;
    doc["cycleTime"] = cycleTime;
    doc["pausedTime"] = pausedTime;
    doc["quantityProduced"] = qtyProd;
    doc["quantityPaused"] = qtyPaused;

    String jsonStr;
    serializeJson(doc, jsonStr);
    
    client.println(jsonStr);
    unsigned long start = millis();
    while (millis() - start < 4000) {
        
        if (client.available()) {
            String line = client.readStringUntil('\n');
            line.trim();
            if (line == "OK_LOG") {
                return true; 
            }
            
        }
        delay(10);
    }

    Serial.println("Timeout esperando OK_LOG");
    return false;
}