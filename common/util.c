/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: util.c,v 5.8 1992/04/28 13:49:45 bostic Exp $ (Berkeley) $Date: 1992/04/28 13:49:45 $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <curses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "vi.h"
#include "options.h"
#include "pathnames.h"
#include "extern.h"

/*
 * bell --
 *	Ring the terminal's bell.
 */
void
bell()
{
	if (ISSET(O_VBELL)) {
		(void)tputs(VB, 1, __putchar);
		(void)fflush(stdout);
	}
	else if (ISSET(O_ERRORBELLS))
		(void)write(STDOUT_FILENO, "\007", 1);	/* '\a' */
}

/*
 * __putchar --
 *	Functional version of putchar, for tputs.
 */
void
__putchar(ch)
	int ch;
{
	(void)putchar(ch);
}

/*
 * parseptrn --
 */
char *
parseptrn(ptrn)
	register char *ptrn;
{
	register char sep;

	/*
	 * Return a pointer to the end of the string or the first character
	 * after the second occurrence of the first character.  In the latter
	 * case, replace the second occurrence with a '\0'.  Used to parse
	 * search strings.
	 */
	for (sep = *ptrn; *++ptrn && *ptrn != sep;)
		/* Backslash escapes the next character. */
		if (ptrn[0] == '\\' && ptrn[1] != '\0')
			++ptrn;
	if (*ptrn)
		*ptrn++ = '\0';
	return (ptrn);
}

/*
 * onhup --
 *	Handle SIGHUP, restoring sanity to the terminal, preserving the file.
 */
/* ARGSUSED */
void
onhup(signo)
	int signo;
{
	char buf[MAXPATHLEN + sizeof(_PATH_PRESERVE) + 2];

	/* Restore the terminal's sanity. */
	endwin();

	/* If we had a temp file going, then preserve it. */
	if (tmpnum > 0 && tmpfd >= 0) {
		(void)close(tmpfd);
#ifdef RIP_THIS_OUT
		(void)snprintf(buf, sizeof(buf),
		    "%s %s", _PATH_PRESERVE, tmpname);
		(void)system(tmpblk.c);
#endif
	}

	/* Delete any old temp files. */
	cutend();

	/* Exit with the proper exit status. */
	(void)signal(SIGHUP, SIG_DFL);
	(void)kill(0, SIGHUP);

	/* NOTREACHED */
	exit (1);
}

/*
 * onwinch --
 *	Handle SIGWINCH.
 */
/* ARGSUSED */
void
onwinch(signo)
	int signo;
{
	struct winsize win;
	int len;
	char *p, buf[100];

	/* Try TIOCGWINSZ, and, if it fails, the termcap entry. */
	if (ioctl(STDERR_FILENO, TIOCGWINSZ, &win) != -1 &&
	    win.ws_row != 0 && win.ws_col != 0) {
		LINES = win.ws_row;
		COLS = win.ws_col;
	}  else {
		LINES = tgetnum("li");
		COLS = tgetnum("co");
	}

	/* POSIX 1003.2 requires the environment to override. */
	if ((p = getenv("ROWS")) != NULL)
		LINES = strtol(p, NULL, 10);
	if ((p = getenv("COLUMNS")) != NULL)
		COLS = strtol(p, NULL, 10);

	/* Make sure we got values that we can live with. */
	if (LINES < 2 || COLS < 40) {
		endwin();
		len = snprintf(buf, sizeof(buf),
		    "vi: screen rows %d cols %d: too small\n", LINES, COLS);
		(void)write(STDERR_FILENO, buf, len);
		exit(1);
	}
}

/*
 * onstop --
 *	Handle STOP signals.
 */
/* ARGSUSED */
void
onstop(signo)
	int signo;
{
	MARK current;
	sigset_t set;

	current = cursor;
	move(LINES - 1, 0);
	clrtoeol();
	refresh();

	suspend_curses();

	(void)sigemptyset(&set);
	(void)sigaddset(&set, SIGTSTP);
	(void)sigprocmask(SIG_UNBLOCK, &set, NULL);
	(void)signal(SIGTSTP, SIG_DFL);
	kill(0, SIGTSTP);

	/* Time passes... */

	(void)signal(SIGTSTP, onstop);

	cursor = current;
	resume_curses(TRUE);
	touchwin(stdscr);
	redraw(cursor, FALSE);
	refresh();
}
