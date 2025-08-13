// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREQTK_STRINGTYPES_H_
#define SRC_COREQTK_STRINGTYPES_H_

#include <config.h>  // IWYU pragma: keep

// check_includes: include "coreqTk/stringtypes.h"

// IWYU pragma: begin_exports
#include <set>
#include <string>
#include <vector>
// IWYU pragma: end_exports

#include "coreqTk/iterate_map.h"
#include "coreqTk/iterate_set.h"
#include "coreqTk/unordered_map.h"
#include "coreqTk/unordered_set.h"


typedef std::vector<std::string> WordVec;
typedef std::set<std::string> WordSet;
typedef ITERATE_MAP<std::string, std::string> WordIterateMap;
typedef ITERATE_SET<std::string> WordIterateSet;
typedef UNORDERED_MAP<std::string, std::string> WordUnorderedMap;
typedef UNORDERED_SET<std::string> WordUnorderedSet;
typedef std::string::size_type WordSize;
typedef WordVec LineVec;

#endif  // SRC_COREQTK_STRINGTYPES_H_
