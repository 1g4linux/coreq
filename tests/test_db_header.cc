#include <iostream>
#include <set>
#include <string>

#include "database/header.h"

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

static int test_overlay_lookup_by_label_and_number() {
  DBHeader header;
  header.addOverlay(OverlayIdent("/var/db/repos/gentoo", "gentoo"));
  header.addOverlay(OverlayIdent("/var/db/repos/local", "local"));
  ASSERT_EQ(header.countOverlays(), static_cast<ExtendedVersion::Overlay>(2));

  ExtendedVersion::Overlay idx = 99;
  ASSERT_TRUE(header.find_overlay(&idx, "local", NULLPTR, 0, DBHeader::OVTEST_LABEL));
  ASSERT_EQ(idx, static_cast<ExtendedVersion::Overlay>(1));

  idx = 99;
  ASSERT_TRUE(header.find_overlay(&idx, "1", NULLPTR, 0, DBHeader::OVTEST_NUMBER));
  ASSERT_EQ(idx, static_cast<ExtendedVersion::Overlay>(1));

  idx = 99;
  ASSERT_TRUE(!header.find_overlay(&idx, "3", NULLPTR, 0, DBHeader::OVTEST_NUMBER));
  return 0;
}

static int test_overlay_vector_and_empty_name() {
  DBHeader header;
  header.addOverlay(OverlayIdent("/var/db/repos/gentoo", "gentoo"));
  header.addOverlay(OverlayIdent("/var/db/repos/local", "local"));
  header.addOverlay(OverlayIdent("/var/db/repos/extra", "extra"));

  ExtendedVersion::Overlay idx = 99;
  ASSERT_TRUE(header.find_overlay(&idx, "", NULLPTR, 0, DBHeader::OVTEST_ALL));
  ASSERT_EQ(idx, static_cast<ExtendedVersion::Overlay>(1));

  std::set<ExtendedVersion::Overlay> found;
  header.get_overlay_vector(&found, "extra", NULLPTR, 0, DBHeader::OVTEST_LABEL);
  ASSERT_EQ(found.size(), static_cast<std::set<ExtendedVersion::Overlay>::size_type>(1));
  ASSERT_TRUE(found.count(2) == 1);
  return 0;
}

static int test_header_version_acceptance() {
  DBHeader header;
  header.version = DBHeader::current;
  ASSERT_TRUE(header.isCurrent());

  header.version = 0;
  ASSERT_TRUE(!header.isCurrent());
  return 0;
}

int main() {
  int failed = 0;
  if (test_overlay_lookup_by_label_and_number() != 0) {
    failed++;
  }
  if (test_overlay_vector_and_empty_name() != 0) {
    failed++;
  }
  if (test_header_version_acceptance() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All database header tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
