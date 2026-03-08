// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_REGEN_H_
#define SRC_VARIOUS_SUBCOMMAND_REGEN_H_ 1

#include <config.h>
#include <climits>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <iterator>
#include <sstream>
#include <cstdlib>
#include <cerrno>
#include <csignal>
#include <fnmatch.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "various/subcommand.h"
#include "corepkg/conf/corepkgsettings.h"
#include "corepkg/extendedversion.h"
#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"
#include "coreqTk/argsreader.h"
#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"
#include "coreqTk/null.h"
#include "coreqTk/parseerror.h"
#include "coreqTk/utils.h"
#include "coreqTk/stringutils.h"
#include "coreqTk/varsreader.h"
#include "coreqTk/ansicolor.h"
#include "cache/common/selectors.h"
#include "cache/common/assign_reader.h"
#include "corepkg/depend.h"

static volatile std::sig_atomic_t sigint_received = 0;

static void handle_sigint(int) {
    sigint_received = 1;
}

struct ManifestDist {
    std::string filename;
    uint64_t size;
    std::map<std::string, std::string> hashes;
};

class SubcommandRegen : public Subcommand {
 public:
  virtual int run(int argc, char** argv) {
    sigint_received = 0;
#ifdef HAVE_SIGACTION
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULLPTR);
#else
    std::signal(SIGINT, handle_sigint);
#endif

    CoreqRc& coreqrc = get_coreqrc();
    ParseError parse_error;
    CorePkgSettings corepkgsettings(&coreqrc, &parse_error, true, true);
    AnsiColor& c = get_ansicolor();
    ExtendedVersion::use_src_uri = true;

    bool help = false;
    bool verbose = false;
    bool dry_run = false;
    
    OptionList opt_table;
    opt_table.PUSH_BACK(Option("help", 'h', Option::BOOLEAN, &help));
    opt_table.PUSH_BACK(Option("verbose", 'v', Option::BOOLEAN, &verbose));
    opt_table.PUSH_BACK(Option("dry-run", 'n', Option::BOOLEAN, &dry_run));
    
    ArgumentReader ar(argc, argv, opt_table);
    if (help) {
      coreq::say("%s") % usage();
      return EXIT_SUCCESS;
    }
    
    std::string portdir;
    if (!find_repo_root_from_cwd(&portdir)) {
      portdir = corepkgsettings["PORTDIR"];
      if (portdir.empty()) {
        struct stat st;
        if (stat("profiles/categories", &st) == 0) {
          char cwd[PATH_MAX];
          if (getcwd(cwd, sizeof(cwd))) {
            portdir = cwd;
          }
        }
      }
    }

    if (portdir.empty()) {
        coreq::say_error(c.red(_("PORTDIR not set and current directory is not a repository.")));
        return EXIT_FAILURE;
    }

    std::string distdir = corepkgsettings["DISTDIR"];
    if (distdir.empty()) {
        distdir = portdir + "/distfiles";
    }

    std::set<std::string> target_packages;
    if (!collect_targets(portdir, ar, &target_packages)) {
      return EXIT_FAILURE;
    }

    for (std::set<std::string>::const_iterator cp_it = target_packages.begin(); cp_it != target_packages.end(); ++cp_it) {
            if (sigint_received) break;
            const std::string::size_type slash_pos = cp_it->find('/');
            if (slash_pos == std::string::npos) {
              continue;
            }
            const std::string category = cp_it->substr(0, slash_pos);
            const std::string package = cp_it->substr(slash_pos + 1);
            if (category.empty() || package.empty()) {
              continue;
            }
            std::string pkg_path = portdir + "/" + category + "/" + package;
            std::string manifest_path = pkg_path + "/Manifest";
            
            std::map<std::string, ManifestDist> manifest_dists;
            parse_manifest(manifest_path, manifest_dists);

            std::set<std::string> expected_distfiles;
            bool is_complex = false;
            WordVec ebuild_files;
            if (!scandir_cc(pkg_path, &ebuild_files, ebuild_selector)) {
                continue;
            }

            std::string newest_ebuild;
            std::string newest_non_live;
            for (WordVec::const_iterator eb_it = ebuild_files.begin(); eb_it != ebuild_files.end(); ++eb_it) {
                if (eb_it->find(".ebuild") == std::string::npos) continue;

                if (newest_ebuild.empty() || *eb_it > newest_ebuild) {
                    newest_ebuild = *eb_it;
                }

                std::string pn, pv;
                std::string::size_type pos = ebuild_pos(*eb_it);
                if (pos != std::string::npos) {
                    if (ExplodeAtom::split(&pn, &pv, eb_it->substr(0, pos).c_str())) {
                        bool is_live = (pv == "9999" || (pv.size() >= 5 && pv.compare(pv.size() - 5, 5, ".9999") == 0));
                        if (is_live) continue;

                        if (newest_non_live.empty() || *eb_it > newest_non_live) {
                            newest_non_live = *eb_it;
                        }

                        bool eb_complex = false;
                        std::string cpv = category + "/" + pn + "-" + pv;
                        // Try metadata cache first, then parse the ebuild locally
                        // before falling back to asking corepkg directly.
                        if (!parse_metadata_src_uri(portdir, category, package, *eb_it, expected_distfiles)) {
                            if (!parse_ebuild_src_uri(portdir, pkg_path, category, pn, pv, *eb_it, expected_distfiles) &&
                                !parse_corepkg_src_uri(portdir, cpv, expected_distfiles)) {
                                eb_complex = true;
                            }
                        }
                        if (eb_complex) is_complex = true;
                    }
                }
            }

            if (newest_non_live.empty()) newest_non_live = newest_ebuild;

            // Reconciliation
            bool needs_regen = false;
            std::vector<std::string> missing_in_manifest;
            std::vector<std::string> extra_in_manifest;
            std::vector<std::string> size_mismatch;

            // Check if all expected files are in Manifest
            for (std::set<std::string>::iterator it = expected_distfiles.begin(); it != expected_distfiles.end(); ++it) {
                std::string filename = *it;
                if (manifest_dists.find(filename) == manifest_dists.end()) {
                    missing_in_manifest.push_back(filename);
                    needs_regen = true;
                } else {
                    // File is in Manifest, check if it exists in DISTDIR and has correct size
                    struct stat st;
                    std::string distfile_path = distdir + "/" + filename;
                    if (stat(distfile_path.c_str(), &st) == 0) {
                        if (st.st_size != (off_t)manifest_dists[filename].size) {
                            size_mismatch.push_back(filename);
                            needs_regen = true;
                        }
                    }
                }
            }

            // Check if Manifest has files that are no longer expected
            if (!is_complex) {
                for (std::map<std::string, ManifestDist>::iterator it = manifest_dists.begin(); it != manifest_dists.end(); ++it) {
                    if (expected_distfiles.find(it->first) == expected_distfiles.end()) {
                        extra_in_manifest.push_back(it->first);
                        needs_regen = true;
                    }
                }
            }

            if (needs_regen) {
                if (!newest_non_live.empty()) {
                    std::string pkg_name = c.cyan(category + "/" + package);
                    if (dry_run) {
                        coreq::say(c.bold(c.yellow(_("[DRY-RUN]"))) + " " + _("Would regenerate Manifest for %s")) % pkg_name;
                    } else {
                        coreq::say(c.bold(c.green(_("Regenerating Manifest"))) + " " + _("for %s...")) % pkg_name;
                    }

                    if (verbose || dry_run) {
                        for (size_t i = 0; i < missing_in_manifest.size(); ++i) coreq::say("  - " + c.red(_("Missing in Manifest:")) + " %s") % missing_in_manifest[i];
                        for (size_t i = 0; i < extra_in_manifest.size(); ++i) coreq::say("  - " + c.yellow(_("Extra in Manifest:")) + " %s") % extra_in_manifest[i];
                        for (size_t i = 0; i < size_mismatch.size(); ++i) coreq::say("  - " + c.red(_("Size mismatch in DISTDIR:")) + " %s") % size_mismatch[i];
                    }

                    if (!dry_run) {
                        std::string cmd = "ebuild " + pkg_path + "/" + newest_non_live + " manifest";
                        if (verbose) coreq::say("  " + c.blue(_("Running:")) + " %s") % cmd;
                        int status = std::system(cmd.c_str());
                        if (command_was_interrupted(status)) {
                            sigint_received = 1;
                        } else if (status != 0) {
                            coreq::say_error(c.red(_("Failed to run: %s"))) % cmd;
                        }
                    }
                    if (sigint_received) break;
                }
            }
        }

    if (sigint_received) {
        coreq::say(c.bold(c.red(_("Interrupted by user."))));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }

  bool directory_exists(const std::string& path) const {
    struct stat st;
    return (stat(path.c_str(), &st) == 0) && S_ISDIR(st.st_mode);
  }

