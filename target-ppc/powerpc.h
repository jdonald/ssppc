/* * powerpc.h - PowerPC definitions
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
 *
 *
 */

#ifndef PPC_H
#define PPC_H

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "ieee-fp.h"
/* #include "ppcmacros.h" */
/* inline the file ppcmacros.h instead to stick with simplescalar distribution
semantics */

/* ppcmacros.h begin */ 
/*
 *Registers are numbered in the following way:
 *  0 - 31 Integer registers
 * 32 - 63 Floating point registers
 * 64 - CR
 * 65 - FPSCR
 * 66 - XER
 * 67 - LR
 * 68 - CNTR
 */

/*GPR and SET_GPR are not defined here since they are defined correctly in sim-fast/sim-outroder correctly and are not included in any ifdef(TARGET_*)*/

#define PPC_DFPR_DW(N)    PPC_DFPR(N)

#define PPC_DXER_LR_CNTR(N)       ( (N==1)? PPC_DXER :((N==8)? PPC_DLR : PPC_DCNTR) )

/*Macros which help in the reading of the registers are defined here*/

#define PPC_GET_XER_CA 		((XER>>29)&0x1)
#define PPC_GET_XER_SO              ((XER>>31)&0x1)
#define PPC_GET_FPSCR_VE            ((FPSCR>>7)&0x1)

/*FIXME: Speculative mode for these registers have to be defined*/
#define LR	        (regs.regs_L)
#define CNTR	(regs.regs_CNTR)
#define TBR    (regs.regs_TBR)


#define CR	       (regs.regs_C.cr)

#define XER	       (regs.regs_C.xer)

#define FPSCR      (regs.regs_C.fpscr)

#define PPC_GPR(N)    (regs.regs_R[(N)])

#define PPC_FPR(N)    (regs.regs_F.d[(N)])


/*Read the lower 32 bits of the floating foint register N*/

#define PPC_FPR_W(N)   ((word_t)((*(qword_t*)(&(PPC_FPR(N))))&0xffffffff))

/*Read the upper 32 bits of the floating foint register N*/

#define PPC_FPR_UW(N)     ((word_t)( (*(qword_t*)(&(PPC_FPR(N)))>>32 )&0xffffffff))

/*Read the floating foint register N as 64 bits*/
#define PPC_FPR_DW(N)     (*((qword_t*)(&(PPC_FPR(N)))))


/*Macros which help in the writing the registers are defined here*/


/***These have to be defined for the speculative mode as well*/
#define PPC_SET_LR(EXPR)		((regs.regs_L = (EXPR)))

#define PPC_SET_CNTR(EXPR)		((regs.regs_CNTR = (EXPR)))



#define PPC_SET_CR(EXPR)	 (regs.regs_C.cr = (EXPR))

#define PPC_SET_XER(EXPR)	 (regs.regs_C.xer = (EXPR))


#define PPC_SET_XER_SO      	 (regs.regs_C.xer = ((XER)|0x80000000))

#define PPC_RESET_XER_SO      	 (regs.regs_C.xer = ((XER)&0x7fffffff))

#define PPC_SET_XER_CA      	 (regs.regs_C.xer = ((XER)|0x20000000))

#define PPC_RESET_XER_CA      	 (regs.regs_C.xer = ((XER)&0xdfffffff))


#define PPC_SET_XER_OV      	(regs.regs_C.xer = ((XER)|0x40000000))

#define PPC_RESET_XER_OV      	(regs.regs_C.xer = ((XER)&0xbfffffff))



#define PPC_SET_FPSCR(EXPR)	(regs.regs_C.fpscr = (EXPR))


#define PPC_SET_FPSCR_FX    	(regs.regs_C.fpscr =  ((FPSCR)|0x80000000))

#define PPC_RESET_FPSCR_FX    	(regs.regs_C.fpscr = ((FPSCR)&0x7fffffff))

#define PPC_SET_FPSCR_FEX    	(regs.regs_C.fpscr = ((FPSCR)|0x40000000))

#define PPC_RESET_FPSCR_FEX    	(regs.regs_C.fpscr = ((FPSCR)&0xbfffffff))

#define PPC_SET_FPSCR_VX    	(regs.regs_C.fpscr = ((FPSCR)|0x20000000))

#define PPC_RESET_FPSCR_VX    	(regs.regs_C.fpscr = ((FPSCR)&0xdfffffff))

#define PPC_SET_FPSCR_OX    	(regs.regs_C.fpscr = ((FPSCR)|0x10000000))

#define PPC_RESET_FPSCR_OX    	(regs.regs_C.fpscr = ((FPSCR)&0xefffffff))

#define PPC_SET_FPSCR_UX    	(regs.regs_C.fpscr = ((FPSCR)|0x08000000))

#define PPC_RESET_FPSCR_UX    	(regs.regs_C.fpscr = ((FPSCR)&0xf7ffffff))

#define PPC_SET_FPSCR_ZX    	(regs.regs_C.fpscr = ((FPSCR)|0x04000000))

