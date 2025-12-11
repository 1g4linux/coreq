#include <iostream>
#include <string>

#include "coreqTk/ansicolor.h"

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

static int test_basic_parsing() {
  std::string err;
  AnsiColor color;

  ASSERT_TRUE(color.initcolor("", &err));
  ASSERT_EQ(color.asString(), std::string("\x1B[0m"));

  ASSERT_TRUE(color.initcolor("red", &err));
  ASSERT_EQ(color.asString(), std::string("\x1B[0;31m"));

  ASSERT_TRUE(color.initcolor("blue,1;underline", &err));
  ASSERT_EQ(color.asString(), std::string("\x1B[0;1;4;34m"));

  ASSERT_TRUE(color.initcolor("none", &err));
  ASSERT_TRUE(color.asString().empty());

  return 0;
}

static int test_scheme_selection_and_errors() {
  std::string err;
  AnsiColor color;
  const unsigned int old_scheme = AnsiColor::colorscheme;

  AnsiColor::colorscheme = 1;
  ASSERT_TRUE(color.initcolor("red|blue", &err));
  ASSERT_EQ(color.asString(), std::string("\x1B[0;34m"));

  AnsiColor::colorscheme = 0;
  ASSERT_TRUE(!color.initcolor("not-a-color", &err));
  ASSERT_TRUE(!err.empty());

  AnsiColor::colorscheme = old_scheme;
  return 0;
}

static int test_helpers() {
  AnsiColor color;
  ASSERT_EQ(color.red("x"), std::string("\x1B[31mx\x1B[0m"));
  ASSERT_EQ(color.green("x"), std::string("\x1B[32mx\x1B[0m"));
  ASSERT_EQ(color.yellow("x"), std::string("\x1B[33mx\x1B[0m"));
  ASSERT_EQ(color.blue("x"), std::string("\x1B[34mx\x1B[0m"));
  ASSERT_EQ(color.cyan("x"), std::string("\x1B[36mx\x1B[0m"));
  ASSERT_EQ(color.bold("x"), std::string("\x1B[1mx\x1B[0m"));
  return 0;
}

int main() {
  AnsiColor::init_static();

  int failed = 0;
  if (test_basic_parsing() != 0) {
    failed++;
  }
  if (test_scheme_selection_and_errors() != 0) {
    failed++;
  }
  if (test_helpers() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All ansicolor tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
