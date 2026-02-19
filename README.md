# 🏭 Monitoramento de Produção com Heltec V3

Sistema de controle de produção e pausa com envio de dados via TCP

Este projeto utiliza um Heltec V3 para monitorar o ciclo de produção de uma máquina ou posto de trabalho.
Ele contabiliza automaticamente a ordem de produção, tempo de produção, o tempo de pausa, quantidade de pausas e o tempo total, enviando essas informações para um servidor via TCP/IP.

O dispositivo possui:

🚦 LED RGB (VM/VD/AM/AZ) para indicar os status

🔊 Buzzer para sinal sonoro até conexão com servidor e iniciar a contagem de tempo.

🎛️ Chave principal (liga/desliga do ciclo de produção)

⏸️ Botão de pausa

🌐 Envio de dados para servidor SOCKET TCP

<hr/>

## 📌 Funcionalidades

### 🚦 Indicação por LED

- Vermelho fixo → Desconectado do Servidor
- Verde fixo → Aguardando (Conectado)
- Azul fixo → Produzindo (Chave Principal Acionada)
- Amarelo fixo → Pausado

<hr/>

### 🔊 Buzzer

- Bips curtos até conexão com o servidor.
- Um bip curto é emitido ao iniciar o ciclo.

<hr/>

### 🌐 Comunicação

- O módulo se conecta a um Wi-Fi para coletar dados da ordem de produção atual e envia informações para um servidor via SOCKET TCP.

<hr/>

### 📡 Formato do Envio de Dados

O Heltec V3 envia os seguintes campos:

- orderId (ID da Ordem de Produção)
- cycleTime (Tempo do Ciclo de Produção (S))
- pausedTime (Tempo de Pausa durante o ciclo (S))
- quantityProduced (Quantidade Produzida por padrão = 1)
- quantityPaused (Quantidade de Pausas durante o ciclo)

<hr/>

### 🔧 Ligações (Hardware)

```mermaid

graph TD

    subgraph HELTEC_V3[Heltec WiFi LoRa 32 V3]
        direction TB
        
        VIN[5V]
        GND[GND]
        GPIO46[GPIO 46]
        GPIO45[GPIO 45]

        %% LEDs lado a lado
        subgraph LED_PINS[GPIOs LED RGB]
            direction LR
            GPIO36[GPIO 36 - R]
            GPIO37[GPIO 37 - G]
            GPIO35[GPIO 35 - B]
        end

        GPIO48[GPIO 48]
    end

    PSU[Fonte 5V]
    Switch[Chave Produção]
    PauseBtn[Botão Pausa]

    LEDR[LED RGB - Vermelho]
    LEDG[LED RGB - Verde]
    LEDB[LED RGB - Azul]

    R1[Resistor 1kΩ]
    R2[Resistor 1kΩ]
    R3[Resistor 1kΩ]

    Buzzer[Buzzer Ativo]

    %% Alimentação
    PSU -->|5V| VIN
    PSU --> GND

    %% Entradas
    GPIO46 -->|INPUT_PULLUP| Switch --> GND
    GPIO45 -->|INPUT_PULLUP| PauseBtn --> GND

    %% LED RGB com resistores
    GPIO36 --> R1 --> LEDR --> GND
    GPIO37 --> R2 --> LEDG --> GND
    GPIO35 --> R3 --> LEDB --> GND

    %% Buzzer
    GPIO48 --> Buzzer --> GND

    
```

<hr/>

### 🚀 Como Usar

1. Configure os dados do Wi-Fi no main.cpp

2. Suba o código no Heltec V3 (PlatformIO recomendado)

3. Execute o servidor TCP na máquina destino (porta 5050 por padrão)

4. Inicie o ciclo com a chave → LED verde piscando

5. Pressione o botão de pausa → LED amarelo piscando

6. Desligue a chave → O heltec envia os tempos automaticamente

<hr/>