/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_scroll.c,v 5.1 1992/05/15 11:15:03 bostic Exp $ (Berkeley) $Date: 1992/05/15 11:15:03 $";
#endif /* not lint */

#include <sys/types.h>
#include <stdio.h>
#include <curses.h>

#include "vi.h"
#include "options.h"
#include "vcmd.h"
#include "extern.h"

#define	DOWN(lno) {							\
	if (file_gline(curf, lno, NULL) == NULL) {			\
		v_eof(cp);						\
		return (1);						\
	}								\
	rp->lno = lno;							\
}

/*
 * v_nbdown -- [count]^M, [count]+
 *	Move down by lines, moving cursor to first non-blank character.
 */
int
v_nbdown(vp, cp, rp)
	VICMDARG *vp;
	MARK *cp, *rp;
{
	u_long lno;

	lno = cp->lno + (vp->flags & VC_C1SET ? vp->count : 1);

	DOWN(lno);
	return (v_nonblank(rp));
}

/*
 * v_down -- [count]^J, [count]^N, [count]j
 *	Move down by lines.
 */
int
v_down(vp, cp, rp)
	VICMDARG *vp;
	MARK *cp, *rp;
{
	u_long lno;

	lno = cp->lno + (vp->flags & VC_C1SET ? vp->count : 1);

	DOWN(lno);
	rp->cno = cp->cno;
	return (0);
}

/*
 * v_hpagedown -- [count]^D
 *	Page down half screens.
 */
int
v_hpagedown(vp, cp, rp)
	VICMDARG *vp;
	MARK *cp, *rp;
{
	u_long lno;

	/* 
	 * Half screens set the scroll value even if the command ultimately
	 * failed in historic vi.  It's probably a don't care.
	 */
	if (vp->flags & VC_C1SET)
		LVAL(O_SCROLL) = vp->count;
	else
		vp->count = LVAL(O_SCROLL);

	/* Half screens always succeed unless at the bottom of the file. */
	lno = cp->lno + vp->count;

	if (file_gline(curf, lno, NULL) == NULL) {
		lno = file_lline(curf);
		if (lno == cp->lno) {
			v_eof(cp);
			return (1);
		}
	}
	rp->lno = lno;
	return (v_nonblank(rp));
}

/*
 * v_pagedown -- [count]^F
 *	Page down by screens.
 */
int
v_pagedown(vp, cp, rp)
	VICMDARG *vp;
	MARK *cp, *rp;
{
	u_long lno;

	/* Calculation from POSIX 1003.2/D8. */
	lno = cp->lno + (vp->flags & VC_C1SET ? vp->count : 1) * (LINES - 2);

	DOWN(lno);
	return (v_nonblank(rp));
}

/*
 * v_lineup -- [count]^E
 *	Page up by lines.
 */
int
v_linedown(vp, cp, rp)
	VICMDARG *vp;
	MARK *cp, *rp;
{
	u_long off;

	off = vp->flags & VC_C1SET ? vp->count : 1;

	/* If the line exists, move to it. */
	if (file_gline(curf, BOTLINE + off, NULL) == NULL) {
		v_eof(cp);
		return (1);
	}

	/*
	 * Alter the topline.
	 * XXX
	 * This is really sleazy; the problem is that the cursor doesn't
	 * move based on the command movement, instead it moves relative
	 * to the top line of the screen.  *snarl*
	 */
	topline += vp->count;

	/*
	 * The cursor moves up, staying with its original line, unless it
	 * reaches the top of the screen.
	 */
	rp->lno =
	    vp->count >= cp->lno - topline ? 1 : cp->lno - vp->count;
	rp->cno = cp->cno;
	return (0);
}
