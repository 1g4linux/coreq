#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"

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

static void add_defaults(CoreqRc* rc) {
#ifdef JUMBO_BUILD
  fill_defaults(rc);
#else
  fill_defaults_part_1(rc);
  fill_defaults_part_2(rc);
  fill_defaults_part_3(rc);
  fill_defaults_part_4(rc);
  fill_defaults_part_5(rc);
  fill_defaults_part_6(rc);
#endif
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
  return std::fclose(fp) == 0;
}

static int test_defaults_are_loaded_and_resolved() {
  const std::string prefix = "/tmp/coreq-defaults-prefix";
  ASSERT_TRUE(setenv("COREQ_PREFIX", prefix.c_str(), 1) == 0);

  CoreqRc& rc = get_coreqrc(COREQ_VARS_PREFIX);

  ASSERT_TRUE(rc.cstr("COREQ_WORLD") != NULLPTR);
  ASSERT_TRUE(rc.cstr("COREQ_WORLD_SETS") != NULLPTR);

  ASSERT_EQ(rc["CURRENT_WORLD"], std::string("true"));
  ASSERT_EQ(rc["REMOTE_DEFAULT"], std::string("0"));
  ASSERT_EQ(rc["COREQ_WORLD_SETS"], rc["COREQ_WORLD"] + "_sets");

  const std::string& eprefix = rc["EPREFIX"];
  ASSERT_TRUE(eprefix.find(prefix) == 0);
  ASSERT_TRUE(rc["COREPKG_CONFIGROOT"].find(eprefix) == 0);

  ASSERT_TRUE(!rc["FORMAT_BEFORE_SET_USE"].empty());
  ASSERT_TRUE(!rc["FORMAT_AFTER_SET_USE"].empty());
  return 0;
}

static int test_coreqrc_file_and_source_loading_precedence() {
  char tmpdir_template[] = "/tmp/coreqrc-loading-XXXXXX";
  char* tmpdir = mkdtemp(tmpdir_template);
  ASSERT_TRUE(tmpdir != NULLPTR);

  const std::string base(tmpdir);
  const std::string main_rc(base + "/main.rc");
  const std::string sourced_rc(base + "/extras.rc");

  ASSERT_TRUE(write_text_file(main_rc,
                              "REMOTE_DEFAULT=1\n"
                              "QUIETMODE=false\n"
                              "source extras.rc\n"));
  ASSERT_TRUE(write_text_file(sourced_rc,
                              "REMOTE_DEFAULT=2\n"
                              "QUIETMODE=true\n"));

  ASSERT_TRUE(setenv("COREQRC", main_rc.c_str(), 1) == 0);
  ASSERT_TRUE(setenv("REMOTE_DEFAULT", "3", 1) == 0);
  ASSERT_TRUE(unsetenv("COREQ_PREFIX") == 0);
  ASSERT_TRUE(unsetenv("COREPKG_CONFIGROOT") == 0);

  CoreqRc rc(COREQ_VARS_PREFIX);
  add_defaults(&rc);
  rc.read();

  ASSERT_EQ(rc["REMOTE_DEFAULT"], std::string("3"));
  ASSERT_TRUE(rc.getBool("QUIETMODE"));

  ASSERT_TRUE(unsetenv("COREQRC") == 0);
  ASSERT_TRUE(unsetenv("REMOTE_DEFAULT") == 0);

  ASSERT_TRUE(std::remove(main_rc.c_str()) == 0);
  ASSERT_TRUE(std::remove(sourced_rc.c_str()) == 0);
  ASSERT_TRUE(::rmdir(base.c_str()) == 0);
  return 0;
}

int main() {
  int failed = 0;
  if (test_defaults_are_loaded_and_resolved() != 0) {
    failed++;
  }
  if (test_coreqrc_file_and_source_loading_precedence() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All coreqrc defaults tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
