/* $OpenBSD: cmd-delete-buffer.c,v 1.12 2014/05/13 07:34:35 nicm Exp $ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <stdlib.h>

#include "tmux.h"

/*
 * Delete a paste buffer.
 */

enum cmd_retval	 cmd_delete_buffer_exec(struct cmd *, struct cmd_q *);

const struct cmd_entry cmd_delete_buffer_entry = {
	"delete-buffer", "deleteb",
	"b:", 0, 0,
	CMD_BUFFER_USAGE,
	0,
	NULL,
	cmd_delete_buffer_exec
};

enum cmd_retval
cmd_delete_buffer_exec(struct cmd *self, struct cmd_q *cmdq)
{
	struct args	*args = self->args;
	const char 	*bufname;

	if (!args_has(args, 'b')) {
		paste_free_top();
		return (CMD_RETURN_NORMAL);
	}
	bufname = args_get(args, 'b');

	if (paste_free_name(bufname) != 0) {
		cmdq_error(cmdq, "no buffer %s", bufname);
		return (CMD_RETURN_ERROR);
	}

	return (CMD_RETURN_NORMAL);
}
