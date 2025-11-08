// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include <config.h>  // IWYU pragma: keep

#ifdef WITH_PROTOBUF
#include "coreqTk/diagnostics.h"
WSUGGEST_ATTRIBUTE_PURE_OFF
WSUGGEST_ATTRIBUTE_CONST_OFF
WSUGGEST_FINAL_METHODS_OFF
GCC_DIAG_OFF(sign-conversion)

#include "output/coreq.pb.cc"  // NOLINT(build/include)

GCC_DIAG_ON(sign-conversion)
WSUGGEST_FINAL_METHODS_ON
WSUGGEST_ATTRIBUTE_CONST_ON
WSUGGEST_ATTRIBUTE_PURE_ON
#endif
