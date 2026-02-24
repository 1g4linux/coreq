#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include <sqlite3.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cache/coreqcache/coreqcache.h"
#include "cache/parse/parse.h"
#include "cache/sqlite/sqlite.h"
#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"
#include "corepkg/eapi.h"
#include "corepkg/package.h"
#include "corepkg/packagetree.h"
#include "corepkg/version.h"

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

static std::string g_last_error;

static void capture_error(const std::string& str) { g_last_error = str; }

static bool mkdir_single(const std::string& path) {
  if (::mkdir(path.c_str(), 0700) == 0) {
    return true;
  }
  return errno == EEXIST;
}

static bool mkdir_p(const std::string& path) {
  if (path.empty()) {
    return false;
  }
  if (path[0] != '/') {
    return false;
  }
  std::string cur;
  cur.reserve(path.size());
  for (std::string::size_type i = 0; i < path.size(); ++i) {
    cur.push_back(path[i]);
    if ((path[i] == '/') && (i != 0)) {
      if (!mkdir_single(cur.substr(0, cur.size() - 1))) {
        return false;
      }
    }
  }
  return mkdir_single(cur);
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

static int test_parse_reader_reads_ebuild_directory() {
  ParseCache cache;
  ASSERT_TRUE(cache.initialize("parse*"));

  char template_dir[] = "/tmp/coreq-parse-cache-XXXXXX";
  char* tmpdir = mkdtemp(template_dir);
  ASSERT_TRUE(tmpdir != NULLPTR);
  const std::string base(tmpdir);
  const std::string pkgdir(base + "/repo/cat/demo");
  ASSERT_TRUE(mkdir_p(pkgdir));

  const std::string ebuild_path(pkgdir + "/demo-1.0.ebuild");
  ASSERT_TRUE(write_text_file(
      ebuild_path,
      "EAPI=8\n"
      "KEYWORDS=\"~amd64\"\n"
      "SLOT=\"0\"\n"
      "IUSE=\"flag\"\n"
      "DESCRIPTION=\"demo package\"\n"
      "HOMEPAGE=\"https://example.invalid\"\n"
      "LICENSE=\"MIT\"\n"));

  cache.setScheme(base.c_str(), base.c_str(), "/repo");
  cache.setKey(1);
  cache.setErrorCallback(capture_error);
  g_last_error.clear();

  Category cat;
  ASSERT_TRUE(cache.readCategoryPrepare("cat"));
  ASSERT_TRUE(cache.readCategory(&cat));
  cache.readCategoryFinalize();

  ASSERT_TRUE(g_last_error.empty());
  Package* pkg = cat.findPackage("demo");
  ASSERT_TRUE(pkg != NULLPTR);
  ASSERT_EQ(pkg->category, std::string("cat"));
  ASSERT_EQ(pkg->name, std::string("demo"));
  ASSERT_EQ(pkg->size(), static_cast<Package::size_type>(1));
  ASSERT_EQ(pkg->homepage, std::string("https://example.invalid"));

  ASSERT_TRUE(std::remove(ebuild_path.c_str()) == 0);
  ASSERT_TRUE(::rmdir(pkgdir.c_str()) == 0);
  ASSERT_TRUE(::rmdir((base + "/repo/cat").c_str()) == 0);
  ASSERT_TRUE(::rmdir((base + "/repo").c_str()) == 0);
  ASSERT_TRUE(::rmdir(base.c_str()) == 0);
  return 0;
}

static bool create_sqlite_cache_file(const std::string& sqlite_file) {
  sqlite3* db = NULLPTR;
  if (sqlite3_open(sqlite_file.c_str(), &db) != SQLITE_OK) {
    sqlite3_close(db);
    return false;
  }
  const char* schema =
      "CREATE TABLE corepkg_packages ("
      "corepkg_package_key TEXT,"
      "BDEPEND TEXT,"
      "DEPEND TEXT,"
      "DESCRIPTION TEXT,"
      "EAPI TEXT,"
      "HOMEPAGE TEXT,"
      "IDEPEND TEXT,"
      "IUSE TEXT,"
      "KEYWORDS TEXT,"
      "LICENSE TEXT,"
      "PDEPEND TEXT,"
      "PROPERTIES TEXT,"
      "RDEPEND TEXT,"
      "REQUIRED_USE TEXT,"
      "RESTRICT TEXT,"
      "SLOT TEXT,"
      "SRC_URI TEXT"
      ");";
  if (sqlite3_exec(db, schema, NULLPTR, NULLPTR, NULLPTR) != SQLITE_OK) {
    sqlite3_close(db);
    return false;
  }
  const char* insert =
      "INSERT INTO corepkg_packages VALUES("
      "'cat/demo-2.0','','','sqlite pkg','8','https://sqlite.invalid','',"
      "'sqliteflag','amd64','BSD','', '', '', '', '', '0', 'https://src.invalid'"
      ");";
  const bool ok = (sqlite3_exec(db, insert, NULLPTR, NULLPTR, NULLPTR) == SQLITE_OK);
  sqlite3_close(db);
  return ok;
}

static int test_sqlite_reader_reads_category_data() {
  char template_dir[] = "/tmp/coreq-sqlite-cache-XXXXXX";
  char* tmpdir = mkdtemp(template_dir);
  ASSERT_TRUE(tmpdir != NULLPTR);
  const std::string base(tmpdir);
  const std::string dep_dir(base + "/var/cache/edb/dep");
  ASSERT_TRUE(mkdir_p(dep_dir));

  const std::string sqlite_file(dep_dir + "/repo.sqlite");
  ASSERT_TRUE(create_sqlite_cache_file(sqlite_file));

  SqliteCache cache;
  cache.setScheme(base.c_str(), "", "/repo");
  cache.setErrorCallback(capture_error);
  g_last_error.clear();

  PackageTree tree;
  tree.insert("cat");
  ASSERT_TRUE(cache.readCategories(&tree, NULLPTR, NULLPTR));
  ASSERT_TRUE(g_last_error.empty());

  Package* pkg = tree.findPackage("cat", "demo");
  ASSERT_TRUE(pkg != NULLPTR);
  ASSERT_EQ(pkg->desc, std::string("sqlite pkg"));
  ASSERT_EQ(pkg->size(), static_cast<Package::size_type>(1));

  ASSERT_TRUE(std::remove(sqlite_file.c_str()) == 0);
  ASSERT_TRUE(::rmdir(dep_dir.c_str()) == 0);
  ASSERT_TRUE(::rmdir((base + "/var/cache/edb").c_str()) == 0);
  ASSERT_TRUE(::rmdir((base + "/var/cache").c_str()) == 0);
  ASSERT_TRUE(::rmdir((base + "/var").c_str()) == 0);
  ASSERT_TRUE(::rmdir(base.c_str()) == 0);
  return 0;
}

static int test_coreqcache_reader_reports_missing_file() {
  char template_dir[] = "/tmp/coreq-coreqcache-XXXXXX";
  char* tmpdir = mkdtemp(template_dir);
  ASSERT_TRUE(tmpdir != NULLPTR);
  const std::string base(tmpdir);

  CoreqCache cache;
  ASSERT_TRUE(cache.initialize("coreq:/missing.coreq:*"));
  cache.setScheme(base.c_str(), "", "/unused");
  cache.setErrorCallback(capture_error);
  g_last_error.clear();

  PackageTree tree;
  tree.insert("cat");
  ASSERT_TRUE(!cache.readCategories(&tree, NULLPTR, NULLPTR));
  ASSERT_TRUE(g_last_error.find("cannot read cache file") != std::string::npos);
  ASSERT_TRUE(::rmdir(base.c_str()) == 0);
  return 0;
}

int main() {
  Category::init_static();
  Eapi::init_static();
  get_coreqrc(COREQ_VARS_PREFIX);

  int failed = 0;
  if (test_parse_reader_reads_ebuild_directory() != 0) {
    failed++;
  }
  if (test_sqlite_reader_reads_category_data() != 0) {
    failed++;
  }
  if (test_coreqcache_reader_reports_missing_file() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All cache reader tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