#define PPC_RESET_FPSCR_ZX    	(regs.regs_C.fpscr = ((FPSCR)&0xfbffffff))

#define PPC_SET_FPSCR_XX    	(regs.regs_C.fpscr = ((FPSCR)|0x02000000))

#define PPC_RESET_FPSCR_XX    	(regs.regs_C.fpscr = ((FPSCR)&0xfdffffff))

#define PPC_SET_FPSCR_VXSNAN    (regs.regs_C.fpscr = ((FPSCR)|0x01000000))

#define PPC_RESET_FPSCR_VXSNAN  (regs.regs_C.fpscr = ((FPSCR)&0xfeffffff))

#define PPC_SET_FPSCR_VXISI    	(regs.regs_C.fpscr = ((FPSCR)|0x00800000))

#define PPC_RESET_FPSCR_VXISI   (regs.regs_C.fpscr = ((FPSCR)&0xff7fffff))

#define PPC_SET_FPSCR_VXIDI    	(regs.regs_C.fpscr = ((FPSCR)|0x00400000))

#define PPC_RESET_FPSCR_VXIDI   (regs.regs_C.fpscr = ((FPSCR)&0xffbfffff))

#define PPC_SET_FPSCR_VXZDZ    	(regs.regs_C.fpscr = ((FPSCR)|0x00200000))

#define PPC_RESET_FPSCR_VXZDZ   (regs.regs_C.fpscr = ((FPSCR)&0xffdfffff))

#define PPC_SET_FPSCR_VXIMZ    	(regs.regs_C.fpscr = ((FPSCR)|0x00100000))

#define PPC_RESET_FPSCR_VXIMZ   (regs.regs_C.fpscr = ((FPSCR)&0xffefffff))

#define PPC_SET_FPSCR_VXVC   	(regs.regs_C.fpscr = ((FPSCR)|0x00080000))

#define PPC_RESET_FPSCR_VXVC    (regs.regs_C.fpscr = ((FPSCR)&0xfff7ffff))

#define PPC_SET_FPSCR_VXSOFT   	(regs.regs_C.fpscr = ((FPSCR)|0x00000400))

#define PPC_RESET_FPSCR_VXSOFT  (regs.regs_C.fpscr = ((FPSCR)&0xfffffbff))

#define PPC_SET_FPSCR_VXSQRT   	(regs.regs_C.fpscr = ((FPSCR)|0x00000200))

#define PPC_RESET_FPSCR_VXSQRT  (regs.regs_C.fpscr = ((FPSCR)&0xfffffdff))

#define PPC_SET_FPSCR_VXCVI   	(regs.regs_C.fpscr = ((FPSCR)|0x00000100))

#define PPC_RESET_FPSCR_VXCVI   (regs.regs_C.fpscr= ((FPSCR)&0xfffffeff))

#define PPC_SET_FPSCR_FPCC(EXPR)  (regs.regs_C.fpscr = (((FPSCR)&0xffff0fff)|((EXPR)&0x0000f000)))

#define PPC_SET_FPSCR_FPRF(EXPR) (regs.regs_C.fpscr = (((FPSCR)&0xfffe0fff)|((EXPR)&0x0001f000)))

#define PPC_SET_CR0_LT          (regs.regs_C.cr = ((CR)|0x80000000))

#define PPC_RESET_CR0_LT        (regs.regs_C.cr = ((CR)&0x7fffffff))

#define PPC_SET_CR1_FX   	(regs.regs_C.cr = ((CR)|0x08000000))

#define PPC_RESET_CR1_FX        (regs.regs_C.cr = ((CR)&0xf7ffffff))

#define PPC_SET_CR0_GT   	(regs.regs_C.cr = ((CR)|0x40000000))

#define PPC_RESET_CR0_GT    	(regs.regs_C.cr = ((CR)&0xbfffffff))

#define PPC_SET_CR1_FEX   	(regs.regs_C.cr = ((CR)|0x04000000))

#define PPC_RESET_CR1_FEX    	(regs.regs_C.cr = ((CR)&0xfbffffff))

#define PPC_SET_CR0_EQ   	(regs.regs_C.cr = ((CR)|0x20000000))

#define PPC_RESET_CR0_EQ    	(regs.regs_C.cr = ((CR)&0xdfffffff))

#define PPC_SET_CR1_VX   	(regs.regs_C.cr = ((CR)|0x02000000))

#define PPC_RESET_CR1_VX    	(regs.regs_C.cr = ((CR)&0xfdffffff))

#define PPC_SET_CR0_SO   	(regs.regs_C.cr = ((CR)|0x10000000))

#define PPC_RESET_CR0_SO    	(regs.regs_C.cr = ((CR)&0xefffffff))

#define PPC_SET_CR1_OX   	(regs.regs_C.cr = ((CR)|0x01000000))

#define PPC_RESET_CR1_OX    	(regs.regs_C.cr = ((CR)&0xfeffffff))







