/*
 * sim-fast.c - sample fast functional simulator implementation
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
 * $Id: sim-fast.c,v 1.1.1.1 2001/08/10 14:58:22 karu Exp $
 *
 * $Log: sim-fast.c,v $
 * Revision 1.1.1.1  2001/08/10 14:58:22  karu
 * creating ss3ppc-linux repository. The only difference between this and
 * the original ss3ppc linux is that crosss endian issues are taken
 * care of here.
 *
 *
 * Revision 1.2  2001/08/10 14:54:05  karu
 * modified SWAP_QWORD macro
 *
 * Revision 1.1.1.1  2001/08/08 21:28:53  karu
 * creating ss3ppc linux repository
 *
 * Revision 1.29  2001/08/03 20:45:34  karu
 * checkpoint also works. checkpointing wasnt working previously because, i was always setting the CR register to a constant value at the begining of sim-fast. this is obviously wrong while doing checking pointing. so i removed that. and voila it works. also added the LR, CTR and TBR to the list of miscellaneous registers saved when a checkpoint is created. everything seems to work now.
 *
 * Revision 1.28  2001/06/29 03:58:41  karu
 * made some minor edits to simfast and simoutorder to remove some warnings
 *
 * Revision 1.27  2001/06/27 03:58:57  karu
 * fixed errno setting in loader.c and added errno support to all syscalls. added 3 more syscalls access, accessx and faccessx to get NAS benchmark MG working. also added MTFB instruction from the patch AK mailed me looooong time back. the NAS benchmarks were also dying becuase of that instruction just like sphinx
 *
 * Revision 1.26  2001/06/08 00:47:03  karu
 * fixed sim-cheetah. eio does not work because i am still not sure how much state to commit for system calls.
 *
 * Revision 1.25  2001/06/05 09:41:02  karu
 * removed warnings and fixed sim-profile. there is still an addressing mode problem unresolved in sim-profile. sim-eio has some system call problems unresolved.
 *
 * Revision 1.24  2001/04/11 05:15:25  karu
 * Fixed Makefile to work properly in Solaris also
 * Fixed sim-cache.c
 * Fixed a minor bug in sim-outorder. initializes stack_recover_idx to 0
 * in line 5192. the top stack other has a junk value when the branch
 * predictor returns from a function call. Dont know how this worked
 * in AIX. the bug manifests itself only on Solaris.
 *
 * Revision 1.23  2000/05/31 01:59:51  karu
 * added support for mis-aligned accesses
 *
 * Revision 1.22  2000/04/15 20:17:27  karu
 * *** empty log message ***
 *
 * Revision 1.21  2000/04/14 04:49:23  karu
 * sim-cache works
 * made minor changes to SC_IMPL macro. SC_IMPL calls SYSCALL macro
 * and SYSCALL is defined in each simulatar following alpha and pisa style
 * of syscall implementation
 *
 * Revision 1.20  2000/04/11 01:00:34  karu
 * removed line in sim-fast which exits on a little endian host
 *
 * Revision 1.19  2000/04/11 00:55:57  karu
 * made source platform independent. compiles on linux, solaris and ofcourse aix.
 * powerpc-nonnative.def is the def file with no inline assemble code.
 * manually create the machine.def symlink if compiling on non-aix OS.
 * also syscall.c does not compile because the syscalls haven't been
 * resourced with diff. flags and names for different operating systems.
 *
 * Revision 1.18  2000/04/10 23:46:39  karu
 * converted all quad to qword. NO OTHER change made.
 * the tag specfp95-before-quad-qword is a snapshow before this change was made
 *
 * Revision 1.17  2000/04/10 23:09:18  karu
 * added step command and a ifdef for interactive mode to speed up compilation
 *
 * Revision 1.16  2000/04/08 00:52:21  karu
 * added support for fsubs, fadds, fmuls etc..
 * modified Makefile and added machine.o with machine.def as its dependency
 *
 * Revision 1.15  2000/04/07 20:50:34  karu
 * fixed problem in loader - argv[0] wasn't working in simulated programs
 * also added showmems to interactive commands
 *
 * Revision 1.14  2000/04/05 01:29:02  karu
 * fixed environ
 *
 * Revision 1.13  2000/04/04 02:51:58  karu
 * added a few more commands to debugger
 *
 * Revision 1.12  2000/04/04 01:31:21  karu
 * added a few more commands to debugger
 * and fixed a problem in system_configuration. thanks to ramdass for finding it out :-)
 *
 * Revision 1.11  2000/04/04 00:55:36  karu
 * added interactive debugger from code steve gave me - ripped from M machine code
 *
 * Revision 1.10  2000/04/03 20:03:22  karu
 * entire specf95 working .
 *
 * Revision 1.9  2000/03/28 00:37:12  karu
 * added fstatx and added fdivs implementation
 *
 * Revision 1.8  2000/03/23 02:50:24  karu
 * cosmetic change to sim-fast.c to see if repository change works.
 * we now use /projects/cart/projects/ss3-ppc/cvs2
 *
 * Revision 1.7  2000/03/16 07:43:11  karu
 * changes made to machine.def
 *
 * while doing revision 1.6 to machine.def cror, crand, crnand, creqv, crnor....
 * were modified.
 * i had used _a & ~_b to say a and not b. should have used the ! opertor
 * ~ inverts all bits. so the condition codes were being screwed.
 *
 * changed all the ~ to ! in the required instrctions. (crnand,
 * crnor, creqv, crandc, crorc).
 *
 * NOW condition codes MATCH exactly what the native program does.
 * verified for two programs so far :-)
 *
 * Revision 1.6  2000/03/16 01:54:10  karu
 * made changes to mtcrf
 *
 * Revision 1.5  2000/03/15 10:13:00  karu
 * made a lot of changes to cror, crand, crnand, crnor, creqv, crandc, crorc
 * the original code wasn't generating a proper mask.
 * but these changes now affect printf with integere width
 * ie printf("%3d", i) does not produce the 3 spaces :-(
 *
 * Revision 1.4  2000/03/09 02:27:28  karu
 * checkpoint support complete. file name passed as env variable FROZENFILE
 *
 * Revision 1.3  2000/03/09 01:30:21  karu
 * added savestate and readstate functions
 *
 * still have incorporate filename to which we save the state
 *
 * Revision 1.2  2000/03/07 09:24:33  karu
 * fixed millicode 3280 encoding bug
 *
 * Revision 1.1.1.1  2000/03/07 05:15:18  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:52  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.5  1998/08/27 16:26:37  taustin
 * USE_JUMP_TABLE option now autodetects GNU GCC jump table support
 * implemented host interface description in host.h
 * added target interface support
 * added support for register and memory contexts
 * instruction predecoding moved to loader module
 * Alpha target support added
 * added support for quadword's
 * added fault support
 *
 * Revision 1.4  1997/03/11  01:28:36  taustin
 * updated copyright
 * simulation now fails when DLite! support requested
 * long/int tweaks made for ALPHA target support
 * supported added for non-GNU C compilers
 *
 * Revision 1.3  1997/01/06  16:04:21  taustin
 * comments updated
 * SPARC-specific compilation supported deleted (most opts now generalized)
 * new stats and options package support added
 * NO_INSN_COUNT added for conditional instruction count support
 * USE_JUMP_TABLE added to support simple and complex main loops
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <sys/stat.h>
#include <assert.h>

/*
 * This file implements a very fast functional simulator.  This functional
 * simulator implementation is much more difficult to digest than the simpler,
 * cleaner sim-safe functional simulator.  By default, this simulator performs
 * no instruction error checking, as a result, any instruction errors will
 * manifest as simulator execution errors, possibly causing sim-fast to
 * execute incorrectly or dump core.  Such is the price we pay for speed!!!!
 *
 * The following options configure the bag of tricks used to make sim-fast
 * live up to its name.  For most machines, defining all the options results
 * in the fastest functional simulator.
 */

