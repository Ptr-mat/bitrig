/*	$OpenBSD: constants.c,v 1.5 1999/02/26 03:35:06 niklas Exp $	*/
/*	$EOM: constants.c,v 1.6 1999/02/25 11:38:48 niklas Exp $	*/

/*
 * Copyright (c) 1998 Niklas Hallqvist.  All rights reserved.
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
 *	This product includes software developed by Ericsson Radio Systems.
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

/*
 * This code was written under funding by Ericsson Radio Systems.
 */

#include <stdio.h>
#include <string.h>

#include "sysdep.h"

#include "constants.h"

int
constant_value (struct constant_map *map, char *name)
{
  struct constant_map *entry = map;

  for (entry = map; entry->name; entry++)
    if (strcasecmp (entry->name, name) == 0)
      return entry->value;
  return 0;
}

char *
constant_lookup (struct constant_map *map, int value)
{
  struct constant_map *entry = map;

  for (entry = map; entry->name; entry++)
    if (entry->value == value)
      return entry->name;
  return 0;
}

struct constant_map *
constant_link_lookup (struct constant_map *map, int value)
{
  struct constant_map *entry = map;

  for (entry = map; entry->name; entry++)
    if (entry->value == value)
      return entry->link;
  return 0;
}

char *
constant_name (struct constant_map *map, int value)
{
  static char tmp[32];		/* XXX Ugly, I know.  */
  char *retval = constant_lookup (map, value);

  if (!retval)
    {
      snprintf (tmp, 32, "<Unknown %d>", value);
      return tmp;
    }
  return retval;
}

char *
constant_name_maps (struct constant_map **maps, int value)
{
  static char tmp[32];		/* XXX Ugly, I know.  */
  char *retval;
  struct constant_map **map;

  for (map = maps; *map; map++)
    {
      retval = constant_lookup (*map, value);
      if (retval)
	return retval;
    }
  snprintf (tmp, 32, "<Unknown %d>", value);
  return tmp;
}