/*Write floating-point reg N as double word. This macro assumes that the expression passed is a quad */

#define PPC_SET_FPR_DW(N,EXPR)    (regs.regs_F.d[(N)] = convertDWToDouble((EXPR)))


#define PPC_SET_FPR_D(N,EXPR)    (regs.regs_F.d[(N)] = (EXPR))





/*These macros are for Cache management instructions*/

/*Data cache block flush, invalidates the block in the cache addressed by EA*/

#define EXEC_DCBF(EA) 

/*Data cache block store*/

#define EXEC_DCBST(EA)        

/*Data cache block touch*/

#define EXEC_DCBT(EA)	

/*Data cache block touch for store*/

#define EXEC_DCBTST(EA) 

/*Data cache block clear to zero*/

#define EXEC_DCBZ(EA)   

/*Instruction cache block invalidate*/

#define EXEC_ICBI(EA)			

/*Memory Synchronization instructions*/

#define EXEC_ISYNC			
#define EXEC_SYNC			
#define EXEC_EIEIO			

/*Returns the word at the address EA*/

#define EXEC_LWARX(EA,FAULT) 		

/*Stores the word and returns CR0*/

#define EXEC_STWCXD(WORD,EA,FAULT,CR0) 	


/*These macros are for the Trap handler invocation*/

/*Call system trap handler*/

#define TRAP 	 	       
/*External control instructions*/

/*Returns the word at the address EA*/

#define EXEC_ECIWX(EA,FAULT) 		

/*Stores the word at EA*/

#define EXEC_ECOWX(WORD,EA,FAULT) 	

#define PPC_SYSCALL                     

/* ppcmacros.h ends */

/*
 * This file contains various definitions needed to decode, disassemble, and
 * execute PowerPC instructions.
 */

/* build for PowerPC target */
#define TARGET_PPC 

/* not applicable/available, usable in most definition contexts */
#define NA		0

/*
 * target-dependent type definitions
 */

/* define MD_QWORD_ADDRS if the target requires 64-bit (quadword) addresses */
#undef MD_QWORD_ADDRS

/* address type definition */
typedef word_t md_addr_t;


/*
 * target-dependent memory module configuration
 */

/* physical memory page size (must be a power-of-two) */
#define MD_PAGE_SIZE		4096
/*Base 2 Logarithm of the physical memory page size*/
#define MD_LOG_PAGE_SIZE	12


/*
 * target-dependent instruction faults
 */

enum md_fault_type {
  md_fault_none = 0,		/* no fault */
  md_fault_access,		/* storage access fault */
  md_fault_alignment,		/* storage alignment fault */
  md_fault_overflow,		/* signed arithmetic overflow fault */
  md_fault_div0,		/* division by zero fault */
  md_fault_break,		/* BREAK instruction fault */
  md_fault_unimpl,		/* unimplemented instruction fault */
  md_fault_invalidinstruction,  /* PowerPC has an invalid instruction fault */
  md_fault_internal		/* internal S/W fault */
};


/*
 * target-dependent register file definitions, used by regs.[hc]
 */

/* number of integer registers */
#define MD_NUM_IREGS		32
#define MD_NUM_LREGS 		1
#define MD_NUM_CTRREGS  1

/* number of floating point registers */
#define MD_NUM_FREGS		32

/* number of control registers */
#define MD_NUM_CREGS		3

/* total number of registers, excluding PC and NPC */
#define MD_TOTAL_REGS							\
  (/*int*/32 + /*fp*/32 + /*condition*/1 + /*fpscr*/1 + /*xer*/1 + /*lr*/1+/*ctr*/1)

/* general purpose (integer) register file entry type */
typedef sword_t md_gpr_t[MD_NUM_IREGS];

/* floating point register file entry type */

/*IMP:This structure is different from the PISA/Alpha because PowerPC has 64 bit FP registers*/

typedef union {
  qword_t q[MD_NUM_FREGS];      /* integer qword view */
  dfloat_t d[MD_NUM_FREGS];	/* floating point view */
} md_fpr_t;

/* control register file contents */
typedef struct {
  word_t cr, xer;		/* control register and XER register */
  word_t fpscr;			/* floating point status and condition regsiter */
} md_ctrl_t;


typedef word_t md_link_t;      /* link regsiter */
typedef word_t md_ctr_t;       /* counter register */


/*
 * To see the system call conventions on Linux for PPC see
 * http://lxr.linux.no/source/include/asm-ppc/unistd.h
 *
 */

/* well known registers */
enum md_reg_names {
  MD_REG_SP =  1,	/* stack pointer */
  MD_REG_FP = 31,       /* frame pointer */
  MD_REG_V0 = 3,        /* Return value register */
  MD_REG_SC = 0,        /* System call number argument register */
  MD_REG_A0 = 3,        /* System call argument registers */
  MD_REG_A1 = 4,
  MD_REG_A2 = 5, 
  MD_REG_A3 = 6 ,
  MD_REG_A4 = 7 ,
  MD_REG_ERR= 0 ,
  MD_REG_ZERO = 0
};


