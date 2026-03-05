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
#include <cstdlib>
#include <cerrno>
#include <csignal>
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
                        // Try metadata cache first, then ask corepkg directly.
                        if (!parse_metadata_src_uri(portdir, category, package, *eb_it, expected_distfiles)) {
                            if (!parse_corepkg_src_uri(portdir, cpv, expected_distfiles)) {
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

  bool extract_distfiles(const std::string& src_uri, std::set<std::string>& files) {
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
