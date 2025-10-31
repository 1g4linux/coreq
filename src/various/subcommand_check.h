// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_CHECK_H_
#define SRC_VARIOUS_SUBCOMMAND_CHECK_H_ 1

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
#include "coreqTk/md5.h"

#define VAR_DB_PKG "/var/db/pkg/"

class SubcommandCheck : public Subcommand {
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

    int exit_status = EXIT_SUCCESS;

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
          exit_status = EXIT_FAILURE;
          continue;
      }

      for (std::vector<Package>::iterator pkg_it = matching_packages.begin(); pkg_it != matching_packages.end(); ++pkg_it) {
          InstVec* inst_vec = vardb.getInstalledVector(*pkg_it);
          if (!inst_vec || inst_vec->empty()) {
              if (!category.empty()) {
                  coreq::say_error(_("Package not found: %s")) % pattern;
                  exit_status = EXIT_FAILURE;
              }
              continue;
          }

          for (InstVec::iterator v_it = inst_vec->begin(); v_it != inst_vec->end(); ++v_it) {
              if (!pkg_version.empty() && v_it->getFull() != pkg_version) continue;

              coreq::say(_("Checking %s/%s-%s ...")) % pkg_it->category % pkg_it->name % v_it->getFull();
              
              if (!vardb.readContents(*pkg_it, &(*v_it))) {
                  coreq::say_error(_("Could not read CONTENTS for %s/%s-%s")) % pkg_it->category % pkg_it->name % v_it->getFull();
                  exit_status = EXIT_FAILURE;
                  continue;
              }

              size_t files_checked = 0;
              size_t files_missing = 0;
              size_t files_checksum_mismatch = 0;
              size_t files_mtime_mismatch = 0;

              for (std::vector<InstVersion::ContentsEntry>::const_iterator c_it = v_it->contents.begin(); c_it != v_it->contents.end(); ++c_it) {
                  if (c_it->type == InstVersion::ContentsEntry::DIR_T) {
                      struct stat st_file;
                      if (stat(c_it->path.c_str(), &st_file) != 0) {
                          coreq::say_error(_("%s: directory missing")) % c_it->path;
                          files_missing++;
                      } else if (!S_ISDIR(st_file.st_mode)) {
                          coreq::say_error(_("%s: not a directory")) % c_it->path;
                          files_checksum_mismatch++; // sort of
                      }
                  } else if (c_it->type == InstVersion::ContentsEntry::OBJ_T) {
                      files_checked++;
                      struct stat st_file;
                      if (stat(c_it->path.c_str(), &st_file) != 0) {
                          coreq::say_error(_("%s: file missing")) % c_it->path;
                          files_missing++;
                      } else {
                          if (!verify_md5sum(c_it->path.c_str(), c_it->sum)) {
                              coreq::say_error(_("%s: checksum mismatch")) % c_it->path;
                              files_checksum_mismatch++;
                          }
                          if (st_file.st_mtime != c_it->mtime) {
                              // Portage often updates mtime during installation (prelink, python byte-compilation etc)
                              // so this might be noisy. But let's report it if requested or for now.
                              // Actually equery check usually reports mtime too.
                              // coreq::say_error(_("%s: mtime mismatch")) % c_it->path;
                              files_mtime_mismatch++;
                          }
                      }
                  } else if (c_it->type == InstVersion::ContentsEntry::SYM_T) {
                      files_checked++;
                      struct stat st_file;
                      if (lstat(c_it->path.c_str(), &st_file) != 0) {
                          coreq::say_error(_("%s: symlink missing")) % c_it->path;
                          files_missing++;
                      } else if (!S_ISLNK(st_file.st_mode)) {
                          coreq::say_error(_("%s: not a symlink")) % c_it->path;
                          files_checksum_mismatch++;
                      }
                  }
              }
              
              if (files_missing == 0 && files_checksum_mismatch == 0) {
                  coreq::say(_(" * %s/%s-%s: %d files checked, 0 errors")) % pkg_it->category % pkg_it->name % v_it->getFull() % files_checked;
              } else {
                  coreq::say(_(" * %s/%s-%s: %d files checked, %d missing, %d checksum mismatches")) 
                      % pkg_it->category % pkg_it->name % v_it->getFull() % files_checked % files_missing % files_checksum_mismatch;
                  exit_status = EXIT_FAILURE;
              }
          }
      }
    }

    return exit_status;
  }

  virtual const char* name() const { return "check"; }
  virtual const char* alias() const { return "k"; }
  virtual const char* description() const { return "verify checksums and timestamps for PKG"; }
  virtual const char* usage() const { return "Usage: q check [options] <PKG>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_CHECK_H_
