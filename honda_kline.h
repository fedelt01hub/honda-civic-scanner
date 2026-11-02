#ifndef HONDA_KLINE_H
#define HONDA_KLINE_H

#include <Arduino.h>
#include <vector>

// ============================================================================
// Tipo de ECU (afecta solamente la fórmula de RPM)
//   1 = OBD1 (P28, P2E, P06...)  -> rpm = 1875000 / (raw + 1)
//   2 = OBD2a/OBD2b (P2P, PCB...) -> rpm = raw / 4
// Si las RPM se ven absurdas en ralentí, cambia este valor.
// ============================================================================
#ifndef HONDA_ECU_TYPE
#define HONDA_ECU_TYPE 2  // 2 = OBD2 (Civic 96-00, RPM = raw / 4), 1 = OBD1 (Civic 92-95)
#endif

// Diagnostic state machine
enum KLineState {
  KLINE_IDLE,
  KLINE_WAKEUP_PULSE_LOW,
  KLINE_WAKEUP_PULSE_HIGH,
  KLINE_WAKEUP_MSG,
  KLINE_INIT_MSG,
  KLINE_READY,
  KLINE_POLLING
};

struct TelemetryData {
  // Principales
  int rpm = 0;
  int vss = 0;

  // SECCIÓN A: Motor y Estados Discretos
  bool vtec = false;          // 0x0A Bit 2/3: Solenoide y switch presión VTEC (VTS/VTPS)
  bool ac = false;            // 0x08 Bit 1/2: Relé y switch A/C
  bool brake = false;         // 0x08 Bit 3: Interruptor pedal de freno
  bool parkNeutral = false;   // 0x08 Bit 4/5: Selector P/N (transmisión auto ATP)
  bool mil = false;           // 0x0B Bit 5: Indicador de falla Check Engine (MIL) en ECU
  int gear = 0;               // 0=N/P, 1..4 marcha estimada por relación RPM/VSS

  // SECCIÓN B: Admisión, Escape y Eléctricos
  int ect = 0;                // 0x10: Temperatura refrigerante °C
  int iat = 0;                // 0x11: Temperatura aire admisión °C
  int map = 0;                // 0x12: Presión múltiple admisión en kPa
  float mapInHg = 0.0f;       // Vacío de múltiple en inHg
  int baro = 0;               // 0x13: Presión barométrica ambiental en kPa
  int tps = 0;                // 0x14: Posición mariposa acelerador %
  float o2 = 0.0f;            // 0x15: Sonda lambda primaria en Volts (0.0 a 1.0 V)
  float o2Sec = 0.0f;         // 0x16: Sonda lambda secundaria en Volts (0.0 a 1.25 V)
  float batt = 0.0f;          // 0x17: Voltaje de batería en V
  int altFr = 0;              // 0x18: Señal de carga del alternador %
  float eld = 0.0f;           // 0x19: Detector de Carga Eléctrica en Amperios (A)

  // SECCIÓN C: Inyección, Combustible y Encendido
  float ign = 0.0f;           // 0x26: Avance de encendido en °BTDC
  float inj = 0.0f;           // 0x24-0x25: Tiempo de inyección en milisegundos (ms)
  int sft = 0;                // 0x20: Short Term Fuel Trim % (-30% a +30%)
  int lft = 0;                // 0x22: Long Term Fuel Trim % (-30% a +30%)
  int iacv = 0;               // 0x28: Válvula de marcha mínima IACV %
  int knock = 0;              // 0x3C: Nivel de detonación / knock (0 a 5)
  float maf = 0.0f;           // Flujo de aire estimado en g/s

  bool connected = false;
  bool demo = false;
  int dtcCount = 0;
  unsigned long lastSuccessTime = 0;
};

struct FreezeFrame {
  bool valid = false;
  int ect = 0;          // °C (0x64)
  int iat = 0;          // °C (0x65)
  int map = 0;          // kPa (0x66)
  int baro = 0;         // kPa (0x67)
  int tps = 0;          // % (0x68)
  uint8_t rawEct = 0;
  uint8_t rawIat = 0;
  uint8_t rawMap = 0;
  uint8_t rawBaro = 0;
  uint8_t rawTps = 0;
};

