#include <iostream>

#include "corepkg/depend.h"
#include "search/algorithms.h"
#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#endif
#define private public
#include "search/packagetest.h"
#undef private
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

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

static int test_name_maps() {
  ASSERT_EQ(PackageTest::name2field("name", true), PackageTest::NAME);
  ASSERT_EQ(PackageTest::name2field("CATEGORY/NAME", true), PackageTest::CATEGORY_NAME);
  ASSERT_EQ(PackageTest::name2field("ERROR", false), PackageTest::NONE);

  ASSERT_EQ(PackageTest::name2algorithm("exact"), PackageTest::ALGO_EXACT);
  ASSERT_EQ(PackageTest::name2algorithm("regex"), PackageTest::ALGO_REGEX);
  ASSERT_EQ(PackageTest::name2algorithm("unknown-algo"), PackageTest::ALGO_REGEX);
  return 0;
}

static int test_parse_field_specification() {
  PackageTest::MatchField or_field = PackageTest::NONE;
  PackageTest::MatchField and_field = PackageTest::NONE;
  PackageTest::MatchField not_field = PackageTest::NONE;

  PackageTest::parse_field_specification("|name&slot!homepage", &or_field, &and_field, &not_field);
  ASSERT_TRUE((or_field & PackageTest::NAME) != PackageTest::NONE);
  ASSERT_TRUE((and_field & PackageTest::SLOT) != PackageTest::NONE);
  ASSERT_TRUE((not_field & PackageTest::HOMEPAGE) != PackageTest::NONE);
  return 0;
}

static int test_calculate_needs_and_depend_toggle() {
  PackageTest pt(reinterpret_cast<VarDbPkg*>(0x1),
                 reinterpret_cast<CorePkgSettings*>(0x1),
                 reinterpret_cast<const PrintFormat*>(0x1),
                 reinterpret_cast<const SetStability*>(0x1),
                 reinterpret_cast<const DBHeader*>(0x1),
                 reinterpret_cast<const ParseError*>(0x1));

  const bool old_use_depend = Depend::use_depend;

  pt.field = PackageTest::DESCRIPTION | PackageTest::DEPSA;
  Depend::use_depend = false;
  pt.calculateNeeds();
  ASSERT_EQ(pt.need, PackageReader::DESCRIPTION);
  ASSERT_TRUE((pt.field & PackageTest::DEPSA) == PackageTest::NONE);

  pt.field = PackageTest::SET;
  pt.calculateNeeds();
  ASSERT_EQ(pt.need, PackageReader::VERSIONS);

  pt.field = PackageTest::NONE;
  pt.installed = true;
  pt.calculateNeeds();
  ASSERT_EQ(pt.need, PackageReader::NAME);

  Depend::use_depend = old_use_depend;
  return 0;
}

static int test_set_pattern_and_algorithm_selection() {
  PackageTest pt(reinterpret_cast<VarDbPkg*>(0x1),
                 reinterpret_cast<CorePkgSettings*>(0x1),
                 reinterpret_cast<const PrintFormat*>(0x1),
                 reinterpret_cast<const SetStability*>(0x1),
                 reinterpret_cast<const DBHeader*>(0x1),
                 reinterpret_cast<const ParseError*>(0x1));

  ASSERT_TRUE(pt.algorithm == NULLPTR);
  ASSERT_TRUE(pt.field == PackageTest::NONE);

  pt.setPattern("sys-apps/coreutils");
  ASSERT_TRUE(pt.know_pattern);
  ASSERT_TRUE(pt.algorithm != NULLPTR);
  ASSERT_TRUE(pt.field != PackageTest::NONE);

  pt.setAlgorithm(PackageTest::ALGO_EXACT);
  pt.algorithm->setString("coreq");
  ASSERT_TRUE((*(pt.algorithm))("coreq", NULLPTR));
  ASSERT_TRUE(!(*(pt.algorithm))("coreq-tools", NULLPTR));

  pt.setAlgorithm(PackageTest::ALGO_FUZZY);
  pt.algorithm->setString("coreq");
  ASSERT_TRUE((*(pt.algorithm))("coreq", NULLPTR));
  return 0;
}

int main() {
  get_coreqrc(COREQ_VARS_PREFIX);
  PackageTest::init_static();

  int failed = 0;
  if (test_name_maps() != 0) {
    failed++;
  }
  if (test_parse_field_specification() != 0) {
    failed++;
  }
  if (test_calculate_needs_and_depend_toggle() != 0) {
    failed++;
  }
  if (test_set_pattern_and_algorithm_selection() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All packagetest tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
