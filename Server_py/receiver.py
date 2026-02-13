import socket
from zeroconf import ServiceInfo, Zeroconf
import json
import queue
import threading
import time


PORT = 5000
HOST = "0.0.0.0"
labview = None

temp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
temp_sock.connect(("8.8.8.8", 80)) 
LOCAL_IP = temp_sock.getsockname()[0]
temp_sock.close()

def connect_labview():
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", 6000))
            sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
            print("🟡 Conectado a LabVIEW")
            return sock
        except:
            print("Esperando LabVIEW...")
            time.sleep(15)
            n
def labview_connector():
    global labview

    while True:
        try:
            labview = connect_labview()

            # Espera hasta que se caiga
            while True:
                time.sleep(1)
                if labview is None:
                    break

        except:
            labview = None


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
    threading.Thread(target=labview_connector, daemon=True).start()

    while True:
        conn, addr = server.accept()
        print(f"🔵 Conectado desde {addr}")

        while True:
            data = conn.recv(4096)

            if not data:
                print("Cliente desconectado")
                break

            msg = data.decode().strip()
            print("📦 JSON recibido:", msg)

            if labview:
                try:
                    labview.sendall((msg + "\n").encode())

                except:
                    print("LabVIEW se cayó, reconectando...")
                    labview = None

except KeyboardInterrupt:
    print("Apagando servidor...")


finally:
    zeroconf.unregister_service(info)
    zeroconf.close()
    server.close()
