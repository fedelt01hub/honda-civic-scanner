#ifndef TELEMETRY_TYPES_H
#define TELEMETRY_TYPES_H

// Sin dependencias de Arduino: también se compila en los tests nativos (pio test -e native).
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

#endif // TELEMETRY_TYPES_H
