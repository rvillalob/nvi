/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * %sccs.include.redist.c%
 */

#ifndef lint
static char sccsid[] = "$Id: ex_version.c,v 9.4 1994/11/14 10:47:19 bostic Exp $ (Berkeley) $Date: 1994/11/14 10:47:19 $";
#endif /* not lint */

#include <sys/types.h>
#include <sys/queue.h>
#include <sys/time.h>

#include <bitstring.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <termios.h>

#include "compat.h"
#include <db.h>
#include <regex.h>

#include "vi.h"
#include "excmd.h"

/*
 * ex_version -- :version
 *	Display the program version.
 */
int
ex_version(sp, cmdp)
	SCR *sp;
	EXCMDARG *cmdp;
{
	static const time_t then = 784828032;

	(void)ex_printf(EXCOOKIE,
"Version 1.40, %sThe CSRG, University of California, Berkeley.\n",
	    ctime(&then));
	F_SET(sp, S_SCR_EXWROTE);
	return (0);
}
