// vim:set noet cinoptions= sw=4 ts=4:
// This file is part of the coreq project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Martin Väth <martin@mvath.de>

#include <config.h>  // IWYU pragma: keep

#include <unistd.h>

#include <cstdlib>
#include <cstring>

#include <string>

#include "coreqTk/formated.h"
#include "coreqTk/i18n.h"
#include "coreqTk/likely.h"
#include "coreqrc/coreqrc.h"
#include "coreqrc/global.h"
#include "main/main.h"
#include "various/drop_permissions.h"

using std::string;

static void print_help() {
	coreq::say(_("Usage: %s [--] command [arguments]\n"
"Executes \"command [arguments]\" with the permissions according to the coreq\n"
"variables COREQ_USER, COREQ_GROUP, COREQ_UID, and COREQ_GID,\n"
"honouring REQUIRE_DROP and NODROP_FATAL.\n"
"\n"
"This program is covered by the GNU General Public License. See COPYING for\n"
"further information.")) % program_name;
}

int run_coreq_drop_permissions(int argc, char *argv[]) {
	CoreqRc& coreqrc(get_coreqrc(DROP_VARS_PREFIX));
	if(argc > 0) {
		++argv;
		--argc;
	}
	if((argc > 0) && (std::strcmp("--", argv[0]) == 0)) {
		++argv;
		--argc;
	}
	if(argc == 0) {
		print_help();
		return EXIT_FAILURE;
	}
	string errtext;
	bool success(drop_permissions(&coreqrc, &errtext));
	if(unlikely(!errtext.empty())) {
		coreq::say_error() % errtext;
	}
	if(unlikely(!success)) {
		return EXIT_FAILURE;
	}
	execv(argv[0], argv);
	coreq::say_error(_("failed to execute %s")) % argv[0];
	return EXIT_FAILURE;
}
