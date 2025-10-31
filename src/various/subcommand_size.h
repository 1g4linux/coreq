// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_SIZE_H_
#define SRC_VARIOUS_SUBCOMMAND_SIZE_H_ 1

#include <config.h>
#include <iostream>
#include <string>
#include <vector>
#include <climits>
#include <unistd.h>
#include <sys/stat.h>
#include <iomanip>
#include <cmath>
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

class SubcommandSize : public Subcommand {
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
            
            long long total_size = 0;
            size_t num_files = 0;
            
            // Try SIZE file first
            std::string size_path = pkg_dir + "/SIZE";
            LineVec lines;
            if (pushback_lines(size_path.c_str(), &lines, false, false)) {
                if (!lines.empty()) {
                    total_size = std::atoll(lines[0].c_str());
                }
            }
            
            // Count files from CONTENTS if we don't have size or just for info
            if (vardb.readContents(*pkg_it, &(*v_it))) {
                num_files = v_it->contents.size();
                if (total_size == 0) {
                    // Fallback: sum up existing files
                    for (std::vector<InstVersion::ContentsEntry>::const_iterator c_it = v_it->contents.begin(); c_it != v_it->contents.end(); ++c_it) {
                        struct stat st_file;
                        if (stat(c_it->path.c_str(), &st_file) == 0) {
                            if (S_ISREG(st_file.st_mode)) {
                                total_size += st_file.st_size;
                            }
                        }
                    }
                }
            }
            
            coreq::say(_(" * %s/%s-%s")) % pkg_it->category % pkg_it->name % v_it->getFull();
            coreq::say(_("   Total files : %d")) % num_files;
            coreq::say(_("   Total size  : %s")) % format_size(total_size);
        }
    }

    return EXIT_SUCCESS;
  }

  std::string format_size(long long bytes) {
      if (bytes == 0) return "0 B";
      const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB"};
      int i = 0;
      double size = bytes;
      while (size >= 1024 && i < 4) {
          size /= 1024;
          i++;
      }
      char buffer[64];
      std::snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[i]);
      return std::string(buffer);
  }

  virtual const char* name() const { return "size"; }
  virtual const char* alias() const { return "s"; }
  virtual const char* description() const { return "display total size of all files owned by PKG"; }
  virtual const char* usage() const { return "Usage: q size [options] <PKG>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_SIZE_H_
