/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: screen.c,v 8.48 1993/12/10 12:20:44 bostic Exp $ (Berkeley) $Date: 1993/12/10 12:20:44 $";
#endif /* not lint */

#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "vi.h"
#include "vcmd.h"
#include "excmd.h"
#include "tag.h"

/*
 * screen_init --
 *	Do the default initialization of an SCR structure.
 */
int
screen_init(orig, spp, flags)
	SCR *orig, **spp;
	u_int flags;
{
	SCR *sp;
	size_t len;

	CALLOC_RET(orig, sp, SCR *, 1, sizeof(SCR));

/* INITIALIZED AT SCREEN CREATE. */
	sp->gp = __global_list;			/* All ref the GS structure. */

	LIST_INIT(&sp->msgq);
	CIRCLEQ_INIT(&sp->frefq);

	sp->ccnt = 2;				/* Anything > 1 */

	FD_ZERO(&sp->rdfd);

	CIRCLEQ_INIT(&sp->tiq);

/* PARTIALLY OR COMPLETELY COPIED FROM PREVIOUS SCREEN. */
	if (orig == NULL) {
		sp->searchdir = NOTSET;
		sp->csearchdir = CNOTSET;

		switch (flags & S_SCREENS) {
		case S_EX:
			if (sex_screen_init(sp))
				return (1);
			break;
		case S_VI_CURSES:
			if (svi_screen_init(sp))
				return (1);
			break;
		case S_VI_XAW:
			if (xaw_screen_init(sp))
				return (1);
			break;
		default:
			abort();
		}

		sp->flags = flags;
	} else {
		if (orig->alt_name != NULL &&
		    (sp->alt_name = strdup(orig->alt_name)) == NULL)
			goto mem;

		/* Retain all searching/substitution information. */
		if (F_ISSET(orig, S_SRE_SET)) {
			F_SET(sp, S_SRE_SET);
			sp->sre = orig->sre;
		}
		if (F_ISSET(orig, S_SUBRE_SET)) {
			F_SET(sp, S_SUBRE_SET);
			sp->subre = orig->subre;
		}
		sp->searchdir = orig->searchdir == NOTSET ? NOTSET : FORWARD;
		sp->csearchdir = CNOTSET;
		sp->lastckey = orig->lastckey;

		if (orig->matchsize) {
			len = orig->matchsize * sizeof(regmatch_t);
			MALLOC(sp, sp->match, regmatch_t *, len);
			if (sp->match == NULL)
				goto mem;
			sp->matchsize = orig->matchsize;
			memmove(sp->match, orig->match, len);
		}
		if (orig->repl_len) {
			MALLOC(sp, sp->repl, char *, orig->repl_len);
			if (sp->repl == NULL)
				goto mem;
			sp->repl_len = orig->repl_len;
			memmove(sp->repl, orig->repl, orig->repl_len);
		}
		if (orig->newl_len) {
			len = orig->newl_len * sizeof(size_t);
			MALLOC(sp, sp->newl, size_t *, len);
			if (sp->newl == NULL)
				goto mem;
			sp->newl_len = orig->newl_len;
			sp->newl_cnt = orig->newl_cnt;
			memmove(sp->newl, orig->newl, len);
		}

		sp->saved_vi_mode = orig->saved_vi_mode;

		if (opts_copy(orig, sp)) {
mem:			msgq(orig, M_SYSERR, "new screen attributes");
			(void)screen_end(sp);
			return (1);
		}

		sp->s_bell		= orig->s_bell;
		sp->s_bg		= orig->s_bg;
		sp->s_busy		= orig->s_busy;
		sp->s_change		= orig->s_change;
		sp->s_chposition	= orig->s_chposition;
		sp->s_clear		= orig->s_clear;
		sp->s_column		= orig->s_column;
		sp->s_confirm		= orig->s_confirm;
		sp->s_copy		= orig->s_copy;
		sp->s_down		= orig->s_down;
		sp->s_edit		= orig->s_edit;
		sp->s_end		= orig->s_end;
		sp->s_ex_cmd		= orig->s_ex_cmd;
		sp->s_ex_run		= orig->s_ex_run;
		sp->s_ex_write		= orig->s_ex_write;
		sp->s_fg		= orig->s_fg;
		sp->s_fill		= orig->s_fill;
		sp->s_get		= orig->s_get;
		sp->s_key_read		= orig->s_key_read;
		sp->s_optchange		= orig->s_optchange;
		sp->s_position		= orig->s_position;
		sp->s_rabs		= orig->s_rabs;
		sp->s_refresh		= orig->s_refresh;
		sp->s_relative		= orig->s_relative;
		sp->s_rrel		= orig->s_rrel;
		sp->s_split		= orig->s_split;
		sp->s_suspend		= orig->s_suspend;
		sp->s_up		= orig->s_up;

		F_SET(sp, F_ISSET(orig, S_SCREENS));
	}

	/* Copy screen private information. */
	if (sp->s_copy(orig, sp))
		return (1);

	*spp = sp;
	return (0);
}