/* don't count instructions flag, enabled by default, disable for inst count */
#undef NO_INSN_COUNT

#ifdef __GNUC__
/* faster dispatch mechanism, requires GNU GCC C extensions, CAVEAT: some
   versions of GNU GCC core dump when optimizing the jump table code with
   optimization levels higher than -O1 */
/* #define USE_JUMP_TABLE */
#endif /* __GNUC__ */

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "sim.h"

#define UNIX 1

/* stuff for interactive debugger */

#include "interactive-types.h"
#include "interactive-read.h"
#include "interactive-cmd.h"


void InitializeCommonCommands(tword);
void msimReadEvalPrintLoop(char *prompt);
/* end stuff */

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;
int found[MD_MAX_MASK];

#if defined(TARGET_ALPHA) || defined(TARGET_PPC) // jdonald combined these two to one #if
/* predecoded text memory */
static struct mem_t *dec = NULL;
#endif

/* cosmetic change made to sim-fast.c to see if repository works */

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-fast: This simulator implements a very fast functional simulator.  This\n"
"functional simulator implementation is much more difficult to digest than\n"
"the simpler, cleaner sim-safe functional simulator.  By default, this\n"
"simulator performs no instruction error checking, as a result, any\n"
"instruction errors will manifest as simulator execution errors, possibly\n"
"causing sim-fast to execute incorrectly or dump core.  Such is the\n"
"price we pay for speed!!!!\n"
		 );
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  if (dlite_active)
    fatal("sim-fast does not support DLite debugging");
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
#ifndef NO_INSN_COUNT
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, sim_num_insn, NULL);
#endif /* !NO_INSN_COUNT */
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);
#ifndef NO_INSN_COUNT
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);
#endif /* !NO_INSN_COUNT */
  ld_reg_stats(sdb);
  mem_reg_stats(mem, sdb);
}