/*
 * target-dependent instruction format definition
 */
/*PowerPc has a 32 bit instruction*/
/* instruction formats */
typedef word_t md_inst_t;

/* preferred nop instruction definition */
extern const md_inst_t MD_NOP_INST;


/*
 * target-dependent loader module configuration
 */
/*For PowerPC other segments starting address is read from the file header by the
loader. So no need to spefcify other segments*/

/* virtual memory segment limits */

#define MD_STACK_BASE  	0x80000000

/* maximum size of argc+argv+envp environment */
#define MD_MAX_ENVIRON 	131072 /* For PPC :- PAGE_SIZE*MAX_ARG_PAGES . To see this
information, you will have to see the Linux source files as mentioned above*/ 


/*
 * machine.def specific definitions
 */

/* returns the opcode field value of PowerPC instruction INST */

/* inst -> enum md_opcode mapping, use this macro to decode insts */
#define MD_TOP_OP(INST)		(((INST) >> 26) & 0x3f)
#define MD_SET_OPCODE(OP, INST)					       \
 {	OP = md_mask2op[((INST>>26)&0x3f)];                            \
	while (md_opmask[OP][md_check_mask(INST)])                     \
	  OP = md_mask2op[((INST) & md_opmask[OP][md_check_mask(INST)])\
			 + md_opoffset[OP]];   }



/* internal decoder state */

extern unsigned int md_opoffset[];
extern const unsigned int md_opmask[][2];
extern const unsigned int md_opshift[];
extern unsigned int md_check_mask();


/* global opcode names, these are returned by the decoder (MD_OP_ENUM()) */

enum md_opcode {
  OP_NA = 0,	/* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) OP,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) OP,
#define CONNECT(OP)
#include "machine.def"
  OP_MAX	/* number of opcodes + NA */
};

extern enum md_opcode md_mask2op[];
#undef DEFINST 

/* largest opcode field value */
#define MD_MAX_MASK             8192



/* inst -> enum md_opcode mapping, use this macro to decode insts */
#define MD_OP_ENUM(MSK)		(md_mask2op[MSK])
extern enum md_opcode md_mask2op[];

/* enum md_opcode -> description string */
#define MD_OP_NAME(OP)		(md_op2name[OP])
extern const char *md_op2name[];

/* enum md_opcode -> opcode operand format, used by disassembler */
#define MD_OP_FORMAT(OP)	(md_op2format[OP])
extern const char *md_op2format[];

/* function unit classes, update md_fu2name if you update this definition */
enum md_fu_class {
  FUClass_NA = 0,	/* inst does not use a functional unit */
  IntALU=0x1,		/* integer ALU */
  IntMULT=0x2,		/* integer multiplier */
  IntDIV=0x4,		/* integer divider */
  FloatADD=0x8,		/* floating point adder/subtractor */
  FloatCMP=0x10,		/* floating point comparator */
  FloatCVT=0x20,		/* floating point<->integer converter */
  FloatMULT=0x40,		/* floating point multiplier */
  FloatDIV=0x80,		/* floating point divider */
  FloatSQRT=0x100,		/* floating point square root */
  RdPort=0x200,		/* memory read port */
  WrPort=0x400,		/* memory write port */
  NUM_FU_CLASSES=0x800	/* total functional unit classes */
};

/* enum md_opcode -> enum md_fu_class, used by performance simulators */
#define MD_OP_FUCLASS(OP)	(md_op2fu[OP])
extern const enum md_fu_class md_op2fu[];

/* enum md_fu_class -> description string */
#define MD_FU_NAME(FU)		(md_fu2name[FU])
extern const char *md_fu2name[];

/* instruction flags */
#define F_ICOMP		0x00000001	/* integer computation */
#define F_FCOMP		0x00000002	/* FP computation */
#define F_CTRL		0x00000004	/* control inst */
#define F_UNCOND	0x00000008	/*   unconditional change */
#define F_COND		0x00000010	/*   conditional change */
#define F_MEM		0x00000020	/* memory access inst */
#define F_LOAD		0x00000040	/*   load inst */
#define F_STORE		0x00000080	/*   store inst */
#define F_DISP		0x00000100	/*   displaced (R+C) addr mode */
#define F_RR		0x00000200	/*   R+R addr mode */
#define F_DIRECT	0x00000400	/*   direct addressing mode */
#define F_TRAP		0x00000800	/* traping inst */
#define F_LONGLAT	0x00001000	/* long latency inst (for sched) */
#define F_DIRJMP	0x00002000	/* direct jump */
#define F_INDIRJMP	0x00004000	/* indirect jump */
#define F_CALL		0x00008000	/* function call */
#define F_FPCOND	0x00010000	/* FP conditional branch */
#define F_IMM		0x00020000	/* instruction has immediate operand */

