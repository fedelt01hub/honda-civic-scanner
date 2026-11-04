#include "honda_kline.h"
#include <math.h>
#include <stdarg.h>

HondaKLine honda;

// ============================================================================
// Honda 3-Pin DLC Protocol (AUTOS Honda pre-2002, ej. Civic 1996-2000)
// ============================================================================
// IMPORTANTE: El protocolo "Keihin 72 05 71 ..." a 10400 baud (repo
// AutotronicCommunity) es de MOTOCICLETAS Honda (ECU K25, Beat, Vario...).
// La ECU de un Civic '99 con conector de 3 pines NO responde a ese protocolo.
//
// Protocolo correcto (referencia: github.com/kerpz/ArduinoHondaOBD):
//   Velocidad: 9600 baud, 8N1, half-duplex, una sola línea (K-Line)
//   Sin pulso de wakeup. La ECU responde directamente.
//   Petición : 20 05 [ADDR] [LEN] [CS]      (leer LEN bytes desde ADDR)
//   Respuesta: 00 [LEN+3] [DATA x LEN] [CS]
//   Borrar   : 21 04 01 DA  -> respuesta 01 03 FC
//   Checksum : (0x100 - suma) & 0xFF  (toda la trama suma 0x00)
//
// Mapa de memoria (RAM de la ECU):
//   0x00-0x01 RPM (raw 16 bits)   0x02 VSS (km/h)
//   0x10 ECT  0x11 IAT  0x12 MAP  0x13 BARO  0x14 TPS  0x15 O2  0x17 BATT
//   0x20 STFT 0x21 LTFT ... 0x40-0x5F códigos de falla (1 nibble por código)
// ============================================================================

HondaKLine::HondaKLine() {
  _serial = &Serial1;
  memset(_log, 0, sizeof(_log));
}

// ============================================================================
// Log: imprime por USB y guarda en un buffer circular para el dashboard
// ============================================================================
void HondaKLine::klog(const char* fmt, ...) {
  char* line = _log[_logSeq % LOG_LINES];
  unsigned long ms = millis();
  int pos = snprintf(line, LOG_LEN, "[%lu.%01lu] ", ms / 1000, (ms % 1000) / 100);
  va_list args;
  va_start(args, fmt);
  vsnprintf(line + pos, LOG_LEN - pos, fmt, args);
  va_end(args);
  Serial.println(line);
  _logSeq++;
}

const char* HondaKLine::logLine(uint32_t seq) const {
  if (seq >= _logSeq) return nullptr;
  if (_logSeq - seq > (uint32_t)LOG_LINES) return nullptr;
  return _log[seq % LOG_LINES];
}

const char* HondaKLine::stateName() const {
  switch (diag.state) {
    case KLINE_IDLE:     return "IDLE";
    case KLINE_INIT_MSG: return "CONECTANDO";
    case KLINE_READY:    return "LISTO";
    case KLINE_POLLING:  return "LEYENDO";
    default:             return "OTRO";
  }
}

void HondaKLine::resetCounters() {
  KLineState s = diag.state;
  diag = DiagInfo();
  diag.state = s;
  klog("Contadores reiniciados");
}

