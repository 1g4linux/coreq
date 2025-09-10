// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#include "various/dispatch.h"
#include <config.h>
#include <iostream>
#include <vector>
#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"

SubcommandDispatcher::SubcommandDispatcher() {}

SubcommandDispatcher::~SubcommandDispatcher() {
  for (std::map<std::string, Subcommand*>::iterator it = m_subcommands.begin(); it != m_subcommands.end(); ++it) {
    delete it->second;
  }
}

void SubcommandDispatcher::registerSubcommand(Subcommand* cmd) { m_subcommands[cmd->name()] = cmd; }

int SubcommandDispatcher::dispatch(int argc, char** argv) {
  if (argc < 2) {
    showHelp();
    return EXIT_FAILURE;
  }

  std::string cmdName = argv[1];
  if (cmdName == "-h" || cmdName == "--help" || cmdName == "help") {
    showHelp();
    return EXIT_SUCCESS;
  }

  std::map<std::string, Subcommand*>::iterator it = m_subcommands.find(cmdName);
  if (it == m_subcommands.end()) {
    coreq::say_error(_("Unknown subcommand: %s")) % cmdName;
    showHelp();
    return EXIT_FAILURE;
  }

  // Shift arguments: argv[0] becomes "q <subcommand>", argv[1...] are the rest
  std::string newName = std::string(argv[0]) + " " + cmdName;
  std::vector<char*> newArgv;
  newArgv.push_back(const_cast<char*>(newName.c_str()));
  for (int i = 2; i < argc; ++i) {
    newArgv.push_back(argv[i]);
  }

  return it->second->run(static_cast<int>(newArgv.size()), newArgv.data());
}

void SubcommandDispatcher::showHelp() {
  coreq::say(_("Usage: q <subcommand> [options]\n\nAvailable subcommands:"));
  for (std::map<std::string, Subcommand*>::iterator it = m_subcommands.begin(); it != m_subcommands.end(); ++it) {
    std::string name = it->first;
    if (name.length() < 12) {
      name.append(12 - name.length(), ' ');
    }
    coreq::say("  %s %s") % name % it->second->description();
  }
  coreq::say(_("\nType 'q <subcommand> --help' for help on a specific subcommand."));
}
