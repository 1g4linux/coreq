#include <iostream>
#include <string>

#include "corepkg/basicversion.h"
#include "corepkg/instversion.h"

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

static int test_default_metadata_flags() {
  InstVersion v;
  ASSERT_TRUE(!v.know_slot);
  ASSERT_TRUE(!v.read_failed);
  ASSERT_TRUE(!v.know_use);
  ASSERT_TRUE(!v.know_restricted);
  ASSERT_TRUE(!v.know_deps);
  ASSERT_TRUE(!v.know_eapi);
  ASSERT_TRUE(!v.know_instDate);
  ASSERT_TRUE(!v.know_contents);
  ASSERT_TRUE(!v.know_overlay);
  ASSERT_TRUE(!v.overlay_failed);
  return 0;
}

static int test_compare_falls_back_without_overlay() {
  InstVersion left;
  InstVersion right;
  std::string errtext;

  ASSERT_EQ(left.parseVersion("1.0", &errtext), BasicVersion::parsedOK);
  ASSERT_EQ(right.parseVersion("1.0", &errtext), BasicVersion::parsedOK);

  left.know_overlay = false;
  right.know_overlay = false;
  left.priority = 1;
  left.overlay_key = 1;
  right.priority = 100;
  right.overlay_key = 100;

  ASSERT_TRUE(InstVersion::compare(left, right) == 0);
  ASSERT_TRUE(left == right);
  return 0;
}

static int test_compare_uses_overlay_when_known() {
  InstVersion left;
  InstVersion right;
  std::string errtext;

  ASSERT_EQ(left.parseVersion("1.0", &errtext), BasicVersion::parsedOK);
  ASSERT_EQ(right.parseVersion("1.0", &errtext), BasicVersion::parsedOK);

  left.know_overlay = true;
  right.know_overlay = true;
  left.overlay_failed = false;
  right.overlay_failed = false;
  left.priority = 5;
  right.priority = 5;
  left.overlay_key = 1;
  right.overlay_key = 2;

  ASSERT_TRUE(InstVersion::compare(left, right) < 0);
  ASSERT_TRUE(left < right);
  return 0;
}

static int test_overlay_failed_falls_back() {
  InstVersion left;
  InstVersion right;
  std::string errtext;

  ASSERT_EQ(left.parseVersion("1.0", &errtext), BasicVersion::parsedOK);
  ASSERT_EQ(right.parseVersion("1.0", &errtext), BasicVersion::parsedOK);

  left.know_overlay = true;
  right.know_overlay = true;
  left.overlay_failed = true;
  right.overlay_failed = false;
  left.priority = 1;
  left.overlay_key = 1;
  right.priority = 100;
  right.overlay_key = 100;

  ASSERT_TRUE(InstVersion::compare(left, right) == 0);
  return 0;
}

static int test_mixed_inst_and_extended_compare() {
  InstVersion inst;
  ExtendedVersion ext;
  std::string errtext;

  ASSERT_EQ(inst.parseVersion("1.0", &errtext), BasicVersion::parsedOK);
  ASSERT_EQ(ext.parseVersion("1.0", &errtext), BasicVersion::parsedOK);

  inst.know_overlay = true;
  inst.overlay_failed = false;
  inst.priority = 10;
  inst.overlay_key = 3;
  ext.priority = 10;
  ext.overlay_key = 1;
  ASSERT_TRUE(inst > ext);
  ASSERT_TRUE(ext < inst);

  inst.know_overlay = false;
  ASSERT_TRUE(InstVersion::compare(inst, ext) == 0);
  ASSERT_TRUE(InstVersion::compare(ext, inst) == 0);
  return 0;
}

int main() {
  int failed = 0;
  if (test_default_metadata_flags() != 0) {
    failed++;
  }
  if (test_compare_falls_back_without_overlay() != 0) {
    failed++;
  }
  if (test_compare_uses_overlay_when_known() != 0) {
    failed++;
  }
  if (test_overlay_failed_falls_back() != 0) {
    failed++;
  }
  if (test_mixed_inst_and_extended_compare() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All instversion tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
