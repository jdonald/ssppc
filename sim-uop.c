/* sim-uop.c - functional simulator implementation with microcode */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2002 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 2000-2002 by The Regents of The University of Michigan.
 * Copyright (C) 1994-2002 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
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
#include "sim.h"

/*
 * This file implements a functional simulator. It is similar to sim-safe but
 * can understand microcode. */

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

#if defined(TARGET_ALPHA) || defined(TARGET_PPC) // jdonald combined these two to one #if
/* predecoded text memory */
static struct mem_t *dec = NULL;
#endif

/* track number of UOPs executed */
static counter_t sim_num_uops = 0;

/* track number of refs */
static counter_t sim_num_refs = 0;

/* total number of loads */
static counter_t sim_num_loads = 0;

/* total number of branches */
static counter_t sim_num_branches = 0;

/* maximum number of inst's to execute */
static unsigned int max_insts;

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-uop: This simulator implements a functional simulator.  It\n"
"is similar to sim-safe but it can process microcode in addition to normal\n"
"instructions\n"
  );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);
}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  /* nada */
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_num_uops",
	   "total number of UOPs executed",
		   &sim_num_uops, sim_num_uops, NULL);
  stat_reg_formula(sdb, "sim_avg_flowlen",
		   "uops per instruction",
		   "sim_num_uops / sim_num_insn", NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
  stat_reg_counter(sdb, "sim_num_loads",
                   "total number of loads committed",
                   &sim_num_loads, 0, NULL);
  stat_reg_formula(sdb, "sim_num_stores",
                   "total number of stores committed",
                   "sim_num_refs - sim_num_loads", NULL);
  stat_reg_counter(sdb, "sim_num_branches",
                   "total number of branches committed",
                   &sim_num_branches, 0, NULL);

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

/* current program counter */
#define CPC			(regs.regs_PC)

/* general purpose registers */

#if defined(TARGET_PISA)

#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))
// hmm, this may be cleaner to keep GPR and SET_GPR in the specific TARGET_* areas
// different from sim-fast.c and other setups

/* floating point registers */
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

#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

/* floating point registers */
#define FPR_Q(N)		(regs.regs_F.q[N])
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#define FPR(N)			(regs.regs_F.d[(N)])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#define FPCR			(regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#define UNIQ			(regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

#elif defined(TARGET_PPC) // jdonald

#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

#elif defined(TARGET_ARM)

#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)							\
  ((void)(((N) == 15) ? setPC++ : 0), regs.regs_R[N] = (EXPR))

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
#define DTOW(D)	({ union x fw; fw.f = (float)(D); fw.w; })
#define WTOD(W)	({ union x fw; fw.w = (W); (double)fw.f; })
#define QSWP(Q)								\
  ((((Q) << 32) & ULL(0xffffffff00000000)) | (((Q) >> 32) & ULL(0xffffffff)))

/* floating point registers */
#define FPR_Q(N)		(QSWP(regs.regs_F.q[N]))
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = QSWP((EXPR)))
#define FPR_W(N)		(DTOW(regs.regs_F.d[N]))
#define SET_FPR_W(N,EXPR)	(regs.regs_F.d[N] = (WTOD(EXPR)))
#define FPR(N)			(regs.regs_F.d[(N)])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#define FPSR			(regs.regs_C.fpsr)
#define SET_FPSR(EXPR)		(regs.regs_C.fpsr = (EXPR))

#elif defined(TARGET_X86)
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

// strangely: the following to macros were missing from sim-uop.c,
// but they were in sim-fast.c and sim-outorder.c /jdonald
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

#else /* !TARGET_X86 */

#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_BYTE(mem, (SRC)))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_HALF(mem, (SRC)))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_WORD(mem, (SRC)))
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_QWORD(mem, (SRC)))

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_BYTE(mem, (DST), (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_HALF(mem, (DST), (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_WORD(mem, (DST), (SRC)))
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_QWORD(mem, (DST), (SRC)))

#endif /* !TARGET_X86 */

/* system call handler macro */
#ifdef TARGET_PPC // jdonald: moved these two to the same spot
  #define SYSCALL SCACTUAL_IMPL
#else
  #define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)
#endif

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  md_inst_t inst;
  register md_addr_t addr;
  enum md_opcode op;
  register int is_write;
  enum md_fault_type fault;
  int setPC;

  int pred_true;
  int in_flow = FALSE;		/* set if we're in ucode flow */
#if defined(TARGET_ARM) || defined(TARGET_X86)
  int nflow;			/* number of uops in current flow */
  int flow_index;		/* index into flowtab */
  md_uop_t flowtab[MD_MAX_FLOWLEN];	/* table of uops */
  md_uop_t *uop;
#endif

  int block_execute = 0;
  
  myfprintf(stderr, "sim: ** starting functional simulation **\n");

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs.regs_PC, /* !access */0, /* addr */0, 0, 0))
    dlite_main(regs.regs_PC - sizeof(md_inst_t),
	       regs.regs_PC, sim_num_insn, &regs, mem);

