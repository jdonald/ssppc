/*
 * sim-eio.c - external I/O trace generator
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
 * $Id: sim-eio.c,v 1.1.1.1 2001/08/10 14:58:22 karu Exp $
 *
 * $Log: sim-eio.c,v $
 * Revision 1.1.1.1  2001/08/10 14:58:22  karu
 * creating ss3ppc-linux repository. The only difference between this and
 * the original ss3ppc linux is that crosss endian issues are taken
 * care of here.
 *
 *
 * Revision 1.1.1.1  2001/08/08 21:28:53  karu
 * creating ss3ppc linux repository
 *
 * Revision 1.5  2001/08/03 06:12:26  karu
 * added support for EIO traces. checkpointing still does not seem to work
 *
 * Revision 1.4  2001/06/08 00:47:03  karu
 * fixed sim-cheetah. eio does not work because i am still not sure how much state to commit for system calls.
 *
 * Revision 1.3  2001/06/05 09:41:01  karu
 * removed warnings and fixed sim-profile. there is still an addressing mode problem unresolved in sim-profile. sim-eio has some system call problems unresolved.
 *
 * Revision 1.2  2000/04/10 23:46:39  karu
 * converted all quad to qword. NO OTHER change made.
 * the tag specfp95-before-quad-qword is a snapshow before this change was made
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
 * Revision 1.1  1998/08/27 16:24:56  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "eio.h"
#include "range.h"
#include "sim.h"

/*
 * This file implements a functional simulator.  This functional simulator is
 * the simplest, most user-friendly simulator in the simplescalar tool set.
 * Unlike sim-fast, this functional simulator checks for all instruction
 * errors, and the implementation is crafted for clarity rather than speed.
 */

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

#if defined(TARGET_ALPHA) || defined(TARGET_PPC) // jdonald adds Alpha
/* predecoded text memory */
static struct mem_t *dec = NULL;
#endif

/* track number of refs */
static counter_t sim_num_refs = 0;

/* maximum number of inst's to execute */
static unsigned int max_insts;

/* number of insts skipped before timing starts */
// static int fastfwd_count;
static sqword_t fastfwd_count; // jdonald changed to 64-bit integer

/* non-zero when fastforward'ing */
static int fastfwding = FALSE;

/* external I/O filename */
static char *trace_fname;
static FILE *trace_fd = NULL;

/* checkpoint filename and file descriptor */
static enum { no_chkpt, one_shot_chkpt, periodic_chkpt } chkpt_kind = no_chkpt;
static int chkpt_active = FALSE; // jdonald reenabled
static char *chkpt_fname;
static FILE *chkpt_fd = NULL;
static struct range_range_t chkpt_range;

/* periodic checkpoint args */
static counter_t per_chkpt_interval;
static counter_t next_chkpt_cycle;
static unsigned int chkpt_num;

/* checkpoint output filename and range */
static int chkpt_nelt = 0;
static char *chkpt_opts[2];

/* periodic checkpoint output filename and range */
static int per_chkpt_nelt = 0;
static char *per_chkpt_opts[2];


