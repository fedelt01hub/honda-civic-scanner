#ifndef TELEMETRY_FRAME_H
#define TELEMETRY_FRAME_H

#include <stdint.h>
#include <math.h>
#include "telemetry_types.h"

// Trama binaria BLE de telemetría (29 bytes) que decodifica app/index.html.
// NO cambiar el formato sin actualizar la app (lo fija test/test_frame).
static const int TELEMETRY_FRAME_LEN = 29;

inline int tfClamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

inline void packTelemetryFrame(const TelemetryData &d, uint8_t *frame) {
  frame[0] = 0xA5; // Byte de sincronización 1
  frame[1] = 0x5A; // Byte de sincronización 2
  frame[2] = TELEMETRY_FRAME_LEN;

  // RPM (uint16_t, Big-Endian)
  frame[3] = (uint8_t)(d.rpm >> 8);
  frame[4] = (uint8_t)(d.rpm & 0xFF);

  // VSS (km/h)
  frame[5] = (uint8_t)d.vss;

  // Bit 0: VTEC | 1: A/C | 2: Freno | 3: P/N | 4: Conectado | 5: Demo | 6: MIL
  frame[6] = (d.vtec ? 0x01 : 0) |
             (d.ac ? 0x02 : 0) |
             (d.brake ? 0x04 : 0) |
             (d.parkNeutral ? 0x08 : 0) |
             (d.connected ? 0x10 : 0) |
             (d.demo ? 0x20 : 0) |
             (d.mil ? 0x40 : 0);

  // Marcha (0=N, 1..4)
  frame[7] = (uint8_t)d.gear;

  // Temperaturas (offset +40)
  frame[8] = (uint8_t)(d.ect + 40);
  frame[9] = (uint8_t)(d.iat + 40);

  // Presiones (kPa)
  frame[10] = (uint8_t)d.map;
  frame[11] = (uint8_t)d.baro;

  // TPS (%)
  frame[12] = (uint8_t)d.tps;

  // O2 primario (V * 100)
  frame[13] = (uint8_t)tfClamp((int)(d.o2 * 100.0f), 0, 255);

  // Batería (V * 10)
  frame[14] = (uint8_t)tfClamp((int)(d.batt * 10.0f), 0, 255);

  // Alt FR (%)
  frame[15] = (uint8_t)d.altFr;

  // ELD (A * 2)
  frame[16] = (uint8_t)tfClamp((int)roundf(d.eld * 2.0f), 0, 255);

  // Avance de encendido (grados * 2, con signo)
  frame[17] = (uint8_t)(int8_t)tfClamp((int)(d.ign * 2.0f), -128, 127);

  // Inyección (ms * 100, uint16_t Big-Endian)
  uint16_t injScaled = (uint16_t)tfClamp((int)(d.inj * 100.0f), 0, 65535);
  frame[18] = (uint8_t)(injScaled >> 8);
  frame[19] = (uint8_t)(injScaled & 0xFF);

  // Fuel trims (%, con signo)
  frame[20] = (uint8_t)(int8_t)tfClamp(d.sft, -128, 127);
  frame[21] = (uint8_t)(int8_t)tfClamp(d.lft, -128, 127);

  // IACV (%)
  frame[22] = (uint8_t)d.iacv;

  // Knock
  frame[23] = (uint8_t)d.knock;

  // MAF (g/s * 10, uint16_t Big-Endian)
  uint16_t mafScaled = (uint16_t)tfClamp((int)(d.maf * 10.0f), 0, 65535);
  frame[24] = (uint8_t)(mafScaled >> 8);
  frame[25] = (uint8_t)(mafScaled & 0xFF);

  // DTC activos
  frame[26] = (uint8_t)d.dtcCount;

  // O2 secundario (V * 100)
  frame[27] = (uint8_t)tfClamp((int)(d.o2Sec * 100.0f), 0, 255);

  // Checksum XOR de bytes [2..27]
  uint8_t cs = 0;
  for (int i = 2; i < TELEMETRY_FRAME_LEN - 1; i++) cs ^= frame[i];
  frame[TELEMETRY_FRAME_LEN - 1] = cs;
}

#endif // TELEMETRY_FRAME_H
