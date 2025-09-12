// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_BELONGS_H_
#define SRC_VARIOUS_SUBCOMMAND_BELONGS_H_ 1

#include <config.h>
#include <iostream>
#include <string>
#include <vector>
#include <climits>
#include <cstdlib>
#include "various/subcommand.h"
#include "corepkg/vardbpkg.h"
#include "corepkg/conf/corepkgsettings.h"
#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"
#include "coreqTk/argsreader.h"
#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"
#include "coreqTk/parseerror.h"
#include "coreqTk/utils.h"
#include "corepkg/packagetree.h"

#define VAR_DB_PKG "/var/db/pkg/"

class SubcommandBelongs : public Subcommand {
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
      coreq::say_error(_("No file specified."));
      coreq::say("%s") % usage();
      return EXIT_FAILURE;
    }

    std::string full_vdb_path = coreqrc["EPREFIX_INSTALLED"] + VAR_DB_PKG;

    VarDbPkg vardb(full_vdb_path, true, true, false, false, false, false);

    for (ArgumentReader::const_iterator it = ar.begin(); it != ar.end(); ++it) {
      if (it->type != Parameter::ARGUMENT) continue;
      
      std::string query_file = it->m_argument;
      
      // If path is relative, try to get absolute path
      if (!query_file.empty() && query_file[0] != '/') {
          char abs_path[PATH_MAX];
          if (realpath(query_file.c_str(), abs_path)) {
              query_file = abs_path;
          }
      }

      // Get all categories
      WordVec categories;
      if (!pushback_files(full_vdb_path, &categories, NULLPTR, 2, true, false)) {
          coreq::say_error(_("Could not read VDB directory: %s")) % full_vdb_path;
          return EXIT_FAILURE;
      }

      bool found = false;
      for (WordVec::const_iterator cat_it = categories.begin(); cat_it != categories.end(); ++cat_it) {
        std::string cat_dir = full_vdb_path;
        if (!cat_dir.empty() && cat_dir[cat_dir.length() - 1] != '/') cat_dir += "/";
        cat_dir += *cat_it;

        WordVec packages;
        if (!pushback_files(cat_dir, &packages, NULLPTR, 2, true, false)) {
            continue;
        }

        for (WordVec::const_iterator pkg_it = packages.begin(); pkg_it != packages.end(); ++pkg_it) {
            std::string pkg_name, pkg_version;
            if (!ExplodeAtom::split(&pkg_name, &pkg_version, pkg_it->c_str())) {
                continue;
            }

            Package pkg;
            pkg.category = *cat_it;
            pkg.name = pkg_name;

            InstVec* inst_vec = vardb.getInstalledVector(pkg);
            if (inst_vec) {
                for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
                    if (v_it->getFull() != pkg_version) continue;

                    if (vardb.readContents(pkg, &(*v_it))) {
                        for (std::vector<InstVersion::ContentsEntry>::const_iterator c_it = v_it->contents.begin(); c_it != v_it->contents.end(); ++c_it) {
                            if (c_it->path == query_file) {
                                coreq::say("%s/%s") % pkg.category % v_it->getFull();
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found) break;
                }
            }
        }
      }
      
      if (!found) {
          coreq::say_error(_("None of the installed packages own the file: %s")) % query_file;
      }
    }

    return EXIT_SUCCESS;
  }

  virtual const char* name() const { return "belongs"; }
  virtual const char* description() const { return "list what package FILES belong to"; }
  virtual const char* usage() const { return "Usage: q belongs [options] <FILE>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_BELONGS_H_
