#!/usr/bin/env python3
"""
main.py — Servidor TCP + Flask + Async Scraper (corrigido e limpo)

Correções pedidas:
 ✔ TMP_DIR sempre dentro da pasta /api (onde main.py está)
 ✔ logs.txt dentro de /api/tmp (Linux e Windows .exe)
 ✔ reports em /api/tmp/reports
 ✔ server Flask sem operador walrus
 ✔ restante do código mantido exatamente igual
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

# Módulos do projeto
from core.session_manager import AuthOnCM
from core.utils.path_utils import resource_path
from collections.abc import Coroutine


# -----------------------------------------------------------
# PASTAS E PATHS
# -----------------------------------------------------------

BASE_DIR = os.path.dirname(os.path.abspath(__file__))  # pasta /api

# Templates e static (compatível com PyInstaller)
if getattr(sys, "frozen", False):
    # Quando empacotado, resource_path já resolve para onde os dados foram extraídos/copiados
    TEMPLATES_DIR = resource_path("templates")
    STATIC_DIR = resource_path("static")
else:
    # modo dev: templates e static estão em /api/templates e /api/static
    TEMPLATES_DIR = os.path.join(BASE_DIR, "templates")
    STATIC_DIR = os.path.join(BASE_DIR, "static")

# TMP absoluto dentro de /api
TMP_DIR = os.path.join(BASE_DIR, "tmp")
REPORTS_DIR = os.path.join(TMP_DIR, "reports")

os.makedirs(REPORTS_DIR, exist_ok=True)
os.makedirs(TMP_DIR, exist_ok=True)

LOGS_PATH = os.path.join(TMP_DIR, "logs.txt")


# -----------------------------------------------------------
# CONFIGURAÇÕES DE REDE
# -----------------------------------------------------------

HOST = "0.0.0.0"
TCP_PORT = 5050
FLASK_PORT = 8080


# -----------------------------------------------------------
# LOGGER
# -----------------------------------------------------------


def salvar_log(texto: str) -> None:
    pathlib.Path(TMP_DIR).mkdir(parents=True, exist_ok=True)

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

    except Exception as e:
        print("[TCP] Erro no handle_client:", e)
    finally:
        conn.close()
        print(f"[TCP] Cliente {addr} desconectado")


def tcp_server() -> None:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    server.bind((HOST, TCP_PORT))
    server.listen()

    print(f"[TCP] Servidor TCP iniciado em {HOST}:{TCP_PORT}")

    while True:
        conn, addr = server.accept()
        threading.Thread(target=handle_client, args=(conn, addr), daemon=True).start()


# -----------------------------------------------------------
# FLASK
# -----------------------------------------------------------

app = Flask(__name__, template_folder=TEMPLATES_DIR, static_folder=STATIC_DIR)


@app.route("/")
def index():
    try:
        return render_template("index.html")
    except Exception as e:
        return jsonify(
            {
                "status": "erro",
                "mensagem": "Falha ao carregar template",
                "detalhes": str(e),
            }
        )


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

            registros.append(
                {
                    "data": data_str,
                    "datetime": dt,
                    "produto": produto,
                    "tempo_producao": int(tp),
                    "tempo_pausa": int(pausa),
                    "tempo_total": int(total),
                    "quantidade": int(qtd),
                }
            )
        except Exception:
            pass

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


def start_flask():
    print(f"[FLASK] Servidor iniciado em http://localhost:{FLASK_PORT}")
    app.run(host="0.0.0.0", port=FLASK_PORT, debug=False, use_reloader=False)


# -----------------------------------------------------------
# EVENT LOOP ASSÍNCRONO
# -----------------------------------------------------------

async_loop = asyncio.new_event_loop()


def start_async_loop() -> None:
    asyncio.set_event_loop(async_loop)
    async_loop.run_forever()


def run_async(coro: Coroutine):
    return asyncio.run_coroutine_threadsafe(coro, async_loop)


# -----------------------------------------------------------
# SCRAPER
# -----------------------------------------------------------

scraper = AuthOnCM(TMP_DIR)


async def scrape_task(op: str) -> dict:
    print(f"[SCRAPER] Iniciando scraping da OP {op}...")

    try:
        session = await scraper.get_client()
    except Exception as e:
        return {"ok": False, "erro": f"login failed: {e}"}

    try:
        data = await scraper.get_orders_by_code(op)
        return {"ok": True, "data": data}
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

    # Inicia loop async
    threading.Thread(target=start_async_loop, daemon=True).start()
    time.sleep(0.1)

    # Login inicial
    try:
        fut_login = run_async(scraper.login())
        fut_login.result(timeout=30)
        print("[SCRAPER] Login inicial realizado com sucesso.")
    except Exception as e:
        print(f"[SCRAPER] Falha no login inicial: {e}")

    # TCP
    threading.Thread(target=tcp_server, daemon=True).start()

    # Flask
    threading.Thread(target=start_flask, daemon=True).start()

    try:
        while True:
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("\n[MAIN] Encerrando...\n")
        run_async(scraper.close())
        async_loop.call_soon_threadsafe(async_loop.stop)

    print("[MAIN] Encerrado.")
