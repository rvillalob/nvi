/*-
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: db.c,v 5.6 1992/11/02 22:11:11 bostic Exp $ (Berkeley) $Date: 1992/11/02 22:11:11 $";
#endif /* not lint */

#include <sys/param.h>

#include <db.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "exf.h"
#include "screen.h"
#include "extern.h"

/*
 * file_gline --
 *	Retrieve a line from the file.
 */
u_char *
file_gline(ep, lno, lenp)
	EXF *ep;
	recno_t lno;				/* Line number. */
	size_t *lenp;				/* Length store. */
{
	DBT data, key;
	TEXT *tp;
	recno_t cnt;

	/*
	 * The underlying recno stuff handles zero by returning NULL, but
	 * have to have an oob condition for the look-aside into the input
	 * buffer anyway.
	 */
	if (lno == 0)
		return (NULL);

	/*
	 * Look-aside into the input buffer and see if the line we want is
	 * there.
	 */
	if (ib.stop.lno >= lno && ib.start.lno <= lno) {
		if (ib.stop.lno == lno) {
			if (lenp)
				*lenp = ib.len;
			return (ib.ilb);
		}
		for (cnt = ib.start.lno, tp = ib.head; cnt < lno; ++cnt)
			tp = tp->next;
		if (lenp)
			*lenp = tp->len;
		return (tp->lp);
	}

	/* Check the cache. */
	if (lno == ep->c_lno) {
		if (lenp)
			*lenp = ep->c_len;
		return (ep->c_lp);
	}
	ep->c_lno = OOBLNO;

	/* Get the line from the underlying database. */
	key.data = &lno;
	key.size = sizeof(lno);
	switch((ep->db->get)(ep->db, &key, &data, 0)) {
        case -1:
		bell();
		msg("Error: %s/%d: unable to get line %u: %s.",
		    tail(__FILE__), __LINE__, lno, strerror(errno));
		/* FALLTHROUGH */
        case 1:
		return (NULL);
		/* NOTREACHED */
	}
	if (lenp)
		*lenp = data.size;

	/* Fill the cache. */
	ep->c_lno = lno;
	ep->c_len = data.size;
	ep->c_lp = data.data;

	return (data.data);
}

/*
 * file_dline --
 *	Delete a line from the file.
 */
int
file_dline(ep, lno)
	EXF *ep;
	recno_t lno;
{
	DBT key;

#if DEBUG && 1
	TRACE("delete line %lu\n", lno);
#endif

	/* Update file. */
	key.data = &lno;
	key.size = sizeof(lno);
	if ((ep->db->del)(ep->db, &key, 0) == 1) {
		bell();
		msg("Error: %s/%d: unable to delete line %u: %s.",
		    tail(__FILE__), __LINE__, lno, strerror(errno));
		return (1);
	}

	/* Flush the cache, update line count, before screen update. */
	if (lno <= ep->c_lno)
		ep->c_lno = OOBLNO;
	if (ep->c_nlines != OOBLNO)
		--ep->c_nlines;

	/* Update screen. */
	scr_update(ep, lno, NULL, 0, LINE_DELETE);

	/* File now dirty. */
	ep->flags |= F_MODIFIED;
	return (0);
}

/*
 * file_aline --
 *	Append a line into the file.
 */
int
file_aline(ep, lno, p, len)
	EXF *ep;
	recno_t lno;
	u_char *p;
	size_t len;
{
	DBT data, key;

#if DEBUG && 1
	TRACE("append to %lu: len %u {%.*s}\n", lno, len, MIN(len, 20), p);
#endif

	/* Update file. */
	key.data = &lno;
	key.size = sizeof(lno);
	data.data = p;
	data.size = len;
	if ((ep->db->put)(ep->db, &key, &data, R_IAFTER) == -1) {
		bell();
		msg("Error: %s/%d: unable to append to line %u: %s.",
		    tail(__FILE__), __LINE__, lno, strerror(errno));
		return (1);
	}

	/* Flush the cache, update line count, before screen update. */
	if (lno >= ep->c_lno)
		ep->c_lno = OOBLNO;
	if (ep->c_nlines != OOBLNO)
		++ep->c_nlines;

	/* Update screen. */
	scr_update(ep, lno, p, len, LINE_APPEND);

	/* File now dirty. */
	ep->flags |= F_MODIFIED;
	return (0);
}

/*
 * file_iline --
 *	Insert a line into the file.
 */
