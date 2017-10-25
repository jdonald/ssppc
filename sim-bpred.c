/*
 * sim-bpred.c - sample branch predictor simulator implementation
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
 * $Id: sim-bpred.c,v 1.1.1.1 2001/08/10 14:58:22 karu Exp $
 *
 * $Log: sim-bpred.c,v $
 * Revision 1.1.1.1  2001/08/10 14:58:22  karu
 * creating ss3ppc-linux repository. The only difference between this and
 * the original ss3ppc linux is that crosss endian issues are taken
 * care of here.
 *
 *
 * Revision 1.1.1.1  2001/08/08 21:28:53  karu
 * creating ss3ppc linux repository
 *
 * Revision 1.4  2001/06/05 09:41:01  karu
 * removed warnings and fixed sim-profile. there is still an addressing mode problem unresolved in sim-profile. sim-eio has some system call problems unresolved.
 *
 * Revision 1.3  2000/04/14 04:55:06  karu
 * sim-bpred works now
 *
 * Revision 1.2  2000/04/10 23:46:35  karu
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
 * Revision 1.1  1998/08/27 15:54:35  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h> // jdonald

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
#include "bpred.h"
#include "sim.h"

/*
 * This file implements a branch predictor analyzer.
 */

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

#ifdef TARGET_PPC
static struct mem_t *dec = NULL;
#endif

/* maximum number of inst's to execute */
static unsigned int max_insts;

/* branch predictor type {nottaken|taken|perfect|bimod|2lev} */
static char *pred_type;

/* bimodal predictor config (<table_size>) */
static int bimod_nelt = 1;
static int bimod_config[1] =
  { /* bimod tbl size */2048 };

/* 2-level predictor config (<l1size> <l2size> <hist_size> <xor>) */
static int twolev_nelt = 4;
static int twolev_config[4] =
  { /* l1size */1, /* l2size */1024, /* hist */8, /* xor */FALSE};

/* combining predictor config (<meta_table_size> */
static int comb_nelt = 1;
static int comb_config[1] =
  { /* meta_table_size */1024 };

/* return address stack (RAS) size */
static int ras_size = 8;

/* BTB predictor config (<num_sets> <associativity>) */
static int btb_nelt = 2;
static int btb_config[2] =
  { /* nsets */512, /* assoc */4 };

/* branch predictor */
static struct bpred_t *pred;

/* track number of insn and refs */
static counter_t sim_num_refs = 0;

/* total number of branches executed */
static counter_t sim_num_branches = 0;


