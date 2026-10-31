# Plan de Mejora Integral: Honda Civic '99 (D16Y8) K-Line Scanner & PWA

Este documento detalla la hoja de ruta técnica completa (mecánica, firmware en C++ y aplicación web PWA) basada **exclusivamente** en la evidencia recolectada el **30 de octubre de 2026** y en la auditoría de lectura/borrado de códigos DTC:

1. **Log Diurno:** `Civic99_Log_2026-10-30T12-48-07.csv` (29.88 min, 29,308 muestras).
2. **Log Nocturno:** `Civic99_Log_2026-10-30T19-55-30.csv` (con jalón VTEC en 1ª y 2ª marcha previo a la falla).
3. **Captura de Diagnóstico en Vivo:** `image_7e429e.jpg` (`Fila 0x00` y `Fila 0x10` en hexadecimal crudo con el último firmware).

---

## 1. Resumen de Evidencia del 30 de Octubre

### A. Comportamiento de los Sensores de Oxígeno y Lazo Cerrado

* **Sensor O2 Primario (`0x15` = `d[5]`) SANO:**
  * En la captura en vivo (`Fila 0x10: 44 8C 3E 8D 18 2B 00 92...`), el byte `d[5]` vale `0x2B` ($43 / 51.3 = \mathbf{0.84\text{ V}}$).
  * En el log nocturno (`19-55-30.csv`), durante el acelerón a fondo (*WOT*) en el **minuto 4.59** (antes de que entrara el modo de fallo), el motor activó el VTEC hasta $6,345\text{ RPM}$ y $88\text{ km/h}$, manteniendo el voltaje del O2 primario entre $0.83\text{ V}$ **y** $0.87\text{ V}$ con pulsos de inyección de $11.5\text{ ms}$. Esto descarta problemas de bomba de gasolina, regulador de presión o degradación del sensor primario.

* **Sensor O2 Secundario (`0x16` = `d[6]`) MUERTO en** $0.00\text{ V}$ **(Causa del Código 63):**
  * En la `Fila 0x10` de tu captura, el byte `d[6]` (dirección `0x16`, ubicado inmediatamente después del O2 primario `0x2B` y antes del voltaje de batería `0x92`) vale **`0x00` (**$0.00\text{ V}$**)**.
  * En ambos recorridos del 30 de octubre ocurre exactamente la misma secuencia cuando el motor cumple \~10 minutos de operación y el refrigerante (`ECT_C`) supera los $85\text{ °C} - 91\text{ °C}$:
    * **Log 12:48:** Minuto **8.8 a 9.70** (segundo 581.9).
    * **Log 19:55:** Minuto **9.8 a 10.64** (segundo 638.4).
  * En ese instante, la ECU corre el monitor de autodiagnóstico del sensor secundario, detecta `0x00` ($0.00\text{ V}$ continuos), **confirma el Código 63 (`P0137` - Bajo voltaje en Sensor O2 Secundario)**, enciende el *Check Engine* (`MIL`) y entra en modo de protección (*Fail-Safe / Open Loop*), **clavando el `SFT_pct` en** $0\%$ por el resto del viaje.

### B. Hallazgos de Software en el Firmware y Dashboard (Telemetría + DTC)

