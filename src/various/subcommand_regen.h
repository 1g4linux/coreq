// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_REGEN_H_
#define SRC_VARIOUS_SUBCOMMAND_REGEN_H_ 1

#include <config.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include "various/subcommand.h"
#include "corepkg/conf/corepkgsettings.h"
#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"
#include "coreqTk/argsreader.h"
#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"
#include "coreqTk/null.h"
#include "coreqTk/parseerror.h"
#include "coreqTk/utils.h"
#include "coreqTk/stringutils.h"
#include "coreqTk/varsreader.h"
#include "cache/common/selectors.h"

struct ManifestDist {
    std::string filename;
    uint64_t size;
    std::map<std::string, std::string> hashes;
};

class SubcommandRegen : public Subcommand {
 public:
  virtual int run(int argc, char** argv) {
    CoreqRc& coreqrc = get_coreqrc();
    ParseError parse_error;
    CorePkgSettings corepkgsettings(&coreqrc, &parse_error, true, true);

    bool help = false;
    bool fix = false;
    bool check = false;
    bool verbose = false;
    
    OptionList opt_table;
    opt_table.PUSH_BACK(Option("help", 'h', Option::BOOLEAN, &help));
    opt_table.PUSH_BACK(Option("fix", 'f', Option::BOOLEAN, &fix));
    opt_table.PUSH_BACK(Option("check", 'c', Option::BOOLEAN, &check));
    opt_table.PUSH_BACK(Option("verbose", 'v', Option::BOOLEAN, &verbose));
    
    ArgumentReader ar(argc, argv, opt_table);
    if (help) {
      coreq::say("%s") % usage();
      return EXIT_SUCCESS;
    }
    
    if (!fix && !check) check = true; // Default to check

    std::string portdir = corepkgsettings["PORTDIR"];
    if (portdir.empty()) {
        struct stat st;
        if (stat("profiles/categories", &st) == 0) {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd))) {
                portdir = cwd;
            }
        }
    }

    if (portdir.empty()) {
        coreq::say_error(_("PORTDIR not set and current directory is not a repository."));
        return EXIT_FAILURE;
    }

    std::string distdir = corepkgsettings["DISTDIR"];
    if (distdir.empty()) {
        distdir = portdir + "/distfiles";
    }

    WordVec categories;
    std::string cat_file = portdir + "/profiles/categories";
    if (!pushback_lines(cat_file.c_str(), &categories, false, false)) {
        coreq::say_error(_("Could not read categories from %s")) % cat_file;
        return EXIT_FAILURE;
    }

    for (WordVec::const_iterator cat_it = categories.begin(); cat_it != categories.end(); ++cat_it) {
        std::string cat_dir = portdir + "/" + *cat_it;
        WordVec packages;
        if (!pushback_files(cat_dir, &packages, NULLPTR, 2, true, false)) {
            continue;
        }

        for (WordVec::const_iterator pkg_it = packages.begin(); pkg_it != packages.end(); ++pkg_it) {
            std::string pkg_path = cat_dir + "/" + *pkg_it;
            std::string manifest_path = pkg_path + "/Manifest";
            
            std::map<std::string, ManifestDist> manifest_dists;
            parse_manifest(manifest_path, manifest_dists);

            std::set<std::string> expected_distfiles;
            WordVec ebuild_files;
            if (!scandir_cc(pkg_path, &ebuild_files, ebuild_selector)) {
                continue;
            }

            std::string newest_ebuild;
            for (WordVec::const_iterator eb_it = ebuild_files.begin(); eb_it != ebuild_files.end(); ++eb_it) {
                if (eb_it->find(".ebuild") == std::string::npos) continue;
                
                std::string full_eb_path = pkg_path + "/" + *eb_it;
                if (newest_ebuild.empty() || *eb_it > newest_ebuild) {
                    newest_ebuild = *eb_it;
                }

                std::string pn, pv;
                std::string::size_type pos = ebuild_pos(*eb_it);
                if (pos != std::string::npos) {
                    if (ExplodeAtom::split(&pn, &pv, eb_it->substr(0, pos).c_str())) {
                        parse_ebuild_src_uri(full_eb_path, pn, pv, expected_distfiles);
                    }
                }
            }

            // Reconciliation
            bool needs_regen = false;
            std::vector<std::string> missing_in_manifest;
            std::vector<std::string> extra_in_manifest;
            std::vector<std::string> size_mismatch;

            // Check if all expected files are in Manifest
            for (std::set<std::string>::iterator it = expected_distfiles.begin(); it != expected_distfiles.end(); ++it) {
                std::string filename = *it;
                if (manifest_dists.find(filename) == manifest_dists.end()) {
                    missing_in_manifest.push_back(filename);
                    needs_regen = true;
                } else {
                    // File is in Manifest, check if it exists in DISTDIR and has correct size
                    struct stat st;
                    std::string distfile_path = distdir + "/" + filename;
                    if (stat(distfile_path.c_str(), &st) == 0) {
                        if (st.st_size != (off_t)manifest_dists[filename].size) {
                            size_mismatch.push_back(filename);
                            needs_regen = true;
                        }
                    }
                }
            }

            // Check if Manifest has files that are no longer expected
            for (std::map<std::string, ManifestDist>::iterator it = manifest_dists.begin(); it != manifest_dists.end(); ++it) {
                if (expected_distfiles.find(it->first) == expected_distfiles.end()) {
                    extra_in_manifest.push_back(it->first);
                    needs_regen = true;
                }
            }

            if (needs_regen) {
                if (verbose || check) {
                    coreq::say("%s/%s: %s") % *cat_it % *pkg_it % "NEEDS_REGEN";
                    if (verbose) {
                        for (size_t i = 0; i < missing_in_manifest.size(); ++i) coreq::say("  - Missing in Manifest: %s") % missing_in_manifest[i];
                        for (size_t i = 0; i < extra_in_manifest.size(); ++i) coreq::say("  - Extra in Manifest: %s") % extra_in_manifest[i];
                        for (size_t i = 0; i < size_mismatch.size(); ++i) coreq::say("  - Size mismatch in DISTDIR: %s") % size_mismatch[i];
                    }
                }

                if (fix && !newest_ebuild.empty()) {
                    coreq::say(_("Regenerating Manifest for %s/%s...")) % *cat_it % *pkg_it;
                    std::string cmd = "ebuild " + pkg_path + "/" + newest_ebuild + " manifest";
                    if (verbose) coreq::say("  Running: %s") % cmd;
                    if (system(cmd.c_str()) != 0) {
                        coreq::say_error(_("Failed to run: %s")) % cmd;
                    }
                }
            }
        }
    }

    return EXIT_SUCCESS;
  }

  void parse_manifest(const std::string& path, std::map<std::string, ManifestDist>& dists) {
    std::ifstream in(path.c_str());
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        WordVec parts = split_string(line);
        if (parts.size() >= 5 && parts[0] == "DIST") {
            ManifestDist info;
            info.filename = parts[1];
            info.size = my_atou(parts[2].c_str());
            for (size_t i = 3; i + 1 < parts.size(); i += 2) {
                info.hashes[parts[i]] = parts[i+1];
            }
            dists[info.filename] = info;
        }
    }
  }

  void parse_ebuild_src_uri(const std::string& path, const std::string& pn, const std::string& pv, std::set<std::string>& expected) {
    WordIterateMap env;
    env["PN"] = pn;
    env["PV"] = pv;
    env["P"] = pn + "-" + pv;
    
    std::string err;
    VarsReader reader(VarsReader::SUBST_VARS | VarsReader::INTO_MAP);
    reader.useMap(&env);
    if (!reader.read(path.c_str(), &err, false)) return;
    
    const std::string* src_uri = reader.find("SRC_URI");
    if (!src_uri) return;
    
    extract_distfiles(*src_uri, expected);
  }

  void extract_distfiles(const std::string& src_uri, std::set<std::string>& files) {
    WordVec parts = split_string(src_uri);
    for (size_t i = 0; i < parts.size(); ++i) {
        if (parts[i] == "(" || parts[i] == ")") continue;
        if (parts[i].find("?") != std::string::npos) continue; // Skip USE conditionals for Tier A

        if (parts[i] == "->") {
            if (i + 1 < parts.size()) {
                files.insert(parts[i+1]);
                i++;
            }
        } else if (parts[i].find("://") != std::string::npos) {
            // It's a URL. If NOT followed by ->, use basename
            if (i + 1 == parts.size() || parts[i+1] != "->") {
                size_t last_slash = parts[i].find_last_of('/');
                if (last_slash != std::string::npos) {
                    files.insert(parts[i].substr(last_slash + 1));
                }
            }
        }
    }
  }

  virtual const char* name() const { return "regen"; }
  virtual const char* description() const { return "intelligently check and regenerate manifests"; }
  virtual const char* usage() const { return "Usage: q regen [options]\n"
                                             "Options:\n"
                                             "  -h, --help     show this help\n"
                                             "  -c, --check    check only (default)\n"
                                             "  -f, --fix      fetch missing and regenerate manifests\n"
                                             "  -v, --verbose  be more talkative"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_REGEN_H_
