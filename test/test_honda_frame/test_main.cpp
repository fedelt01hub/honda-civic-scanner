#include <unity.h>
#include "honda_frame.h"

void setUp() {}
void tearDown() {}

void test_checksum_known_frames() {
  const uint8_t init[4] = {0x20, 0x05, 0x00, 0x10};
  TEST_ASSERT_EQUAL_HEX8(0xCB, hondaChecksum(init, 4));   // visto en el log real: 20 05 00 10 CB
  const uint8_t clr[3] = {0x21, 0x04, 0x01};
  TEST_ASSERT_EQUAL_HEX8(0xDA, hondaChecksum(clr, 3));
}

void test_build_read_request_dtc_rows() {
  uint8_t r[5];
  buildReadRequest(0x40, 0x10, r);
  const uint8_t e40[5] = {0x20, 0x05, 0x40, 0x10, 0x8B};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(e40, r, 5);
  buildReadRequest(0x50, 0x10, r);
  const uint8_t e50[5] = {0x20, 0x05, 0x50, 0x10, 0x7B};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(e50, r, 5);
  buildReadRequest(0x60, 0x10, r);
  const uint8_t e60[5] = {0x20, 0x05, 0x60, 0x10, 0x6B};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(e60, r, 5);
}

static void zeroResponse(uint8_t* r) {   // 00 13 + 16x00 + ED
  r[0] = 0x00; r[1] = 0x13;
  for (int i = 0; i < 16; i++) r[2 + i] = 0x00;
  r[18] = 0xED;
}

void test_read_response_ok() {
  uint8_t r[19]; zeroResponse(r);
  TEST_ASSERT_EQUAL(READ_OK, checkReadResponse(r, 19, 0x10));
}
void test_read_response_short() {
  uint8_t r[19]; zeroResponse(r);
  TEST_ASSERT_EQUAL(READ_SHORT, checkReadResponse(r, 18, 0x10));
  TEST_ASSERT_EQUAL(READ_SHORT, checkReadResponse(r, 0, 0x10));
}
void test_read_response_bad_header() {
  uint8_t r[19]; zeroResponse(r);
  r[1] = 0x12; r[18] = 0xEE;              // checksum válido pero longitud equivocada
  TEST_ASSERT_EQUAL(READ_BAD_HEADER, checkReadResponse(r, 19, 0x10));
  zeroResponse(r);
  r[0] = 0x01; r[18] = 0xEC;              // checksum válido pero primer byte != 00
  TEST_ASSERT_EQUAL(READ_BAD_HEADER, checkReadResponse(r, 19, 0x10));
}
void test_read_response_bad_checksum() {
  uint8_t r[19]; zeroResponse(r);
  r[18] = 0xEE;
  TEST_ASSERT_EQUAL(READ_BAD_CHECKSUM, checkReadResponse(r, 19, 0x10));
}

void test_clear_request_and_ack() {
  const uint8_t e[4] = {0x21, 0x04, 0x01, 0xDA};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(e, CLEAR_DTC_REQ, 4);
  const uint8_t ack[3] = {0x01, 0x03, 0xFC};
  TEST_ASSERT_TRUE(isClearAck(ack, 3));
  TEST_ASSERT_FALSE(isClearAck(ack, 2));
  const uint8_t bad[3] = {0x01, 0x03, 0xFD};
  TEST_ASSERT_FALSE(isClearAck(bad, 3));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_checksum_known_frames);
  RUN_TEST(test_build_read_request_dtc_rows);
  RUN_TEST(test_read_response_ok);
  RUN_TEST(test_read_response_short);
  RUN_TEST(test_read_response_bad_header);
  RUN_TEST(test_read_response_bad_checksum);
  RUN_TEST(test_clear_request_and_ack);
  return UNITY_END();
}
