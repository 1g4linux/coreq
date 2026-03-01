#include <iostream>
#include <string>

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

static int test_iuse_flags_and_merge() {
  IUse plus("(+)foo");
  ASSERT_EQ(plus.name(), std::string("foo"));
  ASSERT_EQ(plus.asString(), std::string("(+)foo"));

  IUse minus("-foo");
  ASSERT_EQ(minus.name(), std::string("foo"));
  ASSERT_EQ(minus.asString(), std::string("-foo"));

  IUseSet set;
  set.insert("foo -foo");
  ASSERT_EQ(set.asString(), std::string("(-)foo"));
  return 0;
}

static int test_iuse_natural_ordering() {
  IUseSet set;
  set.insert("flag10 flag2 flag1");
  WordVec vec = set.asVector();
  ASSERT_EQ(vec.size(), static_cast<WordVec::size_type>(3));
  ASSERT_EQ(vec[0], std::string("flag1"));
  ASSERT_EQ(vec[1], std::string("flag2"));
  ASSERT_EQ(vec[2], std::string("flag10"));
  return 0;
}

static int test_effective_keywords_and_saved_state() {
  Version v;
  v.set_full_keywords("amd64 ~arm64");

  ASSERT_EQ(v.get_effective_keywords(), std::string("amd64 ~arm64"));
  v.modify_effective_keywords("-amd64 ~amd64");
  ASSERT_TRUE(v.get_effective_keywords() != std::string("amd64 ~arm64"));

  v.add_accepted_keywords("amd64");
  v.add_accepted_keywords("~arm64");

  v.save_accepted_effective(Version::SAVEEFFECTIVE_USERPROFILE);
  v.modify_effective_keywords("-~amd64");
  ASSERT_TRUE(v.restore_accepted_effective(Version::SAVEEFFECTIVE_USERPROFILE));
  ASSERT_TRUE(v.get_effective_keywords().find("~amd64") != std::string::npos);

  v.keyflags.set_keyflags(KeywordsFlags::KEY_ARCHUNSTABLE);
  v.maskflags.set(MaskFlags::MASK_PACKAGE);
  v.save_keyflags(Version::SAVEKEY_USER);
  v.save_maskflags(Version::SAVEMASK_USER);

  v.keyflags.set_keyflags(KeywordsFlags::KEY_EMPTY);
  v.maskflags.set(MaskFlags::MASK_NONE);
  ASSERT_TRUE(v.restore_keyflags(Version::SAVEKEY_USER));
  ASSERT_TRUE(v.restore_maskflags(Version::SAVEMASK_USER));
  ASSERT_TRUE(v.keyflags.haveall(KeywordsFlags::KEY_ARCHUNSTABLE));
  ASSERT_TRUE(v.maskflags.isPackageMask());
  return 0;
}

static int test_reason_deduplication() {
  Version v;
  StringList reason;
  reason.push_back("masked-by-profile");
  reason.finalize();
  v.add_reason(reason);
  v.add_reason(reason);
  ASSERT_TRUE(v.have_reasons());
  ASSERT_EQ(v.reasons_ptr()->size(),
            static_cast<Version::Reasons::size_type>(1));
  return 0;
}

int main() {
  int failed = 0;
  if (test_iuse_flags_and_merge() != 0) {
    failed++;
  }
  if (test_iuse_natural_ordering() != 0) {
    failed++;
  }
  if (test_effective_keywords_and_saved_state() != 0) {
    failed++;
  }
  if (test_reason_deduplication() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All corepkg version tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
