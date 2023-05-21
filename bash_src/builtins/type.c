/* type.c, created from type.def. */
#line 23 "./type.def"

#line 42 "./type.def"

#include <stdio.h>
#include <sys/types.h>
#include "posixstat.h"
#include "../shell.h"
#include "../execute_cmd.h"

#if defined (ALIAS)
#include "../alias.h"
#endif /* ALIAS */

#include "common.h"

extern STRING_INT_ALIST word_token_alist[];

/* For each word in LIST, find out what the shell is going to do with
   it as a simple command. i.e., which file would this shell use to
   execve, or if it is a builtin command, or an alias.  Possible flag
   arguments:
	-type		Returns the "type" of the object, one of
			`alias', `keyword', `function', `builtin',
			or `file'.

	-path		Returns the pathname of the file if -type is
			a file.

	-all		Returns all occurrences of words, whether they
			be a filename in the path, alias, function,
			or builtin.
   Order of evaluation:
	alias
	keyword
	function
	builtin
	file
 */
type_builtin (list)
     WORD_LIST *list;
{
  int path_only, type_only, all, verbose;
  int successful_finds;

  path_only = type_only = all = 0;
  successful_finds = 0;

  if (!list)
    return (EXECUTION_SUCCESS);

  while (list && *(list->word->word) == '-')
    {
      char *flag = &(list->word->word[1]);

      if (flag[0] == 't' && (!flag[1] || strcmp (flag + 1, "ype") == 0))
	{
	  type_only = 1;
	  path_only = 0;
	}
      else if (flag[0] == 'p' && (!flag[1] || strcmp (flag + 1, "ath") == 0))
	{
	  path_only = 1;
	  type_only = 0;
	}
      else if (flag[0] == 'a' && (!flag[1] || strcmp (flag + 1, "ll") == 0))
	{
	  all = 1;
	}
      else
	{
	  bad_option (flag);
	  builtin_error ("usage: type [-all | -path | -type ] name [name ...]");
	  return (EX_USAGE);
	}
      list = list->next;
    }

  if (type_only)
    verbose = 1;
  else if (!path_only)
    verbose = 2;
  else if (path_only)
    verbose = 3;
  else
    verbose = 0;

  while (list)
    {
      int found;

      found = describe_command (list->word->word, verbose, all);

      if (!found && !path_only && !type_only)
	builtin_error ("%s: not found", list->word->word);

      successful_finds += found;
      list = list->next;
    }

  fflush (stdout);

  if (successful_finds != 0)
    return (EXECUTION_SUCCESS);
  else
    return (EXECUTION_FAILURE);
}

/*
 * Describe COMMAND as required by the type builtin.
 *
 * If VERBOSE == 0, don't print anything
 * If VERBOSE == 1, print short description as for `type -t'
 * If VERBOSE == 2, print long description as for `type' and `command -V'
 * If VERBOSE == 3, print path name only for disk files
 * If VERBOSE == 4, print string used to invoke COMMAND, for `command -v'
 *
 * ALL says whether or not to look for all occurrences of COMMAND, or
 * return after finding it once.
 */
describe_command (command, verbose, all)
     char *command;
     int verbose, all;
{
  int found = 0, i, found_file = 0;
  char *full_path = (char *)NULL;
  SHELL_VAR *func;

#if defined (ALIAS)
  /* Command is an alias? */
  ASSOC *alias = find_alias (command);

  if (alias)
    {
      if (verbose == 1)
	printf ("alias\n");
      else if (verbose == 2)
	printf ("%s is aliased to `%s'\n", command, alias->value);
      else if (verbose == 4)
	{
	  char *x = single_quote (alias->value);
	  printf ("alias %s=%s\n", command, x);
	  free (x);
	}

      found = 1;

      if (!all)
	return (1);
    }
#endif /* ALIAS */

  /* Command is a shell reserved word? */
  i = find_reserved_word (command);
  if (i >= 0)
    {
      if (verbose == 1)
	printf ("keyword\n");
      else if (verbose == 2)
	printf ("%s is a shell keyword\n", command);
      else if (verbose == 4)
	printf ("%s\n", command);

      found = 1;

      if (!all)
	return (1);
    }

  /* Command is a function? */
  func = find_function (command);

  if (func)
    {
      if (verbose == 1)
	printf ("function\n");
      else if (verbose == 2)
	{
#define PRETTY_PRINT_FUNC 1
	  char *result;

	  printf ("%s is a function\n", command);

	  /* We're blowing away THE_PRINTED_COMMAND here... */

	  result = named_function_string (command,
					  (COMMAND *) function_cell (func),
					  PRETTY_PRINT_FUNC);
	  printf ("%s\n", result);
#undef PRETTY_PRINT_FUNC
	}
      else if (verbose == 4)
	printf ("%s\n", command);

      found = 1;

      if (!all)
	return (1);
    }

  /* Command is a builtin? */
  if (find_shell_builtin (command))
    {
      if (verbose == 1)
	printf ("builtin\n");
      else if (verbose == 2)
	printf ("%s is a shell builtin\n", command);
      else if (verbose == 4)
	printf ("%s\n", command);

      found = 1;

      if (!all)
	return (1);
    }

  /* Command is a disk file? */
  /* If the command name given is already an absolute command, just
     check to see if it is executable. */
  if (absolute_program (command))
    {
      int f = file_status (command);
      if (f & FS_EXECABLE)
        {
	  if (verbose == 1)
	    printf ("file\n");
	  else if (verbose == 2)
	    printf ("%s is %s\n", command, command);
	  else if (verbose == 3 || verbose == 4)
	    printf ("%s\n", command);

	  /* There's no use looking in the hash table or in $PATH,
	     because they're not consulted when an absolute program
	     name is supplied. */
	  return (1);
        }
    }

  /* If the user isn't doing "-all", then we might care about
     whether the file is present in our hash table. */
  if (!all)
    {
      if ((full_path = find_hashed_filename (command)) != (char *)NULL)
	{
	  if (verbose == 1)
	    printf ("file\n");
	  else if (verbose == 2)
	    printf ("%s is hashed (%s)\n", command, full_path);
	  else if (verbose == 3 || verbose == 4)
	    printf ("%s\n", full_path);

	  return (1);
	}
    }

  /* Now search through $PATH. */
  while (1)
    {
      if (!all)
	full_path = find_user_command (command);
      else
	full_path =
	  user_command_matches (command, FS_EXEC_ONLY, found_file);
	  /* XXX - should that be FS_EXEC_PREFERRED? */

      if (!full_path)
	break;

      found_file++;
      found = 1;

      if (verbose == 1)
	printf ("file\n");
      else if (verbose == 2)
	printf ("%s is %s\n", command, full_path);
      else if (verbose == 3 || verbose == 4)
	printf ("%s\n", full_path);

      free (full_path);
      full_path = (char *)NULL;

      if (!all)
	break;
    }

  return (found);
}
