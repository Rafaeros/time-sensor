#!/usr/bin/env python3
"""
main.py
Servidor integrado TCP + Flask + Async Scraper (arquivo completo e corrigido)

Características:
 - Usa TMP_DIR absoluto dentro da pasta /api (onde este arquivo reside)
 - Inicia loop asyncio em thread antes de tentar qualquer login assíncrono
 - Cria scraper com AuthOnCM(TMP_DIR) para que o session_manager salve em /api/tmp/reports
 - Compatível com execução direta e PyInstaller (.exe)

Estrutura esperada:
 /api/
    main.py  <- este arquivo
    core/
       session_manager.py
       utils/
           path_utils.py
    templates/
       index.html
    static/
       ...

Endpoints:
 - GET / -> index.html (se existir)
 - GET /logs -> logs agrupados
 - GET /scrape/<op> -> dispara scraping para OP informada
"""

import os
import sys
import socket
import datetime
import threading
import pathlib
import time
import asyncio
from typing import Any
from flask import Flask, render_template, jsonify

# importe seu AuthOnCM do módulo core
from core.session_manager import AuthOnCM
from core.utils.path_utils import resource_path
from collections.abc import Coroutine

# -----------------------------------------------------------
# PASTAS E PATHS
# -----------------------------------------------------------

# Diretório absoluto onde este arquivo main.py está localizado (pasta /api)
BASE_DIR = os.path.dirname(os.path.abspath(__file__))

# Templates/static (respeita PyInstaller quando aplicável)
if hasattr(sys, '_MEIPASS'):
    TEMPLATES_DIR = resource_path("api/templates")
    STATIC_DIR = resource_path("api/static")
else:
    TEMPLATES_DIR = os.path.join(BASE_DIR, "templates")
    STATIC_DIR = os.path.join(BASE_DIR, "static")

# TMP absoluto dentro de /api
TMP_DIR = os.path.join(BASE_DIR, "tmp")
REPORTS_DIR = os.path.join(TMP_DIR, "reports")
os.makedirs(REPORTS_DIR, exist_ok=True)

LOGS_PATH = os.path.join(TMP_DIR, "logs.txt")

# -----------------------------------------------------------
# CONFIGURAÇÕES DE REDE
# -----------------------------------------------------------

HOST = "0.0.0.0"
TCP_PORT = 5050
FLASK_PORT = 8080

# -----------------------------------------------------------
# LOGGER SIMPLES
# -----------------------------------------------------------

def salvar_log(texto: str) -> None:
    pathlib.Path(TMP_DIR).mkdir(parents=True, exist_ok=True)

    # Cria arquivo se não existir
    if not os.path.exists(LOGS_PATH):
        with open(LOGS_PATH, "w", encoding="utf-8") as f:
            f.write(texto + "\n")

    # Adiciona linha
    with open(LOGS_PATH, "a", encoding="utf-8") as f:
        f.write(texto + "\n")


# -----------------------------------------------------------
# SERVIDOR TCP
# -----------------------------------------------------------

def handle_client(conn: socket.socket, addr: Any) -> None:
    print(f"[TCP] Conexão de {addr}")

    try:
        while True:
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
        pass
    except Exception as e:
        print("[TCP] Erro no handle_client:", e)
    finally:
        print(f"[TCP] Cliente {addr} desconectado")
        try:
            conn.close()
        except Exception:
            pass


def tcp_server() -> None:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    server.bind((HOST, TCP_PORT))
    server.listen()

    print(f"[TCP] Servidor iniciado em {HOST}:{TCP_PORT}")

    try:
        while True:
            conn, addr = server.accept()
            t = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
            t.start()
    except Exception as e:
        print("[TCP] Encerrando servidor TCP:", e)
    finally:
        try:
            server.close()
        except Exception:
            pass


# -----------------------------------------------------------
# FLASK
# -----------------------------------------------------------

app = Flask(__name__, template_folder=TEMPLATES_DIR, static_folder=STATIC_DIR)


@app.route("/")
def index():
    try:
        return render_template("index.html")
    except Exception:
        return jsonify({"status": "ok", "message": "Monitor running"})


