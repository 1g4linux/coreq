// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#include "coreqrc/global.h"
#include <config.h>  // IWYU pragma: keep

#include "coreqTk/assert.h"
#include "coreqTk/null.h"
#include "coreqrc/coreqrc.h"

static CoreqRc* static_coreqrc = NULLPTR;

/**
Must be called exactly once before get_coreqrc() can be used
**/
CoreqRc& get_coreqrc(const char* varprefix) {
  coreq_assert_static(static_coreqrc == NULLPTR);
  static_coreqrc = new CoreqRc(varprefix);

#ifdef JUMBO_BUILD
  fill_defaults(static_coreqrc);
#else
  fill_defaults_part_1(static_coreqrc);
  fill_defaults_part_2(static_coreqrc);
  fill_defaults_part_3(static_coreqrc);
  fill_defaults_part_4(static_coreqrc);
  fill_defaults_part_5(static_coreqrc);
  fill_defaults_part_6(static_coreqrc);
#endif

  static_coreqrc->read();
  return *static_coreqrc;
}

/**
@return reference to internal static CoreqRc.
This can be called everywhere!
**/
CoreqRc& get_coreqrc() {
  coreq_assert_static(static_coreqrc != NULLPTR);
  return *static_coreqrc;
}
