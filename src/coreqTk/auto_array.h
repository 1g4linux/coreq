// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREQTK_AUTO_ARRAY_H_
#define SRC_COREQTK_AUTO_ARRAY_H_ 1

#include <config.h>  // IWYU pragma: keep

#include "coreqTk/null.h"

// check_includes: include "coreqTk/auto_array.h"

namespace coreq {
template <typename m_Type>
class auto_array {
 public:
  explicit auto_array(m_Type* p) : m_p(p) {}

  ~auto_array() {
    if (m_p != NULLPTR) {
      delete[] m_p;
    }
  }

  m_Type* get() const { return m_p; }

 protected:
  m_Type* m_p;
};
}  // namespace coreq

#endif  // SRC_COREQTK_AUTO_ARRAY_H_
