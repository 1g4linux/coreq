// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#include <config.h>
#include "various/dispatch.h"
#include "various/subcommand_list.h"
#include "various/subcommand_files.h"
#include "various/subcommand_belongs.h"
#include "various/subcommand_check.h"
#include "various/subcommand_depends.h"
#include "various/subcommand_depgraph.h"
#include "various/subcommand_has.h"
#include "various/subcommand_hasuse.h"
#include "various/subcommand_keywords.h"
#include "various/subcommand_meta.h"
#include "coreqrc/global.h"
#include "coreqrc/coreqrc.h"
#include "corepkg/eapi.h"
#include "corepkg/package.h"
#include "corepkg/extendedversion.h"
#include "corepkg/packagetree.h"
#include "corepkg/conf/corepkgsettings.h"
#include "coreqTk/ansicolor.h"

int run_q(int argc, char** argv) {
  // Initialize static classes
  Eapi::init_static();
  Category::init_static();
  ExtendedVersion::init_static();
  CorePkgSettings::init_static();
  AnsiColor::init_static();

  get_coreqrc(COREQ_VARS_PREFIX);
  SubcommandDispatcher dispatcher;
  dispatcher.registerSubcommand(new SubcommandList());
  dispatcher.registerSubcommand(new SubcommandFiles());
  dispatcher.registerSubcommand(new SubcommandBelongs());
  dispatcher.registerSubcommand(new SubcommandCheck());
  dispatcher.registerSubcommand(new SubcommandDepends());
  dispatcher.registerSubcommand(new SubcommandDepgraph());
  dispatcher.registerSubcommand(new SubcommandHas());
  dispatcher.registerSubcommand(new SubcommandHasuse());
  dispatcher.registerSubcommand(new SubcommandKeywords());
  dispatcher.registerSubcommand(new SubcommandMeta());
  return dispatcher.dispatch(argc, argv);
}
