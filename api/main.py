import os
import sys
import socket
import datetime
import threading
import pathlib
from flask import Flask, render_template, jsonify

# -----------------------------------------------------------
# CONFIGURAÇÃO DE PASTAS
# -----------------------------------------------------------

def resource_path(relative):
    if hasattr(sys, '_MEIPASS'):
        # Dentro do executável
        base = sys._MEIPASS
    else:
        # Executando normal (Linux ou Windows sem EXE)
        base = os.path.dirname(os.path.abspath(__file__))

    return os.path.join(base, relative)


# agora tenta achar onde os templates realmente estão
if hasattr(sys, '_MEIPASS'):
    TEMPLATES_DIR = resource_path("api/templates")  # dentro do exe
    STATIC_DIR    = resource_path("api/static")
else:
    TEMPLATES_DIR = resource_path("templates")       # rodando normal
    STATIC_DIR    = resource_path("static")

BASE_DIR = resource_path(".")
TMP_DIR = resource_path("tmp")
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
            f.write(texto + "\n")

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


app = Flask(
    __name__,
    template_folder=TEMPLATES_DIR,
    static_folder=STATIC_DIR
)

@app.route("/")
def index():
    return render_template("index.html")


@app.route("/logs")
def get_logs():
    if not os.path.exists(LOGS_PATH):
        return jsonify({})

    registros = []
    with open(LOGS_PATH) as f:
        linhas = [l.strip() for l in f.readlines() if " | " in l]

    for linha in linhas:
        try:
            data_str, resto = linha.split(" | ")
            produto, tp, pausa, total, qtd = resto.split(";")

            dt = datetime.datetime.strptime(data_str, "%Y-%m-%d %H:%M:%S")

            registros.append({
                "data": data_str,
                "datetime": dt,
                "produto": produto,
                "tempo_producao": int(tp),
                "tempo_pausa": int(pausa),
                "tempo_total": int(total),
                "quantidade": int(qtd)
            })

        except Exception as e:
            print("[ERRO PARSE]", linha, e)

    # Ordena do mais novo → mais antigo
    registros.sort(key=lambda x: x["datetime"], reverse=True)

    # Agrupa por produto + calcula média com TODOS os registros
    produtos = {}

    for r in registros:
        prod = r["produto"]

        if prod not in produtos:
            produtos[prod] = {
                "media": 0,
                "logs": [],
                "_soma": 0,
                "_count": 0
            }

        produtos[prod]["_soma"] += r["tempo_total"]
        produtos[prod]["_count"] += 1

        produtos[prod]["logs"].append(r)

    # Finalizar: calcular média e limitar a 50 registros
    for prod in produtos:
        produtos[prod]["media"] = produtos[prod]["_soma"] / produtos[prod]["_count"]
        produtos[prod]["logs"] = produtos[prod]["logs"][:50]  # só os 50 mais recentes
        del produtos[prod]["_soma"]
        del produtos[prod]["_count"]

    return jsonify(produtos)


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
