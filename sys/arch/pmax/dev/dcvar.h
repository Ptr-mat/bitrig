/*	$NetBSD: dcvar.h,v 1.3 1996/09/25 20:48:57 jonathan Exp $	*/

/*
 * External declarations from DECstation dc serial driver.
 */

#ifdef _KERNEL
#ifndef _DCVAR_H
#define _DCVAR_H

#include <pmax/dev/pdma.h>

struct dc_softc {
	struct device sc_dv;
	struct pdma dc_pdma[4];
	struct	tty *dc_tty[4];
	/*
	 * Software copy of brk register since it isn't readable
	 */
	int	dc_brk;

	char	dc_19200;		/* this unit supports 19200 */
	char	dcsoftCAR;		/* mask, lines with carrier on (DSR) */
	char	dc_rtscts;		/* mask, lines with hw flow control */
	char	dc_modem;		/* mask, lines with  DTR wired  */
};

int	dcattach __P((struct dc_softc *sc, void *addr,
			int dtrmask, int rts_ctsmask,
			int speed, int consline));
int	dcintr __P((void * xxxunit));

/*
 * Following declaratios for console code.
 * XXX shuould be separated, or redesigned.
 */
extern int dcGetc __P ((dev_t dev));
extern int dcparam __P((register struct tty *tp, register struct termios *t));
extern void dcPutc __P((dev_t dev, int c));
void dc_consinit __P((dev_t dev, dcregs *dcaddr));

#endif	/* _DCVAR_H */
#endif	/* _KERNEL */
