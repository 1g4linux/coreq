// vim:set noet cinoptions=g0,t0,^-2 sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREPKG_BASICVERSION_H_
#define SRC_COREPKG_BASICVERSION_H_ 1

#include <config.h>  // IWYU pragma: keep

#include <string>
#include <vector>

#include "coreqTk/attribute.h"
#include "coreqTk/diagnostics.h"
#include "coreqTk/dialect.h"
#include "coreqTk/coreqint.h"

class Database;

// check_includes: include "corepkg/basicversion.h"

class BasicPart {
	public:
		enum PartType {
			garbage,
			alpha,
			beta,
			pre,
			rc,
			revision,
			inter_rev,
			patch,
			character,
			primary,
			first
		};

		// This must be larger than PartType elements and should be a power of 2.
		static CONSTEXPR const std::string::size_type max_type = 32;
		PartType parttype;
		std::string partcontent;

		BasicPart() {
		}

		explicit BasicPart(PartType p) : parttype(p), partcontent() {
		}

		BasicPart(PartType p, const std::string& s) : parttype(p), partcontent(s) {
		}

		BasicPart(PartType p, const std::string& s, std::string::size_type start)
			: parttype(p), partcontent(s, start) {
		}

		BasicPart(PartType p, const std::string& s, std::string::size_type start, std::string::size_type end)
			: parttype(p), partcontent(s, start, end) {
		}

		BasicPart(PartType p, char c) : parttype(p), partcontent(1, c) {
		}

		ATTRIBUTE_NONNULL_ BasicPart(PartType p, const char *s) : parttype(p), partcontent(s) {
		}

		ATTRIBUTE_PURE static coreq::SignedBool compare(const BasicPart& left, const BasicPart& right);

		ATTRIBUTE_PURE static bool equal_but_right_is_cut(const BasicPart& left, const BasicPart& right);
};


/**
Parse and represent a corepkg version-string.
**/
class BasicVersion {
		friend class Database;

	private:
		/**
		Compare the version
		**/
		ATTRIBUTE_PURE static coreq::SignedBool compare(const BasicVersion& right, const BasicVersion& left, bool right_maybe_shorter);

	public:
		enum ParseResult {
			parsedOK,
			parsedError,
			parsedGarbage
		};

WSUGGEST_FINAL_METHODS_OFF
		virtual ~BasicVersion() { }
WSUGGEST_FINAL_METHODS_ON

		/**
		Parse the version-string pointed to by str
		**/
		BasicVersion::ParseResult parseVersion(const std::string& str, std::string *errtext, coreq::SignedBool accept_garbage);
		BasicVersion::ParseResult parseVersion(const std::string& str, std::string *errtext) {
			return parseVersion(str, errtext, 1);
		}

		/**
		Compare all except gentoo revisions
		**/
		ATTRIBUTE_PURE static coreq::SignedBool compareTilde(const BasicVersion& left, const BasicVersion& right);

		/**
		Compare the version
		**/
		static coreq::SignedBool compare(const BasicVersion& left, const BasicVersion& right) {
			return BasicVersion::compare(left, right, false);
		}

		/**
		Compare the version, but return equality if right is shorter
		**/
		static coreq::SignedBool compare_right_maybe_shorter(const BasicVersion& left, const BasicVersion& right) {
			return BasicVersion::compare(left, right, true);
		}

		/**
		Compare the version
		**/

		std::string getFull() const;

		std::string getPlain() const;

		std::string getRevision() const;

	protected:
		/**
		Splitted m_primsplit-version
		**/
		typedef std::vector<BasicPart> PartsType;
		PartsType m_parts;
};


/**
Short compare-stuff
**/
ATTRIBUTE_PURE inline static bool operator<(const BasicVersion& left, const BasicVersion& right);
inline static bool operator<(const BasicVersion& left, const BasicVersion& right) {
	return BasicVersion::compare(left, right) < 0;
}

ATTRIBUTE_PURE inline static bool operator>(const BasicVersion& left, const BasicVersion& right);
inline static bool operator>(const BasicVersion& left, const BasicVersion& right) {
	return BasicVersion::compare(left, right) > 0;
}

ATTRIBUTE_PURE inline static bool operator==(const BasicVersion& left, const BasicVersion& right);
inline static bool operator==(const BasicVersion& left, const BasicVersion& right) {
	return BasicVersion::compare(left, right) == 0;
}

ATTRIBUTE_PURE inline static bool operator!=(const BasicVersion& left, const BasicVersion& right);
inline static bool operator!=(const BasicVersion& left, const BasicVersion& right) {
	return BasicVersion::compare(left, right) != 0;
}

ATTRIBUTE_PURE inline static bool operator>=(const BasicVersion& left, const BasicVersion& right);
inline static bool operator>=(const BasicVersion& left, const BasicVersion& right) {
	return BasicVersion::compare(left, right) >= 0;
}

ATTRIBUTE_PURE inline static bool operator<=(const BasicVersion& left, const BasicVersion& right);
inline static bool operator<=(const BasicVersion& left, const BasicVersion& right) {
	return BasicVersion::compare(left, right) <= 0;
}

#endif  // SRC_COREPKG_BASICVERSION_H_
