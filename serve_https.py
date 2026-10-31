#!/usr/bin/env python3
"""
Servidor HTTPS local simple para probar la Web App PWA con Web Bluetooth.
Web Bluetooth requiere una conexión segura (HTTPS o localhost).
Este script genera un certificado SSL temporal y sirve la carpeta app/ en el puerto 8443.
"""

import http.server
import ssl
import os
import socket
import subprocess

PORT = 8443
APP_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'app')

# Generar certificado autofirmado si no existe
CERT_FILE = "/tmp/civic_cert.pem"
KEY_FILE = "/tmp/civic_key.pem"

if not os.path.exists(CERT_FILE) or not os.path.exists(KEY_FILE):
    print("[*] Generando certificado SSL temporal...")
    subprocess.run([
        "openssl", "req", "-x509", "-newkey", "rsa:2048", "-keyout", KEY_FILE,
        "-out", CERT_FILE, "-days", "365", "-nodes",
        "-subj", "/CN=CivicScanner/O=CivicOBD"
    ], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=APP_DIR, **kwargs)

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('8.8.8.8', 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = '127.0.0.1'
    finally:
        s.close()
    return ip

local_ip = get_ip()

print("=" * 60)
print("  HONDA CIVIC '99 PWA WEB BLUETOOTH SERVER")
print("=" * 60)
print(f"[*] Directorio: {APP_DIR}")
print(f"[*] Servidor corriendo en:")
print(f"    👉 https://localhost:{PORT}")
print(f"    👉 https://{local_ip}:{PORT}  (Abre este enlace en tu celular)")
print("=" * 60)
print("[*] NOTA EN EL CELULAR:")
print("    Al abrir por primera vez en Chrome/Brave:")
print("    1. Aparecerá 'La conexión no es privada' (por ser certificado local).")
print("    2. Haz clic en 'Configuración avanzada' -> 'Continuar a sitio no seguro'.")
print("    3. ¡Listo! Presiona 'Conectar BLE' y selecciona Civic99-OBD.")
print("=" * 60)

server = http.server.HTTPServer(('0.0.0.0', PORT), Handler)
context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain(certfile=CERT_FILE, keyfile=KEY_FILE)
server.socket = context.wrap_socket(server.socket, server_side=True)

try:
    server.serve_forever()
except KeyboardInterrupt:
    print("\nServidor detenido.")
