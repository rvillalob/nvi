/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_util.c,v 8.1 1993/06/09 22:26:12 bostic Exp $ (Berkeley) $Date: 1993/06/09 22:26:12 $";
#endif /* not lint */

#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "vi.h"

/*
 * ex_getline --
 *	Return a line from the terminal.
 */
int
ex_getline(sp, fp, lenp)
	SCR *sp;
	FILE *fp;
	size_t *lenp;
{
	size_t off;
	int ch;
	char *p;

	for (off = 0, p = sp->ibp;; ++off) {
		ch = getc(fp);
		if (off >= sp->ibp_len) {
			BINC(sp, sp->ibp, sp->ibp_len, off + 1);
			p = sp->ibp + off;
		}
		if (ch == EOF || ch == '\n') {
			if (ch == EOF && !off)
				return (1);
			*lenp = off;
			return (0);
		}
		*p++ = ch;
	}
	/* NOTREACHED */
}

/*
 * set_altfname --
 *	Set the alternate file name.
 *
 * Swap the alternate file name.  The reason it's a routine is that I wanted
 * some place to hang this comment.  The alternate file name (normally
 * referenced using the special character '#' during file expansion) is set
 * by a large number of operations.  In the historic vi, the commands "ex",
 * "edit" and "next file" obviously set the alternate file name because they
 * switched the underlying file.  Less obviously, the "read", "file", "write"
 * and "wq" commands set it as well.  In this implementation, the new
 * commands "previous", "split", and "mkexrc" have been added to the list.
 * Where it gets interesting is that the alternate file name is set multiple
 * times by some commands.  If an edit attempt fails (for whatever reason,
 * like the current file is modified but as yet unwritten), it is set to the
 * file name that the user didn't get to edit.  If the edit succeeds, it is
 * set to the last file name that was edited.  Good fun.
 *
 * XXX: add errlist if it doesn't get ripped out.
 */
void
set_altfname(sp, altfname)
	SCR *sp;
	char *altfname;
{
	if (sp->altfname != NULL)
		FREE(sp->altfname, strlen(sp->altfname));
	if ((sp->altfname = strdup(altfname)) == NULL)
		msgq(sp, M_ERR, "Error: %s", strerror(errno));
}
