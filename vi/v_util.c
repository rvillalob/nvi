/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_util.c,v 5.1 1992/05/15 11:15:09 bostic Exp $ (Berkeley) $Date: 1992/05/15 11:15:09 $";
#endif /* not lint */

#include <sys/types.h>
#include <termios.h>
#include <curses.h>
#include <unistd.h>
#include <stdio.h>

#include "vi.h"
#include "options.h"
#include "vcmd.h"
#include "extern.h"

/*
 * v_eof --
 *	Vi end-of-file error.
 */
void
v_eof(mp)
	MARK *mp;
{
	u_long lno;

	bell();
	if (ISSET(O_VERBOSE))
		if (mp == NULL)
			msg("Already at end-of-file.");
		else {
			lno = file_lline(curf);
			if (mp->lno == lno)
				msg("Already at end-of-file.");
			else
				msg("Movement past the end-of-file.");
		}
}

/*
 * v_sof --
 *	Vi start-of-file error.
 */
void
v_sof(mp)
	MARK *mp;
{
	bell();
	if (ISSET(O_VERBOSE))
		if (mp == NULL || mp->lno == 1)
			msg("Already at the beginning of the file.");
		else
			msg("Movement past the beginning of the file.");
}
		
/*
 * v_nonblank --
 *	Set the column number to be the first non-blank character of
 *	the line.
 */
int
v_nonblank(rp)
	MARK *rp;
{
	register int cnt;
	register char *p;
	size_t len;

	EGETLINE(p, rp->lno, len);
	for (cnt = 0; len-- && isspace(*p); ++cnt, ++p);
	rp->cno = cnt;
	return (0);
}

static u_long oldy, oldx;

/*
 * v_startex --
 *	Enter into ex mode.
 */
void
v_startex()
{
	struct termios t;

	/*
	 * Go to the ex window line and clear it.
	 *
	 * XXX
	 * This doesn't work yet; curses needs a line oriented semantic
	 * to force writing regardless of differences.
	 */
	getyx(stdscr, oldy, oldx);
	move(LINES - 1, 0);
	clrtoeol();
	refresh();

	/* Suspend the window. */
	suspendwin();

	/* We have special needs... */
	(void)tcgetattr(STDIN_FILENO, &t);
	cfmakeraw(&t);

	/*
	 * XXX
	 * This line means that we know much too much about the tty driver.
	 * We want raw input, but we want NL -> CR mapping on output.  The
	 * only way to get that in the 4.4BSD tty driver is to have OPOST
	 * turned on and it's turned off by cfmakeraw(3).
	 */
	t.c_oflag |= ONLCR|OPOST;
	(void)tcsetattr(STDIN_FILENO, TCSADRAIN, &t);

	/* Initialize the globals that let vi know what happened in ex. */
	ex_prstate = PR_NONE;
}

/*
 * v_leaveex --
 *	Leave into ex mode.
 */
void
v_leaveex()
{
	/* Restart the curses window, repainting if necessary. */
	restartwin(ex_prstate == PR_PRINTED);

	/* Return to the last cursor position. */
	move(oldy, oldx);
}
