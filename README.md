# 🚗 Honda Civic '99 K-Line Diagnostic Scanner & Live BLE Telemetry

Sistema integral de diagnóstico OBD y telemetría en tiempo real para **Honda Civic 1999 (PCM P2P / Motor D16Y8)** utilizando el conector **DLC de 3 pines**, microcontrolador **ESP32-C3** con Bluetooth Low Energy (BLE 5.0) y transceptor **L9637D**.

Incluye:
1. **Firmware ESP32-C3**: Driver de bajo nivel para el protocolo K-Line patentado de Honda (9600 baud, half-duplex) y servidor BLE Nordic UART Service (NUS) a 20 Hz.
2. **Web Dashboard PWA**: Aplicación web progresiva con Web Bluetooth API, instrumentación digital/analógica estilo Type-R, shift lights, lectura de códigos DTC y logueo de datos.
3. **Documentación de hardware**: Esquema electrónico, pines DLC, transceptor y recomendaciones de ensamble.

---

## 📁 Estructura del Repositorio

| Archivo / Directorio | Descripción |
|---|---|
| [`honda-civic-scanner.ino`](honda-civic-scanner.ino) | Firmware principal ESP32-C3 (BLE NUS, ciclo de polling a la ECU y streaming binario). |
| [`honda_kline.h`](honda_kline.h) | Estructuras de telemetría (RPM, VSS, TPS, MAP, ECT, IAT, O2, marchas, etc.) y constantes. |
| [`honda_kline.cpp`](honda_kline.cpp) | Implementación del protocolo K-Line: wake-up pulse, checksums, handshake y decodificación. |
| [`dashboard_html.h`](dashboard_html.h) | Interfaz web embebida en memoria Flash (`PROGMEM`) para visualización directa. |
| [`platformio.ini`](platformio.ini) | Configuración de compilación para PlatformIO (target `esp32-c3-devkitm-1`). |
| [`flash.sh`](flash.sh) | Script bash para compilar y flashear el ESP32 por USB. |
| [`app/`](app/) | PWA independiente (HTML5/CSS3/JS) con Web Bluetooth, Service Worker y modo offline. |
| [`GUIA_CONEXION_Y_ARMADO.md`](GUIA_CONEXION_Y_ARMADO.md) | Diagrama de pines DLC 3 pines, circuito L9637D y guía de armado paso a paso. |
| [`serve_https.py`](serve_https.py) | Servidor local HTTPS para pruebas de Web Bluetooth en navegadores móviles. |
| [`csv/`](csv/) | Registros de telemetría de prueba en ruta real para análisis y calibración. |

---

## ⚡ Conexión de Hardware

El Honda Civic 1999 utiliza el conector azul de 3 pines ubicado debajo de la guantera del copiloto:
* **Pin 1 (+12V Batería)**
* **Pin 2 (GND Tierra)**
* **Pin 3 (K-Line / Comunicación bidireccional con la ECU)**

El transceptor **L9637D** convierte la señal de 12V del bus K-Line a los niveles lógicos de 3.3V requeridos por el **ESP32-C3** (GPIO 4 RX / GPIO 5 TX).

Consulta [`GUIA_CONEXION_Y_ARMADO.md`](GUIA_CONEXION_Y_ARMADO.md) para ver los diagramas detallados de los pines y resistencias de pull-up.

---

## 🚀 Compilación y Flasheo del Firmware

### Requisitos:
* [PlatformIO Core (CLI)](https://platformio.org/) o extensión de PlatformIO en VS Code.

### Pasos:
```bash
# Compilar
pio run

# Flashear al ESP32-C3 (ajustar puerto si es necesario)
./flash.sh
```

---

## 📱 Tablero Web PWA (Dashboard)

La aplicación web se conecta de forma directa al ESP32 a través de **Web Bluetooth API**:
* **Despliegue en vivo**: [https://fedelt01hub.github.io/civic-obd/](https://fedelt01hub.github.io/civic-obd/)
* Compatible con navegadores con soporte Web Bluetooth (Google Chrome / Brave en Android, Windows, Mac y Linux con flags habilitados).
