/*
 * ==============================================================================
 * Honda Civic '99 PGM-FI Diagnostic Scanner & Live BLE Telemetry
 * ==============================================================================
 * Plataforma: ESP32-C3 (BLE 5.0)
 * Transceptor: L9637D (ISO 9141 / K-Line)
 * Protocolo: Honda 3-Pin DLC (9600 Baud, 8N1, Half-Duplex, 20 05 ADDR LEN CS)
 *
 * Anuncia el servicio estándar Nordic UART Service (NUS) como 'Civic99-OBD'
 * y transmite telemetría en tiempo real a 20 Hz (paquetes binarios de 29 bytes)
 * hacia la aplicación móvil PWA / Web Bluetooth.
 * ==============================================================================
 */

#include "honda_kline.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// ==============================================================================
// COLA DE COMANDOS K-LINE (ACCESO EXCLUSIVO DESDE EL LOOP)
// ==============================================================================
enum PendingCmd : uint8_t {
  CMD_NONE,
  CMD_GET_DTC,
  CMD_CLEAR_DTC,
  CMD_ECHO,
  CMD_REINIT,
  CMD_GET_DIAG
};
volatile PendingCmd pendingCmd = CMD_NONE;

// ==============================================================================
// CONFIGURACIÓN DE PINES
// ==============================================================================
#define KLINE_RX_PIN 4
#define KLINE_TX_PIN 5
#define LED_STATUS_PIN 8

// ==============================================================================
// CONFIGURACIÓN BLUETOOTH LOW ENERGY (NORDIC UART SERVICE)
// ==============================================================================
#define BLE_DEVICE_NAME "Civic99-OBD"
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer *pServer = nullptr;
BLECharacteristic *pTxCharacteristic = nullptr;
BLECharacteristic *pRxCharacteristic = nullptr;

bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long lastBleNotify = 0;

// Enviar cadena de texto / JSON por la característica TX
void bleSendText(const String &str) {
  if (!deviceConnected || !pTxCharacteristic) return;
  String full = str.endsWith("\n") ? str : (str + "\n");
  size_t len = full.length();
  const uint8_t *data = (const uint8_t *)full.c_str();
  // Transmitir en bloques de máximo 128 bytes
  size_t offset = 0;
  while (offset < len) {
    size_t chunk = len - offset;
    if (chunk > 128) chunk = 128;
    pTxCharacteristic->setValue((uint8_t *)(data + offset), chunk);
    pTxCharacteristic->notify();
    offset += chunk;
    delay(15);
  }
}

// Callbacks del Servidor BLE (Conexión / Desconexión)
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("[BLE] ¡Dispositivo móvil conectado!");
  }

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("[BLE] Dispositivo móvil desconectado.");
  }
};

void handlePendingCmd(PendingCmd cmd) {
  switch (cmd) {
    case CMD_GET_DTC: {
      std::vector<DtcCode> codes;
      bool ok = honda.getActiveDTCs(codes);
      if (ok) {
        String json = "{\"type\":\"dtc\",\"ok\":true,\"mil\":" + String(honda.data.mil ? "true" : "false");
        if (honda.diag.freezeFrame.valid) {
          char ffBuf[160];
          snprintf(ffBuf, sizeof(ffBuf),
                   ",\"freezeFrame\":{\"ect\":%d,\"iat\":%d,\"map\":%d,\"baro\":%d,\"tps\":%d,\"rawHex\":\"%02X %02X %02X %02X %02X\"}",
                   honda.diag.freezeFrame.ect,
                   honda.diag.freezeFrame.iat,
                   honda.diag.freezeFrame.map,
                   honda.diag.freezeFrame.baro,
                   honda.diag.freezeFrame.tps,
                   honda.diag.freezeFrame.rawEct,
                   honda.diag.freezeFrame.rawIat,
                   honda.diag.freezeFrame.rawMap,
                   honda.diag.freezeFrame.rawBaro,
                   honda.diag.freezeFrame.rawTps);
          json += ffBuf;
        }
        json += ",\"codes\":[";
        for (size_t i = 0; i < codes.size(); i++) {
          json += "{\"code\":" + String(codes[i].code) + ",";
          json += "\"name\":\"" + String(codes[i].name) + "\",";
          json += "\"desc\":\"" + String(codes[i].desc) + "\"}";
          if (i + 1 < codes.size()) json += ",";
        }
        json += "]}";
        bleSendText(json);
      } else {
        char errBuf[128];
        snprintf(errBuf, sizeof(errBuf), "{\"type\":\"dtc\",\"ok\":false,\"error\":\"%s\"}", honda.diag.lastError);
        bleSendText(errBuf);
      }
      break;
    }
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
      lastBleNotify = millis();
      break;
    }
    case CMD_ECHO: {
      honda.echoTest(16);
      char buf[128];
      snprintf(buf, sizeof(buf),
               "{\"type\":\"echo\",\"sent\":%d,\"rx\":%d,\"ok\":%d}",
               honda.diag.echoTestSent, honda.diag.echoTestRx, honda.diag.echoTestOk);
      bleSendText(buf);
      break;
    }
    case CMD_REINIT: {
      bool ok = honda.sendInit();
      bleSendText(String("{\"type\":\"reinit\",\"success\":") + (ok ? "true}" : "false}"));
      break;
    }
    case CMD_GET_DIAG: {
      const DiagInfo &g = honda.diag;
      char diagBuf[850];
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
      lastBleNotify = millis(); // Evita que la trama binaria salga pegada al JSON
      break;
    }
    default:
      break;
  }
}

