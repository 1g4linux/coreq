// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include "coreqTk/statusline.h"
#include <config.h>  // IWYU pragma: keep

#include <string>

#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"

using std::string;

void Statusline::print_force(const string& str) {
  if (soft) {
    coreq::print("\033k%s%s\033\\") % header % str;
  }
  coreq::print("\033]0;%s%s\007", true) % header % str;
}

void Statusline::print(const string& str) {
  if (use) {
    if (header.empty()) {
      header = m_program;
      header.append(": ");
    }
    print_force(str);
  }
}

void Statusline::user_statusline() {
  header.clear();
  if (m_exit[0] == ' ') {
    print_force(m_exit.substr(1));
  }
  else {
    print_force(m_exit);
  }
}

void Statusline::success() {
  if (header.empty()) {
    return;
  }
  if (m_exit.empty()) {
    print_force(P_("Statusline coreq-update", "Finished"));
  }
  else {
    user_statusline();
  }
}

void Statusline::failure() {
  if (header.empty()) {
    return;
  }
  if (m_exit.empty()) {
    print_force(P_("Statusline coreq-update", "Failure"));
  }
  else {
    user_statusline();
  }
}
