#include <iostream>
#include <string>

#include "corepkg/mask.h"
#include "corepkg/package.h"
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

static Version* make_version(const char* ver, const char* slot, const char* repo) {
  Version* v = new Version();
  std::string err;
  if (v->parseVersion(ver, &err) != BasicVersion::parsedOK) {
    delete v;
    return NULLPTR;
  }
  v->set_slotname(slot);
  v->reponame = repo;
  v->keyflags.set_keyflags(KeywordsFlags::KEY_STABLE);
  return v;
}

static int test_parse_slot_repo_and_default_repo() {
  Mask slot_repo(Mask::maskMask);
  std::string err;
  ASSERT_EQ(slot_repo.parseMask("=cat/pkg-1.2:1::gentoo", &err), BasicVersion::parsedOK);

  Package p("cat", "pkg");
  Version* matching = make_version("1.2", "1", "gentoo");
  Version* slot_mismatch = make_version("1.2", "0", "gentoo");
  Version* repo_mismatch = make_version("1.2", "1", "overlay");
  ASSERT_TRUE(matching && slot_mismatch && repo_mismatch);
  p.addVersion(matching);
  p.addVersion(slot_mismatch);
  p.addVersion(repo_mismatch);
  ASSERT_TRUE(slot_repo.have_match(p));

  Mask default_repo(Mask::maskMask);
  ASSERT_EQ(default_repo.parseMask("cat/pkg", &err, 1, "mainrepo"), BasicVersion::parsedOK);
  Package only_default("cat", "pkg");
  Version* def = make_version("3.0", "", "mainrepo");
  ASSERT_TRUE(def != NULLPTR);
  only_default.addVersion(def);
  ASSERT_TRUE(default_repo.have_match(only_default));

  Package no_default("cat", "pkg");
  Version* other = make_version("3.0", "", "otherrepo");
  ASSERT_TRUE(other != NULLPTR);
  no_default.addVersion(other);
  ASSERT_TRUE(!default_repo.have_match(no_default));
  return 0;
}

static int test_parse_rejects_invalid_syntax() {
  std::string err;

  Mask missing_colon(Mask::maskMask);
  ASSERT_EQ(missing_colon.parseMask("=cat/pkg-1:slot:repo", &err), BasicVersion::parsedError);
  ASSERT_TRUE(!err.empty());

  Mask wildcard_with_wrong_op(Mask::maskMask);
  ASSERT_EQ(wildcard_with_wrong_op.parseMask("~cat/pkg-1*", &err), BasicVersion::parsedError);
  ASSERT_TRUE(!err.empty());

  Mask version_operator_without_version(Mask::maskMask);
  ASSERT_EQ(version_operator_without_version.parseMask("<cat/pkg", &err), BasicVersion::parsedError);
  ASSERT_TRUE(!err.empty());
  return 0;
}

static int test_mask_apply_transitions_and_redundancy() {
  Package p("cat", "pkg");
  Version* v = make_version("1.0", "", "gentoo");
  ASSERT_TRUE(v != NULLPTR);
  p.addVersion(v);

  std::string err;
  Mask mask(Mask::maskMask);
  ASSERT_EQ(mask.parseMask("cat/pkg", &err), BasicVersion::parsedOK);
  mask.checkMask(&p, Keywords::RED_IN_MASK | Keywords::RED_DOUBLE_MASK | Keywords::RED_MASK);
  ASSERT_TRUE(v->maskflags.isPackageMask());
  ASSERT_TRUE(v->wanted_masked());
  ASSERT_TRUE(v->was_masked());
  ASSERT_TRUE((v->get_redundant() & Keywords::RED_IN_MASK) != 0);

  mask.checkMask(&p, Keywords::RED_DOUBLE_MASK);
  ASSERT_TRUE((v->get_redundant() & Keywords::RED_DOUBLE_MASK) != 0);

  Mask unmask(Mask::maskUnmask);
  ASSERT_EQ(unmask.parseMask("cat/pkg", &err), BasicVersion::parsedOK);
  unmask.checkMask(&p, Keywords::RED_IN_UNMASK | Keywords::RED_UNMASK | Keywords::RED_DOUBLE_UNMASK);
  ASSERT_TRUE(!v->maskflags.isPackageMask());
  ASSERT_TRUE(v->wanted_unmasked());
  ASSERT_TRUE(v->was_unmasked());
  ASSERT_TRUE((v->get_redundant() & Keywords::RED_IN_UNMASK) != 0);
  return 0;
}

static int test_special_mask_types() {
  std::string err;

  Package p("cat", "pkg");
  Version* v = make_version("1.0", "3", "repo1");
  ASSERT_TRUE(v != NULLPTR);
  p.addVersion(v);

  Mask mark_optional(Mask::maskMarkOptional);
  ASSERT_EQ(mark_optional.parseMask("cat/pkg-1.0", &err), BasicVersion::parsedOK);
  mark_optional.checkMask(&p);
  ASSERT_TRUE(v->maskflags.isMarked());

  Mask in_system(Mask::maskInSystem);
  ASSERT_EQ(in_system.parseMask("cat/pkg", &err), BasicVersion::parsedOK);
  in_system.checkMask(&p);
  ASSERT_TRUE(v->maskflags.isSystem());

  Mask in_profile(Mask::maskInProfile);
  ASSERT_EQ(in_profile.parseMask("cat/pkg", &err), BasicVersion::parsedOK);
  in_profile.checkMask(&p);
  ASSERT_TRUE(v->maskflags.isProfile());

  Mask in_world(Mask::maskInWorld);
  ASSERT_EQ(in_world.parseMask("cat/pkg", &err), BasicVersion::parsedOK);
  in_world.checkMask(&p);
  ASSERT_TRUE(v->maskflags.isWorld());

  Mask set_mask(Mask::maskMask);
  ASSERT_EQ(set_mask.parseMask("@my-set", &err), BasicVersion::parsedOK);
  ASSERT_TRUE(set_mask.is_set());
  return 0;
}

int main() {
  int failed = 0;
  if (test_parse_slot_repo_and_default_repo() != 0) {
    failed++;
  }
  if (test_parse_rejects_invalid_syntax() != 0) {
    failed++;
  }
  if (test_mask_apply_transitions_and_redundancy() != 0) {
    failed++;
  }
  if (test_special_mask_types() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All mask tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
