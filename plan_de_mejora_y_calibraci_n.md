# Plan de Mejora Integral: Honda Civic '99 (D16Y8) K-Line Scanner & PWA

Este documento detalla la hoja de ruta técnica (mecánica, firmware en C++ y aplicación web PWA) basada **exclusivamente** en la evidencia recolectada el **30 de octubre de 2026**:
1. **Log Diurno:** `Civic99_Log_2026-10-30T12-48-07.csv` (29.88 min, 29,308 muestras).
2. **Log Nocturno:** `Civic99_Log_2026-10-30T19-55-30.csv` (con jalón VTEC en 1ª y 2ª marcha previo a la falla).
3. **Captura de Diagnóstico en Vivo:** `image_7e429e.jpg` (`Fila 0x00` y `Fila 0x10` en hexadecimal crudo con el último firmware).

---

## 1. Resumen de Evidencia del 30 de Octubre

### A. Comportamiento de los Sensores de Oxígeno y Lazo Cerrado
* **Sensor O2 Primario (`0x15` = `d[5]`) SANO:**
  * En la captura en vivo (`Fila 0x10: 44 8C 3E 8D 18 2B 00 92...`), el byte `d[5]` vale `0x2B` ($43 / 51.3 = \mathbf{0.84\text{ V}}$).
  * En el log nocturno (`19-55-30.csv`), durante el acelerón a fondo (*WOT*) en el **minuto 4.59** (antes de que entrara el modo de fallo), el motor activó el VTEC hasta **$6,345\text{ RPM}$** y **$88\text{ km/h}$**, manteniendo el voltaje del O2 primario entre **$0.83\text{ V}$ y $0.87\text{ V}$** con pulsos de inyección de $11.5\text{ ms}$. Esto descarta problemas de bomba de gasolina, regulador de presión o degradación del sensor primario.
* **Sensor O2 Secundario (`0x16` = `d[6]`) MUERTO en $0.00\text{ V}$ (Causa del Código 63):**
  * En la `Fila 0x10` de tu captura, el byte `d[6]` (dirección `0x16`, ubicado inmediatamente después del O2 primario `0x2B` y antes del voltaje de batería `0x92`) vale **`0x00` ($0.00\text{ V}$)**.
  * En ambos recorridos del 30 de octubre ocurre exactamente la misma secuencia cuando el motor cumple ~10 minutos de operación y el refrigerante (`ECT_C`) supera los $85\text{ °C} - 91\text{ °C}$:
    * **Log 12:48:** Minuto **8.8 a 9.70** (segundo 581.9).
    * **Log 19:55:** Minuto **9.8 a 10.64** (segundo 638.4).
  * En ese instante, la ECU corre el monitor de autodiagnóstico del sensor secundario, detecta `0x00` ($0.00\text{ V}$ continuos), **confirma el Código 63 (`P0137` - Bajo voltaje en Sensor O2 Secundario)**, enciende el *Check Engine* (`MIL`) y entra en modo de protección (*Fail-Safe / Open Loop*), **clavando el `SFT_pct` en $0\%$** por el resto del viaje.

### B. Hallazgos de Software en el Firmware y Dashboard Actual
1. **Truncamiento del JSON de `GET_DIAG` en BLE (`honda-civic-scanner.ino`):** En la captura de pantalla solo aparecen `Fila 0x00` y `Fila 0x10`, faltando `Fila 0x20`, `Fila 0x30`, `DTC 0x40` y `DTC 0x50`. Esto sucede porque `GET_DIAG` se responde dentro del callback de interrupción BLE (`RxCallbacks::onWrite`) enviando fragmentos con `delay(10)`, mientras el `loop()` principal dispara una trama binaria de 28 bytes (`0xA5 0x5A...`) que se atraviesa a mitad del texto JSON.
2. **Variable inexistente en `app/index.html` (línea 1164):** La función `startDiagPolling()` evalúa `if (bleConnected) sendBleCmd('GET_DIAG');`, pero la variable declarada en el script es `isConnected`. Esto detiene el refresco automático cada segundo en la pestaña de Diagnóstico.
3. **Detección del bit `MIL` en `Fila 0x00` (`0x0B` = `d[11]`):** En tu captura con el Check Engine encendido en el tablero, la `Fila 0x00` muestra `0F 9E 00 00 00 00 00 00 10 00 90 41 01 02 00 01`. El byte `d[11]` (`0x0B`) vale **`0x41`** (`0100 0001` en binario: **Bit 6** y **Bit 0** en `1`). El código actual solo revisa el Bit 5 (`1 << 5`), ignorando el Bit 6 (`0x40`).
4. **Falta de lectura y visualización del Sensor O2 Secundario (`0x16`):** El byte `d[6]` de la fila `0x10` ya llega desde la ECU en cada lectura, pero no se guarda ni se envía a la app.