/* enum md_opcode -> opcode flags, used by simulators */
#define MD_OP_FLAGS(OP)		(md_op2flags[OP])
extern const unsigned int md_op2flags[];

/* integer register specifiers */
#undef  RS	/* defined in /usr/include/sys/syscall.h on HPUX boxes */

#define RD		((inst >> 21) & 0x1f)		/* reg dest */
#define RA              ((inst >> 16) & 0x1f)
#define RB              ((inst >> 11) & 0x1f)
#define RC              ((inst >>  6) & 0x1f)
#define RS              RD

/* floating point register field synonyms */
#define FS		RS
#define FD		RD
#define FB		RB
#define FA              RA
#define FC		RC

/*Condition registers fields*/
#define CRBD		RD
#define CRBA            RA
#define CRBB            RB

#define CRFD		((inst & 0x03800000)>>23)
#define CRFS		((inst & 0x001C0000)>>18)

/* Used in decoding the PowerPC instructions*/
#define MTFSFI_FM       ((inst>>17)&0xff)
#define MTFSFI_IMM	((inst>>12)&0xf)
#define MB		( (inst & 0x7C0) >> 6  ) 
#define ME		( (inst & 0x3E) >> 1 )
#define SPR		( (inst & 0x001FF800) >> 11 ) 
#define SPRVAL          ( (SPR & 0x1F)+((SPR>>5) & 0x1F) )
#define CRM		( (inst >> 12) & 0xff ) 
#define	TO		RD
#define BO		RD
#define BI		RA
#define BD		( (inst>>2) & 0x3fff )
#define SH		RB
#define NB		RB
#define LK		(inst & 0x1)


#define POSZERODP	0x0000000000000000ULL
#define NEGZERODP	0x8000000000000000ULL
#define POSINFDP	0x7ff0000000000000ULL
#define NEGINFDP	0xfff0000000000000ULL

#define POSZEROSP       0x0000000000000000ULL
#define NEGZEROSP       0x8000000000000000ULL
#define POSINFSP        0x0ff0000000000000ULL
#define NEGINFSP        0x8ff0000000000000ULL


/* If this bit is set in certain instructions, they are invalid for the
32 bit implementation*/
#define ISSETL		((inst & 0x00200000)>>21)

/* returns 16-bit signed immediate field value */
#define IMM		((int)((/* signed */short)(inst & 0xffff)))

/* returns 16-bit unsigned immediate field value */
#define UIMM		(inst & 0xffff)

/* load/store 16-bit signed offset field value, synonym for imm field */
#define OFS		IMM		/* alias to IMM */

/* Immediate field value for branch instruction*/
#define LI		((inst & 0x3fffffc))

#define SEXT24(X)                                                       \
  (((X) & 0x800000) ? ((sword_t)(X) | 0xff000000):(sword_t)(X))

#define SEXT8(X)							\
  (((X) & 0x80) ? ((sword_t)(X) | 0xffffff00):(sword_t)(X))


#define SEXT16(X)                                                        \
  (((X) & 0x8000) ? ((sword_t)(X) | 0xffff0000):(sword_t)(X))

#define SEXT26(X)                                                        \
  (((X) & 0x2000000) ? ((sword_t)(X) | 0xfc000000):(sword_t)(X))

/*These functions are used to support floating point computations*/

int isdpNan(qword_t t);
int isdpSNan(qword_t t);
int isdpQNan(qword_t t);

int isspNan(qword_t t);
int isspSNan(qword_t t);
int isspQNan(qword_t t);
dfloat_t convertDWToDouble(qword_t q);

/* largest signed integer */
#define MAXINT_VAL	0x7fffffff

/* check for overflow in X+Y, both signed */
#define OVER(X,Y)							\
  ((((X) > 0) && ((Y) > 0) && (MAXINT_VAL - (X) < (Y)))			\
   || (((X) < 0) && ((Y) < 0) && (-MAXINT_VAL - (X) > (Y))))

/* check for underflow in X-Y, both signed */
#define UNDER(X,Y)							\
  ((((X) > 0) && ((Y) < 0) && (MAXINT_VAL + (Y) < (X)))			\
   || (((X) < 0) && ((Y) > 0) && (-MAXINT_VAL + (Y) > (X))))

/*Check for the carry bit */

int carrygenerated(sword_t a, sword_t b);

/* default target PC handling */
#ifndef SET_TPC
#define SET_TPC(PC)	(void)0
#endif /* SET_TPC */

/*
 * various other helper macros/functions
 */

/* non-zero if system call is an exit() */
#define	SS_SYS_exit			1
#define MD_EXIT_SYSCALL(REGS)		((REGS)->regs_R[MD_REG_SC] == SS_SYS_exit)

/* non-zero if system call is a write to stdout/stderr */
#define	SS_SYS_write		4
#define MD_OUTPUT_SYSCALL(REGS)						\
  ((REGS)->regs_R[MD_REG_SC] == SS_SYS_write				\
       && ((REGS)->regs_R[MD_REG_A0] == /* stdout */1			\
       || (REGS)->regs_R[MD_REG_A0] == /* stderr */2))