/* initialize the simulator */
void
sim_init(void)
{
  /* allocate and initialize register file */
  regs_init(&regs);

  /* allocate and initialize memory space */
  mem = mem_create("mem");
  mem_init(mem);
}

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
	      int argc, char **argv,	/* program arguments */
	      char **envp)		/* program environment */
{
  /* load program text and data, set up environment, memory, and regs */
  ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

#ifdef TARGET_ALPHA
  /* pre-decode text segment */
  {
    unsigned i, num_insn = (ld_text_size + 3) / 4;
	
    fprintf(stderr, "** pre-decoding %u insts...", num_insn);

    /* allocate decoded text space */
    dec = mem_create("dec");
	
    for (i=0; i < num_insn; i++) {
	  enum md_opcode op;
	  md_inst_t inst;
	  md_addr_t PC;
	  
	  /* compute PC */
	  PC = ld_text_base + i * sizeof(md_inst_t);
	  
	  /* get instruction from memory */
	  inst = __UNCHK_MEM_READ(mem, PC, md_inst_t);
	  
	  /* decode the instruction */
	  MD_SET_OPCODE(op, inst);
	  
	  
	  /* insert into decoded opcode space */
	  MEM_WRITE_WORD(dec, PC << 1, (word_t)op);
	  MEM_WRITE_WORD(dec, (PC << 1)+sizeof(word_t), inst);
	}
    fprintf(stderr, "done\n");
  } /* TARGET_ALPHA */
  
#elif defined(TARGET_PPC)

  /* declare prototype for writemillicode to suppres warnings */
#include "target-ppc/predecode.h"
 
#endif /* TARGET_PPC */

}


/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)
{
  /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)
{
  /* nada */
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  /* nada */
}

/*
 * configure the execution engine
 */

/* next program counter */
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#define CPC			(regs.regs_PC)

/* general purpose registers */
#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

#if defined(TARGET_PISA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_L(N)		(regs.regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs.regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs.regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs.regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs.regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs.regs_F.d[(N) >> 1] = (EXPR))

/* miscellaneous register accessors */
#define SET_HI(EXPR)		(regs.regs_C.hi = (EXPR))
#define HI			(regs.regs_C.hi)
#define SET_LO(EXPR)		(regs.regs_C.lo = (EXPR))
#define LO			(regs.regs_C.lo)
#define FCC			(regs.regs_C.fcc)
#define SET_FCC(EXPR)		(regs.regs_C.fcc = (EXPR))

#elif defined(TARGET_ALPHA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)		(regs.regs_F.q[N])
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#define FPR(N)			(regs.regs_F.d[N])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[N] = (EXPR))

/* miscellaneous register accessors */
#define FPCR			(regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#define UNIQ			(regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

#elif defined(TARGET_PPC)

// there is really nothing here. That's the way it's always been /jdonald

#elif defined(TARGET_ARM)
// the entire following block was taken from simplesim-arm/sim-safe.c (their sim-fast.c doesn't work) /jdonald

/* processor status register */
#define PSR			(regs.regs_C.cpsr)
#define SET_PSR(EXPR)		(regs.regs_C.cpsr = (EXPR))

#define PSR_N			_PSR_N(regs.regs_C.cpsr)
#define SET_PSR_N(EXPR)		_SET_PSR_N(regs.regs_C.cpsr, (EXPR))
#define PSR_C			_PSR_C(regs.regs_C.cpsr)
#define SET_PSR_C(EXPR)		_SET_PSR_C(regs.regs_C.cpsr, (EXPR))
#define PSR_Z			_PSR_Z(regs.regs_C.cpsr)
#define SET_PSR_Z(EXPR)		_SET_PSR_Z(regs.regs_C.cpsr, (EXPR))
#define PSR_V			_PSR_V(regs.regs_C.cpsr)
#define SET_PSR_V(EXPR)		_SET_PSR_V(regs.regs_C.cpsr, (EXPR))

