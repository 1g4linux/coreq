// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#include "corepkg/packagetree.h"
#include <config.h>  // IWYU pragma: keep

#include <string>
#include <utility>

#include "coreqTk/assert.h"
#include "coreqTk/coreqint.h"
#include "coreqTk/likely.h"
#include "coreqTk/null.h"
#include "coreqTk/stringtypes.h"
#include "corepkg/package.h"

using std::pair;
using std::string;

PackagePtr* comparer = NULLPTR;

void Category::init_static() {
  coreq_assert_static(comparer == NULLPTR);
  comparer = new PackagePtr(new Package);
}

Category::iterator Category::find(const std::string& pkg_name) {
  coreq_assert_static(comparer != NULLPTR);
  (*comparer)->name = pkg_name;
  return static_cast<const_iterator>(super::find(*comparer));
}

Category::const_iterator Category::find(const std::string& pkg_name) const {
  coreq_assert_static(comparer != NULLPTR);
  (*comparer)->name = pkg_name;
  return static_cast<const_iterator>(super::find(*comparer));
}

#if 0
bool Category::deletePackage(const std::string& pkg_name) {
	iterator i(find(pkg_name));
	if(i == end()) {
		return false;
	}
	delete *i;
	erase(i);
	return true;
}
#endif

Package* Category::addPackage(const string cat_name, const string& pkg_name) {
  Package* p(new Package(cat_name, pkg_name));
  addPackage(p);
  return p;
}

Category* PackageTree::find(const string& cat_name) const {
  const_iterator f(Categories::find(cat_name));
  if (unlikely(f == end())) {
    return NULLPTR;
  }
  return f->second;
}

Category& PackageTree::insert(const string& cat_name) {
  pair<Categories::iterator, bool> n(Categories::insert(Categories::value_type(cat_name, NULLPTR)));
  Category*& catpoint((n.first)->second);
  if (n.second) {
    return *(catpoint = new Category);
  }
  return *catpoint;
}

void PackageTree::insert(const WordVec& cat_vec) {
  for (WordVec::const_iterator it(cat_vec.begin()); likely(it != cat_vec.end()); ++it) {
    insert(*it);
  }
}

Package* PackageTree::findPackage(const string& cat_name, const string& pkg_name) const {
  const_iterator f(Categories::find(cat_name));
  if (unlikely(f == end())) {
    return NULLPTR;
  }
  return f->second->findPackage(pkg_name);
}

#if 0
bool PackageTree::deletePackage(const string& cat_name, const string& pkg_name) {
	iterator i(Categories::find(cat_name));
	if(i == end()) {
		return false;
	}

	if(i->deletePackage(pkg_name)) {
		return false;
	}
	// Check if the category is empty after deleting the package.
	if(unlikely(i->empty())) {
		erase(i);
	}
	return true;
}
#endif

coreq::Treesize PackageTree::countPackages() const {
  coreq::Treesize ret(0);
  for (const_iterator i(begin()); likely(i != end()); ++i) {
    ret += i->second->size();
  }
  return ret;
}

PackageTree::~PackageTree() {
  for (Categories::const_iterator it(Categories::begin()); likely(it != Categories::end()); ++it) {
    delete it->second;
  }
}
