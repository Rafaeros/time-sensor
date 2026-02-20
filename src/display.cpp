#include "display.h"
#include <U8g2lib.h>
#include <Wire.h>

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);

bool hasDisplay = false;

void displayInit()
{
    Wire.begin(21, 22);

    Wire.beginTransmission(0x3C);
    if (Wire.endTransmission() == 0)
    {
        hasDisplay = true;
        u8g2.begin();
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(0, 15, "Iniciando...");
        u8g2.sendBuffer();
    }
    else
    {
        hasDisplay = false;
        Serial.println("OLED not found. Running headless mode.");
    }
}

void updateStatus(String status, String subInfo)
{
    if (!hasDisplay)
        return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB10_tr);
    u8g2.drawStr(0, 16, status.c_str());

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.drawStr(0, 35, subInfo.c_str());

    u8g2.sendBuffer();
}

void displayProduction(String opCode, unsigned long seconds, int qty)
{
    if (!hasDisplay)
        return;

    unsigned long h = seconds / 3600;
    unsigned long m = (seconds % 3600) / 60;
    unsigned long s = seconds % 60;
    char timeStr[10];
    sprintf(timeStr, "%02lu:%02lu:%02lu", h, m, s);

    u8g2.clearBuffer();

    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(0, 10);
    u8g2.print(opCode);
    u8g2.setCursor(80, 10);
    u8g2.print("Qty: ");
    u8g2.print(qty);

    u8g2.setFont(u8g2_font_logisoso24_tr);
    u8g2.setCursor(0, 45);
    u8g2.print(timeStr);

    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(0, 60, "PRODUZINDO...");

    u8g2.sendBuffer();
}

void displayPaused(unsigned long seconds)
{
    if (!hasDisplay)
        return;

    unsigned long m = seconds / 60;
    unsigned long s = seconds % 60;
    char timeStr[10];
    sprintf(timeStr, "%02lu:%02lu", m, s);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB12_tr);
    u8g2.drawStr(20, 15, "PAUSADO");

    u8g2.setFont(u8g2_font_profont12_tr);
    u8g2.drawStr(0, 35, "Tempo Pausa:");

    u8g2.setFont(u8g2_font_helvB18_tr);
    u8g2.setCursor(30, 58);
    u8g2.print(timeStr);

    u8g2.sendBuffer();
}

void displayError(String msg)
{
    if (!hasDisplay)
        return;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_helvB10_tr);
    u8g2.drawStr(0, 15, "ERRO");
    u8g2.setFont(u8g2_font_6x10_tr);
    u8g2.setCursor(0, 35);
    u8g2.print(msg);
    u8g2.sendBuffer();
}