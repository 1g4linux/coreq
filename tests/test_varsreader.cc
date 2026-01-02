#include <iostream>
#include <string>

#include "coreqTk/varsreader.h"

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

static int test_basic_shell_like_parsing() {
  const std::string input(
      "# comment line\n"
      "FOO=bar\n"
      "NO_ASSIGNMENT\n"
      "BAR=\"quoted value\" # trailing comment\n"
      "BAZ='single quoted'\n");

  VarsReader reader(VarsReader::NONE);
  std::string errtext;
  ASSERT_TRUE(reader.readmem(input.c_str(), NULLPTR, &errtext));
  ASSERT_TRUE(errtext.empty());

  const std::string* foo = reader.find("FOO");
  ASSERT_TRUE(foo != NULLPTR);
  ASSERT_EQ(*foo, std::string("bar"));

  const std::string* bar = reader.find("BAR");
  ASSERT_TRUE(bar != NULLPTR);
  ASSERT_EQ(*bar, std::string("quoted value"));

  const std::string* baz = reader.find("BAZ");
  ASSERT_TRUE(baz != NULLPTR);
  ASSERT_EQ(*baz, std::string("single quoted"));

  ASSERT_TRUE(reader.find("NO_ASSIGNMENT") == NULLPTR);
  return 0;
}

static int test_subst_and_escapes() {
  const std::string input(
      "BASE=core\n"
      "REF=$BASE\n"
      "BRACED=${BASE}\n"
      "SINGLE='$BASE'\n"
      "CONT=one\\\n"
      "two\n"
      "DQ=\"x\\\n"
      "y\"\n");

  VarsReader reader(VarsReader::SUBST_VARS);
  std::string errtext;
  ASSERT_TRUE(reader.readmem(input.c_str(), NULLPTR, &errtext));
  ASSERT_TRUE(errtext.empty());

  const std::string* ref = reader.find("REF");
  ASSERT_TRUE(ref != NULLPTR);
  ASSERT_EQ(*ref, std::string("core"));

  const std::string* braced = reader.find("BRACED");
  ASSERT_TRUE(braced != NULLPTR);
  ASSERT_EQ(*braced, std::string("core"));

  const std::string* single = reader.find("SINGLE");
  ASSERT_TRUE(single != NULLPTR);
  ASSERT_EQ(*single, std::string("$BASE"));

  const std::string* cont = reader.find("CONT");
  ASSERT_TRUE(cont != NULLPTR);
  ASSERT_EQ(*cont, std::string("onetwo"));

  const std::string* dq = reader.find("DQ");
  ASSERT_TRUE(dq != NULLPTR);
  ASSERT_EQ(*dq, std::string("xy"));
  return 0;
}

static int test_append_values_for_incremental_keys() {
  const std::string input(
      "USE=new\n"
      "NON=next\n");

  VarsReader reader(VarsReader::APPEND_VALUES);
  const char* const incremental_keys[] = {"USE", NULLPTR};
  reader.accumulatingKeys(incremental_keys);
  reader["USE"] = "old";
  reader["NON"] = "persist";

  std::string errtext;
  ASSERT_TRUE(reader.readmem(input.c_str(), NULLPTR, &errtext));
  ASSERT_TRUE(errtext.empty());

  const std::string* use = reader.find("USE");
  ASSERT_TRUE(use != NULLPTR);
  ASSERT_EQ(*use, std::string("old\nnew"));

  const std::string* non = reader.find("NON");
  ASSERT_TRUE(non != NULLPTR);
  ASSERT_EQ(*non, std::string("next"));
  return 0;
}

static int test_only_keywords_slot_stops_early() {
  const std::string input(
      "KEYWORDS=~amd64\n"
      "OTHER=before\n"
      "SLOT=0\n"
      "AFTER=ignored\n");

  VarsReader reader(VarsReader::ONLY_KEYWORDS_SLOT);
  std::string errtext;
  ASSERT_TRUE(reader.readmem(input.c_str(), NULLPTR, &errtext));
  ASSERT_TRUE(errtext.empty());

  const std::string* keywords = reader.find("KEYWORDS");
  ASSERT_TRUE(keywords != NULLPTR);
  ASSERT_EQ(*keywords, std::string("~amd64"));

  const std::string* slot = reader.find("SLOT");
  ASSERT_TRUE(slot != NULLPTR);
  ASSERT_EQ(*slot, std::string("0"));

  const std::string* other = reader.find("OTHER");
  ASSERT_TRUE(other != NULLPTR);
  ASSERT_EQ(*other, std::string("before"));

  ASSERT_TRUE(reader.find("AFTER") == NULLPTR);
  return 0;
}

int main() {
  int failed = 0;
  if (test_basic_shell_like_parsing() != 0) {
    failed++;
  }
  if (test_subst_and_escapes() != 0) {
    failed++;
  }
  if (test_append_values_for_incremental_keys() != 0) {
    failed++;
  }
  if (test_only_keywords_slot_stops_early() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All varsreader tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