int
file_iline(ep, lno, p, len)
	EXF *ep;
	recno_t lno;
	u_char *p;
	size_t len;
{
	DBT data, key;

#if DEBUG && 1
	TRACE("insert before %lu: len %u {%.*s}\n", lno, len, MIN(len, 20), p);
#endif

	/* Update file. */
	key.data = &lno;
	key.size = sizeof(lno);
	data.data = p;
	data.size = len;
	if ((ep->db->put)(ep->db, &key, &data, R_IBEFORE) == -1) {
		msg("Error: %s/%d: unable to insert at line %u: %s.",
		    tail(__FILE__), __LINE__, lno, strerror(errno));
		return (1);
	}

	/* Flush the cache, update line count, before screen update. */
	if (lno >= ep->c_lno)
		ep->c_lno = OOBLNO;
	if (ep->c_nlines != OOBLNO)
		++ep->c_nlines;

	/* Update screen. */
	scr_update(ep, lno, p, len, LINE_INSERT);

	/* File now dirty. */
	ep->flags |= F_MODIFIED;
	return (0);
}

/*
 * file_sline --
 *	Store a line in the file.
 */
int
file_sline(ep, lno, p, len)
	EXF *ep;
	recno_t lno;
	u_char *p;
	size_t len;
{
	DBT data, key;

#if DEBUG && 1
	TRACE("replace line %lu: len %u {%.*s}\n", lno, len, MIN(len, 20), p);
#endif

	/* Update file. */
	key.data = &lno;
	key.size = sizeof(lno);
	data.data = p;
	data.size = len;
	if ((ep->db->put)(ep->db, &key, &data, 0) == -1) {
		msg("Error: %s/%d: unable to store line %u: %s.",
		    tail(__FILE__), __LINE__, lno, strerror(errno));
		return (1);
	}

	/* Flush the cache, before screen update. */
	if (lno == ep->c_lno)
		ep->c_lno = OOBLNO;

	/* Update screen. */
	scr_update(ep, lno, p, len, LINE_RESET);
	
	/* File now dirty. */
	ep->flags |= F_MODIFIED;
	return (0);
}

/*
 * file_ibresolv --
 *	Resolve the ib structure into the file.
 */
int
file_ibresolv(ep, ibp)
	EXF *ep;
	IB *ibp;
{
	DBT data, key;
	TEXT *tp;
	recno_t lno;

	/* Setup. */
	tp = ib.head;
	lno = ibp->start.lno;
	key.data = &lno;
	key.size = sizeof(lno);

	/* Replace the original line. */
	data.data = tp->lp;
	data.size = tp->len;
	if ((ep->db->put)(ep->db, &key, &data, 0) == -1) {
		msg("Error: %s/%d: unable to store line %u: %s.",
		    tail(__FILE__), __LINE__, lno, strerror(errno));
		return (1);
	}

	/* Add the new lines into the file. */
	for (; tp = tp->next; ++lno) {
		data.data = tp->lp;
		data.size = tp->len;
		if ((ep->db->put)(ep->db, &key, &data, R_IAFTER) == -1) {
			msg("Error: %s/%d: unable to store line %u: %s.",
			    tail(__FILE__), __LINE__, lno, strerror(errno));
			return (1);
		}
	}

	/* Flush the cache, update line count. */
	ep->c_lno = OOBLNO;
	ep->c_nlines = OOBLNO;

	/* File now dirty. */
	ep->flags |= F_MODIFIED;
	return (0);
}

/*
 * file_lline --
 *	Return the number of lines in the file.
 */
recno_t
file_lline(ep)
	EXF *ep;
{
	DBT data, key;
	recno_t lno;

	/* Check the cache. */
	if (ep->c_nlines != OOBLNO)
		return (ep->c_nlines);

	key.data = &lno;
	key.size = sizeof(lno);

	switch((ep->db->seq)(ep->db, &key, &data, R_LAST)) {
        case -1:
		msg("Error: %s/%d: unable to get last line: %s.",
		    tail(__FILE__), __LINE__, strerror(errno));
		/* FALLTHROUGH */
        case 1:
		lno = 0;
		break;
	default:
		bcopy(key.data, &lno, sizeof(lno));
		break;
	}

	/* Fill the cache. */
	ep->c_lno = lno;
	ep->c_len = data.size;
	ep->c_lp = data.data;

	return (lno);
}