---

## 2. FASE 1: Plan de Acción Mecánico (Corrección del Código 63)

1. **Inspección física debajo del auto (Catalizador y Arnés):**
   * Localizar el **Sensor de O2 Secundario** atornillado en el cuerpo del convertidor catalítico (debajo de la consola central/palanca de velocidades).
   * Revisar que el cableado no esté rozando con el tubo de escape, trozado o desconectado en el enchufe de 4 pines que sube a través del piso hacia la cabina.
2. **Prueba eléctrica rápida con multímetro:**
   * **Resistencia del calefactor (lado del sensor desconectado):** Medir continuidad entre los **2 cables del mismo color** (usualmente negros). Debe marcar entre **$10\text{ }\Omega$ y $40\text{ }\Omega$**. Si marca circuito abierto ($OL$), el calefactor está quemado.
   * **Alimentación y señal (lado del arnés del auto con switch en ON II):** Verificar que haya $12\text{ V}$ en el circuito del calefactor y que el cable de señal (blanco) y tierra de sensores (verde) no estén en corto a chasis (lo cual forzaría la lectura de $0.00\text{ V}$).
3. **Reemplazo del Sensor Secundario:**
   * Si el arnés está íntegro, reemplazar el sensor de oxígeno secundario (tipo planar/circonio de 4 cables compatible con D16Y8) y borrar códigos desde la app con motor apagado y llave en `ON (II)`.

---

## 3. FASE 2: Cambios en el Firmware C++ (ESP32-C3)

### Archivo 1: `honda_kline.h`
Agregar `o2Sec` en `TelemetryData` y los bytes crudos `rawO2SecByte` y `d11Byte` en `DiagInfo` para auditar el sensor secundario y el testigo MIL:

```cpp
// Dentro de struct TelemetryData (Sección B):
  float o2 = 0.0f;            // 0x15: Sonda lambda primaria en Volts (0.0 a 1.25 V)
  float o2Sec = 0.0f;         // 0x16: Sonda lambda secundaria en Volts (0.0 a 1.25 V)
  float batt = 0.0f;          // 0x17: Voltaje de batería en V
```

```cpp
// Dentro de struct DiagInfo:
  uint8_t rawEldByte = 0;
  uint8_t rawO2SecByte = 0;
  uint8_t d8Byte = 0;
  uint8_t d10Byte = 0;
  uint8_t d11Byte = 0;
```

---

### Archivo 2: `honda_kline.cpp`

#### A. En `HondaKLine::update()` -> `case 0:` (Bloque `0x00`)
Actualizar la captura de `d[11]` (`0x0B`) para reconocer el valor `0x41` observado en la captura del 30 de octubre cuando el Check Engine está encendido:

```cpp
        diag.d8Byte = d[8];
        diag.d10Byte = d[10];
        diag.d11Byte = d[11];

        // Estados discretos (0x08, 0x0A y 0x0B)
        data.vtec = ((d[10] & (1 << 2)) != 0) || ((d[10] & (1 << 3)) != 0) || ((d[8] & (1 << 7)) != 0);
        data.brake = (d[8] & (1 << 3)) != 0;
        data.ac = ((d[8] & (1 << 2)) != 0) || ((d[8] & (1 << 1)) != 0);
        data.parkNeutral = ((d[8] & (1 << 4)) != 0) || ((d[8] & (1 << 5)) != 0);
        // En ECU P2P (D16Y8), 0x0B reporta 0x41 con Check Engine activo (Bit 6 / Bit 5)
        data.mil = ((d[11] & (1 << 5)) != 0) || ((d[11] & (1 << 6)) != 0);
```

#### B. En `HondaKLine::update()` -> `case 1:` (Bloque `0x10`)
Decodificar `d[6]` (`0x16`) como el voltaje del Sensor O2 Secundario:

```cpp
        data.baro  = (int)roundf(d[3] * 0.716f - 5.0f);
        data.tps   = constrain(((int)d[4] - 24) / 2, 0, 100);
        data.o2    = constrain(d[5] / 51.3f, 0.0f, 1.25f);
        data.o2Sec = constrain(d[6] / 51.3f, 0.0f, 1.25f);
        diag.rawO2SecByte = d[6];
        data.batt  = d[7] / 10.45f;
        data.altFr = constrain((int)roundf(d[8] / 2.55f), 0, 100);
```

