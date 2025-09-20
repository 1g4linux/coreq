// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_DEPENDS_H_
#define SRC_VARIOUS_SUBCOMMAND_DEPENDS_H_ 1

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
#include "database/header.h"
#include "database/io.h"
#include "database/package_reader.h"
#include "corepkg/mask.h"
#include "corepkg/depend.h"

#define VAR_DB_PKG "/var/db/pkg/"

class SubcommandDepends : public Subcommand {
 public:
  virtual int run(int argc, char** argv) {
    CoreqRc& coreqrc = get_coreqrc();
    Depend::use_depend = true; // Force dependency usage
    
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

    Database db;
    std::string db_errtext;
    std::string cache_file = coreqrc["CACHE_FILE"];
    bool have_db = db.openread(cache_file.c_str());
    DBHeader header;
    if (have_db) {
        db.read_header(&header, &db_errtext, 0);
    }

    for (ArgumentReader::const_iterator it = ar.begin(); it != ar.end(); ++it) {
      if (it->type != Parameter::ARGUMENT) continue;
      
      std::string target_pattern = it->m_argument;
      
      Mask target_mask(Mask::maskTypeNone);
      std::string target_errtext;
      target_mask.parseMask(target_pattern.c_str(), &target_errtext);

      coreq::say(_("Finding packages depending on %s...")) % target_pattern;

      WordVec categories;
      if (!pushback_files(full_vdb_path, &categories, NULLPTR, 2, true, false)) {
          coreq::say_error(_("Could not read VDB directory: %s")) % full_vdb_path;
          return EXIT_FAILURE;
      }

      for (WordVec::const_iterator cat_it = categories.begin(); cat_it != categories.end(); ++cat_it) {
        std::string cat_dir = full_vdb_path + *cat_it;
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

                    vardb.readDepend(pkg, &(*v_it), header);
                    if (match_any_dependency(v_it->depend, target_pattern, target_mask)) {
                        coreq::say("%s/%s-%s") % pkg.category % pkg.name % v_it->getFull();
                    }
                }
            }
        }
      }
    }

    return EXIT_SUCCESS;
  }

  bool match_any_dependency(const Depend& dep, const std::string& target_str, const Mask& target_mask) {
      return match_dependency_str(dep.get_depend(), target_str, target_mask) ||
             match_dependency_str(dep.get_rdepend(), target_str, target_mask) ||
             match_dependency_str(dep.get_pdepend(), target_str, target_mask) ||
             match_dependency_str(dep.get_bdepend(), target_str, target_mask) ||
             match_dependency_str(dep.get_idepend(), target_str, target_mask);
  }

  bool match_dependency_str(const std::string& dep_str, const std::string& target_str, const Mask& target_mask) {
      if (dep_str.empty()) return false;
      
      std::string target_name = target_mask.getName();
      if (target_name.empty()) target_name = target_str;
      // If target_str is cat/pkg, target_name might be 'pkg'. 
      // Searching for 'pkg' in dep_str might be too broad but good for quick filter.
      if (dep_str.find(target_name) == std::string::npos) return false;

      WordVec words;
      split_string(&words, dep_str);
      for (WordVec::const_iterator it = words.begin(); it != words.end(); ++it) {
          std::string word = *it;
          while (!word.empty() && (word[0] == '(' || word[0] == '!' || word[0] == '|')) word.erase(0, 1);
          while (!word.empty() && (word[word.length()-1] == ')' || word[word.length()-1] == '?')) word.erase(word.length()-1);
          
          if (word.empty()) continue;
          if (word.find('/') == std::string::npos) continue;

          Mask atom(Mask::maskTypeNone);
          std::string err;
          if (atom.parseMask(word.c_str(), &err) == BasicVersion::parsedOK) {
              if (atom.getCategory() == target_mask.getCategory() || target_mask.getCategory()[0] == '\0') {
                  if (std::string(atom.getName()) == target_name) {
                      return true;
                  }
              }
          } else {
              if (word.find(target_str) != std::string::npos) return true;
          }
      }
      return false;
  }

  virtual const char* name() const { return "depends"; }
  virtual const char* description() const { return "list all packages directly depending on ATOM"; }
  virtual const char* usage() const { return "Usage: q depends [options] <ATOM>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_DEPENDS_H_
