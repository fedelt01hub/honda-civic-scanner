#ifndef POWER_POLICY_H
#define POWER_POLICY_H

#include <stdint.h>

// El +12V del DLC de 3 pines es constante (medido): sin dormir, el escáner
// descargaría la batería con el auto apagado.
// Ciclo con llave en OFF: 15 s dormido -> 3 s despierto intentando hablar con la ECU.
static const uint32_t SLEEP_SECONDS = 15;
static const uint32_t AWAKE_WINDOW_AFTER_WAKE_MS = 3000;   // tras despertar por timer
static const uint32_t AWAKE_WINDOW_COLD_BOOT_MS = 120000;  // tras conectar el escáner / tras actividad

// lastActivityMs = último millis() con celular o ECU conectados (o el arranque).
inline bool shouldDeepSleep(uint32_t nowMs, uint32_t lastActivityMs,
                            bool phoneConnected, bool ecuConnected, uint32_t windowMs) {
  if (phoneConnected || ecuConnected) return false;
  return (uint32_t)(nowMs - lastActivityMs) >= windowMs;
}

#endif // POWER_POLICY_H
