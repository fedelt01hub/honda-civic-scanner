#include <unity.h>
#include "power_policy.h"

void setUp() {}
void tearDown() {}

void test_never_sleep_with_phone() {
  TEST_ASSERT_FALSE(shouldDeepSleep(999999, 0, true, false, 3000));
}
void test_never_sleep_with_ecu() {
  TEST_ASSERT_FALSE(shouldDeepSleep(999999, 0, false, true, 3000));
}
void test_no_sleep_before_window() {
  TEST_ASSERT_FALSE(shouldDeepSleep(2999, 0, false, false, 3000));
}
void test_sleep_after_window() {
  TEST_ASSERT_TRUE(shouldDeepSleep(3000, 0, false, false, 3000));
}
void test_millis_wraparound() {
  // millis() dio la vuelta: de 0xFFFFFF00 a 100 son 356 ms, todavía no duerme
  TEST_ASSERT_FALSE(shouldDeepSleep(100, 0xFFFFFF00u, false, false, 3000));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_never_sleep_with_phone);
  RUN_TEST(test_never_sleep_with_ecu);
  RUN_TEST(test_no_sleep_before_window);
  RUN_TEST(test_sleep_after_window);
  RUN_TEST(test_millis_wraparound);
  return UNITY_END();
}
