/* GNU Objective C Runtime initialization 
   Copyright (C) 1993 Free Software Foundation, Inc.

Author: Kresten Krab Thorup

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify it under the
   terms of the GNU General Public License as published by the Free Software
   Foundation; either version 2, or (at your option) any later version.

GNU CC is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
   FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
   details.

You should have received a copy of the GNU General Public License along with
   GNU CC; see the file COPYING.  If not, write to the Free Software
   Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef lint
static char rcsid[] = "$Id: init.c,v 1.1 1995/10/18 08:39:49 deraadt Exp $";
#endif /* not lint */

/* As a special exception, if you link this library with files compiled with
   GCC to produce an executable, this does not cause the resulting executable
   to be covered by the GNU General Public License. This exception does not
   however invalidate any other reasons why the executable file might be
   covered by the GNU General Public License.  */

#include "runtime.h"

/* The version number of this runtime.  This must match the number 
   defined in gcc (objc-act.c) */
#define OBJC_VERSION 5
#define PROTOCOL_VERSION 2

/* This list contains all modules currently loaded into the runtime */
static struct objc_list* __objc_module_list = 0;

/* This list contains all proto_list's not yet assigned class links */
static struct objc_list* unclaimed_proto_list = 0;

/* Check compiler vs runtime version */
static void init_check_module_version(Module_t);

/* Assign isa links to protos */
static void __objc_init_protocols (struct objc_protocol_list* protos);

/* Add protocol to class */
static void __objc_class_add_protocols (Class*, struct objc_protocol_list*);

/* Is all categories/classes resolved? */
BOOL __objc_dangling_categories = NO;

/* This function is called by constructor functions generated for each
   module compiled.  (_GLOBAL_$I$...) The purpose of this function is to
   gather the module pointers so that they may be processed by the
   initialization routines as soon as possible */

void
__objc_exec_class (Module_t module)
{
  /* Has we processed any constructors previously?  This flag used to 
     indicate that some global data structures need to be built.  */ 
  static BOOL previous_constructors = 0;

  static struct objc_list* unclaimed_categories = 0;

  /* The symbol table (defined in objc.h) generated by gcc */
  Symtab_t symtab = module->symtab;

  /* Entry used to traverse hash lists */
  struct objc_list** cell;

  /* The table of selector references for this module */
  SEL *selectors = symtab->refs; 

  /* dummy counter */
  int i;

  DEBUG_PRINTF ("received module: %s\n", module->name);
  /* check gcc version */
  init_check_module_version(module);

  /* On the first call of this routine, initialize some data structures.  */
  if (!previous_constructors)
    {
      __objc_init_selector_tables();
      __objc_init_class_tables();
      __objc_init_dispatch_tables();
      previous_constructors = 1;
    }

  /* Save the module pointer for later processing. (not currently used) */
  __objc_module_list = list_cons(module, __objc_module_list);

  /* Parse the classes in the load module and gather selector information.  */
  DEBUG_PRINTF ("gathering selectors from module: %s\n", module->name);
  for (i = 0; i < symtab->cls_def_cnt; ++i)
    {
      Class* class = (Class*) symtab->defs[i];

      /* Make sure we have what we think.  */
      assert (CLS_ISCLASS(class));
      assert (CLS_ISMETA(class->class_pointer));
      DEBUG_PRINTF ("phase 1, processing class: %s\n", class->name);

      /* Store the class in the class table and assign class numbers.  */
      __objc_add_class_to_hash (class);

      /* Register all of the selectors in the class and meta class.  */
      __objc_register_selectors_from_class (class);
      __objc_register_selectors_from_class ((Class*) class->class_pointer);

      /* Install the fake dispatch tables */
      __objc_install_premature_dtable(class);
      __objc_install_premature_dtable(class->class_pointer);

      if (class->protocols)
	__objc_init_protocols (class->protocols);
   }

  /* Replace referenced selectors from names to SEL's.  */
  if (selectors)
    {
      for (i = 0; selectors[i]; ++i)
	selectors[i] = sel_register_name ((const char *) selectors[i]);
    }

  /* Process category information from the module.  */
  for (i = 0; i < symtab->cat_def_cnt; ++i)
    {
      Category_t category = symtab->defs[i + symtab->cls_def_cnt];
      Class* class = objc_lookup_class (category->class_name);
      
      /* If the class for the category exists then append its methods.  */
      if (class)
	{

	  DEBUG_PRINTF ("processing categories from (module,object): %s, %s\n",
			module->name,
			class->name);

	  /* Do instance methods.  */
	  if (category->instance_methods)
	    class_add_method_list (class, category->instance_methods);

	  /* Do class methods.  */
	  if (category->class_methods)
	    class_add_method_list ((Class*) class->class_pointer, 
				   category->class_methods);

	  if (category->protocols)
	    {
	      __objc_init_protocols (category->protocols);
	      __objc_class_add_protocols (class, category->protocols);
	    }

	}
      else
	{
	  /* The object to which the category methods belong can't be found.
	     Save the information.  */
	  unclaimed_categories = list_cons(category, unclaimed_categories);
	}
    }

  /* Scan the unclaimed category hash.  Attempt to attach any unclaimed
     categories to objects.  */
  for (cell = &unclaimed_categories;
       *cell;
       *cell && ((cell = &(*cell)->tail)))
    {
      Category_t category = (*cell)->head;
      Class* class = objc_lookup_class (category->class_name);
      
      if (class)
	{
	  DEBUG_PRINTF ("attaching stored categories to object: %s\n",
			class->name);
	  
	  list_remove_head (cell);
	  
	  if (category->instance_methods)
	    class_add_method_list (class, category->instance_methods);
	  
	  if (category->class_methods)
	    class_add_method_list ((Class*) class->class_pointer,
				   category->class_methods);
	  
	  if (category->protocols)
	    {
	      __objc_init_protocols (category->protocols);
	      __objc_class_add_protocols (class, category->protocols);
	    }
	  
	}
    }
  
  if (unclaimed_proto_list && objc_lookup_class ("Protocol"))
    {
      list_mapcar (unclaimed_proto_list,(void(*)(void*))__objc_init_protocols);
      list_free (unclaimed_proto_list);
      unclaimed_proto_list = 0;
    }

}

