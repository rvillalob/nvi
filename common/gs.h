/*-
 * Copyright (c) 1993 The Regents of the University of California.
 * All rights reserved.
 *
 * %sccs.include.redist.c%
 *
 *	$Id: gs.h,v 5.1 1993/03/26 13:41:26 bostic Exp $ (Berkeley) $Date: 1993/03/26 13:41:26 $
 */

#include <termios.h>

struct _scr;
typedef struct _gs {
	struct _hdr	 exfhdr;	/* Linked list of EXF structures. */
	struct _hdr	 scrhdr;	/* Linked list of SCR structures. */

	struct _scr	*snext;		/* Next screen to display. */
	struct _scr	*sprev;		/* Last screen displayed. */

	mode_t	 origmode;		/* Original terminal mode. */
	struct termios
		 original_termios;	/* Original terminal values. */

	char	*tmp_bp;		/* Temporary buffer. */
	size_t	 tmp_blen;		/* Size of temporary buffer. */

#ifdef DEBUG
	FILE	*tracefp;		/* Trace file pointer. */
#endif

#define	G_SETMODE	0x01		/* Tty mode changed. */
#define	G_TMP_INUSE	0x02		/* Temporary buffer in use. */
	u_int	 flags;
} GS;
