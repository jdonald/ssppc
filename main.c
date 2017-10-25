/*
 * main.c - main line routines
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 *
 * Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
 *
 * This source file is distributed "as is" in the hope that it will be
 * useful.  The tool set comes with no warranty, and no author or
 * distributor accepts any responsibility for the consequences of its
 * use. 
 *
 * Everyone is granted permission to copy, modify and redistribute
 * this tool set under the following conditions:
 *
 *    This source code is distributed for non-commercial use only. 
 *    Please contact the maintainer for restrictions applying to 
 *    commercial use.
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * $Id: main.c,v 1.1.1.1 2001/08/10 14:58:22 karu Exp $
 *
 * $Log: main.c,v $
 * Revision 1.1.1.1  2001/08/10 14:58:22  karu
 * creating ss3ppc-linux repository. The only difference between this and
 * the original ss3ppc linux is that crosss endian issues are taken
 * care of here.
 *
 *
 * Revision 1.1.1.1  2001/08/08 21:28:52  karu
 * creating ss3ppc linux repository
 *
 * Revision 1.7  2001/06/08 00:47:01  karu
 * fixed sim-cheetah. eio does not work because i am still not sure how much state to commit for system calls.
 *
 * Revision 1.6  2000/04/11 00:55:55  karu
 * made source platform independent. compiles on linux, solaris and ofcourse aix.
 * powerpc-nonnative.def is the def file with no inline assemble code.
 * manually create the machine.def symlink if compiling on non-aix OS.
 * also syscall.c does not compile because the syscalls haven't been
 * resourced with diff. flags and names for different operating systems.
 *
 * Revision 1.5  2000/04/03 20:03:21  karu
 * entire specf95 working .
 *
 * Revision 1.4  2000/03/15 10:12:59  karu
 * made a lot of changes to cror, crand, crnand, crnor, creqv, crandc, crorc
 * the original code wasn't generating a proper mask.
 * but these changes now affect printf with integere width
 * ie printf("%3d", i) does not produce the 3 spaces :-(
 *
 * Revision 1.3  2000/03/09 02:27:27  karu
 * checkpoint support complete. file name passed as env variable FROZENFILE
 *
 * Revision 1.2  2000/03/09 01:30:20  karu
 * added savestate and readstate functions
 *
 * still have incorporate filename to which we save the state
 *
 * Revision 1.1.1.1  2000/03/07 05:15:17  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:50  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.8  1998/08/31 17:09:49  taustin
 * dumbed down parts for MS VC++
 * added unistd.h include to remove warnings
 *
 * Revision 1.7  1998/08/27 08:39:28  taustin
 * added support for MS VC++ compilation
 * implemented host interface description in host.h
 * added target interface support
 * added simulator and program output redirection (via "-redir:sim"
 *       and "redir:prog" options, respectively)
 * added "-nice" option (available on all simulator) that resets
 *       simulator scheduling priority to specified level
 * added checkpoint support
 * implemented a more portable random() interface
 * all simulators now emit command line used to invoke them
 *
 * Revision 1.6  1997/04/16  22:09:20  taustin
 * added standalone loader support
 *
 * Revision 1.5  1997/03/11  01:13:36  taustin
 * updated copyright
 * random number generator seed option added
 * versioning format simplified (X.Y)
 * fast terminate (-q) option added
 * RUNNING flag now helps stats routine determine when to spew stats
 *
 * Revision 1.3  1996/12/27  15:52:20  taustin
 * updated comments
 * integrated support for options and stats packages
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <unistd.h>
#include <sys/time.h>
#endif
#ifdef BFD_LOADER
#include <bfd.h>
#endif /* BFD_LOADER */

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "endian.h"
#include "version.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "loader.h"
#include "sim.h"

/* stats signal handler */
static void
signal_sim_stats(int sigtype)
{
  sim_dump_stats = TRUE;
}

/* exit signal handler */
static void
signal_exit_now(int sigtype)
{
  sim_exit_now = TRUE;
}

/* execution instruction counter */
__thread counter_t sim_num_insn = 0;

#if 0 /* not portable... :-( */
/* total simulator (data) memory usage */
__thread unsigned int sim_mem_usage = 0;
#endif

/* execution start/end times */
time_t sim_start_time;
time_t sim_end_time;
int sim_elapsed_time;
// jdonald leaving these sim_start_time, etc. as global, non-__thread

/* byte/word swapping required to execute target executable on this host */
int sim_swap_bytes;
int sim_swap_words;

/* exit when this becomes non-zero */
int sim_exit_now = FALSE;
// sim_exit_now remains as global not __thread /jdonald

/* longjmp here when simulation is completed */
jmp_buf sim_exit_buf; // TODO: create different jumping points for the different child threads /jdonald

/* set to non-zero when simulator should dump statistics */
int sim_dump_stats = FALSE;

// options kept global /jdonald not __thread
/* options database */
struct opt_odb_t *sim_odb;

