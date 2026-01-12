#include <iostream>

#include "search/matchtree.h"

#define ASSERT_TRUE(condition)                                                   \
  if (!(condition)) {                                                            \
    std::cerr << "Assertion failed: " << #condition << " at " << __FILE__       \
              << ":" << __LINE__ << std::endl;                                   \
    return 1;                                                                    \
  }

static int test_empty_tree_matches() {
  MatchTree t(false);
  t.end_parse();
  ASSERT_TRUE(t.match(NULLPTR));
  return 0;
}

static int test_negation_and_and_or() {
  MatchTree and_tree(false);
  and_tree.parse_negate();
  and_tree.parse_test(NULLPTR, false);  // !true => false
  and_tree.parse_and();
  and_tree.parse_test(NULLPTR, false);  // true
  and_tree.end_parse();
  ASSERT_TRUE(!and_tree.match(NULLPTR));

  MatchTree or_tree(false);
  or_tree.parse_negate();
  or_tree.parse_test(NULLPTR, false);  // !true => false
  or_tree.parse_or();
  or_tree.parse_test(NULLPTR, false);  // true
  or_tree.end_parse();
  ASSERT_TRUE(or_tree.match(NULLPTR));
  return 0;
}

static int test_braces_and_default_operator() {
  MatchTree t(false);  // default AND
  t.parse_negate();
  t.parse_open();
  t.parse_test(NULLPTR, false);  // -( true ) => false
  t.parse_close();
  t.end_parse();
  ASSERT_TRUE(!t.match(NULLPTR));

  MatchTree default_and(false);
  default_and.parse_test(NULLPTR, false);
  default_and.parse_test(NULLPTR, false);  // implicit AND
  default_and.end_parse();
  ASSERT_TRUE(default_and.match(NULLPTR));
  return 0;
}

static int test_pipetest() {
  MatchTree t(false);
  t.set_pipetest(NULLPTR);      // pipetest is always true
  t.parse_test(NULLPTR, true);  // leaf also true
  t.end_parse();
  ASSERT_TRUE(t.match(NULLPTR));
  return 0;
}

int main() {
  int failed = 0;
  if (test_empty_tree_matches() != 0) {
    failed++;
  }
  if (test_negation_and_and_or() != 0) {
    failed++;
  }
  if (test_braces_and_default_operator() != 0) {
    failed++;
  }
  if (test_pipetest() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All matchtree tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