// ============================================================================
// Prueba de eco: envía un patrón por TX y cuenta cuántos bytes regresan por
// RX. Con el L9637D cada byte transmitido aparece en RX si:
//   ESP32 TX -> L9637D pin 4 -> pin 6 (K) sube/baja -> pin 1 RX -> ESP32
// ============================================================================
void HondaKLine::echoTest(int n) {
  if (n > 32) n = 32;
  uint8_t tx[32], rx[64];
  for (int i = 0; i < n; i++) tx[i] = (i & 1) ? 0xAA : 0x55;

  while (_serial->available()) _serial->read();
  _serial->write(tx, n);
  _serial->flush();

  int rxCount = 0;
  unsigned long deadline = millis() + 60;
  while (millis() < deadline && rxCount < (int)sizeof(rx)) {
    if (_serial->available()) rx[rxCount++] = _serial->read();
  }

  int ok = 0;
  for (int i = 0; i < n && i < rxCount; i++) if (rx[i] == tx[i]) ok++;

  diag.echoTestSent = n;
  diag.echoTestOk = ok;
  diag.echoTestRx = rxCount;

  klog("PRUEBA ECO: enviados %d, recibidos %d, correctos %d", n, rxCount, ok);
  if (rxCount > 0) logHex("ECO RX", rx, rxCount > 24 ? 24 : rxCount);
  if (rxCount == 0)       klog("  -> NADA regresa: revisa TX(pin4), RX(pin1), VS(pin7) y VCC(pin3)");
  else if (ok == n)       klog("  -> ECO PERFECTO: ESP32 y L9637D funcionan");
  else                    klog("  -> Eco corrupto: ruido, pull-up o VCC del L9637D fuera de rango");
}

void HondaKLine::begin(int rxPin, int txPin) {
  _rxPin = rxPin;
  _txPin = txPin;

  // UART1 del ESP32-C3 a 9600 baud, 8N1 (protocolo Honda 3-pin de autos).
  // NOTA: no llamar pinMode()/digitalWrite() sobre el TX después de begin():
  // en el core 3.x del ESP32 eso desconecta el pin del periférico UART.
  _serial->begin(9600, SERIAL_8N1, _rxPin, _txPin);
  _serial->setTimeout(100);

  data.connected = false;
  data.demo = false;
  _initialized = false;
  
  diag.state = KLINE_IDLE;
  snprintf(diag.lastError, sizeof(diag.lastError), "Esperando primera inicialización...");
}

// ============================================================================
// Log hex bytes to diag struct for debug display
// ============================================================================
void HondaKLine::logHex(const char* prefix, const uint8_t* buf, int len) {
  char tmp[128];
  int pos = snprintf(tmp, sizeof(tmp), "%s: ", prefix);
  for (int i = 0; i < len && pos < 120; i++) {
    pos += snprintf(tmp + pos, sizeof(tmp) - pos, "%02X ", buf[i]);
  }
  strncpy(diag.lastRxHex, tmp, sizeof(diag.lastRxHex) - 1);
  diag.lastRxHex[sizeof(diag.lastRxHex) - 1] = '\0';
  klog("%s", tmp);
}

// ============================================================================
// Inicialización: no hay wakeup. Simplemente se intenta leer el bloque 0x00;
// si la ECU contesta con checksum válido, hay comunicación.
// ============================================================================
bool HondaKLine::sendInit() {
  if (data.demo) return true;
  
  diag.initAttempts++;
  diag.lastAttemptTime = millis();
  diag.state = KLINE_INIT_MSG;
  
  klog("=== Intento de conexión #%d (9600 baud) ===", diag.initAttempts);
  
  uint8_t buf[16];
  if (!readMemory(0x00, 0x10, buf)) {
    diag.initFail++;
    diag.state = KLINE_IDLE;
    klog("La ECU no respondió a 20 05 00 10 CB");
    return false;
  }
  
  diag.initSuccess++;
  diag.state = KLINE_READY;
  _initialized = true;
  data.connected = true;
  data.lastSuccessTime = millis();

  // Estado real (RPM, VSS, MIL...) desde el bloque 0x00 recién leído
  applyStatusBlock(buf);
  
  klog("CONEXIÓN EXITOSA CON LA ECU (MIL=%d)", data.mil ? 1 : 0);
  snprintf(diag.lastError, sizeof(diag.lastError), "Conectado OK (intento #%d)", diag.initAttempts);
  return true;
}

