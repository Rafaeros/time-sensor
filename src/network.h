#ifndef NETWORK_H
#define NETWORK_H
#include <Arduino.h>

struct OrderInfo {
    long id;
    String code;
};

void networkInit(const char* ssid, const char* pass, const char* serverIP, int port);
void networkLoop();
bool isConnected();

OrderInfo requestOrderInfo(); 

void sendStatus(String status);
bool sendJsonLog(long orderId, double cycleTime, double pausedTime, int qtyProd, int qtyPaused);

#endif