/* Sanity check the version of gcc used to compile `module'*/
static void init_check_module_version(Module_t module)
{
  if ((module->version != OBJC_VERSION) || (module->size != sizeof (Module)))
    {
      fprintf (stderr, "Module %s version %d doesn't match runtime %d\n",
	       module->name, module->version, OBJC_VERSION);
      if(module->version > OBJC_VERSION)
	fprintf (stderr, "Runtime (libobjc.a) is out of date\n");
      else if (module->version < OBJC_VERSION)
	fprintf (stderr, "Compiler (gcc) is out of date\n");
      else
	fprintf (stderr, "Objective C internal error -- bad Module size\n");
      abort ();
    }
}

static void
__objc_init_protocols (struct objc_protocol_list* protos)
{
  int i;
  static Class* proto_class = 0;

  if (! protos)
    return;

  if (!proto_class)
    proto_class = objc_lookup_class("Protocol");

  if (!proto_class)
    {
      unclaimed_proto_list = list_cons (protos, unclaimed_proto_list);
      return;
    }

  assert (protos->next == 0);	/* only single ones allowed */

  for(i = 0; i < protos->count; i++)
    {
      struct objc_protocol* aProto = protos->list[i];
      if (((size_t)aProto->class_pointer) == PROTOCOL_VERSION)
	{
	  /* assign class pointer */
	  aProto->class_pointer = proto_class;

	  /* init super protocols */
	  __objc_init_protocols (aProto->protocol_list);
	}
      else if (protos->list[i]->class_pointer != proto_class)
	{
	  fprintf (stderr,
		   "Version %d doesn't match runtime protocol version %d\n",
		   ((size_t)protos->list[i]->class_pointer),
		   PROTOCOL_VERSION);
	  abort ();
	}
    }
}

static void __objc_class_add_protocols (Class* class,
					struct objc_protocol_list* protos)
{
  /* Well... */
  if (! protos)
    return;

  /* Add it... */
  protos->next = class->protocols;
  class->protocols = protos;
}
