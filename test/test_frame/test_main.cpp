#include <unity.h>
#include "telemetry_frame.h"

void setUp() {}
void tearDown() {}

static TelemetryData sample() {
  TelemetryData d;
  d.rpm = 3000; d.vss = 80; d.vtec = true; d.connected = true; d.gear = 3;
  d.ect = 90; d.iat = 35; d.map = 45; d.baro = 101; d.tps = 20;
  d.o2 = 0.5f; d.batt = 14.5f; d.altFr = 30; d.eld = 12.5f; d.ign = -5.0f;
  d.inj = 2.5f; d.sft = -3; d.lft = 5; d.iacv = 33; d.knock = 0;
  d.maf = 12.5f; d.dtcCount = 1; d.o2Sec = 0.75f;
  return d;
}

// Formato que decodifica app/index.html. Si este test falla, la app se rompe.
void test_golden_frame() {
  const uint8_t expected[TELEMETRY_FRAME_LEN] = {
    0xA5, 0x5A, 0x1D, 0x0B, 0xB8, 0x50, 0x11, 0x03, 0x82, 0x4B,
    0x2D, 0x65, 0x14, 0x32, 0x91, 0x1E, 0x19, 0xF6, 0x00, 0xFA,
    0xFD, 0x05, 0x21, 0x00, 0x00, 0x7D, 0x01, 0x4B, 0x3F
  };
  uint8_t f[TELEMETRY_FRAME_LEN];
  packTelemetryFrame(sample(), f);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, f, TELEMETRY_FRAME_LEN);
}

void test_checksum_xor_is_zero() {
  uint8_t f[TELEMETRY_FRAME_LEN];
  packTelemetryFrame(sample(), f);
  uint8_t x = 0;
  for (int i = 2; i < TELEMETRY_FRAME_LEN; i++) x ^= f[i];
  TEST_ASSERT_EQUAL_HEX8(0x00, x);
}

void test_clamps_out_of_range() {
  TelemetryData d;
  d.batt = 99.0f;   // 990 -> se limita a 255
  d.ign = -200.0f;  // -400 -> se limita a -128 (0x80)
  uint8_t f[TELEMETRY_FRAME_LEN];
  packTelemetryFrame(d, f);
  TEST_ASSERT_EQUAL_HEX8(0xFF, f[14]);
  TEST_ASSERT_EQUAL_HEX8(0x80, f[17]);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_golden_frame);
  RUN_TEST(test_checksum_xor_is_zero);
  RUN_TEST(test_clamps_out_of_range);
  return UNITY_END();
}
