/* Language independent support for printing types for GDB, the GNU debugger.
   Copyright 1986, 1988, 1989, 1991, 1992, 1993 Free Software Foundation, Inc.

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

	$Id: typeprint.c,v 1.1 1995/10/18 08:40:05 deraadt Exp $
*/

#include "defs.h"
#include "obstack.h"
#include "bfd.h"		/* Binary File Description */
#include "symtab.h"
#include "gdbtypes.h"
#include "expression.h"
#include "value.h"
#include "gdbcore.h"
#include "command.h"
#include "gdbcmd.h"
#include "target.h"
#include "language.h"
#include "demangle.h"

#include <string.h>
#include <errno.h>

static void
ptype_command PARAMS ((char *, int));

static struct type *
ptype_eval PARAMS ((struct expression *));

static void
whatis_command PARAMS ((char *, int));

static void
whatis_exp PARAMS ((char *, int));

/* Print a description of a type TYPE in the form of a declaration of a
   variable named VARSTRING.  (VARSTRING is demangled if necessary.)
   Output goes to STREAM (via stdio).
   If SHOW is positive, we show the contents of the outermost level
   of structure even if there is a type name that could be used instead.
   If SHOW is negative, we never show the details of elements' types.  */

void
type_print (type, varstring, stream, show)
     struct type *type;
     char *varstring;
     FILE *stream;
     int show;
{
  LA_PRINT_TYPE (type, varstring, stream, show, 0);
}

/* Print type of EXP, or last thing in value history if EXP == NULL.
   show is passed to type_print.  */

static void
whatis_exp (exp, show)
     char *exp;
     int show;
{
  struct expression *expr;
  register value val;
  register struct cleanup *old_chain = NULL;

  if (exp)
    {
      expr = parse_expression (exp);
      old_chain = make_cleanup (free_current_contents, &expr);
      val = evaluate_type (expr);
    }
  else
    val = access_value_history (0);

  printf_filtered ("type = ");
  type_print (VALUE_TYPE (val), "", stdout, show);
  printf_filtered ("\n");

  if (exp)
    do_cleanups (old_chain);
}

/* ARGSUSED */
static void
whatis_command (exp, from_tty)
     char *exp;
     int from_tty;
{
  /* Most of the time users do not want to see all the fields
     in a structure.  If they do they can use the "ptype" command.
     Hence the "-1" below.  */
  whatis_exp (exp, -1);
}

/* Simple subroutine for ptype_command.  */

static struct type *
ptype_eval (exp)
     struct expression *exp;
{
  if (exp->elts[0].opcode == OP_TYPE)
    {
      return (exp->elts[1].type);
    }
  else
    {
      return (NULL);
    }
}

/* TYPENAME is either the name of a type, or an expression.  */

/* ARGSUSED */
static void
ptype_command (typename, from_tty)
     char *typename;
     int from_tty;
{
  register struct type *type;
  struct expression *expr;
  register struct cleanup *old_chain;

  if (typename == NULL)
    {
      /* Print type of last thing in value history. */
      whatis_exp (typename, 1);
    }
  else
    {
      expr = parse_expression (typename);
      old_chain = make_cleanup (free_current_contents, &expr);
      type = ptype_eval (expr);
      if (type != NULL)
	{
	  /* User did "ptype <typename>" */
	  printf_filtered ("type = ");
	  type_print (type, "", stdout, 1);
	  printf_filtered ("\n");
	  do_cleanups (old_chain);
	}
      else
	{
	  /* User did "ptype <symbolname>" */
	  do_cleanups (old_chain);
	  whatis_exp (typename, 1);
	}
    }
}

/* Print integral scalar data VAL, of type TYPE, onto stdio stream STREAM.
   Used to print data from type structures in a specified type.  For example,
   array bounds may be characters or booleans in some languages, and this
   allows the ranges to be printed in their "natural" form rather than as
   decimal integer values.

   FIXME:  This is here simply because only the type printing routines
   currently use it, and it wasn't clear if it really belonged somewhere
   else (like printcmd.c).  There are a lot of other gdb routines that do
   something similar, but they are generally concerned with printing values
   that come from the inferior in target byte order and target size. */

void
print_type_scalar (type, val, stream)
     struct type *type;
     LONGEST val;
     FILE *stream;
{
  unsigned int i;
  unsigned len;

  switch (TYPE_CODE (type))
    {

    case TYPE_CODE_ENUM:
      len = TYPE_NFIELDS (type);
      for (i = 0; i < len; i++)
	{
	  if (TYPE_FIELD_BITPOS (type, i) == val)
	    {
	      break;
	    }
	}
      if (i < len)
	{
	  fputs_filtered (TYPE_FIELD_NAME (type, i), stream);
	}
      else
	{
	  print_longest (stream, 'd', 0, val);
	}
      break;

    case TYPE_CODE_INT:
      print_longest (stream, TYPE_UNSIGNED (type) ? 'u' : 'd', 0, val);
      break;

    case TYPE_CODE_CHAR:
      LA_PRINT_CHAR ((unsigned char) val, stream);
      break;

    case TYPE_CODE_BOOL:
      fprintf_filtered (stream, val ? "TRUE" : "FALSE");
      break;

    case TYPE_CODE_UNDEF:
    case TYPE_CODE_PTR:
    case TYPE_CODE_ARRAY:
    case TYPE_CODE_STRUCT:
    case TYPE_CODE_UNION:
    case TYPE_CODE_FUNC:
    case TYPE_CODE_FLT:
    case TYPE_CODE_VOID:
    case TYPE_CODE_SET:
    case TYPE_CODE_RANGE:
    case TYPE_CODE_STRING:
    case TYPE_CODE_ERROR:
    case TYPE_CODE_MEMBER:
    case TYPE_CODE_METHOD:
    case TYPE_CODE_REF:
      error ("internal error: unhandled type in print_type_scalar");
      break;

    default:
      error ("Invalid type code in symbol table.");
    }
  fflush (stream);
}

#if MAINTENANCE_CMDS

/* Dump details of a type specified either directly or indirectly.
   Uses the same sort of type lookup mechanism as ptype_command()
   and whatis_command(). */

void
maintenance_print_type (typename, from_tty)
     char *typename;
     int from_tty;
{
  register value val;
  register struct type *type;
  register struct cleanup *old_chain;
  struct expression *expr;

  if (typename != NULL)
  {
    expr = parse_expression (typename);
    old_chain = make_cleanup (free_current_contents, &expr);
    if (expr -> elts[0].opcode == OP_TYPE)
      {
	/* The user expression names a type directly, just use that type. */
	type = expr -> elts[1].type;
      }
    else
      {
	/* The user expression may name a type indirectly by naming an
	   object of that type.  Find that indirectly named type. */
	val = evaluate_type (expr);
	type = VALUE_TYPE (val);
      }
    if (type != NULL)
      {
	recursive_dump_type (type, 0);
      }
    do_cleanups (old_chain);
  }
}

#endif	/* MAINTENANCE_CMDS */


void
_initialize_typeprint ()
{

  add_com ("ptype", class_vars, ptype_command,
	   "Print definition of type TYPE.\n\
Argument may be a type name defined by typedef, or \"struct STRUCTNAME\"\n\
or \"union UNIONNAME\" or \"enum ENUMNAME\".\n\
The selected stack frame's lexical context is used to look up the name.");

  add_com ("whatis", class_vars, whatis_command,
	   "Print data type of expression EXP.");

}
