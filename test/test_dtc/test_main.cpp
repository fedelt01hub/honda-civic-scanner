#include <unity.h>
#include <string.h>
#include "honda_dtc.h"

void setUp() {}
void tearDown() {}

static uint8_t z[16];
static void zero() { memset(z, 0, sizeof(z)); }

void test_all_zero_no_codes() {
  zero();
  int out[DTC_MAX_CODES];
  TEST_ASSERT_EQUAL(0, decodeDtcBytes(z, 16, 0, out, 0, DTC_MAX_CODES));
  TEST_ASSERT_EQUAL(0, decodeDtcBytes(z, 16, 32, out, 0, DTC_MAX_CODES));
}

void test_row40_nibbles() {
  int out[DTC_MAX_CODES];
  zero(); z[0] = 0x01;                     // 0x40 bajo -> código 1 (O2)
  TEST_ASSERT_EQUAL(1, decodeDtcBytes(z, 16, 0, out, 0, DTC_MAX_CODES));
  TEST_ASSERT_EQUAL(1, out[0]);
  zero(); z[5] = 0x10;                     // 0x45 alto -> código 10 (IAT)
  TEST_ASSERT_EQUAL(1, decodeDtcBytes(z, 16, 0, out, 0, DTC_MAX_CODES));
  TEST_ASSERT_EQUAL(10, out[0]);
  zero(); z[7] = 0x10;                     // 0x47 alto -> código 14 (IACV)
  decodeDtcBytes(z, 16, 0, out, 0, DTC_MAX_CODES);
  TEST_ASSERT_EQUAL(14, out[0]);
  zero(); z[3] = 0x11;                     // 0x43 ambos -> 6 y 7, en ese orden
  TEST_ASSERT_EQUAL(2, decodeDtcBytes(z, 16, 0, out, 0, DTC_MAX_CODES));
  TEST_ASSERT_EQUAL(6, out[0]);
  TEST_ASSERT_EQUAL(7, out[1]);
}

void test_nibble_value_above_one_counts() {
  int out[DTC_MAX_CODES];
  zero(); z[5] = 0x20;
  TEST_ASSERT_EQUAL(1, decodeDtcBytes(z, 16, 0, out, 0, DTC_MAX_CODES));
  TEST_ASSERT_EQUAL(10, out[0]);
}

void test_code_zero_is_ignored() {
  int out[DTC_MAX_CODES];
  zero(); z[0] = 0x10;                     // nibble alto de 0x40 = bandera interna
  TEST_ASSERT_EQUAL(0, decodeDtcBytes(z, 16, 0, out, 0, DTC_MAX_CODES));
}

void test_ff_byte_is_ignored() {
  int out[DTC_MAX_CODES];
  zero(); z[2] = 0xFF;
  TEST_ASSERT_EQUAL(0, decodeDtcBytes(z, 16, 0, out, 0, DTC_MAX_CODES));
}

void test_row50_offset() {
  int out[DTC_MAX_CODES];
  zero(); z[4] = 0x01;                     // 0x54 bajo -> 32+8+1 = 41
  TEST_ASSERT_EQUAL(1, decodeDtcBytes(z, 16, 32, out, 0, DTC_MAX_CODES));
  TEST_ASSERT_EQUAL(41, out[0]);
  zero(); z[15] = 0x01;                    // 0x5F bajo -> 63 (O2 secundario, confirmado en el auto)
  decodeDtcBytes(z, 16, 32, out, 0, DTC_MAX_CODES);
  TEST_ASSERT_EQUAL(63, out[0]);
}

void test_append_across_rows() {
  int out[DTC_MAX_CODES];
  uint8_t a[16] = {0}; a[0] = 0x01;
  uint8_t b[16] = {0}; b[6] = 0x01;
  int n = decodeDtcBytes(a, 16, 0, out, 0, DTC_MAX_CODES);
  n = decodeDtcBytes(b, 16, 32, out, n, DTC_MAX_CODES);
  TEST_ASSERT_EQUAL(2, n);
  TEST_ASSERT_EQUAL(1, out[0]);
  TEST_ASSERT_EQUAL(45, out[1]);
}

void test_respects_max_out() {
  int out[3];
  uint8_t a[16]; memset(a, 0x11, sizeof(a));
  TEST_ASSERT_EQUAL(3, decodeDtcBytes(a, 16, 0, out, 0, 3));
}

