// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#include <config.h>
#include "various/dispatch.h"
#include "various/subcommand_list.h"

int run_q(int argc, char** argv) {
  SubcommandDispatcher dispatcher;
  dispatcher.registerSubcommand(new SubcommandList());
  return dispatcher.dispatch(argc, argv);
}
