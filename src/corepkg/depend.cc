// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include "corepkg/depend.h"
#include <config.h>  // IWYU pragma: keep

#include <string>

#include "coreqTk/dialect.h"
#include "coreqTk/likely.h"
#include "coreqTk/stringutils.h"

using std::string;

bool Depend::use_depend;

const char Depend::c_depend_name[] = "${DEPEND}";
const char Depend::c_rdepend_name[] = "${RDEPEND}";
const char Depend::c_pdepend_name[] = "${PDEPEND}";
const char Depend::c_bdepend_name[] = "${BDEPEND}";
const char Depend::c_idepend_name[] = "${IDEPEND}";

const char Depend::c_depend_shortcut  = '\x01';
const char Depend::c_rdepend_shortcut = '\x02';
const char Depend::c_pdepend_shortcut = '\x03';
const char Depend::c_bdepend_shortcut = '\x04';
const char Depend::c_idepend_shortcut = '\x05';

static bool subst_the_same(string *in, const string& from, char shortcut);

static bool subst_the_same(string *in, const string& from, char shortcut) {
	if(from.empty()) {
		return false;
	}
	string::size_type pos(in->find(from));
	if(pos == string::npos) {
		return false;
	}
	string::size_type len(from.size());
	string::size_type next(pos + len);
	if(next < in->size()) {
		if((*in)[next] != ' ') {
			return false;
		}
	}
	if(pos > 0) {
		if((*in)[pos - 1] != ' ') {
			return false;
		}
	}
	in->erase(pos, len - 1);
	(*in)[pos] = shortcut;
	return true;
}

void Depend::set(const string& depend, const string& rdepend, const string& pdepend, const string& bdepend, const string& idepend, bool normspace) {
	m_depend = depend;
	m_rdepend = rdepend;
	m_pdepend = pdepend;
	m_bdepend = bdepend;
	m_idepend = idepend;
	if(normspace) {
		trimall(&m_depend);
		trimall(&m_rdepend);
		trimall(&m_pdepend);
		trimall(&m_bdepend);
		trimall(&m_idepend);
	}
	// Try to shorten everything by everything else.
	// We use a fixed priority to avoid cycles and ensure deterministic results.
	// RDEPEND often contains DEPEND
	subst_the_same(&m_rdepend, m_depend, c_depend_shortcut);
	// PDEPEND often contains RDEPEND or DEPEND
	subst_the_same(&m_pdepend, m_rdepend, c_rdepend_shortcut) || \
		subst_the_same(&m_pdepend, m_depend, c_depend_shortcut);
	// BDEPEND often contains DEPEND
	subst_the_same(&m_bdepend, m_depend, c_depend_shortcut);
	// IDEPEND often contains DEPEND
	subst_the_same(&m_idepend, m_depend, c_depend_shortcut);
	// Finally, also DEPEND might contain RDEPEND (rare but possible)
	if(m_depend.find(c_depend_shortcut) == string::npos) {
		subst_the_same(&m_depend, m_rdepend, c_rdepend_shortcut);
	}
	obsolete = false;
}

string Depend::expand(const string& in, bool brief) const {
	if(in.empty()) {
		return in;
	}
	string ret(in);
	// We must expand in an order which allows nested shortcuts to work.
	// Since set() uses m_depend to shorten others, we must expand DEPEND last.
	if(brief) {
		subst(ret, c_idepend_shortcut, c_idepend_name, obsolete);
		subst(ret, c_bdepend_shortcut, c_bdepend_name, obsolete);
		subst(ret, c_pdepend_shortcut, c_pdepend_name, obsolete);
		subst(ret, c_rdepend_shortcut, c_rdepend_name, obsolete);
		subst(ret, c_depend_shortcut,  c_depend_name,  obsolete);
	} else {
		subst(ret, c_idepend_shortcut, m_idepend, obsolete);
		subst(ret, c_bdepend_shortcut, m_bdepend, obsolete);
		subst(ret, c_pdepend_shortcut, m_pdepend, obsolete);
		subst(ret, c_rdepend_shortcut, m_rdepend, obsolete);
		subst(ret, c_depend_shortcut,  m_depend,  obsolete);
	}
	return ret;
}

void Depend::subst(string& ret, char shortcut, const string& text, bool obs) {
	string::size_type pos;
	while((pos = ret.find(shortcut)) != string::npos) {
		if(unlikely(obs) && ((pos + 1) != ret.size())) {
			ret[pos] = ' ';
			if(pos > 0) {
				ret.insert(++pos, 1, ' ');
			}
		} else if(unlikely(obs) && (pos > 0)) {
			ret[pos++] = ' ';
		} else {
			ret.erase(pos, 1);
		}
		ret.insert(pos, text);
	}
}

bool Depend::operator==(const Depend& d) const {
	return ((get_depend() == d.get_depend()) &&
		(get_rdepend() == d.get_rdepend()) &&
		(get_pdepend() == d.get_pdepend()) &&
		(get_bdepend() == d.get_bdepend()) &&
		(get_idepend() == d.get_idepend()));
}