// Callbacks de Comandos entrantes desde la App móvil (RX)
class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String rxValue = pCharacteristic->getValue();
    if (rxValue.length() == 0) return;

    rxValue.trim();
    Serial.printf("[BLE-RX] Comando recibido: '%s'\n", rxValue.c_str());

    // 1. Alternar tipo de ECU (1 = OBD1, 2 = OBD2)
    if (rxValue.startsWith("ECU:")) {
      int t = rxValue.substring(4).toInt();
      if (t == 1 || t == 2) {
        honda.ecuType = t;
        bleSendText("{\"type\":\"ecuType\",\"val\":" + String(t) + "}");
      }
    }
    // 2. Alternar modo Demo
    else if (rxValue.startsWith("DEMO:")) {
      bool en = (rxValue.substring(5).toInt() == 1);
      honda.setDemoMode(en);
      bleSendText(String("{\"type\":\"demo\",\"val\":") + (en ? "true}" : "false}"));
    }
    // 3. Solicitar Códigos DTC (se encola para ejecución exclusiva en loop)
    else if (rxValue == "GET_DTC") {
      pendingCmd = CMD_GET_DTC;
    }
    // 4. Borrar códigos DTC (se encola para ejecución exclusiva en loop)
    else if (rxValue == "CLEAR_DTC") {
      pendingCmd = CMD_CLEAR_DTC;
    }
    // 5. Solicitar estadísticas de diagnóstico (se encola solo si no hay comando prioritario)
    else if (rxValue == "GET_DIAG") {
      if (pendingCmd == CMD_NONE) {
        pendingCmd = CMD_GET_DIAG;
      }
    }
    // 6. Prueba de eco de hardware (se encola)
    else if (rxValue == "ECHO") {
      pendingCmd = CMD_ECHO;
    }
    // 7. Reiniciar contadores
    else if (rxValue == "RESET_COUNTERS") {
      honda.resetCounters();
      bleSendText("{\"type\":\"resetCounters\",\"success\":true}");
    }
    // 8. Reintentar inicialización K-Line (se encola)
    else if (rxValue == "REINIT") {
      pendingCmd = CMD_REINIT;
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n--- Honda Civic '99 Scanner BLE Inicializando ---");

  // Configurar LED de estado
  pinMode(LED_STATUS_PIN, OUTPUT);
  digitalWrite(LED_STATUS_PIN, LOW);

  // Inicializar driver K-Line Honda
  Serial.println("[K-LINE] Configurando UART a 9600 baudios...");
  honda.begin(KLINE_RX_PIN, KLINE_TX_PIN);

  // Configurar e inicializar BLE
  Serial.printf("[BLE] Inicializando dispositivo BLE '%s'...\n", BLE_DEVICE_NAME);
  BLEDevice::init(BLE_DEVICE_NAME);

  // Crear Servidor BLE
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Crear Servicio Nordic UART
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Característica TX (Notificación hacia el celular)
  pTxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_TX,
      BLECharacteristic::PROPERTY_NOTIFY
  );

  // Característica RX (Escritura desde el celular)
  pRxCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID_RX,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  pRxCharacteristic->setCallbacks(new RxCallbacks());

  // Iniciar servicio y anuncio (Advertising)
  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // Funciones de baja latencia en iOS/Android
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] Servicio Nordic UART activo y anunciando.");

  // Enviar secuencia inicial de detección de ECU
  honda.sendInit();
}

