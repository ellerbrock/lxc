/*
 * lxc: linux Container library
 *
 * (C) Copyright IBM Corp. 2007, 2008
 *
 * Authors:
 * Daniel Lezcano <daniel.lezcano at free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "conf.h"
#include "log.h"
#include "start.h"
#include "utils.h"

lxc_log_define(lxc_execute, lxc_start);

static int execute_start(struct lxc_handler *handler, void* data)
{
	int j, i = 0;
	struct execute_args *my_args = data;
	char **argv;
	int argc = 0, argc_add;

	while (my_args->argv[argc++]);

	/* lxc-init -n name -- [argc] NULL -> 5 */
	argc_add = 5;
	if (my_args->quiet)
		argc_add++;
	if (!handler->conf->rootfs.path)
		argc_add += 2;
	if (lxc_log_has_valid_level())
		argc_add += 2;

	argv = malloc((argc + argc_add) * sizeof(*argv));
	if (!argv)
		goto out1;

	if (!my_args->init_path)
		goto out2;

	argv[i++] = my_args->init_path;

	argv[i++] = "-n";
	argv[i++] = (char *)handler->name;

	if (lxc_log_has_valid_level()) {
		argv[i++] = "-l";
		argv[i++] = (char *)lxc_log_priority_to_string(lxc_log_get_level());
	}

	if (handler->conf->logfile) {
		argv[i++] = "-o";
		argv[i++] = (char *)handler->conf->logfile;
	}

	if (my_args->quiet)
		argv[i++] = "--quiet";

	if (!handler->conf->rootfs.path) {
		argv[i++] = "-P";
		argv[i++] = (char *)handler->lxcpath;
	}

	argv[i++] = "--";
	for (j = 0; j < argc; j++)
		argv[i++] = my_args->argv[j];
	argv[i++] = NULL;

	NOTICE("Exec'ing \"%s\"", my_args->argv[0]);

	execvp(argv[0], argv);
	SYSERROR("Failed to exec %s", argv[0]);

out2:
	free(argv);
out1:
	return 1;
}

static int execute_post_start(struct lxc_handler *handler, void* data)
{
	struct execute_args *my_args = data;
	NOTICE("'%s' started with pid '%d'", my_args->argv[0], handler->pid);
	return 0;
}

static struct lxc_operations execute_start_ops = {
	.start = execute_start,
	.post_start = execute_post_start
};

int lxc_execute(const char *name, char *const argv[], int quiet,
		struct lxc_handler *handler, const char *lxcpath,
		bool backgrounded, int *error_num)
{
	struct execute_args args = {.argv = argv, .quiet = quiet};

	if (lxc_check_inherited(handler->conf, false, &handler->conf->maincmd_fd, 1))
		return -1;

	handler->conf->is_execute = 1;
	return __lxc_start(name, handler, &execute_start_ops, &args, lxcpath,
			   backgrounded, error_num);
}