/* stats database */
__thread struct stat_sdb_t *sim_sdb; // jdonald: Hmm, this should have been __thread all along

/* EIO interfaces */
__thread char *sim_eio_fname = NULL;
__thread char *sim_chkpt_fname = NULL;
__thread FILE *sim_eio_fd = NULL;

/* redirected program/simulator output file names */
static __thread char *sim_simout = NULL;
static __thread char *sim_progout = NULL;
FILE __thread *sim_progfd = NULL;

/* track first argument orphan, this is the program to execute */
static __thread int exec_index = -1;

/* dump help information */
static int help_me;

/* random number generator seed */
static int rand_seed; // no __thread. good thing rand_seed doesn't seem to be used in SimpleScalar anymore /jdonald

/* initialize and quit immediately */
static int init_quit;

#ifndef _MSC_VER
/* simulator scheduling priority */
static int nice_priority;
#endif

/* default simulator scheduling priority */
#define NICE_DEFAULT_VALUE		0

static int
orphan_fn(int i, int argc, char **argv)
{
  exec_index = i;
  return /* done */FALSE;
}

static void
banner(FILE *fd, int argc, char **argv)
{
  char *s;

  fprintf(fd,
	  "%s: SimpleScalar/%s Tool Set version %d.%d of %s.\n"
	  "Copyright (c) 1994-1998 by Todd M. Austin.  All Rights Reserved.\n"
	  "\n",
	  ((s = strrchr(argv[0], '/')) ? s+1 : argv[0]),
	  VER_TARGET, VER_MAJOR, VER_MINOR, VER_UPDATE);
}

static void
usage(FILE *fd, int argc, char **argv)
{
  fprintf(fd, "Usage: %s {-options} executable {arguments}\n", argv[0]);
  opt_print_help(sim_odb, fd);
}

static int running = FALSE; // keep running as global non __thread? /jdonald 

/* print all simulator stats */
void
sim_print_stats(FILE *fd)		/* output stream */
{
#if 0 /* not portable... :-( */
  extern char etext, *sbrk(int);
#endif

  if (!running)
    return;

  /* get stats time */
  sim_end_time = time((time_t *)NULL);
  sim_elapsed_time = MAX(sim_end_time - sim_start_time, 1);

#if 0 /* not portable... :-( */
  /* compute simulator memory usage */
  sim_mem_usage = (sbrk(0) - &etext) / 1024;
#endif

  /* print simulation stats */
  fprintf(fd, "\nsim: ** simulation statistics **\n");
  stat_print_stats(sim_sdb, fd);
  sim_aux_stats(fd);
  fprintf(fd, "\n");
}

/* print stats, uninitialize simulator components, and exit w/ exitcode */
static void
exit_now(int exit_code)
{
  /* print simulation stats */
  sim_print_stats(stderr);

  /* un-initialize the simulator */
  sim_uninit();

  /* all done! */
  exit(exit_code);
}

