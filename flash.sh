#!/usr/bin/env bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="/dev/ttyACM0"

echo "=== Honda Civic OBD1 ESP32-C3 Uploader ==="

# Verificar permisos del puerto si existe
if [ -e "$PORT" ] && [ ! -w "$PORT" ]; then
    echo "[!] No tienes permisos de escritura en $PORT."
    echo "[*] Solicitando permisos temporales con sudo..."
    sudo chmod 666 "$PORT"
fi

echo "[*] Subiendo firmware a través de PlatformIO..."
"$DIR/.venv/bin/pio" run -t upload

echo "================================================="
echo "[OK] Firmware subido exitosamente al ESP32-C3!"
echo "================================================="