/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-eio: This simulator implements simulator support for generating\n"
"external event traces (EIO traces) and checkpoint files.  External\n"
"event traces capture one execution of a program, and allow it to be\n"
"packaged into a single file for later re-execution.  EIO trace executions\n"
"are 100% reproducible between subsequent executions (on the same platform.\n"
"This simulator also provides functionality to generate checkpoints at\n"
"arbitrary points within an external event trace (EIO) execution.  The\n"
"checkpoint file (along with the EIO trace) can be used to start any\n"
"SimpleScalar simulator in the middle of a program execution.\n"
		 );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

  opt_reg_sqword(odb, "-fastfwd", "number of insts to execute in fast fwd mode before timing simulation starts",
	      &fastfwd_count, /* default */0,
	      /* print */TRUE, /* format */NULL); // jdonald changed to 64-bit integer

  opt_reg_string(odb, "-trace", "EIO trace file output file name",
		 &trace_fname, /* default */NULL,
		 /* print */TRUE, NULL);

  opt_reg_string_list(odb, "-perdump",
					  "periodic checkpoint every n instructions: "
					  "<base fname> <interval>",
					  per_chkpt_opts, /* sz */2, &per_chkpt_nelt,
					  /* default */NULL,
					  /* !print */FALSE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_string_list(odb, "-dump",
		      "specify checkpoint file and trigger: <fname> <range>",
		      chkpt_opts, /* sz */2, &chkpt_nelt, /* default */NULL,
		      /* !print */FALSE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_note(odb,
"  Checkpoint range triggers are formatted as follows:\n"
"\n"
"    {{@|#}<start>}:{{@|#|+}<end>}\n"
"\n"
"  Both ends of the range are optional, if neither are specified, the range\n"
"  triggers immediately.  Ranges that start with a `@' designate an address\n"
"  range to trigger on, those that start with an `#' designate a cycle count\n"
"  trigger.  All other ranges represent an instruction count range.  The\n"
"  second argument, if specified with a `+', indicates a value relative\n"
"  to the first argument, e.g., 1000:+100 == 1000:1100.\n"
"\n"
"    Examples:   -dump FOO.chkpt #0:#1000\n" // jdonald fixing -ptrace to say -dump
"                -dump BAR.chkpt @2000:\n"
"                -dump BLAH.chkpt :1500\n"
"                -dump UXXE.chkpt :\n"
	       );}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  // if (fastfwd_count < 0 || fastfwd_count >= 2147483647)
  if (fastfwd_count < 0) // jdonald, upgrade fastfwd_count to 64-bit
    fatal("bad fast forward count: %d", fastfwd_count);
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);
  ld_reg_stats(sdb);
  mem_reg_stats(mem, sdb);
}

/* initialize the simulator */
void
sim_init(void)
{
  sim_num_refs = 0;

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

#ifdef TARGET_PPC
#include "target-ppc/predecode.h"
#endif

  if (chkpt_nelt == 2)
    {
      char *errstr;

      /* generate a checkpoint */
	  /*
      if (!sim_eio_fd)
	fatal("checkpoints can only be generated while running an EIO trace");
	  */

	  /* can't do regular & periodic chkpts at the same time */
	  if (per_chkpt_nelt != 0)
		fatal("can't do both regular and periodic checkpoints");

#if 0 /* this should work fine... */
      if (trace_fname != NULL)
	fatal("checkpoints cannot be generated with generating an EIO trace");
#endif

      /* parse the range */
      errstr = range_parse_range(chkpt_opts[1], &chkpt_range);
      if (errstr)
	fatal("cannot parse pipetrace range, use: {<start>}:{<end>}");

      /* create the checkpoint file */
      chkpt_fname = chkpt_opts[0];
      chkpt_fd = eio_create(chkpt_fname);

      /* indicate checkpointing is now active... */
      //chkpt_active = TRUE; // jdonald saves this var for until the file is actually created
	  chkpt_kind = one_shot_chkpt;
    }

  if (per_chkpt_nelt == 2)
    {
      chkpt_fname = per_chkpt_opts[0];
      if (strchr(chkpt_fname, '%') == NULL)
        fatal("periodic checkpoint filename must be printf-style format");
	  
	  per_chkpt_interval = 0;
      if (sscanf(per_chkpt_opts[1], "%lld", &per_chkpt_interval) != 1)
        fatal("can't parse periodic checkpoint interval '%s'",
              per_chkpt_opts[1]);
	  
      /* indicate checkpointing is now active... */
      chkpt_kind = periodic_chkpt;
      chkpt_num = 1;
      next_chkpt_cycle = per_chkpt_interval;
    }
  
  

  if (trace_fname != NULL)
    {
      fprintf(stderr, "sim: tracing execution to EIO file `%s'...\n",
	      trace_fname);

      /* create an EIO trace file */
      trace_fd = eio_create(trace_fname);
    }

  /* initialize the DLite debugger */
  dlite_init(md_reg_obj, dlite_mem_obj, dlite_mstate_obj);
}


/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
  /* nada */
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  if (trace_fd != NULL)
    eio_close(trace_fd);
}


