#include <Arduino.h>
#include "sensor.h"
#include "leds.h"
#include "timer.h"
#include "network.h"

#define LED_R 4
#define LED_G 5
#define SWITCH_PIN 7
#define TRIG_PIN 8
#define ECHO_PIN 9

const int DIST_THRESHOLD = 20;

void setup()
{
    Serial.begin(115200);

    sensorInit(TRIG_PIN, ECHO_PIN);
    ledsInit(LED_R, LED_G);

    pinMode(SWITCH_PIN, INPUT_PULLUP);

    networkInit("WIFI_SSID", "WIFI_PASSWORD", "10.48.0.188", 5050);
}

void loop()
{
    bool switchState = digitalRead(SWITCH_PIN);
    long dist = measureDistanceCM();

    // NADA PERTO → LED vermelho → reseta tempo
    if (dist > DIST_THRESHOLD)
    {
        setRGB(1, 0);

        unsigned long tempo = timerGetSeconds();

        if (tempo > 20)
        {
            sendData("MWM035 000 000", tempo, 1);
        }

        timerReset();
        return;
    }

    // SENSOR ATIVO + SWITCH DESLIGADO → verde fixo
    if (switchState == HIGH)
    {
        setRGB(0, 1);
        timerStart();

        Serial.print("Tempo ativo: ");
        Serial.println(timerGetSeconds());
        return;
    }

    // SENSOR ATIVO + SWITCH LIGADO → piscar
    static unsigned long prev = 0;
    static bool state = false;

    if (millis() - prev >= 500)
    {
        prev = millis();
        state = !state;
        setRGB(state, !state);
    }

    timerPause();
}
