#include <WiFi.h>
#include <ArduinoJson.h>
#include "network.h"

WiFiClient client;
String _ssid, _pass, _serverIP;
int _serverPort;
unsigned long _lastPing = 0;
unsigned long _lastWifiAttempt = 0;
unsigned long _lastTcpAttempt = 0;

void networkInit(const char* ssid, const char* pass, const char* serverIP, int port) {
    _ssid = ssid;
    _pass = pass;
    _serverIP = serverIP;
    _serverPort = port;
    
    Serial.println("--- INICIANDO REDE ---");
    Serial.print("SSID Alvo: "); Serial.println(_ssid);
    Serial.print("Server Alvo: "); Serial.print(_serverIP); Serial.print(":"); Serial.println(_serverPort);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true); // Deixa o ESP32 gerenciar quedas rápidas
    WiFi.begin(_ssid.c_str(), _pass.c_str());
}

void networkLoop() {
    unsigned long now = millis();

    // 1. Gerenciamento do Wi-Fi
    if (WiFi.status() != WL_CONNECTED) {
        // Tenta reconectar a cada 15 segundos para não travar o DHCP do roteador
        if (now - _lastWifiAttempt > 15000) {
            _lastWifiAttempt = now;
            Serial.print("[WiFi] Tentando reconectar ao SSID: ");
            Serial.println(_ssid);
            WiFi.disconnect();
            WiFi.begin(_ssid.c_str(), _pass.c_str());
        }
        return; // Se não tem Wi-Fi, não tenta TCP
    }

    // 2. Gerenciamento do Servidor TCP
    if (!client.connected()) {
        if (now - _lastTcpAttempt > 3000) { // Tenta TCP a cada 3 segundos
            _lastTcpAttempt = now;
            
            Serial.print("[TCP] Tentando conectar ao Server: ");
            Serial.println(_serverIP);

            if (client.connect(_serverIP.c_str(), _serverPort)) {
                Serial.println("✅ [TCP] CONECTADO! Enviando Handshake...");
                client.println("ID:" + WiFi.macAddress());
                unsigned long start = millis();
                while (!client.available() && millis() - start < 1000) delay(10);
                while (client.available()) client.read(); // Limpa buffer
            } else {
                Serial.println("❌ [TCP] Falha na conexão. Servidor desligado ou porta bloqueada.");
            }
        }
    } else {
        // 3. Keep-Alive (Ping)
        if (now - _lastPing > 5000) {
            _lastPing = now;
            client.println("PING");
            // Nota: Limpar o buffer agressivamente pode engolir respostas assíncronas do servidor.
            // Se notar perda de dados, comente o while abaixo no futuro.
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

    while(client.available()) client.read(); // Limpa lixo do buffer
    
    client.println("GET_ORDER");

    unsigned long start = millis();
    while (!client.available()) {
        if (millis() - start > 2000) return info; // Timeout de 2 segundos
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

    JsonDocument doc; // Suporta ArduinoJson v7
    doc["orderId"] = orderId;
    doc["cycleTime"] = cycleTime;
    doc["pausedTime"] = pausedTime;
    doc["quantityProduced"] = qtyProd;
    doc["quantityPaused"] = qtyPaused;

    String jsonStr;
    serializeJson(doc, jsonStr);
    
    client.println(jsonStr);
    
    unsigned long start = millis();
    while (millis() - start < 4000) { // Timeout de 4 segundos esperando resposta
        if (client.available()) {
            String line = client.readStringUntil('\n');
            line.trim();
            if (line == "OK_LOG") {
                return true; 
            }
        }
        delay(10);
    }

    Serial.println("❌ Timeout esperando OK_LOG do servidor");
    return false;
}