#if !defined(TARGET_ARM) && !defined(TARGET_X86) // jdonald
  myfprintf(stderr, "warning: For this particular ISA, sim-uop isn't different from sim-fast\n");
#endif

  while (TRUE)
    {
      /* maintain $r0 semantics */
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#elif defined(TARGET_X86)
      MD_CLEAR_ZEROREGS;
#elif !defined(TARGET_PPC) && !defined(TARGET_ARM)
      regs.regs_R[MD_REG_ZERO] = 0;
#endif

// oddly this is a bit different from the current zero-clearing configuration in sim-fast.c... /jdonald

      /* set defaults */
      addr = 0;
      is_write = FALSE;
      setPC = FALSE;
      pred_true = TRUE;
      fault = md_fault_none;
      block_execute = FALSE;

      /* if not in microcode flow, get next instruction */
      if (!in_flow)
	{
#if defined(TARGET_PISA)
      /* load instruction */
      inst = __UNCHK_MEM_READ(mem, regs.regs_PC, md_inst_t);
      MD_SET_OPCODE(op, inst);

#elif defined(TARGET_ALPHA)
      /* load predecoded instruction */
      op = (enum md_opcode)__UNCHK_MEM_READ(dec, regs.regs_PC << 1, word_t);
      inst = __UNCHK_MEM_READ(dec, (regs.regs_PC << 1)+sizeof(word_t), md_inst_t);
#elif defined(TARGET_PPC)
		/* increment time base */
		regs.regs_TBR++;
		
		op = (enum md_opcode)__UNCHK_MEM_READ(dec, regs.regs_PC << 1, word_t);
     	inst = MD_SWAPW((word_t)__UNCHK_MEM_READ(mem, regs.regs_PC, word_t));
#elif defined(TARGET_X86) || defined(TARGET_ARM)
	  /* get the next instruction to execute */
	  MD_FETCH_INST(inst, mem, regs.regs_PC);

	  /* decode the instruction */
	  MD_SET_OPCODE(op, inst);
#endif
	  
	  /* set up initial default next PC */
	  // jdonald: hmm simplesim-arm/sim-uop.c sets the NPC at an entirely different place.
	  // maybe this has no chance of working?
#ifdef TARGET_X86 // apparently necessary because the x86 instruction structure holds a lot of stuff /jdonald
	  regs.regs_NPC = regs.regs_PC + MD_INST_SIZE(inst);
#else
	  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);
#endif

// !in_flow inner section /jdonald
#ifdef TARGET_ARM // jdonald: doing ARM and x86 microcode separately since the code is kind of different
          if (MD_OP_FLAGS(op) & F_CISC)
            {
              /* get instruction flow */
              nflow = md_get_flow(op, inst, flowtab);
              if (nflow > 0)
                {
                  in_flow = TRUE;
                  flow_index = 0;
                }
              else
                fatal("could not locate CISC flow");
              sim_num_uops += nflow;
            }
          else
            sim_num_uops++;
#elif defined(TARGET_X86) // x86 !in_flow inner section
	  /* if microcode instruction, get the flow */
	  if (MD_OP_FLAGS(op) & F_UCODE)
	    {
	      /* get instruction flow */
	      nflow = md_get_flow(op, &inst, flowtab);
	      if (nflow > 0)
		{
		  in_flow = TRUE;
		  flow_index = 0;
		}
	      else
		fatal("could not locate UCODE flow");
	    }
	  
	  /* check for insts with repeat count of 0 */
	  if(!REP_FIRST){
	    block_execute = TRUE;
	    in_flow = FALSE;
	  }
#endif // TARGET_X86 /jdonald
        }
        
#ifdef TARGET_ARM // jdonald: in_flow section
      if (in_flow)
        {
          op = flowtab[flow_index].op;
          inst = flowtab[flow_index++].inst;
          if (flow_index == nflow)
            in_flow = FALSE;
        }

      if (op == NA)
        panic("bogus opcode detected @ 0x%08p", regs.regs_PC);
      if (MD_OP_FLAGS(op) & F_CISC)
        panic("CISC opcode decoded");

      setPC = 0;
      regs.regs_R[MD_REG_PC] = regs.regs_PC; // this routine is ARM-only, which is the only one that uses MD_REG_PC
    
