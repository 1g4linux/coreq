// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREQTK_UNORDERED_MAP_H_
#define SRC_COREQTK_UNORDERED_MAP_H_ 1

#include <config.h>  // IWYU pragma: keep

// check_includes: #include "coreqTk/unordered_map.h" std::map<int, int>

// IWYU pragma: begin_exports

#ifdef HAVE_UNORDERED_MAP
#include <unordered_map>
#define UNORDERED_MAP std::unordered_map
#else
#include <map>
#define UNORDERED_MAP std::map
#endif

// IWYU pragma: end_exports

#endif  // SRC_COREQTK_UNORDERED_MAP_H_
