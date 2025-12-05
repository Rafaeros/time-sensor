#!/usr/bin/env python3
"""
main.py — Servidor TCP + Flask + Async Scraper (corrigido e limpo)

Mudanças aplicadas (seguras e retro-compatíveis):
 - BASE_DIR fixo: se EXE -> pasta do executável; senão -> pasta do script
 - TMP_DIR/REPORTS_DIR criados antes de iniciar qualquer servidor
 - LOGS_PATH garantido (touch) para sempre existir
 - Debug prints de BASE_DIR / TMP_DIR / LOGS_PATH (úteis para verificar runtime)
 - Leitura do logs mais tolerante (não descarta linhas por pequenos formatos)
 - Criação/append do logs sem truncar
 - Mantive o restante do seu código (TCP, Flask, Scraper) intactos
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
# PASTAS E PATHS (BASE_DIR fixo e persistente)
# -----------------------------------------------------------

if getattr(sys, "frozen", False):
    # executável empacotado: base é a pasta do executável (persistente)
    BASE_DIR = pathlib.Path(sys.executable).parent
else:
    # modo dev: base é a pasta do main.py
    BASE_DIR = pathlib.Path(__file__).parent

# Templates e static (compatível com PyInstaller / dev)
if getattr(sys, "frozen", False):
    TEMPLATES_DIR = resource_path("templates")
    STATIC_DIR = resource_path("static")
else:
    TEMPLATES_DIR = str(BASE_DIR / "templates")
    STATIC_DIR = str(BASE_DIR / "static")

# TMP absoluto dentro da pasta base
TMP_DIR = BASE_DIR / "tmp"
REPORTS_DIR = TMP_DIR / "reports"
LOGS_PATH = TMP_DIR / "logs.txt"

# garante existência das pastas e do arquivo de log
TMP_DIR.mkdir(parents=True, exist_ok=True)
REPORTS_DIR.mkdir(parents=True, exist_ok=True)
# touch logs file so it exists and permissions are correct
try:
    LOGS_PATH.touch(exist_ok=True)
except Exception:
    # em casos restritos de permissão, tentamos criar via open
    with open(str(LOGS_PATH), "a", encoding="utf-8"):
        pass

# debug - mostra onde o app está lendo/escrevendo
print(f"[DEBUG] BASE_DIR    : {BASE_DIR}")
print(f"[DEBUG] TMP_DIR     : {TMP_DIR}")
print(f"[DEBUG] LOGS_PATH   : {LOGS_PATH}")

# -----------------------------------------------------------
# CONFIGURAÇÕES DE REDE
# -----------------------------------------------------------

HOST = "0.0.0.0"
TCP_PORT = 5050
FLASK_PORT = 8080

# -----------------------------------------------------------
# FUNÇÕES AUXILIARES DE LOG (escrita/parse robusto)
# -----------------------------------------------------------


def salvar_log(texto: str) -> None:
    """Append seguro ao arquivo de logs (cria pasta se necessário)."""
    TMP_DIR.mkdir(parents=True, exist_ok=True)
    # usa modo append e encoding sempre
    with open(str(LOGS_PATH), "a", encoding="utf-8") as f:
        f.write(texto + "\n")


def _parse_log_line(line: str):
    """
    Tenta parsear uma linha de log no formato:
    "YYYY-MM-DD HH:MM:SS | produto;tp;pausa;total;qtd"

    Retorna um dict com os campos ou None se não for possível.
    A função é tolerante: aceita espaços variados e tenta preencher valores faltantes.
    """
    if not line:
        return None

    # Tentativa simples: encontrar ' | ' (com espaço) ou '|' sem espaços
    if " | " in line:
        ts_part, rest = line.split(" | ", 1)
    elif "|" in line:
        ts_part, rest = line.split("|", 1)
        ts_part = ts_part.strip()
        rest = rest.strip()
    else:
        # sem separador reconhecível -> não conseguimos parsear
        return None

    # tenta converter timestamp
    try:
        dt = datetime.datetime.strptime(ts_part.strip(), "%Y-%m-%d %H:%M:%S")
    except Exception:
        # se falhar, rejeita
        return None

    # separa o resto por ';' e normaliza quantidade de itens
    parts = [p.strip() for p in rest.split(";")]
    # esperamos 5 campos: produto, tp, pausa, total, qtd
    while len(parts) < 5:
        parts.append("0")
    produto = parts[0]
    try:
        tp = int(parts[1]) if parts[1] else 0
    except Exception:
        tp = 0
    try:
        pausa = int(parts[2]) if parts[2] else 0
    except Exception:
        pausa = 0
    try:
        total = int(parts[3]) if parts[3] else 0
    except Exception:
        total = tp + pausa
    try:
        qtd = int(parts[4]) if parts[4] else 0
    except Exception:
        qtd = 0

    return {
        "data": dt.strftime("%Y-%m-%d %H:%M:%S"),
        "datetime": dt,
        "produto": produto,
        "tempo_producao": tp,
        "tempo_pausa": pausa,
        "tempo_total": total,
        "quantidade": qtd,
    }


# -----------------------------------------------------------
# SERVIDOR TCP
# -----------------------------------------------------------


def handle_client(conn: socket.socket, addr: Any) -> None:
    print(f"[TCP] Conexão de {addr}")

    try:
        while True:
            data = conn.recv(4096)
            if not data:
                break

            # decodifica ignorando bytes errados
            try:
                msg = data.decode("utf-8", errors="ignore").strip()
            except Exception:
                msg = str(data)

            if msg == "":
                continue

            timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            print(f"[{timestamp}] RECEBIDO: {msg}")

            # salva no formato já usado pelo frontend: "TIMESTAMP | RESTO"
            salvar_log(f"{timestamp} | {msg}")
            try:
                conn.sendall(b"OK")
            except Exception:
                pass

    except Exception as e:
        print("[TCP] Erro no handle_client:", e)
    finally:
        try:
            conn.close()
        except Exception:
            pass
        print(f"[TCP] Cliente {addr} desconectado")


def tcp_server() -> None:
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, TCP_PORT))
    server.listen()
    print(f"[TCP] Servidor TCP iniciado em {HOST}:{TCP_PORT}")

    try:
        while True:
            conn, addr = server.accept()
            threading.Thread(target=handle_client, args=(conn, addr), daemon=True).start()
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
    except Exception as e:
        return jsonify(
            {"status": "erro", "mensagem": "Falha ao carregar template", "detalhes": str(e)}
        )


@app.route("/logs")
def get_logs():
    """
    Lê o arquivo logs.txt sempre do TMP_DIR configurado e retorna o JSON esperado
    A leitura é tolerante a linhas mal-formadas.
    """
    registros = []

    # garante que o arquivo exista (touch)
    try:
        LOGS_PATH.touch(exist_ok=True)
    except Exception:
        pass

    try:
        with open(str(LOGS_PATH), encoding="utf-8") as f:
            raw_lines = f.readlines()
    except Exception as e:
        print("[LOGS] erro ao abrir arquivo:", e)
        return jsonify({})

    # percorre linhas do arquivo do mais antigo ao mais novo, mas depois ordena
    for line in raw_lines:
        line = line.strip()
        parsed = _parse_log_line(line)
        if parsed:
            registros.append(parsed)

    # ordena do mais novo → mais antigo
    registros.sort(key=lambda x: x["datetime"], reverse=True)

    # agrupa por produto e calcula médias
    produtos: dict[str, Any] = {}
    for r in registros:
        prod = r["produto"]
        if prod not in produtos:
            produtos[prod] = {"media": 0, "logs": [], "_soma": 0, "_count": 0}

        produtos[prod]["_soma"] += r["tempo_total"]
        produtos[prod]["_count"] += 1
        produtos[prod]["logs"].append(r)

    for prod in produtos:
        produtos[prod]["media"] = produtos[prod]["_soma"] / produtos[prod]["_count"] if produtos[prod]["_count"] else 0
        produtos[prod]["logs"] = produtos[prod]["logs"][:50]
        del produtos[prod]["_soma"]
        del produtos[prod]["_count"]

    # converte datetime para iso string para serialização
    for prod in produtos:
        for log in produtos[prod]["logs"]:
            if isinstance(log.get("datetime"), datetime.datetime):
                log["datetime"] = log["datetime"].isoformat()

    return jsonify(produtos)


def start_flask():
    print(f"[FLASK] Servidor iniciado em http://localhost:{FLASK_PORT}")
    # use_reloader=False evita spawn extra de processo ao executar em thread
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

scraper = AuthOnCM(str(TMP_DIR))


async def scrape_task(op: str) -> dict:
    print(f"[SCRAPER] Iniciando scraping da OP {op}...")
    try:
        await scraper.get_client()
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

    # inicia loop async em thread antes de qualquer run_async()
    threading.Thread(target=start_async_loop, daemon=True).start()
    time.sleep(0.05)

    # Login inicial (mantém sessão pronta)
    try:
        fut_login = run_async(scraper.login())
        fut_login.result(timeout=30)
        print("[SCRAPER] Login inicial realizado com sucesso.")
    except Exception as e:
        print(f"[SCRAPER] Falha no login inicial: {e}")

    # inicia servidores
    threading.Thread(target=tcp_server, daemon=True).start()
    threading.Thread(target=start_flask, daemon=True).start()

    try:
        while True:
            time.sleep(0.5)
    except KeyboardInterrupt:
        print("\n[MAIN] Encerrando...\n")
        try:
            fut = run_async(scraper.close())
            fut.result(timeout=5)
        except Exception:
            pass
        async_loop.call_soon_threadsafe(async_loop.stop)

    print("[MAIN] Encerrado.")