/* returns stream of an output system call, translated to host */
#define MD_STREAM_FILENO(REGS)		((REGS)->regs_R[MD_REG_A0])

/* returns non-zero if instruction is a function call */
#define MD_IS_CALL(OP) ((MD_OP_FLAGS(OP) & (F_CTRL|F_CALL)) == (F_CTRL|F_CALL))

/* returns non-zero if instruction is a function return */
#define MD_IS_RETURN(OP)		(((OP) == BCLR) && ((LK) == 0))

/* returns non-zero if instruction is an indirect jump */
#define MD_IS_INDIR(OP)			( ((OP) == BCCTR) ||((OP) == BCCTRL)|| ((OP) == BCLR) ||((OP) == BCLRL) )

/* addressing mode probe, enums and strings */
enum md_amode_type {
  md_amode_imm,		/* immediate addressing mode */
  md_amode_gp,		/* global data access through global pointer */
  md_amode_sp,		/* stack access through stack pointer */
  md_amode_fp,		/* stack access through frame pointer */
  md_amode_disp,	/* (reg + const) addressing */
  md_amode_rr,		/* (reg + reg) addressing */
  md_amode_NUM
};
extern const char *md_amode_str[md_amode_NUM];

/* addressing mode pre-probe FSM, must see all instructions */
/*
#define MD_AMODE_PREPROBE(OP, FSM)					\
  { if ((OP) == LUI) (FSM) = (RT); }
*/

#define MD_AMODE_PREPROBE(OP, FSM) 
#define MD_AMODE_POSTPROBE(FSM) 

/* compute addressing mode, only for loads/stores */
#define MD_AMODE_PROBE(AM, OP, FSM)					\
  {									\
    if (MD_OP_FLAGS(OP) & F_DISP)					\
      {									\
	if ((RA) == MD_REG_SP)					\
	  (AM) = md_amode_sp;						\
	else								\
	  (AM) = md_amode_disp;						\
      }									\
    else if (MD_OP_FLAGS(OP) & F_RR)					\
      (AM) = md_amode_rr;						\
    else								\
      panic("cannot decode addressing mode");				\
  }

/*#define MD_AMODE_PROBE(AM, OP, FSM)					\
  {									\
    if (MD_OP_FLAGS(OP) & F_DISP)					\
      {									\
	if ((BS) == (FSM))						\
	  (AM) = md_amode_imm;						\
	else if ((BS) == MD_REG_GP)					\
	  (AM) = md_amode_gp;						\
	else if ((BS) == MD_REG_SP)					\
	  (AM) = md_amode_sp;						\
	else if ((BS) == MD_REG_FP)                                     \
	  (AM) = md_amode_fp;						\
	else								\
	  (AM) = md_amode_disp;						\
      }									\
    else if (MD_OP_FLAGS(OP) & F_RR)					\
      (AM) = md_amode_rr;						\
    else								\
      panic("cannot decode addressing mode");				\
  }
  */
/* addressing mode pre-probe FSM, after all loads and stores */
/*#define MD_AMODE_POSTPROBE(FSM)      { (FSM) = MD_REG_ZERO; }*/


/*
 * EIO package configuration/macros
 */

/* expected EIO file format */
#define MD_EIO_FILE_FORMAT		EIO_POWERPC_FORMAT

#define MD_MISC_REGS_TO_EXO(REGS)					\
  exo_new(ec_list,							\
	  /*icnt*/exo_new(ec_integer, (exo_integer_t)sim_num_insn),	\
	  /*PC*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_PC),	\
	  /*NPC*/exo_new(ec_address, (exo_integer_t)(REGS)->regs_NPC),	\
	  /*CR*/exo_new(ec_integer, (exo_integer_t)(REGS)->regs_C.cr),	\
	  /*XER*/exo_new(ec_integer, (exo_integer_t)(REGS)->regs_C.xer),\
	  /*FPSCR*/exo_new(ec_integer, (exo_integer_t)(REGS)->regs_C.fpscr),\
	  /*LINK*/exo_new(ec_integer, (exo_integer_t)(REGS)->regs_L),\
	  /*COUNT*/exo_new(ec_integer, (exo_integer_t)(REGS)->regs_CNTR),\
	  /*TBR*/exo_new(ec_integer, (exo_integer_t)(REGS)->regs_TBR),\
	  NULL)

#define MD_IREG_TO_EXO(REGS, IDX)					\
  exo_new(ec_address, (exo_integer_t)(REGS)->regs_R[IDX])

/*FixMe : Needs to be fixed for PowerPC since PowerPC has 64 bit FP registers*/

#define MD_FREG_TO_EXO(REGS, IDX)					\
  exo_new(ec_address, (exo_integer_t)(REGS)->regs_F.q[IDX])

