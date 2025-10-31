// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_SUBCOMMAND_H_
#define SRC_VARIOUS_SUBCOMMAND_H_ 1

#include <config.h>

#include <string>
#include <vector>

class Subcommand {
 public:
  virtual ~Subcommand() {}
  virtual int run(int argc, char** argv) = 0;
  virtual const char* name() const = 0;
  virtual const char* description() const = 0;
  virtual const char* usage() const = 0;
  virtual const char* alias() const { return nullptr; }
};

#endif  // SRC_VARIOUS_SUBCOMMAND_H_
