![Licença](https://img.shields.io/badge/Licen%C3%A7a-Comercial_Restrita-red)

# 🏭 Monitoramento de Produção com Heltec ESP32 V3

Sistema de controle de produção e pausa com envio de dados via TCP

Este projeto utiliza um Heltec V3 para monitorar o ciclo de produção de uma máquina ou posto de trabalho.
Ele contabiliza automaticamente a ordem de produção, tempo de produção, o tempo de pausa, quantidade de pausas e o tempo total, enviando essas informações para um servidor via TCP/IP.

Materiais necessários:

🚦 1 un. - Led RGB Alto Brilho 5mm - Cátodo Comum 

⚡️ 3 un. - Resistor 1KΩ

🔊 1 un. - Buzzer Piezo (EPI-2412A-3312L)

🎛️ 1 un. - Chave principal (Chave Microswitch 3 Pinos P/ Fio C/ Alavanca (D2F-01FL-D)

⏸️ 1 un. - Botão de pausa (Botão On Off PBS-11A / Chave Push Button3A com Trava)

📱 1 un. - Heltec ESP32 V3

📦 1 un. - Caixa Patola PB107-PR

<hr/>

## 📌 Funcionalidades

### 🚦 Indicação por LED

- Vermelho fixo → Desconectado do Servidor
- Verde fixo → Aguardando (Conectado)
- Azul piscando → Produzindo (Chave Principal Acionada)
- Amarelo piscando → Pausado

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

graph TB

    %% =========================
    %% PLACA HELTEC V3 (Top View)
    %% =========================
    subgraph BOARD[Heltec WiFi LoRa 32 V3 - Vista Superior]

        direction LR

        %% Lado esquerdo (Entradas)
        subgraph LEFT[Entradas]
            direction TB
            GPIO46[GPIO 46<br/>SW_PROD]
            GPIO45[GPIO 45<br/>BTN_PAUSE]
        end

        %% Centro (Core)
        subgraph CENTER[ESP32 Core]
            direction TB
            MCU[ESP32]
        end

        %% Lado direito (LED RGB)
        subgraph RIGHT[Saídas LED RGB]
            direction TB
            GPIO36[GPIO 36 - R]
            GPIO37[GPIO 37 - G]
            GPIO35[GPIO 35 - B]
        end

    end

    %% Parte superior (Alimentação)
    PSU[Fonte 5V]
    VIN[5V]
    GND[GND]

    PSU -->|5V| VIN
    PSU --> GND

    %% Conexões externas
    Switch[Chave Produção]
    PauseBtn[Botão Pausa]

    LEDR[LED Vermelho]
    LEDG[LED Verde]
    LEDB[LED Azul]

    R1[1kΩ]
    R2[1kΩ]
    R3[1kΩ]

    Buzzer[Buzzer]
    GPIO48[GPIO 48]

    %% Entradas
    GPIO46 -->|INPUT_PULLUP| Switch --> GND
    GPIO45 -->|INPUT_PULLUP| PauseBtn --> GND

    %% LEDs com resistores
    GPIO36 --> R1 --> LEDR --> GND
    GPIO37 --> R2 --> LEDG --> GND
    GPIO35 --> R3 --> LEDB --> GND

    %% Buzzer
    GPIO48 --> Buzzer --> GND


    
```

<hr/>

![Heltec](images/pinout.png)

<hr/>

### 🚀 Como Usar

1. Configure os dados do Wi-Fi no main.cpp

2. Suba o código no Heltec V3 (PlatformIO recomendado)

3. Execute o servidor TCP na máquina destino (porta 5050 por padrão)

4. Inicie o ciclo com a chave → LED Azul piscando

5. Pressione o botão de pausa → LED Amarelo Piscando

6. Desligue a chave → O heltec envia os tempos automaticamente

<hr/>

## 📄 Licença

Este projeto está sob a Licença de Uso Não-Comercial Rafaeros. Consulte o arquivo [LICENSE](LICENSE) para obter os detalhes completos sobre os termos de uso e as restrições para fins lucrativos.