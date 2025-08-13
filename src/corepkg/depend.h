// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREPKG_DEPEND_H_
#define SRC_COREPKG_DEPEND_H_ 1

#include <config.h>  // IWYU pragma: keep

#include <string>

class Database;
class DBHeader;
class Version;
class PackageTree;

class Depend {
	friend class Database;

	private:
		std::string m_depend, m_rdepend, m_pdepend, m_bdepend, m_idepend;
		bool obsolete;

		static const char c_depend_name[];
		static const char c_rdepend_name[];
		static const char c_pdepend_name[];
		static const char c_bdepend_name[];
		static const char c_idepend_name[];

		static const char c_depend_shortcut;
		static const char c_rdepend_shortcut;
		static const char c_pdepend_shortcut;
		static const char c_bdepend_shortcut;
		static const char c_idepend_shortcut;

		std::string expand(const std::string& in, bool brief) const;
		static void subst(std::string& ret, char shortcut, const std::string& text, bool obs);

	public:
		static bool use_depend;

		Depend() : obsolete(false) {
		}

		void set(const std::string& depend, const std::string& rdepend, const std::string& pdepend, const std::string& bdepend, const std::string& idepend, bool normspace);

		std::string get_depend() const {
			return expand(m_depend, false);
		}

		std::string get_depend_brief() const {
			return expand(m_depend, true);
		}

		std::string get_rdepend() const {
			return expand(m_rdepend, false);
		}

		std::string get_rdepend_brief() const {
			return expand(m_rdepend, true);
		}

		std::string get_pdepend() const {
			return expand(m_pdepend, false);
		}

		std::string get_pdepend_brief() const {
			return expand(m_pdepend, true);
		}

		std::string get_bdepend() const {
			return expand(m_bdepend, false);
		}

		std::string get_bdepend_brief() const {
			return expand(m_bdepend, true);
		}

		std::string get_idepend() const {
			return expand(m_idepend, false);
		}

		std::string get_idepend_brief() const {
			return expand(m_idepend, true);
		}

		bool depend_empty() const {
			return m_depend.empty();
		}

		bool rdepend_empty() const {
			return m_rdepend.empty();
		}

		bool pdepend_empty() const {
			return m_pdepend.empty();
		}

		bool bdepend_empty() const {
			return m_bdepend.empty();
		}

		bool idepend_empty() const {
			return m_idepend.empty();
		}

		bool empty() const {
			return (m_depend.empty() && m_rdepend.empty() && m_pdepend.empty() && m_bdepend.empty() && m_idepend.empty());
		}

		void clear() {
			m_depend.clear();
			m_rdepend.clear();
			m_pdepend.clear();
			m_bdepend.clear();
			m_idepend.clear();
			obsolete = false;
		}

		bool operator==(const Depend& d) const;

		bool operator!=(const Depend& d) const {
			return !(*this == d);
		}
};


#endif  // SRC_COREPKG_DEPEND_H_
