#include <cstdlib>
#include <iostream>
#include <string>

#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"

#define ASSERT_TRUE(condition)                                                   \
  if (!(condition)) {                                                            \
    std::cerr << "Assertion failed: " << #condition << " at " << __FILE__       \
              << ":" << __LINE__ << std::endl;                                   \
    return 1;                                                                    \
  }

#define ASSERT_EQ(val1, val2)                                                    \
  if (!((val1) == (val2))) {                                                     \
    std::cerr << "Assertion failed: " << #val1 << " == " << #val2 << " ("       \
              << (val1) << " != " << (val2) << ") at " << __FILE__ << ":"       \
              << __LINE__ << std::endl;                                          \
    return 1;                                                                    \
  }

static int test_defaults_are_loaded_and_resolved() {
  const std::string prefix = "/tmp/coreq-defaults-prefix";
  ASSERT_TRUE(setenv("COREQ_PREFIX", prefix.c_str(), 1) == 0);

  CoreqRc& rc = get_coreqrc(COREQ_VARS_PREFIX);

  ASSERT_TRUE(rc.cstr("COREQ_WORLD") != NULLPTR);
  ASSERT_TRUE(rc.cstr("COREQ_WORLD_SETS") != NULLPTR);

  ASSERT_EQ(rc["CURRENT_WORLD"], std::string("true"));
  ASSERT_EQ(rc["REMOTE_DEFAULT"], std::string("0"));
  ASSERT_EQ(rc["COREQ_WORLD_SETS"], rc["COREQ_WORLD"] + "_sets");

  const std::string& eprefix = rc["EPREFIX"];
  ASSERT_TRUE(eprefix.find(prefix) == 0);
  ASSERT_TRUE(rc["COREPKG_CONFIGROOT"].find(eprefix) == 0);

  ASSERT_TRUE(!rc["FORMAT_BEFORE_SET_USE"].empty());
  ASSERT_TRUE(!rc["FORMAT_AFTER_SET_USE"].empty());
  return 0;
}

int main() {
  int failed = 0;
  if (test_defaults_are_loaded_and_resolved() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All coreqrc defaults tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
