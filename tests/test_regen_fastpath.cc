#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <set>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

#include "various/subcommand_regen.h"

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

static bool mkdir_single(const std::string& path) {
  if (::mkdir(path.c_str(), 0700) == 0) {
    return true;
  }
  return errno == EEXIST;
}

static bool mkdir_p(const std::string& path) {
  if (path.empty() || path[0] != '/') {
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

static int expect_fastpath_distfiles(const std::string& category,
                                     const std::string& package,
                                     const std::string& ebuild,
                                     const std::string& contents,
                                     const std::set<std::string>& expected) {
  char template_dir[] = "/tmp/coreq-regen-fastpath-XXXXXX";
  char* tmpdir = mkdtemp(template_dir);
  ASSERT_TRUE(tmpdir != NULLPTR);

  const std::string repo(tmpdir);
  const std::string pkgdir(repo + "/" + category + "/" + package);
  ASSERT_TRUE(mkdir_p(pkgdir));

  const std::string ebuild_path(pkgdir + "/" + ebuild);
  ASSERT_TRUE(write_text_file(ebuild_path, contents));

  const std::string::size_type pos = ebuild_pos(ebuild);
  ASSERT_TRUE(pos != std::string::npos);

  std::string pn;
  std::string pv;
  ASSERT_TRUE(ExplodeAtom::split(&pn, &pv, ebuild.substr(0, pos).c_str()));

  std::set<std::string> actual;
  SubcommandRegen regen;
  if (!regen.parse_ebuild_src_uri(repo, pkgdir, category, pn, pv, ebuild, actual)) {
    std::cerr << "fast path parse failed for " << category << "/" << package << "/" << ebuild << std::endl;
    std::cerr << contents << std::endl;
    std::string normalized(contents);
    regen.normalize_src_uri_appends(&normalized);
    WordIterateMap raw_vars;
    regen.initialize_src_uri_vars(repo, pkgdir, category, pn, pv, ebuild_path, &raw_vars);
    std::string errtext;
    if (regen.read_ebuild_vars(normalized, VarsReader::INTO_MAP, &raw_vars, &errtext)) {
      std::cerr << "raw SRC_URI: " << raw_vars["SRC_URI"] << std::endl;
      WordVec inherited_eclasses;
      regen.collect_inherited_eclasses(contents, &inherited_eclasses);
      std::string inherited_src_uri;
      if (regen.compute_inherited_src_uri(inherited_eclasses, raw_vars, &inherited_src_uri)) {
        WordIterateMap shell_vars(raw_vars);
        if (!inherited_src_uri.empty()) {
          shell_vars["SRC_URI"] = inherited_src_uri;
        }
        std::string expanded;
        if (regen.expand_shell_value(raw_vars["SRC_URI"], shell_vars, &expanded)) {
          std::cerr << "expanded raw SRC_URI: " << expanded << std::endl;
          std::set<std::string> extracted;
          if (regen.extract_distfiles(expanded, extracted)) {
            std::cerr << "extracted count: " << extracted.size() << std::endl;
          }
        }
      }
    } else {
      std::cerr << "raw parse error: " << errtext << std::endl;
    }
    return 1;
  }
  if (actual != expected) {
    std::cerr << "unexpected distfiles for " << category << "/" << package << "/" << ebuild << std::endl;
    std::string normalized(contents);
    regen.normalize_src_uri_appends(&normalized);
    WordIterateMap raw_vars;
    regen.initialize_src_uri_vars(repo, pkgdir, category, pn, pv, ebuild_path, &raw_vars);
    std::string errtext;
    if (regen.read_ebuild_vars(normalized, VarsReader::INTO_MAP, &raw_vars, &errtext)) {
      std::cerr << "raw SRC_URI: " << raw_vars["SRC_URI"] << std::endl;
      WordVec inherited_eclasses;
      regen.collect_inherited_eclasses(contents, &inherited_eclasses);
      std::string inherited_src_uri;
      if (regen.compute_inherited_src_uri(inherited_eclasses, raw_vars, &inherited_src_uri)) {
        WordIterateMap shell_vars(raw_vars);
        if (!inherited_src_uri.empty()) {
          shell_vars["SRC_URI"] = inherited_src_uri;
        }
        std::string expanded;
        if (regen.expand_shell_value(raw_vars["SRC_URI"], shell_vars, &expanded)) {
          std::cerr << "expanded raw SRC_URI: " << expanded << std::endl;
          std::set<std::string> extracted;
          if (regen.extract_distfiles(expanded, extracted)) {
            std::cerr << "extracted count: " << extracted.size() << std::endl;
          }
        }
      }
    } else {
      std::cerr << "raw parse error: " << errtext << std::endl;
    }
    std::cerr << "expected:" << std::endl;
    for (std::set<std::string>::const_iterator it = expected.begin(); it != expected.end(); ++it) {
      std::cerr << "  " << *it << std::endl;
    }
    std::cerr << "actual:" << std::endl;
    for (std::set<std::string>::const_iterator it = actual.begin(); it != actual.end(); ++it) {
      std::cerr << "  " << *it << std::endl;
    }
    return 1;
  }
  return 0;
}

static int test_inherited_pypi_default() {
  std::set<std::string> expected;
  expected.insert("jaraco_context-6.1.0.tar.gz");
  return expect_fastpath_distfiles(
      "dev-python",
      "jaraco-context",
      "jaraco-context-6.1.0.ebuild",
      "EAPI=8\n"
      "PYPI_PN=${PN/-/.}\n"
      "inherit pypi\n",
      expected);
}

static int test_pypi_default_plus_wheel_append() {
  std::set<std::string> expected;
  expected.insert("installer-0.7.0.tar.gz");
  expected.insert("installer-0.7.0-py3-none-any.whl.zip");
  return expect_fastpath_distfiles(
      "dev-python",
      "installer",
      "installer-0.7.0.ebuild",
      "EAPI=8\n"
      "inherit pypi\n"
      "SRC_URI+=\" $(pypi_wheel_url --unpack)\"\n",
      expected);
}

static int test_ver_cut_in_src_uri() {
  std::set<std::string> expected;
  expected.insert("ruby-3.4.3.tar.gz");
  return expect_fastpath_distfiles(
      "app-lang",
      "ruby",
      "ruby-3.4.3.ebuild",
      "EAPI=8\n"
      "SRC_URI=\"https://cache.ruby-lang.org/pub/ruby/$(ver_cut 1-2)/${P}.tar.gz\"\n",
      expected);
}

static int test_nested_pypi_helper_with_ver_cut() {
  std::set<std::string> expected;
  expected.insert("cryptography-46.0.5.tar.gz");
  expected.insert("cryptography_vectors-46.0.5.tar.gz");
  return expect_fastpath_distfiles(
      "app-crypto",
      "cryptography",
      "cryptography-46.0.5.ebuild",
      "EAPI=8\n"
      "inherit pypi\n"
      "SRC_URI+=\" test? ( $(pypi_sdist_url cryptography_vectors \\\"$(ver_cut 1-3)\\\") )\"\n",
      expected);
}

static int test_inherited_tree_sitter_default() {
  std::set<std::string> expected;
  expected.insert("tree-sitter-bash-20251202.tar.gz");
  return expect_fastpath_distfiles(
      "lib-dev",
      "tree-sitter-bash",
      "tree-sitter-bash-20251202.ebuild",
      "EAPI=8\n"
      "inherit tree-sitter-grammar\n",
      expected);
}

static int maybe_sweep_tree() {
  const char* repo_env = std::getenv("COREQ_REGEN_SWEEP_REPO");
  if ((repo_env == NULLPTR) || (*repo_env == '\0')) {
    return 0;
  }

  const std::string repo(repo_env);
  LineVec categories;
  ASSERT_TRUE(pushback_lines((repo + "/profiles/categories").c_str(), &categories, false, false));

  SubcommandRegen regen;
  int failures = 0;
  int metadata_skips = 0;

  for (LineVec::const_iterator cat_it = categories.begin(); cat_it != categories.end(); ++cat_it) {
    WordVec packages;
    if (!pushback_files(repo + "/" + *cat_it, &packages, NULLPTR, 2, true, false)) {
      continue;
    }
    for (WordVec::const_iterator pkg_it = packages.begin(); pkg_it != packages.end(); ++pkg_it) {
      const std::string pkgdir = repo + "/" + *cat_it + "/" + *pkg_it;
      WordVec ebuilds;
      if (!pushback_files(pkgdir, &ebuilds, NULLPTR, 1, true, false)) {
        continue;
      }
      for (WordVec::const_iterator eb_it = ebuilds.begin(); eb_it != ebuilds.end(); ++eb_it) {
        if (eb_it->size() <= 7 || eb_it->compare(eb_it->size() - 7, 7, ".ebuild") != 0) {
          continue;
        }
        const std::string::size_type pos = ebuild_pos(*eb_it);
        if (pos == std::string::npos) {
          continue;
        }

        std::string pn;
        std::string pv;
        if (!ExplodeAtom::split(&pn, &pv, eb_it->substr(0, pos).c_str())) {
          continue;
        }
        const bool is_live = (pv == "9999" || ((pv.size() >= 5) && pv.compare(pv.size() - 5, 5, ".9999") == 0));
        if (is_live) {
          continue;
        }

        std::set<std::string> expected;
        if (!regen.parse_ebuild_src_uri(repo, pkgdir, *cat_it, pn, pv, *eb_it, expected)) {
          std::cerr << "fast path failed: " << *cat_it << "/" << *pkg_it << "/" << *eb_it << std::endl;
          ++failures;
          continue;
        }

        std::set<std::string> actual;
        if (!regen.parse_corepkg_src_uri(repo, *cat_it + "/" + pn + "-" + pv, actual)) {
          std::cerr << "corepkg metadata skipped: " << *cat_it << "/" << *pkg_it << "/" << *eb_it << std::endl;
          ++metadata_skips;
          continue;
        }
        if (expected != actual) {
          std::cerr << "fast path mismatch: " << *cat_it << "/" << *pkg_it << "/" << *eb_it << std::endl;
          std::cerr << "expected:" << std::endl;
          for (std::set<std::string>::const_iterator it = actual.begin(); it != actual.end(); ++it) {
            std::cerr << "  " << *it << std::endl;
          }
          std::cerr << "actual:" << std::endl;
          for (std::set<std::string>::const_iterator it = expected.begin(); it != expected.end(); ++it) {
            std::cerr << "  " << *it << std::endl;
          }
          ++failures;
        }
      }
    }
  }

  if (metadata_skips != 0) {
    std::cerr << "corepkg metadata skips: " << metadata_skips << std::endl;
  }

  ASSERT_EQ(failures, 0);
  return 0;
}

static int maybe_debug_ebuild() {
  const char* repo_env = std::getenv("COREQ_REGEN_DEBUG_REPO");
  const char* ebuild_env = std::getenv("COREQ_REGEN_DEBUG_EBUILD");
  if ((repo_env == NULLPTR) || (*repo_env == '\0') || (ebuild_env == NULLPTR) || (*ebuild_env == '\0')) {
    return 0;
  }

  const std::string repo(repo_env);
  const std::string relpath(ebuild_env);
  const std::string fullpath(repo + "/" + relpath);
  const std::string::size_type slash2 = relpath.rfind('/');
  ASSERT_TRUE(slash2 != std::string::npos);
  const std::string::size_type slash1 = relpath.rfind('/', slash2 - 1);
  ASSERT_TRUE(slash1 != std::string::npos);

  const std::string category = relpath.substr(0, slash1);
  const std::string package = relpath.substr(slash1 + 1, slash2 - slash1 - 1);
  const std::string ebuild = relpath.substr(slash2 + 1);
  const std::string pkgdir = repo + "/" + category + "/" + package;
  const std::string::size_type pos = ebuild_pos(ebuild);
  ASSERT_TRUE(pos != std::string::npos);

  std::string pn;
  std::string pv;
  ASSERT_TRUE(ExplodeAtom::split(&pn, &pv, ebuild.substr(0, pos).c_str()));

  SubcommandRegen regen;
  std::set<std::string> expected;
  const bool ok = regen.parse_ebuild_src_uri(repo, pkgdir, category, pn, pv, ebuild, expected);
  std::cerr << "parse_ebuild_src_uri: " << (ok ? "ok" : "failed") << std::endl;
  if (ok) {
    for (std::set<std::string>::const_iterator it = expected.begin(); it != expected.end(); ++it) {
      std::cerr << "  distfile: " << *it << std::endl;
    }
    return 0;
  }

  std::string source;
  ASSERT_TRUE(regen.read_file_to_string(fullpath, &source));
  regen.strip_noop_quoted_blocks(&source);
  regen.prune_simple_live_conditionals(pv, &source);

  WordVec inherited_eclasses;
  regen.collect_inherited_eclasses(source, &inherited_eclasses);
  std::cerr << "inherited:";
  for (WordVec::const_iterator it = inherited_eclasses.begin(); it != inherited_eclasses.end(); ++it) {
    std::cerr << " " << *it;
  }
  std::cerr << std::endl;
  std::cerr << "allow_append_without_plain: "
            << (regen.has_supported_inherited_src_uri(inherited_eclasses) ? "yes" : "no") << std::endl;
  std::cerr << "unsafe_src_uri_assignment: "
            << (regen.has_unsafe_src_uri_assignment(source, regen.has_supported_inherited_src_uri(inherited_eclasses)) ? "yes" : "no")
            << std::endl;
  std::cerr << "unsafe_preamble_control_flow: "
            << (regen.has_unsafe_preamble_control_flow(source) ? "yes" : "no") << std::endl;

  WordIterateMap raw_vars;
  regen.initialize_src_uri_vars(repo, pkgdir, category, pn, pv, fullpath, &raw_vars);
  std::string errtext;
  const bool read_ok = regen.read_ebuild_vars(source, VarsReader::INTO_MAP, &raw_vars, &errtext);
  std::cerr << "read_ebuild_vars: " << (read_ok ? "ok" : "failed") << std::endl;
  if (!read_ok) {
    std::cerr << errtext << std::endl;
  }
  static const char* debug_vars[] = {
      "MY_P", "MY_PN", "FLIT_CORE_PV", "CLDR_PV", "TEST_TAG", "TEST_P",
      "MY_UP_PV", "MY_REL", "MOZ_PV", "MOZ_VER", "BASE_URI", "CARGO_CRATE_URIS"
  };
  for (unsigned i = 0; i < sizeof(debug_vars) / sizeof(debug_vars[0]); ++i) {
    WordIterateMap::const_iterator it = raw_vars.find(debug_vars[i]);
    if (it != raw_vars.end()) {
      std::cerr << debug_vars[i] << "=" << it->second << std::endl;
    }
  }

  std::string inherited_src_uri;
  const bool inherited_ok = regen.compute_inherited_src_uri(inherited_eclasses, raw_vars, &inherited_src_uri);
  std::cerr << "compute_inherited_src_uri: " << (inherited_ok ? "ok" : "failed") << std::endl;
  if (inherited_ok) {
    std::cerr << "inherited SRC_URI: " << inherited_src_uri << std::endl;
  }

  std::string src_uri_expr;
  const bool expr_ok = regen.extract_src_uri_expression(source, inherited_src_uri, &src_uri_expr);
  std::cerr << "extract_src_uri_expression: " << (expr_ok ? "ok" : "failed") << std::endl;
  if (expr_ok) {
    std::cerr << "src_uri_expr: " << src_uri_expr << std::endl;
  }

  WordIterateMap shell_vars(raw_vars);
  if (!inherited_src_uri.empty()) {
    shell_vars["SRC_URI"] = inherited_src_uri;
  }
  std::string expanded;
  const bool expand_ok = regen.expand_shell_value(src_uri_expr, shell_vars, &expanded);
  std::cerr << "expand_shell_value: " << (expand_ok ? "ok" : "failed") << std::endl;
  if (expand_ok) {
    std::cerr << "expanded SRC_URI: " << expanded << std::endl;
  }

  std::set<std::string> distfiles;
  const bool extract_ok = expand_ok && regen.extract_distfiles(expanded, distfiles);
  std::cerr << "extract_distfiles: " << (extract_ok ? "ok" : "failed") << std::endl;
  if (extract_ok) {
    for (std::set<std::string>::const_iterator it = distfiles.begin(); it != distfiles.end(); ++it) {
      std::cerr << "  distfile: " << *it << std::endl;
    }
  }

  return 0;
}

int main() {
  int failed = 0;

  if (test_inherited_pypi_default() != 0) {
    std::cerr << "test_inherited_pypi_default failed" << std::endl;
    failed++;
  }
  if (test_pypi_default_plus_wheel_append() != 0) {
    std::cerr << "test_pypi_default_plus_wheel_append failed" << std::endl;
    failed++;
  }
  if (test_ver_cut_in_src_uri() != 0) {
    std::cerr << "test_ver_cut_in_src_uri failed" << std::endl;
    failed++;
  }
  if (test_nested_pypi_helper_with_ver_cut() != 0) {
    std::cerr << "test_nested_pypi_helper_with_ver_cut failed" << std::endl;
    failed++;
  }
  if (test_inherited_tree_sitter_default() != 0) {
    std::cerr << "test_inherited_tree_sitter_default failed" << std::endl;
    failed++;
  }
  if (maybe_debug_ebuild() != 0) {
    std::cerr << "maybe_debug_ebuild failed" << std::endl;
    failed++;
  }
  if (maybe_sweep_tree() != 0) {
    std::cerr << "maybe_sweep_tree failed" << std::endl;
    failed++;
  }

  if (failed == 0) {
    std::cout << "All regen fast-path tests passed!" << std::endl;
    return 0;
  }
  std::cerr << failed << " tests failed!" << std::endl;
  return 1;
}