struct DiagInfo {
  KLineState state = KLINE_IDLE;
  int initAttempts = 0;
  int initSuccess = 0;
  int initFail = 0;
  int echoOk = 0;
  int echoFail = 0;
  int cmdSent = 0;
  int cmdOk = 0;
  int cmdTimeout = 0;
  int cmdCrcFail = 0;
  int rxBytes = 0;          // bytes recibidos de la ECU (sin contar el eco)
  int echoTestSent = 0;     // última prueba de eco: bytes enviados
  int echoTestOk = 0;       // última prueba de eco: bytes que regresaron bien
  int echoTestRx = 0;       // última prueba de eco: bytes totales recibidos
  unsigned long lastAttemptTime = 0;
  char lastError[64] = "Esperando...";
  char lastRxHex[128] = "";
  char row0Hex[64] = "";
  char row1Hex[64] = "";
  char row2Hex[64] = "";
  char row3Hex[64] = "";
  char dtcRawHex[3][64] = {"", "", ""};
  FreezeFrame freezeFrame;
  uint8_t rawEldByte = 0;
  uint8_t rawO2SecByte = 0;
  uint8_t d8Byte = 0;
  uint8_t d10Byte = 0;
  uint8_t d11Byte = 0;
};

struct DtcCode {
  int code;
  const char* name;
  const char* desc;
};

class HondaKLine {
public:
  HondaKLine();
  void begin(int rxPin = 4, int txPin = 5);
  bool sendInit();
  void update();
  
  bool clearDTCs();
  bool getActiveDTCs(std::vector<DtcCode>& activeCodes);

  TelemetryData data;
  DiagInfo diag;
  int ecuType = HONDA_ECU_TYPE;
  void setDemoMode(bool enable);

  // Prueba de eco: envía bytes por TX y verifica que regresen por RX.
  // Funciona aunque la ECU no conteste (prueba el ESP32 + L9637D + línea).
  void echoTest(int n = 16);
  void resetCounters();

  // Log en memoria para verlo desde el dashboard (sin laptop)
  static const int LOG_LINES = 40;
  static const int LOG_LEN = 110;
  void klog(const char* fmt, ...);
  uint32_t logSeq() const { return _logSeq; }
  // Devuelve la línea #seq o nullptr si ya fue sobrescrita
  const char* logLine(uint32_t seq) const;
  const char* stateName() const;

private:
  char _log[LOG_LINES][LOG_LEN];
  uint32_t _logSeq = 0;
  int _rxPin;
  int _txPin;
  HardwareSerial* _serial;
  unsigned long _lastPollTime = 0;
  unsigned long _lastInitAttempt = 0;
  int _pollStep = 0;
  bool _initialized = false;
  
  // Honda 3-pin DLC (autos) - lectura de memoria de la ECU
  // Trama: 20 05 [ADDR] [LEN] [CS]  ->  Respuesta: 00 [LEN+3] [DATA x LEN] [CS]
  // Devuelve true si se recibieron LEN bytes con checksum válido.
  bool readMemory(uint8_t addr, uint8_t len, uint8_t* out);

  // Send a command frame and receive response
  // Returns number of response bytes, or 0 on failure
  int sendCommand(const uint8_t* cmd, int cmdLen, uint8_t* respBuf, int respBufSize, int timeoutMs = 250);
  
  // Honda checksum: (0x100 - sum) & 0xFF  (la suma de toda la trama da 0x00)
  uint8_t calcChecksum(const uint8_t* data, int len);
  
  // Thermistor conversion for Honda ECT & IAT
  int calculateHondaTemp(uint8_t raw);

  // Demo simulation generator
  void updateDemo();
  
  // Log hex bytes to diag.lastRxHex
  void logHex(const char* prefix, const uint8_t* buf, int len);
};

extern HondaKLine honda;

#endif // HONDA_KLINE_H
