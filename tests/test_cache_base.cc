#include <iostream>
#include <string>

#include "cache/base.h"
#include "corepkg/packagetree.h"

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

class DummyCache : public BasicCache {
 public:
  explicit DummyCache(bool use_prefixport_mode) : use_prefixport_mode_(use_prefixport_mode), read_called(false) {}

  bool use_prefixport() const override { return use_prefixport_mode_; }

  const char* getType() const override { return "dummy"; }

  bool readCategories(PackageTree*, const char* cat_name, Category*) override {
    read_called = true;
    seen_cat = (cat_name == NULLPTR) ? "" : cat_name;
    return true;
  }

  std::string current_catname() const { return m_catname; }

  bool use_prefixport_mode_;
  bool read_called;
  std::string seen_cat;
};

static int test_scheme_and_prefix_behavior() {
  DummyCache default_mode(false);
  default_mode.setScheme("/prefix", "/prefixport", "/metadata/md5-cache");
  ASSERT_EQ(default_mode.getPath(), std::string("/metadata/md5-cache"));
  ASSERT_EQ(default_mode.getPrefixedPath(),
            std::string("/prefix/metadata/md5-cache"));
  ASSERT_EQ(default_mode.getPathHumanReadable(),
            std::string("/metadata/md5-cache in /prefix"));

  DummyCache prefixport_mode(true);
  prefixport_mode.setScheme("/prefix", "/prefixport", "/metadata/md5-cache");
  ASSERT_EQ(prefixport_mode.getPrefixedPath(),
            std::string("/prefixport/metadata/md5-cache"));

  DummyCache without_prefix(false);
  without_prefix.setScheme(NULLPTR, "/ignored", "/cache/path");
  ASSERT_EQ(without_prefix.getPrefixedPath(), std::string("/cache/path"));
  ASSERT_EQ(without_prefix.getPathHumanReadable(), std::string("/cache/path"));
  return 0;
}

static int test_read_category_lifecycle() {
  DummyCache cache(false);
  Category category;
  ASSERT_TRUE(cache.readCategoryPrepare("sys-apps"));
  ASSERT_EQ(cache.current_catname(), std::string("sys-apps"));

  ASSERT_TRUE(cache.readCategory(&category));
  ASSERT_TRUE(cache.read_called);
  ASSERT_EQ(cache.seen_cat, std::string("sys-apps"));

  cache.readCategoryFinalize();
  ASSERT_EQ(cache.current_catname(), std::string(""));
  return 0;
}

int main() {
  int failed = 0;
  if (test_scheme_and_prefix_behavior() != 0) {
    failed++;
  }
  if (test_read_category_lifecycle() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All cache base tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
