// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include "cache/coreqcache/coreqcache.h"
#include <config.h>  // IWYU pragma: keep

#include <cerrno>
#include <cstring>

#include <algorithm>
#include <string>

#include "database/header.h"
#include "database/io.h"
#include "database/package_reader.h"
#include "coreqTk/dialect.h"
#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"
#include "coreqTk/likely.h"
#include "coreqTk/null.h"
#include "coreqTk/ptr_container.h"
#include "coreqTk/stringtypes.h"
#include "coreqTk/stringutils.h"
#include "corepkg/basicversion.h"
#include "corepkg/conf/corepkgsettings.h"
#include "corepkg/package.h"
#include "corepkg/packagetree.h"
#include "corepkg/version.h"

using std::string;

CoreqCache::CachesList* CoreqCache::all_coreqcaches = NULLPTR;

CoreqCache::~CoreqCache() {
  if (all_coreqcaches != NULLPTR) {
    CachesList::iterator it(std::find(all_coreqcaches->begin(), all_coreqcaches->end(), this));
    if (it != all_coreqcaches->end()) {
      all_coreqcaches->erase(it);
    }
  }
}

bool CoreqCache::initialize(const string& name) {
  WordVec args;
  split_string(&args, name, true, ":", false);
  if (casecontains(args[0], "coreq")) {
    if (args[0].find('*') == string::npos) {
      m_name = "coreq";
      never_add_categories = true;
    }
    else {
      m_name = "coreq*";
      never_add_categories = false;
    }
  }
  else {
    return false;
  }

  m_file.clear();
  if (args.size() >= 2) {
    if (!(args[1].empty())) {
      m_name.append(1, ' ');
      m_name.append(args[1]);
      m_file = args[1];
    }
  }

  m_only_overlay = true;
  m_overlay.clear();
  m_get_overlay = 0;
  if (args.size() >= 3) {
    if (!(args[2].empty())) {
      m_name.append(" [");
      m_name.append(args[2]);
      m_name.append("]");
      if (args[2] == "*") {
        m_only_overlay = false;
      }
      else {
        m_overlay = args[2];
      }
    }
  }
  slavemode = false;
  if (all_coreqcaches == NULLPTR) {
    all_coreqcaches = new CachesList;
  }
  all_coreqcaches->PUSH_BACK(this);
  return (args.size() <= 3);
}

void CoreqCache::setSchemeFinish() {
  if (!m_file.empty())
    m_full = m_prefix + m_file;
  else
    m_full = m_prefix + COREQ_CACHEFILE;
}

void CoreqCache::allerrors(const CachesList& slaves, const string& msg) {
  for (CachesList::const_iterator sl(slaves.begin()); unlikely(sl != slaves.end()); ++sl) {
    string& s(sl->err_msg);
    if (s.empty()) {
      s = msg;
    }
  }
  m_error_callback(err_msg);
}

void CoreqCache::thiserror(const string& msg) {
  err_msg = msg;
  if (!slavemode) {
    m_error_callback(err_msg);
  }
}

bool CoreqCache::get_overlaydat(const DBHeader& header) {
  if ((!m_only_overlay) || (m_overlay.empty())) {
    return true;
  }
  const char* portdir(NULLPTR);
  if (corepkgsettings)
    portdir = (*corepkgsettings)["PORTDIR"].c_str();
  if (m_overlay == "~") {
    bool found(false);
    if (!m_overlay_name.empty()) {
      found = header.find_overlay(&m_get_overlay, m_overlay_name.c_str(), portdir, 0, DBHeader::OVTEST_LABEL);
    }
    if (!found) {
      found = header.find_overlay(&m_get_overlay, m_scheme.c_str(), portdir, 0, DBHeader::OVTEST_LABEL);
    }
    if (!found) {
      thiserror(coreq::format(_("cache file %s does not contain overlay \"%s\" [%s]")) % m_overlay_name % m_scheme);
      return false;
    }
  }
  else if (!header.find_overlay(&m_get_overlay, m_overlay.c_str(), portdir, 0, DBHeader::OVTEST_ALL)) {
    thiserror(coreq::format(_("cache file %s does not contain overlay %s")) % m_full % m_overlay);
    return false;
  }
  return true;
}

