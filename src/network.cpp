#include <Arduino.h>
#include <WiFi.h>
#include "network.h"

static WiFiClient client;
static String _serverIP;
static int _serverPort;

unsigned long lastReconnectAttempt = 0;

// ------------------------
//  Iniciar WiFi
// ------------------------
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
}

// ------------------------
//  Reconectar WiFi
// ------------------------
void ensureWiFi()
{
    if (WiFi.status() == WL_CONNECTED)
        return;

    Serial.println("[WiFi] PERDEU A CONEXAO! Tentando voltar...");

    WiFi.disconnect();
    WiFi.reconnect();

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 5000)
    {
        delay(200);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED)
        Serial.println("\n[WiFi] Reconectado!");
}

// ------------------------
//  Reconectar TCP
// ------------------------
bool ensureTCP()
{
    if (client.connected())
        return true;

    Serial.println("[TCP] Desconectado! Tentando reconectar...");

    client.stop();
    delay(150);

    if (client.connect(_serverIP.c_str(), _serverPort, 2000))
    {
        Serial.println("[TCP] Reconectado!");
        return true;
    }

    Serial.println("[TCP] Falha ao reconectar.");
    return false;
}

// ------------------------
//  Loop opcional (manutenção)
// ------------------------
void networkLoop()
{
    unsigned long now = millis();

    if (now - lastReconnectAttempt >= 2000)
    {
        lastReconnectAttempt = now;
        ensureWiFi();
        ensureTCP();
    }
}

// ------------------------
//   Envio confiável
// ------------------------
void sendData(const char *codigo,
              unsigned long tempoProd,
              unsigned long tempoPause,
              unsigned long tempoTotal,
              int qtd)
{
    // ========== GARANTE WIFI ==========
    ensureWiFi();
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("[WiFi] Sem WiFi, não enviando.");
        return;
    }

    // ========== GARANTE TCP ==========
    if (!ensureTCP())
    {
        Serial.println("[TCP] Sem TCP, não enviando.");
        return;
    }

    // ---------- Monta mensagem ----------
    String msg =
        String(codigo) + ";" +
        tempoProd + ";" +
        tempoPause + ";" +
        tempoTotal + ";" +
        qtd + "\n";

    // ---------- Envia ----------
    size_t enviado = client.write((const uint8_t *)msg.c_str(), msg.length());

    if (enviado != msg.length())
    {
        Serial.println("[TCP] Erro ao enviar! Tentando reconectar...");

        // PRIMEIRA TENTATIVA DE RECONECTAR
        if (!ensureTCP())
        {
            Serial.println("[TCP] Falha no retry.");
            return;
        }

        // SEGUNDA TENTATIVA DE ENVIO
        enviado = client.write((const uint8_t *)msg.c_str(), msg.length());

        if (enviado != msg.length())
        {
            Serial.println("[TCP] Falhou mesmo após reconectar!");
            return;
        }
    }

    Serial.print("[TCP] Enviado: ");
    Serial.println(msg);
}
