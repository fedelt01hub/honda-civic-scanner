# Guía de Conexión y Armado: Escáner Honda 3-Pin con ESP32-C3, L9637D y LM2596

Esta guía explica paso a paso cómo soldar, armar y conectar tu escáner casero para un **Honda Civic 1999** (transmisión automática) utilizando el protocolo propietario Honda DLC de 3 pines.

---

## 1. Lista de Componentes

1. **ESP32-C3** (placa compacta, ej. SuperMini o DevKit).
2. **L9637D** (transceptor automotriz K-Line en encapsulado SOIC-8).
3. **Módulo LM2596** (convertidor Buck Step-Down con potenciómetro de ajuste).
4. **Resistencia de 510 Ω a 1 kΩ** (1/4 W o 1/2 W, para pull-up de la K-Line).
5. **Condensador cerámico de 100 nF (0.1 µF)** (filtro de desacoplo para VCC del L9637D).
6. **Placa perforada (perfboard)**, estaño y cables delgados (ideal alambre esmaltado o cable fino de wrapping AWG 30).
7. **3 terminales macho o espadas delgadas** (para conectar firmemente al conector hembra de 3 pines de Honda).

---

## 2. Paso Crucial: Calibración Previa del Regulador LM2596

> [!CAUTION]
> **NO CONECTES EL ESP32 AL LM2596 ANTES DE ESTE PASO.**
> Muchos módulos LM2596 vienen de fábrica configurados para entregar 12V o más. Debes calibrarlo primero con un multímetro para no quemar el microcontrolador.

1. Conecta la entrada del LM2596 (`IN+` y `IN-`) a una fuente de 12V (o a la batería del auto).
2. Coloca el multímetro en modo medición de voltaje continuo (DC) sobre los terminales de salida (`OUT+` y `OUT-`).
3. Con un destornillador perillero plano, gira el tornillo del potenciómetro azul en sentido antihorario hasta que la salida marque exactamente **5.00V**.
4. Ahora la salida está lista para conectarse al pin **5V (o VIN)** y **GND** del ESP32-C3.

---

## 3. Preparación y Soldadura del L9637D (SMD SOIC-8)

El L9637D viene en formato miniatura de 8 pines (SOIC-8).

### Identificación de pines (Vista Superior):
```
        Muesca / Punto (Pin 1)
             |
       +-----v-----+
RX  ---| 1       8 |--- LI (GND o Flotante)
LO  ---| 2       7 |--- VS (+12V Batería)
VCC ---| 3       6 |--- K  (K-Line a Honda Pin 1)
TX  ---| 4       5 |--- GND (Masa Común)
       +-----------+
```

### Cómo soldarlo a la placa perforada:
* **Método "Dead Bug"**: Pega el chip boca arriba sobre la placa perforada con una gota de pegamento instantáneo o cinta doble faz. Suelda alambres delgados desde cada pata del chip hacia los agujeros de la placa perforada.
* **Método adaptador**: Si consigues una placa adaptadora "SOIC-8 a DIP-8" (valen centavos), solo sueldas el chip allí y queda como un integrado común que entra en cualquier protoboard o placa perforada.

---

## 4. Diagrama Pin a Pin del Circuito Completo

| Origen (Componente) | Destino (A dónde va) | Función / Notas |
| :--- | :--- | :--- |
| **LM2596 IN+** | **Pin 2 (+12V) Honda DLC** | Entrada de 12V desde el auto |
| **LM2596 IN-** | **Pin 3 (GND) Honda DLC** | Masa común del auto |
| **LM2596 OUT+ (5V)** | **ESP32-C3 Pin 5V / VIN** | Alimentación 5V al ESP32 |
| **LM2596 OUT- (GND)** | **ESP32-C3 Pin GND** | Masa común del sistema |
| | | |
| **L9637D Pin 7 (VS)** | **Pin 2 (+12V) Honda DLC** | Alimentación del bus de 12V del transceptor |
| **L9637D Pin 5 (GND)**| **GND Común** | Tierra del L9637D |
| **L9637D Pin 3 (VCC)**| **ESP32-C3 Pin 3.3V (3V3)**| **¡IMPORTANTE!** Al alimentar VCC a 3.3V, el chip entrega niveles de 3.3V en RX, protegiendo al ESP32 sin divisores de voltaje. |
| **Condensador 100nF** | **Entre L9637D Pin 3 y Pin 5** | Filtrado de ruido eléctrico |
| **L9637D Pin 1 (RX)** | **ESP32-C3 GPIO 4** | Datos recibidos desde el ECU al ESP32 |
| **L9637D Pin 4 (TX)** | **ESP32-C3 GPIO 5** | Datos transmitidos desde el ESP32 al ECU |
| **L9637D Pin 6 (K)**  | **Pin 1 (K-Line) Honda DLC** | Línea de datos serie bidireccional |
| **Resistencia 1kΩ**   | **Entre L9637D Pin 6 y Pin 7**| Resistencia de Pull-up de K-Line a 12V |
| **L9637D Pin 2 (LO)** | *Sin Conexión* | No se utiliza |
| **L9637D Pin 8 (LI)** | **GND Común** (o flotante) | Entrada de línea L (a masa) |

