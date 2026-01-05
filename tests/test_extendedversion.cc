#include <iostream>
#include <string>

#include "corepkg/basicversion.h"
#include "corepkg/extendedversion.h"

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

static int test_slot_helpers() {
  ExtendedVersion v;

  v.set_slotname("0");
  ASSERT_EQ(v.get_shortfullslot(), std::string(""));
  ASSERT_EQ(v.get_longslot(), std::string("0"));
  ASSERT_EQ(v.get_longfullslot(), std::string("0"));

  v.set_slotname("2/3");
  ASSERT_EQ(v.get_shortfullslot(), std::string("2/3"));
  ASSERT_EQ(v.get_longslot(), std::string("2"));
  ASSERT_EQ(v.get_longfullslot(), std::string("2/3"));

  v.set_slotname("0/1");
  ASSERT_EQ(v.get_shortfullslot(), std::string("/1"));
  ASSERT_EQ(v.get_longslot(), std::string("0"));
  ASSERT_EQ(v.get_longfullslot(), std::string("0/1"));
  return 0;
}

static int test_restrict_and_properties() {
  ExtendedVersion::Restrict restrict_flags =
      ExtendedVersion::calcRestrict("fetch mirror unknown");
  ASSERT_TRUE((restrict_flags & ExtendedVersion::RESTRICT_FETCH) != 0);
  ASSERT_TRUE((restrict_flags & ExtendedVersion::RESTRICT_MIRROR) != 0);
  ASSERT_TRUE((restrict_flags & ExtendedVersion::RESTRICT_TEST) == 0);

  ExtendedVersion::Properties properties_flags =
      ExtendedVersion::calcProperties("interactive virtual unknown");
  ASSERT_TRUE((properties_flags & ExtendedVersion::PROPERTIES_INTERACTIVE) != 0);
  ASSERT_TRUE((properties_flags & ExtendedVersion::PROPERTIES_VIRTUAL) != 0);
  ASSERT_TRUE((properties_flags & ExtendedVersion::PROPERTIES_LIVE) == 0);
  return 0;
}

static int test_compare_prefers_basic_version() {
  ExtendedVersion left;
  ExtendedVersion right;
  std::string errtext;

  ASSERT_EQ(left.parseVersion("1.0", &errtext), BasicVersion::parsedOK);
  ASSERT_EQ(right.parseVersion("2.0", &errtext), BasicVersion::parsedOK);

  left.priority = 100;
  left.overlay_key = 100;
  right.priority = 0;
  right.overlay_key = 0;

  ASSERT_TRUE(left < right);
  ASSERT_TRUE(right > left);
  return 0;
}

static int test_compare_overlay_and_priority() {
  ExtendedVersion a;
  ExtendedVersion b;
  std::string errtext;
  ASSERT_EQ(a.parseVersion("1.0", &errtext), BasicVersion::parsedOK);
  ASSERT_EQ(b.parseVersion("1.0", &errtext), BasicVersion::parsedOK);

  a.overlay_key = 3;
  a.priority = 1;
  b.overlay_key = 3;
  b.priority = 100;
  ASSERT_TRUE(ExtendedVersion::compare(a, b) == 0);

  a.overlay_key = 1;
  a.priority = 50;
  b.overlay_key = 2;
  b.priority = 50;
  ASSERT_TRUE(a < b);

  a.overlay_key = 9;
  a.priority = 10;
  b.overlay_key = 1;
  b.priority = 20;
  ASSERT_TRUE(a < b);
  return 0;
}

int main() {
  ExtendedVersion::init_static();

  int failed = 0;
  if (test_slot_helpers() != 0) {
    failed++;
  }
  if (test_restrict_and_properties() != 0) {
    failed++;
  }
  if (test_compare_prefers_basic_version() != 0) {
    failed++;
  }
  if (test_compare_overlay_and_priority() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All extendedversion tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