// Empaquetar y notificar la trama binaria de telemetría (29 bytes)
void sendTelemetryFrame() {
  if (!deviceConnected || !pTxCharacteristic) return;

  const TelemetryData &d = honda.data;
  uint8_t frame[29];

  frame[0] = 0xA5; // Byte de sincronización 1
  frame[1] = 0x5A; // Byte de sincronización 2
  frame[2] = 29;   // Longitud de la trama

  // RPM (uint16_t, Big-Endian)
  frame[3] = (uint8_t)(d.rpm >> 8);
  frame[4] = (uint8_t)(d.rpm & 0xFF);

  // VSS (Velocidad km/h)
  frame[5] = (uint8_t)d.vss;

  // Estado discreto / Bitfield (1 byte)
  // Bit 0: VTEC | Bit 1: A/C Clutch | Bit 2: Freno | Bit 3: P/N | Bit 4: Conectado | Bit 5: Demo | Bit 6: MIL Check Engine
  frame[6] = (d.vtec ? 0x01 : 0) |
             (d.ac ? 0x02 : 0) |
             (d.brake ? 0x04 : 0) |
             (d.parkNeutral ? 0x08 : 0) |
             (d.connected ? 0x10 : 0) |
             (d.demo ? 0x20 : 0) |
             (d.mil ? 0x40 : 0);

  // Velocidad de transmisión automática (0=N, 1=1st, 2=2nd, 3=D3, 4=D4)
  frame[7] = (uint8_t)d.gear;

  // Temperaturas (offset +40 para asegurar valores positivos 0-255)
  frame[8] = (uint8_t)(d.ect + 40);
  frame[9] = (uint8_t)(d.iat + 40);

  // Presiones
  frame[10] = (uint8_t)d.map;
  frame[11] = (uint8_t)d.baro;

  // TPS (% de apertura)
  frame[12] = (uint8_t)d.tps;

  // Sensor de Oxígeno O2 Primario (Voltios * 100, ej: 0.85V -> 85)
  frame[13] = (uint8_t)constrain((int)(d.o2 * 100.0f), 0, 255);

  // Batería (Voltios * 10, ej: 13.8V -> 138)
  frame[14] = (uint8_t)constrain((int)(d.batt * 10.0f), 0, 255);

  // Carga de alternador Alt FR (%)
  frame[15] = (uint8_t)d.altFr;

  // Detector de carga eléctrica ELD (Amperes * 2, rango 0 a 127.5 A con resolución de 0.5 A)
  frame[16] = (uint8_t)constrain((int)roundf(d.eld * 2.0f), 0, 255);

  // Avance de encendido IGN (Grados * 2, cast a uint8_t para transporte)
  frame[17] = (uint8_t)(int8_t)constrain((int)(d.ign * 2.0f), -128, 127);

  // Tiempo de inyección INJ (Milisegundos * 100, uint16_t Big-Endian, ej: 2.50ms -> 250)
  uint16_t injScaled = (uint16_t)constrain((int)(d.inj * 100.0f), 0, 65535);
  frame[18] = (uint8_t)(injScaled >> 8);
  frame[19] = (uint8_t)(injScaled & 0xFF);

  // Trims de combustible (Short / Long Term Fuel Trim: -100% a +100%)
  frame[20] = (uint8_t)(int8_t)constrain(d.sft, -128, 127);
  frame[21] = (uint8_t)(int8_t)constrain(d.lft, -128, 127);

  // Válvula de ralentí IACV (%)
  frame[22] = (uint8_t)d.iacv;

  // Sensor de cascabeleo / Detonación Knock
  frame[23] = (uint8_t)d.knock;

  // Flujo de aire MAF (g/s * 10, uint16_t Big-Endian)
  uint16_t mafScaled = (uint16_t)constrain((int)(d.maf * 10.0f), 0, 65535);
  frame[24] = (uint8_t)(mafScaled >> 8);
  frame[25] = (uint8_t)(mafScaled & 0xFF);

  // Cantidad de códigos de falla DTC activos
  frame[26] = (uint8_t)d.dtcCount;

  // Sensor de Oxígeno O2 Secundario (Voltios * 100, ej: 0.65V -> 65)
  frame[27] = (uint8_t)constrain((int)(d.o2Sec * 100.0f), 0, 255);

  // Checksum XOR de bytes [2..27]
  uint8_t cs = 0;
  for (int i = 2; i < 28; i++) {
    cs ^= frame[i];
  }
  frame[28] = cs;

  pTxCharacteristic->setValue(frame, sizeof(frame));
  pTxCharacteristic->notify();
}

void loop() {
  // Procesar comandos pesados pendientes (DTC, eco, reinit) con acceso exclusivo al UART K-Line
  if (pendingCmd != CMD_NONE) {
    PendingCmd c = pendingCmd;
    pendingCmd = CMD_NONE;
    handlePendingCmd(c);
  } else {
    // Actualizar lectura continua de la ECU Honda
    honda.update();
  }

  // Gestión de reconexión BLE
  if (!deviceConnected && oldDeviceConnected) {
    delay(100); // Pequeña pausa para que la pila BLE limpie buffers
    pServer->startAdvertising();
    Serial.println("[BLE] Reiniciando publicidad...");
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }

  // Notificar telemetría por BLE cada 50ms (20 Hz)
  unsigned long now = millis();
  if (deviceConnected && (now - lastBleNotify >= 50)) {
    lastBleNotify = now;
    sendTelemetryFrame();
  }

  // Parpadeo de LED de estado:
  // - Rápido (200ms) si no hay conexión BLE ni de ECU
  // - Pulsos suaves (1000ms) cuando todo está sincronizado
  static unsigned long lastBlink = 0;
  unsigned long blinkInterval = (deviceConnected && honda.data.connected) ? 1000 : 250;
  if (now - lastBlink > blinkInterval) {
    lastBlink = now;
    digitalWrite(LED_STATUS_PIN, !digitalRead(LED_STATUS_PIN));
  }

  // Ceder 2ms al planificador FreeRTOS
  delay(2);
}