/* floating point conversions */
union x { float f; word_t w; };
#define DTOW(D)		({ union x fw; fw.f = (float)(D); fw.w; })
#define WTOD(W)		({ union x fw; fw.w = (W); (double)fw.f; })
#define QSWP(Q)								\
  ((((Q) << 32) & ULL(0xffffffff00000000)) | (((Q) >> 32) & ULL(0xffffffff)))

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)		(QSWP(regs.regs_F.q[N]))
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = QSWP((EXPR)))
#define FPR_W(N)		(DTOW(regs.regs_F.d[N]))
#define SET_FPR_W(N,EXPR)	(regs.regs_F.d[N] = (WTOD(EXPR)))
#define FPR(N)			(regs.regs_F.d[(N)])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#define FPSR			(regs.regs_C.fpsr)
#define SET_FPSR(EXPR)		(regs.regs_C.fpsr = (EXPR))

#elif defined(TARGET_X86) // taken from ss-x86-sel/sim-fast.c /jdonald
/* current program counter */
#define CPC			(regs.regs_PC)

/* next program counter */
#define NPC			(regs.regs_NPC)
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))
#define SET_NPC_D(EXPR)         SET_NPC(EXPR)
#define SET_NPC_V(EXPR)							\
  ((inst.mode & MODE_OPER32) ? SET_NPC((EXPR)) : SET_NPC((EXPR) & 0xffff))

/* general purpose registers */
#define _HI(N)			((N) & 0x04)
#define _ID(N)			((N) & 0x03)
#define _ARCH(N)		((N) < MD_REG_TMP0)

/* segment registers ; UCSD*/
#define SEG_W(N)		(regs.regs_S.w[N])
#define SET_SEG_W(N,EXPR)	(regs.regs_S.w[N] = (EXPR))

#define GPR_B(N)		(_ARCH(N)				\
				 ? (_HI(N)				\
				    ? regs.regs_R.b[_ID(N)].hi		\
				    : regs.regs_R.b[_ID(N)].lo)  	\
				 : regs.regs_R.b[N].lo)
#define SET_GPR_B(N,EXPR)	(_ARCH(N)				   \
				 ? (_HI(N)				   \
				    ? (regs.regs_R.b[_ID(N)].hi = (EXPR))  \
				    : (regs.regs_R.b[_ID(N)].lo = (EXPR))) \
				 : (regs.regs_R.b[N].lo = (EXPR)))

#define GPR_W(N)		(regs.regs_R.w[N].lo)
#define SET_GPR_W(N,EXPR)	(regs.regs_R.w[N].lo = (EXPR))

#define GPR_D(N)		(regs.regs_R.d[N])
#define SET_GPR_D(N,EXPR)	(regs.regs_R.d[N] = (EXPR))

/* FIXME: move these to the DEF file? */
#define GPR_V(N)		((inst.mode & MODE_OPER32)		\
				 ? GPR_D(N)				\
				 : (word_t)GPR_W(N))
#define SET_GPR_V(N,EXPR)	((inst.mode & MODE_OPER32)		\
				 ? SET_GPR_D(N, EXPR)			\
				 : SET_GPR_W(N, EXPR))

#define GPR_A(N)		((inst.mode & MODE_ADDR32)		\
				 ? GPR_D(N)				\
				 : (word_t)GPR_W(N))
#define SET_GPR_A(N,EXPR)	((inst.mode & MODE_ADDR32)		\
				 ? SET_GPR_D(N, EXPR)			\
				 : SET_GPR_W(N, EXPR))

#define GPR_S(N)		((inst.mode & MODE_STACK32)		\
				 ? GPR_D(N)				\
				 : (word_t)GPR_W(N))
#define SET_GPR_S(N,EXPR)	((inst.mode & MODE_STACK32)		\
				 ? SET_GPR_D(N, EXPR)			\
				 : SET_GPR_W(N, EXPR))

#undef GPR     // jdonald: x86 is the only one that doesn't use the same GPR and SET_GPR as everyone else
#undef SET_GPR // ""

#define GPR(N)                  GPR_D(N)
#define SET_GPR(N,EXPR)         SET_GPR_D(N,EXPR)

#define AFLAGS(MSK)		(regs.regs_C.aflags & (MSK))
#define SET_AFLAGS(EXPR,MSK)	(assert(((EXPR) & ~(MSK)) == 0),	\
				 regs.regs_C.aflags =			\
				 ((regs.regs_C.aflags & ~(MSK))	\
				  | ((EXPR) & (MSK))))

