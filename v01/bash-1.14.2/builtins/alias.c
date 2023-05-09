/* alias.c, created from alias.def. */
#line 33 "./alias.def"

#include "../config.h"

#if defined (ALIAS)
#  include <stdio.h>
#  include "../shell.h"
#  include "../alias.h"
#  include "common.h"

extern int interactive;
static void print_alias ();

/* Hack the alias command in a Korn shell way. */
alias_builtin (list)
     WORD_LIST *list;
{
  int any_failed = 0;

  if (!list)
    {
      register int i;
      ASSOC **alias_list;

      if (!aliases)
	return (EXECUTION_FAILURE);

      alias_list = all_aliases ();

      if (!alias_list)
	return (EXECUTION_FAILURE);

      for (i = 0; alias_list[i]; i++)
	print_alias (alias_list[i]);

      free (alias_list);	/* XXX - Do not free the strings. */
    }
  else
    {
      while (list)
	{
	  register char *value, *name = list->word->word;
	  register int offset;

	  for (offset = 0; name[offset] && name[offset] != '='; offset++)
	    ;

	  if (offset && name[offset] == '=')
	    {
	      name[offset] = '\0';
	      value = name + offset + 1;

	      add_alias (name, value);
	    }
	  else
	    {
	      ASSOC *t = find_alias (name);
	      if (t)
		print_alias (t);
	      else
		{
		  if (interactive)
		    builtin_error ("`%s' not found", name);
		  any_failed++;
		}
	    }
	  list = list->next;
	}
    }
  if (any_failed)
    return (EXECUTION_FAILURE);
  else
    return (EXECUTION_SUCCESS);
}
#endif /* ALIAS */

#line 115 "./alias.def"

#if defined (ALIAS)
/* Remove aliases named in LIST from the aliases database. */
unalias_builtin (list)
     register WORD_LIST *list;
{
  register ASSOC *alias;
  int any_failed = 0;

  while (list && *list->word->word == '-')
    {
      register char *word = list->word->word;

      if (ISOPTION (word, 'a'))
	{
	  delete_all_aliases ();
	  list = list->next;
	}
      else if (ISOPTION (word, '-'))
	{
	  list = list->next;
	  break;
	}
      else
	{
	  bad_option (word);
	  return (EXECUTION_FAILURE);
	}
    }

  while (list)
    {
      alias = find_alias (list->word->word);

      if (alias)
	remove_alias (alias->name);
      else
	{
	  if (interactive)
	    builtin_error ("`%s' not an alias", list->word->word);

	  any_failed++;
	}

      list = list->next;
    }

  if (any_failed)
    return (EXECUTION_FAILURE);
  else
    return (EXECUTION_SUCCESS);
}

/* Output ALIAS in such a way as to allow it to be read back in. */
static void
print_alias (alias)
     ASSOC *alias;
{
  char *value = single_quote (alias->value);

  printf ("alias %s=%s\n", alias->name, value);
  free (value);

  fflush (stdout);
}
#endif /* ALIAS */