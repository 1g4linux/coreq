// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREQRC_GLOBAL_H_
#define SRC_COREQRC_GLOBAL_H_ 1

#include <config.h>  // IWYU pragma: keep

#include "coreqTk/attribute.h"

class CoreqRc;

/**
Must be called exactly once before get_coreqrc() can be used
**/
ATTRIBUTE_NONNULL_ CoreqRc& get_coreqrc(const char *varprefix);

/**
@return a static coreqrc.
**/
ATTRIBUTE_PURE CoreqRc& get_coreqrc();

#ifdef JUMBO_BUILD
ATTRIBUTE_NONNULL_ void fill_defaults(CoreqRc *coreqrc);
#else
ATTRIBUTE_NONNULL_ void fill_defaults_part_1(CoreqRc *coreqrc);
ATTRIBUTE_NONNULL_ void fill_defaults_part_2(CoreqRc *coreqrc);
ATTRIBUTE_NONNULL_ void fill_defaults_part_3(CoreqRc *coreqrc);
ATTRIBUTE_NONNULL_ void fill_defaults_part_4(CoreqRc *coreqrc);
ATTRIBUTE_NONNULL_ void fill_defaults_part_5(CoreqRc *coreqrc);
ATTRIBUTE_NONNULL_ void fill_defaults_part_6(CoreqRc *coreqrc);
#endif

#endif  // SRC_COREQRC_GLOBAL_H_
