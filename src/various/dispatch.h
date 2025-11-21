// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#ifndef SRC_VARIOUS_DISPATCH_H_
#define SRC_VARIOUS_DISPATCH_H_ 1

#include <config.h>
#include <map>
#include <string>
#include <vector>
#include "various/subcommand.h"

class SubcommandDispatcher {
 public:
  SubcommandDispatcher();
  ~SubcommandDispatcher();

  void registerSubcommand(Subcommand* cmd);
  bool hasSubcommand(const std::string& cmd_name) const;
  int dispatch(int argc, char** argv);
  void showHelp();

 private:
  std::map<std::string, Subcommand*> m_subcommands;
};

#endif  // SRC_VARIOUS_DISPATCH_H_
