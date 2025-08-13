// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREQRC_GLOBALS_H_
#define SRC_COREQRC_GLOBALS_H_ 1

// This include file is exceptional:
// It collects common includes and macros for coreqrc/global?.cc
// It should be included *only* by these files!

#include <config.h>  // IWYU pragma: keep

#include <cstdlib>

#include "coreqTk/i18n.h"
#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"

// check_includes: _(

#define DO_STRINGIFY(a) #a
#define EXPAND_STRINGIFY(a) DO_STRINGIFY(a)
#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_FAILURE
#define EXIT_FAILURE 1
#endif

#define AddOption(opt_type, opt_name, opt_default, opt_description) \
	coreqrc->addDefault(CoreqRcOption(CoreqRcOption::opt_type, opt_name, \
		opt_default, opt_description))

#endif  // SRC_COREQRC_GLOBALS_H_