/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
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
#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, MEM_READ_BYTE(mem, addr = (SRC)))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, MEM_READ_HALF(mem, addr = (SRC)))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, MEM_READ_WORD(mem, addr = (SRC)))
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, MEM_READ_QWORD(mem, addr = (SRC)))
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, MEM_WRITE_BYTE(mem, addr = (DST), (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, MEM_WRITE_HALF(mem, addr = (DST), (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, MEM_WRITE_WORD(mem, addr = (DST), (SRC)))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, MEM_WRITE_QWORD(mem, addr = (DST), (SRC)))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#ifdef TARGET_PPC

#define SC_EIO \
	 if (PPC_GPR(2) ==6) return;             \
     eio_write_trace(trace_fd, sim_num_insn, \
		     &regs, mem_access, mem, inst);  \
     SET_NPC(LR);

#define SYSCALL							\
  if (trace_fd != NULL && !fastfwding)					\
{ SC_EIO; } else { SCACTUAL_IMPL; }
  
#else
#define SYSCALL(INST)							\
  ((trace_fd != NULL && !fastfwding)					\
   ? eio_write_trace(trace_fd, sim_num_insn,				\
		     &regs, mem_access, mem, INST)			\
   : sys_syscall(&regs, mem_access, mem, INST, TRUE))
#endif

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  md_inst_t inst;
  register md_addr_t addr;
  enum md_opcode op;
  register bool_t is_write;
  enum md_fault_type fault;

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs.regs_PC, /* !access */0, /* addr */0, 0, 0))
    dlite_main(regs.regs_PC - sizeof(md_inst_t), regs.regs_PC,
	       sim_num_insn, &regs, mem);

  /* fast forward simulator loop, performs functional simulation for
     FASTFWD_COUNT insts, then turns on performance (timing) simulation */
  if (fastfwd_count > 0)
    {
      int icount;

      fprintf(stderr, "sim: ** fast forwarding %lld insts **\n", fastfwd_count);

      fastfwding = TRUE;
      for (icount=0; icount < fastfwd_count; icount++)
	{
      /* maintain $r0 semantics */
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#elif defined(TARGET_X86)
      MD_CLEAR_ZEROREGS;
#elif !defined(TARGET_PPC) && !defined(TARGET_ARM)
      regs.regs_R[MD_REG_ZERO] = 0;
#endif

      /* load instruction */
#if defined(TARGET_PISA)
      inst = __UNCHK_MEM_READ(mem, regs.regs_PC, md_inst_t);
#elif defined(TARGET_ALPHA)
      inst = __UNCHK_MEM_READ(dec, (regs.regs_PC << 1)+sizeof(word_t), md_inst_t);
#elif defined(TARGET_PPC)
	  inst = MD_SWAPW((word_t)__UNCHK_MEM_READ(mem, regs.regs_PC, md_inst_t)); // jdonald
	  // inst = __UNCHK_MEM_READ(mem, regs.regs_PC, md_inst_t);
#elif defined(TARGET_ARM) || defined(TARGET_X86)
	  MD_FETCH_INST(inst, mem, regs.regs_PC);
#endif

	  /* set default reference address */
	  addr = 0; is_write = FALSE;

	  /* set default fault - none */
	  fault = md_fault_none;

	  /* decode the instruction */
	  MD_SET_OPCODE(op, inst);

	  /* execute the instruction */
	  switch (op)
	    {
#define MYDEFINST(OP)\
	    case OP:							\
	      SYMCAT(OP,_IMPL);						\
	      break;
#define MYDEFLINK(OP)					\
	    case OP:							\
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
#undef DECLARE_FAULT
#define DECLARE_FAULT(FAULT)						\
	      { fault = (FAULT); break; }
#include "machine.def"

#undef DEFINST
#undef DEFLINK

	    default:
	      panic("attempted to execute a bogus opcode");
	    }

	  if (fault != md_fault_none)
	    fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

	  /* update memory access stats */
	  if (MD_OP_FLAGS(op) & F_MEM)
	    {
	      if (MD_OP_FLAGS(op) & F_STORE)
		is_write = TRUE;
	    }

	  /* check for DLite debugger entry condition */
	  if (dlite_check_break(regs.regs_NPC,
				is_write ? ACCESS_WRITE : ACCESS_READ,
				addr, sim_num_insn, sim_num_insn))
	    dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem);

	  /* go to the next instruction */
	  regs.regs_PC = regs.regs_NPC;
	  regs.regs_NPC += sizeof(md_inst_t);
	}
    }
  fastfwding = FALSE;

  if (trace_fd != NULL)
    {
      fprintf(stderr, "sim: writing EIO file initial checkpoint...\n");
      if (eio_write_chkpt(&regs, mem, trace_fd) != -1)
	panic("checkpoint code is broken");
    }

  fprintf(stderr, "sim: ** starting functional simulation **\n");

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

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

      /* check if checkpoint should be generated here... */
    if (chkpt_kind == one_shot_chkpt
      && !chkpt_active // jdonald
	  && !range_cmp_range1(&chkpt_range, regs.regs_NPC,
			       sim_num_insn, sim_num_insn))
	{
	  fprintf(stderr, "sim: writing checkpoint file `%s' @ inst %.0f...\n",
		  chkpt_fname, (double)sim_num_insn);

	  /* write the checkpoint file */
	  eio_write_chkpt(&regs, mem, chkpt_fd);

	  /* close the checkpoint file */
	  eio_close(chkpt_fd);
	  chkpt_active = TRUE;
	  if (trace_fname == NULL) // if we're using (rather than outputing) an eio, end now
  		  longjmp(sim_exit_buf, /* exitcode + fudge */0+1);
	}
	else if (chkpt_kind == one_shot_chkpt
	  && chkpt_active
	  && range_cmp_range1(&chkpt_range, regs.regs_NPC,
	  				sim_num_insn, sim_num_insn))
	// jdonald added this to enable convenient eio tracing and checkpoint generation in one shot
	{
	  /* exit jumps to the target set in main() */
	  // as I understand it, this substitutes -max:inst in a sense /jdonald
	  longjmp(sim_exit_buf, /* exitcode + fudge */0+1);
	  chkpt_active = FALSE; // shouldn't reach here
	}
	else if (chkpt_kind == periodic_chkpt 
			   && sim_num_insn == next_chkpt_cycle) {
	  char this_chkpt_fname[256];
	  
	  /* 'chkpt_fname' should be a printf format string */
	  sprintf(this_chkpt_fname, chkpt_fname, chkpt_num);
	  chkpt_fd = eio_create(this_chkpt_fname);
	  
	  myfprintf(stderr, "sim: writing checkpoint file `%s' @ inst %n...\n",
				this_chkpt_fname, sim_num_insn);
	  
	  /* write the checkpoint file */
	  eio_write_chkpt(&regs, mem, chkpt_fd);
	  
	  /* close the checkpoint file */
	  eio_close(chkpt_fd);
	  
	  chkpt_num++;
	  next_chkpt_cycle += per_chkpt_interval;
	}

      /* get the next instruction to execute */