#define FSW(MSK)		(regs.regs_C.fsw & (MSK))
#define SET_FSW(EXPR,MSK)	(assert(((EXPR) & ~(MSK)) == 0),	\
				 regs.regs_C.fsw =			\
				 ((regs.regs_C.fsw & ~(MSK))		\
				  | ((EXPR) & (MSK))))

// added by cristiano
#define CWD(MSK)                (regs.regs_C.cwd & (MSK))
#define SET_CWD(EXPR,MSK)       (assert(((EXPR) & ~(MSK)) == 0),        \
                                 regs.regs_C.cwd =                      \
                                 ((regs.regs_C.cwd & ~(MSK))            \
                                  | ((EXPR) & (MSK))))

/* floating point registers, L->word, F->single-prec, D->double-prec */
/* FIXME: bad overlap, also need to support stack indexing... */
#define _FPARCH(N)		((N) < MD_REG_FTMP0)
#define F2P(N)								\
  (_FPARCH(N)								\
   ? ((FSW_TOP(regs.regs_C.fsw) + (N)) & 0x07)				\
   : (N))
#define FPR(N)			(regs.regs_F.e[F2P(N)])
#define SET_FPR(N,EXPR)		(regs.regs_F.e[F2P(N)] = (EXPR))

// direct references, no top of the stack indirection used
//#define FPR_NS(N)			(regs.regs_F.e[(N)])
//#define SET_FPR_NS(N,EXPR)		(regs.regs_F.e[(N)] = (EXPR))

#define FPSTACK_POP()							\
  SET_FSW_TOP(regs.regs_C.fsw, (FSW_TOP(regs.regs_C.fsw) + 1) & 0x07)
#define FPSTACK_PUSH()							\
  SET_FSW_TOP(regs.regs_C.fsw, (FSW_TOP(regs.regs_C.fsw) + 7) & 0x07)

#define FPSTACK_OP(OP)							\
  {									\
    if ((OP) == fpstk_nop)						\
      /* nada... */;							\
    else if ((OP) == fpstk_pop)						\
      SET_FSW_TOP(regs.regs_C.fsw, (FSW_TOP(regs.regs_C.fsw)+1)&0x07);\
    else if ((OP) == fpstk_poppop)					\
      {									\
        SET_FSW_TOP(regs.regs_C.fsw, (FSW_TOP(regs.regs_C.fsw)+1)&0x07);\
        SET_FSW_TOP(regs.regs_C.fsw, (FSW_TOP(regs.regs_C.fsw)+1)&0x07);\
      }									\
    else if ((OP) == fpstk_push)					\
      SET_FSW_TOP(regs.regs_C.fsw, (FSW_TOP(regs.regs_C.fsw)+7)&0x07);\
    else								\
      panic("bogus FP stack operation");				\
  }
  
#else

#error No ISA target defined...

#endif

/* precise architected memory state accessor macros */
#ifdef TARGET_X86


