#include <unity.h>
#include <string.h>
#include "device_info.h"

void setUp() {}
void tearDown() {}

void test_name_uses_last_two_mac_bytes() {
  const uint8_t mac[6] = {0x34, 0x85, 0x18, 0x01, 0xA1, 0x0B};
  char name[24];
  formatDeviceName("Civic99-OBD", mac, name, sizeof(name));
  TEST_ASSERT_EQUAL_STRING("Civic99-OBD-A10B", name);
}

void test_name_truncates_to_buffer() {
  const uint8_t mac[6] = {0, 0, 0, 0, 0xFF, 0xEE};
  char name[8];
  formatDeviceName("Civic99-OBD", mac, name, sizeof(name));
  TEST_ASSERT_EQUAL_UINT(7, strlen(name));
}

void test_info_json() {
  char buf[160];
  formatInfoJson("1.0.0", "proto", "Civic99-OBD-A10B", 2, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING(
      "{\"type\":\"info\",\"fw\":\"1.0.0\",\"hw\":\"proto\",\"name\":\"Civic99-OBD-A10B\",\"ecuType\":2}",
      buf);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_name_uses_last_two_mac_bytes);
  RUN_TEST(test_name_truncates_to_buffer);
  RUN_TEST(test_info_json);
  return UNITY_END();
}
