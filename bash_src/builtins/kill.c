/* kill.c, created from kill.def. */


/* Not all systems declare ERRNO in errno.h... and some systems #define it! */
#if !defined (errno)
extern int errno;
#endif /* !errno */

#include "../shell.h"
#include "../trap.h"
#include "../jobs.h"
#include "common.h"
#include <errno.h>
#include "dsignal.h"

#if defined (JOB_CONTROL)
extern int interactive;
extern int posixly_correct;

#if !defined (CONTINUE_AFTER_KILL_ERROR)
#  define CONTINUE_OR_FAIL return (EXECUTION_FAILURE)
#else
#  define CONTINUE_OR_FAIL goto continue_killing
#endif /* CONTINUE_AFTER_KILL_ERROR */

/* Here is the kill builtin.  We only have it so that people can type
   kill -KILL %1?  No, if you fill up the process table this way you
   can still kill some. */
int
kill_builtin (list)
     WORD_LIST *list;
{
  int signal = SIGTERM;
  int any_succeeded = 0, listing = 0, saw_signal = 0;
  char *sigspec = "TERM", *word;
  pid_t pid;

  if (!list)
    return (EXECUTION_SUCCESS);

  /* Process options. */
  while (list)
    {
      word = list->word->word;

      if (ISOPTION (word, 'l'))
	{
	  listing++;
	  list = list->next;
	}
      else if (ISOPTION (word, 's'))
	{
	  list = list->next;
	  if (list)
	    {
	      sigspec = list->word->word;
	      if (sigspec[0] == '0' && !sigspec[1])
		signal = 0;
	      else
		signal = decode_signal (sigspec, 0);
	      list = list->next;
	    }
	  else
	    {
	      builtin_error ("-s requires an argument");
	      return (EXECUTION_FAILURE);
	    }
	}
      else if (ISOPTION (word, '-'))
	{
	  list = list->next;
	  break;
	}
      /* If this is a signal specification then process it.  We only process
	 the first one seen; other arguments may signify process groups (e.g,
	 -num == process group num). */
      else if ((*word == '-') && !saw_signal)
	{
	  sigspec = word + 1;
	  signal = decode_signal (sigspec, 0);
	  saw_signal++;
	  list = list->next;
	}
      else
	break;
    }

  if (listing)
    {
      if (!list)
	{
	  register int i;
	  register int column = 0;
	  char *name;

	  for (i = 1; i < NSIG; i++)
	    {
	      name = signal_name (i);
	      if (STREQN (name, "SIGJUNK", 7) || STREQN (name, "Unknown", 7))
		continue;

	      if (posixly_correct)
	        printf ("%s%s", name, (i == NSIG - 1) ? "" : " ");
	      else
		{
		  printf ("%2d) %s", i, name);

		  if (++column < 4)
		    printf ("\t");
		  else
		    {
		      printf ("\n");
		      column = 0;
		    }
		}
	    }

	  if (posixly_correct || column != 0)
	    printf ("\n");
	}
      else
	{
	  /* List individual signal names. */
	  while (list)
	    {
	      int signum;
	      char *name;

	      if ((sscanf (list->word->word, "%d", &signum) != 1) ||
		  (signum <= 0))
		{
	    list_error:
		  builtin_error ("bad signal number: %s", list->word->word);
		  list = list->next;
		  continue;
		}

	      /* This is specified by Posix.2 so that exit statuses can be
		 mapped into signal numbers. */
	      if (signum > 128)
		signum -= 128;

	      if (signum >= NSIG)
		goto list_error;

	      name = signal_name (signum);
	      if (STREQN (name, "SIGJUNK", 7) || STREQN (name, "Unknown", 7))
		{
		  list = list->next;
		  continue;
		}
	      printf ("%s\n", name);
	      list = list->next;
	    }
	}
      return (EXECUTION_SUCCESS);
    }

  /* OK, we are killing processes. */
  if (signal == NO_SIG)
    {
      builtin_error ("bad signal spec `%s'", sigspec);
      return (EXECUTION_FAILURE);
    }

  while (list)
    {
      word = list->word->word;

      if (*word == '-')
	word++;

      if (all_digits (word))
	{
	  /* Use the entire argument in case of minus sign presence. */
	  pid = (pid_t) atoi (list->word->word);

	  if (kill_pid (pid, signal, 0) < 0)
	    goto signal_error;
	  else
	    any_succeeded++;
	}
      else if (*list->word->word != '%')
	{
	  builtin_error ("No such pid %s", list->word->word);
	  CONTINUE_OR_FAIL;
	}
#if 1
      else if (interactive)
	/* Posix.2 says you can kill without job control active (4.32.4) */
#else
      else if (job_control)	/* can't kill jobs if not using job control */
#endif
	{			/* Must be a job spec.  Check it out. */
	  int job;
	  sigset_t set, oset;

	  BLOCK_CHILD (set, oset);
	  job = get_job_spec (list);

	  if (job < 0 || job >= job_slots || !jobs[job])
	    {
	      if (job != DUP_JOB)
		builtin_error ("No such job %s", list->word->word);
	      UNBLOCK_CHILD (oset);
	      CONTINUE_OR_FAIL;
	    }

	  /* Job spec used.  Kill the process group. If the job was started
	     without job control, then its pgrp == shell_pgrp, so we have
	     to be careful.  We take the pid of the first job in the pipeline
	     in that case. */
	  if (jobs[job]->flags & J_JOBCONTROL)
	    pid = jobs[job]->pgrp;
	  else
	    pid = jobs[job]->pipe->pid;

	  UNBLOCK_CHILD (oset);

	  if (kill_pid (pid, signal, 1) < 0)
	    {
	    signal_error:
	      if (errno == EPERM)
		builtin_error ("(%d) - Not owner", (int)pid);
	      else if (errno == ESRCH)
		builtin_error ("(%d) - No such pid", (int)pid);
	      else
		builtin_error ("Invalid signal %d", signal);
	      CONTINUE_OR_FAIL;
	    }
	  else
	    any_succeeded++;
	}
      else
	{
	  builtin_error ("bad process specification `%s'", list->word->word);
	  CONTINUE_OR_FAIL;
	}
    continue_killing:
      list = list->next;
    }

  if (any_succeeded)
    return (EXECUTION_SUCCESS);
  else
    return (EXECUTION_FAILURE);
}
#endif /* JOB_CONTROL */