1. **Truncamiento del JSON de `GET_DIAG` en BLE (`honda-civic-scanner.ino`):** `GET_DIAG` se respondía dentro del callback de interrupción BLE mientras el `loop()` principal disparaba una trama binaria de 28 bytes (`0xA5 0x5A...`) en medio de los fragmentos JSON.
2. **Colisión entre `GET_DIAG` y `GET_DTC` / `CLEAR_DTC`:** Como `startDiagPolling()` dispara `GET_DIAG` cada 1.5 s en la misma pestaña donde están los botones de *Escanear* y *Borrar*, un `GET_DIAG` entrante puede sobrescribir `pendingCmd` antes de que `loop()` ejecute `CMD_GET_DTC` o `CMD_CLEAR_DTC`, además de provocar errores `GATT operation already in progress` en el navegador si no se encolan las escrituras BLE.
3. **Verificación incompleta tras `CMD_CLEAR_DTC` (`honda-civic-scanner.ino`):** Al borrar códigos (`21 04 01 DA`), la ECU se reinicia y desconecta la sesión. Llamar a `getActiveDTCs()` tras 500 ms sin reconectar con `sendInit()` ni validar el `bool` de retorno hacía que un fallo de lectura por timeout reportara `remaining = 0` (falso éxito).
4. **Bug de código muerto en DTC 22/23 y lectura de fila `0x60` (`honda_kline.cpp`):** El ajuste `if (row == 0 && code == 23) code = 22; else if (row == 0 && code == 24) code = 23;` convertía el código 23 (Sensor Knock) en 22 y tenía una rama inalcanzable (`code` siempre es impar en el nibble bajo). Además, la fila `0x60` (códigos 64 a 95: Catalizador 67, Misfires 70–74, EVAP 90–92) solo se imprimía en consola pero no se agregaba a `activeCodes` ni a `dtcRawHex`.
5. **Falta de actualización de `lastSuccessTime` en `readMemory()`:** Durante la lectura de múltiples bloques DTC (`0x40`, `0x50`, `0x60`) con reintentos, `data.lastSuccessTime` no se actualizaba, acercándose peligrosamente al timeout de desconexión de 3 segundos.

---

## 2. FASE 1: Plan de Acción Mecánico (Corrección del Código 63)

1. **Inspección física debajo del auto (Catalizador y Arnés):**
   * Localizar el **Sensor de O2 Secundario** atornillado en el cuerpo del convertidor catalítico (debajo de la consola central/palanca de velocidades).
   * Revisar que el cableado no esté rozando con el tubo de escape, trozado o desconectado en el enchufe de 4 pines que sube a través del piso hacia la cabina.
2. **Prueba eléctrica rápida con multímetro:**
   * **Resistencia del calefactor (lado del sensor desconectado):** Medir continuidad entre los **2 cables del mismo color** (usualmente negros). Debe marcar entre $10\text{ }\Omega$ **y** $40\text{ }\Omega$. Si marca circuito abierto ($OL$), el calefactor está quemado.
   * **Alimentación y señal (lado del arnés del auto con switch en ON II):** Verificar que haya $12\text{ V}$ en el circuito del calefactor y que el cable de señal (blanco) y tierra de sensores (verde) no estén en corto a chasis (lo cual forzaría la lectura de $0.00\text{ V}$).
3. **Reemplazo del Sensor Secundario:**
   * Si el arnés está íntegro, reemplazar el sensor de oxígeno secundario (tipo planar/circonio de 4 cables compatible con D16Y8) y borrar códigos desde la app con **motor apagado y llave en `ON (II)`**.

---

## 3. FASE 2: Cambios en el Firmware C++ (ESP32-C3)

### Archivo 1: `honda_kline.h`

1. Agregar `o2Sec` en `TelemetryData`.
2. En `DiagInfo`, ampliar `dtcRawHex` a 3 filas (`0x40`, `0x50`, `0x60`) y agregar `rawO2SecByte` y `d11Byte`:

```cpp
// Dentro de struct TelemetryData (Sección B):
  float o2 = 0.0f;            // 0x15: Sonda lambda primaria en Volts (0.0 a 1.25 V)
  float o2Sec = 0.0f;         // 0x16: Sonda lambda secundaria en Volts (0.0 a 1.25 V)
  float batt = 0.0f;          // 0x17: Voltaje de batería en V
```

```cpp
// Dentro de struct DiagInfo:
  char row0Hex[64] = "";
  char row1Hex[64] = "";
  char row2Hex[64] = "";
  char row3Hex[64] = "";
  char dtcRawHex[3][64] = {"", "", ""};
  uint8_t rawEldByte = 0;
  uint8_t rawO2SecByte = 0;
  uint8_t d8Byte = 0;
  uint8_t d10Byte = 0;
  uint8_t d11Byte = 0;
```

