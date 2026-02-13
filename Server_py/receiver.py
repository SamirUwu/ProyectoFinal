import socket
from zeroconf import ServiceInfo, Zeroconf
import json

PORT = 5000
HOST = "0.0.0.0"

temp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
temp_sock.connect(("8.8.8.8", 80)) # no envía nada realmente
LOCAL_IP = temp_sock.getsockname()[0]
temp_sock.close()

print("IP local:", LOCAL_IP)

zeroconf = Zeroconf()

service_type = "_guitarfx._tcp.local."
service_name = "PythonGuitarFX._guitarfx._tcp.local."

info = ServiceInfo(
    type_=service_type,
    name=service_name,
    addresses=[socket.inet_aton(LOCAL_IP)],
    port=PORT,
    properties={"desc": "Bridge Flutter → LabVIEW"},
)

zeroconf.register_service(info)

print("🟣 Servicio anunciado por mDNS")

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

server.bind((HOST, PORT))
server.listen(5)

print("🟢 Esperando conexión...")

try:
    conn, addr = server.accept()
    print(f"🔵 Conectado desde {addr}")

    while True:
        data = conn.recv(4096)

        if not data:
            break

        msg = data.decode().strip()

        print("📦 JSON recibido:", msg)

except KeyboardInterrupt:
    print("Apagando servidor...")

finally:
    zeroconf.unregister_service(info)
    zeroconf.close()
    server.close()