#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, MEM_READ_BYTE(mem, /*addr = */(SRC)))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, XMEM_READ_HALF(mem, /*addr = */(SRC)))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, XMEM_READ_WORD(mem, /*addr = */(SRC)))
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, XMEM_READ_QWORD(mem, /*addr = */(SRC)))

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, MEM_WRITE_BYTE(mem, /*addr = */(DST), (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, XMEM_WRITE_HALF(mem, /*addr = */(DST), (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, XMEM_WRITE_WORD(mem, /*addr = */(DST), (SRC)))
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, XMEM_WRITE_QWORD(mem, /*addr = */(DST), (SRC)))

#else // !defined(TARGET_X86) from ss-x86-sel/sim-fast.c, the following #else contents were originally here too

/* precise architected memory state accessor macros */
#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, MEM_READ_BYTE(mem, (SRC)))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, MEM_READ_HALF(mem, (SRC)))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, MEM_READ_WORD(mem, (SRC)))

#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, MEM_READ_QWORD(mem, (SRC)))
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, MEM_WRITE_BYTE(mem, (DST), (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, MEM_WRITE_HALF(mem, (DST), (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, MEM_WRITE_WORD(mem, (DST), (SRC)))

  #ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, MEM_WRITE_QWORD(mem, (DST), (SRC)))
#endif /* HOST_HAS_QWORD */

#endif // !defined(TARGET_X86) /jdonald

/* system call handler macro */
#ifdef TARGET_PPC // jdonald: moved these to the same spot
  #define SYSCALL SCACTUAL_IMPL
#else
  #define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)
#endif

#ifndef NO_INSN_COUNT
#define INC_INSN_CTR()	sim_num_insn++
#else /* !NO_INSN_COUNT */
#define INC_INSN_CTR()	/* nada */
#endif /* NO_INSN_COUNT */

#ifdef TARGET_ALPHA
#define ZERO_FP_REG()	regs.regs_F.d[MD_REG_ZERO] = 0.0
#else
#define ZERO_FP_REG()	/* nada... */
#endif

void testfn(void) {
	printf("TEST FN\n");
}

tword cmd_showregs(tword r) { 
  int i, j;
  j = (int) r.data.val;
  if (j == -1) {
	printf("\n");
	for (i = 0; i < MD_NUM_IREGS; i++) {
#ifdef TARGET_ALPHA // jdonald fix: Alpha has 64-bit regs
	  printf("%d: 0x%16llx\t",i, regs.regs_R[i]);
#elif defined(TARGET_X86)
	  printf("%d: 0x%8x\t",i, regs.regs_R.d[i]); // jdonald
#else
	  printf("%d: 0x%8x\t",i, regs.regs_R[i]);
#endif
	}
	printf("\n");
#ifdef TARGET_PPC // jdonald non-PPC compatible
	printf("CR: 0x%8x, LR: 0x%8x, CNTR: 0x%8x, XER: 0x%8x, PC 0x%8x\n",
		   regs.regs_C.cr, regs.regs_L, regs.regs_CNTR, regs.regs_C.xer,
		   regs.regs_PC);
#else
	printf("CR: register output disabled in non-PPC modes\n");
#endif // jdonald: non-PPC compatible
	return TAGGED_TRUE;	
  }
#ifdef TARGET_ALPHA // jdonald fix: Alpha has 64-bit regs
  printf("%2d: 0x%16llx\n", j, regs.regs_R[j]);
#elif defined(TARGET_X86) // jdonald
  printf("%2d: 0x%8x\n", j, regs.regs_R.d[j]);
#else
  printf("%2d: 0x%8x\n", j, regs.regs_R[j]);
#endif
  return TAGGED_TRUE;	
}

tword cmd_showmem(tword r1, tword r2) {
  int i, j, v;
  int range;
  enum md_fault_type _fault;
  j = (int) r1.data.val;
  range = (int) r2.data.val;
  printf("mem %x %x\n", j, range);
  for (i = 0; i < range; i+=4) {
	v = READ_WORD(j, _fault); j += 4;
	if (_fault != md_fault_none) {
	  printf("Read generated fault\n");
	} else {
	  printf("%x ", v);
	}
  }
  printf("\n");
  return TAGGED_TRUE;
}

tword cmd_showmems(tword r1, tword r2) {
  int i, j, v;
  int range;
  enum md_fault_type _fault;
  j = (int) r1.data.val;
  range = (int) r2.data.val;
  printf("mems %x %x\n", j, range);
  for (i = 0; i < range; i++) {
	v = READ_BYTE(j, _fault); j++;
	if (_fault != md_fault_none) {
	  printf("Read generated fault\n");
	} else {
	  printf("%c", v);
	}
	if (v == 0x0) break;
  }
  printf("\n");
  return TAGGED_TRUE;
}          

#ifdef TARGET_X86 // jdonald: for x86, don't put md_inst_t inst as a 'register' variable
#define register
#endif

inline void stepi(void) {
#ifdef INTERACTIVE
  register md_inst_t inst;
  register enum md_opcode op;
  char ch;
  
  op = (enum md_opcode)( __UNCHK_MEM_READ(dec, regs.regs_PC << 1, word_t) );
  inst = (word_t)__UNCHK_MEM_READ(dec, (regs.regs_PC << 1)+sizeof(word_t), word_t) );
#ifdef TARGET_PISA // jdonald: fixing PISA problem of 64-bit instructions
  printf("INST 0x%x%x, PC 0x%x\n", inst.a, inst.b, regs.regs_PC, regs.regs_C.cr);
#else // TARGET_PPC, TARGET_ARM, TARGET_ALPHA, TARGET_X86
  printf("INST 0x%x, PC 0x%x\n", inst, regs.regs_PC, regs.regs_C.cr);
#endif // jdonald: fixing PISA problem of 64-bit instructions

   switch (op) {
	   
#define MYDEFINST(OP, NAME) \
     case OP:                   \
     	 SYMCAT(OP,_IMPL);                 \
     printf("%x %s\n", regs.regs_PC, NAME);fflush(stdout);	\
     break;
#define MYDEFLINK(OP) \
     case OP:                   \
       panic("attempted to execute a linking opcode");
     
#if defined(TARGET_PPC) // jdonald: support for PPC, ARM and others
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) \
	MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) \
  	MYDEFLINK(OP)
#elif defined(TARGET_ARM)
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4)      \
	MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,SHIFT,MASK) \
  	MYDEFLINK(OP)
#elif defined(TARGET_X86)
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4,OFLAGS,IFLAGS)\
    MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
    MYDEFLINK(OP)
#else // non-PPC/ARM/x86: TARGET_PISA, TARGET_ALPHA
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) \
	MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,MASK,SHIFT) \
  	MYDEFLINK(OP)
