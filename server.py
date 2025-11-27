import socket

HOST = '0.0.0.0'   # escuta em todas interfaces
PORT = 5050        # porta do servidor

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((HOST, PORT))
server.listen()

print(f"Servidor TCP iniciado em {HOST}:{PORT}")

conn, addr = server.accept()
print(f"Conexão de {addr}")

while True:
    data = conn.recv(1024)
    if not data:
        break
    print("Recebido:", data.decode())
    conn.sendall(b"OK")  # resposta opcional

conn.close()