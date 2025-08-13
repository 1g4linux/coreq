// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREQTK_UNORDERED_SET_H_
#define SRC_COREQTK_UNORDERED_SET_H_ 1

#include <config.h>  // IWYU pragma: keep

// check_includes: #include "coreqTk/unordered_set.h" std::set<int>

// IWYU pragma: begin_exports
#ifdef HAVE_UNORDERED_SET
#include <unordered_set>
#define UNORDERED_SET std::unordered_set
#else
#include <set>
#define UNORDERED_SET std::set
#endif
// IWYU pragma: end_exports

#endif  // SRC_COREQTK_UNORDERED_SET_H_