*(En `HondaKLine::updateDemo()` también se puede añadir `data.o2Sec = 0.65f;` para mantener consistencia en modo simulación).*

---

### Archivo 3: `honda-civic-scanner.ino`

#### A. Evitar colisión entre el JSON de `GET_DIAG` y la telemetría binaria
Actualmente `GET_DIAG` transmite desde `RxCallbacks::onWrite()`, lo que permite que `loop()` envíe la trama binaria `0xA5 0x5A` en medio de los fragmentos JSON. Para solucionarlo:

1. Agregar `CMD_GET_DIAG` al `enum PendingCmd`:
```cpp
enum PendingCmd : uint8_t {
  CMD_NONE,
  CMD_GET_DTC,
  CMD_CLEAR_DTC,
  CMD_ECHO,
  CMD_REINIT,
  CMD_GET_DIAG
};
```

2. En `RxCallbacks::onWrite()`, encolar `CMD_GET_DIAG` en lugar de transmitir ahí mismo:
```cpp
    } else if (rxValue == "GET_DIAG") {
      pendingCmd = CMD_GET_DIAG;
    }
```

3. En `handlePendingCmd(PendingCmd cmd)`, agregar el bloque `case CMD_GET_DIAG:` que se ejecuta de forma síncrona dentro de `loop()` (pausando el envío binario hasta terminar todos los fragmentos del JSON e incluyendo `rawO2Sec` y `d11`):
```cpp
    case CMD_GET_DIAG: {
      const DiagInfo &g = honda.diag;
      char diagBuf[768];
      snprintf(diagBuf, sizeof(diagBuf),
               "{\"type\":\"diag\",\"state\":\"%s\",\"connected\":%s,\"mil\":%s,"
               "\"cmdSent\":%d,\"cmdOk\":%d,\"cmdTimeout\":%d,\"cmdCrcFail\":%d,"
               "\"rxBytes\":%d,\"echoOk\":%d,\"echoFail\":%d,\"ecuType\":%d,"
               "\"uptime\":%lu,\"row0\":\"%s\",\"row1\":\"%s\",\"row2\":\"%s\",\"row3\":\"%s\","
               "\"dtc0\":\"%s\",\"dtc1\":\"%s\",\"rawEld\":%d,\"rawO2Sec\":%d,"
               "\"d8\":%d,\"d10\":%d,\"d11\":%d}",
               honda.stateName(), honda.data.connected ? "true" : "false", honda.data.mil ? "true" : "false",
               g.cmdSent, g.cmdOk, g.cmdTimeout, g.cmdCrcFail,
               g.rxBytes, g.echoOk, g.echoFail, honda.ecuType,
               millis() / 1000,
               g.row0Hex, g.row1Hex, g.row2Hex, g.row3Hex,
               g.dtcRawHex[0], g.dtcRawHex[1], g.rawEldByte, g.rawO2SecByte,
               g.d8Byte, g.d10Byte, g.d11Byte);
      bleSendText(diagBuf);
      lastBleStreamMs = millis(); // Evita que la trama binaria salga pegada al JSON
      break;
    }
```

4. *(Opcional recomendado en `bleSendText`)*: Subir el tamaño de fragmento o dar `delay(15)` entre fragmentos para asegurar que la pila BLE del teléfono procese las 6 filas hexadecimales completas sin perder paquetes.

---

## 4. FASE 3: Cambios en el Dashboard PWA (`app/index.html` y `app/sw.js`)

### 1. Corregir el bug de `bleConnected` en `startDiagPolling()` (`app/index.html`)
Reemplazar la función `startDiagPolling()` (alrededor de la línea 1160) para usar la variable correcta `isConnected`:

```javascript
    let diagPollInterval = null;
    function startDiagPolling() {
      if (diagPollInterval) clearInterval(diagPollInterval);
      if (isConnected) sendBleCmd('GET_DIAG');
      diagPollInterval = setInterval(() => {
        if (isConnected) sendBleCmd('GET_DIAG');
      }, 1500);
    }
```

### 2. Corregir el reensamblaje de fragmentos JSON en `handleBleNotification()` (`app/index.html`)
Eliminar el segundo `if` que intentaba parsear el buffer antes de recibir el delimitador de fin de línea `\n` (lo cual fallaba cuando un fragmento de 128 bytes terminaba casualmente en `}` o se mezclaba con otro mensaje):

