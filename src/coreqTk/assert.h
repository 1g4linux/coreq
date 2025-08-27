// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREQTK_ASSERT_H_
#define SRC_COREQTK_ASSERT_H_ 1

#include <config.h>  // IWYU pragma: keep

// check_includes: include "coreqTk/assert.h"

#ifndef NDEBUG

#if defined(COREQ_STATIC_ASSERT) || defined(COREQ_PARANOIC_ASSERT)
#include <cassert>  // IWYU pragma: export

#include "coreqTk/diagnostics.h"
#endif

/**
coreq_assert_static is used to check that static initializers (for static classes)
are called exactly once
**/

#ifdef COREQ_STATIC_ASSERT
#define coreq_assert_static(a)  \
  WGNU_STATEMENT_EXPRESSION_OFF \
  assert(a) WGNU_STATEMENT_EXPRESSION_ON
#else
#define coreq_assert_static(a)
#endif

#ifdef COREQ_PARANOIC_ASSERT
#define coreq_assert_paranoic(a) \
  WGNU_STATEMENT_EXPRESSION_OFF  \
  assert(a) WGNU_STATEMENT_EXPRESSION_ON
#else
#define coreq_assert_paranoic(a)
#endif

#else
#define coreq_assert_static(a)
#define coreq_assert_paranoic(a)
#endif

#endif  // SRC_COREQTK_ASSERT_H_
