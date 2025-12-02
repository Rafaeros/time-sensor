#ifndef NETWORK_H
#define NETWORK_H

void networkInit(const char* ssid, const char* pass, const char* serverIP, int port);
void networkLoop();

void sendData(const char* codigo,
              unsigned long tempoProd,
              unsigned long tempoPause,
              unsigned long tempoTotal,
              int qtd);

void ensureWiFi();
bool ensureTCP();

#endif
