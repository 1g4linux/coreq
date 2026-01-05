#include <iostream>
#include <string>

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

static int test_insert_and_counts() {
  WordVec cats;
  cats.PUSH_BACK("sys-apps");
  cats.PUSH_BACK("dev-libs");
  cats.PUSH_BACK("sys-apps");

  PackageTree tree(cats);
  ASSERT_EQ(tree.countCategories(), static_cast<coreq::Catsize>(2));
  ASSERT_EQ(tree.countPackages(), static_cast<coreq::Treesize>(0));

  tree["sys-apps"].addPackage("sys-apps", "portage");
  tree["sys-apps"].addPackage("sys-apps", "coreutils");
  tree["dev-libs"].addPackage("dev-libs", "openssl");
  ASSERT_EQ(tree.countPackages(), static_cast<coreq::Treesize>(3));
  return 0;
}

static int test_find_category_and_package() {
  PackageTree tree;
  tree.insert("app-editors");
  tree["app-editors"].addPackage("app-editors", "vim");

  Category* cat = tree.find("app-editors");
  ASSERT_TRUE(cat != NULLPTR);
  ASSERT_TRUE(tree.find("does-not-exist") == NULLPTR);

  Package* vim = tree.findPackage("app-editors", "vim");
  ASSERT_TRUE(vim != NULLPTR);
  ASSERT_EQ(vim->category, std::string("app-editors"));
  ASSERT_EQ(vim->name, std::string("vim"));

  ASSERT_TRUE(tree.findPackage("app-editors", "nano") == NULLPTR);
  ASSERT_TRUE(tree.findPackage("unknown-cat", "vim") == NULLPTR);
  return 0;
}

int main() {
  Category::init_static();

  int failed = 0;
  if (test_insert_and_counts() != 0) {
    failed++;
  }
  if (test_find_category_and_package() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All packagetree tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
