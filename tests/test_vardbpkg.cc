#include <sys/stat.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "corepkg/package.h"
#include "corepkg/vardbpkg.h"

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

static bool make_dir(const std::string& path) {
  if (mkdir(path.c_str(), 0755) == 0) {
    return true;
  }
  return errno == EEXIST;
}

static bool write_file(const std::string& path, const std::string& content) {
  std::ofstream out(path.c_str(), std::ios::out | std::ios::trunc);
  if (!out.is_open()) {
    return false;
  }
  out << content;
  return out.good();
}

static int test_vardbpkg_reads_installed_metadata() {
  char templ[] = "/tmp/coreq-vardb-XXXXXX";
  char* tmp = mkdtemp(templ);
  ASSERT_TRUE(tmp != NULLPTR);

  const std::string root(tmp);
  const std::string catdir = root + "/app-editors";
  const std::string pkgdir = catdir + "/vim-1.2.3";

  ASSERT_TRUE(make_dir(catdir));
  ASSERT_TRUE(make_dir(pkgdir));
  ASSERT_TRUE(write_file(pkgdir + "/SLOT", "0\n"));
  ASSERT_TRUE(write_file(pkgdir + "/IUSE", "+python -gtk\n"));
  ASSERT_TRUE(write_file(pkgdir + "/USE", "python wayland\n"));
  ASSERT_TRUE(write_file(pkgdir + "/repository", "gentoo\n"));
  ASSERT_TRUE(write_file(pkgdir + "/CONTENTS",
                         "dir /usr/share/vim\n"
                         "obj /usr/bin/vim deadbeef 1700000000\n"
                         "sym /usr/bin/vi -> /usr/bin/vim 1700000001\n"));

  VarDbPkg db(root + "/", true, true, false, false, false, false);
  Package pkg("app-editors", "vim");

  InstVec* installed = db.getInstalledVector(pkg);
  ASSERT_TRUE(installed != NULLPTR);
  ASSERT_EQ(installed->size(), static_cast<InstVec::size_type>(1));
  ASSERT_EQ(db.numInstalled(pkg), static_cast<InstVec::size_type>(1));

  std::string errtext;
  BasicVersion query;
  ASSERT_EQ(query.parseVersion("1.2.3", &errtext), BasicVersion::parsedOK);
  InstVersion* inst = NULLPTR;
  ASSERT_TRUE(db.isInstalled(pkg, &query, &inst));
  ASSERT_TRUE(inst != NULLPTR);

  ASSERT_TRUE(db.readSlot(pkg, inst));
  ASSERT_TRUE(inst->know_slot);
  ASSERT_EQ(inst->get_longslot(), std::string("0"));

  ASSERT_TRUE(db.readUse(pkg, inst));
  ASSERT_TRUE(inst->know_use);
  ASSERT_EQ(inst->inst_iuse.size(), static_cast<WordVec::size_type>(2));
  ASSERT_EQ(inst->inst_iuse[0], std::string("gtk"));
  ASSERT_EQ(inst->inst_iuse[1], std::string("python"));
  ASSERT_TRUE(inst->usedUse.count("python") == 1);
  ASSERT_TRUE(inst->usedUse.count("gtk") == 0);

  ASSERT_TRUE(db.readContents(pkg, inst));
  ASSERT_TRUE(inst->know_contents);
  ASSERT_EQ(inst->contents.size(),
            static_cast<std::vector<InstVersion::ContentsEntry>::size_type>(3));
  ASSERT_EQ(inst->contents[0].type, InstVersion::ContentsEntry::DIR_T);
  ASSERT_EQ(inst->contents[1].type, InstVersion::ContentsEntry::OBJ_T);
  ASSERT_EQ(inst->contents[2].type, InstVersion::ContentsEntry::SYM_T);
  ASSERT_EQ(inst->contents[2].target, std::string("/usr/bin/vim"));

  ASSERT_EQ(db.readOverlayLabel(&pkg, inst), std::string("gentoo"));
  return 0;
}

int main() {
  int failed = 0;
  if (test_vardbpkg_reads_installed_metadata() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All vardbpkg tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
