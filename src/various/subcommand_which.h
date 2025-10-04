// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_WHICH_H_
#define SRC_VARIOUS_SUBCOMMAND_WHICH_H_ 1

#include <config.h>
#include <iostream>
#include <string>
#include <vector>
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

#define VAR_DB_PKG "/var/db/pkg/"

class SubcommandWhich : public Subcommand {
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

    for (std::vector<Package>::iterator pkg_it = matching_packages.begin(); pkg_it != matching_packages.end(); ++pkg_it) {
        InstVec* inst_vec = vardb.getInstalledVector(*pkg_it);
        if (!inst_vec) continue;

        for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
            if (!pkg_version.empty() && v_it->getFull() != pkg_version) continue;

            std::string pkg_dir = full_vdb_path + pkg_it->category + "/" + pkg_it->name + "-" + v_it->getFull();
            std::string ebuild_name = pkg_it->name + "-" + v_it->getFull() + ".ebuild";
            std::string ebuild_path = pkg_dir + "/" + ebuild_name;
            
            if (stat(ebuild_path.c_str(), &st) == 0) {
                coreq::say("%s") % ebuild_path;
            } else {
                // Try searching in the tree if configured
                std::string portdir = corepkgsettings["PORTDIR"];
                if (!portdir.empty()) {
                    std::string tree_ebuild = portdir + "/" + pkg_it->category + "/" + pkg_it->name + "/" + ebuild_name;
                    if (stat(tree_ebuild.c_str(), &st) == 0) {
                        coreq::say("%s") % tree_ebuild;
                        continue;
                    }
                }
                coreq::say_error(_("Ebuild not found for %s/%s-%s")) % pkg_it->category % pkg_it->name % v_it->getFull();
            }
        }
    }

    return EXIT_SUCCESS;
  }

  virtual const char* name() const { return "which"; }
  virtual const char* description() const { return "print full path to ebuild for PKG"; }
  virtual const char* usage() const { return "Usage: q which [options] <PKG>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_WHICH_H_