// ============================================================================
// Lee LEN bytes de la memoria de la ECU a partir de ADDR.
// ============================================================================
bool HondaKLine::readMemory(uint8_t addr, uint8_t len, uint8_t* out) {
  uint8_t req[5];
  buildReadRequest(addr, len, req);

  uint8_t resp[40];
  int expected = len + 3;  // 00 + LEN+3 + DATA + CS
  if (expected > (int)sizeof(resp)) return false;

  diag.cmdSent++;
  int rxLen = sendCommand(req, 5, resp, expected, 200);

  switch (checkReadResponse(resp, rxLen, len)) {
    case READ_SHORT:
      diag.cmdTimeout++;
      if (rxLen > 0) logHex("RX-PARCIAL", resp, rxLen);
      snprintf(diag.lastError, sizeof(diag.lastError), "Timeout addr %02X (%d/%d bytes)", addr, rxLen, expected);
      klog("ECU NO CONTESTA: addr %02X, %d/%d bytes", addr, rxLen, expected);
      return false;
    case READ_BAD_HEADER:
      diag.cmdCrcFail++;
      logHex("RX-CABECERA-MAL", resp, rxLen);
      snprintf(diag.lastError, sizeof(diag.lastError), "Cabecera inválida addr %02X", addr);
      return false;
    case READ_BAD_CHECKSUM:
      diag.cmdCrcFail++;
      logHex("RX-CRC-MAL", resp, rxLen);
      snprintf(diag.lastError, sizeof(diag.lastError), "Checksum inválido addr %02X", addr);
      return false;
    case READ_OK:
      break;
  }

  diag.cmdOk++;
  data.lastSuccessTime = millis();
  memcpy(out, resp + 2, len);
  return true;
}

// ============================================================================
// Bloque 0x00: RPM, VSS, estados discretos y MIL (usado por update, sendInit y DTC)
// ============================================================================
void HondaKLine::applyStatusBlock(const uint8_t* d) {
  uint16_t raw = (d[0] << 8) | d[1];
  if (ecuType == 1) {
    data.rpm = (raw == 0xFFFF) ? 0 : (int)(1875000UL / ((uint32_t)raw + 1));
  } else {
    data.rpm = raw / 4;
  }
  if (data.rpm < 0 || data.rpm > 9999) data.rpm = 0;
  data.vss = d[2];

  // Guardar representación hex cruda de la fila 0x00 para diagnóstico
  snprintf(diag.row0Hex, sizeof(diag.row0Hex),
           "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
           d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);

  diag.d8Byte = d[8];
  diag.d10Byte = d[10];
  diag.d11Byte = d[11];

  // Estados discretos (0x08, 0x0A y 0x0B)
  // Dirección 0x08 (d[8]): Bit 1 = Relé A/C, Bit 2 = Switch A/C, Bit 3 = Switch Pedal Freno
  // Bit 4 y Bit 5 = Selector P/N (transmisión automática ATP)
  // Bit 7 = Switch presión VTEC (en variantes D16Y8 P2P OBD2)
  // Dirección 0x0A (d[10]): Bit 2 = Solenoide VTEC (VTS), Bit 3 = Switch Presión VTEC (VTPS)
  // Dirección 0x0B (d[11]): Bit 5 / Bit 6 = Indicador de falla Check Engine (MIL) en ECU
  data.vtec = ((d[10] & (1 << 2)) != 0) || ((d[10] & (1 << 3)) != 0) || ((d[8] & (1 << 7)) != 0);
  data.brake = (d[8] & (1 << 3)) != 0;
  data.ac = ((d[8] & (1 << 2)) != 0) || ((d[8] & (1 << 1)) != 0);
  data.parkNeutral = ((d[8] & (1 << 4)) != 0) || ((d[8] & (1 << 5)) != 0);
  // En ECU P2P (D16Y8), 0x0B reporta 0x41 con Check Engine activo (Bit 6 / Bit 5)
  data.mil = ((d[11] & (1 << 5)) != 0) || ((d[11] & (1 << 6)) != 0);

  // Marcha estimada (Civic 4-velocidades automático)
  if (data.vss > 5 && data.rpm > 600) {
    float ratio = (float)data.rpm / (float)data.vss;
    if (ratio > 75.0f) data.gear = 1;
    else if (ratio > 48.0f) data.gear = 2;
    else if (ratio > 33.0f) data.gear = 3;
    else data.gear = 4;
  } else {
    // En reposo: N si la palanca está en P/N, o 1 si está engranado en D o R
    data.gear = data.parkNeutral ? 0 : 1;
  }
}