---

### Archivo 2: `honda_kline.cpp`

#### A. Actualizar `data.lastSuccessTime` dentro de `HondaKLine::readMemory()`
Justo después de verificar el checksum válido (`diag.cmdOk++;`), actualizar el reloj de última respuesta exitosa para que las lecturas de DTC nunca disparen el timeout de 3 segundos:

```cpp
  diag.cmdOk++;
  data.lastSuccessTime = millis();
  memcpy(out, resp + 2, len);
  return true;
```

#### B. En `HondaKLine::update()` -> `case 0:` (Bloque `0x00`) y `case 1:` (Bloque `0x10`)
Detectar el bit `MIL` en `0x0B` (`0x41`) y leer `o2Sec` en `0x16`:

```cpp
        // En case 0 (Bloque 0x00):
        diag.d8Byte = d[8];
        diag.d10Byte = d[10];
        diag.d11Byte = d[11];

        data.vtec = ((d[10] & (1 << 2)) != 0) || ((d[10] & (1 << 3)) != 0) || ((d[8] & (1 << 7)) != 0);
        data.brake = (d[8] & (1 << 3)) != 0;
        data.ac = ((d[8] & (1 << 2)) != 0) || ((d[8] & (1 << 1)) != 0);
        data.parkNeutral = ((d[8] & (1 << 4)) != 0) || ((d[8] & (1 << 5)) != 0);
        // En ECU P2P (D16Y8), 0x0B reporta 0x41 con Check Engine activo (Bit 6 / Bit 5)
        data.mil = ((d[11] & (1 << 5)) != 0) || ((d[11] & (1 << 6)) != 0);
```

```cpp
        // En case 1 (Bloque 0x10):
        data.baro  = (int)roundf(d[3] * 0.716f - 5.0f);
        data.tps   = constrain(((int)d[4] - 24) / 2, 0, 100);
        data.o2    = constrain(d[5] / 51.3f, 0.0f, 1.25f);
        data.o2Sec = constrain(d[6] / 51.3f, 0.0f, 1.25f);
        diag.rawO2SecByte = d[6];
        data.batt  = d[7] / 10.45f;
        data.altFr = constrain((int)roundf(d[8] / 2.55f), 0, 100);
```

#### C. Reemplazar `pushDtc` y `HondaKLine::getActiveDTCs()` (Corrección de DTC 22/23, filtro de duplicados y soporte de fila `0x60`)
Eliminar el bug que impedía detectar el Código 23 (Knock) y agregar soporte seguro para códigos mayores a 63 en la fila `0x60` validando contra `HONDA_DTC_TABLE`:

