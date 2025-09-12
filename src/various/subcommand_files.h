// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_FILES_H_
#define SRC_VARIOUS_SUBCOMMAND_FILES_H_ 1

#include <config.h>
#include <iostream>
#include <string>
#include <vector>
#include "various/subcommand.h"
#include "corepkg/vardbpkg.h"
#include "corepkg/conf/corepkgsettings.h"
#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"
#include "coreqTk/argsreader.h"
#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"
#include "coreqTk/parseerror.h"

class SubcommandFiles : public Subcommand {
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

    VarDbPkg vardb(corepkgsettings["ROOT"] + corepkgsettings["VDB_PATH"], 
                   true, true, false, false, false, false);

    for (ArgumentReader::const_iterator it = ar.begin(); it != ar.end(); ++it) {
      if (it->type != Parameter::ARGUMENT) continue;
      
      std::string pattern = it->m_argument;
      
      std::string category, name;
      size_t slash = pattern.find('/');
      if (slash != std::string::npos) {
        category = pattern.substr(0, slash);
        name = pattern.substr(slash + 1);
      } else {
        name = pattern;
      }

      Package pkg;
      pkg.category = category;
      pkg.name = name;

      InstVec* inst_vec = vardb.getInstalledVector(pkg);
      if (inst_vec && !inst_vec->empty()) {
        for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
          if (vardb.readContents(pkg, &(*v_it))) {
            for (std::vector<InstVersion::ContentsEntry>::const_iterator c_it = v_it->contents.begin(); c_it != v_it->contents.end(); ++c_it) {
              coreq::say("%s") % c_it->path;
            }
          }
        }
      } else {
        coreq::say_error(_("Package not found: %s")) % pattern;
      }
    }

    return EXIT_SUCCESS;
  }

  virtual const char* name() const { return "files"; }
  virtual const char* description() const { return "list all files installed by PKG"; }
  virtual const char* usage() const { return "Usage: q files [options] <PKG>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_FILES_H_
