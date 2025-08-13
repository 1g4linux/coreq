// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREQTK_NULL_H_
#define SRC_COREQTK_NULL_H_ 1

#include <config.h>  // IWYU pragma: keep

// check_includes: include "coreqTk/null.h"

#ifdef HAVE_NULLPTR
#define NULLPTR nullptr  // IWYU pragma: export
#else
#include <cstddef>
#define NULLPTR NULL  // IWYU pragma: export
#endif

#endif  // SRC_COREQTK_NULL_H_
