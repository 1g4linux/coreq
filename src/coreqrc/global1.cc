// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#include "coreqrc/globals.h"
#include <config.h>  // IWYU pragma: keep  // NOLINT(build/include_order)

#define DEFAULT_PART 1

void fill_defaults_part_1(CoreqRc* coreqrc) {
#include "coreqrc/defaults.cc"  // NOLINT(build/include)
}
