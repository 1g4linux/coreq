// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#include "coreqTk/parseerror.h"
#include <config.h>  // IWYU pragma: keep

#include <string>

#include "coreqTk/dialect.h"
#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"
#include "coreqTk/likely.h"
#include "coreqTk/null.h"
#include "coreqTk/stringtypes.h"
#include "coreqTk/stringutils.h"

using std::string;

static WordUnorderedSet *printed = NULLPTR;

/**
Provide a common look for error-messages for parse-errors in
corepkg.{mask,keywords,..}
**/
void ParseError::output(const string& file, const LineVec::size_type line_nr, const string& line, const string& errtext) const {
	if(tacit) {
		return;
	}
	if(printed == NULLPTR) {
		printed = new WordUnorderedSet;
	}
	string cache(coreq::format("%s\a%s\v%s") % file % line_nr % errtext);
	if(printed->count(cache) != 0) {
		return;
	}
	printed->INSERT(MOVE(cache));
	coreq::say_error(_("-- invalid line %s in %s: \"%s\""))
		% line_nr % file % line;

	// Indent the message correctly
	WordVec lines;
	split_string(&lines, errtext, false, "\n", false);
	for(WordVec::const_iterator i(lines.begin()); likely(i != lines.end()); ++i) {
		coreq::say_error("    %s") % (*i);
	}
}
