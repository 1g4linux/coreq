// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_KEYWORDS_H_
#define SRC_VARIOUS_SUBCOMMAND_KEYWORDS_H_ 1

#include <config.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
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

class SubcommandKeywords : public Subcommand {
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

    coreq::say(_("Keywords for %s/%s:")) % matching_packages[0].category % matching_packages[0].name;

    std::set<std::string> all_arches;
    std::map<std::string, std::map<std::string, std::string>> table; // version -> {arch -> status}

    for (std::vector<Package>::iterator pkg_it = matching_packages.begin(); pkg_it != matching_packages.end(); ++pkg_it) {
        InstVec* inst_vec = vardb.getInstalledVector(*pkg_it);
        if (!inst_vec) continue;

        for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
            std::string ver = v_it->getFull();
            std::string keywords_path = full_vdb_path + pkg_it->category + "/" + pkg_it->name + "-" + ver + "/KEYWORDS";
            LineVec lines;
            if (pushback_lines(keywords_path.c_str(), &lines, false, false)) {
                for (LineVec::const_iterator l_it = lines.begin(); l_it != lines.end(); ++l_it) {
                    WordVec words;
                    split_string(&words, *l_it);
                    for (WordVec::const_iterator w_it = words.begin(); w_it != words.end(); ++w_it) {
                        std::string kw = *w_it;
                        std::string arch = kw;
                        std::string status = "V"; // default to stable if no prefix
                        if (!kw.empty()) {
                            if (kw[0] == '~') {
                                arch = kw.substr(1);
                                status = "~";
                            } else if (kw[0] == '-') {
                                arch = kw.substr(1);
                                status = "M"; // masked
                            }
                        }
                        all_arches.insert(arch);
                        table[ver][arch] = status;
                    }
                }
            }
        }
    }

    AnsiColor green("green", NULLPTR);
    AnsiColor yellow("yellow", NULLPTR);
    AnsiColor red("red", NULLPTR);
    std::string reset = AnsiColor::reset();

    // Print header
    std::string header = "Version          ";
    for (std::set<std::string>::iterator arch_it = all_arches.begin(); arch_it != all_arches.end(); ++arch_it) {
        header += " | " + *arch_it;
    }
    coreq::say("%s") % header;
    
    std::string separator(header.length(), '-');
    coreq::say("%s") % separator;

    for (std::map<std::string, std::map<std::string, std::string>>::iterator v_it = table.begin(); v_it != table.end(); ++v_it) {
        std::string ver_str = v_it->first;
        if (ver_str.length() < 16) ver_str.append(16 - ver_str.length(), ' ');
        
        std::string line = ver_str;
        for (std::set<std::string>::iterator arch_it = all_arches.begin(); arch_it != all_arches.end(); ++arch_it) {
            std::string status = v_it->second[*arch_it];
            if (status.empty()) {
                line += " |  ";
            } else {
                std::string colored_status = status;
                if (status == "V") colored_status = green.asString() + status + reset;
                else if (status == "~") colored_status = yellow.asString() + status + reset;
                else if (status == "M") colored_status = red.asString() + status + reset;
                line += " | " + colored_status;
            }
        }
        coreq::say("%s") % line;
    }

    return EXIT_SUCCESS;
  }

  virtual const char* name() const { return "keywords"; }
  virtual const char* alias() const { return "y"; }
  virtual const char* description() const { return "display keywords for specified PKG"; }
  virtual const char* usage() const { return "Usage: q keywords [options] <PKG>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_KEYWORDS_H_
