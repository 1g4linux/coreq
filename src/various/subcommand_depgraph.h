// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_DEPGRAPH_H_
#define SRC_VARIOUS_SUBCOMMAND_DEPGRAPH_H_ 1

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
#include "database/header.h"
#include "database/io.h"
#include "corepkg/mask.h"
#include "corepkg/depend.h"

#define VAR_DB_PKG "/var/db/pkg/"

class Atom : public Mask {
 public:
  Atom() : Mask(maskTypeNone) {}
  bool match_version(const ExtendedVersion* ev) const { return test(ev); }
};

class SubcommandDepgraph : public Subcommand {
 public:
  virtual int run(int argc, char** argv) {
    CoreqRc& coreqrc = get_coreqrc();
    Depend::use_depend = true;
    
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
                      for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
                          if (pkg_version.empty() || v_it->getFull() == pkg_version) {
                              std::set<std::string> seen;
                              resolve_dependencies(vardb, header, pkg, &(*v_it), 0, seen);
                              break;
                          }
                      }
                      break;
                  }
              }
          }
      } else {
          InstVec* inst_vec = vardb.getInstalledVector(pkg);
          if (inst_vec && !inst_vec->empty()) {
            for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
              if (pkg_version.empty() || v_it->getFull() == pkg_version) {
                std::set<std::string> seen;
                resolve_dependencies(vardb, header, pkg, &(*v_it), 0, seen);
                break;
              }
            }
          } else {
            coreq::say_error(_("Package not found: %s")) % pattern;
          }
      }
    }

    return EXIT_SUCCESS;
  }

  void resolve_dependencies(VarDbPkg& vardb, const DBHeader& header, const Package& pkg, InstVersion* ver, int depth, std::set<std::string>& seen) {
      std::string pkg_full = pkg.category + "/" + pkg.name + "-" + ver->getFull();
      
      std::string indent = "";
      for (int i = 0; i < depth; ++i) indent += "  ";
      coreq::say("%s%s") % indent % pkg_full;

      if (seen.count(pkg_full)) {
          return;
      }
      seen.insert(pkg_full);

      vardb.readDepend(pkg, ver, header);
      
      std::vector<std::string> deps;
      parse_deps(ver->depend.get_rdepend(), deps);
      parse_deps(ver->depend.get_pdepend(), deps);
      parse_deps(ver->depend.get_depend(), deps);

      for (std::vector<std::string>::iterator it = deps.begin(); it != deps.end(); ++it) {
          std::string atom_str = *it;
          Atom atom;
          std::string err;
          if (atom.parseMask(atom_str.c_str(), &err) != BasicVersion::parsedOK) continue;

          Package dep_pkg;
          dep_pkg.category = atom.getCategory();
          dep_pkg.name = atom.getName();

          InstVec* inst_vec = vardb.getInstalledVector(dep_pkg);
          if (inst_vec && !inst_vec->empty()) {
              for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
                  if (atom.match_version(&(*v_it))) {
                      resolve_dependencies(vardb, header, dep_pkg, &(*v_it), depth + 1, seen);
                      break;
                  }
              }
          }
      }
  }

  void parse_deps(const std::string& dep_str, std::vector<std::string>& deps) {
      if (dep_str.empty()) return;
      WordVec words;
      split_string(&words, dep_str);
      for (WordVec::const_iterator it = words.begin(); it != words.end(); ++it) {
          std::string word = *it;
          while (!word.empty() && (word[0] == '(' || word[0] == '!' || word[0] == '|')) word.erase(0, 1);
          while (!word.empty() && (word[word.length()-1] == ')' || word[word.length()-1] == '?')) word.erase(word.length()-1);
          
          if (word.empty()) continue;
          if (word.find('/') == std::string::npos) continue;
          
          size_t use_start = word.find('[');
          if (use_start != std::string::npos) {
              word.erase(use_start);
          }

          deps.push_back(word);
      }
  }

  virtual const char* name() const { return "depgraph"; }
  virtual const char* description() const { return "display a tree of all dependencies for PKG"; }
  virtual const char* usage() const { return "Usage: q depgraph [options] <PKG>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_DEPGRAPH_H_
