/*	$OpenBSD: endian.h,v 1.12 1999/05/10 16:02:09 espie Exp $	*/

/*-
 * Copyright (c) 1997 Niklas Hallqvist.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Niklas Hallqvist.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _M68K_ENDIAN_H_
#define _M68K_ENDIAN_H_

#ifdef __GNUC__

#define	__swap32md(x) ({						\
	u_int32_t __swap32md_x = (x);					\
									\
	__asm ("rorw #8, %0; swap %0; rorw #8, %0" :			\
	    "=d" (__swap32md_x) : "0" (__swap32md_x));			\
	__swap32md_x;							\
})

#define	__swap16md(x) ({						\
	u_int16_t __swap16md_x = (x);					\
									\
	__asm ("rorw #8, %0" : "=d" (__swap16md_x) :			\
	    "0" (__swap16md_x));					\
	__swap16md_x;							\
})

/* Tell sys/endian.h we have MD variants of the swap macros.  */
#define MD_SWAP

#endif /* __GNUC__ */

#define BYTE_ORDER BIG_ENDIAN
#include <sys/endian.h>

#endif /* _M68K_ENDIAN_H_ */
