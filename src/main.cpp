#include <Arduino.h>
#include "leds.h"
#include "timer.h"
#include "network.h"

#define LED_R 4
#define LED_G 5
#define BUZZER_PIN 18
#define SWITCH_PIN 27
#define PAUSE_PIN 23

// tempo mínimo para enviar (só envia se for maior que isso)
const int TEMPO_LIMITE = 5;

// Flags e variáveis internas
bool ultimoEstado = false;
bool bipFeito = false;

unsigned long lastBlink = 0;
bool blinkState = false;

unsigned long producaoStart = 0;
unsigned long pausaStart = 0;

unsigned long tempoProducao = 0;
unsigned long tempoPausa = 0;

void setup()
{
    Serial.begin(115200);

    ledsInit(LED_R, LED_G);
    pinMode(PAUSE_PIN, INPUT_PULLUP);
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);

    networkInit("", "", "10.48.0.113", 5050);
}

void loop()
{
    bool ativo = (digitalRead(SWITCH_PIN) == LOW);      // sensor ativo
    bool pausePressionado = (digitalRead(PAUSE_PIN) == LOW); // botão de pausa

    // ======================================================
    //  SENSOR DESATIVADO → FINALIZA CICLO E ENVIA TEMPOS
    // ======================================================
    if (!ativo)
    {
        setRGB(1, 0); // LED vermelho

        // Finaliza tempos se estavam rolando
        if (producaoStart > 0)
            tempoProducao += (millis() - producaoStart);

        if (pausaStart > 0)
            tempoPausa += (millis() - pausaStart);

        unsigned long tempoTotal = tempoProducao + tempoPausa;

        if (tempoTotal > TEMPO_LIMITE * 1000)
        {
            // Envia: código, produção, pausa, total, qtd (sempre 1)
            sendData(
                "TKC110 002 002",
                tempoProducao / 1000,
                tempoPausa / 1000,
                tempoTotal / 1000,
                1
            );
        }

        // Reset das variáveis
        producaoStart = 0;
        pausaStart = 0;
        tempoProducao = 0;
        tempoPausa = 0;
        bipFeito = false;
        ultimoEstado = false;

        return;
    }

    // ======================================================
    //  ATIVO + PAUSE → PISCAR LED E CONTAR PAUSA
    // ======================================================
    if (pausePressionado)
    {
        // Pausa produção
        if (producaoStart > 0)
        {
            tempoProducao += (millis() - producaoStart);
            producaoStart = 0;
        }

        // Inicia pausa
        if (pausaStart == 0)
            pausaStart = millis();

        // Piscar LED vermelho / verde
        if (millis() - lastBlink >= 300)
        {
            lastBlink = millis();
            blinkState = !blinkState;

            if (blinkState)
                setRGB(1, 0);
            else
                setRGB(0, 1);
        }

        return;
    }

    // ======================================================
    //  ATIVO SEM PAUSE → CONTAGEM DE PRODUÇÃO
    // ======================================================

    setRGB(0, 1); // LED verde

    // Parar contagem de pausa se estava pausado
    if (pausaStart > 0)
    {
        tempoPausa += (millis() - pausaStart);
        pausaStart = 0;
    }

    // Inicia contagem de produção
    if (producaoStart == 0)
        producaoStart = millis();

    // ---------- BIP NA TRANSIÇÃO ----------
    if (ativo && !ultimoEstado && !bipFeito)
    {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);

        bipFeito = true;
    }

    ultimoEstado = true;

    // Debug
    Serial.print("Produção: ");
    Serial.print((tempoProducao + (producaoStart ? millis() - producaoStart : 0)) / 1000);
    Serial.print("s | Pausa: ");
    Serial.print((tempoPausa + (pausaStart ? millis() - pausaStart : 0)) / 1000);
    Serial.println("s");
}
