/*
 * ptrace.h - pipeline tracing definitions and interfaces
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
 * $Id: ptrace.h,v 1.1.1.1 2001/08/10 14:58:22 karu Exp $
 *
 * $Log: ptrace.h,v $
 * Revision 1.1.1.1  2001/08/10 14:58:22  karu
 * creating ss3ppc-linux repository. The only difference between this and
 * the original ss3ppc linux is that crosss endian issues are taken
 * care of here.
 *
 *
 * Revision 1.1.1.1  2001/08/08 21:28:53  karu
 * creating ss3ppc linux repository
 *
 * Revision 1.1.1.1  2000/03/07 05:15:17  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:51  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.2  1998/08/27 15:50:04  taustin
 * implemented host interface description in host.h
 * added target interface support
 *
 * Revision 1.1  1997/03/11  01:32:28  taustin
 * Initial revision
 *
 *
 */

#ifndef PTRACE_H
#define PTRACE_H

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "range.h"

/*
 * pipeline events:
 *
 *	+ <iseq> <pc> <addr> <inst>	- new instruction def
 *	- <iseq>			- instruction squashed or retired
 *	@ <cycle>			- new cycle def
 *	* <iseq> <stage> <events>	- instruction stage transition
 *
 */

/*
	[IF]   [DA]   [EX]   [WB]   [CT]
         aa     dd     jj     ll     nn
         bb     ee     kk     mm     oo
         cc                          pp
 */

/* pipeline stages */
#define PST_IFETCH		"IF"
#define PST_DISPATCH		"DA"
#define PST_EXECUTE		"EX"
#define PST_WRITEBACK		"WB"
#define PST_COMMIT		"CT"

/* pipeline events */
#define PEV_CACHEMISS		0x00000001	/* I/D-cache miss */
#define PEV_TLBMISS		0x00000002	/* I/D-tlb miss */
#define PEV_MPOCCURED		0x00000004	/* mis-pred branch occurred */
#define PEV_MPDETECT		0x00000008	/* mis-pred branch detected */
#define PEV_AGEN		0x00000010	/* address generation */

/* pipetrace file */
extern __thread FILE *ptrace_outfd;

#ifdef TARGET_X86 // jdonald: no need for ptracepipe to be exclusive to x86, but doing it this way for now
/* pipetrace pipe */
extern __thread int ptracepipe;
#endif // TARGET_X86

/* pipetracing is active */
extern __thread int ptrace_active;

/* pipetracing range */
extern __thread struct range_range_t ptrace_range;

/* one-shot switch for pipetracing */
extern __thread int ptrace_oneshot;

/* open pipeline trace */
void
ptrace_open(char *range,		/* trace range */
	    char *fname);		/* output filename */

/* close pipeline trace */
void
ptrace_close(void);

/* NOTE: pipetracing is a one-shot switch, since turning on a trace more than
   once will mess up the pipetrace viewer */
#define ptrace_check_active(PC, ICNT, CYCLE)				\
  ((ptrace_outfd != NULL						\
    && !range_cmp_range1(&ptrace_range, (PC), (ICNT), (CYCLE)))		\
   ? (!ptrace_oneshot ? (ptrace_active = ptrace_oneshot = TRUE) : FALSE)\
   : (ptrace_active = FALSE))

/* main interfaces, with fast checks */
#define ptrace_newinst(A,B,C,D)						\
  if (ptrace_active) __ptrace_newinst((A),(B),(C),(D))

#if defined(TARGET_ARM) || defined(TARGET_X86) // jdonald: not clear why ARM and x86 have different interfaces for ptrace_newuop()
  #define ptrace_newuop(A,B,C,D,E)	       				\
    if (ptrace_active) __ptrace_newuop((A),(B),(C),(D),(E))
#else // !defined(TARGET_ARM) && !defined(TARGET_X86)
  #define ptrace_newuop(A,B,C,D)						\
    if (ptrace_active) __ptrace_newuop((A),(B),(C),(D))
#endif // !defined(TARGET_ARM) && !defined(TARGET_X86)

#define ptrace_endinst(A)						\
  if (ptrace_active) __ptrace_endinst((A))
#define ptrace_newcycle(A)						\
  if (ptrace_active) __ptrace_newcycle((A))