@app.route("/logs")
def get_logs():
    if not os.path.exists(LOGS_PATH):
        return jsonify({})

    registros = []
    with open(LOGS_PATH, encoding="utf-8") as f:
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
                "quantidade": int(qtd),
            })

        except Exception as e:
            print("[ERRO PARSE]", linha, e)

    registros.sort(key=lambda x: x["datetime"], reverse=True)

    produtos: dict[str, Any] = {}

    for r in registros:
        prod = r["produto"]

        if prod not in produtos:
            produtos[prod] = {"media": 0, "logs": [], "_soma": 0, "_count": 0}

        produtos[prod]["_soma"] += r["tempo_total"]
        produtos[prod]["_count"] += 1
        produtos[prod]["logs"].append(r)

    for prod in produtos:
        produtos[prod]["media"] = produtos[prod]["_soma"] / produtos[prod]["_count"]
        produtos[prod]["logs"] = produtos[prod]["logs"][:50]
        del produtos[prod]["_soma"]
        del produtos[prod]["_count"]

    for prod in produtos:
        for log in produtos[prod]["logs"]:
            log["datetime"] = log["datetime"].isoformat()

    return jsonify(produtos)


def flask_server() -> None:
    print(f"[FLASK] Servidor iniciado em http://localhost:{FLASK_PORT}")
    app.run(host="0.0.0.0", port=FLASK_PORT, debug=False, use_reloader=False)


# -----------------------------------------------------------
# EVENT LOOP ASSÍNCRONO GLOBAL
# -----------------------------------------------------------

async_loop = asyncio.new_event_loop()


def start_async_loop() -> None:
    global async_loop
    asyncio.set_event_loop(async_loop)
    async_loop.run_forever()


def run_async(coro: Coroutine):
    return asyncio.run_coroutine_threadsafe(coro, async_loop)


# -----------------------------------------------------------
# SCRAPER
# -----------------------------------------------------------

# Instancia o scraper com o TMP_DIR absoluto
scraper = AuthOnCM(TMP_DIR)


async def scrape_task(op: str) -> dict:
    print(f"[SCRAPER] Iniciando scraping da OP {op}...")

    try:
        # garante sessão válida (get_client reloga se necessário)
        session = await scraper.get_client()
    except Exception as e:
        return {"ok": False, "erro": f"login failed: {e}"}

    try:
        html = await scraper.get_orders_by_code(op)
        return {"ok": True, "raw_html_len": len(html) if html else 0}
    except Exception as e:
        return {"ok": False, "erro": str(e)}


@app.route("/scrape/<op>")
def scrape_op(op: str):
    try:
        future = run_async(scrape_task(op))
        result = future.result(timeout=90)
        return jsonify({"op": op, "status": "concluido", "resultado": result})
    except Exception as e:
        return jsonify({"erro": str(e)}), 500


# -----------------------------------------------------------
# START
# -----------------------------------------------------------

if __name__ == "__main__":
    print("\n=== MONITOR LOGS INICIADO ===")

    # Thread do loop async — iniciar antes de qualquer run_async/login
    _loop_thread = threading.Thread(target=start_async_loop, daemon=True)
    _loop_thread.start()

    # aguarda rápido o loop subir
    time.sleep(0.1)

    # Login inicial: agenda login no loop e aguarda resultado
    try:
        fut_login = run_async(scraper.login())
        fut_login.result(timeout=30)
        print("[SCRAPER] Login inicial realizado com sucesso.")
    except Exception as e:
        print(f"[SCRAPER] Falha no login inicial: {e}")

    # Thread TCP
    t_tcp = threading.Thread(target=tcp_server, daemon=True)
    t_tcp.start()

    # Thread Flask
    t_flask = threading.Thread(target=flask_server, daemon=True)
    t_flask.start()

    stop_event = threading.Event()

    try:
        while not stop_event.is_set():
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("\n[MAIN] KeyboardInterrupt recebido — encerrando...\n")
        stop_event.set()

        try:
            fut = run_async(scraper.close())
            fut.result(timeout=5)
        except Exception:
            pass

        try:
            async_loop.call_soon_threadsafe(async_loop.stop)
        except Exception:
            pass

    print("[MAIN] Encerrado.")