bool CoreqCache::get_destcat(PackageTree* packagetree, const char* cat_name, Category* category, const string& pcat) {
  if (likely(err_msg.empty())) {
    if (unlikely(packagetree == NULLPTR)) {
      if (unlikely(pcat == cat_name)) {
        dest_cat = category;
        return true;
      }
    }
    else if (never_add_categories) {
      dest_cat = packagetree->find(pcat);
      return (dest_cat != NULLPTR);
    }
    else {
      dest_cat = &((*packagetree)[pcat]);
      return true;
    }
  }
  dest_cat = NULLPTR;
  return false;
}

void CoreqCache::get_package(Package* p) {
  if (dest_cat == NULLPTR) {
    return;
  }
  bool have_onetime_info(false), have_pkg(false);
  Package* pkg(NULLPTR);
  for (Package::iterator it(p->begin()); likely(it != p->end()); ++it) {
    if (m_only_overlay) {
      if (likely(it->overlay_key != m_get_overlay))
        continue;
    }
    Version* version(new Version);
    *static_cast<BasicVersion*>(version) = *static_cast<BasicVersion*>(*it);
    version->overlay_key = m_overlay_key;
    version->set_full_keywords(it->get_full_keywords());
    version->slotname = it->slotname;
    version->subslotname = it->subslotname;
    version->restrictFlags = it->restrictFlags;
    version->propertiesFlags = it->propertiesFlags;
    version->iuse = it->iuse;
    version->required_use = it->required_use;
    version->eapi = it->eapi;
    version->depend = it->depend;
    version->src_uri = it->src_uri;
    if (pkg == NULLPTR) {
      pkg = dest_cat->findPackage(p->name);
      if (pkg != NULLPTR) {
        have_onetime_info = have_pkg = true;
      }
      else {
        pkg = new Package(p->category, p->name);
      }
    }
    pkg->addVersion(version);
    if (*(pkg->latest()) == *version) {
      pkg->homepage = p->homepage;
      pkg->licenses = p->licenses;
      pkg->desc = p->desc;
      have_onetime_info = true;
    }
  }
  if (have_onetime_info) {  // if the package exists:
    // add collected iuse from the saved data
    pkg->iuse.insert(p->iuse);
    if (!have_pkg) {
      dest_cat->addPackage(pkg);
    }
  }
  else {
    delete pkg;
  }
}

bool CoreqCache::readCategories(PackageTree* packagetree, const char* cat_name, Category* category) {
  if (slavemode) {
    if (err_msg.empty()) {
      return true;
    }
    m_error_callback(err_msg);
    return false;
  }
  CachesList slaves;
  if (all_coreqcaches != NULLPTR) {
    for (CachesList::const_iterator sl(all_coreqcaches->begin()); unlikely(sl != all_coreqcaches->end()); ++sl) {
      if (sl->m_full == m_full) {
        if (*sl != this) {
          sl->slavemode = true;
        }
        sl->err_msg.clear();
        slaves.PUSH_BACK(*sl);
      }
    }
  }
  Database db;
  if (unlikely(!db.openread(m_full.c_str()))) {
    allerrors(slaves, coreq::format(_("cannot read cache file %s: %s")) % m_full % std::strerror(errno));
    m_error_callback(err_msg);
    return false;
  }

  DBHeader header;

  string errtext;
  if (unlikely(!db.read_header(&header, &errtext, 0))) {
    allerrors(slaves, coreq::format(_("error in file %s: %s")) % m_full % errtext);
    m_error_callback(err_msg);
    return false;
  }
  bool success(false);
  for (CachesList::const_iterator sl(slaves.begin()); unlikely(sl != slaves.end()); ++sl) {
    if (sl->get_overlaydat(header))
      success = true;
  }
  if (!success) {
    return false;
  }

  PackageReader reader(&db, header);
  for (; reader.next(); reader.skip()) {
    if (unlikely(!reader.read(PackageReader::NAME))) {
      break;
    }
    Package* p(reader.get());

    success = false;
    for (CachesList::const_iterator sl(slaves.begin()); unlikely(sl != slaves.end()); ++sl) {
      if (sl->get_destcat(packagetree, cat_name, category, p->category)) {
        success = true;
      }
    }
    if (!success) {
      continue;
    }
    if (unlikely(!reader.read())) {
      break;
    }
    p = reader.get();
    for (CachesList::const_iterator sl(slaves.begin()); unlikely(sl != slaves.end()); ++sl) {
      sl->get_package(p);
    }
  }
  const char* err_cstr(reader.get_errtext());
  if (unlikely(err_cstr != NULLPTR)) {
    allerrors(slaves, coreq::format(_("error in file %s: %s")) % m_full % err_cstr);
    m_error_callback(err_msg);
    return false;
  }
  return true;
}
