import os
import socket
import datetime
import threading
import pathlib
from flask import Flask, render_template, jsonify

# -----------------------------------------------------------
# CONFIGURAÇÃO DE PASTAS
# -----------------------------------------------------------

BASE_DIR = "api"
TMP_DIR = os.path.join(BASE_DIR, "tmp")
STATIC_DIR = os.path.join(BASE_DIR, "static")
TEMPLATES_DIR = os.path.join(BASE_DIR, "templates")

LOGS_PATH = os.path.join(TMP_DIR, "logs.txt")

# -----------------------------------------------------------
# CONFIGURAÇÕES DE REDE
# -----------------------------------------------------------

HOST = "0.0.0.0"
TCP_PORT = 5050
FLASK_PORT = 8080


# -----------------------------------------------------------
# FUNÇÃO PARA REGISTRAR LOG
# -----------------------------------------------------------

def salvar_log(texto):
    pathlib.Path(TMP_DIR).mkdir(parents=True, exist_ok=True)

    # Cria cabeçalho se o arquivo não existir
    if not os.path.exists(LOGS_PATH):
        with open(LOGS_PATH, "w") as f:
            f.write("data;produto;tempo_producao;tempo_pausa;tempo_total;quantidade\n")

    # Adiciona nova linha
    with open(LOGS_PATH, "a") as f:
        f.write(texto + "\n")


# -----------------------------------------------------------
# SERVIDOR TCP
# -----------------------------------------------------------

def handle_client(conn, addr):
    print(f"[TCP] Conexão de {addr}")

    while True:
        try:
            data = conn.recv(1024)
            if not data:
                break

            msg = data.decode(errors="ignore").strip()
            if msg == "":
                continue

            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

            print(f"[{timestamp}] RECEBIDO: {msg}")
            salvar_log(f"{timestamp} | {msg}")

            conn.sendall(b"OK")

        except ConnectionResetError:
            break

    print(f"[TCP] Cliente {addr} desconectado")
    conn.close()


def tcp_server():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    server.bind((HOST, TCP_PORT))
    server.listen()

    print(f"[TCP] Servidor iniciado em {HOST}:{TCP_PORT}")

    while True:
        conn, addr = server.accept()
        t = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
        t.start()


# -----------------------------------------------------------
# SERVIDOR FLASK
# -----------------------------------------------------------
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

app = Flask(
    __name__,
    template_folder=os.path.join(BASE_DIR, "templates"),
    static_folder=os.path.join(BASE_DIR, "static")
)

@app.route("/")
def index():
    return render_template("index.html")


@app.route("/logs")
def get_logs():
    if not os.path.exists(LOGS_PATH):
        return jsonify([])

    logs = []
    with open(LOGS_PATH) as f:
        linhas = f.readlines()

    # Remove cabeçalho e pega últimas 200 entradas
    linhas = [l.strip() for l in linhas if "|" in l][-200:]

    for linha in linhas:
        try:
            data, resto = linha.split(" | ")
            produto, tp, pausa, total, qtd = resto.split(";")

            logs.append({
                "data": data,
                "produto": produto,
                "tempo_producao": int(tp),
                "tempo_pausa": int(pausa),
                "tempo_total": int(total),
                "quantidade": int(qtd)
            })

        except Exception as e:
            print("[ERRO PARSE]", linha, e)

    return jsonify(logs)


def flask_server():
    print(f"[FLASK] Servidor iniciado em http://localhost:{FLASK_PORT}")
    app.run(host="0.0.0.0", port=FLASK_PORT, debug=False)


# -----------------------------------------------------------
# MAIN - INICIA AS THREADS
# -----------------------------------------------------------

if __name__ == "__main__":
    print("\n=== MONITOR LOGS INICIADO ===\n")

    # Thread do servidor TCP
    t_tcp = threading.Thread(target=tcp_server, daemon=True)
    t_tcp.start()

    # Thread do servidor Flask
    t_flask = threading.Thread(target=flask_server, daemon=True)
    t_flask.start()

    # Mantém o processo vivo
    try:
        while True:
            pass
    except KeyboardInterrupt:
        print("Encerrando...")
