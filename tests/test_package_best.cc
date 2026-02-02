#include <iostream>
#include <string>
#include <vector>

#include "corepkg/package.h"
#include "corepkg/version.h"

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

static Version* make_version(const char* ver, const char* slot,
                             ExtendedVersion::Overlay overlay,
                             KeywordsFlags::KeyType keyflags) {
  Version* v = new Version();
  std::string err;
  if (v->parseVersion(ver, &err) != BasicVersion::parsedOK) {
    delete v;
    return NULLPTR;
  }
  v->set_slotname(slot);
  v->overlay_key = overlay;
  v->keyflags.set_keyflags(keyflags);
  return v;
}

static int test_best_and_best_slots_basic() {
  Package p("sys-apps", "coreq-best");
  Version* v1 = make_version("1.0", "0", 1, KeywordsFlags::KEY_STABLE);
  Version* v2 = make_version("2.0", "1", 1, KeywordsFlags::KEY_STABLE);
  ASSERT_TRUE(v1 != NULLPTR);
  ASSERT_TRUE(v2 != NULLPTR);
  p.addVersion(v1);
  p.addVersion(v2);

  Package::VerVec slots;
  p.best_slots(&slots);
  ASSERT_EQ(slots.size(), static_cast<Package::VerVec::size_type>(2));
  ASSERT_TRUE(p.best() == v2);
  ASSERT_TRUE(p.best_slot("") == v1);
  ASSERT_TRUE(p.best_slot("1") == v2);
  return 0;
}

static int test_allow_unstable_selection() {
  Package p("sys-apps", "coreq-unstable");
  Version* unstable = make_version("1.2", "0", 1, KeywordsFlags::KEY_ARCHUNSTABLE);
  ASSERT_TRUE(unstable != NULLPTR);
  p.addVersion(unstable);

  ASSERT_TRUE(p.best(false) == NULLPTR);
  ASSERT_TRUE(p.best(true) == unstable);
  ASSERT_TRUE(p.best_slot("", false) == NULLPTR);
  ASSERT_TRUE(p.best_slot("", true) == unstable);
  return 0;
}

static int test_compare_best_and_slots() {
  Package newer("sys-apps", "coreq-cmp");
  Package older("sys-apps", "coreq-cmp");

  Version* n0 = make_version("1.0", "0", 1, KeywordsFlags::KEY_STABLE);
  Version* n1 = make_version("2.0", "1", 1, KeywordsFlags::KEY_STABLE);
  Version* o0 = make_version("1.0", "0", 1, KeywordsFlags::KEY_STABLE);
  Version* o1 = make_version("1.5", "1", 1, KeywordsFlags::KEY_STABLE);
  ASSERT_TRUE(n0 && n1 && o0 && o1);

  newer.addVersion(n0);
  newer.addVersion(n1);
  older.addVersion(o0);
  older.addVersion(o1);

  ASSERT_EQ(newer.compare_best_slots(older), static_cast<coreq::TinySigned>(1));
  ASSERT_EQ(older.compare_best_slots(newer), static_cast<coreq::TinySigned>(-1));

  Package overlay_diff_a("sys-apps", "coreq-overlay");
  Package overlay_diff_b("sys-apps", "coreq-overlay");
  Version* oa = make_version("3.0", "0", 1, KeywordsFlags::KEY_STABLE);
  Version* ob = make_version("3.0", "0", 2, KeywordsFlags::KEY_STABLE);
  ASSERT_TRUE(oa && ob);
  overlay_diff_a.addVersion(oa);
  overlay_diff_b.addVersion(ob);

  const coreq::TinySigned ab = overlay_diff_a.compare_best(overlay_diff_b, false);
  const coreq::TinySigned ba = overlay_diff_b.compare_best(overlay_diff_a, false);
  ASSERT_TRUE((ab == 1 && ba == -1) || (ab == -1 && ba == 1));
  return 0;
}

static int test_check_best_without_installed_versions() {
  Package p("sys-apps", "coreq-check");
  Version* v = make_version("1.0", "0", 1, KeywordsFlags::KEY_STABLE);
  ASSERT_TRUE(v != NULLPTR);
  p.addVersion(v);

  ASSERT_EQ(p.check_best(NULLPTR, false, false), static_cast<coreq::TinySigned>(4));
  ASSERT_EQ(p.check_best(NULLPTR, true, false), static_cast<coreq::TinySigned>(0));
  ASSERT_EQ(p.check_best_slots(NULLPTR, false), static_cast<coreq::TinySigned>(4));
  ASSERT_EQ(p.check_best_slots(NULLPTR, true), static_cast<coreq::TinySigned>(0));
  return 0;
}

int main() {
  int failed = 0;
  if (test_best_and_best_slots_basic() != 0) {
    failed++;
  }
  if (test_allow_unstable_selection() != 0) {
    failed++;
  }
  if (test_compare_best_and_slots() != 0) {
    failed++;
  }
  if (test_check_best_without_installed_versions() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All package_best tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
