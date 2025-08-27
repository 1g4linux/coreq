// vim:set noet cinoptions=g0,t0,(0 sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#include "corepkg/extendedversion.h"
#include <config.h>  // IWYU pragma: keep

#include <string>

#include "coreqTk/coreqint.h"
#include "coreqTk/likely.h"
#include "corepkg/basicversion.h"
#include "corepkg/package.h"

using std::string;

const ExtendedVersion::Restrict ExtendedVersion::RESTRICT_NONE, ExtendedVersion::RESTRICT_BINCHECKS, ExtendedVersion::RESTRICT_STRIP, ExtendedVersion::RESTRICT_TEST,
    ExtendedVersion::RESTRICT_USERPRIV, ExtendedVersion::RESTRICT_INSTALLSOURCES, ExtendedVersion::RESTRICT_FETCH, ExtendedVersion::RESTRICT_MIRROR, ExtendedVersion::RESTRICT_PRIMARYURI,
    ExtendedVersion::RESTRICT_BINDIST, ExtendedVersion::RESTRICT_PARALLEL, ExtendedVersion::RESTRICT_ALL;

const ExtendedVersion::Properties ExtendedVersion::PROPERTIES_NONE, ExtendedVersion::PROPERTIES_INTERACTIVE, ExtendedVersion::PROPERTIES_LIVE, ExtendedVersion::PROPERTIES_VIRTUAL,
    ExtendedVersion::PROPERTIES_SET, ExtendedVersion::PROPERTIES_ALL;

const ExtendedVersion::HaveBinPkg ExtendedVersion::HAVEBINPKG_UNKNOWN, ExtendedVersion::HAVEBINPKG_TBZ_NO, ExtendedVersion::HAVEBINPKG_TBZ_YES, ExtendedVersion::HAVEBINPKG_TBZ,
    ExtendedVersion::HAVEBINPKG_GPKG, ExtendedVersion::HAVEBINPKG_MULTI;

bool ExtendedVersion::use_src_uri;

string ExtendedVersion::get_longfullslot() const {
  return (subslotname.empty() ? (slotname.empty() ? "0" : slotname) : (slotname.empty() ? (string("0/") + subslotname) : (slotname + "/" + subslotname)));
}

coreq::SignedBool ExtendedVersion::compare(const ExtendedVersion& left, const ExtendedVersion& right) {
  coreq::SignedBool r(BasicVersion::compare(left, right));
  if (likely(r != 0)) {
    return r;
  }
  if (unlikely(left.overlay_key == right.overlay_key)) {
    return 0;  // Do not care about possibly unknown priorities
  }
  if (likely(left.priority == right.priority)) {
    if (left.overlay_key < right.overlay_key) {
      return -1;
    }
    // if(left.overlay_key > right.overlay_key)
    return 1;
  }
  if (left.priority < right.priority) {
    return -1;
  }
  // if(left.priority > right.priority)
  return 1;
}
