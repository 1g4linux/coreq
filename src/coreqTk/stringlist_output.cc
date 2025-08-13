// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include "coreqTk/stringlist.h"
#include <config.h>  // IWYU pragma: keep

#include "coreqTk/likely.h"
#include "coreqTk/outputstring.h"
#include "coreqTk/stringtypes.h"

void StringListContent::append_to_string(OutputString *s, const OutputString& skip) const {
	bool sep(false);
	for(WordVec::const_iterator it(m_list.begin());
		likely(it != m_list.end()); ++it) {
		if(sep) {
			s->append(skip);
		} else {
			sep = true;
		}
		s->append_smart(*it);
	}
}
