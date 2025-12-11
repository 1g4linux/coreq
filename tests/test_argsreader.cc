#include <iostream>
#include <string>
#include <vector>

#include "coreqTk/argsreader.h"

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

static int test_basic_folding() {
  bool verbose = false;
  int count = 0;
  const char* name = "";

  OptionList options;
  options.PUSH_BACK(Option("verbose", 'v', Option::BOOLEAN_T, &verbose));
  options.PUSH_BACK(Option("count", 'c', Option::INTEGER, &count));
  options.PUSH_BACK(Option("name", 'n', Option::STRING, &name));

  const char* argv[] = {"prog", "-v", "--count", "--name=alice", "tail"};
  ArgumentReader ar(static_cast<int>(sizeof(argv) / sizeof(argv[0])), argv, options);

  ASSERT_TRUE(verbose);
  ASSERT_EQ(count, 1);
  ASSERT_EQ(std::string(name), std::string("alice"));
  ASSERT_EQ(ar.size(), static_cast<ArgumentReader::size_type>(1));
  ASSERT_EQ(ar[0].type, Parameter::ARGUMENT);
  ASSERT_EQ(std::string(ar[0].m_argument), std::string("tail"));
  return 0;
}

static int test_keep_and_lists() {
  const char* opt_arg = "";
  std::vector<const char*> strings;
  std::vector<ArgPair> pairs;

  OptionList options;
  options.PUSH_BACK(Option("keep", 'k'));
  options.PUSH_BACK(Option("list", 'l', Option::STRINGLIST, &strings));
  options.PUSH_BACK(Option("pair", 'p', Option::PAIRLIST, &pairs));
  options.PUSH_BACK(Option("opt", 'o', Option::STRING_OPTIONAL, &opt_arg));

  const char* argv[] = {"prog", "--keep", "--list=a", "--list", "b", "--pair", "x", "y", "--opt", "value", "tail"};
  ArgumentReader ar(static_cast<int>(sizeof(argv) / sizeof(argv[0])), argv, options);

  ASSERT_EQ(strings.size(), static_cast<std::vector<const char*>::size_type>(2));
  ASSERT_EQ(std::string(strings[0]), std::string("a"));
  ASSERT_EQ(std::string(strings[1]), std::string("b"));
  ASSERT_EQ(pairs.size(), static_cast<std::vector<ArgPair>::size_type>(1));
  ASSERT_EQ(std::string(pairs[0].first), std::string("x"));
  ASSERT_EQ(std::string(pairs[0].second), std::string("y"));
  ASSERT_EQ(std::string(opt_arg), std::string("value"));

  ASSERT_EQ(ar.size(), static_cast<ArgumentReader::size_type>(2));
  ASSERT_EQ(ar[0].type, Parameter::OPTION);
  ASSERT_EQ(*ar[0], 'k');
  ASSERT_EQ(ar[1].type, Parameter::ARGUMENT);
  ASSERT_EQ(std::string(ar[1].m_argument), std::string("tail"));
  return 0;
}

int main() {
  int failed = 0;
  if (test_basic_folding() != 0) {
    failed++;
  }
  if (test_keep_and_lists() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All argsreader tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