#if defined(TARGET_ARM) // jdonald: different ARM ptrace_newstage (it calls ptrace_newstage_lat)
  #define ptrace_newstage(A,B,C,D,E) 			\
    if (ptrace_active) __ptrace_newstage_lat((A),(B),(C),(D),(E))
#elif defined(TARGET_X86) // jdonald: different x86 ptrace_newstage
  #define ptrace_newstage(A,B,C,D,E)				       	\
    if (ptrace_active) __ptrace_newstage((A),(B),(C),(D),(E))
#else // !defined(TARGET_ARM) && !defined(TARGET_X86)
  #define ptrace_newstage(A,B,C)						\
    if (ptrace_active) __ptrace_newstage((A),(B),(C))
#endif // !defined(TARGET_ARM) && !defined(TARGET_X86)

#define ptrace_active(A,I,C)						\
  (ptrace_outfd != NULL	&& !range_cmp_range(&ptrace_range, (A), (I), (C)))

/* declare a new instruction */
void
__ptrace_newinst(unsigned int iseq,	/* instruction sequence number */
		 md_inst_t inst,	/* new instruction */
		 md_addr_t pc,		/* program counter of instruction */
		 md_addr_t addr);	/* address referenced, if load/store */

/* declare a new uop */
#if defined(TARGET_ARM) // jdonald
/* declare a new uop */
void
__ptrace_newuop(unsigned int iseq,	/* instruction sequence number */
		md_inst_t inst,		/* new uop description */
		enum md_opcode op,
		md_addr_t pc,		/* program counter of instruction */
		md_addr_t addr);	/* address referenced, if load/store */

#elif defined(TARGET_X86) // jdonald
void
__ptrace_newuop(unsigned int iseq,	/* instruction sequence number */
		enum md_opcode op,      /* microcode opcode */
		md_inst_t inst,		/* instruction */
		md_addr_t pc,		/* program counter of instruction */
		md_addr_t addr);	/* address referenced, if load/store */
#else // !defined(TARGET_ARM) && !defined(TARGET_X86)

/* declare a new uop */
void
__ptrace_newuop(unsigned int iseq,	/* instruction sequence number */
		char *uop_desc,		/* new uop description */
		md_addr_t pc,		/* program counter of instruction */
		md_addr_t addr);	/* address referenced, if load/store */
#endif // !defined(TARGET_X86)

/* declare instruction retirement or squash */
void
__ptrace_endinst(unsigned int iseq);	/* instruction sequence number */

/* declare a new cycle */
void
__ptrace_newcycle(tick_t cycle);	/* new cycle */

/* indicate instruction transition to a new pipeline stage */
#if defined(TARGET_ARM) // jdonald: these really ought to be merged somehow... probably just by taking the most advanced version from x86
/* indicate instruction transition to a new pipeline stage */
void
__ptrace_newstage(unsigned int iseq,	/* instruction sequence number */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents);/* pipeline events while in stage */

void
__ptrace_newstage_lat(unsigned int iseq,	/* instruction sequence number */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents, unsigned int latency, unsigned int longerlat);/* pipeline events while in stage */
#elif defined(TARGET_X86)
void
__ptrace_newstage(unsigned int iseq,	/* instruction sequence number */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents, unsigned int latency, unsigned int longerlat);/* pipeline events while in stage */
#else // !defined(TARGET_ARM) && !defined(TARGET_X86)
void
__ptrace_newstage(unsigned int iseq,	/* instruction sequence number */
		  char *pstage,		/* pipeline stage entered */
		  unsigned int pevents);/* pipeline events while in stage */
#endif // !defined(TARGET_X86)

#ifdef TARGET_X86 // jdonald: taken ptrace_print_stat() and ptrace_print_stats() from ss-x86-sel/ptrace.h
/* print the value of stat variable STAT */
void
ptrace_print_stat(struct stat_sdb_t *sdb,       /* stat database */
                struct stat_stat_t *stat,/* stat variable */
                FILE *fd);              /* output stream */

/* print the value of all stat variables in stat database SDB */
void
ptrace_print_stats(struct stat_sdb_t *sdb,/* stat database */
                 FILE *fd);             /* output stream */
#endif // TARGET_X86
#endif /* PTRACE_H */
