#ifndef HONDA_DTC_H
#define HONDA_DTC_H

// Códigos de falla Honda PGM-FI (sin Arduino: se prueba con pio test -e native).
// Memoria de la ECU: 0x40-0x5F = 1 nibble por código
//   byte 0x40+i -> nibble alto = código 2i, nibble bajo = código 2i+1
// 0x60: solo el byte 0x60 tiene códigos (64/65); 0x61+ = Freeze Frame.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

struct DtcCode {
  int code;
  const char* name;
  const char* desc;
};

// Known Honda PGM-FI Diagnostic Trouble Codes (Civic 1996-2000)
// Ordenada por código, sin comillas dobles ni barra invertida (se inserta tal cual en JSON).
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
static const size_t HONDA_DTC_TABLE_LEN = sizeof(HONDA_DTC_TABLE) / sizeof(HONDA_DTC_TABLE[0]);

// Filas 0x40 + 0x50 = 64 códigos posibles, + 2 de la fila 0x60
static const int DTC_MAX_CODES = 66;

inline const DtcCode* findDtc(int code) {
  for (size_t i = 0; i < HONDA_DTC_TABLE_LEN; i++)
    if (HONDA_DTC_TABLE[i].code == code) return &HONDA_DTC_TABLE[i];
  return nullptr;
}

inline DtcCode lookupDtc(int code) {
  const DtcCode* d = findDtc(code);
  if (d) return *d;
  DtcCode unknown = { code, "Código Honda", "Código no listado en la tabla; consulta el manual de servicio." };
  return unknown;
}

// Agrega a out[count..] los códigos activos de n bytes que empiezan en firstCode.
// Bytes 0xFF = memoria sin inicializar (se ignoran). Código 0 = bandera interna (se ignora).
// Devuelve el nuevo count (nunca pasa de maxOut).
inline int decodeDtcBytes(const uint8_t* bytes, int n, int firstCode, int* out, int count, int maxOut) {
  for (int i = 0; i < n; i++) {
    if (bytes[i] == 0xFF) continue;
    int base = firstCode + i * 2;
    if ((bytes[i] >> 4) && base > 0 && count < maxOut) out[count++] = base;
    if ((bytes[i] & 0x0F) && count < maxOut) out[count++] = base + 1;
  }
  return count;
}

// Fila 0x60: solo el byte 0x60 (códigos 64/65) y solo si el código existe en la tabla.
// En el Civic del usuario, leer 0x61+ como códigos daba falsos 72-91 (commit d34c962).
inline int decodeDtcRow60(const uint8_t* row, int* out, int count, int maxOut) {
  uint8_t b = row[0];
  if (b == 0x00 || b == 0xFF) return count;
  if ((b >> 4) && findDtc(64) && count < maxOut) out[count++] = 64;
  if ((b & 0x0F) && findDtc(65) && count < maxOut) out[count++] = 65;
  return count;
}

// Seguridad: 21 04 01 reinicia la ECU. Solo con motor apagado y auto detenido.
inline bool clearDtcAllowed(int rpm, int vss) {
  return rpm < 200 && vss == 0;
}

struct ClearDtcResult {
  bool ack = false;        // la ECU respondió 01 03 FC
  bool verifyOk = false;   // se pudo releer la memoria después del reset
  int remaining = -1;      // códigos que siguen tras el reset (-1 = no se pudo verificar)
  bool mil = false;        // Check Engine tras el reset
  const char* error = "";  // "", "no_ecu", "engine_running"
};

// Éxito real = memoria releída limpia y MIL apagado (con o sin ACK).
inline bool clearDtcSucceeded(const ClearDtcResult& r) {
  return r.verifyOk && r.remaining == 0 && !r.mil;
}

inline void formatClearDtcJson(const ClearDtcResult& r, char* out, size_t outLen) {
  snprintf(out, outLen,
           "{\"type\":\"clearDtc\",\"ack\":%s,\"verifyOk\":%s,\"remaining\":%d,\"mil\":%s,\"cleared\":%s,\"error\":\"%s\"}",
           r.ack ? "true" : "false", r.verifyOk ? "true" : "false", r.remaining,
           r.mil ? "true" : "false", clearDtcSucceeded(r) ? "true" : "false", r.error);
}

#endif // HONDA_DTC_H