  bool collect_all_packages(const std::string& portdir, std::set<std::string>* targets) const {
    WordVec categories;
    const std::string cat_file = portdir + "/profiles/categories";
    if (!pushback_lines(cat_file.c_str(), &categories, false, false)) {
      return false;
    }
    for (WordVec::const_iterator cat_it = categories.begin(); cat_it != categories.end(); ++cat_it) {
      const std::string cat_dir = portdir + "/" + *cat_it;
      WordVec packages;
      if (!pushback_files(cat_dir, &packages, NULLPTR, 2, true, false)) {
        continue;
      }
      for (WordVec::const_iterator pkg_it = packages.begin(); pkg_it != packages.end(); ++pkg_it) {
        targets->insert(*cat_it + "/" + *pkg_it);
      }
    }
    return true;
  }

  bool add_category_packages(const std::string& portdir, const std::string& category, std::set<std::string>* targets) const {
    const std::string cat_dir = portdir + "/" + category;
    if (!directory_exists(cat_dir)) {
      return false;
    }
    WordVec packages;
    if (!pushback_files(cat_dir, &packages, NULLPTR, 2, true, false)) {
      return false;
    }
    for (WordVec::const_iterator pkg_it = packages.begin(); pkg_it != packages.end(); ++pkg_it) {
      targets->insert(category + "/" + *pkg_it);
    }
    return true;
  }

  bool repo_relative_path(const std::string& portdir, const std::string& path, std::string* relative) const {
    char repo_abs[PATH_MAX];
    char path_abs[PATH_MAX];
    if (realpath(portdir.c_str(), repo_abs) == NULLPTR) {
      return false;
    }
    if (realpath(path.c_str(), path_abs) == NULLPTR) {
      return false;
    }

    const std::string repo(repo_abs);
    const std::string target(path_abs);
    if (target.compare(0, repo.size(), repo) != 0) {
      return false;
    }
    if ((target.size() > repo.size()) && (target[repo.size()] != '/')) {
      return false;
    }
    if (target.size() == repo.size()) {
      relative->clear();
      return true;
    }
    *relative = target.substr(repo.size() + 1);
    return true;
  }

  bool add_targets_from_path(const std::string& portdir, const std::string& path, std::set<std::string>* targets) const {
    std::string rel;
    if (!repo_relative_path(portdir, path, &rel)) {
      return false;
    }
    if (rel.empty()) {
      return collect_all_packages(portdir, targets);
    }

    WordVec parts;
    split_string(&parts, rel, false, "/");
    if (parts.empty()) {
      return collect_all_packages(portdir, targets);
    }
    if (parts.size() == 1) {
      return add_category_packages(portdir, parts[0], targets);
    }

    const std::string cp = parts[0] + "/" + parts[1];
    if (directory_exists(portdir + "/" + cp)) {
      targets->insert(cp);
      return true;
    }
    return false;
  }

  bool add_target_from_atom(const std::string& portdir, const std::string& atom, std::set<std::string>* targets) const {
    std::string cp(atom);
    const std::string::size_type repo_pos = cp.find("::");
    if (repo_pos != std::string::npos) {
      cp.erase(repo_pos);
    }
    const std::string::size_type slot_pos = cp.find(':');
    if (slot_pos != std::string::npos) {
      cp.erase(slot_pos);
    }
    while (!cp.empty() && ((cp[0] == '=') || (cp[0] == '<') || (cp[0] == '>') || (cp[0] == '~'))) {
      cp.erase(0, 1);
    }

    const std::string::size_type slash_pos = cp.find('/');
    if (slash_pos == std::string::npos) {
      return false;
    }
    const std::string category = cp.substr(0, slash_pos);
    const std::string package = cp.substr(slash_pos + 1);
    if (category.empty() || package.empty()) {
      return false;
    }
    const std::string pkg_dir = portdir + "/" + category + "/" + package;
    if (!directory_exists(pkg_dir)) {
      return false;
    }
    targets->insert(category + "/" + package);
    return true;
  }

  bool collect_targets(const std::string& portdir, const ArgumentReader& ar, std::set<std::string>* targets) const {
    bool have_args = false;
    for (ArgumentReader::const_iterator it = ar.begin(); it != ar.end(); ++it) {
      if (it->type != Parameter::ARGUMENT) {
        continue;
      }
      have_args = true;
      const std::string target(it->m_argument);
      struct stat st;
      if (stat(target.c_str(), &st) == 0) {
        if (!add_targets_from_path(portdir, target, targets)) {
          return false;
        }
      } else if (!add_target_from_atom(portdir, target, targets)) {
        return false;
      }
    }

    if (!have_args) {
      char cwd[PATH_MAX];
      if (getcwd(cwd, sizeof(cwd)) != NULLPTR) {
        if (!add_targets_from_path(portdir, cwd, targets)) {
          return collect_all_packages(portdir, targets);
        }
      } else {
        return collect_all_packages(portdir, targets);
      }
    }
    return !targets->empty();
  }

  bool find_repo_root_from_cwd(std::string* root) const {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULLPTR) {
      return false;
    }

