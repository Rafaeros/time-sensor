#include <Arduino.h>
#include "leds.h"
#include "timer.h"
#include "network.h"

#define LED_R 4
#define LED_G 5
#define BUZZER_PIN 18
#define SWITCH_PIN 27
#define PAUSE_PIN 23

const int TEMPO_LIMITE = 5; // segundos
bool ultimoEstado = false;   // false = desligado, true = ligado
bool bipFeito = false;

bool ativadoAnterior = false;
unsigned long lastBlink = 0;
bool blinkState = false;


void setup()
{
    Serial.begin(115200);

    ledsInit(LED_R, LED_G);
    pinMode(PAUSE_PIN, INPUT_PULLUP);
    pinMode(SWITCH_PIN, INPUT_PULLUP);
    pinMode(BUZZER_PIN, OUTPUT);

    networkInit("", "", "10.48.0.188", 5050);
}

void loop()
{
    bool ativo = (digitalRead(SWITCH_PIN) == LOW); // switch pressionado/ativado
    bool pausePressionado = (digitalRead(PAUSE_PIN) == LOW);
    // -----------------------
    // SWITCH DESLIGADO → LED vermelho → zera tempo
    // -----------------------
    if (!ativo)
    {
        setRGB(1, 0); // vermelho

        unsigned long t = timerGetSeconds();

        if (t > TEMPO_LIMITE)
        {
            sendData("MWM035 000 000", t, 1);
        }

        bipFeito = false;   // permite novo bip quando ativar de novo
        ultimoEstado = false;

        timerReset();
        return;
    }

    if (pausePressionado)
    {
        // Pausar a contagem
        timerPause();

        // Piscar LED vermelho ↔ verde
        if (millis() - lastBlink >= 300)
        {
            lastBlink = millis();
            blinkState = !blinkState;

            if (blinkState)
                setRGB(1, 0); // vermelho
            else
                setRGB(0, 1); // verde
        }

        return;
    }

    // -----------------------
    // SWITCH LIGADO → LED verde → contar tempo
    // -----------------------
    setRGB(0, 1); // verde

    timerStart();

     // ---------- BIP NA TRANSIÇÃO ----------
    if (ativo && !ultimoEstado && !bipFeito)
    {
        digitalWrite(BUZZER_PIN, HIGH);
        delay(1000);  // bip curto 1s
        digitalWrite(BUZZER_PIN, LOW);

        bipFeito = true;
    }
    ultimoEstado = true;
    Serial.print("Tempo ativo: ");
    Serial.println(timerGetSeconds());
}
