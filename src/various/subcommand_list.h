// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_LIST_H_
#define SRC_VARIOUS_SUBCOMMAND_LIST_H_ 1

#include <config.h>
#include "main/main.h"
#include "various/subcommand.h"

class SubcommandList : public Subcommand {
 public:
  virtual int run(int argc, char** argv) { return run_coreq(argc, argv); }
  virtual const char* name() const { return "list"; }
  virtual const char* description() const { return "list packages matching a pattern"; }
  virtual const char* usage() const { return "Usage: q list [options] <pattern>"; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_LIST_H_
