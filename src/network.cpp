#include <Arduino.h>
#include <WiFi.h>
#include "network.h"

static WiFiClient client;

void networkInit(const char* ssid, const char* pass, const char* serverIP, int port) {
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(300);
    }

    client.connect(serverIP, port);
}

void sendData(const char* codigo, unsigned long tempo, int qtd) {
    if (!client.connected()) return;

    String msg = String(codigo) + ";" + tempo + ";" + qtd;
    client.println(msg);
}