#if defined(TARGET_PISA)
      inst = __UNCHK_MEM_READ(mem, regs.regs_PC, md_inst_t);
#elif defined(TARGET_ALPHA)
      inst = __UNCHK_MEM_READ(dec, (regs.regs_PC << 1)+sizeof(word_t), md_inst_t);
#elif defined(TARGET_PPC)
	  inst = MD_SWAPW((word_t)__UNCHK_MEM_READ(mem, regs.regs_PC, md_inst_t)); // jdonald
	  // inst = __UNCHK_MEM_READ(mem, regs.regs_PC, md_inst_t);
#elif defined(TARGET_ARM) || defined(TARGET_X86)
	  MD_FETCH_INST(inst, mem, regs.regs_PC);
#endif

	  
      /* keep an instruction count */
      sim_num_insn++;

      /* set default reference address and access mode */
      addr = 0; is_write = FALSE;

      /* set default fault - none */
      fault = md_fault_none;

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

      /* execute the instruction */
      switch (op)
	{
#define MYDEFINST(OP) \
	case OP:							\
          SYMCAT(OP,_IMPL);						\
          break;
#define MYDEFLINK(OP)					\
        case OP:							\
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
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }
#include "machine.def"

#undef DEFINST
#undef DEFLINK

	default:
	  panic("bogus opcode");
      }

      if (fault != md_fault_none)
	fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

      if (MD_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (MD_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}

      /* check for DLite debugger entry condition */
      if (dlite_check_break(regs.regs_NPC,
			    is_write ? ACCESS_WRITE : ACCESS_READ,
			    addr, sim_num_insn, sim_num_insn))
	dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem);

      /* go to the next instruction */
      regs.regs_PC = regs.regs_NPC;
      regs.regs_NPC += sizeof(md_inst_t);

      /* finish early? */
      if (max_insts && sim_num_insn >= max_insts)
	return;
    }
}