int
main(int argc, char **argv, char **envp)
{
#if 1 // This functionality is needed, mainly for sim-* programs other than sim-outorder
  char *s;
  int i;
#endif
  int exit_code;


#ifndef _MSC_VER
  /* catch SIGUSR1 and dump intermediate stats */
  signal(SIGUSR1, signal_sim_stats);

  /* catch SIGUSR2 and dump final stats and exit */
  signal(SIGUSR2, signal_exit_now);
#endif /* _MSC_VER */

  /* register an error handler */
  fatal_hook(sim_print_stats);

  // set up a non-local exit point
  
  if ((exit_code = setjmp(sim_exit_buf)) != 0)
    {
      // special handling as longjmp cannot pass 0
      exit_now(exit_code-1);
    }

  /* register global options */
  sim_odb = opt_new(orphan_fn);
  opt_reg_flag(sim_odb, "-h", "print help message",
	       &help_me, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_flag(sim_odb, "-v", "verbose operation",
	       &verbose, /* default */FALSE, /* !print */FALSE, NULL);
#ifdef DEBUG
  opt_reg_flag(sim_odb, "-d", "enable debug message",
	       &debugging, /* default */FALSE, /* !print */FALSE, NULL);
#endif /* DEBUG */
  opt_reg_flag(sim_odb, "-i", "start in Dlite debugger",
	       &dlite_active, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_int(sim_odb, "-seed",
	      "random number generator seed (0 for timer seed)",
	      &rand_seed, /* default */1, /* print */TRUE, NULL);
  opt_reg_flag(sim_odb, "-q", "initialize and terminate immediately",
	       &init_quit, /* default */FALSE, /* !print */FALSE, NULL);
  opt_reg_string(sim_odb, "-chkpt", "restore EIO trace execution from <fname>",
		 &sim_chkpt_fname, /* default */NULL, /* !print */FALSE, NULL);
  /*
  opt_reg_string(sim_odb, "-freezeto", "file to save state in",
			&freezetofilename, NULL, TRUE, NULL);
  opt_reg_string(sim_odb, "-meltfrom", "file to save state in",
         &meltfromfilename, NULL, TRUE, NULL);
  */
  opt_reg_flag(sim_odb, "-interactive", "run in interactive mode",
         &interactive, /* default */FALSE, /* !print */FALSE, NULL);


  /* stdio redirection options */
  opt_reg_string(sim_odb, "-redir:sim",
		 "redirect simulator output to file (non-interactive only)",
		 &sim_simout,
		 /* default */NULL, /* !print */FALSE, NULL);
  opt_reg_string(sim_odb, "-redir:prog",
		 "redirect simulated program output to file",
		 &sim_progout, /* default */NULL, /* !print */FALSE, NULL);

#ifndef _MSC_VER
  /* scheduling priority option */
  opt_reg_int(sim_odb, "-nice",
	      "simulator scheduling priority", &nice_priority,
	      /* default */NICE_DEFAULT_VALUE, /* print */TRUE, NULL);
#endif

  /* FIXME: add stats intervals and max insts... */

  /* register all simulator-specific options */
  sim_reg_options(sim_odb);

  /* parse simulator options */
  exec_index = -1;
  opt_process_options(sim_odb, argc, argv);

  /* redirect I/O? */
  if (sim_simout != NULL)
    {
      /* send simulator non-interactive output (STDERR) to file SIM_SIMOUT */
      fflush(stderr);
      if (!freopen(sim_simout, "w", stderr))
	fatal("unable to redirect simulator output to file `%s'", sim_simout);
    }

  if (sim_progout != NULL)
    {
      /* redirect simulated program output to file SIM_PROGOUT */
      sim_progfd = fopen(sim_progout, "w");
      if (!sim_progfd)
	fatal("unable to redirect program output to file `%s'", sim_progout);
    }

  /* need at least two argv values to run */
  if (argc < 2)
    {
      banner(stderr, argc, argv);
      usage(stderr, argc, argv);
      exit(1);
    }

  /* opening banner */
  banner(stderr, argc, argv);

  if (help_me)
    {
      /* print help message and exit */
      usage(stderr, argc, argv);
      exit(1);
    }

  /* seed the random number generator */
  if (rand_seed == 0)
    {
      /* seed with the timer value, true random */
      mysrand(time((time_t *)NULL));
    }
  else
    {
      /* seed with default or user-specified random number generator seed */
      mysrand(rand_seed);
    }

  /* exec_index is set in orphan_fn() */
  if (exec_index == -1)
    {
      /* executable was not found */
      fprintf(stderr, "error: no executable specified\n");
      usage(stderr, argc, argv);
      exit(1);
    }
  /* else, exec_index points to simulated program arguments */

  /* check simulator-specific options */
  sim_check_options(sim_odb, argc, argv);

#ifndef _MSC_VER
  /* set simulator scheduling priority */
  if (nice(0) != nice_priority)
    {
      if (nice(nice_priority - nice(0)) < 0)
	fatal("could not renice simulator process");
    }
#endif

  /* default architected value... */
  sim_num_insn = 0;

#ifdef BFD_LOADER
  /* initialize the bfd library */
  bfd_init();
#endif /* BFD_LOADER */

  /* initialize the instruction decoder */
  md_init_decoder();

  /* initialize all simulation modules */
  sim_init();

  /* initialize architected state */
  sim_load_prog(argv[exec_index], argc-exec_index, argv+exec_index, envp);

#if 1 // the rest of main() is code which is used in sim-outorder.c, but still needed for other sim-* programs /jdonald
  /* register all simulator stats */
  sim_sdb = stat_new();
  sim_reg_stats(sim_sdb);
#if 0 /* not portable... :-( */
  stat_reg_uint(sim_sdb, "sim_mem_usage",
		"total simulator (data) memory usage",
		&sim_mem_usage, sim_mem_usage, "%11dk");
#endif

  /* record start of execution time, used in rate stats */
  sim_start_time = time((time_t *)NULL);

  /* emit the command line for later reuse */
  fprintf(stderr, "sim: command line: ");
  for (i=0; i < argc; i++)
    fprintf(stderr, "%s ", argv[i]);
  fprintf(stderr, "\n");

  /* output simulation conditions */
  s = ctime(&sim_start_time);
  if (s[strlen(s)-1] == '\n')
    s[strlen(s)-1] = '\0';
  fprintf(stderr, "\nsim: simulation started @ %s, options follow:\n", s);
  opt_print_options(sim_odb, stderr, /* short */TRUE, /* notes */TRUE);
  sim_aux_config(stderr);
  fprintf(stderr, "\n");

  /* omit option dump time from rate stats */
  sim_start_time = time((time_t *)NULL);

  if (init_quit)
    exit_now(0);

  running = TRUE;
  sim_main();
  /* simulation finished early */
  exit_now(0);
#endif // code which was moved to sim-outorder.c

  return 0;
}