/*
 * screen_end --
 *	Release a screen.
 */
int
screen_end(sp)
	SCR *sp;
{
	/* Cleanup screen private information. */
	(void)sp->s_end(sp);

	/* Free FREF's. */
	{ FREF *frp;
		while ((frp = sp->frefq.cqh_first) != (FREF *)&sp->frefq) {
			CIRCLEQ_REMOVE(&sp->frefq, frp, q);
			if (frp->cname != NULL)
				free(frp->cname);
			if (frp->name != NULL)
				free(frp->name);
			if (frp->tname != NULL)
				free(frp->tname);
			FREE(frp, sizeof(FREF));
		}
	}

	/* Free any text input. */
	text_lfree(&sp->tiq);

	/* Free any script information. */
	if (F_ISSET(sp, S_SCRIPT))
		sscr_end(sp);

	/* Free alternate file name. */
	if (sp->alt_name != NULL)
		FREE(sp->alt_name, strlen(sp->alt_name) + 1);

	/* Free up search information. */
	if (sp->match != NULL)
		FREE(sp->match, sizeof(regmatch_t));
	if (sp->repl != NULL)
		FREE(sp->repl, sp->repl_len);
	if (sp->newl != NULL)
		FREE(sp->newl, sp->newl_len);

	/* Free all the options */
	opts_free(sp);

	/*
	 * Free the message chain last, so previous failures have a place
	 * to put messages.  Copy messages to (in order) a related screen,
	 * any screen, the global area. 
	 */
	{ SCR *c_sp; MSG *mp, *next;
		if ((c_sp = sp->q.cqe_prev) != (void *)&sp->gp->dq) {
			if (F_ISSET(sp, S_BELLSCHED))
				F_SET(c_sp, S_BELLSCHED);
		} else if ((c_sp = sp->q.cqe_next) != (void *)&sp->gp->dq) {
			if (F_ISSET(sp, S_BELLSCHED))
				F_SET(c_sp, S_BELLSCHED);
		} else if ((c_sp =
		    sp->gp->hq.cqh_first) != (void *)&sp->gp->hq) {
			if (F_ISSET(sp, S_BELLSCHED))
				F_SET(c_sp, S_BELLSCHED);
		} else {
			c_sp = NULL;
			if (F_ISSET(sp, S_BELLSCHED))
				F_SET(sp->gp, G_BELLSCHED);
		}

		for (mp = sp->msgq.lh_first; mp != NULL; mp = next) {
			if (!F_ISSET(mp, M_EMPTY))
				msg_app(sp->gp, c_sp,
				    mp->flags & M_INV_VIDEO, mp->mbuf, mp->len);
			next = mp->q.le_next;
			if (mp->mbuf != NULL)
				FREE(mp->mbuf, mp->blen);
			FREE(mp, sizeof(MSG));
		}
	}

	/* Remove the screen from the displayed queue. */
	CIRCLEQ_REMOVE(&sp->gp->dq, sp, q);

	/* Free the screen itself. */
	FREE(sp, sizeof(SCR));

	return (0);
}
