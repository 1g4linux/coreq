#include <iostream>
#include <string>

#include "corepkg/keywords.h"

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

static int test_keyflags_arch_and_masks() {
  WordSet accepted;
  accepted.INSERT("amd64");

  KeywordsFlags::KeyType stable =
      KeywordsFlags::get_keyflags(accepted, "amd64");
  ASSERT_TRUE((stable & KeywordsFlags::KEY_STABLE) != 0);
  ASSERT_TRUE((stable & KeywordsFlags::KEY_ARCHSTABLE) != 0);

  KeywordsFlags::KeyType unstable =
      KeywordsFlags::get_keyflags(accepted, "~amd64");
  ASSERT_TRUE((unstable & KeywordsFlags::KEY_ARCHUNSTABLE) != 0);
  ASSERT_TRUE((unstable & KeywordsFlags::KEY_STABLE) == 0);

  KeywordsFlags::KeyType minus_keyword =
      KeywordsFlags::get_keyflags(accepted, "-amd64");
  ASSERT_TRUE((minus_keyword & KeywordsFlags::KEY_MINUSKEYWORD) != 0);

  KeywordsFlags::KeyType minus_all =
      KeywordsFlags::get_keyflags(accepted, "-* -~*");
  ASSERT_TRUE((minus_all & KeywordsFlags::KEY_MINUSASTERISK) != 0);
  ASSERT_TRUE((minus_all & KeywordsFlags::KEY_MINUSUNSTABLE) != 0);
  return 0;
}

static int test_keyflags_wildcard_accept() {
  WordSet accepted;
  accepted.INSERT("~*");

  KeywordsFlags::KeyType flags =
      KeywordsFlags::get_keyflags(accepted, "~arm64");
  ASSERT_TRUE((flags & KeywordsFlags::KEY_ALIENUNSTABLE) != 0);
  ASSERT_TRUE((flags & KeywordsFlags::KEY_STABLE) != 0);
  return 0;
}

static int test_modify_keywords() {
  std::string result("untouched");
  ASSERT_TRUE(!Keywords::modify_keywords(&result, "amd64 ~arm64", ""));
  ASSERT_EQ(result, std::string("untouched"));

  ASSERT_TRUE(
      Keywords::modify_keywords(&result, "amd64 ~arm64", "-amd64 ~amd64 ~arm64"));
  ASSERT_EQ(result, std::string("~arm64 ~amd64"));
  return 0;
}

static int test_keywordsave_roundtrip() {
  Keywords original;
  original.keyflags.set_keyflags(
      KeywordsFlags::KEY_ARCHSTABLE | KeywordsFlags::KEY_STABLE);
  original.maskflags.set(MaskFlags::MASK_PROFILE);

  KeywordSave save(&original);

  Keywords mutated;
  mutated.keyflags.set_keyflags(KeywordsFlags::KEY_EMPTY);
  mutated.maskflags.set(MaskFlags::MASK_NONE);
  save.restore(&mutated);

  ASSERT_TRUE(mutated.keyflags == original.keyflags);
  ASSERT_TRUE(mutated.maskflags == original.maskflags);
  return 0;
}

int main() {
  int failed = 0;
  if (test_keyflags_arch_and_masks() != 0) {
    failed++;
  }
  if (test_keyflags_wildcard_accept() != 0) {
    failed++;
  }
  if (test_modify_keywords() != 0) {
    failed++;
  }
  if (test_keywordsave_roundtrip() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All keywords tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
