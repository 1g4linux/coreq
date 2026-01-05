#include <iostream>
#include <string>

#include "corepkg/overlay.h"

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

static int test_compare_and_names() {
  OverlayIdent a("/a", "repo-a", 1, false);
  OverlayIdent b("/b", "repo-b", 2, false);
  ASSERT_TRUE(a < b);

  OverlayIdent same_prio_label_a("/x", "a", 5, false);
  OverlayIdent same_prio_label_b("/x", "b", 5, false);
  ASSERT_TRUE(same_prio_label_a < same_prio_label_b);

  OverlayIdent label_missing("/path-only", "", 0, false);
  ASSERT_EQ(label_missing.name(), std::string("/path-only"));
  ASSERT_EQ(label_missing.human_readable(), std::string("/path-only"));

  OverlayIdent labeled("/path", "repo", 0, false);
  ASSERT_EQ(labeled.name(), std::string("repo"));
  ASSERT_EQ(labeled.human_readable(), std::string("\"repo\" /path"));
  return 0;
}

static int test_repolist_push_back_dedup() {
  RepoList repos;
  repos.push_back(OverlayIdent("/ov1", NULLPTR, 5, false), true);
  repos.push_back(OverlayIdent("/ov1", "repo1", 9, false), true);

  ASSERT_EQ(repos.size(), static_cast<RepoList::size_type>(1));
  ASSERT_EQ(repos[0].priority, static_cast<OverlayIdent::Priority>(9));
  ASSERT_TRUE(repos[0].know_label);
  ASSERT_EQ(repos[0].label, std::string("repo1"));
  return 0;
}

static int test_set_priority_by_label_and_path() {
  RepoList repos;
  repos.push_back(OverlayIdent("/main", "main", 0, true));
  repos.push_back(OverlayIdent("/ov1", "repo1", 17, false));

  OverlayIdent by_label(NULLPTR, "repo1", 0, false);
  repos.set_priority(&by_label);
  ASSERT_EQ(by_label.priority, static_cast<OverlayIdent::Priority>(17));

  OverlayIdent by_path("/ov1", NULLPTR, 0, false);
  repos.set_priority(&by_path);
  ASSERT_EQ(by_path.priority, static_cast<OverlayIdent::Priority>(17));
  return 0;
}

static int test_sort_keeps_first_entry_fixed() {
  RepoList repos;
  repos.push_back(OverlayIdent("/main", "main", 999, true));
  repos.push_back(OverlayIdent("/b", "b", 20, false));
  repos.push_back(OverlayIdent("/a", "a", 10, false));

  repos.sort();

  ASSERT_EQ(repos[0].path, std::string("/main"));
  ASSERT_EQ(repos[1].path, std::string("/a"));
  ASSERT_EQ(repos[2].path, std::string("/b"));
  return 0;
}

int main() {
  int failed = 0;
  if (test_compare_and_names() != 0) {
    failed++;
  }
  if (test_repolist_push_back_dedup() != 0) {
    failed++;
  }
  if (test_set_priority_by_label_and_path() != 0) {
    failed++;
  }
  if (test_sort_keeps_first_entry_fixed() != 0) {
    failed++;
  }

  if (failed == 0) {
    std::cout << "All overlay tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
