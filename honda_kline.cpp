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

// Known Honda PGM-FI Diagnostic Trouble Codes (Civic 1996-2000)
static const DtcCode HONDA_DTC_TABLE[] = {
  { 1,  "P0131/P0132 - Sensor O2 Primario (S1)", "Voltaje bajo o alto en sensor de oxígeno primario (A/F)." },
  { 3,  "P0107/P0108 - Sensor MAP",              "Lectura de presión absoluta del múltiple fuera de rango eléctrico." },
  { 4,  "P0335/P0336 - Sensor CKP (Cigüeñal)",   "Pérdida de señal de posición del cigüeñal (Crankshaft Position)." },
  { 5,  "P0106/P1128 - Sensor MAP (Rango)",      "Falla mecánica o fuga de vacío en el colector de admisión." },
  { 6,  "P0117/P0118 - Sensor ECT (Refrigerante)", "Falla o circuito abierto en sensor de temperatura de refrigerante." },
  { 7,  "P0122/P0123 - Sensor TPS (Acelerador)", "Falla de potenciómetro o voltaje anormal en mariposa de aceleración." },
  { 8,  "P1359/P1361 - Sensor TDC (Punto Muerto)", "Falla en sensor de posición de cilindro / distribuidor." },
  { 9,  "P1381/P1382 - Sensor CYP (Cilindro 1)", "Falla en señal de identificación de cilindro 1 en distribuidor." },
  { 10, "P0111/P0112 - Sensor IAT (Aire Admisión)", "Falla en sensor de temperatura de aire de admisión." },
  { 12, "P1491/P1498 - Sistema EGR",             "Falla en la válvula o sensor de elevación de recirculación EGR." },
  { 13, "P1106/P1107 - Sensor BARO",             "Falla en el sensor de presión barométrica atmosférica." },
  { 14, "P0505/P1508 - Válvula IACV",            "Falla en circuito o atasco de la válvula de control de marcha mínima." },
  { 16, "Inyectores de Combustible",             "Circuito abierto o falla en el circuito de inyectores." },
  { 17, "P0500/P0501 - Sensor VSS (Velocímetro)","Pérdida de señal del sensor de velocidad del vehículo." },
  { 20, "P1297/P1298 - Sensor ELD (Carga Eléc.)","Falla en detector de carga eléctrica (fusiblera bajo capó)." },
  { 21, "P1259 - Solenoide VTEC",                "Falla en circuito o solenoide de accionamiento VTEC." },
  { 22, "P1259 - Switch Presión VTEC",           "Falla en interruptor de presión de aceite del VTEC." },
  { 23, "P0325 - Sensor Knock (Detonación)",     "Falla en el sensor de cascabeleo o circuito del sensor knock." },
  { 30, "P1655 - Comunicación A/T (TMA/TMB)",    "Falla en la línea de comunicación de transmisión automática." },
  { 41, "P0135/P1166 - Calefactor O2 Primario",  "Resistencia calefactora del sensor O2 primario dañada o quemada." },
  { 43, "Sistema de Combustible",                "Falla en suministro o control de mezcla de combustible." },
  { 45, "P0171/P0172 - Mezcla Pobre / Rica",     "Límite de compensación de combustible (Fuel Trim) superado." },
  { 48, "P1162/P1168 - Sensor O2 (Línea LABEL)", "Falla de circuito en sensor de oxígeno primario." },
  { 54, "P1336/P1337 - Sensor CKF (Fluctuación)","Falla en sensor de fluctuación de cigüeñal (detrás de polea)." },
  { 61, "P0133/P1163 - Sensor O2 (Lento)",       "Respuesta lenta en sensor de oxígeno primario." },
  { 63, "P0137/P0138 - Sensor O2 Secundario (S2)", "Voltaje bajo/alto o respuesta lenta en sensor O2 después del catalizador." },
  { 65, "P0141 - Calefactor O2 Secundario (S2)",  "Resistencia calefactora del sensor O2 secundario dañada." },
  { 67, "P0420 - Catalizador (Baja Eficiencia)", "Eficiencia del convertidor catalítico por debajo del umbral mínimo." },
  { 70, "P0700/P0300 - Transmisión A/T / Misfire","Falla en caja automática o fallo de encendido múltiple." },
  { 71, "P0301 - Misfire Cilindro 1",            "Fallo de encendido detectado en el cilindro 1." },
  { 72, "P0302 - Misfire Cilindro 2",            "Fallo de encendido detectado en el cilindro 2." },
  { 73, "P0303 - Misfire Cilindro 3",            "Fallo de encendido detectado en el cilindro 3." },
  { 74, "P0304 - Misfire Cilindro 4",            "Fallo de encendido detectado en el cilindro 4." },
  { 80, "P0401 - Sistema EGR (Flujo)",           "Flujo de recirculación de gases EGR insuficiente." },
  { 86, "P0116 - Sensor ECT (Rango)",            "Rango o rendimiento anormal en sensor de temperatura de refrigerante." },
  { 90, "P1456/P1457 - Fuga Sistema EVAP",       "Fuga detectada en tanque de combustible o cánister EVAP." },
  { 91, "P0451/P0452 - Presión Tanque Gasolina", "Falla en sensor de presión del tanque de combustible." },
  { 92, "P0441 - Purga Sistema EVAP",            "Flujo de purga insuficiente en el sistema EVAP." }
};

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
// Checksum: (0x100 - sum_of_all_bytes) & 0xFF
// ============================================================================
uint8_t HondaKLine::calcChecksum(const uint8_t* data, int len) {
  uint16_t sum = 0;
  for (int i = 0; i < len; i++) {
    sum += data[i];
  }
  return (uint8_t)((0x100 - (sum & 0xFF)) & 0xFF);
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
  
  klog("CONEXIÓN EXITOSA CON LA ECU");
  snprintf(diag.lastError, sizeof(diag.lastError), "Conectado OK (intento #%d)", diag.initAttempts);
  return true;
}

