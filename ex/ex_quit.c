/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_quit.c,v 8.7 1993/12/10 12:20:58 bostic Exp $ (Berkeley) $Date: 1993/12/10 12:20:58 $";
#endif /* not lint */

#include <sys/types.h>

#include "vi.h"
#include "excmd.h"

/*
 * ex_quit -- :quit[!]
 *	Quit.
 */
int
ex_quit(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	int force;

	force = F_ISSET(cmdp, E_FORCE);

	/* Check for modifications. */
	if (F_ISSET(ep, F_MODIFIED) && ep->refcnt <= 1 && !force) {
		msgq(sp, M_ERR,
		    "Modified since last write; write or use ! to override.");
		return (1);
	}

	/*
	 * !!!
	 * Historic practice: quit! or two quit's done in succession
	 * (where ZZ counts as a quit) didn't check for other files.
	 *
	 * Also check for related screens; if they exist, quit, the
	 * user will get the message on the last screen.
	 */
	if (!force && sp->ccnt != sp->q_ccnt + 1 &&
	    ep->refcnt <= 1 && file_unedited(sp) != NULL) {
		sp->q_ccnt = sp->ccnt;
		msgq(sp, M_ERR,
	"More files; use \":n\" to go to the next file, \":q!\" to quit.");
		return (1);
	}

	F_SET(sp, force ? S_EXIT_FORCE : S_EXIT);
	return (0);
}
