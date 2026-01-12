#include <iostream>
#include <string>

#include "corepkg/package.h"
#include "search/algorithms.h"

#define ASSERT_TRUE(condition)                                                   \
  if (!(condition)) {                                                            \
    std::cerr << "Assertion failed: " << #condition << " at " << __FILE__       \
              << ":" << __LINE__ << std::endl;                                   \
    return 1;                                                                    \
  }

static int test_exact_begin_end_pattern() {
  ExactAlgorithm exact;
  exact.setString("dev-libs/openssl");
  ASSERT_TRUE(exact("dev-libs/openssl", NULLPTR));
  ASSERT_TRUE(!exact("dev-libs/libressl", NULLPTR));

  BeginAlgorithm begin;
  begin.setString("dev-");
  ASSERT_TRUE(begin("dev-libs/openssl", NULLPTR));
  ASSERT_TRUE(!begin("sys-apps/portage", NULLPTR));

  EndAlgorithm end;
  end.setString("openssl");
  ASSERT_TRUE(end("dev-libs/openssl", NULLPTR));
  ASSERT_TRUE(!end("dev-libs/libressl", NULLPTR));

  PatternAlgorithm pattern;
  pattern.setString("dev-*/*ssl*");
  ASSERT_TRUE(pattern("dev-libs/openssl", NULLPTR));
  ASSERT_TRUE(!pattern("sys-apps/portage", NULLPTR));
  return 0;
}

static int test_substring_with_simplify() {
  SubstringAlgorithm sub;
  BaseAlgorithm& base = sub;
  sub.setString("$$dev-libs/openssl##");
  ASSERT_TRUE(base("dev-libs/openssl", NULLPTR, true));
  ASSERT_TRUE(!base("sys-apps/portage", NULLPTR, true));
  return 0;
}

static int test_fuzzy_threshold_and_compare() {
  FuzzyAlgorithm::init_static();

  FuzzyAlgorithm fuzzy(2);
  fuzzy.setString("portage");
  ASSERT_TRUE(fuzzy("portag", NULLPTR));
  ASSERT_TRUE(!fuzzy("xyz", NULLPTR));

  Package p1("sys-apps", "portage");
  Package p2("sys-apps", "porto");
  ASSERT_TRUE(fuzzy("portage", &p1));  // distance 0
  ASSERT_TRUE(fuzzy("portag", &p2));   // distance 1
  ASSERT_TRUE(FuzzyAlgorithm::compare(&p1, &p2));
  ASSERT_TRUE(!FuzzyAlgorithm::compare(&p2, &p1));
  return 0;
}

int main() {
  int failed = 0;
  if (test_exact_begin_end_pattern() != 0) {
    failed++;
  }
  if (test_substring_with_simplify() != 0) {
    failed++;
  }
  if (test_fuzzy_threshold_and_compare() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All algorithms tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