```cpp
static bool isKnownDtc(int codeNum) {
  for (size_t t = 0; t < sizeof(HONDA_DTC_TABLE) / sizeof(HONDA_DTC_TABLE[0]); t++) {
    if (HONDA_DTC_TABLE[t].code == codeNum) return true;
  }
  return false;
}

static void pushDtc(std::vector<DtcCode>& v, int codeNum) {
  // Evitar códigos duplicados
  for (size_t i = 0; i < v.size(); i++) {
    if (v[i].code == codeNum) return;
  }
  for (size_t t = 0; t < sizeof(HONDA_DTC_TABLE) / sizeof(HONDA_DTC_TABLE[0]); t++) {
    if (HONDA_DTC_TABLE[t].code == codeNum) {
      v.push_back(HONDA_DTC_TABLE[t]);
      return;
    }
  }
  v.push_back({ codeNum, "Código Honda", "Código no listado en la tabla; consulta el manual de servicio." });
}

bool HondaKLine::getActiveDTCs(std::vector<DtcCode>& activeCodes) {
  activeCodes.clear();

  if (data.demo) {
    activeCodes.push_back(HONDA_DTC_TABLE[0]); // Code 1 (O2)
    activeCodes.push_back(HONDA_DTC_TABLE[9]); // Code 14 (IACV)
    data.dtcCount = (int)activeCodes.size();
    return true;
  }

  uint8_t d[16];
  // Leer bloques 0x40 (row 0) y 0x50 (row 1) obligatorios
  for (int row = 0; row < 2; row++) {
    uint8_t addr = 0x40 + row * 0x10;
    bool ok = false;
    for (int attempt = 0; attempt < 3 && !ok; attempt++) {
      ok = readMemory(addr, 0x10, d);
      if (!ok) delay(25);
    }
    if (!ok) {
      klog("ERROR: Fallo al leer memoria DTC en 0x%02X tras 3 intentos", addr);
      return false;
    }

    snprintf(diag.dtcRawHex[row], sizeof(diag.dtcRawHex[row]),
             "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
             d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
    logHex(row == 0 ? "DTC40" : "DTC50", d, 16);

    for (int i = 0; i < 16; i++) {
      if (d[i] == 0xFF) continue; // Ignorar byte sin inicializar
      int baseCode = row * 32 + i * 2;
      if (d[i] >> 4) {
        if (baseCode > 0) pushDtc(activeCodes, baseCode);
        else klog("DTC byte 0 hi-nibble set (bandera interna ECU, ignorando)");
      }
      if (d[i] & 0x0F) {
        int oddCode = baseCode + 1;
        pushDtc(activeCodes, oddCode);
      }
    }
  }

  // Leer bloque extendido 0x60 (row 2: códigos 64 a 95 como P0420, Misfires 71-74, EVAP 90-92)
  if (readMemory(0x60, 0x10, d)) {
    snprintf(diag.dtcRawHex[2], sizeof(diag.dtcRawHex[2]),
             "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
             d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
    logHex("DTC60", d, 16);

    for (int i = 0; i < 16; i++) {
      if (d[i] == 0x00 || d[i] == 0xFF) continue;
      int baseCode = 64 + i * 2;
      if ((d[i] >> 4) && isKnownDtc(baseCode)) {
        pushDtc(activeCodes, baseCode);
      }
      if ((d[i] & 0x0F) && isKnownDtc(baseCode + 1)) {
        pushDtc(activeCodes, baseCode + 1);
      }
    }
  }

  data.dtcCount = (int)activeCodes.size();
  return true;
}
```

---

### Archivo 3: `honda-civic-scanner.ino`

#### A. Prioridad de Comandos DTC sobre `GET_DIAG` en `RxCallbacks::onWrite()`
Evitar que el polling automático de `GET_DIAG` pise un comando `GET_DTC` o `CLEAR_DTC` recién solicitado por el usuario:

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

```cpp
    // Dentro de RxCallbacks::onWrite():
    else if (rxValue == "GET_DIAG") {
      // Solo encolar GET_DIAG si no hay un comando prioritario (GET_DTC / CLEAR_DTC) pendiente
      if (pendingCmd == CMD_NONE) {
        pendingCmd = CMD_GET_DIAG;
      }
    }
```

#### B. Reconexión y Verificación Real en `handlePendingCmd(PendingCmd cmd)`
Actualizar `CMD_CLEAR_DTC` para que reconecte con `sendInit()` tras el reset de la ECU, actualice el estado real del bit `MIL` (`0x0B`) y verifique si la lectura post-borrado fue exitosa (`verifyOk`), además de enviar `dtc2` (`0x60`) en `CMD_GET_DIAG`:

