// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_USES_H_
#define SRC_VARIOUS_SUBCOMMAND_USES_H_ 1

#include <config.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <climits>
#include <unistd.h>
#include <sys/stat.h>
#include "various/subcommand.h"
#include "corepkg/vardbpkg.h"
#include "corepkg/conf/corepkgsettings.h"
#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"
#include "coreqTk/argsreader.h"
#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"
#include "coreqTk/parseerror.h"
#include "coreqTk/stringutils.h"
#include "coreqTk/utils.h"
#include "coreqTk/ansicolor.h"

#define VAR_DB_PKG "/var/db/pkg/"

class SubcommandUses : public Subcommand {
 public:
  virtual int run(int argc, char** argv) {
    CoreqRc& coreqrc = get_coreqrc();
    
    ParseError parse_error;
    CorePkgSettings corepkgsettings(&coreqrc, &parse_error, true, true);

    OptionList opt_table;
    bool help = false;
    Option help_opt("help", 'h', Option::BOOLEAN, &help);
    opt_table.PUSH_BACK(help_opt);
    
    ArgumentReader ar(argc, argv, opt_table);
    if (help) {
      coreq::say("%s") % usage();
      return EXIT_SUCCESS;
    }

    if (ar.empty()) {
      coreq::say_error(_("No package specified."));
      coreq::say("%s") % usage();
      return EXIT_FAILURE;
    }

    std::string pattern = ar.begin()->m_argument;

    std::string full_vdb_path;
    struct stat st;
    if (stat("pkg", &st) == 0 && S_ISDIR(st.st_mode)) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            full_vdb_path = std::string(cwd) + "/pkg/";
        } else {
            full_vdb_path = "pkg/";
        }
    } else {
        full_vdb_path = coreqrc["EPREFIX_INSTALLED"] + VAR_DB_PKG;
    }

    VarDbPkg vardb(full_vdb_path, true, true, false, false, false, false);

    std::string category, name_ver;
    size_t slash = pattern.find('/');
    if (slash != std::string::npos) {
      category = pattern.substr(0, slash);
      name_ver = pattern.substr(slash + 1);
    } else {
      name_ver = pattern;
    }

    std::string pkg_name, pkg_version;
    if (!ExplodeAtom::split(&pkg_name, &pkg_version, name_ver.c_str())) {
        pkg_name = name_ver;
        pkg_version = "";
    }

    Package pkg;
    pkg.category = category;
    pkg.name = pkg_name;

    std::vector<Package> matching_packages;
    if (category.empty()) {
        WordVec categories;
        if (pushback_files(full_vdb_path, &categories, NULLPTR, 2, true, false)) {
            for (WordVec::const_iterator cat_it = categories.begin(); cat_it != categories.end(); ++cat_it) {
                Package p;
                p.category = *cat_it;
                p.name = pkg_name;
                InstVec* inst_vec = vardb.getInstalledVector(p);
                if (inst_vec && !inst_vec->empty()) {
                    matching_packages.push_back(p);
                }
            }
        }
    } else {
        matching_packages.push_back(pkg);
    }

    if (matching_packages.empty()) {
        coreq::say_error(_("Package not found: %s")) % pattern;
        return EXIT_FAILURE;
    }

    AnsiColor green("green", NULLPTR);
    AnsiColor red("red", NULLPTR);
    std::string reset = AnsiColor::reset();

    for (std::vector<Package>::iterator pkg_it = matching_packages.begin(); pkg_it != matching_packages.end(); ++pkg_it) {
        InstVec* inst_vec = vardb.getInstalledVector(*pkg_it);
        if (!inst_vec) continue;

        for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
            if (!pkg_version.empty() && v_it->getFull() != pkg_version) continue;

            std::string pkg_dir = full_vdb_path + pkg_it->category + "/" + pkg_it->name + "-" + v_it->getFull();
            
            coreq::say(_(" * %s/%s-%s")) % pkg_it->category % pkg_it->name % v_it->getFull();
            
            // Get enabled flags
            std::set<std::string> enabled_flags;
            std::string use_path = pkg_dir + "/USE";
            LineVec lines;
            if (pushback_lines(use_path.c_str(), &lines, false, false)) {
                for (LineVec::iterator l_it = lines.begin(); l_it != lines.end(); ++l_it) {
                    WordVec words;
                    split_string(&words, *l_it);
                    for (WordVec::iterator w_it = words.begin(); w_it != words.end(); ++w_it) {
                        enabled_flags.insert(*w_it);
                    }
                }
            }

            // Get available flags (IUSE)
            std::set<std::string> iuse_flags;
            std::string iuse_path = pkg_dir + "/IUSE";
            if (pushback_lines(iuse_path.c_str(), &lines, false, false)) {
                for (LineVec::iterator l_it = lines.begin(); l_it != lines.end(); ++l_it) {
                    WordVec words;
                    split_string(&words, *l_it);
                    for (WordVec::iterator w_it = words.begin(); w_it != words.end(); ++w_it) {
                        std::string flag = *w_it;
                        if (!flag.empty() && (flag[0] == '+' || flag[0] == '-')) flag.erase(0, 1);
                        if (!flag.empty()) iuse_flags.insert(flag);
                    }
                }
            }
            
            if (iuse_flags.empty()) {
                // Try fallback to environment.bz2
                std::string env_path = pkg_dir + "/environment.bz2";
                if (stat(env_path.c_str(), &st) == 0) {
                    std::string cmd = "bzcat " + env_path + " | grep -E '^(declare (.* )?)?IUSE='";
                    FILE* pipe = popen(cmd.c_str(), "r");
                    if (pipe) {
                        char buffer[4096];
                        if (fgets(buffer, sizeof(buffer), pipe)) {
                            std::string line = buffer;
                            size_t start = line.find('"');
                            size_t end = line.rfind('"');
                            if (start != std::string::npos && end != std::string::npos && end > start) {
                                std::string iuse_val = line.substr(start + 1, end - start - 1);
                                WordVec words;
                                split_string(&words, iuse_val);
                                for (WordVec::iterator w_it = words.begin(); w_it != words.end(); ++w_it) {
                                    std::string flag = *w_it;
                                    if (!flag.empty() && (flag[0] == '+' || flag[0] == '-')) flag.erase(0, 1);
                                    if (!flag.empty()) iuse_flags.insert(flag);
                                }
                            }
                        }
                        pclose(pipe);
                    }
                }
            }

            if (iuse_flags.empty()) {
                coreq::say(_("  (no USE flags)"));
            } else {
                for (std::set<std::string>::iterator f_it = iuse_flags.begin(); f_it != iuse_flags.end(); ++f_it) {
                    bool enabled = enabled_flags.count(*f_it);
                    std::string sign = enabled ? "+" : "-";
                    std::string colored_flag = enabled ? (green.asString() + *f_it + reset) : (red.asString() + *f_it + reset);
                    coreq::say("  %s %s") % sign % colored_flag;
                }
            }
            
            coreq::say("");
        }
    }

    return EXIT_SUCCESS;
  }

  virtual const char* name() const { return "uses"; }
  virtual const char* description() const { return "display USE flags for PKG"; }
  virtual const char* usage() const { return "Usage: q uses [options] <PKG>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_USES_H_
