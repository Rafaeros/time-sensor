#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>

struct OrderInfo {
    long id;
    String code;
};

// Initialization requires the Gateway MAC address
void networkInit(uint8_t *gatewayMac);
bool pingServer();
void networkLoop();
bool isConnected();
OrderInfo requestOrderInfo();
void sendStatus(String status);
bool sendLog(long orderId, double cycleTime, double pausedTime, int qtyProd, int qtyPaused);

#endif