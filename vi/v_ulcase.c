/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_ulcase.c,v 5.17 1993/02/24 13:03:17 bostic Exp $ (Berkeley) $Date: 1993/02/24 13:03:17 $";
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "vi.h"
#include "screen.h"
#include "vcmd.h"

/*
 * v_ulcase -- [count]~
 *	This function toggles upper & lower case letters.  This command
 *	in historic vi ignored the count.  It really should have had an
 *	associated motion, but it's too late to change it now.
 */
int
v_ulcase(ep, vp, fm, tm, rp)
	EXF *ep;
	VICMDARG *vp;
	MARK *fm, *tm, *rp;
{
	register int ch;
	register u_char *p;
	size_t cno, len, nplen;
	recno_t lno;
	u_long cnt;
	int change;
	u_char *np;

	lno = fm->lno;
	cno = fm->cno;

	if ((p = file_gline(ep, lno, &len)) == NULL) {
		if (file_lline(ep) == 0)
			v_eof(ep, NULL);
		else
			GETLINE_ERR(ep, lno);
		return (1);
	}

	np = NULL;
	nplen = 0;
	if (binc(ep, &np, &nplen, len))
		return (1);
	memmove(np, p, len);

	p = np + cno;
	cnt = vp->flags & VC_C1SET ? vp->count : 1;
	for (change = 0; cnt--; ++cno) {
		if (cno == len) {
			if (change) {
				if (file_sline(ep, lno, np, len)) {
					rp->lno = lno;
					rp->cno = cno;
					return (1);
				}
			}
			if ((p = file_gline(ep, ++lno, &len)) == NULL) {
				rp->lno = --lno;
				rp->cno = len - 1;
				return (0);
			}
			if (binc(ep, &np, &nplen, len))
				return (1);
			change = 0;
			cno = 0;
			p = np;
		}

		ch = *p;
		if (islower(ch)) {
			*p++ = toupper(ch);
			change = 1;
		} else if (isupper(ch)) {
			*p++ = tolower(ch);
			change = 1;
		} else
			++p;
	}
	rp->lno = lno;
	rp->cno = cno;
	if (change) {
		if (file_sline(ep, lno, np, len))
			return (1);
	}
	return (0);
}