#define MD_EXO_TO_MISC_REGS(EXO, ICNT, REGS)				\
  /* check EXO format for errors... */					\
  if (!exo								\
      || exo->ec != ec_list						\
      || !exo->as_list.head						\
      || exo->as_list.head->ec != ec_integer				\
      || !exo->as_list.head->next					\
      || exo->as_list.head->next->ec != ec_address			\
      || !exo->as_list.head->next->next					\
      || exo->as_list.head->next->next->ec != ec_address		\
      || !exo->as_list.head->next->next->next				\
      || exo->as_list.head->next->next->next->ec != ec_integer		\
      || !exo->as_list.head->next->next->next->next			\
      || exo->as_list.head->next->next->next->next->ec != ec_integer	\
      || !exo->as_list.head->next->next->next->next->next		\
      || exo->as_list.head->next->next->next->next->next->ec != ec_integer\
      || !exo->as_list.head->next->next->next->next->next->next	\
      || exo->as_list.head->next->next->next->next->next->next->ec != ec_integer \
      || !exo->as_list.head->next->next->next->next->next->next->next	\
      || exo->as_list.head->next->next->next->next->next->next->next->ec != ec_integer\
      || !exo->as_list.head->next->next->next->next->next->next->next->next	\
      || exo->as_list.head->next->next->next->next->next->next->next->next->ec != ec_integer\
      || exo->as_list.head->next->next->next->next->next->next->next->next->next != NULL )	\
    fatal("could not read EIO misc regs");				\
  (ICNT) = (counter_t)exo->as_list.head->as_integer.val;		\
  (REGS)->regs_PC = (md_addr_t)exo->as_list.head->next->as_integer.val;	\
  (REGS)->regs_NPC =							\
    (md_addr_t)exo->as_list.head->next->next->as_integer.val;		\
  (REGS)->regs_C.cr =							\
    (word_t)exo->as_list.head->next->next->next->as_integer.val;	\
  (REGS)->regs_C.xer =							\
    (word_t)exo->as_list.head->next->next->next->next->as_integer.val;	\
  (REGS)->regs_C.fpscr =							\
    (int)exo->as_list.head->next->next->next->next->next->as_integer.val; \
  (REGS)->regs_L =							\
    (md_addr_t)exo->as_list.head->next->next->next->next->next->next->as_integer.val; \
  (REGS)->regs_CNTR =							\
    (int)exo->as_list.head->next->next->next->next->next->next->next->as_integer.val; \
  (REGS)->regs_TBR =							\
    (int)exo->as_list.head->next->next->next->next->next->next->next->next->as_integer.val;


#define MD_EXO_TO_IREG(EXO, REGS, IDX)					\
  ((REGS)->regs_R[IDX] = (word_t)(EXO)->as_integer.val)

/*FixMe: The registers in PowerPC are double precision - 64 bit*/

#define MD_EXO_TO_FREG(EXO, REGS, IDX)					\
  ((REGS)->regs_F.q[IDX] = (qword_t)(EXO)->as_integer.val)

#define MD_EXO_CMP_IREG(EXO, REGS, IDX)					\
  ((REGS)->regs_R[IDX] != (sword_t)(EXO)->as_integer.val)

#define MD_FIRST_IN_REG			0
#define MD_LAST_IN_REG			31

#define MD_FIRST_OUT_REG		0
#define MD_LAST_OUT_REG			31


/*
 * configure the EXO package
 */

	 /* modified for PPC. just make exo_integer_t 64 bit so that
		all regs written are written as if they were 64 bit.
		at this point of time i am too pained to add an extra type
		to libexo for handling double. i suppose this is what float_t
		is supposed to do. but i am not sure and i know this
		method works for alpha. so i guess i'll leave it to future
		generations to fix this if necessary :-)

	 */

/* EXO pointer class */
typedef qword_t exo_address_t;

/* EXO integer class, 64-bit encoding */
typedef qword_t exo_integer_t;

/* EXO floating point class, 64-bit encoding */
typedef double exo_float_t;


/*
 * configure the stats package
 */

/* counter stats */
#ifdef HOST_HAS_QWORD
#define stat_reg_counter		stat_reg_sqword
#define sc_counter			sc_sqword
#define for_counter			for_sqword
#else /* !HOST_HAS_QWORD */
#define stat_reg_counter		stat_reg_double
#define sc_counter			sc_double
#define for_counter			for_double
#endif /* HOST_HAS_QWORD */

/* address stats */
#define stat_reg_addr			stat_reg_uint


/*
 * configure the DLite! debugger
 */

/* register bank specifier */
enum md_reg_type {
	rt_lpr,               /* integer-precison floating point register */
	rt_fpr,               /* single-precision floating point register */
  rt_gpr,		/* general purpose register */
  rt_dpr,		/* double-precision floating pointer register */
  rt_link,		/* link register */
  rt_cntr,		/* counter register */
  rt_ctrl,		/* control register */
  rt_PC,		/* program counter */
  rt_NPC,		/* next program counter */
  rt_NUM
};

