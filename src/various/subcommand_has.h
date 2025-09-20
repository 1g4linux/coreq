// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_HAS_H_
#define SRC_VARIOUS_SUBCOMMAND_HAS_H_ 1

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

class SubcommandHas : public Subcommand {
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

    if (ar.size() < 2) {
      coreq::say_error(_("Too few arguments."));
      coreq::say("%s") % usage();
      return EXIT_FAILURE;
    }

    std::string target_var = "";
    std::string target_value = "";
    
    int arg_count = 0;
    for (ArgumentReader::const_iterator it = ar.begin(); it != ar.end(); ++it) {
        if (it->type != Parameter::ARGUMENT) continue;
        if (arg_count == 0) target_var = it->m_argument;
        else if (arg_count == 1) target_value = it->m_argument;
        arg_count++;
    }

    if (target_var.empty() || target_value.empty()) {
        coreq::say_error(_("Both variable name and value must be specified."));
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

    coreq::say(_("Searching for packages where %s contains %s...")) % target_var % target_value;

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
          std::string env_path = cat_dir + "/" + *pkg_it + "/environment.bz2";
          if (stat(env_path.c_str(), &st) != 0) continue;

          // Use bzcat and grep to efficiently find the variable
          // We look for patterns like:
          // declare -x VAR="...VALUE..."
          // VAR="...VALUE..."
          // We use a simple approach: uncompress and look for the variable.
          
          std::string cmd = "bzcat " + env_path + " | grep -E '^(declare (.* )?)?" + target_var + "=' | grep '" + target_value + "' > /dev/null 2>&1";
          if (system(cmd.c_str()) == 0) {
              coreq::say("%s/%s") % *cat_it % *pkg_it;
          }
      }
    }

    return EXIT_SUCCESS;
  }

  virtual const char* name() const { return "has"; }
  virtual const char* description() const { return "list all packages for matching ENVIRONMENT data stored in /var/db/pkg"; }
  virtual const char* usage() const { return "Usage: q has <variable> <value>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_HAS_H_
