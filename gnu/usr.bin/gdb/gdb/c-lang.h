/* C language support definitions for GDB, the GNU debugger.
   Copyright 1992 Free Software Foundation, Inc.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	$Id: c-lang.h,v 1.1 1995/10/18 08:39:57 deraadt Exp $
*/

extern int
c_parse PARAMS ((void));	/* Defined in c-exp.y */

extern void
c_error PARAMS ((char *));	/* Defined in c-exp.y */

extern void			/* Defined in c-typeprint.c */
c_print_type PARAMS ((struct type *, char *, FILE *, int, int));

extern int
c_val_print PARAMS ((struct type *, char *, CORE_ADDR, FILE *, int, int,
		     int, enum val_prettyprint));
