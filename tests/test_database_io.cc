#include <cstdio>
#include <iostream>
#include <string>

#include <unistd.h>

#include "database/io.h"
#include "database/package_reader.h"
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

static Version* make_version(const char* ver, const char* slot,
                             ExtendedVersion::Overlay overlay) {
  Version* v = new Version();
  std::string err;
  if (v->parseVersion(ver, &err) != BasicVersion::parsedOK) {
    delete v;
    return NULLPTR;
  }
  v->set_slotname(slot);
  v->overlay_key = overlay;
  v->set_full_keywords("amd64");
  v->eapi.assign("8");
  return v;
}

static bool build_sample_tree(PackageTree* tree) {
  Package* alpha = (*tree)["cat-a"].addPackage("cat-a", "alpha");
  alpha->desc = "alpha description";
  alpha->homepage = "https://alpha.invalid";
  alpha->licenses = "MIT";
  Version* a1 = make_version("1.0", "0", 0);
  if (a1 == NULLPTR) {
    return false;
  }
  alpha->addVersion(a1);

  Package* beta = (*tree)["cat-b"].addPackage("cat-b", "beta");
  beta->desc = "beta description";
  beta->homepage = "https://beta.invalid";
  beta->licenses = "BSD";
  Version* b1 = make_version("2.0", "1", 1);
  if (b1 == NULLPTR) {
    return false;
  }
  beta->addVersion(b1);
  return true;
}

static int test_database_roundtrip_and_package_reader_iteration() {
  char tmp_template[] = "/tmp/coreq-db-io-XXXXXX";
  char* tmpdir = mkdtemp(tmp_template);
  ASSERT_TRUE(tmpdir != NULLPTR);
  const std::string db_path = std::string(tmpdir) + "/cache.coreq";

  PackageTree tree;
  ASSERT_TRUE(build_sample_tree(&tree));

  DBHeader hdr;
  hdr.addOverlay(OverlayIdent("/var/db/repos/gentoo", "gentoo"));
  hdr.addOverlay(OverlayIdent("/var/db/repos/local", "local"));
  Database::prep_header_hashs(&hdr, tree);

  {
    Database writer;
    std::string err;
    ASSERT_TRUE(writer.openwrite(db_path.c_str()));
    ASSERT_TRUE(writer.write_header(hdr, &err));
    ASSERT_TRUE(writer.write_packagetree(tree, hdr, &err));
  }

  {
    Database reader_db;
    std::string err;
    ASSERT_TRUE(reader_db.openread(db_path.c_str()));
    DBHeader read_hdr;
    ASSERT_TRUE(reader_db.read_header(&read_hdr, &err, 0));
    ASSERT_TRUE(read_hdr.isCurrent());

    PackageReader reader(&reader_db, read_hdr);

    ASSERT_TRUE(reader.next());
    ASSERT_TRUE(reader.read(PackageReader::NAME));
    ASSERT_EQ(reader.category(), std::string("cat-a"));
    ASSERT_EQ(reader.get()->name, std::string("alpha"));
    ASSERT_TRUE(reader.skip());

    ASSERT_TRUE(reader.next());
    Package* pkg = reader.release();
    ASSERT_TRUE(pkg != NULLPTR);
    ASSERT_EQ(pkg->category, std::string("cat-b"));
    ASSERT_EQ(pkg->name, std::string("beta"));
    ASSERT_EQ(pkg->desc, std::string("beta description"));
    ASSERT_EQ(pkg->size(), static_cast<Package::size_type>(1));
    ASSERT_TRUE(pkg->latest() != NULLPTR);
    ASSERT_EQ(pkg->latest()->eapi.get(), std::string("8"));
    delete pkg;

    ASSERT_TRUE(!reader.next());
    ASSERT_TRUE(reader.get_errtext() == NULLPTR);
  }

  ASSERT_TRUE(std::remove(db_path.c_str()) == 0);
  ASSERT_TRUE(::rmdir(tmpdir) == 0);
  return 0;
}

static int test_read_header_rejects_corrupted_file() {
  char tmp_template[] = "/tmp/coreq-db-io-bad-XXXXXX";
  char* tmpdir = mkdtemp(tmp_template);
  ASSERT_TRUE(tmpdir != NULLPTR);
  const std::string db_path = std::string(tmpdir) + "/bad.coreq";

  FILE* fp = std::fopen(db_path.c_str(), "wb");
  ASSERT_TRUE(fp != NULLPTR);
  const char bad[] = "not-a-valid-coreq-cache";
  ASSERT_TRUE(std::fwrite(bad, sizeof(char), sizeof(bad) - 1, fp) ==
              sizeof(bad) - 1);
  ASSERT_TRUE(std::fclose(fp) == 0);

  Database db;
  std::string err;
  ASSERT_TRUE(db.openread(db_path.c_str()));
  DBHeader hdr;
  ASSERT_TRUE(!db.read_header(&hdr, &err, 0));
  ASSERT_TRUE(!err.empty());

  ASSERT_TRUE(std::remove(db_path.c_str()) == 0);
  ASSERT_TRUE(::rmdir(tmpdir) == 0);
  return 0;
}

int main() {
  int failed = 0;
  if (test_database_roundtrip_and_package_reader_iteration() != 0) {
    failed++;
  }
  if (test_read_header_rejects_corrupted_file() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All database io tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
