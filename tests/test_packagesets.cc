#include <iostream>

#include "corepkg/packagesets.h"

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

static int test_add_and_has() {
  SetsList sets;
  ASSERT_TRUE(!sets.has_system());
  ASSERT_TRUE(sets.add_system());
  ASSERT_TRUE(sets.has_system());
  ASSERT_TRUE(!sets.add_system());

  ASSERT_TRUE(sets.add(3));
  ASSERT_TRUE(sets.has(3));
  ASSERT_TRUE(!sets.add(3));
  ASSERT_TRUE(!sets.has(9));
  return 0;
}

static int test_merge_lists() {
  SetsList left;
  left.add(1);

  SetsList right(true);
  right.add(2);
  right.add(1);

  ASSERT_TRUE(left.add(right));
  ASSERT_TRUE(left.has_system());
  ASSERT_TRUE(left.has(1));
  ASSERT_TRUE(left.has(2));
  ASSERT_EQ(left.size(), static_cast<SetsList::size_type>(2));

  ASSERT_TRUE(!left.add(right));
  return 0;
}

static int test_clear_resets_system_flag() {
  SetsList sets(true);
  sets.add(42);
  sets.clear();
  ASSERT_TRUE(!sets.has_system());
  ASSERT_TRUE(!sets.has(42));
  ASSERT_EQ(sets.size(), static_cast<SetsList::size_type>(0));
  return 0;
}

int main() {
  int failed = 0;
  if (test_add_and_has() != 0) {
    failed++;
  }
  if (test_merge_lists() != 0) {
    failed++;
  }
  if (test_clear_resets_system_flag() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All packagesets tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