    std::string curr(cwd);
    for (;;) {
      struct stat st;
      if (stat((curr + "/profiles/categories").c_str(), &st) == 0) {
        *root = curr;
        return true;
      }
      if (curr == "/") {
        break;
      }
      const std::string::size_type pos = curr.find_last_of('/');
      if (pos == std::string::npos) {
        break;
      }
      if (pos == 0) {
        curr = "/";
      } else {
        curr.erase(pos);
      }
    }
    return false;
  }

  bool command_was_interrupted(int status) const {
#ifdef WIFSIGNALED
    if (status != -1 && WIFSIGNALED(status) && (WTERMSIG(status) == SIGINT)) {
      return true;
    }
#endif
#ifdef WIFEXITED
    if (status != -1 && WIFEXITED(status) && (WEXITSTATUS(status) == 128 + SIGINT)) {
      return true;
    }
#endif
    return false;
  }

  void parse_manifest(const std::string& path, std::map<std::string, ManifestDist>& dists) {
    std::ifstream in(path.c_str());
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        WordVec parts = split_string(line);
        if (parts.size() >= 5 && parts[0] == "DIST") {
            ManifestDist info;
            info.filename = parts[1];
            info.size = my_atou(parts[2].c_str());
            for (size_t i = 3; i + 1 < parts.size(); i += 2) {
                info.hashes[parts[i]] = parts[i+1];
            }
            dists[info.filename] = info;
        }
    }
  }

  bool parse_metadata_src_uri(const std::string& portdir, const std::string& cat, const std::string& pkg, const std::string& ebuild, std::set<std::string>& expected) {
    std::string p = ebuild;
    if (p.size() > 7 && p.compare(p.size() - 7, 7, ".ebuild") == 0) {
        p.erase(p.size() - 7);
    }
    
    std::string md5_path = portdir + "/metadata/md5-cache/" + cat + "/" + p;
    struct stat st;
    if (stat(md5_path.c_str(), &st) != 0) return false;

    // We need a dummy cache for the readers
    class DummyCache : public BasicCache {
    public:
        virtual bool initialize(const std::string&) { return true; }
        virtual const char* getType() const { return "dummy"; }
        virtual bool readCategoryPrepare(const char*) { return true; }
        virtual bool readCategory(Category*) { return true; }
    } dummy;

    AssignReader reader(&dummy);
    std::string eapi, keywords, slot, iuse, req_use, restr, props, src_uri;
    Depend dep;
    reader.get_keywords_slot_iuse_restrict(md5_path, &eapi, &keywords, &slot, &iuse, &req_use, &restr, &props, &dep, &src_uri);
    
    if (src_uri.empty()) return true;

    return extract_distfiles(src_uri, expected);
  }

  bool parse_ebuild_src_uri(const std::string& portdir,
                            const std::string& pkg_path,
                            const std::string& category,
                            const std::string& pn,
                            const std::string& pvr,
                            const std::string& ebuild,
                            std::set<std::string>& expected) const {
    const std::string ebuild_path = pkg_path + "/" + ebuild;
    std::string ebuild_source;
    if (!read_file_to_string(ebuild_path, &ebuild_source)) {
      return false;
    }
    strip_noop_quoted_blocks(&ebuild_source);
    prune_simple_live_conditionals(pvr, &ebuild_source);
    WordVec inherited_eclasses;
    collect_inherited_eclasses(ebuild_source, &inherited_eclasses);
    if (has_unsafe_src_uri_assignment(ebuild_source, has_supported_inherited_src_uri(inherited_eclasses))) {
      return false;
    }

    WordIterateMap raw_vars;
    initialize_src_uri_vars(portdir, pkg_path, category, pn, pvr, ebuild_path, &raw_vars);
    std::string errtext;
    if (!read_ebuild_vars(ebuild_source, VarsReader::INTO_MAP, &raw_vars, &errtext)) {
      return false;
    }
    initialize_inherited_helper_vars(inherited_eclasses, &raw_vars);

    std::string inherited_src_uri;
    if (!compute_inherited_src_uri(inherited_eclasses, raw_vars, &inherited_src_uri)) {
      return false;
    }

    std::string src_uri_expr;
    if (!extract_src_uri_expression(ebuild_source, inherited_src_uri, &src_uri_expr)) {
      return false;
    }
    if (src_uri_expr.empty()) {
      expected.clear();
      return true;
    }

    WordIterateMap shell_vars(raw_vars);
    if (!inherited_src_uri.empty()) {
      shell_vars["SRC_URI"] = inherited_src_uri;
    }
    std::string expanded_src_uri;
    if (!expand_shell_value(src_uri_expr, shell_vars, &expanded_src_uri)) {
      return false;
    }
    return extract_distfiles(expanded_src_uri, expected);
  }

  bool parse_corepkg_src_uri(const std::string& portdir, const std::string& cpv, std::set<std::string>& expected) {
    std::string src_uri;
    if (!query_corepkg_src_uri(portdir, cpv, &src_uri)) {
      return false;
    }
    if (src_uri.empty()) {
      return true;
    }
    return extract_distfiles(src_uri, expected);
  }

  bool query_corepkg_src_uri(const std::string& portdir, const std::string& cpv, std::string* src_uri) const {
    int fds[2];
    if (pipe(fds) != 0) {
      return false;
    }

    pid_t child = fork();
    if (child == -1) {
      close(fds[0]);
      close(fds[1]);
      return false;
    }
    if (child == 0) {
      close(fds[0]);
      if (dup2(fds[1], STDOUT_FILENO) == -1) {
        _exit(EXIT_FAILURE);
      }
      close(fds[1]);
      int devnull = open("/dev/null", O_WRONLY);
      if (devnull != -1) {
        dup2(devnull, STDERR_FILENO);
        close(devnull);
      }
      setenv("PORTDIR", portdir.c_str(), 1);
      execlp("corepkgq", "corepkgq", "metadata", "/", "ebuild", cpv.c_str(), "SRC_URI", static_cast<const char*>(NULLPTR));
      _exit(EXIT_FAILURE);
    }

    close(fds[1]);
    src_uri->clear();
    char buffer[8192];
    for (;;) {
      ssize_t curr = read(fds[0], buffer, sizeof(buffer));
      if (curr > 0) {
        src_uri->append(buffer, static_cast<size_t>(curr));
        continue;
      }
      if (curr == 0) {
        break;
      }
      if (errno == EINTR) {
        continue;
      }
      close(fds[0]);
      int status;
      while ((waitpid(child, &status, 0) == -1) && (errno == EINTR)) {
      }
      return false;
    }
    close(fds[0]);

    int status;
    while ((waitpid(child, &status, 0) == -1) && (errno == EINTR)) {
    }
    if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0)) {
      return false;
    }

    while (!src_uri->empty() && ((src_uri->back() == '\n') || (src_uri->back() == '\r'))) {
      src_uri->erase(src_uri->size() - 1);
    }
    return true;
  }

  bool read_ebuild_vars(const std::string& ebuild_source,
                        VarsReader::Flags flags,
                        WordIterateMap* vars,
                        std::string* errtext) const {
    (void)flags;
    if (errtext != NULLPTR) {
      errtext->clear();
    }

    std::istringstream in(ebuild_source);
    std::string line;
    while (std::getline(in, line)) {
      std::string stripped(line);
      trim(&stripped);
      if (stripped.empty() || stripped[0] == '#') {
        continue;
      }
      if (is_function_definition_line(stripped)) {
        break;
      }

      std::string assignment_line;
      if (rewrite_simple_short_circuit_assignment(stripped, *vars, &assignment_line)) {
        if (assignment_line.empty()) {
          continue;
        }
      } else {
        assignment_line = line;
      }

      if (!read_preamble_assignment(assignment_line, &in, vars)) {
        continue;
      }
    }
    return true;
  }

  bool rewrite_simple_short_circuit_assignment(const std::string& stripped,
                                               const WordIterateMap& vars,
                                               std::string* assignment_line) const {
    if (stripped.find("[[") != 0) {
      return false;
    }
    const std::string::size_type cond_end = stripped.find("]]");
    if (cond_end == std::string::npos) {
      return false;
    }
    const std::string::size_type and_pos = stripped.find("&&", cond_end + 2);
    if (and_pos == std::string::npos) {
      return false;
    }

    bool result = false;
    if (!evaluate_simple_test_condition(stripped.substr(2, cond_end - 2), vars, &result)) {
      return false;
    }
    if (!result) {
      assignment_line->clear();
      return true;
    }
    assignment_line->assign(stripped, and_pos + 2, std::string::npos);
    trim(assignment_line);
    return true;
  }

  bool evaluate_simple_test_condition(const std::string& condition,
                                      const WordIterateMap& vars,
                                      bool* result) const {
    WordVec words = split_string(condition);
    if (words.size() != 3) {
      return false;
    }

    std::string lhs;
    std::string rhs;
    if (!expand_shell_value(words[0], vars, &lhs) ||
        !expand_shell_value(words[2], vars, &rhs)) {
      return false;
    }

    const bool matches = (fnmatch(rhs.c_str(), lhs.c_str(), 0) == 0);
    if ((words[1] == "=") || (words[1] == "==")) {
      *result = matches;
      return true;
    }
    if (words[1] == "!=") {
      *result = !matches;
      return true;
    }
    return false;
  }

  bool read_preamble_assignment(const std::string& line,
                                std::istringstream* in,
                                WordIterateMap* vars) const {
    std::string::size_type curr = 0;
    while ((curr < line.size()) && ((line[curr] == ' ') || (line[curr] == '\t'))) {
      ++curr;
    }
    if (curr >= line.size()) {
      return false;
    }

    if (line.compare(curr, 7, "export ") == 0) {
      curr += 7;
      while ((curr < line.size()) && ((line[curr] == ' ') || (line[curr] == '\t'))) {
        ++curr;
      }
    }
    if ((curr >= line.size()) || !is_valid_shell_name_start(line[curr])) {
      return false;
    }

    const std::string::size_type name_begin = curr;
    while ((curr < line.size()) && is_valid_shell_name_char(line[curr])) {
      ++curr;
    }
    const std::string::size_type name_end = curr;
    if (curr >= line.size()) {
      return false;
    }

    bool append = false;
    if (line[curr] == '+') {
      append = true;
      ++curr;
    }
    if ((curr >= line.size()) || (line[curr] != '=')) {
      return false;
    }
    ++curr;
    while ((curr < line.size()) && ((line[curr] == ' ') || (line[curr] == '\t'))) {
      ++curr;
    }
    if ((curr < line.size()) && (line[curr] == '(')) {
      return skip_array_assignment(line, curr, in);
    }

    std::string value;
    if (!read_shell_assignment_value(line, curr, in, &value)) {
      return false;
    }

    const std::string name(line, name_begin, name_end - name_begin);
    if (append) {
      (*vars)[name].append(value);
    } else {
      (*vars)[name] = value;
    }
    return true;
  }

  bool skip_array_assignment(const std::string& first_line,
                             std::string::size_type begin,
                             std::istringstream* in) const {
    enum QuoteState {
      ARRAY_QUOTE_NONE,
      ARRAY_QUOTE_SINGLE,
      ARRAY_QUOTE_DOUBLE
    };

    std::string line(first_line);
    std::string::size_type curr = begin;
    QuoteState quote = ARRAY_QUOTE_NONE;
    unsigned depth = 0;

    for (;;) {
      for (; curr < line.size(); ++curr) {
        const char ch = line[curr];
        if (quote == ARRAY_QUOTE_SINGLE) {
          if (ch == '\'') {
            quote = ARRAY_QUOTE_NONE;
          }
          continue;
        }
        if (quote == ARRAY_QUOTE_DOUBLE) {
          if ((ch == '\\') && (curr + 1 < line.size())) {
            ++curr;
            continue;
          }
          if (ch == '"') {
            quote = ARRAY_QUOTE_NONE;
          }
          continue;
        }

        if (ch == '\'') {
          quote = ARRAY_QUOTE_SINGLE;
          continue;
        }
        if (ch == '"') {
          quote = ARRAY_QUOTE_DOUBLE;
          continue;
        }
        if (ch == '(') {
          ++depth;
          continue;
        }
        if (ch == ')') {
          if (depth == 0) {
            return false;
          }
          if (--depth == 0) {
            return true;
          }
        }
      }
      if (!std::getline(*in, line)) {
        return false;
      }
      curr = 0;
    }
  }

  bool read_file_to_string(const std::string& path, std::string* contents) const {
    std::ifstream in(path.c_str());
    if (!in) {
      return false;
    }
    contents->assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return true;
  }

  void initialize_src_uri_vars(const std::string& portdir,
                               const std::string& pkg_path,
                               const std::string& category,
                               const std::string& pn,
                               const std::string& pvr,
                               const std::string& ebuild_path,
                               WordIterateMap* vars) const {
    vars->clear();
    (*vars)["PORTDIR"] = portdir;
    (*vars)["CATEGORY"] = category;
    (*vars)["PN"] = pn;
    (*vars)["PVR"] = pvr;
    (*vars)["PF"] = pn + "-" + pvr;
    (*vars)["EBUILD"] = ebuild_path;
    (*vars)["O"] = pkg_path;
    (*vars)["FILESDIR"] = pkg_path + "/files";

    std::string mainversion(pvr);
    const std::string::size_type revision_pos = revision_index(pvr);
    if (revision_pos == std::string::npos) {
      (*vars)["PR"] = "r0";
    } else {
      (*vars)["PR"] = pvr.substr(revision_pos + 1);
      mainversion.erase(revision_pos);
    }
    (*vars)["PV"] = mainversion;
    (*vars)["P"] = pn + "-" + mainversion;
    (*vars)["COREQ_ORIG_PV"] = mainversion;
  }

  void strip_noop_quoted_blocks(std::string* ebuild_source) const {
    std::istringstream in(*ebuild_source);
    std::string filtered;
    std::string line;
    bool in_block = false;
    char quote = '\0';

    while (std::getline(in, line)) {
      std::string stripped(line);
      trim(&stripped);

      if (in_block) {
        if (stripped.size() == 1 && stripped[0] == quote) {
          in_block = false;
        }
        continue;
      }

      if (!stripped.empty() && stripped[0] == ':') {
        std::string::size_type quote_pos = 1;
        while ((quote_pos < stripped.size()) && ((stripped[quote_pos] == ' ') || (stripped[quote_pos] == '\t'))) {
          ++quote_pos;
        }
        if ((quote_pos + 1 == stripped.size()) &&
            ((stripped[quote_pos] == '\'') || (stripped[quote_pos] == '"'))) {
          in_block = true;
          quote = stripped[quote_pos];
          continue;
        }
      }

      filtered.append(line);
      filtered.push_back('\n');
    }
    ebuild_source->swap(filtered);
  }

  void prune_simple_live_conditionals(const std::string& pvr, std::string* ebuild_source) const {
    std::istringstream in(*ebuild_source);
    std::string filtered;
    std::string line;
    while (std::getline(in, line)) {
      std::string stripped(line);
      trim(&stripped);
      if (is_function_definition_line(stripped)) {
        filtered.append(line);
        filtered.push_back('\n');
        while (std::getline(in, line)) {
          filtered.append(line);
          filtered.push_back('\n');
        }
        break;
      }
      if (!is_simple_live_conditional(stripped)) {
        filtered.append(line);
        filtered.push_back('\n');
        continue;
      }

      std::string then_block;
      std::string else_block;
      std::string* active = &then_block;
      unsigned depth = 1;
      bool failed = false;
      while (std::getline(in, line)) {
        stripped = line;
        trim(&stripped);
        if (stripped.compare(0, 2, "if") == 0) {
          ++depth;
        }
        if ((depth == 1) && (stripped == "else")) {
          active = &else_block;
          continue;
        }
        if (stripped == "fi") {
          if (--depth == 0) {
            break;
          }
        }
        active->append(line);
        active->push_back('\n');
      }
      if (depth != 0) {
        failed = true;
      }
      if (failed) {
        filtered.append("if [[ ${PV} == *9999* ]]; then\n");
        filtered.append(then_block);
        if (!else_block.empty()) {
          filtered.append("else\n");
          filtered.append(else_block);
        }
        filtered.append("fi\n");
        continue;
      }
      filtered.append(is_live_version(pvr) ? then_block : else_block);
    }
    ebuild_source->swap(filtered);
  }

  bool is_simple_live_conditional(const std::string& stripped) const {
    if ((stripped.find("if [[") != 0) || (stripped.find("${PV}") == std::string::npos)) {
      return false;
    }
    if ((stripped.find("9999") == std::string::npos) ||
        (stripped.find("!=") != std::string::npos) ||
        ((stripped.find("==") == std::string::npos) && (stripped.find(" = ") == std::string::npos))) {
      return false;
    }
    return (stripped.find("then") != std::string::npos);
  }

  bool is_live_version(const std::string& pvr) const {
    return (pvr == "9999") || ((pvr.size() >= 5) && (pvr.compare(pvr.size() - 5, 5, ".9999") == 0));
  }

  void strip_inline_shell_comment(std::string* value) const {
    for (std::string::size_type i = 0; i < value->size(); ++i) {
      if ((*value)[i] != '#') {
        continue;
      }
      if ((i == 0) || ((*value)[i - 1] == ' ') || ((*value)[i - 1] == '\t')) {
        value->erase(i);
        return;
      }
    }
  }

  bool extract_src_uri_expression(const std::string& ebuild_source,
                                  const std::string& inherited_src_uri,
                                  std::string* src_uri_expr) const {
    *src_uri_expr = inherited_src_uri;

    std::istringstream in(ebuild_source);
    std::string line;
    while (std::getline(in, line)) {
      std::string stripped(line);
      trim(&stripped);
      if (stripped.empty() || stripped[0] == '#') {
        continue;
      }
      if (is_function_definition_line(stripped)) {
        break;
      }

      const std::string::size_type pos = find_top_level_src_uri_token(line);
      if (pos == std::string::npos) {
        continue;
      }

      std::string::size_type curr = pos + 7;
      while ((curr < line.size()) && ((line[curr] == ' ') || (line[curr] == '\t'))) {
        ++curr;
      }
      bool append = false;
      if ((curr < line.size()) && (line[curr] == '=')) {
        ++curr;
      } else if ((curr + 1 < line.size()) && (line[curr] == '+') && (line[curr + 1] == '=')) {
        append = true;
        curr += 2;
      } else {
        return false;
      }

      std::string value;
      if (!read_shell_assignment_value(line, curr, &in, &value)) {
        return false;
      }
      if (append) {
        src_uri_expr->append(value);
      } else {
        *src_uri_expr = value;
      }
    }
    return true;
  }

  bool read_shell_assignment_value(const std::string& first_line,
                                   std::string::size_type begin,
                                   std::istringstream* in,
                                   std::string* value) const {
    std::string line(first_line);
    std::string::size_type curr = begin;
    while ((curr < line.size()) && ((line[curr] == ' ') || (line[curr] == '\t'))) {
      ++curr;
    }
    if (curr >= line.size()) {
      value->clear();
      return true;
    }

    value->clear();
    const char quote = line[curr];
    if (quote != '"' && quote != '\'') {
      value->assign(line, curr, std::string::npos);
      strip_inline_shell_comment(value);
      trim(value);
      return true;
    }

    ++curr;
    unsigned command_depth = 0;
    unsigned parameter_depth = 0;
    for (;;) {
      for (; curr < line.size(); ++curr) {
        if (quote == '"') {
          if ((line[curr] == '\\') && (curr + 1 < line.size())) {
            const char escaped = line[curr + 1];
            if ((escaped == '"') || (escaped == '\\') || (escaped == '$') || (escaped == '`')) {
              value->append(1, escaped);
              ++curr;
              continue;
            }
          }
          if ((line[curr] == '$') && (curr + 1 < line.size())) {
            if (line[curr + 1] == '(') {
              ++command_depth;
            } else if (line[curr + 1] == '{') {
              ++parameter_depth;
            }
          } else if ((line[curr] == ')') && (command_depth > 0)) {
            --command_depth;
          } else if ((line[curr] == '}') && (parameter_depth > 0)) {
            --parameter_depth;
          } else if ((line[curr] == '"') && (command_depth == 0) && (parameter_depth == 0)) {
            return true;
          }
          value->append(1, line[curr]);
          continue;
        }
        if (line[curr] == quote) {
          return true;
        }
        value->append(1, line[curr]);
      }
      if (!std::getline(*in, line)) {
        return false;
      }
      value->push_back('\n');
      curr = 0;
    }
  }

  void collect_inherited_eclasses(const std::string& ebuild_source, WordVec* eclasses) const {
    eclasses->clear();
    std::istringstream in(ebuild_source);
    std::string line;
    while (std::getline(in, line)) {
      std::string stripped(line);
      trim(&stripped);
      if (stripped.empty() || stripped[0] == '#') {
        continue;
      }
      if (is_function_definition_line(stripped)) {
        break;
      }
      if (stripped.compare(0, 7, "inherit") != 0 ||
          ((stripped.size() > 7) && stripped[7] != ' ' && stripped[7] != '\t')) {
        continue;
      }
      WordVec parts = split_string(stripped);
      for (WordVec::size_type i = 1; i < parts.size(); ++i) {
        eclasses->push_back(parts[i]);
      }
    }
  }

  bool compute_inherited_src_uri(const WordVec& eclasses,
                                 const WordIterateMap& vars,
                                 std::string* src_uri) const {
    src_uri->clear();
    for (WordVec::const_iterator it = eclasses.begin(); it != eclasses.end(); ++it) {
      if (*it == "pypi") {
        if (!compute_pypi_inherited_src_uri(vars, src_uri)) {
          return false;
        }
      } else if (*it == "tree-sitter-grammar") {
        if (!compute_tree_sitter_inherited_src_uri(vars, src_uri)) {
          return false;
        }
      } else if (*it == "perl-module") {
        if (!compute_perl_module_inherited_src_uri(vars, src_uri)) {
          return false;
        }
      }
    }
    return true;
  }

  bool has_supported_inherited_src_uri(const WordVec& eclasses) const {
    for (WordVec::const_iterator it = eclasses.begin(); it != eclasses.end(); ++it) {
      if ((*it == "pypi") || (*it == "tree-sitter-grammar") || (*it == "perl-module")) {
        return true;
      }
    }
    return false;
  }

  void initialize_inherited_helper_vars(const WordVec& eclasses,
                                        WordIterateMap* vars) const {
    for (WordVec::const_iterator it = eclasses.begin(); it != eclasses.end(); ++it) {
      if ((*it == "cargo") && (vars->find("CARGO_CRATE_URIS") == vars->end())) {
        (*vars)["CARGO_CRATE_URIS"] = "";
      }
    }
  }

  bool compute_pypi_inherited_src_uri(const WordIterateMap& vars, std::string* src_uri) const {
    std::string project;
    if (!get_pypi_project(vars, &project)) {
      return false;
    }

    std::string version;
    if (!pypi_translate_version(get_var_or_empty(vars, "PV"), &version)) {
      return false;
    }

    const bool normalize = get_var_or_empty(vars, "PYPI_NO_NORMALIZE").empty();
    return pypi_sdist_url(project, version, ".tar.gz", normalize, src_uri);
  }

  bool compute_tree_sitter_inherited_src_uri(const WordIterateMap& vars, std::string* src_uri) const {
    return expand_shell_value("https://github.com/tree-sitter/${PN}/archive/${TS_PV:-v${PV}}.tar.gz -> ${P}.tar.gz", vars, src_uri);
  }

  bool compute_perl_module_inherited_src_uri(const WordIterateMap& vars, std::string* src_uri) const {
    if (!get_var_or_empty(vars, "DIST_AUTHOR").empty()) {
      WordIterateMap perl_vars(vars);
      if (get_var_or_empty(perl_vars, "DIST_NAME").empty()) {
        perl_vars["DIST_NAME"] = get_var_or_empty(vars, "PN");
      }
      if (get_var_or_empty(perl_vars, "DIST_P").empty()) {
        std::string dist_version = get_var_or_empty(perl_vars, "DIST_VERSION");
        if (dist_version.empty()) {
          dist_version = get_var_or_empty(vars, "PV");
        }
        perl_vars["DIST_P"] = perl_vars["DIST_NAME"] + "-" + dist_version;
      }
      if (get_var_or_empty(perl_vars, "DIST_A").empty()) {
        std::string ext = get_var_or_empty(perl_vars, "DIST_A_EXT");
        if (ext.empty()) {
          ext = "tar.gz";
        }
        perl_vars["DIST_A"] = perl_vars["DIST_P"] + "." + ext;
      }
      return expand_shell_value(
          "mirror://cpan/authors/id/${DIST_AUTHOR:0:1}/${DIST_AUTHOR:0:2}/${DIST_AUTHOR}/${DIST_SECTION:+${DIST_SECTION}/}${DIST_A}",
          perl_vars, src_uri);
    }
    src_uri->clear();
    return true;
  }

  std::string get_var_or_empty(const WordIterateMap& vars, const std::string& name) const {
    const WordIterateMap::const_iterator it = vars.find(name);
    if (it == vars.end()) {
      return std::string();
    }
    return it->second;
  }

  std::string get_default_package_pv(const WordIterateMap& vars) const {
    const std::string original = get_var_or_empty(vars, "COREQ_ORIG_PV");
    if (!original.empty()) {
      return original;
    }
    return get_var_or_empty(vars, "PV");
  }

  bool expand_shell_value(const std::string& value,
                          const WordIterateMap& vars,
                          std::string* expanded) const {
    std::set<std::string> var_stack;
    return expand_shell_value(value, vars, &var_stack, expanded);
  }

  bool expand_shell_value(const std::string& value,
                          const WordIterateMap& vars,
                          std::set<std::string>* var_stack,
                          std::string* expanded) const {
    expanded->clear();
    for (std::string::size_type i = 0; i < value.size(); ++i) {
      if (value[i] != '$') {
        expanded->append(1, value[i]);
        continue;
      }
      if (++i >= value.size()) {
        return false;
      }

      std::string piece;
      if (value[i] == '{') {
        std::string expr;
        if (!extract_parameter_expansion(value, i + 1, &i, &expr) ||
            !expand_parameter_expansion(expr, vars, var_stack, &piece)) {
          return false;
        }
      } else if (value[i] == '(') {
        std::string command;
        if (!extract_command_substitution(value, i + 1, &i, &command) ||
            !expand_command_substitution(command, vars, var_stack, &piece)) {
          return false;
        }
      } else {
        if (!is_valid_shell_var_start(value[i])) {
          return false;
        }
        const std::string::size_type begin = i;
        while ((i + 1 < value.size()) && is_valid_shell_var_char(value[i + 1])) {
          ++i;
        }
        if (!expand_shell_variable(value.substr(begin, i - begin + 1), vars, var_stack, &piece)) {
          return false;
        }
      }
      expanded->append(piece);
    }
    return true;
  }

  bool expand_shell_variable(const std::string& varname,
                             const WordIterateMap& vars,
                             std::set<std::string>* var_stack,
                             std::string* expanded) const {
    if (var_stack->find(varname) != var_stack->end()) {
      return false;
    }
    const WordIterateMap::const_iterator it = vars.find(varname);
    if (it == vars.end()) {
      return false;
    }
    var_stack->insert(varname);
    const bool ok = expand_shell_value(it->second, vars, var_stack, expanded);
    var_stack->erase(varname);
    return ok;
  }

  bool extract_parameter_expansion(const std::string& value,
                                   std::string::size_type begin,
                                   std::string::size_type* end,
                                   std::string* expr) const {
    unsigned depth = 1;
    for (std::string::size_type i = begin; i < value.size(); ++i) {
      if ((value[i] == '$') && (i + 1 < value.size())) {
        if (value[i + 1] == '{') {
          ++depth;
          ++i;
          continue;
        }
        if (value[i + 1] == '(') {
          std::string nested;
          std::string::size_type nested_end = i + 1;
          if (!extract_command_substitution(value, i + 1, &nested_end, &nested)) {
            return false;
          }
          i = nested_end;
          continue;
        }
      }
      if (value[i] == '}') {
        if (--depth == 0) {
          expr->assign(value, begin, i - begin);
          *end = i;
          return true;
        }
      }
    }
    return false;
  }

  bool extract_command_substitution(const std::string& value,
                                    std::string::size_type begin,
                                    std::string::size_type* end,
                                    std::string* expr) const {
    unsigned depth = 1;
    for (std::string::size_type i = begin; i < value.size(); ++i) {
      if ((value[i] == '$') && (i + 1 < value.size())) {
        if (value[i + 1] == '(') {
          ++depth;
          ++i;
          continue;
        }
        if (value[i + 1] == '{') {
          std::string nested;
          std::string::size_type nested_end = i + 1;
          if (!extract_parameter_expansion(value, i + 1, &nested_end, &nested)) {
            return false;
          }
          i = nested_end;
          continue;
        }
      }
      if (value[i] == ')') {
        if (--depth == 0) {
          expr->assign(value, begin, i - begin);
          *end = i;
          return true;
        }
      }
    }
    return false;
  }

  bool expand_parameter_expansion(const std::string& expr,
                                  const WordIterateMap& vars,
                                  std::set<std::string>* var_stack,
                                  std::string* expanded) const {
    if (expr.empty() || !is_valid_shell_var_start(expr[0])) {
      return false;
    }

    std::string::size_type name_end = 1;
    while ((name_end < expr.size()) && is_valid_shell_var_char(expr[name_end])) {
      ++name_end;
    }
    const std::string varname(expr, 0, name_end);
    const bool has_var = (vars.find(varname) != vars.end());
    std::string value;
    if (has_var && !expand_shell_variable(varname, vars, var_stack, &value)) {
      return false;
    }
    if (name_end == expr.size()) {
      if (!has_var) {
        return false;
      }
      *expanded = value;
      return true;
    }

    const std::string suffix(expr, name_end);
    if (suffix == "^") {
      if (!value.empty()) {
        value[0] = static_cast<char>(my_toupper(value[0]));
      }
      *expanded = value;
      return true;
    }
    if (suffix == ",,") {
      for (std::string::size_type i = 0; i < value.size(); ++i) {
        value[i] = static_cast<char>(my_tolower(value[i]));
      }
      *expanded = value;
      return true;
    }
    if (suffix.compare(0, 2, ":-") == 0) {
      if (has_var && !value.empty()) {
        *expanded = value;
        return true;
      }
      return expand_shell_value(suffix.substr(2), vars, var_stack, expanded);
    }
    if (suffix.compare(0, 2, ":+") == 0) {
      if (!has_var || value.empty()) {
        expanded->clear();
        return true;
      }
      return expand_shell_value(suffix.substr(2), vars, var_stack, expanded);
    }
    if (!suffix.empty() && ((suffix[0] == '#') || (suffix[0] == '%'))) {
      const bool remove_prefix = (suffix[0] == '#');
      const bool longest = (suffix.size() > 1) && (suffix[1] == suffix[0]);
      const std::string::size_type pattern_begin = longest ? 2 : 1;
      std::string pattern;
      if (!expand_shell_value(suffix.substr(pattern_begin), vars, var_stack, &pattern)) {
        return false;
      }
      return apply_shell_trim_operator(value, pattern, remove_prefix, longest, expanded);
    }
    if (!suffix.empty() && suffix[0] == '/') {
      const std::string::size_type second_slash = suffix.find('/', 1);
      std::string pattern;
      std::string replacement;
      if (second_slash == std::string::npos) {
        if (!expand_shell_value(suffix.substr(1), vars, var_stack, &pattern)) {
          return false;
        }
        replacement.clear();
      } else if (!expand_shell_value(suffix.substr(1, second_slash - 1), vars, var_stack, &pattern) ||
                 !expand_shell_value(suffix.substr(second_slash + 1), vars, var_stack, &replacement)) {
        return false;
      }
      const std::string::size_type match_pos = value.find(pattern);
      if (match_pos != std::string::npos) {
        value.replace(match_pos, pattern.size(), replacement);
      }
      *expanded = value;
      return true;
    }
    if (!suffix.empty() && suffix[0] == ':') {
      std::string offset_text;
      std::string length_text;
      if ((suffix.size() > 1) && suffix[1] == ':') {
        offset_text = "0";
        length_text.assign(suffix, 2, std::string::npos);
      } else {
        const std::string::size_type second_colon = suffix.find(':', 1);
        if (second_colon == std::string::npos) {
          offset_text.assign(suffix, 1, std::string::npos);
        } else {
          offset_text.assign(suffix, 1, second_colon - 1);
          length_text.assign(suffix, second_colon + 1, std::string::npos);
        }
      }

      int offset = 0;
      int length = -1;
      if (!parse_nonnegative_int(offset_text, &offset) ||
          (!length_text.empty() && !parse_nonnegative_int(length_text, &length))) {
        return false;
      }
      if (static_cast<std::string::size_type>(offset) >= value.size()) {
        expanded->clear();
        return true;
      }
      if (length < 0) {
        *expanded = value.substr(static_cast<std::string::size_type>(offset));
      } else {
        *expanded = value.substr(static_cast<std::string::size_type>(offset), static_cast<std::string::size_type>(length));
      }
      return true;
    }
    return false;
  }

  bool apply_shell_trim_operator(const std::string& value,
                                 const std::string& pattern,
                                 bool remove_prefix,
                                 bool longest,
                                 std::string* expanded) const {
    if (remove_prefix) {
      if (longest) {
        for (std::string::size_type i = value.size(); ; --i) {
          if (fnmatch(pattern.c_str(), value.substr(0, i).c_str(), 0) == 0) {
            *expanded = value.substr(i);
            return true;
          }
          if (i == 0) {
            break;
          }
        }
      } else {
        for (std::string::size_type i = 0; i <= value.size(); ++i) {
          if (fnmatch(pattern.c_str(), value.substr(0, i).c_str(), 0) == 0) {
            *expanded = value.substr(i);
            return true;
          }
        }
      }
    } else if (longest) {
      for (std::string::size_type i = 0; i <= value.size(); ++i) {
        if (fnmatch(pattern.c_str(), value.substr(i).c_str(), 0) == 0) {
          *expanded = value.substr(0, i);
          return true;
        }
      }
    } else {
      for (std::string::size_type i = value.size(); ; --i) {
        if (fnmatch(pattern.c_str(), value.substr(i).c_str(), 0) == 0) {
          *expanded = value.substr(0, i);
          return true;
        }
        if (i == 0) {
          break;
        }
      }
    }

    *expanded = value;
    return true;
  }

  bool parse_nonnegative_int(const std::string& text, int* value) const {
    if (text.empty()) {
      return false;
    }
    int result = 0;
    for (std::string::size_type i = 0; i < text.size(); ++i) {
      if (!my_isdigit(text[i])) {
        return false;
      }
      result = result * 10 + (text[i] - '0');
    }
    *value = result;
    return true;
  }

  bool expand_command_substitution(const std::string& command,
                                   const WordIterateMap& vars,
                                   std::set<std::string>* var_stack,
                                   std::string* expanded) const {
    WordVec words;
    if (!split_shell_words(command, vars, var_stack, &words) || words.empty()) {
      return false;
    }

    const std::string& helper = words[0];
    if (helper == "ver_cut") {
      return expand_ver_cut(words, vars, expanded);
    }
    if (helper == "ver_rs") {
      return expand_ver_rs(words, vars, expanded);
    }
    if (helper == "pypi_translate_version") {
      return expand_pypi_translate_version(words, vars, expanded);
    }
    if (helper == "pypi_normalize_name") {
      return expand_pypi_normalize_name(words, vars, expanded);
    }
    if (helper == "pypi_sdist_url") {
      return expand_pypi_sdist_url(words, vars, expanded);
    }
    if (helper == "pypi_wheel_name") {
      return expand_pypi_wheel_name(words, vars, expanded);
    }
    if (helper == "pypi_wheel_url") {
      return expand_pypi_wheel_url(words, vars, expanded);
    }
    return false;
  }

  bool split_shell_words(const std::string& text,
                         const WordIterateMap& vars,
                         std::set<std::string>* var_stack,
                         WordVec* words) const {
    enum QuoteState {
      QUOTE_NONE,
      QUOTE_SINGLE,
      QUOTE_DOUBLE
    };

    words->clear();
    QuoteState quote = QUOTE_NONE;
    std::string current;

    for (std::string::size_type i = 0; i < text.size(); ++i) {
      const char ch = text[i];
      if (quote == QUOTE_SINGLE) {
        if (ch == '\'') {
          quote = QUOTE_NONE;
        } else {
          current.append(1, ch);
        }
        continue;
      }
      if (quote == QUOTE_DOUBLE) {
        if (ch == '"') {
          quote = QUOTE_NONE;
          continue;
        }
        if ((ch == '\\') && (i + 1 < text.size())) {
          const char escaped = text[i + 1];
          if ((escaped == '"') || (escaped == '\\') || (escaped == '$') || (escaped == '`')) {
            current.append(1, escaped);
            ++i;
            continue;
          }
        }
        if (ch == '$') {
          std::string piece;
          if (++i >= text.size()) {
            return false;
          }
          if (text[i] == '{') {
            std::string expr;
            if (!extract_parameter_expansion(text, i + 1, &i, &expr) ||
                !expand_parameter_expansion(expr, vars, var_stack, &piece)) {
              return false;
            }
          } else if (text[i] == '(') {
            std::string command;
            if (!extract_command_substitution(text, i + 1, &i, &command) ||
                !expand_command_substitution(command, vars, var_stack, &piece)) {
              return false;
            }
          } else {
            if (!is_valid_shell_var_start(text[i])) {
              return false;
            }
            const std::string::size_type begin = i;
            while ((i + 1 < text.size()) && is_valid_shell_var_char(text[i + 1])) {
              ++i;
            }
            if (!expand_shell_variable(text.substr(begin, i - begin + 1), vars, var_stack, &piece)) {
              return false;
            }
          }
          current.append(piece);
        } else {
          current.append(1, ch);
        }
        continue;
      }

      if ((ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\r')) {
        if (!current.empty()) {
          words->push_back(current);
          current.clear();
        }
        continue;
      }
      if (ch == '\'') {
        quote = QUOTE_SINGLE;
        continue;
      }
      if (ch == '"') {
        quote = QUOTE_DOUBLE;
        continue;
      }
      if ((ch == '\\') && (i + 1 < text.size())) {
        current.append(1, text[i + 1]);
        ++i;
        continue;
      }
      if (ch == '$') {
        std::string piece;
        if (++i >= text.size()) {
          return false;
        }
        if (text[i] == '{') {
          std::string expr;
          if (!extract_parameter_expansion(text, i + 1, &i, &expr) ||
              !expand_parameter_expansion(expr, vars, var_stack, &piece)) {
            return false;
          }
        } else if (text[i] == '(') {
          std::string command;
          if (!extract_command_substitution(text, i + 1, &i, &command) ||
              !expand_command_substitution(command, vars, var_stack, &piece)) {
            return false;
          }
        } else {
          if (!is_valid_shell_var_start(text[i])) {
            return false;
          }
          const std::string::size_type begin = i;
          while ((i + 1 < text.size()) && is_valid_shell_var_char(text[i + 1])) {
            ++i;
          }
          if (!expand_shell_variable(text.substr(begin, i - begin + 1), vars, var_stack, &piece)) {
            return false;
          }
        }
        current.append(piece);
        continue;
      }
      current.append(1, ch);
    }

    if (quote != QUOTE_NONE) {
      return false;
    }
    if (!current.empty()) {
      words->push_back(current);
    }
    return true;
  }

  bool expand_ver_cut(const WordVec& words,
                      const WordIterateMap& vars,
                      std::string* expanded) const {
    if (words.size() < 2 || words.size() > 3) {
      return false;
    }

    std::string version = (words.size() == 3) ? words[2] : get_default_package_pv(vars);
    if (version.empty()) {
      return false;
    }

    WordVec parts;
    split_version_components(version, &parts);
    const int max_component = static_cast<int>(parts.size() / 2);

    int start = 0;
    int end = 0;
    if (!parse_ver_cut_range(words[1], max_component, &start, &end)) {
      return false;
    }

    std::ostringstream out;
    int begin_index = 0;
    if (start > 0) {
      begin_index = start * 2 - 1;
    }
    const int count = end * 2 - begin_index;
    const int end_index = begin_index + count - 1;
    for (int index = begin_index; index <= end_index && index < static_cast<int>(parts.size()); ++index) {
      out << parts[static_cast<WordVec::size_type>(index)];
    }
    *expanded = out.str();
    return true;
  }

  bool expand_ver_rs(const WordVec& words,
                     const WordIterateMap& vars,
                     std::string* expanded) const {
    if (words.size() < 3) {
      return false;
    }

    std::string version = get_default_package_pv(vars);
    WordVec args(words);
    if ((args.size() % 2) == 0) {
      version = args.back();
      args.pop_back();
    }
    if (version.empty()) {
      return false;
    }

    WordVec parts;
    split_version_components(version, &parts);
    const int max_component = static_cast<int>(parts.size() / 2) - 1;

    for (WordVec::size_type i = 1; i + 1 < args.size(); i += 2) {
      int start = 0;
      int end = 0;
      if (!parse_ver_cut_range(args[i], max_component, &start, &end)) {
        return false;
      }
      for (int component = start; component <= end; ++component) {
        const int index = component * 2;
        if ((index == 0) && parts[0].empty()) {
          continue;
        }
        if ((index >= 0) && (index < static_cast<int>(parts.size()))) {
          parts[static_cast<WordVec::size_type>(index)] = args[i + 1];
        }
      }
    }

    expanded->clear();
    for (WordVec::const_iterator it = parts.begin(); it != parts.end(); ++it) {
      expanded->append(*it);
    }
    return true;
  }

  void split_version_components(const std::string& version, WordVec* parts) const {
    parts->clear();
    std::string::size_type i = 0;
    while (i < version.size()) {
      const std::string::size_type separator_begin = i;
      while ((i < version.size()) && !my_isalnum(version[i])) {
        ++i;
      }
      parts->push_back(version.substr(separator_begin, i - separator_begin));

      const std::string::size_type component_begin = i;
      if ((i < version.size()) && my_isdigit(version[i])) {
        while ((i < version.size()) && my_isdigit(version[i])) {
          ++i;
        }
      } else {
        while ((i < version.size()) && my_isalpha(version[i])) {
          ++i;
        }
      }
      parts->push_back(version.substr(component_begin, i - component_begin));
    }
  }

  bool parse_ver_cut_range(const std::string& range, int max_component, int* start, int* end) const {
    if (range.empty() || !my_isdigit(range[0])) {
      return false;
    }
    const std::string::size_type dash = range.find('-');
    const std::string start_text = (dash == std::string::npos) ? range : range.substr(0, dash);
    const std::string end_text = (dash == std::string::npos) ? start_text : range.substr(dash + 1);

    if (!parse_nonnegative_int(start_text, start)) {
      return false;
    }
    if (end_text.empty()) {
      *end = max_component;
    } else if (!parse_nonnegative_int(end_text, end)) {
      return false;
    }
    if (*start > *end) {
      return false;
    }
    if (*end > max_component) {
      *end = max_component;
    }
    return true;
  }

  bool expand_pypi_translate_version(const WordVec& words,
                                     const WordIterateMap& vars,
                                     std::string* expanded) const {
    if (words.size() > 2) {
      return false;
    }
    const std::string version = (words.size() == 2) ? words[1] : get_var_or_empty(vars, "PV");
    return pypi_translate_version(version, expanded);
  }

  bool pypi_translate_version(const std::string& version, std::string* translated) const {
    if (version.empty()) {
      return false;
    }
    *translated = version;
    replace_first(translated, "_alpha", "a");
    replace_first(translated, "_beta", "b");
    replace_first(translated, "_pre", ".dev");
    replace_first(translated, "_rc", "rc");
    replace_first(translated, "_p", ".post");
    return true;
  }

  void replace_first(std::string* text, const std::string& from, const std::string& to) const {
    const std::string::size_type pos = text->find(from);
    if (pos != std::string::npos) {
      text->replace(pos, from.size(), to);
    }
  }

  bool expand_pypi_normalize_name(const WordVec& words,
                                  const WordIterateMap&,
                                  std::string* expanded) const {
    if (words.size() != 2) {
      return false;
    }
    return pypi_normalize_name(words[1], expanded);
  }

  bool pypi_normalize_name(const std::string& project, std::string* normalized) const {
    if (project.empty()) {
      return false;
    }
    normalized->clear();
    bool last_was_separator = false;
    for (std::string::size_type i = 0; i < project.size(); ++i) {
      const char ch = project[i];
      if ((ch == '.') || (ch == '_') || (ch == '-')) {
        if (!last_was_separator) {
          normalized->append(1, '_');
          last_was_separator = true;
        }
      } else {
        normalized->append(1, static_cast<char>(my_tolower(ch)));
        last_was_separator = false;
      }
    }
    return true;
  }

  bool get_pypi_project(const WordIterateMap& vars, std::string* project) const {
    const WordIterateMap::const_iterator it = vars.find("PYPI_PN");
    if (it == vars.end() || it->second.empty()) {
      *project = get_var_or_empty(vars, "PN");
      return !project->empty();
    }
    return expand_shell_value(it->second, vars, project);
  }

  bool expand_pypi_sdist_url(const WordVec& words,
                             const WordIterateMap& vars,
                             std::string* expanded) const {
    bool normalize = true;
    WordVec::size_type arg = 1;
    if ((arg < words.size()) && (words[arg] == "--no-normalize")) {
      normalize = false;
      ++arg;
    }
    if (words.size() - arg > 3) {
      return false;
    }

    std::string project;
    if (arg < words.size()) {
      project = words[arg++];
    } else if (!get_pypi_project(vars, &project)) {
      return false;
    }

    std::string version;
    if (arg < words.size()) {
      version = words[arg++];
    } else if (!pypi_translate_version(get_var_or_empty(vars, "PV"), &version)) {
      return false;
    }

    const std::string suffix = (arg < words.size()) ? words[arg] : ".tar.gz";
    return pypi_sdist_url(project, version, suffix, normalize, expanded);
  }

  bool pypi_sdist_url(const std::string& project,
                      const std::string& version,
                      const std::string& suffix,
                      bool normalize,
                      std::string* url) const {
    if (project.empty() || version.empty() || suffix.empty()) {
      return false;
    }
    std::string filename_project(project);
    if (normalize && !pypi_normalize_name(project, &filename_project)) {
      return false;
    }
    *url = "https://files.pythonhosted.org/packages/source/";
    url->append(1, project[0]);
    *url += "/" + project + "/" + filename_project + "-" + version + suffix;
    return true;
  }

  bool expand_pypi_wheel_name(const WordVec& words,
                              const WordIterateMap& vars,
                              std::string* expanded) const {
    if (words.size() > 5) {
      return false;
    }

    std::string project;
    if (words.size() > 1) {
      project = words[1];
    } else if (!get_pypi_project(vars, &project)) {
      return false;
    }

    std::string normalized;
    if (!pypi_normalize_name(project, &normalized)) {
      return false;
    }

    std::string version;
    if (words.size() > 2) {
      version = words[2];
    } else if (!pypi_translate_version(get_var_or_empty(vars, "PV"), &version)) {
      return false;
    }

    const std::string pytag = (words.size() > 3) ? words[3] : "py3";
    const std::string abitag = (words.size() > 4) ? words[4] : "none-any";
    *expanded = normalized + "-" + version + "-" + pytag + "-" + abitag + ".whl";
    return true;
  }

  bool expand_pypi_wheel_url(const WordVec& words,
                             const WordIterateMap& vars,
                             std::string* expanded) const {
    bool unpack = false;
    WordVec args(words);
    if ((args.size() > 1) && (args[1] == "--unpack")) {
      unpack = true;
      args.erase(args.begin() + 1);
    }
    if (args.size() > 5) {
      return false;
    }

    std::string project;
    if (args.size() > 1) {
      project = args[1];
    } else if (!get_pypi_project(vars, &project)) {
      return false;
    }

    std::string filename;
    if (!expand_pypi_wheel_name(args, vars, &filename)) {
      return false;
    }

    const std::string pytag = (args.size() > 3) ? args[3] : "py3";
    *expanded = "https://files.pythonhosted.org/packages/" + pytag + "/";
    expanded->append(1, project[0]);
    *expanded += "/" + project + "/" + filename;
    if (unpack) {
      *expanded += " -> " + filename + ".zip";
    }
    return true;
  }

  bool has_unsafe_src_uri_assignment(const std::string& ebuild_source,
                                     bool allow_append_without_plain) const {
    std::istringstream in(ebuild_source);
    std::string line;
    bool seen_plain_src_uri = false;
    while (std::getline(in, line)) {
      std::string stripped(line);
      trim(&stripped);
      if (stripped.empty() || stripped[0] == '#') {
        continue;
      }
      if (is_function_definition_line(stripped)) {
        break;
      }

      const std::string::size_type pos = find_top_level_src_uri_token(line);
      if (pos == std::string::npos) {
        continue;
      }

      std::string::size_type curr = pos + 7;
      while (curr < line.size() && ((line[curr] == ' ') || (line[curr] == '\t'))) {
        ++curr;
      }
      if (curr >= line.size()) {
        continue;
      }
      if (line[curr] == '=') {
        seen_plain_src_uri = true;
        continue;
      }
      if (line[curr] == '+' && (curr + 1 < line.size()) && line[curr + 1] == '=') {
        if (!seen_plain_src_uri && !allow_append_without_plain) {
          return true;
        }
        continue;
      }
      return true;
    }
    return false;
  }

  bool has_unsafe_preamble_control_flow(const std::string& ebuild_source) const {
    std::istringstream in(ebuild_source);
    std::string line;
    bool seen_src_uri = false;
    bool seen_control_after_src_uri = false;
    while (std::getline(in, line)) {
      std::string stripped(line);
      trim(&stripped);
      if (stripped.empty() || stripped[0] == '#') {
        continue;
      }
      if (is_function_definition_line(stripped)) {
        break;
      }
      if (find_top_level_src_uri_token(line) != std::string::npos) {
        if (seen_control_after_src_uri) {
          return true;
        }
        seen_src_uri = true;
      }
      if ((stripped.compare(0, 2, "if") == 0) ||
          (stripped.compare(0, 4, "elif") == 0) ||
          (stripped.compare(0, 4, "else") == 0) ||
          (stripped.compare(0, 4, "case") == 0) ||
          (stripped.compare(0, 3, "for") == 0) ||
          (stripped.compare(0, 5, "while") == 0) ||
          (stripped.find("[[") != std::string::npos)) {
        if (!seen_src_uri) {
          return true;
        }
        seen_control_after_src_uri = true;
      }
    }
    return false;
  }

  void normalize_src_uri_appends(std::string* ebuild_source) const {
    std::istringstream in(*ebuild_source);
    std::string normalized;
    std::string line;
    bool in_functions = false;
    while (std::getline(in, line)) {
      std::string stripped(line);
      trim(&stripped);
      if (!in_functions && is_function_definition_line(stripped)) {
        in_functions = true;
      }
      if (!in_functions) {
        const std::string::size_type pos = find_top_level_src_uri_token(line);
        if (pos != std::string::npos) {
          std::string::size_type curr = pos + 7;
          while (curr < line.size() && ((line[curr] == ' ') || (line[curr] == '\t'))) {
            ++curr;
          }
          if ((curr + 1 < line.size()) && line[curr] == '+' && line[curr + 1] == '=') {
            const std::string prefix = line.substr(0, pos);
            const std::string before_value = line.substr(curr + 2);
            const std::string::size_type quote_pos = before_value.find_first_not_of(" \t");
            if ((quote_pos != std::string::npos) && before_value[quote_pos] == '"') {
              line = prefix + "SRC_URI=\"${SRC_URI}" + before_value.substr(quote_pos + 1);
            }
          }
        }
      }
      normalized.append(line);
      normalized.push_back('\n');
    }
    ebuild_source->swap(normalized);
  }

  std::string::size_type find_top_level_src_uri_token(const std::string& line) const {
    const std::string::size_type pos = line.find("SRC_URI");
    if (pos == std::string::npos) {
      return pos;
    }
    if (pos != 0 && is_valid_shell_var_char(line[pos - 1])) {
      return std::string::npos;
    }
    if ((pos + 7 < line.size()) && is_valid_shell_name_char(line[pos + 7])) {
      return std::string::npos;
    }
    return pos;
  }

  bool is_function_definition_line(const std::string& line) const {
    return (line.find("() {") != std::string::npos) || (line.find("(){") != std::string::npos);
  }

  bool is_valid_shell_var_char(char c) const {
    return (my_isalnum(c) || (c == '_') || (c == '-'));
  }

  bool is_valid_shell_var_start(char c) const {
    return (my_isalpha(c) || (c == '_'));
  }

  bool is_valid_shell_name_char(char c) const {
    return (my_isalnum(c) || (c == '_'));
  }

  bool is_valid_shell_name_start(char c) const {
    return (my_isalpha(c) || (c == '_'));
  }

  std::string::size_type revision_index(const std::string& ver) const {
    const std::string::size_type pos = ver.rfind("-r");
    if (pos == std::string::npos) {
      return pos;
    }
    if (ver.find_first_not_of("0123456789", pos + 2) == std::string::npos) {
      return pos;
    }
    return std::string::npos;
  }

  bool extract_distfiles(const std::string& src_uri, std::set<std::string>& files) const {
    // Gentoo conditionals are control tokens like "foo?" / "!foo?".
    // URLs may also contain '?', so only treat a trailing '?' token as conditional.
    static const std::string operators[] = { "||", "^^", "??" };

    WordVec parts = split_string(src_uri);
    bool complete = true;
    for (size_t i = 0; i < parts.size(); ++i) {
        const std::string& token = parts[i];
        if (token == "(" || token == ")") continue;
        if (token.find("://") == std::string::npos && !token.empty() && token[token.size() - 1] == '?') continue;
        if (token == operators[0] || token == operators[1] || token == operators[2]) {
            continue;
        }

        std::string filename;
        if (token == "->") {
            if (i + 1 < parts.size()) {
                filename = parts[i+1];
                i++;
            }
        } else if (token.find("://") != std::string::npos) {
            // It's a URL. If NOT followed by ->, use basename
            if (i + 1 >= parts.size() || parts[i+1] != "->") {
                std::string clean_token = token;
                size_t trim_pos = clean_token.find_first_of("?#");
                if (trim_pos != std::string::npos) {
                    clean_token.erase(trim_pos);
                }
                size_t last_slash = clean_token.find_last_of('/');
                if (last_slash != std::string::npos) {
                    filename = clean_token.substr(last_slash + 1);
                }
            }
        }

        if (!filename.empty()) {
            if (filename.find('$') != std::string::npos || 
                filename.find('(') != std::string::npos || 
                filename.find('{') != std::string::npos ||
                filename.find(')') != std::string::npos ||
                filename.find('}') != std::string::npos ||
                filename.find('*') != std::string::npos ||
                filename.find('?') != std::string::npos) {
                complete = false;
            } else {
                files.insert(filename);
            }
        }
    }
    return complete;
  }

  virtual const char* name() const { return "regen"; }
  virtual const char* alias() const { return "r"; }
  virtual const char* description() const { return "intelligently check and regenerate manifests"; }
  virtual const char* usage() const { return "Usage: q regen [options] [target ...]\n"
                                             "Targets:\n"
                                             "  path            package/category/repo path inside PORTDIR\n"
                                             "  category/pkg    package atom (exact category/package)\n"
                                             "  (none)          defaults to current working directory\n"
                                             "Options:\n"
                                             "  -h, --help     show this help\n"
                                             "  -n, --dry-run  show what would be done and why\n"
                                             "  -v, --verbose  be more talkative"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_REGEN_H_