bool HondaKLine::readMemoryRetry(uint8_t addr, uint8_t len, uint8_t* out, int attempts) {
  for (int i = 0; i < attempts; i++) {
    if (readMemory(addr, len, out)) return true;
    delay(25);
  }
  return false;
}

bool HondaKLine::readStatus() {
  uint8_t d[16];
  if (!readMemoryRetry(0x00, 0x10, d, 3)) return false;
  applyStatusBlock(d);
  return true;
}

// ============================================================================
// Send command and receive response (handles echo stripping)
// Returns: number of response bytes received, or 0 on failure
// ============================================================================
int HondaKLine::sendCommand(const uint8_t* cmd, int cmdLen, uint8_t* respBuf, int respBufSize, int timeoutMs) {
  // Flush RX
  while (_serial->available()) _serial->read();
  
  // Send command bytes
  _serial->write(cmd, cmdLen);
  _serial->flush();
  
  int rxCount = 0;

  // Consumir eco (el L9637D devuelve por RX cada byte transmitido).
  // Si el primer byte NO coincide con el eco esperado, asumimos que el
  // hardware no hace eco y lo tratamos como inicio de la respuesta.
  unsigned long echoDeadline = millis() + 50;
  int echoCount = 0;
  while (echoCount < cmdLen && millis() < echoDeadline) {
    if (_serial->available()) {
      uint8_t b = _serial->read();
      if (b == cmd[echoCount]) {
        echoCount++;
        diag.echoOk++;
      } else {
        diag.echoFail++;
        klog("ECO: esperaba %02X, llegó %02X (¿sin eco?)", cmd[echoCount], b);
        if (rxCount < respBufSize) respBuf[rxCount++] = b;
        break;
      }
    }
  }
  if (echoCount == 0 && rxCount == 0) {
    // Ni eco: el TX no está llegando a la línea o el RX no ve la línea.
    klog("SIN ECO: lo transmitido no regresa por RX (hardware)");
  } else if (echoCount > 0 && echoCount < cmdLen) {
    klog("ECO INCOMPLETO: %d/%d bytes", echoCount, cmdLen);
  }
  
  // Receive response
  unsigned long respDeadline = millis() + timeoutMs;
  while (rxCount < respBufSize && millis() < respDeadline) {
    if (_serial->available()) {
      respBuf[rxCount++] = _serial->read();
      // Byte índice 1 = longitud total de la trama de respuesta
      if (rxCount >= 2 && rxCount >= respBuf[1]) break;
    }
  }
  diag.rxBytes += rxCount;
  
  return rxCount;
}

// ============================================================================
// Thermistor lookup (Honda NTC polynomial)
// ============================================================================
int HondaKLine::calculateHondaTemp(uint8_t raw) {
  // Simple linear approximation for Honda NTC thermistors
  // raw 0 = very hot (~155°C), raw 255 = very cold (~-40°C)
  float f = (float)raw;
  float c = 155.04149f - (f * 3.0414878f) 
            + (powf(f, 2.0f) * 0.03952185f) 
            - (powf(f, 3.0f) * 0.00029383913f) 
            + (powf(f, 4.0f) * 0.0000010792568f) 
            - (powf(f, 5.0f) * 0.0000000015618437f);
  int temp = (int)roundf(c);
  if (temp < -40) temp = -40;
  if (temp > 150) temp = 150;
  return temp;
}