#elif defined(TARGET_X86) // just doing the ARM and x86 cases separately /jdonald
	  /* If we have a microcode op, get the op and inst, this
       * has already been done for non-microcode instructions */
      if (in_flow)
	{
	  op = MD_GET_UOP(flowtab,flow_index);
	  /* check for completed flow */
	  flow_index += MD_INC_FLOW;
	  if(flow_index >= nflow)
	    in_flow = FALSE;
	}

      if(!block_execute && op != MD_NOP_OP){

	/* sanity check: ucode generating insts should not get here */
	if (MD_OP_FLAGS(op) & F_UCODE) panic("UCODE opcode decoded");

	/* check for predicate false instructions: ARM only */
	pred_true = MD_CHECK_PRED(op,inst);

	/* Update stats */
	if (op != MD_NOP_OP && pred_true) {
	  sim_num_uops++;
	  if (!in_flow) sim_num_insn++;
	  if (MD_OP_FLAGS(op) & F_CTRL) sim_num_branches++;
	  if (MD_OP_FLAGS(op) & F_MEM) 
	    {
	      sim_num_refs++;
	      if (MD_OP_FLAGS(op) & F_STORE)
		is_write = TRUE;
	      else
		sim_num_loads++;
	    }
	}
#endif // TARGET_X86

	/* execute the instruction */
	switch (op)
	  {
		  
#define MYDEFINST(OP) \
     case OP:                   \
     	 SYMCAT(OP,_IMPL);                 \
     break;
#define MYDEFLINK(OP) \
     case OP:                   \
       panic("attempted to execute a linking opcode");
#define MYDEFUOP(OP) \
	case OP:							\
          SYMCAT(OP,_IMPL);						\
          break;

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
  #define DEFUOP(OP,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4)\
  	MYDEFUOP(OP)
#elif defined(TARGET_X86)
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4,OFLAGS,IFLAGS)\
    MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
    MYDEFLINK(OP)
  #define DEFUOP(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4,OFLAGS,IFLAGS)\
  	MYDEFUOP(OP)
#define DEFUOP(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,O3,I1,I2,I3,I4,OFLAGS,IFLAGS)\
  	MYDEFUOP(OP)
#else // non-PPC/ARM/x86: TARGET_PISA, TARGET_ALPHA
  #define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3) \
	MYDEFINST(OP)
  #define DEFLINK(OP,MSK,NAME,MASK,SHIFT) \
  	MYDEFLINK(OP)
#endif // jdonald: support for PPC, ARM and others

#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }
#include "machine.def"

#undef MYDEFINST
#undef MYDEFLINK
#undef MYDEFUOP

	  default:
	    panic("attempted to execute a bogus opcode");
	  }
	
	if (fault != md_fault_none)
	  fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

	/* Did instruction write to the PC register? ARM only */
	if (setPC) {
#ifdef TARGET_ARM
	  regs.regs_NPC = GPR(MD_REG_PC);
#else
	  regs.regs_NPC = regs.regs_PC;
	  panic("setPC operation should only be on ARM architectures");
#endif
	}

#ifdef TARGET_X86 // pred_true, debug_trace, REP_COUNT, and such seem to only be for x86 /jdonald
	/* NOP'ify all predicate-false instructions */
	if (!pred_true) 
	  op = MD_NOP_OP;
	
	/* print retirement trace if requested */
	if (debug_trace && pred_true &&
	    (sim_num_insn >= debug_count)  &&
	    (!max_insts || (sim_num_insn <= max_insts)))
	  {
	    myfprintf(stderr, "COMMIT: %10n %10n [xor: 0x%08x] @ 0x%08p: ",
		      (in_flow) ? sim_num_insn + 1 : sim_num_insn,
		      sim_num_uops, md_xor_regs(&regs), regs.regs_PC);
	    md_print_uop(op, inst, regs.regs_PC, stderr);
	    fprintf(stderr, "\n");
	    
	    /* debug_trace for register file */
	    if (debug_regs)
	      {
		md_print_iregs(regs.regs_R, stderr);
		md_print_fpregs(regs.regs_F, stderr);
		md_print_cregs(regs.regs_C, stderr);
		fprintf(stderr, "\n");
	      }
	  }

	/* mark repeat iteration for repeating instructions */
	if (!in_flow){
	  REP_COUNT;
	  /* check for additional repeats */
	  if(REP_AGAIN)
	  {
	  	sim_num_insn--; // skumar
	    regs.regs_NPC = regs.regs_PC;
	   }
	}
#endif // TARGET_X86 pred_true, debug_trace, REP_COUNT stuff /jdonald
 	
#ifdef TARGET_X86
      } /* if !block_execute */
#endif // TARGET_X86. jdonald: hairy conditional ending brace

      /* check for DLite debugger entry condition */
      if (dlite_check_break(regs.regs_NPC,
			    is_write ? ACCESS_WRITE : ACCESS_READ,
			    addr, sim_num_insn, sim_num_insn))
	dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem);

      /* go to the next instruction if not if flow */
      if (!in_flow)
	  regs.regs_PC = regs.regs_NPC;


      /* finish early? */
      if (max_insts && sim_num_insn >= max_insts)
	return;
    }
}