// ============================================================================
// Lee LEN bytes de la memoria de la ECU a partir de ADDR.
// ============================================================================
bool HondaKLine::readMemory(uint8_t addr, uint8_t len, uint8_t* out) {
  uint8_t req[5] = {0x20, 0x05, addr, len, 0};
  req[4] = calcChecksum(req, 4);

  uint8_t resp[40];
  int expected = len + 3;  // 00 + LEN+3 + DATA + CS
  if (expected > (int)sizeof(resp)) return false;

  diag.cmdSent++;
  int rxLen = sendCommand(req, 5, resp, expected, 200);

  if (rxLen < expected) {
    diag.cmdTimeout++;
    if (rxLen > 0) logHex("RX-PARCIAL", resp, rxLen);
    snprintf(diag.lastError, sizeof(diag.lastError), "Timeout addr %02X (%d/%d bytes)", addr, rxLen, expected);
    klog("ECU NO CONTESTA: addr %02X, %d/%d bytes", addr, rxLen, expected);
    return false;
  }

  // La suma de toda la trama (incluido el CS) debe ser 0x00
  if (calcChecksum(resp, expected - 1) != resp[expected - 1]) {
    diag.cmdCrcFail++;
    logHex("RX-CRC-MAL", resp, rxLen);
    snprintf(diag.lastError, sizeof(diag.lastError), "Checksum inválido addr %02X", addr);
    return false;
  }

  diag.cmdOk++;
  memcpy(out, resp + 2, len);
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
      // Bloque 0x00: RPM, VSS, VTEC, A/C, Freno, P/N
      if (readMemory(0x00, 0x10, d)) {
        data.lastSuccessTime = millis();
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

        // Estados discretos (0x08, 0x0A y 0x0B)
        // Dirección 0x08 (d[8]): Bit 1 = Relé A/C, Bit 2 = Switch A/C, Bit 3 = Switch Pedal Freno
        // Bit 4 y Bit 5 = Selector P/N (transmisión automática ATP)
        // Bit 7 = Switch presión VTEC (en variantes D16Y8 P2P OBD2)
        // Dirección 0x0A (d[10]): Bit 2 = Solenoide VTEC (VTS), Bit 3 = Switch Presión VTEC (VTPS)
        // Dirección 0x0B (d[11]): Bit 5 = Indicador de falla Check Engine (MIL) en ECU
        data.vtec = ((d[10] & (1 << 2)) != 0) || ((d[10] & (1 << 3)) != 0) || ((d[8] & (1 << 7)) != 0);
        data.brake = (d[8] & (1 << 3)) != 0;
        data.ac = ((d[8] & (1 << 2)) != 0) || ((d[8] & (1 << 1)) != 0);
        data.parkNeutral = ((d[8] & (1 << 4)) != 0) || ((d[8] & (1 << 5)) != 0);
        data.mil = (d[11] & (1 << 5)) != 0;

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
        data.mapInHg = (101.325f - (float)data.map) * 0.2953f;
        if (data.mapInHg < 0.0f) data.mapInHg = 0.0f;

        data.baro = (int)roundf(d[3] * 0.716f - 5.0f);
        data.tps  = constrain(((int)d[4] - 24) / 2, 0, 100);
        data.o2   = constrain(d[5] / 51.3f, 0.0f, 1.25f);
        data.batt = d[7] / 10.45f;
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
// Read DTC codes
// Memoria 0x40-0x5F: cada byte contiene 2 códigos (nibble alto = código par,
// nibble bajo = código impar). Basado en kerpz/ArduinoHondaOBD.
// ============================================================================
static void pushDtc(std::vector<DtcCode>& v, int codeNum) {
  for (size_t t = 0; t < sizeof(HONDA_DTC_TABLE)/sizeof(HONDA_DTC_TABLE[0]); t++) {
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
    // Return sample codes in demo mode
    activeCodes.push_back(HONDA_DTC_TABLE[0]); // Code 1 (O2)
    activeCodes.push_back(HONDA_DTC_TABLE[9]); // Code 14 (IACV)
    data.dtcCount = (int)activeCodes.size();
    return true;
  }

  uint8_t d[16];
  for (int row = 0; row < 2; row++) {
    uint8_t addr = 0x40 + row * 0x10;
    bool ok = false;
    for (int attempt = 0; attempt < 3 && !ok; attempt++) {
      ok = readMemory(addr, 0x10, d);
      if (!ok) delay(25);
    }
    if (!ok) {
      klog("ERROR: Fallo al leer memoria DTC en 0x%02X tras 3 intentos", addr);
      return false; // NUNCA reportar "sin códigos" si la comunicación falló
    }

    snprintf(diag.dtcRawHex[row], sizeof(diag.dtcRawHex[row]),
             "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7],
             d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15]);
    logHex(row == 0 ? "DTC40" : "DTC50", d, 16);

    for (int i = 0; i < 16; i++) {
      int base = row * 32 + i * 2;
      if (d[i] >> 4) {
        if (base > 0) pushDtc(activeCodes, base);
        else klog("DTC byte 0 hi-nibble set (código interno ECU/ignorando)");
      }
      if (d[i] & 0x0F) {
        int code = base + 1;
        // Ajuste observado por kerpz en la primera fila
        if (row == 0 && code == 23) code = 22;
        else if (row == 0 && code == 24) code = 23;
        pushDtc(activeCodes, code);
      }
    }
  }

  // Sondeo de diagnóstico experimental en fila 0x60 (para detectar códigos > 63)
  if (readMemory(0x60, 0x10, d)) {
    logHex("DTC60", d, 16);
    bool hasNonZero = false;
    for (int i = 0; i < 16; i++) {
      if (d[i] != 0) hasNonZero = true;
    }
    if (hasNonZero) {
      klog("AVISO: Detectada actividad en 0x60 (posibles códigos > 63 o A/T)");
    }
  }

  data.dtcCount = (int)activeCodes.size();
  return true;
}

// ============================================================================
// Clear DTC codes (reset ECU): 21 04 01 DA  ->  01 03 FC
// ============================================================================
bool HondaKLine::clearDTCs() {
  if (data.demo) {
    data.dtcCount = 0;
    data.mil = false;
    return true;
  }

  uint8_t clearReq[] = {0x21, 0x04, 0x01, 0xDA};
  uint8_t resp[8];
  
  int rxLen = sendCommand(clearReq, 4, resp, 3, 400);
  logHex("CLEAR", resp, rxLen);
  
  bool ack = (rxLen == 3 && resp[0] == 0x01 && resp[1] == 0x03 && resp[2] == 0xFC);
  if (ack) {
    klog("ECU ACK: Códigos borrados exitosamente (01 03 FC)");
    data.dtcCount = 0;
    data.mil = false;
    _initialized = false;
    data.connected = false; // La ECU reinicia la interfaz de comunicación tras el reset
  } else {
    klog("ECU NACK / Respuesta inesperada al borrar (%d bytes)", rxLen);
  }
  return ack;
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
