#ifndef DEVICE_INFO_H
#define DEVICE_INFO_H

#include <stdint.h>
#include <stdio.h>
#include <stddef.h>

#define FW_VERSION "1.2.0"   // 1.2.0: DTC confiables (borrado seguro y verificado)
#define HW_REV "proto"   // cambiar a "A" al flashear la PCB rev A

// Nombre BLE único por unidad: "<prefijo>-XXXX" con los 2 últimos bytes de la MAC.
// Los 4 caracteres se imprimen en la etiqueta del equipo.
inline void formatDeviceName(const char *prefix, const uint8_t mac[6], char *out, size_t outLen) {
  snprintf(out, outLen, "%s-%02X%02X", prefix, mac[4], mac[5]);
}

// Respuesta al comando BLE "INFO".
inline void formatInfoJson(const char *fw, const char *hw, const char *name, int ecuType,
                           char *out, size_t outLen) {
  snprintf(out, outLen,
           "{\"type\":\"info\",\"fw\":\"%s\",\"hw\":\"%s\",\"name\":\"%s\",\"ecuType\":%d}",
           fw, hw, name, ecuType);
}

#endif // DEVICE_INFO_H