void test_row60_only_byte0_known_codes() {
  int out[DTC_MAX_CODES];
  zero(); z[0] = 0x01;                     // 0x60 bajo -> 65 (en tabla)
  TEST_ASSERT_EQUAL(1, decodeDtcRow60(z, out, 0, DTC_MAX_CODES));
  TEST_ASSERT_EQUAL(65, out[0]);
  zero(); z[0] = 0x10;                     // 64 no está en la tabla -> se ignora
  TEST_ASSERT_EQUAL(0, decodeDtcRow60(z, out, 0, DTC_MAX_CODES));
  zero(); z[1] = 0x55; z[4] = 0x99;        // 0x61+ es Freeze Frame, nunca códigos
  TEST_ASSERT_EQUAL(0, decodeDtcRow60(z, out, 0, DTC_MAX_CODES));
  zero(); z[0] = 0xFF;
  TEST_ASSERT_EQUAL(0, decodeDtcRow60(z, out, 0, DTC_MAX_CODES));
}

void test_lookup_known_and_unknown() {
  TEST_ASSERT_NOT_NULL(strstr(lookupDtc(14).name, "IACV"));
  TEST_ASSERT_EQUAL(14, lookupDtc(14).code);
  DtcCode u = lookupDtc(2);
  TEST_ASSERT_EQUAL(2, u.code);
  TEST_ASSERT_EQUAL_STRING("Código Honda", u.name);
  TEST_ASSERT_NULL(findDtc(2));
}

void test_table_sorted_unique_and_json_safe() {
  for (size_t i = 0; i < HONDA_DTC_TABLE_LEN; i++) {
    if (i > 0) TEST_ASSERT_TRUE(HONDA_DTC_TABLE[i].code > HONDA_DTC_TABLE[i - 1].code);
    const char* s[2] = {HONDA_DTC_TABLE[i].name, HONDA_DTC_TABLE[i].desc};
    for (int k = 0; k < 2; k++)
      for (const char* p = s[k]; *p; p++)
        TEST_ASSERT_TRUE(*p != '"' && *p != '\\' && (unsigned char)*p >= 0x20);
  }
}

void test_clear_guard() {
  TEST_ASSERT_TRUE(clearDtcAllowed(0, 0));
  TEST_ASSERT_TRUE(clearDtcAllowed(150, 0));
  TEST_ASSERT_FALSE(clearDtcAllowed(750, 0));   // motor en ralentí
  TEST_ASSERT_FALSE(clearDtcAllowed(0, 5));     // auto moviéndose
}

void test_clear_json_success() {
  ClearDtcResult r; r.ack = true; r.verifyOk = true; r.remaining = 0; r.mil = false;
  char buf[160];
  formatClearDtcJson(r, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING(
      "{\"type\":\"clearDtc\",\"ack\":true,\"verifyOk\":true,\"remaining\":0,\"mil\":false,\"cleared\":true,\"error\":\"\"}",
      buf);
}

void test_clear_json_cleared_without_ack() {
  ClearDtcResult r; r.ack = false; r.verifyOk = true; r.remaining = 0; r.mil = false;
  TEST_ASSERT_TRUE(clearDtcSucceeded(r));
}

void test_clear_json_not_cleared_when_code_returns() {
  ClearDtcResult r; r.ack = true; r.verifyOk = true; r.remaining = 1; r.mil = true;
  TEST_ASSERT_FALSE(clearDtcSucceeded(r));
}

void test_clear_json_refused() {
  ClearDtcResult r; r.error = "engine_running";
  char buf[160];
  formatClearDtcJson(r, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING(
      "{\"type\":\"clearDtc\",\"ack\":false,\"verifyOk\":false,\"remaining\":-1,\"mil\":false,\"cleared\":false,\"error\":\"engine_running\"}",
      buf);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_all_zero_no_codes);
  RUN_TEST(test_row40_nibbles);
  RUN_TEST(test_nibble_value_above_one_counts);
  RUN_TEST(test_code_zero_is_ignored);
  RUN_TEST(test_ff_byte_is_ignored);
  RUN_TEST(test_row50_offset);
  RUN_TEST(test_append_across_rows);
  RUN_TEST(test_respects_max_out);
  RUN_TEST(test_row60_only_byte0_known_codes);
  RUN_TEST(test_lookup_known_and_unknown);
  RUN_TEST(test_table_sorted_unique_and_json_safe);
  RUN_TEST(test_clear_guard);
  RUN_TEST(test_clear_json_success);
  RUN_TEST(test_clear_json_cleared_without_ack);
  RUN_TEST(test_clear_json_not_cleared_when_code_returns);
  RUN_TEST(test_clear_json_refused);
  return UNITY_END();
}
