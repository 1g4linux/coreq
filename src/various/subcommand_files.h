// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_FILES_H_
#define SRC_VARIOUS_SUBCOMMAND_FILES_H_ 1

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
#include "coreqTk/ansicolor.h"

#define VAR_DB_PKG "/var/db/pkg/"

class SubcommandFiles : public Subcommand {
 public:
  virtual int run(int argc, char** argv) {
    CoreqRc& coreqrc = get_coreqrc();
    
    ParseError parse_error;
    CorePkgSettings corepkgsettings(&coreqrc, &parse_error, true, true);
    AnsiColor& c = get_ansicolor();

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
      coreq::say_error(c.red(_("No package specified.")));
      coreq::say("%s") % usage();
      return EXIT_FAILURE;
    }

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

    for (ArgumentReader::const_iterator it = ar.begin(); it != ar.end(); ++it) {
      if (it->type != Parameter::ARGUMENT) continue;
      
      std::string pattern = it->m_argument;
      
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

      if (category.empty()) {
          WordVec categories;
          if (pushback_files(full_vdb_path, &categories, NULLPTR, 2, true, false)) {
              for (WordVec::const_iterator cat_it = categories.begin(); cat_it != categories.end(); ++cat_it) {
                  pkg.category = *cat_it;
                  InstVec* inst_vec = vardb.getInstalledVector(pkg);
                  if (inst_vec && !inst_vec->empty()) {
                      bool found_any = false;
                      for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
                          if (pkg_version.empty() || v_it->getFull() == pkg_version) {
                              if (vardb.readContents(pkg, &(*v_it))) {
                                  coreq::say(c.bold(c.cyan(_("Contents of %s/%s:"))) ) % pkg.category % v_it->getFull();
                                  for (std::vector<InstVersion::ContentsEntry>::const_iterator c_it = v_it->contents.begin(); c_it != v_it->contents.end(); ++c_it) {
                                      if (c_it->type == InstVersion::ContentsEntry::DIR_T) {
                                          coreq::say(c.bold(c.blue("%s"))) % c_it->path;
                                      } else if (c_it->type == InstVersion::ContentsEntry::SYM_T) {
                                          coreq::say(c.cyan("%s") + " -> %s") % c_it->path % c_it->target;
                                      } else {
                                          coreq::say("%s") % c_it->path;
                                      }
                                  }
                                  found_any = true;
                              }
                          }
                      }
                      if (found_any) break;
                  }
              }
          }
      } else {
          InstVec* inst_vec = vardb.getInstalledVector(pkg);
          if (inst_vec && !inst_vec->empty()) {
            bool found_any = false;
            for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
              if (pkg_version.empty() || v_it->getFull() == pkg_version) {
                if (vardb.readContents(pkg, &(*v_it))) {
                  coreq::say(c.bold(c.cyan(_("Contents of %s/%s:"))) ) % pkg.category % v_it->getFull();
                  for (std::vector<InstVersion::ContentsEntry>::const_iterator c_it = v_it->contents.begin(); c_it != v_it->contents.end(); ++c_it) {
                    if (c_it->type == InstVersion::ContentsEntry::DIR_T) {
                        coreq::say(c.bold(c.blue("%s"))) % c_it->path;
                    } else if (c_it->type == InstVersion::ContentsEntry::SYM_T) {
                        coreq::say(c.cyan("%s") + " -> %s") % c_it->path % c_it->target;
                    } else {
                        coreq::say("%s") % c_it->path;
                    }
                  }
                  found_any = true;
                }
              }
            }
            if (!found_any) {
                coreq::say_error(c.red(_("Package found but version mismatch: %s"))) % pattern;
            }
          } else {
            coreq::say_error(c.red(_("Package not found: %s"))) % pattern;
          }
      }
    }

    return EXIT_SUCCESS;
  }

  virtual const char* name() const { return "files"; }
  virtual const char* alias() const { return "f"; }
  virtual const char* description() const { return "list all files installed by PKG"; }
  virtual const char* usage() const { return "Usage: q files [options] <PKG>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_FILES_H_