```javascript
      // Reensamblaje seguro de fragmentos JSON delimitados por '\n'
      try {
        bleTextBuffer += textDecoder.decode(value, { stream: true });
        if (bleTextBuffer.length > 4096) bleTextBuffer = ''; // Protección contra desbordamiento
        while (bleTextBuffer.includes('\n')) {
          const idx = bleTextBuffer.indexOf('\n');
          const line = bleTextBuffer.substring(0, idx).trim();
          bleTextBuffer = bleTextBuffer.substring(idx + 1);
          const startBrace = line.indexOf('{');
          const endBrace = line.lastIndexOf('}');
          if (startBrace !== -1 && endBrace > startBrace) {
            handleJsonMessage(line.substring(startBrace, endBrace + 1));
          }
        }
      } catch (err) {
        console.error('Error procesando texto BLE:', err);
      }
```

### 3. Mostrar diagnóstico del Sensor O2 Secundario (`0x16`) y `Fila 0x20 / 0x30` en `handleJsonMessage()` (`app/index.html`)
Dentro de `else if (msg.type === 'diag')`, extraer `rawO2Sec` y mostrar el voltaje calculado junto con su estado para que puedas verificar la reparación del Código 63 en tiempo real sin necesidad de exportar un CSV:

```javascript
            const rawO2Sec = (msg.rawO2Sec !== undefined) ? msg.rawO2Sec : 0;
            const o2SecVolt = (rawO2Sec / 51.3).toFixed(2);
            const o2SecState = rawO2Sec === 0
              ? '0.00V (Circuito abierto / Sensor S2 inactivo - Causa Código 63)'
              : `${o2SecVolt} V (Señal activa)`;
```
Y agregar el bloque visual en el contenedor `rawContainer.innerHTML` debajo del bloque del ELD:
```javascript
                <div style="font-size: 0.75rem; font-weight: bold; color: var(--primary); margin-top: 8px; margin-bottom: 6px; text-transform: uppercase; letter-spacing: 0.5px;">
                  Sensor O2 Secundario Post-Catalizador (Byte 0x16)
                </div>
                <div style="font-size: 0.8rem; background: rgba(0,0,0,0.3); padding: 8px 10px; border-radius: 6px; margin-bottom: 10px; border: 1px solid rgba(255,255,255,0.06); line-height: 1.5;">
                  <div>Byte O2 S2 ADC: <strong style="color:var(--text);">${rawO2Sec}</strong> (0x${rawO2Sec.toString(16).toUpperCase().padStart(2, '0')}) | <strong>${o2SecVolt} V</strong></div>
                  <div style="color: var(--text-dim); font-size: 0.75rem;">Estado: <span style="color: ${rawO2Sec === 0 ? '#ff1744' : 'var(--vtec)'}; font-weight: bold;">${o2SecState}</span></div>
                </div>
```

### 4. Incrementar versión de caché en `app/sw.js`
Para forzar a Chrome en tu teléfono a descargar el nuevo `index.html` inmediatamente:
```javascript
const CACHE_NAME = 'civic-obd-v21';
```

---

## 5. FASE 4: Protocolo de Pruebas y Validación

1. **Verificación de Filas Hexadecimales Completas (`GET_DIAG`):**
   * Compilar y flashear el ESP32-C3 (`./flash.sh`) y actualizar la PWA.
   * Conectar por Bluetooth y abrir la pestaña **DIAG**.
   * Confirmar que ahora se visualizan las **6 filas completas** (`Fila 0x00`, `Fila 0x10`, `Fila 0x20`, `Fila 0x30`, `DTC 0x40` y `DTC 0x50`) actualizándose cada 1.5 segundos y el recuadro de **Sensor O2 Secundario (`0x16`)**.
2. **Validación de Reparación del Sensor O2 Secundario:**
   * Tras reconectar o reemplazar el sensor secundario en el catalizador, verificar en la pestaña **DIAG** que el `Byte O2 S2 ADC (0x16)` suba de `0 (0x00)` a valores normales de operación ($0.10\text{ V} - 0.85\text{ V}$).
3. **Prueba de Ruta de 15+ Minutos (Confirmación de Lazo Cerrado Continuo):**
   * Iniciar grabación en el **Datalogger** y conducir hasta superar los **12 minutos** con `ECT_C >= 88 °C`.
   * Confirmar que el testigo **CHECK** no se encienda y que el ajuste de mezcla **`SFT_pct` siga ciclando activamente** (entre $-15\%$ y $+10\%$) después del minuto 11, sin volver a clavarse en $0\%$.