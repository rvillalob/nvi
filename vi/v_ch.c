/*-
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: v_ch.c,v 8.8 1994/04/09 18:21:05 bostic Exp $ (Berkeley) $Date: 1994/04/09 18:21:05 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>

#include "compat.h"
#include <db.h>
#include <regex.h>

#include "vi.h"
#include "vcmd.h"

static void notfound __P((SCR *, ARG_CHAR_T));
static void noprev __P((SCR *));

/*
 * v_chrepeat -- [count];
 *	Repeat the last F, f, T or t search.
 */
int
v_chrepeat(sp, ep, vp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
{
	vp->character = VIP(sp)->lastckey;

	switch (VIP(sp)->csearchdir) {
	case CNOTSET:
		noprev(sp);
		return (1);
	case FSEARCH:
		return (v_chF(sp, ep, vp));
	case fSEARCH:
		return (v_chf(sp, ep, vp));
	case TSEARCH:
		return (v_chT(sp, ep, vp));
	case tSEARCH:
		return (v_cht(sp, ep, vp));
	default:
		abort();
	}
	/* NOTREACHED */
}

/*
 * v_chrrepeat -- [count],
 *	Repeat the last F, f, T or t search in the reverse direction.
 */
int
v_chrrepeat(sp, ep, vp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
{
	enum cdirection savedir;
	int rval;

	vp->character = VIP(sp)->lastckey;
	savedir = VIP(sp)->csearchdir;

	switch (VIP(sp)->csearchdir) {
	case CNOTSET:
		noprev(sp);
		return (1);
	case FSEARCH:
		rval = v_chf(sp, ep, vp);
		break;
	case fSEARCH:
		rval = v_chF(sp, ep, vp);
		break;
	case TSEARCH:
		rval = v_cht(sp, ep, vp);
		break;
	case tSEARCH:
		rval = v_chT(sp, ep, vp);
		break;
	default:
		abort();
	}
	VIP(sp)->csearchdir = savedir;
	return (rval);
}

/*
 * v_cht -- [count]tc
 *	Search forward in the line for the next occurrence of the character.
 *	Place the cursor on it if it's a motion command, to its left if not.
 */
int
v_cht(sp, ep, vp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
{
	if (v_chf(sp, ep, vp))
		return (1);

	/*
	 * v_chf places the cursor on the character, and the 't' command
	 * wants it to its left.  We know this is safe since we had to
	 * have moved right for v_chf() to have succeeded.
	 */
	--vp->m_stop.cno;

	VIP(sp)->csearchdir = tSEARCH;
	return (0);
}

/*
 * v_chf -- [count]fc
 *	Search forward in the line for the next occurrence of the character.
 */
int
v_chf(sp, ep, vp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
{
	size_t len;
	recno_t lno;
	u_long cnt;
	int key;
	char *endp, *p, *startp;

	/*
	 * !!!
	 * If it's a dot command, it doesn't reset the key for which we're
	 * searching, e.g. in "df1|f2|.|;", the ';' searches for a '2'.
	 */
	key = vp->character;
	if (!F_ISSET(vp, VC_ISDOT))
		VIP(sp)->lastckey = key;
	VIP(sp)->csearchdir = fSEARCH;

	if ((p = file_gline(sp, ep, vp->m_start.lno, &len)) == NULL) {
		if (file_lline(sp, ep, &lno))
			return (1);
		if (lno == 0) {
			notfound(sp, key);
			return (1);
		}
		GETLINE_ERR(sp, vp->m_start.lno);
		return (1);
	}

	if (len == 0) {
		notfound(sp, key);
		return (1);
	}

	endp = (startp = p) + len;
	p += vp->m_start.cno;
	for (cnt = F_ISSET(vp, VC_C1SET) ? vp->count : 1; cnt--;) {
		while (++p < endp && *p != key);
		if (p == endp) {
			notfound(sp, key);
			return (1);
		}
	}

	vp->m_stop.cno = p - startp;

	/*
	 * Non-motion commands move to the end of the range.  VC_D and
	 * VC_Y stay at the start.  Ignore VC_C and VC_S.
	 */
	vp->m_final = ISMOTION(vp) ? vp->m_start : vp->m_stop;
	return (0);
}

/*
 * v_chT -- [count]Tc
 *	Search backward in the line for the next occurrence of the character.
 *	Place the cursor to its right.
 */
int
v_chT(sp, ep, vp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
{
	if (v_chF(sp, ep, vp))
		return (1);

	/*
	 * v_chF places the cursor on the character, and the 'T' command
	 * wants it to its right.  We know this is safe since we had to
	 * have moved left for v_chF() to have succeeded.
	 */
	++vp->m_stop.cno;
	++vp->m_final.cno;

	VIP(sp)->csearchdir = TSEARCH;
	return (0);
}

/*
 * v_chF -- [count]Fc
 *	Search backward in the line for the next occurrence of the character.
 *	Place the cursor on it.
 */
int
v_chF(sp, ep, vp)
	SCR *sp;
	EXF *ep;
	VICMDARG *vp;
{
	recno_t lno;
	size_t len;
	u_long cnt;
	int key;
	char *endp, *p;

	/*
	 * !!!
	 * If it's a dot command, it doesn't reset the key for which
	 * we're searching, e.g. in "df1|f2|.|;", the ';' searches
	 * for a '2'.
	 */
	key = vp->character;
	if (!F_ISSET(vp, VC_ISDOT))
		VIP(sp)->lastckey = key;
	VIP(sp)->csearchdir = FSEARCH;

	if ((p = file_gline(sp, ep, vp->m_start.lno, &len)) == NULL) {
		if (file_lline(sp, ep, &lno))
			return (1);
		if (lno == 0) {
			notfound(sp, key);
			return (1);
		}
		GETLINE_ERR(sp, vp->m_start.lno);
		return (1);
	}

	if (len == 0) {
		notfound(sp, key);
		return (1);
	}

	endp = p - 1;
	p += vp->m_start.cno;
	for (cnt = F_ISSET(vp, VC_C1SET) ? vp->count : 1; cnt--;) {
		while (--p > endp && *p != key);
		if (p == endp) {
			notfound(sp, key);
			return (1);
		}
	}

	vp->m_stop.cno = (p - endp) - 1;

	/*
	 * VC_D and non-motion commands move to the end of the range,
	 * VC_Y stays at the start.  Ignore VC_C and VC_S.  Motion
	 * commands adjust the starting point to the character before
	 * the current one.
	 */
	vp->m_final = F_ISSET(vp, VC_Y) ? vp->m_start : vp->m_stop;
	if (ISMOTION(vp))
		--vp->m_start.cno;
	return (0);
}

static void
noprev(sp)
	SCR *sp;
{
	msgq(sp, M_BERR, "No previous F, f, T or t search.");
}

static void
notfound(sp, ch)
	SCR *sp;
	ARG_CHAR_T ch;
{
	msgq(sp, M_BERR, "%s not found.", KEY_NAME(sp, ch));
}