// ============================================================================
// Main update loop - polls ECU for data
// ============================================================================
void HondaKLine::update() {
  if (data.demo) {
    updateDemo();
    return;
  }

  unsigned long now = millis();

  // If connection has been silent for more than 3 seconds, mark as disconnected
  if (data.connected && (now - data.lastSuccessTime > 3000)) {
    data.connected = false;
    data.rpm = 0;
    data.vss = 0;
    _initialized = false;
    diag.state = KLINE_IDLE;
    klog("Timeout de comunicación - desconectado.");
    snprintf(diag.lastError, sizeof(diag.lastError), "Timeout de comunicación");
  }

  // If not connected, attempt init every 2 seconds
  if (!data.connected) {
    if (now - _lastInitAttempt >= 2000) {
      _lastInitAttempt = now;
      sendInit();
    }
    return;
  }

  // Poll cycle interval (30ms per row gives ~8 full updates/sec)
  if (now - _lastPollTime < 30) {
    return;
  }
  _lastPollTime = now;
  diag.state = KLINE_POLLING;

  uint8_t d[16];
  
  switch (_pollStep) {
    case 0: {
      // Bloque 0x00: RPM, VSS, VTEC, A/C, Freno, P/N, MIL
      if (readMemory(0x00, 0x10, d)) {
        applyStatusBlock(d);
      }
      _pollStep = 1;
      break;
    }
    
    case 1: {
      // Bloque 0x10: ECT, IAT, MAP, BARO, TPS, O2, BATT, ALT_FR, ELD
      if (readMemory(0x10, 0x10, d)) {
        data.lastSuccessTime = millis();
        // Guardar representación hex cruda de la fila 0x10 para diagnóstico
        snprintf(diag.row1Hex, sizeof(diag.row1Hex),
                 "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                 d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
                 d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);

        data.ect  = calculateHondaTemp(d[0]);
        data.iat  = calculateHondaTemp(d[1]);
        data.map  = (int)roundf(d[2] * 0.716f - 5.0f);
        if (data.map < 0) data.map = 0;

        data.baro = (int)roundf(d[3] * 0.716f - 5.0f);
        float baroRef = (data.baro > 50) ? (float)data.baro : 101.325f;
        data.mapInHg = (baroRef - (float)data.map) * 0.2953f;
        if (data.mapInHg < 0.0f) data.mapInHg = 0.0f;
        data.tps   = constrain(((int)d[4] - 24) / 2, 0, 100);
        data.o2    = constrain(d[5] / 51.3f, 0.0f, 1.25f);
        data.o2Sec = constrain(d[6] / 51.3f, 0.0f, 1.25f);
        diag.rawO2SecByte = d[6];
        data.batt  = d[7] / 10.45f;
        data.altFr = constrain((int)roundf(d[8] / 2.55f), 0, 100);
        
        // Calibración ELD (Detector de Carga Eléctrica Honda):
        diag.rawEldByte = d[9];
        float rawEld = (float)d[9];
        if (rawEld < 252.0f) {
          data.eld = constrain((245.0f - rawEld) * 0.35f, 0.0f, 100.0f);
        } else {
          data.eld = 0.0f;
        }

        // Flujo másico de aire estimado (Speed-Density MAF en g/s)
        float imap = (float)data.rpm * (float)data.map / ((float)data.iat + 273.15f) / 2.0f;
        data.maf = (imap / 60.0f) * 0.80f * 1.595f * 28.9644f / 8.314472f;
        if (data.maf < 0.0f) data.maf = 0.0f;
      }
      _pollStep = 2;
      break;
    }

    case 2: {
      // Bloque 0x20: STFT, LTFT, INJ, IGN, IACV
      if (readMemory(0x20, 0x10, d)) {
        data.lastSuccessTime = millis();
        snprintf(diag.row2Hex, sizeof(diag.row2Hex),
                 "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                 d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
                 d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);

        data.sft = constrain((int)roundf(((float)d[0] / 128.0f - 1.0f) * 100.0f), -40, 40);
        data.lft = constrain((int)roundf(((float)d[2] / 128.0f - 1.0f) * 100.0f), -40, 40);
        data.inj = ((float)((d[4] << 8) | d[5])) / 250.0f;
        if (data.inj > 30.0f) data.inj = 0.0f;
        data.ign = ((float)d[6] - 24.0f) / 4.0f;
        data.iacv = constrain((int)roundf((float)d[8] / 2.55f), 0, 100);
      }
      _pollStep = 3;
      break;
    }

    case 3: {
      // Bloque 0x30: Knock sensor
      if (readMemory(0x30, 0x10, d)) {
        data.lastSuccessTime = millis();
        snprintf(diag.row3Hex, sizeof(diag.row3Hex),
                 "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                 d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
                 d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);

        data.knock = d[12] / 51;
        if (data.knock > 5) data.knock = 0;
      }
      _pollStep = 0;
      break;
    }
    
    default:
      _pollStep = 0;
      break;
  }
}

