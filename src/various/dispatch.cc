// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.

#include "various/dispatch.h"
#include <config.h>
#include <iostream>
#include <vector>
#include <set>
#include <cstring>
#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"
#include "coreqTk/ansicolor.h"

SubcommandDispatcher::SubcommandDispatcher() {}

SubcommandDispatcher::~SubcommandDispatcher() {
  std::set<Subcommand*> unique_cmds;
  for (std::map<std::string, Subcommand*>::iterator it = m_subcommands.begin(); it != m_subcommands.end(); ++it) {
    unique_cmds.insert(it->second);
  }
  for (std::set<Subcommand*>::iterator it = unique_cmds.begin(); it != unique_cmds.end(); ++it) {
    delete *it;
  }
}

void SubcommandDispatcher::registerSubcommand(Subcommand* cmd) {
  m_subcommands[cmd->name()] = cmd;
  if (cmd->alias()) {
    m_subcommands[cmd->alias()] = cmd;
  }
}

bool SubcommandDispatcher::hasSubcommand(const std::string& cmd_name) const {
  return (m_subcommands.find(cmd_name) != m_subcommands.end());
}

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
  AnsiColor& c = get_ansicolor();
  coreq::say(c.bold(c.yellow(_("Usage:"))) + " " + c.bold(_("q <subcommand> [options]")) + "\n\n" + c.cyan(_("Available subcommands:")));
  
  // To avoid duplicates in help (since both name and alias are in the map),
  // we use a set to keep track of unique subcommand pointers.
  std::set<Subcommand*> unique_cmds;
  for (std::map<std::string, Subcommand*>::iterator it = m_subcommands.begin(); it != m_subcommands.end(); ++it) {
    unique_cmds.insert(it->second);
  }

  for (std::set<Subcommand*>::iterator it = unique_cmds.begin(); it != unique_cmds.end(); ++it) {
    Subcommand* cmd = *it;
    std::string name = cmd->name();
    const char* alias = cmd->alias();
    
    std::string display_name;
    if (alias && std::strlen(alias) == 1) {
      size_t pos = name.find(alias[0]);
      if (pos != std::string::npos) {
        display_name = c.green(name.substr(0, pos)) + c.bold(c.red(std::string("(") + alias[0] + ")")) + c.green(name.substr(pos + 1));
      } else {
        display_name = c.bold(c.red(std::string("(") + alias + ")")) + c.green(name);
      }
    } else {
      display_name = c.green(name);
    }

    std::string line = "  " + display_name;
    // Padding for alignment (manually calculated since display_name contains ANSI codes)
    size_t visible_len = name.length() + (alias ? 2 : 0);
    if (visible_len < 24) {
      line.append(24 - visible_len, ' ');
    }
    line += " " + c.blue(cmd->description());
    
    coreq::say("%s") % line;
  }
  coreq::say(_("\nType '") + c.bold(_("q <subcommand> --help")) + _("' for help on a specific subcommand."));
}
