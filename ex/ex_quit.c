/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_quit.c,v 5.19 1993/04/19 15:29:28 bostic Exp $ (Berkeley) $Date: 1993/04/19 15:29:28 $";
#endif /* not lint */

#include <sys/types.h>

#include "vi.h"
#include "excmd.h"

int
ex_quit(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	int force;

	force = cmdp->flags & E_FORCE;

	/* Historic practice: quit! doesn't do autowrite. */
	if (!force)
		MODIFY_CHECK(sp, ep, 0);

	/*
	 * Historic practice: quit! doesn't check for other files.
	 * Also check for related screens; if they exist, quit.
	 */
	if (!force && ep->refcnt <= 1 && file_next(sp, ep, 0)) {
		msgq(sp, M_ERR,
	"More files; use \":n\" to go to the next file, \":q!\" to quit.");
		return (1);
	}

	F_SET(sp, force ? S_EXIT_FORCE : S_EXIT);
	return (0);
}

int
ex_wq(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	int force;

	force = cmdp->flags & E_FORCE;

	if (file_sync(sp, ep, force))
		return (1);

	if (!force && ep->refcnt <= 1 && file_next(sp, ep, 0)) {
		msgq(sp, M_ERR,
		    "More files to edit; use \":n\" to go to the next file");
		return (1);
	}

	F_SET(sp, force ? S_EXIT_FORCE : S_EXIT);
	return (0);
}

int
ex_xit(sp, ep, cmdp)
	SCR *sp;
	EXF *ep;
	EXCMDARG *cmdp;
{
	int force;

	force = cmdp->flags & E_FORCE;

	MODIFY_CHECK(sp, ep, force);

	F_SET(ep, force ? S_EXIT_FORCE : S_EXIT);
	return (0);
}
