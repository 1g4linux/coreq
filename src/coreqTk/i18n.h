// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREQTK_I18N_H_
#define SRC_COREQTK_I18N_H_ 1

#define _(a) (a)
#define N_(a, b, n) (((n) == 1) ? (a) : (b))
#define P_(p, a) (a)
#define NP_(p, a, b, n) (((n) == 1) ? (a) : (b))

#endif  // SRC_COREQTK_I18N_H_
