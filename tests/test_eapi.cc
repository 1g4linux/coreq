#include <iostream>
#include <string>

#include "corepkg/eapi.h"

#define ASSERT_EQ(val1, val2)                                                   \
  if (!((val1) == (val2))) {                                                    \
    std::cerr << "Assertion failed: " << #val1 << " == " << #val2 << " ("      \
              << (val1) << " != " << (val2) << ") at " << __FILE__ << ":"      \
              << __LINE__ << std::endl;                                         \
    return 1;                                                                    \
  }

static int test_assign_and_lookup() {
  Eapi::init_static();

  Eapi default_eapi;
  ASSERT_EQ(default_eapi.get(), "0");

  Eapi eapi_8;
  eapi_8.assign("8");
  ASSERT_EQ(eapi_8.get(), "8");

  Eapi eapi_8_again;
  eapi_8_again.assign("8");
  ASSERT_EQ(eapi_8_again.get(), "8");

  Eapi eapi_7;
  eapi_7.assign("7");
  ASSERT_EQ(eapi_7.get(), "7");

  return 0;
}

static int test_reinit_resets_mapping() {
  Eapi::free_static();
  Eapi::init_static();

  Eapi default_eapi;
  ASSERT_EQ(default_eapi.get(), "0");

  Eapi eapi_9;
  eapi_9.assign("9");
  ASSERT_EQ(eapi_9.get(), "9");

  Eapi::free_static();
  Eapi::init_static();

  Eapi reset_default;
  ASSERT_EQ(reset_default.get(), "0");

  return 0;
}

int main() {
  int failed = 0;
  if (test_assign_and_lookup() != 0) {
    failed++;
  }
  if (test_reinit_resets_mapping() != 0) {
    failed++;
  }

  // Intentionally do not call Eapi::free_static() here.
  // Old heap-backed storage leaks at process exit; static-storage implementations
  // should remain leak-free.
  if (failed == 0) {
    std::cout << "All EAPI tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
