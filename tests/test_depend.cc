#include <iostream>
#include <string>

#include "corepkg/depend.h"

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

static int test_expand_and_brief_views() {
  Depend d;
  d.set("a/b", "  a/b   c/d ", "a/b e/f", "a/b g/h", "a/b i/j", true);

  ASSERT_EQ(d.get_depend(), std::string("a/b"));
  ASSERT_EQ(d.get_rdepend(), std::string("a/b c/d"));
  ASSERT_EQ(d.get_pdepend(), std::string("a/b e/f"));
  ASSERT_EQ(d.get_bdepend(), std::string("a/b g/h"));
  ASSERT_EQ(d.get_idepend(), std::string("a/b i/j"));

  ASSERT_EQ(d.get_rdepend_brief(), std::string("${DEPEND} c/d"));
  ASSERT_EQ(d.get_pdepend_brief(), std::string("${DEPEND} e/f"));
  ASSERT_EQ(d.get_bdepend_brief(), std::string("${DEPEND} g/h"));
  ASSERT_EQ(d.get_idepend_brief(), std::string("${DEPEND} i/j"));
  return 0;
}

static int test_shortcut_boundary_checks() {
  Depend d;
  d.set("cat/pkg", "xcat/pkg y", "", "", "", true);

  ASSERT_EQ(d.get_rdepend(), std::string("xcat/pkg y"));
  ASSERT_EQ(d.get_rdepend_brief(), std::string("xcat/pkg y"));
  return 0;
}

static int test_equality_uses_expanded_semantics() {
  Depend left;
  Depend right;

  left.set("a/b", " a/b   c/d ", "a/b e/f", "a/b g/h", "a/b i/j", true);
  right.set("a/b", "a/b c/d", "a/b e/f", "a/b g/h", "a/b i/j", true);

  ASSERT_TRUE(left == right);
  ASSERT_TRUE(!(left != right));
  return 0;
}

static int test_clear_and_empty_flags() {
  Depend d;
  ASSERT_TRUE(d.empty());
  ASSERT_TRUE(d.depend_empty());
  ASSERT_TRUE(d.rdepend_empty());
  ASSERT_TRUE(d.pdepend_empty());
  ASSERT_TRUE(d.bdepend_empty());
  ASSERT_TRUE(d.idepend_empty());

  d.set("a/b", "", "", "", "", true);
  ASSERT_TRUE(!d.empty());
  ASSERT_TRUE(!d.depend_empty());

  d.clear();
  ASSERT_TRUE(d.empty());
  ASSERT_TRUE(d.depend_empty());
  ASSERT_TRUE(d.rdepend_empty());
  ASSERT_TRUE(d.pdepend_empty());
  ASSERT_TRUE(d.bdepend_empty());
  ASSERT_TRUE(d.idepend_empty());
  return 0;
}

int main() {
  int failed = 0;
  if (test_expand_and_brief_views() != 0) {
    failed++;
  }
  if (test_shortcut_boundary_checks() != 0) {
    failed++;
  }
  if (test_equality_uses_expanded_semantics() != 0) {
    failed++;
  }
  if (test_clear_and_empty_flags() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All depend tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