#endif


#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)                 \
     { /* uncaught... */break; }
#include "machine.def"

#undef MYDEFINST
#undef MYDEFLINK

   default:
#ifdef TARGET_PISA // jdonald: fixing PISA problem of 64-bit instructions
		fprintf(stderr, "pc %x inst %x%x\n",  regs.regs_PC, inst.a, inst.b);
        panic("attempted to execute a bogus opcode %x %x%x", regs.regs_PC, inst.a, inst.b);
#else // TARGET_PPC, TARGET_ARM, TARGET_ALPHA, TARGET_X86
		fprintf(stderr, "pc %x inst %x\n",  regs.regs_PC, inst);
        panic("attempted to execute a bogus opcode %x %x", regs.regs_PC, inst);
#endif // jdonald: fixing PISA problem of 64-bit instructions

   }

/* execute next instruction */
   regs.regs_PC = regs.regs_NPC;
   regs.regs_NPC += sizeof(md_inst_t);
#endif // INTERACTIVE, I think
}

void runstepi() {
#ifdef INTERACTIVE
   register md_inst_t inst;
   register enum md_opcode op;
   char ch;

   op = (enum md_opcode)( __UNCHK_MEM_READ(dec, regs.regs_PC << 1, word_t) );
   inst = (word_t)__UNCHK_MEM_READ(dec, (regs.regs_PC << 1)+sizeof(word_t), word_t);

   switch (op)
   {
#define MYDEFINST(OP) \
     case OP:                   \
       SYMCAT(OP,_IMPL);                 \
       break;
#define MYDEFLINK(OP) \
     case OP:                   \
       panic("attempted to execute a linking opcode");

#if defined(TARGET_PPC) // jdonald: support for PPC, ARM and others
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) \
	MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) \
  	MYDEFLINK(OP)
#elif defined(TARGET_ARM)
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4)      \
	MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,SHIFT,MASK) \
  	MYDEFLINK(OP)
#else // non-PPC/ARM: TARGET_PISA, TARGET_ALPHA, TARGET_X86
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) \
	MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,MASK,SHIFT) \
  	MYDEFLINK(OP)
#endif

#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)                 \
     { /* uncaught... */break; }
#include "machine.def"

#undef MYDEFINST
#undef MYDEFLINK

   default:

     panic("attempted to execute a bogus opcode %x",op);
   }

      /* execute next instruction */
   regs.regs_PC = regs.regs_NPC;
   regs.regs_NPC += sizeof(md_inst_t);
