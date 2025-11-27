import socket
import datetime

HOST = '0.0.0.0'
PORT = 5050

def salvar_log(texto):
    with open("log.txt", "a") as f:
        f.write(texto + "\n")

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen()

print(f"Servidor TCP iniciado em {HOST}:{PORT}")

while True:
    print("Aguardando conexão...")
    conn, addr = server.accept()
    print(f"Conexão de {addr}")

    while True:
        data = conn.recv(1024)

        if not data:
            print("Cliente desconectado!")
            conn.close()
            break

        msg = data.decode().strip()

        # ⚠️ IGNORA se vier vazio
        if msg == "":
            continue

        timestamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        print(f"[{timestamp}] Recebido:", msg)

        salvar_log(f"{timestamp} | {msg}")

        conn.sendall(b"OK")