```cpp
    case CMD_CLEAR_DTC: {
      bool clearAck = honda.clearDTCs();
      delay(800); // Esperar reinicio interno de la ECU P2P tras comando 21 04 01 DA
      bool reinitOk = honda.sendInit(); // Reconecta y lee bloque 0x00 (actualizando data.mil real)
      std::vector<DtcCode> remainingCodes;
      bool verifyOk = reinitOk && honda.getActiveDTCs(remainingCodes);
      int remainingCount = verifyOk ? (int)remainingCodes.size() : -1;

      char clearBuf[180];
      snprintf(clearBuf, sizeof(clearBuf),
               "{\"type\":\"clearDtc\",\"ack\":%s,\"verifyOk\":%s,\"remaining\":%d,\"mil\":%s}",
               clearAck ? "true" : "false",
               verifyOk ? "true" : "false",
               remainingCount,
               honda.data.mil ? "true" : "false");
      bleSendText(clearBuf);
      lastBleStreamMs = millis();
      break;
    }

    case CMD_GET_DIAG: {
      const DiagInfo &g = honda.diag;
      char diagBuf[820];
      snprintf(diagBuf, sizeof(diagBuf),
               "{\"type\":\"diag\",\"state\":\"%s\",\"connected\":%s,\"mil\":%s,"
               "\"cmdSent\":%d,\"cmdOk\":%d,\"cmdTimeout\":%d,\"cmdCrcFail\":%d,"
               "\"rxBytes\":%d,\"echoOk\":%d,\"echoFail\":%d,\"ecuType\":%d,"
               "\"uptime\":%lu,\"row0\":\"%s\",\"row1\":\"%s\",\"row2\":\"%s\",\"row3\":\"%s\","
               "\"dtc0\":\"%s\",\"dtc1\":\"%s\",\"dtc2\":\"%s\",\"rawEld\":%d,\"rawO2Sec\":%d,"
               "\"d8\":%d,\"d10\":%d,\"d11\":%d}",
               honda.stateName(), honda.data.connected ? "true" : "false", honda.data.mil ? "true" : "false",
               g.cmdSent, g.cmdOk, g.cmdTimeout, g.cmdCrcFail,
               g.rxBytes, g.echoOk, g.echoFail, honda.ecuType,
               millis() / 1000,
               g.row0Hex, g.row1Hex, g.row2Hex, g.row3Hex,
               g.dtcRawHex[0], g.dtcRawHex[1], g.dtcRawHex[2], g.rawEldByte, g.rawO2SecByte,
               g.d8Byte, g.d10Byte, g.d11Byte);
      bleSendText(diagBuf);
      lastBleStreamMs = millis();
      break;
    }
```

---

## 4. FASE 3: Cambios en el Dashboard PWA (`app/index.html` y `app/sw.js`)

### 1. Cola Asíncrona de Escritura BLE (`sendBleCmd`) en `app/index.html`
Evita el error `DOMException: GATT operation already in progress` en Web Bluetooth cuando coincide un clic en *Escanear/Borrar* con el polling de `GET_DIAG`:

```javascript
    let isBleWriting = false;
    const bleCmdQueue = [];

    async function sendBleCmd(cmd) {
      if (!rxChar || !isConnected) return;
      // Evitar acumular múltiples GET_DIAG repetidos en la cola
      if (cmd === 'GET_DIAG' && bleCmdQueue.includes('GET_DIAG')) return;
      bleCmdQueue.push(cmd);
      if (isBleWriting) return;

      isBleWriting = true;
      while (bleCmdQueue.length > 0 && rxChar && isConnected) {
        const nextCmd = bleCmdQueue.shift();
        try {
          const encoder = new TextEncoder();
          await rxChar.writeValue(encoder.encode(nextCmd));
        } catch (err) {
          console.error('Error enviando comando BLE:', nextCmd, err);
        }
      }
      isBleWriting = false;
    }
```

### 2. Mejorar `fetchDtcs()`, `executeClearDtcs()` y `handleClearDtcResponse()` en `app/index.html`
Dar prioridad inmediata en la cola BLE a las acciones manuales del usuario, mostrar estado visual mientras la ECU se reinicia y validar `msg.verifyOk`:

