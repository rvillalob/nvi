/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	$Id: common.h,v 8.2 1993/07/22 12:05:59 bostic Exp $ (Berkeley) $Date: 1993/07/22 12:05:59 $
 */

					/* Ordered before local includes. */
#include <limits.h>			/* Required by screen.h. */
#include <regex.h>			/* Required by screen.h. */
#include <stdio.h>			/* Required by screen.h. */

/*
 * Forward structure declarations.  Not pretty, but the include files
 * are far too interrelated for a clean solution.
 */
struct _cb;
struct _excmdarg;
struct _excmdlist;
struct _exf;
struct _gs;
struct _hdr;
struct _ib;
struct _mark;
struct _msg;
struct _option;
struct _scr;
struct _seq;
struct _tag;
struct _tagf;
struct _text;

#include "hdr.h"			/* Include before any local includes. */
#include "gs.h"				/* Includes <termios.h>. */
					/* Includes compat.h. */
#include <db.h>				/* Required by exf.h. */

#include "mark.h"			/* Include before cut.h. */
#include "cut.h"

#include "search.h"			/* Include before screen.h. */
#include "options.h"			/* Include before screen.h. */
#include "screen.h"

#include "char.h"
#include "exf.h"
#include "log.h"
#include "msg.h"
#include "seq.h"
#include "term.h"

/* Macros to set/clear/test flags. */
#define	F_SET(p, f)	(p)->flags |= (f)
#define	F_CLR(p, f)	(p)->flags &= ~(f)
#define	F_ISSET(p, f)	((p)->flags & (f))

#define	LF_INIT(f)	flags = (f)
#define	LF_SET(f)	flags |= (f)
#define	LF_CLR(f)	flags &= ~(f)
#define	LF_ISSET(f)	(flags & (f))

/* Memory allocation macros. */
#define	BINC(sp, lp, llen, nlen) {					\
	if ((nlen) > llen && binc(sp, &(lp), &(llen), nlen))		\
		return (1);						\
}
int	binc __P((SCR *, void *, size_t *, size_t));

#define	GET_SPACE(sp, bp, blen, nlen) {					\
	GS *__gp = (sp)->gp;						\
	if (F_ISSET(__gp, G_TMP_INUSE)) {				\
		bp = NULL;						\
		blen = 0;						\
		BINC(sp, bp, blen, nlen); 				\
	} else {							\
		BINC(sp, __gp->tmp_bp, __gp->tmp_blen, nlen);		\
		bp = __gp->tmp_bp;					\
		blen = __gp->tmp_blen;					\
		F_SET(__gp, G_TMP_INUSE);				\
	}								\
}

#define	ADD_SPACE(sp, bp, blen, nlen) {					\
	GS *__gp = (sp)->gp;						\
	if (bp == __gp->tmp_bp) {					\
		F_CLR(__gp, G_TMP_INUSE);				\
		BINC(sp, __gp->tmp_bp, __gp->tmp_blen, nlen);		\
		bp = __gp->tmp_bp;					\
		blen = __gp->tmp_blen;					\
		F_SET(__gp, G_TMP_INUSE);				\
	} else								\
		BINC(sp, bp, blen, nlen);				\
}

#define	FREE_SPACE(sp, bp, blen) {					\
	if (bp == sp->gp->tmp_bp)					\
		F_CLR(sp->gp, G_TMP_INUSE);				\
	else								\
		FREE(bp, blen);						\
}

#ifdef DEBUG
#define	FREE(p, sz) {							\
	memset(p, 0xff, sz);						\
	free(p);							\
}
#else
#define	FREE(p, sz)	free(p);
#endif

/* Filter type. */
enum filtertype { STANDARD, NOINPUT, NOOUTPUT };
int	filtercmd __P((SCR *, EXF *, MARK *,
	    MARK *, MARK *, char *, enum filtertype));

/* Portability stuff. */
#ifndef DEFFILEMODE			/* Default file permissions. */
#define	DEFFILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#endif

typedef void (*sig_ret_t) __P((int));	/* Type of signal function. */

#ifndef MIN
#define	MIN(a, b) (((a) < (b)) ? (a) : (b))
#define	MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/* Function prototypes that don't seem to belong anywhere else. */
char	*charname __P((SCR *, int));
int	 nonblank __P((SCR *, EXF *, recno_t, size_t *));
int	 set_window_size __P((SCR *, u_int));
int	 status __P((SCR *, EXF *, recno_t, int));
char	*tail __P((char *));

#ifdef DEBUG
void	TRACE __P((SCR *, const char *, ...));
#endif

/* Digraphs (not currently real). */
int	digraph __P((SCR *, int, int));
int	digraph_init __P((SCR *));
void	digraph_save __P((SCR *, int));
