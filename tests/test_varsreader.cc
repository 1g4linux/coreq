#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

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

static bool write_text_file(const std::string& path, const std::string& text) {
  FILE* fp = std::fopen(path.c_str(), "wb");
  if (fp == NULLPTR) {
    return false;
  }
  if (std::fwrite(text.data(), sizeof(char), text.size(), fp) != text.size()) {
    std::fclose(fp);
    return false;
  }
  return (std::fclose(fp) == 0);
}

static int test_corepkg_sections_and_references() {
  const std::string input(
      "[DEFAULT]\n"
      "BASE = root\n"
      "[repo one]\n"
      "location = /gentoo\n"
      "merged = %(location)s/%(BASE)s\n"
      "escaped = 'a\n"
      "b'\n");

  VarsReader reader(VarsReader::SUBST_VARS | VarsReader::COREPKG_SECTIONS);
  std::string errtext;
  ASSERT_TRUE(reader.readmem(input.c_str(), NULLPTR, &errtext));
  ASSERT_TRUE(errtext.empty());

  const std::string* base = reader.find("BASE");
  ASSERT_TRUE(base != NULLPTR);
  ASSERT_EQ(*base, std::string("root"));

  const std::string* location = reader.find("location:repo one");
  ASSERT_TRUE(location != NULLPTR);
  ASSERT_EQ(*location, std::string("/gentoo"));

  const std::string* merged = reader.find("merged:repo one");
  ASSERT_TRUE(merged != NULLPTR);
  ASSERT_EQ(*merged, std::string("/gentoo/root"));

  const std::string* escaped = reader.find("escaped:repo one");
  ASSERT_TRUE(escaped != NULLPTR);
  ASSERT_EQ(*escaped, std::string("a b"));
  return 0;
}

static int test_source_and_source_varname_prefix() {
  char tmpdir_template[] = "/tmp/coreq-varsreader-XXXXXX";
  char* tmpdir = mkdtemp(tmpdir_template);
  ASSERT_TRUE(tmpdir != NULLPTR);
  const std::string include_file = std::string(tmpdir) + "/include.conf";
  const std::string main_file = std::string(tmpdir) + "/main.conf";
  const std::string main_var_file = std::string(tmpdir) + "/main-var.conf";
  ASSERT_TRUE(write_text_file(include_file, "FROM_INCLUDE=1\n"));
  ASSERT_TRUE(write_text_file(main_file, ". include.conf\nMAIN=ok\n"));
  ASSERT_TRUE(write_text_file(main_var_file, ". include.conf\n"));

  {
    VarsReader reader(VarsReader::ALLOW_SOURCE);
    reader.setPrefix(tmpdir);
    std::string errtext;
    ASSERT_TRUE(reader.read(main_file.c_str(), &errtext, false));
    ASSERT_TRUE(errtext.empty());
    const std::string* from_include = reader.find("FROM_INCLUDE");
    ASSERT_TRUE(from_include != NULLPTR);
    ASSERT_EQ(*from_include, std::string("1"));
    ASSERT_EQ(*reader.find("MAIN"), std::string("ok"));
  }

  {
    VarsReader reader(VarsReader::ALLOW_SOURCE_VARNAME);
    reader.setPrefix("SRCROOT");
    reader["SRCROOT"] = tmpdir;
    std::string errtext;
    ASSERT_TRUE(reader.read(main_var_file.c_str(), &errtext, false));
    ASSERT_TRUE(errtext.empty());
    const std::string* from_include = reader.find("FROM_INCLUDE");
    ASSERT_TRUE(from_include != NULLPTR);
    ASSERT_EQ(*from_include, std::string("1"));
  }

  ASSERT_TRUE(std::remove(main_file.c_str()) == 0);
  ASSERT_TRUE(std::remove(main_var_file.c_str()) == 0);
  ASSERT_TRUE(std::remove(include_file.c_str()) == 0);
  ASSERT_TRUE(rmdir(tmpdir) == 0);
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
  if (test_corepkg_sections_and_references() != 0) {
    failed++;
  }
  if (test_source_and_source_varname_prefix() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All varsreader tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
