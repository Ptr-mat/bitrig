/*	$NetBSD: nameglue.h,v 1.1 1995/08/10 05:29:11 jonathan Exp $	*/

/*
 * Use macros to glue names for "machine-independent" functions 
 *  (e.g., from new-config code modelled after DEC Alpha drivers)
 * to old-style pmax names.
 *
 */

/*
 * What is magic_read ?
 */
#ifdef  pmax
#ifdef MAGIC_READ
#undef MAGIC_READ
#endif
#define MAGIC_READ \
		 MachEmptyWriteBuffer()	/*XXX*/ /*FIXME*/

/* 
 * memory barrier
 */
#define MB() \
	 MachEmptyWriteBuffer()	/*XXX*/ /*FIXME*/
/*
 * Map physical addresses to kernel-virtual addresses.
 */
#define KV(x) ((void *)MACH_PHYS_TO_UNCACHED(x))

/*
 * Print debugging messages only if DEBUG defined on a pmax.
 */

#ifdef DEBUG
#define DPRINTF(x) do { printf x ;} while (0)	/* general debug */
#else
#define DPRINTF(x) /* nothing */		/* general debug */
#endif
#endif /* pmax */
