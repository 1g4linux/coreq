// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_HASUSE_H_
#define SRC_VARIOUS_SUBCOMMAND_HASUSE_H_ 1

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

class SubcommandHasuse : public Subcommand {
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
      coreq::say_error(_("No USE flag specified."));
      coreq::say("%s") % usage();
      return EXIT_FAILURE;
    }

    std::string target_flag = ar.begin()->m_argument;

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

    coreq::say(_("Searching for packages with USE flag %s...")) % target_flag;

    WordVec categories;
    if (!pushback_files(full_vdb_path, &categories, NULLPTR, 2, true, false)) {
        return EXIT_FAILURE;
    }

    for (WordVec::const_iterator cat_it = categories.begin(); cat_it != categories.end(); ++cat_it) {
      std::string cat_dir = full_vdb_path + *cat_it;
      WordVec packages;
      if (!pushback_files(cat_dir, &packages, NULLPTR, 2, true, false)) {
          continue;
      }

      for (WordVec::const_iterator pkg_it = packages.begin(); pkg_it != packages.end(); ++pkg_it) {
          std::string pkg_dir = cat_dir + "/" + *pkg_it;
          
          // Check IUSE file
          std::string iuse_path = pkg_dir + "/IUSE";
          bool found = false;
          LineVec lines;
          if (pushback_lines(iuse_path.c_str(), &lines, false, false)) {
              for (LineVec::const_iterator l_it = lines.begin(); l_it != lines.end(); ++l_it) {
                  WordVec words;
                  split_string(&words, *l_it);
                  for (WordVec::const_iterator w_it = words.begin(); w_it != words.end(); ++w_it) {
                      std::string flag = *w_it;
                      if (!flag.empty() && (flag[0] == '+' || flag[0] == '-')) flag.erase(0, 1);
                      if (flag == target_flag) {
                          found = true;
                          break;
                      }
                  }
                  if (found) break;
              }
          }

          if (!found) {
              // Try environment.bz2 as fallback
              std::string env_path = pkg_dir + "/environment.bz2";
              if (stat(env_path.c_str(), &st) == 0) {
                  std::string cmd = "bzcat " + env_path + " | grep -E '^(declare (.* )?)?IUSE=' | grep -E '[[:space:]\"]" + target_flag + "[[:space:]\"]' > /dev/null 2>&1";
                  if (system(cmd.c_str()) == 0) {
                      found = true;
                  }
              }
          }

          if (found) {
              // Check if it's enabled
              std::string use_path = pkg_dir + "/USE";
              std::string status = " ";
              if (pushback_lines(use_path.c_str(), &lines, false, false)) {
                  bool enabled = false;
                  for (LineVec::const_iterator l_it = lines.begin(); l_it != lines.end(); ++l_it) {
                      WordVec words;
                      split_string(&words, *l_it);
                      for (WordVec::const_iterator w_it = words.begin(); w_it != words.end(); ++w_it) {
                          if (*w_it == target_flag) {
                              enabled = true;
                              break;
                          }
                      }
                      if (enabled) break;
                  }
                  status = enabled ? "+" : "-";
              }
              
              coreq::say("%s %s/%s") % status % *cat_it % *pkg_it;
          }
      }
    }

    return EXIT_SUCCESS;
  }

  virtual const char* name() const { return "hasuse"; }
  virtual const char* description() const { return "list all packages that have USE flag"; }
  virtual const char* usage() const { return "Usage: q hasuse [options] <flag>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_HASUSE_H_