---

## 5. Identificación y Verificación del Conector Honda en el Civic '99

En los Honda Civic 1996–2000 (chasis EK / EJ):
1. Ubícate en el asiento del copiloto.
2. Mira por debajo de la guantera, hacia el panel lateral derecho (kick panel de la ECU).
3. Verás una funda de goma protectora (usualmente azul o gris) encajada en un soporte plástico.
4. Desconecta los enchufes alojados en esa goma:
   - **Enchufe de 2 pines**: Conector SCS (Service Check Connector).
   - **Enchufe de 3 pines**: Conector DLC (Data Link Connector) donde conectaremos nuestro escáner.

### Verificación con Multímetro antes de enchufar:
1. Pon el tester en Voltímetro DC (20V).
2. Punta negra al chasis o al cable negro del conector (Pin 3 GND).
3. Con el contacto puesto (**IGN II**, sin arrancar):
   - **Pin 2 (Alimentación)**: Debe medir entre **+12.0V y +12.6V**.
   - **Pin 1 (K-Line)**: Debe medir entre **5V y 11V** (voltaje de reposo de la línea de datos).

---

## 6. Carga del Firmware en el ESP32-C3

### Opción A: Con Arduino IDE
1. Abre Arduino IDE y ve a **Ajustes / Preferencias**.
2. En *Gestor de URLs Adicionales de Tarjetas*, asegúrate de tener:  
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. En **Herramientas -> Placa**, selecciona: **ESP32C3 Dev Module** (o tu placa específica).
4. Opciones de compilación recomendadas:
   - **USB CDC On Boot**: *Enabled*
   - **Flash Frequency**: *80MHz*
   - **Flash Mode**: *DIO*
5. Abre el archivo `honda-civic-scanner.ino` (todos los archivos `.h` y `.cpp` deben estar en la misma carpeta).
6. Presiona **Subir** (Upload).

---

## 7. Puesta en Marcha y Uso con Bluetooth Low Energy (BLE)

El escáner ahora funciona de forma nativa por **Bluetooth Low Energy (BLE 5.0)** con el servicio industrial **Nordic UART (NUS)**, lo que elimina las desconexiones de Wi-Fi y mantiene tus datos móviles (4G/5G) activos.

### Paso 1: Conectar y encender
1. Conecta el escáner al conector de 3 pines de tu Civic.
2. Gira la llave de encendido a la posición **ON (IGN II)** (el LED del ESP32 comenzará a parpadear).

### Paso 2: Abrir la Aplicación Móvil (PWA)
Web Bluetooth requiere una conexión segura (HTTPS o localhost). Tienes dos formas súper sencillas:

* **Opción A (Desde tu red local con tu laptop):**
  1. En tu laptop, ejecuta en la terminal:
     ```bash
     ./serve_https.py
     ```
  2. En tu celular (conectado al mismo Wi-Fi de tu casa o al hotspot de tu teléfono), abre Chrome o Brave y navega a la dirección que te indique el script (ej. `https://192.168.1.50:8443`).
  3. Acepta el certificado local (*Configuración avanzada -> Continuar a sitio no seguro*).
  4. Pulsa en el menú de Chrome de 3 puntos: **"Agregar a la pantalla de inicio"** o **"Instalar aplicación"**.
  5. ¡Listo! Se creará un icono llamado **Civic OBD** en tu celular que abre la app a pantalla completa y funciona offline.

* **Opción B (Publicado en GitHub Pages):**
  Puedes subir la carpeta `app/` a tu repositorio de GitHub y activar GitHub Pages para tener una URL HTTPS pública permanente gratuita (ej. `https://usuario.github.io/honda-civic-scanner/`).

### Paso 3: Conectar el Escáner por Bluetooth
1. Abre la aplicación **Civic OBD** en tu celular.
2. Presiona el botón **⚡ Conectar BLE**.
3. Selecciona tu dispositivo de la lista: **`K-Lino-XXXX`** (los 4 caracteres finales identifican tu equipo; firmware anterior: `Civic99-OBD`).
4. ¡Listo! En milisegundos se sincronizarán en vivo:
   - Tacómetro de aguja fluido a 20-30 Hz (con selector de fórmula OBD1/OBD2).
   - Indicadores de VTEC, A/C Clutch, Freno, Gear automático y sensores.
   - Pestaña **Datalogger**: Graba pasadas con botón Rec/Stop, mide aceleración 0-60 km/h, 0-100 km/h y 1/4 de milla automático, y descarga el archivo **CSV** directo a tu teléfono.
   - Pestaña **Diagnóstico**: Escaneo de códigos de falla DTC en memoria 0x40-0x5F y borrado con 1 toque.
