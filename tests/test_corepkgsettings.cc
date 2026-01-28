#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "corepkg/conf/corepkgsettings.h"
#include "coreqTk/parseerror.h"
#include "coreqrc/coreqrc.h"

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

static int test_use_expand_and_world_sets() {
  CoreqRc rc(COREQ_VARS_PREFIX);
  rc.read();
  ParseError pe(true);
  CorePkgSettings settings(&rc, &pe, false, false);

  const CorePkgSettings& csettings = settings;
  ASSERT_TRUE(csettings["NON_EXISTING_COREPKG_SETTINGS_KEY"].empty());
  ASSERT_TRUE(csettings.cstr("NON_EXISTING_COREPKG_SETTINGS_KEY") == NULLPTR);

  settings["USE_EXPAND"] = "FOO BAR";
  std::string var;
  std::string expvar;
  ASSERT_TRUE(settings.use_expand(&var, &expvar, "foo_value"));
  ASSERT_EQ(var, std::string("FOO"));
  ASSERT_EQ(expvar, std::string("value"));
  ASSERT_TRUE(!settings.use_expand(&var, &expvar, "no_separator"));

  WordVec world_sets;
  world_sets.push_back("");
  world_sets.push_back("alpha");
  world_sets.push_back("beta");
  settings.store_world_sets(&world_sets, true);

  const WordVec* saved = settings.get_world_sets();
  ASSERT_EQ(saved->size(), static_cast<WordVec::size_type>(2));
  ASSERT_EQ((*saved)[0], std::string("alpha"));
  ASSERT_EQ((*saved)[1], std::string("beta"));

  WordVec ignored_update;
  ignored_update.push_back("gamma");
  settings.store_world_sets(&ignored_update, false);
  saved = settings.get_world_sets();
  ASSERT_EQ(saved->size(), static_cast<WordVec::size_type>(2));
  ASSERT_EQ((*saved)[0], std::string("alpha"));
  return 0;
}

static int test_read_local_sets_and_recursive_add_name() {
  CoreqRc rc(COREQ_VARS_PREFIX);
  rc.read();
  ParseError pe(true);
  CorePkgSettings settings(&rc, &pe, false, false);

  char tmpdir_template[] = "/tmp/coreq-sets-XXXXXX";
  char* tmpdir = mkdtemp(tmpdir_template);
  ASSERT_TRUE(tmpdir != NULLPTR);

  const std::string base(tmpdir);
  const std::string set_a = base + "/set-a";
  const std::string set_b = base + "/set-b";

  ASSERT_TRUE(write_text_file(set_a, "@set-b\ncat/pkg\n"));
  ASSERT_TRUE(write_text_file(set_b, "cat/other\n"));

  settings.m_recurse_sets = true;
  WordVec dirs;
  dirs.push_back(base);
  settings.read_local_sets(dirs);

  SetsIndex index_a = static_cast<SetsIndex>(settings.set_names.size());
  SetsIndex index_b = static_cast<SetsIndex>(settings.set_names.size());
  for (SetsIndex i = 0; i < settings.set_names.size(); ++i) {
    if (settings.set_names[i] == "set-a") {
      index_a = i;
    }
    if (settings.set_names[i] == "set-b") {
      index_b = i;
    }
  }

  ASSERT_TRUE(index_a < settings.set_names.size());
  ASSERT_TRUE(index_b < settings.set_names.size());

  SetsList l;
  settings.add_name(&l, "set-a", true);
  ASSERT_TRUE(l.has(index_a));
  ASSERT_TRUE(l.has(index_b));

  ASSERT_TRUE(std::remove(set_a.c_str()) == 0);
  ASSERT_TRUE(std::remove(set_b.c_str()) == 0);
  ASSERT_TRUE(rmdir(base.c_str()) == 0);
  return 0;
}

int main() {
  CorePkgSettings::init_static();

  int failed = 0;
  if (test_use_expand_and_world_sets() != 0) {
    failed++;
  }
  if (test_read_local_sets_and_recursive_add_name() != 0) {
    failed++;
  }

  CorePkgSettings::free_static();

  if (failed == 0) {
    std::cout << "All corepkgsettings tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
