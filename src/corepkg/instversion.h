// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREPKG_INSTVERSION_H_
#define SRC_COREPKG_INSTVERSION_H_ 1

#include <config.h>  // IWYU pragma: keep

#include <ctime>

#include "coreqTk/attribute.h"
#include "coreqTk/dialect.h"
#include "coreqTk/coreqint.h"
#include "coreqTk/stringtypes.h"
#include "corepkg/basicversion.h"
#include "corepkg/extendedversion.h"
#include "corepkg/version.h"

/**
InstVersion expands the BasicVersion class by data relevant for vardbpkg
**/
class InstVersion FINAL : public ExtendedVersion, public Keywords {
 public:
  /**
  For versions in vardbpkg we might not yet know the slot.
  For caching, we mark this here.
  **/
  bool know_slot, read_failed;
  /**
  Similarly for iuse and usedUse
  **/
  bool know_use;
  /**
  And the same for restricted
  **/
  bool know_restricted;
  /**
  and for deps
  **/
  bool know_deps;
  /**
  and for eapi
  **/
  bool know_eapi;
  /**
  and for instDate
  **/
  bool know_instDate;

  std::time_t instDate;  ///< Installation date according to vardbpkg
  WordVec inst_iuse;     ///< Useflags in iuse according to vardbpkg
  WordSet usedUse;       ///< Those useflags in iuse actually used

  /**
  Similarly for overlay_keys
  **/
  bool know_overlay, overlay_failed;

  InstVersion() : know_slot(false), read_failed(false), know_use(false), know_restricted(false), know_deps(false), know_eapi(false), know_instDate(false), know_overlay(false), overlay_failed(false) {}

  ATTRIBUTE_PURE static coreq::SignedBool compare(const InstVersion& left, const InstVersion& right);

  ATTRIBUTE_PURE static coreq::SignedBool compare(const InstVersion& left, const ExtendedVersion& right);

  ATTRIBUTE_PURE static coreq::SignedBool compare(const ExtendedVersion& left, const InstVersion& right);
};

/**
Short compare-stuff
**/
ATTRIBUTE_PURE inline static bool operator<(const ExtendedVersion& left, const InstVersion& right);
inline static bool operator<(const ExtendedVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) < 0;
}

ATTRIBUTE_PURE inline static bool operator>(const ExtendedVersion& left, const InstVersion& right);
inline static bool operator>(const ExtendedVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) > 0;
}

ATTRIBUTE_PURE inline static bool operator==(const ExtendedVersion& left, const InstVersion& right);
inline static bool operator==(const ExtendedVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) == 0;
}

ATTRIBUTE_PURE inline static bool operator!=(const ExtendedVersion& left, const InstVersion& right);
inline static bool operator!=(const ExtendedVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) != 0;
}

ATTRIBUTE_PURE inline static bool operator>=(const ExtendedVersion& left, const InstVersion& right);
inline static bool operator>=(const ExtendedVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) >= 0;
}

ATTRIBUTE_PURE inline static bool operator<=(const ExtendedVersion& left, const InstVersion& right);
inline static bool operator<=(const ExtendedVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) <= 0;
}

ATTRIBUTE_PURE inline static bool operator<(const InstVersion& left, const ExtendedVersion& right);
inline static bool operator<(const InstVersion& left, const ExtendedVersion& right) {
  return InstVersion::compare(left, right) < 0;
}

ATTRIBUTE_PURE inline static bool operator>(const InstVersion& left, const ExtendedVersion& right);
inline static bool operator>(const InstVersion& left, const ExtendedVersion& right) {
  return InstVersion::compare(left, right) > 0;
}

ATTRIBUTE_PURE inline static bool operator==(const InstVersion& left, const ExtendedVersion& right);
inline static bool operator==(const InstVersion& left, const ExtendedVersion& right) {
  return InstVersion::compare(left, right) == 0;
}

ATTRIBUTE_PURE inline static bool operator!=(const InstVersion& left, const ExtendedVersion& right);
inline static bool operator!=(const InstVersion& left, const ExtendedVersion& right) {
  return InstVersion::compare(left, right) != 0;
}

ATTRIBUTE_PURE inline static bool operator>=(const InstVersion& left, const ExtendedVersion& right);
inline static bool operator>=(const InstVersion& left, const ExtendedVersion& right) {
  return InstVersion::compare(left, right) >= 0;
}

ATTRIBUTE_PURE inline static bool operator<=(const InstVersion& left, const ExtendedVersion& right);
inline static bool operator<=(const InstVersion& left, const ExtendedVersion& right) {
  return InstVersion::compare(left, right) <= 0;
}

ATTRIBUTE_PURE inline static bool operator<(const InstVersion& left, const InstVersion& right);
inline static bool operator<(const InstVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) < 0;
}

ATTRIBUTE_PURE inline static bool operator>(const InstVersion& left, const InstVersion& right);
inline static bool operator>(const InstVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) > 0;
}

ATTRIBUTE_PURE inline static bool operator==(const InstVersion& left, const InstVersion& right);
inline static bool operator==(const InstVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) == 0;
}

ATTRIBUTE_PURE inline static bool operator!=(const InstVersion& left, const InstVersion& right);
inline static bool operator!=(const InstVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) != 0;
}

ATTRIBUTE_PURE inline static bool operator>=(const InstVersion& left, const InstVersion& right);
inline static bool operator>=(const InstVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) >= 0;
}

ATTRIBUTE_PURE inline static bool operator<=(const InstVersion& left, const InstVersion& right);
inline static bool operator<=(const InstVersion& left, const InstVersion& right) {
  return InstVersion::compare(left, right) <= 0;
}

#endif  // SRC_COREPKG_INSTVERSION_H_
