# 🏭 Monitoramento de Produção com ESP32

Sistema de controle de produção e pausa com envio de dados via TCP

Este projeto utiliza um ESP32 para monitorar o ciclo de produção de uma máquina ou posto de trabalho.
Ele contabiliza automaticamente o tempo de produção, o tempo de pausa e o tempo total, enviando essas informações para um servidor via TCP/IP.

O dispositivo possui:

🚦 LED bicolor (vermelho/verde) indicando status

🔊 Buzzer para sinal sonoro inicial

🎛️ Chave principal (liga/desliga do ciclo)

⏸️ Botão de pausa

🌐 Envio de dados para servidor SOCKET TCP

<hr/>

## 📌 Funcionalidades

### 🚦 Indicação por LED

- Verde fixo → Produção ativa

- Vermelho/Verde piscando → Pausa

- Vermelho fixo → Máquina desligada

<hr/>

### 🔊 Buzzer

- Um bip curto é emitido ao iniciar o ciclo.

<hr/>

### 🌐 Comunicação

- O módulo se conecta a um Wi-Fi e envia informações para um servidor via SOCKET TCP.

<hr/>

### 📡 Formato do Envio de Dados

O ESP32 envia os seguintes campos:

- codigo	(Código da peça / operação)
- tempoProducao	(Tempo em segundos)
- tempoPausa	(Tempo em segundos)
- tempoTotal	(Tempo em segundos)
- qtd	(Quantidade produzida (fixo = 1))

Exemplo de pacote enviado:

TKC110 002 002;120;15;135;1

<hr/>

### 🔧 Ligações (Hardware)

- Componente	Pino ESP32
- LED Vermelho	GPIO 4
- LED Verde	GPIO 5
- Buzzer	GPIO 18
- Chave Principal	GPIO 27
- Botão Pausa	GPIO 23

Recomenda-se uso de resistores em série para LEDs e pull-ups internos dos pinos de entrada.

<hr/>

### 🚀 Como Usar

1. Configure os dados do Wi-Fi no networkInit()

2. Suba o código no ESP32 (PlatformIO recomendado)

3. Execute o servidor TCP na máquina destino (porta 5050 por padrão)

4. Inicie o ciclo com a chave → LED verde acende

5. Pressione o botão de pausa → LED pisca

6. Desligue a chave → O ESP envia os tempos automaticamente

<hr/>

### 📝 Licença

Este projeto é de uso livre para fins educacionais e industriais.
<hr/>