/* register name specifier */
struct md_reg_names_t {
  char *str;			/* register name */
  enum md_reg_type file;	/* register file */
  int reg;			/* register index */
};

/* symbolic register names, parser is case-insensitive */
extern const struct md_reg_names_t md_reg_names[];

/* returns a register name string */
char *md_reg_name(enum md_reg_type rt, int reg);

/* default register accessor object */
struct eval_value_t;
struct regs_t;
char *						/* err str, NULL for no err */
md_reg_obj(struct regs_t *regs,			/* registers to access */
	   int is_write,			/* access type */
	   enum md_reg_type rt,			/* reg bank to probe */
	   int reg,				/* register number */
	   struct eval_value_t *val);		/* input, output */

/* print integer REG(S) to STREAM */
void md_print_ireg(md_gpr_t regs, int reg, FILE *stream);
void md_print_iregs(md_gpr_t regs, FILE *stream);

/* print floating point REG(S) to STREAM */
void md_print_fpreg(md_fpr_t regs, int reg, FILE *stream);
void md_print_fpregs(md_fpr_t regs, FILE *stream);

/* print control REG(S) to STREAM */
void md_print_creg(md_ctrl_t regs, int reg, FILE *stream);
void md_print_cregs(md_ctrl_t regs, FILE *stream);

/* xor checksum registers */
word_t md_xor_regs(struct regs_t *regs);


/*
 * configure sim-outorder specifics
 */

/* primitive operation used to compute addresses within pipeline */
#define MD_AGEN_OP		ADD

/* NOP operation when injected into the pipeline */
#define MD_NOP_OP	0x18	

/* non-zero for a valid address, used to determine if speculative accesses
   should access the DL1 data cache */
#define MD_VALID_ADDR(ADDR)						\
  (((ADDR) >= ld_text_base && (ADDR) < (ld_text_base + ld_text_size))	\
   || ((ADDR) >= ld_data_base && (ADDR) < ld_stack_base))

/* this is the address written by the loader for system calls that are imported.
in the 2 word sequence for every sequences, the first word is this value
and the second word is different for each system call. those values
are defined in aixloader.h */
/* in sim-fast we break on a bctr to PPC_SYSCALL_ADDRESS */
#define PPC_SYSCALL_ADDRESS 0x0


/*
 * configure branch predictors
 */

/* shift used to ignore branch address least significant bits, usually
   log2(sizeof(md_inst_t)) */
#define MD_BR_SHIFT		2	/* log2(4) */


/*
 * target-dependent routines
 */

/* intialize the inst decoder, this function builds the ISA decode tables */
void md_init_decoder(void);

/* disassemble an instruction */
void
md_print_insn(md_inst_t inst,		/* instruction to disassemble */
	      md_addr_t pc,		/* addr of inst, used for PC-rels */
	      FILE *stream);		/* output stream */

/* defines ripped from semantics.c for the instruction implementations copied from there */

#define CR_LT_BIT ((unsigned int )0x8)
#define CR_GT_BIT ((unsigned int)0x4)
#define CR_EQ_BIT ((unsigned int)0x2)
#define GET_L(_x)       (((_x) >> 21) &  1)

#define GET_AA(_x)      (((_x) >>  1) &  1)
#define GET_BA(_x)      (((_x) >> 16) & 31)
#define GET_BB(_x)      (((_x) >> 11) & 31)
#define GET_BD(_x)      ((val) ((int16) ((_x) & -4)))
#define GET_BF(_x)      (((_x) >> 23) &  7)

#define GET_BO(_x)      (((_x) >> 21) & 31)
#define GET_BI(_x)      (((_x) >> 16) & 31)
#define GET_LK(_x)       ((_x)        &  1)

#define DECREMENTS_CTR(_x) (0 == (GET_BO (_x) & 4))
#define BR_IF_CTR_ZERO(_x) (GET_BO (_x) & 2)
#define CONDITIONAL_BR(_x) (0 == (GET_BO (_x) & 16))
#define BR_IF_TRUE(_x)     (GET_BO (_x) & 8)

unsigned int sim_mask32(unsigned int start, unsigned int end);
unsigned int sim_rotate_left_32(unsigned int source, unsigned int count);

/*
unsigned int mask32(unsigned int start, unsigned int end)
{
    return(((start > end) ?
            ~(((unsigned int)-1 >> start) ^ ((end >= 31) ? 0 : (unsigned int)-1 >> (end + 1)
)):
            (((unsigned int)-1 >> start) ^ ((end >= 31) ? 0 : (unsigned int)-1 >> (end+1))))
);
}

unsigned int rotate_left_32(unsigned int source, unsigned int count)
{
    unsigned int value;
    
    if (count == 0) 
        return(source);
    
    value = source & mask32(0, count-1);
    value >>= (32 - count);
    value |= (source << count);
    
    return(value);
}
*/

/* Max buffer sizes when args are passed to system call */
#define PPC_SYSCALL_BUFFER 1024

#endif /* POWERPC_H */