#endif // INTERACTIVE, I think
}

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void) {

  /* register allocate instruction buffer */
  register md_inst_t inst;
  int i;
  int qflag;

  /* decoded opcode */
  register enum md_opcode op;

	for (i = 0; i < MD_MAX_MASK; i++) { found[i] = 0; }
	qflag = 0;
  fprintf(stderr, "sim: ** starting *fast* functional simulation **\n");

  /* must have natural byte/word ordering */
	/*
  if (sim_swap_bytes || sim_swap_words)
    fatal("sim: *fast* functional simulation cannot swap bytes or words");
	*/

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);
//regs.regs_C.cr = 0x84222842;


	/* stuff for interactive debugger */
	
	if (interactive) {
		fprintf(stderr, "INTERACTIVE MODE\n");
		ReaderInit();
		InitializeCommonCommands(TAGGED_FALSE);
	   InitializeInterrupts();
	   CntlcInstallPollingHandler();
		ReadEvalPrintLoop( "ss-ppc> " );
		return;
	}
  while (TRUE)
    {
      /* maintain $r0 semantics */
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#elif defined(TARGET_X86)
      MD_CLEAR_ZEROREGS; // this was found only in ss-x86-sel/sim-fast.c /jdonald
#elif !defined(TARGET_PPC) && !defined(TARGET_ARM)
      regs.regs_R[MD_REG_ZERO] = 0;
#endif

// for x86, for some reason it wants to do the register-zeroing at the end... /jdonald

      /* keep an instruction count */
#ifndef NO_INSN_COUNT
      sim_num_insn++; 
#endif /* !NO_INSN_COUNT */

#if defined(TARGET_PISA)
      /* load instruction */
      inst = __UNCHK_MEM_READ(mem, regs.regs_PC, md_inst_t);

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

#elif defined(TARGET_ALPHA)
      /* load predecoded instruction */

      printf("PC = %016llx: ", regs.regs_PC); // jdonald: leaving this debug info in here. Remove it once the 'JSR' bug in Alpha is fixed
      
      op = (enum md_opcode)__UNCHK_MEM_READ(dec, regs.regs_PC << 1, word_t);
      inst = __UNCHK_MEM_READ(dec, (regs.regs_PC << 1)+sizeof(word_t), md_inst_t);
      
	  md_print_insn(inst, regs.regs_PC, stdout); // jdonald: leaving this debug info in here. Remove it once the 'JSR' bug in Alpha is fixeds
	  printf("\n");                              // ""

#elif defined(TARGET_PPC)
		/* increment time base */
		regs.regs_TBR++;
		
		/* read opcode from predecoded memory segment */	
		// printf("INST 0x%x, PC 0x%x, CR %x\n", inst, regs.regs_PC, regs.regs_C.cr);
		// printf("   ");
		// md_print_insn(inst, regs.regs_PC, stdout);
		// printf("\n");
		// fflush(stdout);

		op = (enum md_opcode)__UNCHK_MEM_READ(dec, regs.regs_PC << 1, word_t);
		// inst = (word_t)__UNCHK_MEM_READ(dec, (regs.regs_PC << 1)+sizeof(word_t), word_t);
     	inst = MD_SWAPW((word_t)__UNCHK_MEM_READ(mem, regs.regs_PC, word_t));
		/*fprintf(stderr, "INST 0x%x, PC 0x%x\n", inst, regs.regs_PC ); */
		/* print inst trace and opcodes */

#elif defined(TARGET_ARM) || defined(TARGET_X86) // in ss-x86-sel/sim-fast.c, x86 uses the same fetch mechanism as PISA/ARM
// but the code doesn't match ours :( /jdonald
      /* load instruction */
      MD_FETCH_INST(inst, mem, regs.regs_PC);

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

#else // no ISA
#error no ISA defined

#endif

	/* execute the instruction */
	switch (op)
	{
#define MYDEFINST(OP) \
    case OP:							\
  	    SYMCAT(OP,_IMPL);				\
	    break;
#define MYDEFLINK(OP) \
     case OP:                   \
       panic("attempted to execute a linking opcode");
	    
#if defined(TARGET_PPC) // jdonald: support for PPC, ARM and others
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) \
	MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) \
  	MYDEFLINK(OP)
#elif defined(TARGET_ARM)
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4)      \
	MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,SHIFT,MASK) \
  	MYDEFLINK(OP)
#elif defined(TARGET_X86)
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4,OFLAGS,IFLAGS)\
    MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
    MYDEFLINK(OP)
#else // non-PPC/ARM/x86: TARGET_PISA, TARGET_ALPHA
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) \
	MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,MASK,SHIFT) \
  	MYDEFLINK(OP)
#endif

#define CONNECT(OP)

#ifdef TARGET_PISA // jdonald: fixing PISA and Alpha problem of 64-bit instructions
  #define DECLARE_FAULT(FAULT)						\
	  { printf("inst %x%x pc %x\n", inst.a, inst.b, regs.regs_PC); panic("fault\n"); break; }
#elif defined(TARGET_ALPHA) // jdonald: Alpha has a 64-bit PC
  #define DECLARE_FAULT(FAULT)						\
	  { printf("inst %x pc %llx\n", inst, regs.regs_PC); panic("fault\n"); break; }
#elif defined(TARGET_X86) // jdonald writing this since x86 instructions can't easiily be fed to printf()
  #define DECLARE_FAULT(FAULT)						\
	  { md_print_insn(inst, regs.regs_PC, stdout); \
	    panic("fault\n"); break; }
#else // TARGET_PPC, TARGET_ARM, TARGET_X86
  #define DECLARE_FAULT(FAULT)						\
	  { printf("inst %x pc %x\n", inst, regs.regs_PC); panic("fault\n"); break; }
#endif // jdonald: fixing PISA and Alpha problem of 64-bit instructions

#include "machine.def"

#undef MYDEFINST
#undef MYDEFLINK

	default:
	  panic("attempted to execute a bogus opcode %x %x", regs.regs_PC, inst);
	}
		found[(int) op]++;							\

      /* execute next instruction */
      regs.regs_PC = regs.regs_NPC;
      regs.regs_NPC += sizeof(md_inst_t);
    }
			
}
