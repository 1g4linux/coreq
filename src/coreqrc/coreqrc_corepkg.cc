// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include "coreqrc/coreqrc.h"
#include <config.h>  // IWYU pragma: keep

#include <string>

#include "coreqTk/dialect.h"
#include "coreqTk/formated.h"
#include "coreqTk/likely.h"
#include "coreqTk/null.h"
#include "coreqTk/parseerror.h"
#include "coreqTk/stringtypes.h"
#include "coreqTk/stringutils.h"
#include "corepkg/conf/corepkgsettings.h"

using std::string;

void CoreqRc::known_vars() {
	WordSet vars;
	for(WordUnorderedMap::const_iterator it(main_map.begin());
		it != main_map.end(); ++it) {
		vars.INSERT(it->first);
	}
	ParseError parse_error(true);
	CorePkgSettings ps(this, &parse_error, false, true);
	for(WordIterateMap::const_iterator it(ps.begin());
		it != ps.end(); ++it) {
		vars.INSERT(it->first);
	}
	for(WordSet::const_iterator it(vars.begin());
		it != vars.end(); ++it) {
		coreq::say() % *it;
	}
}

bool CoreqRc::print_var(const string& key) {
	string print_append((*this)["PRINT_APPEND"]);
	unescape_string(&print_append);
	const char *s;
	if(likely(key != "PORTDIR")) {
		s = cstr(key);
		if(likely(s != NULLPTR)) {
			coreq::print("%s%s") % s % print_append;
			return true;
		}
	}
	ParseError parse_error(true);
	CorePkgSettings ps(this, &parse_error, false, true);
	s = ps.cstr(key);
	if(likely(s != NULLPTR)) {
		coreq::print("%s%s") % s % print_append;
		return true;
	}
	return false;
}