// ============================================================================
// Leer códigos DTC (decodificación en honda_dtc.h, probada en test/test_dtc)
// ============================================================================
bool HondaKLine::getActiveDTCs(std::vector<DtcCode>& activeCodes) {
  activeCodes.clear();
  diag.freezeFrame = FreezeFrame();   // nunca mostrar el Freeze Frame de una lectura anterior
  diag.dtcRawHex[2][0] = '\0';

  if (data.demo) {
    activeCodes.push_back(lookupDtc(1));   // O2
    activeCodes.push_back(lookupDtc(14));  // IACV
    data.dtcCount = (int)activeCodes.size();
    diag.freezeFrame.valid    = true;
    diag.freezeFrame.rawVss   = 0x00;
    diag.freezeFrame.rawRpmHi = 0x0B;
    diag.freezeFrame.rawRpmLo = 0x00;
    diag.freezeFrame.rawEct   = 0x1D;
    diag.freezeFrame.rawIat   = 0x57;
    diag.freezeFrame.rawMap   = 0x6A;
    diag.freezeFrame.rawBaro  = 0x8D;
    diag.freezeFrame.rawTps   = 0x18;
    diag.freezeFrame.vss  = 0;
    diag.freezeFrame.rpm  = 704;
    diag.freezeFrame.ect  = 94;
    diag.freezeFrame.iat  = 53;
    diag.freezeFrame.map  = 71;
    diag.freezeFrame.baro = 96;
    diag.freezeFrame.tps  = 0;
    return true;
  }

  // Confirma que la ECU contesta y refresca el MIL antes de leer la memoria de fallas
  if (!readStatus()) {
    snprintf(diag.lastError, sizeof(diag.lastError), "ECU sin respuesta: pon la llave en ON (II)");
    klog("DTC: la ECU no respondió al bloque 0x00");
    return false;
  }

  int codes[DTC_MAX_CODES];
  int n = 0;
  uint8_t d[16];

  for (int row = 0; row < 2; row++) {
    uint8_t addr = 0x40 + row * 0x10;
    if (!readMemoryRetry(addr, 0x10, d, 3)) {
      klog("ERROR: Fallo al leer memoria DTC en 0x%02X tras 3 intentos", addr);
      return false; // NUNCA reportar "sin códigos" si la comunicación falló
    }
    snprintf(diag.dtcRawHex[row], sizeof(diag.dtcRawHex[row]),
             "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
             d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
    logHex(row == 0 ? "DTC40" : "DTC50", d, 16);
    n = decodeDtcBytes(d, 16, row * 32, codes, n, DTC_MAX_CODES);
  }

  // Fila 0x60 opcional: código 65 y Freeze Frame (0x61-0x68)
  if (readMemoryRetry(0x60, 0x10, d, 2)) {
    snprintf(diag.dtcRawHex[2], sizeof(diag.dtcRawHex[2]),
             "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
             d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
    logHex("DTC60", d, 16);
    n = decodeDtcRow60(d, codes, n, DTC_MAX_CODES);

    // Decodificar Freeze Frame si hay datos válidos grabados (0x61 a 0x68)
    uint8_t rawVss   = d[1]; // 0x61
    uint8_t rawRpmHi = d[2]; // 0x62
    uint8_t rawRpmLo = d[3]; // 0x63
    uint8_t rawEct   = d[4]; // 0x64
    uint8_t rawIat   = d[5]; // 0x65
    uint8_t rawMap   = d[6]; // 0x66
    uint8_t rawBaro  = d[7]; // 0x67
    uint8_t rawTps   = d[8]; // 0x68

    if (rawEct != 0x00 && rawEct != 0xFF && rawMap != 0x00 && rawMap != 0xFF) {
      diag.freezeFrame.valid    = true;
      diag.freezeFrame.rawVss   = rawVss;
      diag.freezeFrame.rawRpmHi = rawRpmHi;
      diag.freezeFrame.rawRpmLo = rawRpmLo;
      diag.freezeFrame.rawEct   = rawEct;
      diag.freezeFrame.rawIat   = rawIat;
      diag.freezeFrame.rawMap   = rawMap;
      diag.freezeFrame.rawBaro  = rawBaro;
      diag.freezeFrame.rawTps   = rawTps;

      uint16_t rawFfRpm = ((uint16_t)rawRpmHi << 8) | rawRpmLo;
      diag.freezeFrame.rpm  = (ecuType == 1)
        ? ((rawFfRpm == 0xFFFF || rawFfRpm == 0) ? 0 : (int)(1875000UL / ((uint32_t)rawFfRpm + 1)))
        : (rawFfRpm / 4);
      diag.freezeFrame.vss  = rawVss;
      diag.freezeFrame.ect  = calculateHondaTemp(rawEct);
      diag.freezeFrame.iat  = calculateHondaTemp(rawIat);
      diag.freezeFrame.map  = (int)roundf(rawMap * 0.716f - 5.0f);
      if (diag.freezeFrame.map < 0) diag.freezeFrame.map = 0;
      diag.freezeFrame.baro = (int)roundf(rawBaro * 0.716f - 5.0f);
      diag.freezeFrame.tps  = constrain(((int)rawTps - 24) / 2, 0, 100);

      klog("FreezeFrame ECU: RPM=%d VSS=%dkm/h ECT=%d°C IAT=%d°C MAP=%dkPa BARO=%dkPa TPS=%d%%",
           diag.freezeFrame.rpm, diag.freezeFrame.vss,
           diag.freezeFrame.ect, diag.freezeFrame.iat, diag.freezeFrame.map,
           diag.freezeFrame.baro, diag.freezeFrame.tps);
    }
  }

  for (int i = 0; i < n; i++) activeCodes.push_back(lookupDtc(codes[i]));
  data.dtcCount = n;
  klog("DTC: %d código(s) activos, MIL=%d", n, data.mil ? 1 : 0);
  return true;
}

// ============================================================================
// Reset de ECU / borrar códigos: 21 04 01 DA  ->  01 03 FC
// Si no llega nada, se reintenta una vez (el reset es idempotente).
// ============================================================================
bool HondaKLine::clearDTCs() {
  diag.freezeFrame = FreezeFrame();
  if (data.demo) {
    data.dtcCount = 0;
    data.mil = false;
    return true;
  }

  uint8_t resp[8];
  int rxLen = 0;
  for (int attempt = 0; attempt < 2 && rxLen == 0; attempt++) {
    rxLen = sendCommand(CLEAR_DTC_REQ, 4, resp, 3, 400);
  }
  if (rxLen > 0) logHex("CLEAR", resp, rxLen);
  else klog("CLEAR: la ECU no respondió al reset");

  bool ack = isClearAck(resp, rxLen);
  if (ack) klog("ECU ACK: reset aceptado (01 03 FC)");
  else klog("ECU sin ACK al borrar (%d bytes); se verifica releyendo la memoria", rxLen);

  data.dtcCount = 0;
  _initialized = false;
  data.connected = false; // la ECU reinicia su interfaz tras el reset
  return ack;
}

void HondaKLine::clearAndVerify(ClearDtcResult& r) {
  r = ClearDtcResult();

  if (data.demo) {
    clearDTCs();
    r.ack = true; r.verifyOk = true; r.remaining = 0; r.mil = false;
    return;
  }

  if (!readStatus()) {
    r.error = "no_ecu";
    snprintf(diag.lastError, sizeof(diag.lastError), "ECU sin respuesta: pon la llave en ON (II)");
    klog("BORRAR cancelado: la ECU no responde");
    return;
  }
  if (!clearDtcAllowed(data.rpm, data.vss)) {
    r.error = "engine_running";
    klog("BORRAR rechazado: motor encendido o en movimiento (RPM=%d VSS=%d)", data.rpm, data.vss);
    return;
  }

  r.ack = clearDTCs();

  // La ECU tarda en volver tras el reset: reintentar hasta 6 x 500 ms
  bool reinitOk = false;
  for (int i = 0; i < 6 && !reinitOk; i++) {
    delay(500);
    reinitOk = sendInit();
  }

  std::vector<DtcCode> remaining;
  r.verifyOk = reinitOk && getActiveDTCs(remaining);
  r.remaining = r.verifyOk ? (int)remaining.size() : -1;
  r.mil = data.mil;
  klog("BORRAR: ack=%d verificado=%d restantes=%d MIL=%d",
       r.ack ? 1 : 0, r.verifyOk ? 1 : 0, r.remaining, r.mil ? 1 : 0);
}

void HondaKLine::setDemoMode(bool enable) {
  data.demo = enable;
  data.connected = enable;
  if (!enable) {
    _initialized = false;
    diag.state = KLINE_IDLE;
  }
}

void HondaKLine::updateDemo() {
  static float angle = 0.0f;
  angle += 0.08f;
  if (angle > 6.283f) angle = 0.0f;

  // Sine-wave simulated engine values
  float s = (sinf(angle) + 1.0f) * 0.5f; // 0.0 to 1.0
  data.connected = true;
  data.rpm = (int)(800 + s * 5200);
  data.vss = (int)(s * 110);
  data.tps = (int)(s * 85);
  data.map = (int)(32 + s * 66);
  data.mapInHg = (101.325f - (float)data.map) * 0.2953f;
  if (data.mapInHg < 0.0f) data.mapInHg = 0.0f;
  data.baro = 101;
  data.ect = (int)(80 + s * 12);
  data.iat = 32;
  data.batt = 14.2f;

  data.vtec = (data.rpm >= 4800);
  data.ac = false;
  data.brake = (s < 0.1f);
  data.parkNeutral = false;
  data.mil = false;
  if (data.vss > 5) {
    if (data.vss < 30) data.gear = 1;
    else if (data.vss < 60) data.gear = 2;
    else if (data.vss < 90) data.gear = 3;
    else data.gear = 4;
  } else data.gear = 1;

  data.o2 = 0.1f + s * 0.8f;
  data.o2Sec = 0.65f;
  data.altFr = (int)(30 + s * 40);
  data.eld = 18.5f + s * 22.0f;
  data.ign = 14.0f + s * 18.0f;
  data.inj = 2.4f + s * 9.5f;
  data.sft = (int)((s - 0.5f) * 8.0f);
  data.lft = 2;
  data.iacv = (int)(15 + (1.0f - s) * 20);
  data.knock = 0;

  float imap = (float)data.rpm * (float)data.map / ((float)data.iat + 273.15f) / 2.0f;
  data.maf = (imap / 60.0f) * 0.80f * 1.595f * 28.9644f / 8.314472f;
}
