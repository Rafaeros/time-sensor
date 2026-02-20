![Licença](https://img.shields.io/badge/Licen%C3%A7a-Comercial_Restrita-red)

# 🏭 Monitoramento de Produção com ESP32 Devkit V1

Sistema de controle de produção e pausa com envio de dados via TCP.

Este projeto utiliza um **ESP32 Devkit V1** para monitorar o ciclo de
produção de uma máquina ou posto de trabalho.

Ele contabiliza automaticamente:

-   Ordem de Produção
-   Tempo de Produção
-   Tempo de Pausa
-   Quantidade de Pausas
-   Tempo Total

Os dados são enviados para um servidor via **TCP/IP (Socket)**.

------------------------------------------------------------------------

# 📦 Materiais Necessários

-   🚦 1x Led RGB Alto Brilho 5mm - Cátodo Comum
-   ⚡️ 3x Resistores 1KΩ
-   🔊 1x Buzzer Piezo (EPI-2412A-3312L)
-   🎛️ 1x Chave principal (Chave Microswitch 3 Pinos P/ Fio C/ Alavanca (D2F-01FL-D)
-   ⏸️ 1x Botão de pausa (Botão On Off PBS-11A / Chave Push Button3A com Trava)
-   📱 1x ESP32 Devkit V1
-   📺 1x Display OLED 128x64 I2C (SSD1306)
-   📦 1x Caixa Patola PB107-PR

------------------------------------------------------------------------

# 📺 Display OLED (I2C)

Configuração utilizada:

``` cpp
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0,
    U8X8_PIN_NONE,
    22,   // SCL
    21    // SDA
);
```

### 📌 Ligações do OLED

  OLED   ESP32 Devkit V1
  ------ -----------------
  VCC    3.3V
  GND    GND
  SCL    GPIO 22
  SDA    GPIO 21

------------------------------------------------------------------------

# 📌 Funcionalidades

## 🚦 Indicação por LED

-   🔴 Vermelho fixo → Desconectado do Servidor\
-   🟢 Verde fixo → Conectado / Aguardando\
-   🔵 Azul piscando → Produzindo\
-   🟡 Amarelo piscando → Pausado

------------------------------------------------------------------------

## 🔊 Buzzer

-   Bips curtos até conectar ao servidor
-   Um bip curto ao iniciar o ciclo

------------------------------------------------------------------------

## 🌐 Comunicação

-   Conexão Wi-Fi
-   Comunicação via Socket TCP
-   Porta padrão: **5050**

------------------------------------------------------------------------

# 📡 Formato do Envio de Dados

Campos enviados ao servidor:

-   `orderId`
-   `cycleTime` (segundos)
-   `pausedTime` (segundos)
-   `quantityProduced` (padrão = 1)
-   `quantityPaused`

------------------------------------------------------------------------

# 🔧 Ligações de Hardware (ESP32 Devkit V1)

## 📥 Entradas

  Função           GPIO
  ---------------- ---------
  Chave Produção   GPIO 26
  Botão Pausa      GPIO 27

Configuração recomendada:

``` cpp
pinMode(26, INPUT_PULLUP);
pinMode(27, INPUT_PULLUP);
```

------------------------------------------------------------------------

## 📤 Saídas

  Função         GPIO
  -------------- ---------
  LED Vermelho   GPIO 18
  LED Verde      GPIO 19
  LED Azul       GPIO 23
  Buzzer         GPIO 25

------------------------------------------------------------------------

## 🔌 Esquema Simplificado

``` mermaid
graph TB

    PSU[Fonte 5V] --> VIN[5V ESP32]
    PSU --> GND

    GPIO26[GPIO 26] --> Switch[Chave Produção] --> GND
    GPIO27[GPIO 27] --> PauseBtn[Botão Pausa] --> GND

    GPIO18 --> R1[1kΩ] --> LEDR --> GND
    GPIO19 --> R2[1kΩ] --> LEDG --> GND
    GPIO23 --> R3[1kΩ] --> LEDB --> GND

    GPIO25 --> Buzzer --> GND

    GPIO22 --> OLED_SCL[SCL]
    GPIO21 --> OLED_SDA[SDA]
```

------------------------------------------------------------------------

# 🚀 Como Usar

1.  Configure Wi-Fi no `main.cpp`
2.  Configure IP e Porta do servidor
3.  Compile e envie via PlatformIO
4.  Ligue o equipamento
5.  Inicie produção acionando a chave
6.  Pause quando necessário
7.  Ao desligar a chave, os dados são enviados automaticamente

------------------------------------------------------------------------

# 📄 Licença

Este projeto está sob a Licença de Uso Não‑Comercial Rafaeros. Consulte
o arquivo `LICENSE` para mais detalhes.