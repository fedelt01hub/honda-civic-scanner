#ifndef HONDA_FRAME_H
#define HONDA_FRAME_H

// Tramas del protocolo Honda 3 pines (sin Arduino: se prueba con pio test -e native).
//   Leer   : 20 05 ADDR LEN CS   ->  00 LEN+3 DATA x LEN CS
//   Borrar : 21 04 01 DA         ->  01 03 FC
//   CS     : (0x100 - suma) & 0xFF  (la trama completa suma 0x00)
#include <stdint.h>

inline uint8_t hondaChecksum(const uint8_t* d, int len) {
  uint8_t sum = 0;
  for (int i = 0; i < len; i++) sum += d[i];
  return (uint8_t)(0x100 - sum);
}

inline void buildReadRequest(uint8_t addr, uint8_t len, uint8_t out[5]) {
  out[0] = 0x20;
  out[1] = 0x05;
  out[2] = addr;
  out[3] = len;
  out[4] = hondaChecksum(out, 4);
}

enum ReadFrameStatus { READ_OK, READ_SHORT, READ_BAD_HEADER, READ_BAD_CHECKSUM };

inline ReadFrameStatus checkReadResponse(const uint8_t* resp, int rxLen, uint8_t len) {
  int expected = len + 3;
  if (rxLen < expected) return READ_SHORT;
  if (resp[0] != 0x00 || resp[1] != expected) return READ_BAD_HEADER;
  if (hondaChecksum(resp, expected - 1) != resp[expected - 1]) return READ_BAD_CHECKSUM;
  return READ_OK;
}

static const uint8_t CLEAR_DTC_REQ[4] = {0x21, 0x04, 0x01, 0xDA};

inline bool isClearAck(const uint8_t* resp, int rxLen) {
  return rxLen == 3 && resp[0] == 0x01 && resp[1] == 0x03 && resp[2] == 0xFC;
}

#endif // HONDA_FRAME_H
