// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Wolfgang Frisch <xororand@users.sourceforge.net>
//   Emil Beinroth <emilbeinroth@gmx.net>
//   Martin Väth <martin@mvath.de>

#ifndef SRC_COREQTK_PTR_CONTAINER_H_
#define SRC_COREQTK_PTR_CONTAINER_H_ 1

#include <config.h>  // IWYU pragma: keep

#include "coreqTk/ptr_iterator.h"

// check_includes:  include "coreqTk/ptr_container.h"

namespace coreq {
/**
A set that only stores pointers to type
**/
template <class type>
class ptr_forward_container : public type {
 public:
  using type::begin;
  using type::clear;
  using type::end;

  /**
  Normal access iterator
  **/
  typedef ptr_iterator<typename type::iterator> iterator;

  /**
  Constant access iterator
  **/
  typedef ptr_iterator<typename type::const_iterator> const_iterator;

  void delete_and_clear() {
    delete_all(begin(), end());
    clear();
  }
};

template <class type>
class ptr_container : public ptr_forward_container<type> {
 public:
  /**
  Reverse access iterator
  **/
  typedef ptr_iterator<typename type::reverse_iterator> reverse_iterator;

  /**
  Constant reverse access iterator
  **/
  typedef ptr_iterator<typename type::const_reverse_iterator> const_reverse_iterator;
};

}  // namespace coreq

#endif  // SRC_COREQTK_PTR_CONTAINER_H_
