/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_argv.c,v 8.1 1993/06/09 22:23:31 bostic Exp $ (Berkeley) $Date: 1993/06/09 22:23:31 $";
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "vi.h"
#include "excmd.h"

#define	SHELLECHO	"echo "
#define	SHELLOFFSET	(sizeof(SHELLECHO) - 1)

/*
 * buildargv --
 *	Build an argv from a string.
 */
int
buildargv(sp, ep, s, expand, argcp, argvp)
	SCR *sp;
	EXF *ep;
	char *s, ***argvp;
	int expand, *argcp;
{
	size_t blen, tlen;
	int cnt, done, len, off;
	char *ap, *bp, *p, *t;

	GET_SPACE(sp, bp, blen, 512);

	p = bp;
	len = 0;
	if (expand) {
		/* It's going to become a shell command. */
		memmove(p, SHELLECHO, SHELLOFFSET);
		p += SHELLOFFSET;
		len += SHELLOFFSET;
	}

#if DEBUG && 0
	TRACE(sp, "argv: {%s}\n", s);
#endif

	/* Replace file name characters. */
	for (; *s; ++s)
		switch (*s) {
		case '%':
			if (F_ISSET(ep, F_NONAME)) {
				msgq(sp, M_ERR,
				    "No filename to substitute for %%.");
				return (1);
			}
			ADD_SPACE(sp, bp, blen, len + ep->nlen);
			memmove(p, ep->name, ep->nlen);
			p += ep->nlen;
			len += ep->nlen;
			break;
		case '#':
			if (sp->altfname != NULL)
				tlen = strlen(t = sp->altfname);
			else if (sp->eprev != NULL &&
			     !F_ISSET(sp->eprev, F_NONAME)) {
				t = sp->eprev->name;
				tlen = sp->eprev->nlen;
			} else {
				msgq(sp, M_ERR,
				    "No filename to substitute for #.");
				return (1);
			}
			ADD_SPACE(sp, bp, blen, len + tlen);
			memmove(p, t, tlen);
			p += tlen;
			len += tlen;
			break;
		case '\\':
			if (p[1] != '\0')
				++s;
			/* FALLTHROUGH */
		default:
			ADD_SPACE(sp, bp, blen, len + 1);
			*p++ = *s;
			++len;
		}

	/* Nul termination. */
	ADD_SPACE(sp, bp, blen, len + 1);
	*p = '\0';

#if DEBUG && 0
	TRACE(sp, "pre-shell: {%s}\n", bp);
#endif
	/*
	 * Do shell word expansion -- it's very, very hard to figure out
	 * what magic characters the user's shell expects.  If it's not
	 * pure vanilla, don't even try.
	 */
	if (expand) {
		for (p = bp; *p; ++p)
			if (!isalnum(*p) && !isspace(*p) && *p != '/')
				break;
		if (*p) {
			if (ex_run_process(sp, bp, &len, bp, blen))
				return (1);
			p = bp;
		} else
			p = bp + SHELLOFFSET;
	} else
		p = bp;

#if DEBUG && 0
	TRACE(sp, "post-shell: {%s}\n", bp);
#endif

	/* Break into argv vector. */
	for (done = off = 0;; ) {
		/*
		 * New argument; NULL terminate, skipping anything that's
		 * preceded by a quoting character.
		 */
		for (ap = p; p[0]; ++p) {
			if (p[0] == '\\' && p[1])
				p += 2;
			if (isspace(p[0]))
				break;
		}
		if (*p)
			*p = '\0';
		else
			done = 1;

		/*
		 * Allocate more pointer space if necessary; leave a space
		 * for a trailing NULL.
		 */
		len = (p - ap) + 1;
#define	INCREMENT	20
		if (off + 2 >= sp->argscnt - 1) {
			sp->argscnt += cnt = MAX(INCREMENT, 2);
			if ((sp->args = realloc(sp->args,
			    sp->argscnt * sizeof(ARGS))) == NULL) {
				free(sp->argv);
				goto mem1;
			}
			if ((sp->argv = realloc(sp->argv,
			    sp->argscnt * sizeof(char *))) == NULL) {
				free(sp->args);
mem1:				sp->argscnt = 0;
				sp->args = NULL;
				sp->argv = NULL;
				msgq(sp, M_ERR,
				    "Error: %s.", strerror(errno));
				return (1);
			}
			memset(&sp->args[off], 0, cnt * sizeof(ARGS));
		}

		/*
		 * Copy the argument(s) into place, allocating space if
		 * necessary.
		 */
		if (sp->args[off].len < len && (sp->args[off].bp =
		    realloc(sp->args[off].bp, len)) == NULL) {
			sp->args[off].bp = NULL;
			sp->args[off].len = 0;
			msgq(sp, M_ERR, "Error: %s.", strerror(errno));
			FREE_SPACE(sp, bp, blen);
			return (1);
		}
		sp->argv[off] = sp->args[off].bp;
		sp->args[off].flags |= A_ALLOCATED;
		/* Copy the argument into place, losing quote chars. */
		for (t = sp->args[off].bp; len; *t++ = *ap++, --len)
			if (*ap == '\\' && len) {
				++ap;
				--len;
			}
		++off;

		if (done)
			break;

		/* Skip whitespace. */
		while (isspace(*++p));
		if (!*p)
			break;
	}
	sp->argv[off] = NULL;
	*argvp = sp->argv;
	*argcp = off;

	FREE_SPACE(sp, bp, blen);

#if DEBUG && 0
	for (cnt = 0; cnt < off; ++cnt)
		TRACE(sp, "arg %d: {%s}\n", cnt, sp->argv[cnt]);
#endif
	return (0);
}
