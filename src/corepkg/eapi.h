// vim:set noet cinoptions=g0,t0,^-2 sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREPKG_EAPI_H_
#define SRC_COREPKG_EAPI_H_ 1

#include <config.h>  // IWYU pragma: keep

#include <string>

#include "coreqTk/inttypes.h"

class Eapi {
 public:
  typedef uint16_t EapiIndex;

 private:
  EapiIndex eapi_index;

 public:
  static void init_static();  // must be called exactly once

  Eapi() : eapi_index(0) {}

  void assign(const std::string& str);

  std::string get() const;
};

#endif  // SRC_COREPKG_EAPI_H_
