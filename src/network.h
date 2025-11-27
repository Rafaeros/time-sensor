#ifndef NETWORK_H
#define NETWORK_H

void networkInit(const char* ssid, const char* pass, const char* serverIP, int port);
void sendData(const char* codigo, unsigned long tempo, int qtd);

#endif