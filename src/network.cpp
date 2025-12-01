#include <Arduino.h>
#include <WiFi.h>
#include "network.h"

static WiFiClient client;
static String _serverIP;
static int _serverPort;

bool safeConnect();

void networkInit(const char *ssid, const char *pass, const char *serverIP, int port)
{
    _serverIP = serverIP;
    _serverPort = port;

    Serial.println("[WiFi] Conectando...");
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }

    Serial.println("\n[WiFi] Conectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    safeConnect();
}

// ------------------------
//  Conexão com verificação
// ------------------------
bool safeConnect()
{
    Serial.println("[TCP] Conectando...");

    client.stop(); // limpa estado antigo
    delay(100);

    if (client.connect(_serverIP.c_str(), _serverPort, 2000))
    {
        Serial.println("[TCP] Conectado!");
        return true;
    }

    Serial.println("[TCP] Falha na conexão!");
    return false;
}

// ------------------------
//  Envio seguro
// ------------------------
void sendData(const char *codigo,
              unsigned long tempoProd,
              unsigned long tempoPause,
              unsigned long tempoTotal,
              int qtd)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[WiFi] Desconectado!.");
        return;
    }

    if (!client.connected())
    {
        Serial.println("[TCP] Socket morto! Reconectando...");
        if (!safeConnect())
            return;
    }

    String msg =
        String(codigo) + ";" +
        tempoProd + ";" +
        tempoPause + ";" +
        tempoTotal + ";" +
        qtd + "\n";

    size_t enviado = client.write((const uint8_t *)msg.c_str(), msg.length());

    if (enviado != msg.length())
    {
        Serial.println("[TCP] Erro ao enviar! Reconectando...");
        safeConnect();
        return;
    }

    Serial.print("[TCP] Enviado: ");
    Serial.println(msg);
}
