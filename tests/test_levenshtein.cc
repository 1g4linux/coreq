#include <iostream>

#include "search/levenshtein.h"

#define ASSERT_EQ(val1, val2)                                                    \
  if (!((val1) == (val2))) {                                                     \
    std::cerr << "Assertion failed: " << #val1 << " == " << #val2 << " ("       \
              << (val1) << " != " << (val2) << ") at " << __FILE__ << ":"       \
              << __LINE__ << std::endl;                                          \
    return 1;                                                                    \
  }

static int test_distance_basics() {
  ASSERT_EQ(get_levenshtein_distance("", ""), static_cast<Levenshtein>(0));
  ASSERT_EQ(get_levenshtein_distance("", "abc"), static_cast<Levenshtein>(3));
  ASSERT_EQ(get_levenshtein_distance("abc", ""), static_cast<Levenshtein>(3));
  ASSERT_EQ(get_levenshtein_distance("abc", "abc"), static_cast<Levenshtein>(0));
  ASSERT_EQ(get_levenshtein_distance("kitten", "sitting"),
            static_cast<Levenshtein>(3));
  return 0;
}

static int test_distance_is_symmetric() {
  const char* a = "portage";
  const char* b = "potrage";
  ASSERT_EQ(get_levenshtein_distance(a, b), get_levenshtein_distance(b, a));
  return 0;
}

static int test_distance_operations() {
  ASSERT_EQ(get_levenshtein_distance("abc", "ab"), static_cast<Levenshtein>(1));
  ASSERT_EQ(get_levenshtein_distance("ab", "abc"), static_cast<Levenshtein>(1));
  ASSERT_EQ(get_levenshtein_distance("abc", "axc"), static_cast<Levenshtein>(1));
  ASSERT_EQ(get_levenshtein_distance("gumbo", "gambol"),
            static_cast<Levenshtein>(2));
  return 0;
}

int main() {
  int failed = 0;
  if (test_distance_basics() != 0) {
    failed++;
  }
  if (test_distance_is_symmetric() != 0) {
    failed++;
  }
  if (test_distance_operations() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All levenshtein tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
