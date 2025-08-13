// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#include "cache/base.h"
#include <config.h>  // IWYU pragma: keep

#include <cstdlib>

#include <string>

#include "coreqTk/attribute.h"
#include "coreqTk/likely.h"
#include "coreqTk/null.h"
#include "coreqTk/stringtypes.h"
#include "corepkg/conf/corepkgsettings.h"
#include "corepkg/package.h"
#include "corepkg/packagetree.h"

using std::string;

ATTRIBUTE_PURE inline static string::size_type revision_index(const string& ver) {
	string::size_type i(ver.rfind("-r"));
	if(i == string::npos) {
		return string::npos;
	}
	if(ver.find_first_not_of("0123456789", i + 2) == string::npos) {
		return i;
	}
	return string::npos;
}

void BasicCache::setScheme(const char *prefix, const char *prefixport, const std::string& scheme) {
	m_scheme = scheme;
	if(use_prefixport()) {
		prefix = prefixport;
	}
	if(prefix != NULLPTR) {
		have_prefix = true;
		m_prefix = prefix;
	} else {
		have_prefix = false;
		m_prefix = "";
	}
	setSchemeFinish();
}

string BasicCache::getPrefixedPath() const {
	if(have_prefix) {
		return m_prefix + m_scheme;
	}
	return m_scheme;
}

string BasicCache::getPathHumanReadable() const {
	string ret(m_scheme);
	if(have_prefix) {
		ret.append(" in ");
		ret.append(m_prefix);
	}
	return ret;
}

void BasicCache::env_add_package(WordIterateMap *env, const Package& package, const Version& version, const string& ebuild_dir, const char *ebuild_full) const {
	string full(version.getFull());
	string eroot;

	// Set default variables

	const char *envptr(std::getenv("PATH"));
	if(likely(envptr != NULLPTR)) {
		(*env)["PATH"] = envptr;
	}
	envptr = std::getenv("ROOT");
	if(unlikely(envptr != NULLPTR)) {
		(*env)["ROOT"] = envptr;
		eroot = envptr + m_prefix;
	} else {
		(*env)["ROOT"] = "/";
		eroot = m_prefix;
	}
	if(have_prefix) {
		(*env)["EPREFIX"] = m_prefix;
		(*env)["EROOT"]   = eroot;
	}
	string portdir((*corepkgsettings)["PORTDIR"]);
	(*env)["ECLASSDIR"] = eroot + portdir + "/eclass";

	// Set variables from corepkgsettings (make.globals/make.conf/...)
	// (Possibly overriding defaults)

	for(CorePkgSettings::const_iterator it(corepkgsettings->begin());
		likely(it != corepkgsettings->end()); ++it) {
		(*env)[it->first] = it->second;
	}

	// Set ebuild-specific variables

	(*env)["EBUILD"]       = ebuild_full;
	(*env)["O"]            = ebuild_dir;
	(*env)["FILESDIR"]     = ebuild_dir + "/files";
	(*env)["EBUILD_PHASE"] = "depend";
	(*env)["CATEGORY"]     = package.category;
	(*env)["PN"]           = package.name;
	(*env)["PVR"]          = full;
	(*env)["PF"]           = package.name + "-" + full;
	string mainversion;
	string::size_type ind(revision_index(full));
	if(ind == string::npos) {
		(*env)["PR"]   = "r0";
		mainversion = full;
	} else {
		(*env)["PR"].assign(full, ind + 1, string::npos);
		mainversion.assign(full, 0, ind);
	}
	(*env)["PV"]           = mainversion;
	(*env)["P"]            = package.name + "-" + mainversion;
}