```javascript
    function fetchDtcs() {
      bleCmdQueue.length = 0; // Limpiar GET_DIAG pendientes para prioridad inmediata
      sendBleCmd('GET_DTC');
      document.getElementById('dtcList').innerHTML =
        '<div style="text-align:center; padding: 20px; color: var(--text-dim);">Consultando memoria 0x40-0x6F de la ECU...</div>';
    }

    function executeClearDtcs() {
      closeModal();
      bleCmdQueue.length = 0;
      document.getElementById('dtcList').innerHTML =
        '<div style="text-align:center; padding: 20px; color: var(--amber); font-weight: 700;">Enviando comando de reinicio (21 04 01 DA) a la ECU...</div>';
      sendBleCmd('CLEAR_DTC');
    }

    function handleClearDtcResponse(msg) {
      if (msg.ack) {
        if (msg.verifyOk && msg.remaining === 0 && !msg.mil) {
          alert('¡Códigos borrados y confirmados exitosamente por la ECU!');
        } else if (!msg.verifyOk) {
          alert('La ECU aceptó el borrado y se está reiniciando. Verificando memoria nuevamente...');
        } else {
          alert('La ECU aceptó el comando de borrado, pero la falla aún está físicamente presente (se volvió a detectar de inmediato). Revisa el sensor o circuito indicado.');
        }
      } else {
        alert('La ECU no confirmó el comando de borrado. IMPORTANTE: En el Civic 99 debes tener el MOTOR APAGADO y la llave en posición ON (II) para que la ECU permita borrar códigos.');
      }
      fetchDtcs();
    }
```

### 3. Mostrar `DTC 0x60` (`msg.dtc2`) y el Sensor O2 Secundario (`0x16`) en `handleJsonMessage()` (`app/index.html`)
Dentro del bloque `else if (msg.type === 'diag')`, asegurarse de incluir también la fila `msg.dtc2` debajo de `msg.dtc1`:

```javascript
                  ${msg.dtc0 ? `<div style="margin-top: 4px;"><strong style="color:#ffb300;">DTC 0x40:</strong> ${msg.dtc0}</div>` : ''}
                  ${msg.dtc1 ? `<div style="margin-top: 4px;"><strong style="color:#ffb300;">DTC 0x50:</strong> ${msg.dtc1}</div>` : ''}
                  ${msg.dtc2 ? `<div style="margin-top: 4px;"><strong style="color:#ffb300;">DTC 0x60:</strong> ${msg.dtc2}</div>` : ''}
```

### 4. Incrementar versión de caché en `app/sw.js`
Subir el identificador de caché para que el navegador móvil actualice la aplicación inmediatamente:

```javascript
const CACHE_NAME = 'civic-obd-v22';
```

---

## 5. FASE 4: Protocolo de Pruebas y Validación

1. **Prueba de Lectura y Prioridad BLE (`GET_DTC`):**
   * Entrar a la pestaña **DIAG** con el motor encendido o llave en `ON (II)`.
   * Presionar **Escanear Códigos de Falla** repetidas veces mientras `GET_DIAG` está corriendo en segundo plano y verificar que siempre responde en menos de 1 segundo sin errores GATT ni bloqueos.
   * Confirmar que en el bloque hexadecimal aparecen `DTC 0x40`, `DTC 0x50` y `DTC 0x60`.
2. **Prueba de Borrado de Códigos (`CLEAR_DTC`):**
   * Con el **motor apagado y llave en posición `ON (II)`**, presionar **Borrar Códigos (Reset ECU)**.
   * Confirmar que aparece el mensaje *"Enviando comando de reinicio (21 04 01 DA) a la ECU..."*, la ECU responde con `ack: true`, se reconecta automáticamente sin quedarse en `"ECU: BUSCANDO"` y muestra la lista limpia.
3. **Prueba Estacionaria de Temperatura ($85\text{ °C} - 91\text{ °C}$):**
   * Dejar el auto encendido en **Park (P)** monitoreando `ECT` y `Byte O2 S2 ADC (0x16)` hasta alcanzar los $90\text{ °C}$ (aprox. 9 a 11 minutos) para validar el estado del sensor secundario.