/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-bpred: This simulator implements a branch predictor analyzer.\n"
		 );

  /* branch predictor options */
  opt_reg_note(odb,
"  Branch predictor configuration examples for 2-level predictor:\n"
"    Configurations:   N, M, W, X\n"
"      N   # entries in first level (# of shift register(s))\n"
"      W   width of shift register(s)\n"
"      M   # entries in 2nd level (# of counters, or other FSM)\n"
"      X   (yes-1/no-0) xor history and address for 2nd level index\n"
"    Sample predictors:\n"
"      GAg     : 1, W, 2^W, 0\n"
"      GAp     : 1, W, M (M > 2^W), 0\n"
"      PAg     : N, W, 2^W, 0\n"
"      PAp     : N, W, M (M == 2^(N+W)), 0\n"
"      gshare  : 1, W, 2^W, 1\n"
"  Predictor `comb' combines a bimodal and a 2-level predictor.\n"
               );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-bpred",
		 "branch predictor type {nottaken|taken|bimod|2lev|comb}",
                 &pred_type, /* default */"bimod",
                 /* print */TRUE, /* format */NULL);

  opt_reg_int_list(odb, "-bpred:bimod",
		   "bimodal predictor config (<table size>)",
		   bimod_config, bimod_nelt, &bimod_nelt,
		   /* default */bimod_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int_list(odb, "-bpred:2lev",
                   "2-level predictor config "
		   "(<l1size> <l2size> <hist_size> <xor>)",
                   twolev_config, twolev_nelt, &twolev_nelt,
		   /* default */twolev_config,
                   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int_list(odb, "-bpred:comb",
		   "combining predictor config (<meta_table_size>)",
		   comb_config, comb_nelt, &comb_nelt,
		   /* default */comb_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);

  opt_reg_int(odb, "-bpred:ras",
              "return address stack size (0 for no return stack)",
              &ras_size, /* default */ras_size,
              /* print */TRUE, /* format */NULL);

  opt_reg_int_list(odb, "-bpred:btb",
		   "BTB config (<num_sets> <associativity>)",
		   btb_config, btb_nelt, &btb_nelt,
		   /* default */btb_config,
		   /* print */TRUE, /* format */NULL, /* !accrue */FALSE);
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  if (!mystricmp(pred_type, "taken"))
    {
      /* static predictor, not taken */
      pred = bpred_create(BPredTaken, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
  else if (!mystricmp(pred_type, "nottaken"))
    {
      /* static predictor, taken */
      pred = bpred_create(BPredNotTaken, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
  else if (!mystricmp(pred_type, "bimod"))
    {
      if (bimod_nelt != 1)
	fatal("bad bimod predictor config (<table_size>)");
      if (btb_nelt != 2)
	fatal("bad btb config (<num_sets> <associativity>)");

      /* bimodal predictor, bpred_create() checks BTB_SIZE */
      pred = bpred_create(BPred2bit,
			  /* bimod table size */bimod_config[0],
			  /* 2lev l1 size */0,
			  /* 2lev l2 size */0,
			  /* meta table size */0,
			  /* history reg size */0,
			  /* history xor address */0,
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size);
    }
  else if (!mystricmp(pred_type, "2lev"))
    {
      /* 2-level adaptive predictor, bpred_create() checks args */
      if (twolev_nelt != 4)
	fatal("bad 2-level pred config (<l1size> <l2size> <hist_size> <xor>)");
      if (btb_nelt != 2)
	fatal("bad btb config (<num_sets> <associativity>)");

      pred = bpred_create(BPred2Level,
			  /* bimod table size */0,
			  /* 2lev l1 size */twolev_config[0],
			  /* 2lev l2 size */twolev_config[1],
			  /* meta table size */0,
			  /* history reg size */twolev_config[2],
			  /* history xor address */twolev_config[3],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size);
    }
  else if (!mystricmp(pred_type, "comb"))
    {
      /* combining predictor, bpred_create() checks args */
      if (twolev_nelt != 4)
	fatal("bad 2-level pred config (<l1size> <l2size> <hist_size> <xor>)");
      if (bimod_nelt != 1)
	fatal("bad bimod predictor config (<table_size>)");
      if (comb_nelt != 1)
	fatal("bad combining predictor config (<meta_table_size>)");
      if (btb_nelt != 2)
	fatal("bad btb config (<num_sets> <associativity>)");

      pred = bpred_create(BPredComb,
			  /* bimod table size */bimod_config[0],
			  /* l1 size */twolev_config[0],
			  /* l2 size */twolev_config[1],
			  /* meta table size */comb_config[0],
			  /* history reg size */twolev_config[2],
			  /* history xor address */twolev_config[3],
			  /* btb sets */btb_config[0],
			  /* btb assoc */btb_config[1],
			  /* ret-addr stack size */ras_size);
    }
  else
    fatal("cannot parse predictor type `%s'", pred_type);
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

  stat_reg_counter(sdb, "sim_num_branches",
                   "total number of branches executed",
                   &sim_num_branches, /* initial value */0, /* format */NULL);
  stat_reg_formula(sdb, "sim_IPB",
                   "instruction per branch",
                   "sim_num_insn / sim_num_branches", /* format */NULL);

  /* register predictor stats */
  if (pred)
    bpred_reg_stats(pred, sdb);
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

/* local machine state accessor */
static char *					/* err str, NULL for no err */
bpred_mstate_obj(FILE *stream,			/* output stream */
		 char *cmd,			/* optional command string */
		 struct regs_t *regs,		/* register to access */
		 struct mem_t *mem)		/* memory to access */
{
  /* just dump intermediate stats */
  sim_print_stats(stream);

  /* no error */
  return NULL;
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
  /* initialize the DLite debugger */
  dlite_init(md_reg_obj, dlite_mem_obj, bpred_mstate_obj);
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
  /* nada */
}


/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
 */

/* next program counter */
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* target program counter */
#undef  SET_TPC
#define SET_TPC(EXPR)		(target_PC = (EXPR))

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

/* precise architected memory state help functions */
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
#define SYSCALL SCACTUAL_IMPL
#else
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)
#endif
/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  md_inst_t inst;
  register md_addr_t addr, target_PC = -1; // jdonald set default invalid value for target_PC
  enum md_opcode op;
  register int is_write;
  int stack_idx;
  enum md_fault_type fault;

  fprintf(stderr, "sim: ** starting functional simulation w/ predictors **\n");

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs.regs_PC, /* no access */0, /* addr */0, 0, 0))
    dlite_main(regs.regs_PC - sizeof(md_inst_t), regs.regs_PC,
	       sim_num_insn, &regs, mem);

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

      /* get the next instruction to execute */
      mem_access(mem, Read, regs.regs_PC, &inst, sizeof(md_inst_t));

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
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }
#include "machine.def"

#undef MYDEFINST
#undef MYDEFLINK

	default:
	  panic("attempted to execute a bogus opcode");
      }

      if (fault != md_fault_none)
	fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

      if (MD_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (MD_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}

      if (MD_OP_FLAGS(op) & F_CTRL)
	{
	  md_addr_t pred_PC;
	  struct bpred_update_t update_rec;

	  sim_num_branches++;

	  if (pred)
	    {
		  assert(target_PC != -1); // jdonald putting this check to heed the compiler warning
	      /* get the next predicted fetch address */
	      pred_PC = bpred_lookup(pred,
				     /* branch addr */regs.regs_PC,
				     /* target */target_PC,
				     /* inst opcode */op,
				     /* call? */MD_IS_CALL(op),
				     /* return? */MD_IS_RETURN(op),
				     /* stash an update ptr */&update_rec,
				     /* stash return stack ptr */&stack_idx);

	      /* valid address returned from branch predictor? */
	      if (!pred_PC)
		{
		  /* no predicted taken target, attempt not taken target */
		  pred_PC = regs.regs_PC + sizeof(md_inst_t);
		}

	      bpred_update(pred,
			   /* branch addr */regs.regs_PC,
			   /* resolved branch target */regs.regs_NPC,
			   /* taken? */regs.regs_NPC != (regs.regs_PC +
							 sizeof(md_inst_t)),
			   /* pred taken? */pred_PC != (regs.regs_PC +
							sizeof(md_inst_t)),
			   /* correct pred? */pred_PC == regs.regs_NPC,
			   /* opcode */op,
			   /* predictor update pointer */&update_rec);
	    }
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
