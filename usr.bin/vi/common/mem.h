/*	$OpenBSD: mem.h,v 1.6 2014/10/14 22:23:12 deraadt Exp $	*/

/*-
 * Copyright (c) 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (c) 1993, 1994, 1995, 1996
 *	Keith Bostic.  All rights reserved.
 *
 * See the LICENSE file for redistribution information.
 *
 *	@(#)mem.h	10.7 (Berkeley) 3/30/96
 */

/* Increase the size of a malloc'd buffer.  Two versions, one that
 * returns, one that jumps to an error label.
 */
#define	BINC_GOTO(sp, lp, llen, nlen) do {				\
	void *L__bincp;							\
	if ((nlen) > (llen)) {						\
		if ((L__bincp = binc((sp), (lp), &(llen), (nlen)))	\
		    == NULL)						\
			goto alloc_err;					\
		/*							\
		 * !!!							\
		 * Possible pointer conversion.				\
		 */							\
		(lp) = L__bincp;					\
	}								\
} while (0)
#define	BINC_RET(sp, lp, llen, nlen) do {				\
	void *L__bincp;							\
	if ((nlen) > (llen)) {						\
		if ((L__bincp = binc((sp), (lp), &(llen), (nlen)))	\
		    == NULL)						\
			return (1);					\
		/*							\
		 * !!!							\
		 * Possible pointer conversion.				\
		 */							\
		(lp) = L__bincp;					\
	}								\
} while (0)

/*
 * Get some temporary space, preferably from the global temporary buffer,
 * from a malloc'd buffer otherwise.  Two versions, one that returns, one
 * that jumps to an error label.
 */
#define	GET_SPACE_GOTO(sp, bp, blen, nlen) do {				\
	GS *L__gp = (sp) == NULL ? NULL : (sp)->gp;			\
	if (L__gp == NULL || F_ISSET(L__gp, G_TMP_INUSE)) {		\
		(bp) = NULL;						\
		(blen) = 0;						\
		BINC_GOTO((sp), (bp), (blen), (nlen)); 			\
	} else {							\
		BINC_GOTO((sp), L__gp->tmp_bp, L__gp->tmp_blen, (nlen));\
		(bp) = L__gp->tmp_bp;					\
		(blen) = L__gp->tmp_blen;				\
		F_SET(L__gp, G_TMP_INUSE);				\
	}								\
} while (0)
#define	GET_SPACE_RET(sp, bp, blen, nlen) do {				\
	GS *L__gp = (sp) == NULL ? NULL : (sp)->gp;			\
	if (L__gp == NULL || F_ISSET(L__gp, G_TMP_INUSE)) {		\
		(bp) = NULL;						\
		(blen) = 0;						\
		BINC_RET((sp), (bp), (blen), (nlen));			\
	} else {							\
		BINC_RET((sp), L__gp->tmp_bp, L__gp->tmp_blen, (nlen));	\
		(bp) = L__gp->tmp_bp;					\
		(blen) = L__gp->tmp_blen;				\
		F_SET(L__gp, G_TMP_INUSE);				\
	}								\
} while (0)

/*
 * Add space to a GET_SPACE returned buffer.  Two versions, one that
 * returns, one that jumps to an error label.
 */
#define	ADD_SPACE_GOTO(sp, bp, blen, nlen) do {				\
	GS *L__gp = (sp) == NULL ? NULL : (sp)->gp;			\
	if (L__gp == NULL || (bp) == L__gp->tmp_bp) {			\
		F_CLR(L__gp, G_TMP_INUSE);				\
		BINC_GOTO((sp), L__gp->tmp_bp, L__gp->tmp_blen, (nlen));\
		(bp) = L__gp->tmp_bp;					\
		(blen) = L__gp->tmp_blen;				\
		F_SET(L__gp, G_TMP_INUSE);				\
	} else								\
		BINC_GOTO((sp), (bp), (blen), (nlen));			\
} while (0)
#define	ADD_SPACE_RET(sp, bp, blen, nlen) do {				\
	GS *L__gp = (sp) == NULL ? NULL : (sp)->gp;			\
	if (L__gp == NULL || (bp) == L__gp->tmp_bp) {			\
		F_CLR(L__gp, G_TMP_INUSE);				\
		BINC_RET((sp), L__gp->tmp_bp, L__gp->tmp_blen, (nlen));	\
		(bp) = L__gp->tmp_bp;					\
		(blen) = L__gp->tmp_blen;				\
		F_SET(L__gp, G_TMP_INUSE);				\
	} else								\
		BINC_RET((sp), (bp), (blen), (nlen));			\
} while (0)

/* Free a GET_SPACE returned buffer. */
#define	FREE_SPACE(sp, bp, blen) do {					\
	GS *L__gp = (sp) == NULL ? NULL : (sp)->gp;			\
	if (L__gp != NULL && (bp) == L__gp->tmp_bp)			\
		F_CLR(L__gp, G_TMP_INUSE);				\
	else								\
		free(bp);						\
} while (0)

/*
 * Malloc a buffer.  Various versions.
 */
#define	CALLOC(sp, p, nmemb, size) do {					\
	if (((p) = calloc((nmemb), (size))) == NULL)			\
		msgq((sp), M_SYSERR, NULL);				\
} while (0)
#define	CALLOC_GOTO(sp, p, nmemb, size) do {				\
	if (((p) = calloc((nmemb), (size))) == NULL)			\
		goto alloc_err;						\
} while (0)
#define	CALLOC_RET(sp, p, nmemb, size) do {				\
	if (((p) = calloc((nmemb), (size))) == NULL) {			\
		msgq((sp), M_SYSERR, NULL);				\
		return (1);						\
	}								\
} while (0)

#define	MALLOC(sp, p, size) do {					\
	if (((p) = malloc(size)) == NULL)				\
		msgq((sp), M_SYSERR, NULL);				\
} while (0)
#define	MALLOC_GOTO(sp, p, size) do {					\
	if (((p) = malloc(size)) == NULL)				\
		goto alloc_err;						\
} while (0)
#define	MALLOC_RET(sp, p, size) do {					\
	if (((p) = malloc(size)) == NULL) {				\
		msgq((sp), M_SYSERR, NULL);				\
		return (1);						\
	}								\
} while (0)

#define	REALLOC(sp, p, size) do {					\
	if (((p) = (realloc((p), (size)))) == NULL)			\
		msgq((sp), M_SYSERR, NULL);				\
} while (0)

#define	REALLOCARRAY(sp, p, nelem, size) do {				\
	if (((p) = (reallocarray((p), (nelem), (size)))) == NULL)	\
		msgq((sp), M_SYSERR, NULL);				\
} while (0)
