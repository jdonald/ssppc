/*
 * powerpc.c - PowerPC definition routines
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
 */

#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "eval.h"
#include "regs.h"
#include "ieee-fp.c"

/* FIXME: currently SimpleScalar/AXP only builds with quadword support... */
#if !defined(HOST_HAS_QWORD)
#error SimpleScalar/PowerPC only builds on hosts with builtin quadword support...
#error Try building with GNU GCC, as it supports quadwords on most machines.
#endif



/* preferred nop instruction definition */
const md_inst_t MD_NOP_INST = 0x60000000;/*ori 0,0,0*/ // const jdonald

/* opcode mask -> enum md_opcodem, used by decoder (MD_OP_ENUM()) */
enum md_opcode md_mask2op[MD_MAX_MASK+1];
unsigned int md_opoffset[OP_MAX];
// for sake of these two without __thread, make sure md_init_decoder() is called globally for all cores /jdonald



/* enum md_opcode -> mask for decoding next level */
const unsigned int md_opmask[OP_MAX+1][2] = { // const jdonald
  {0,0}, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) {0,0},
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) {MASK1,MASK2},
#define CONNECT(OP)
#include "machine.def"
  {0,0}
};

/* enum md_opcode -> shift for decoding next level */
const unsigned int md_opshift[OP_MAX] = { // const jdonald
  0, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) 0,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) SHIFT,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> description string */
const char *md_op2name[OP_MAX] = { // const jdonald
  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) NAME,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) NAME,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> opcode operand format, used by disassembler */
const char *md_op2format[OP_MAX] = { // const jdonald
  NULL, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) OPFORM,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) NULL,
#define CONNECT(OP)
#include "machine.def"
};

/* enum md_opcode -> enum md_fu_class, used by performance simulators */
const enum md_fu_class md_op2fu[OP_MAX] = { // const jdonald
  FUClass_NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) RES,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) FUClass_NA,
#define CONNECT(OP)
#include "machine.def"
};



/* enum md_fu_class -> description string */
const char *md_fu2name[NUM_FU_CLASSES] = { // const jdonald
  NULL, /* NA */
  "fu-int-ALU",
  "fu-int-multiply",
  "fu-int-divide",
  "fu-FP-add/sub",
  "fu-FP-comparison",
  "fu-FP-conversion",
  "fu-FP-multiply",
  "fu-FP-divide",
  "fu-FP-sqrt",
  "rd-port",
  "wr-port"
};

/* enum md_opcode -> opcode flags, used by simulators */
const unsigned int md_op2flags[OP_MAX] = { // const jdonald
  NA, /* NA */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5) FLAGS,
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2) NA,
#define CONNECT(OP)
#include "machine.def"
};

const char *md_amode_str[md_amode_NUM] =
{
  "(const)",		/* immediate addressing mode */
  "(gp + const)",	/* global data access through global pointer */
  "(sp + const)",	/* stack access through stack pointer */
  "(fp + const)",	/* stack access through frame pointer */
  "(reg + const)",	/* (reg + const) addressing */
  "(reg + reg)"		/* (reg + reg) addressing */
};

/* symbolic register names, parser is case-insensitive */
const struct md_reg_names_t md_reg_names[] =
{
  /* name */	/* file */	/* reg */

  /* integer register file */
  { "$r0",	rt_gpr,		0 },
  { "$r1",	rt_gpr,		1 },
  { "$sp",	rt_gpr,		1 },
  { "$r2",	rt_gpr,		2 },
  { "$r3",	rt_gpr,		3 },
  { "$r4",	rt_gpr,		4 },
  { "$r5",	rt_gpr,		5 },
  { "$r6",	rt_gpr,		6 },
  { "$r7",	rt_gpr,		7 },
  { "$r8",	rt_gpr,		8 },
  { "$r9",	rt_gpr,		9 },
  { "$r10",	rt_gpr,		10 },
  { "$r11",	rt_gpr,		11 },
  { "$r12",	rt_gpr,		12 },
  { "$r13",	rt_gpr,		13 },
  { "$r14",	rt_gpr,		14 },
  { "$r15",	rt_gpr,		15 },
  { "$r16",	rt_gpr,		16 },
  { "$r17",	rt_gpr,		17 },
  { "$r18",	rt_gpr,		18 },
  { "$r19",	rt_gpr,		19 },
  { "$r20",	rt_gpr,		20 },
  { "$r21",	rt_gpr,		21 },
  { "$r22",	rt_gpr,		22 },
  { "$r23",	rt_gpr,		23 },
  { "$r24",	rt_gpr,		24 },
  { "$r25",	rt_gpr,		25 },
  { "$r26",	rt_gpr,		26 },
  { "$r27",	rt_gpr,		27 },
  { "$r28",	rt_gpr,		28 },
  { "$r29",	rt_gpr,		29 },
  { "$r30",	rt_gpr,		30 },
  { "$r31",	rt_gpr,		31 },
  { "$fp",	rt_gpr,		31 },
  /* floating point register file - double precision */
  { "$f0",	rt_dpr,		0 },
  { "$f1",	rt_dpr,		1 },
  { "$f2",	rt_dpr,		2 },
  { "$f3",	rt_dpr,		3 },
  { "$f4",	rt_dpr,		4 },
  { "$f5",	rt_dpr,		5 },
  { "$f6",	rt_dpr,		6 },
  { "$f7",	rt_dpr,		7 },
  { "$f8",	rt_dpr,		8 },
  { "$f9",	rt_dpr,		9 },
  { "$f10",	rt_dpr,		10 },
  { "$f11",	rt_dpr,		11 },
  { "$f12",	rt_dpr,		12 },
  { "$f13",	rt_dpr,		13 },
  { "$f14",	rt_dpr,		14 },
  { "$f15",	rt_dpr,		15 },
  { "$f16",	rt_dpr,		16 },
  { "$f17",	rt_dpr,		17 },
  { "$f18",	rt_dpr,		18 },
  { "$f19",	rt_dpr,		19 },
  { "$f20",	rt_dpr,		20 },
  { "$f21",	rt_dpr,		21 },
  { "$f22",	rt_dpr,		22 },
  { "$f23",	rt_dpr,		23 },
  { "$f24",	rt_dpr,		24 },
  { "$f25",	rt_dpr,		25 },
  { "$f26",	rt_dpr,		26 },
  { "$f27",	rt_dpr,		27 },
  { "$f28",	rt_dpr,		28 },
  { "$f29",	rt_dpr,		29 },
  { "$f30",	rt_dpr,		30 },
  { "$f31",	rt_dpr,		31 },
  /* miscellaneous registers */
  { "$cr",	rt_ctrl,	0 },
  { "$xer",	rt_ctrl,	1 },
  { "$fpscr",	rt_ctrl,	2 },
  /*Link register*/
  { "$lr",	rt_link,	0 },
  /*Counter register*/
  { "$cntr",	rt_cntr,	0 },
  /* program counters */
  { "$pc",	rt_PC,		0 },
  { "$npc",	rt_NPC,		0 },
  /* sentinel */
  { NULL,	rt_gpr,		0 }
};

/* returns a register name string */
char *
md_reg_name(enum md_reg_type rt, int reg)
{
  int i;

  for (i=0; md_reg_names[i].str != NULL; i++)
    {
      if (md_reg_names[i].file == rt && md_reg_names[i].reg == reg)
	return md_reg_names[i].str;
    }

  /* not found... */
  return NULL;
}

char *						/* err str, NULL for no err */
md_reg_obj(struct regs_t *regs,			/* registers to access */
	   int is_write,			/* access type */
	   enum md_reg_type rt,			/* reg bank to probe */
	   int reg,				/* register number */
	   struct eval_value_t *val)		/* input, output */
{
  switch (rt)
    {
    case rt_gpr:
      if (reg < 0 || reg >= MD_NUM_IREGS)
	return "register number out of range";

      if (!is_write)
	{
	  val->type = et_uint;
	  val->value.as_uint = regs->regs_R[reg];
	}
      else
	regs->regs_R[reg] = eval_as_uint(*val);
      break;

    case rt_dpr:
      if (reg < 0 || reg >= MD_NUM_FREGS)
	return "register number out of range";

      if (!is_write)
	{
	  val->type = et_double;
	  val->value.as_double = regs->regs_F.d[reg];
	}
      else
	regs->regs_F.d[reg] = eval_as_double(*val);
      break;

    case rt_ctrl:
      switch (reg)
	{
	case /* cr */0:
	  if (!is_write)
	    {
	      val->type = et_uint;
	      val->value.as_uint = regs->regs_C.cr;
	    }
	  else
	    regs->regs_C.cr = eval_as_uint(*val);
	  break;

	case /* xer */1:
	  if (!is_write)
	    {
	      val->type = et_uint;
	      val->value.as_uint = regs->regs_C.xer;
	    }
	  else
	    regs->regs_C.xer = eval_as_uint(*val);
	  break;

	case /* FPSCR */2:
	  if (!is_write)
	    {
	      val->type = et_uint;
	      val->value.as_uint = regs->regs_C.fpscr;
	    }
	  else
	    regs->regs_C.fpscr = eval_as_uint(*val);
	  break;

	default:
	  return "register number out of range";
	}
      break;

    case rt_PC:
      if (!is_write)
	{
	  val->type = et_addr;
	  val->value.as_addr = regs->regs_PC;
	}
      else
	regs->regs_PC = eval_as_addr(*val);
      break;

    case rt_NPC:
      if (!is_write)
	{
	  val->type = et_addr;
	  val->value.as_addr = regs->regs_NPC;
	}
      else
	regs->regs_NPC = eval_as_addr(*val);
      break;

    case rt_link:
      if (!is_write)
	{
	  val->type = et_uint;
	  val->value.as_uint = regs->regs_L;
	}
      else
	regs->regs_L = eval_as_uint(*val);
      break;

    case rt_cntr:
      if (!is_write)
	{
	  val->type = et_uint;
	  val->value.as_uint = regs->regs_CNTR;
	}
      else
	regs->regs_CNTR = eval_as_uint(*val);
      break;


    default:
      panic("bogus register bank");
    }

  /* no error */
  return NULL;
}

/* print integer REG(S) to STREAM */
void
md_print_ireg(md_gpr_t regs, int reg, FILE *stream)
{
  fprintf(stream, "%4s: %12d/0x%08x",
	  md_reg_name(rt_gpr, reg), regs[reg], regs[reg]);
}

void
md_print_iregs(md_gpr_t regs, FILE *stream)
{
  int i;

  for (i=0; i < MD_NUM_IREGS; i += 2)
    {
      md_print_ireg(regs, i, stream);
      fprintf(stream, "  ");
      md_print_ireg(regs, i+1, stream);
      fprintf(stream, "\n");
    }
}

/* print floating point REGS to STREAM */
void
md_print_fpreg(md_fpr_t regs, int reg, FILE *stream)
{
  fprintf(stream, "%4s: %f",
	  md_reg_name(rt_dpr, reg), regs.d[reg]);
}

void
md_print_fpregs(md_fpr_t regs, FILE *stream)
{
  int i;

  /* floating point registers */
  for (i=0; i < MD_NUM_FREGS; i ++)
    {
      md_print_fpreg(regs, i, stream);
      fprintf(stream, "\n");
    }
}

void
md_print_creg(md_ctrl_t regs, int reg, FILE *stream)
{
  /* index is only used to iterate over these registers, hence no enums... */
  switch (reg)
    {
    case 0:
      fprintf(stream, "CR: 0x%08x", regs.cr);
      break;

    case 1:
      fprintf(stream, "XER: 0x%08x", regs.xer);
      break;

    case 2:
      fprintf(stream, "FPSCR: 0x%08x", regs.fpscr);
      break;

    default:
      panic("bogus control register index");
    }
}

void
md_print_cregs(md_ctrl_t regs, FILE *stream)
{
  md_print_creg(regs, 0, stream);
  fprintf(stream, "  ");
  md_print_creg(regs, 1, stream);
  fprintf(stream, "  ");
  md_print_creg(regs, 2, stream);
  fprintf(stream, "\n");
}



/* intialize the inst decoder, this function builds the ISA decode tables */
void
md_init_decoder(void)
{
  unsigned long max_offset = 0;
  unsigned long offset = 0;
  unsigned int i=0;
 for(i=0;i<MD_MAX_MASK;i++) md_mask2op[i]=0;

#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5)\
  if ((MSK)+offset >= MD_MAX_MASK) panic("MASK_MAX is too small");	\
  if (md_mask2op[(MSK)+offset]) {fatal("DEFINST: doubly defined opcode %x %x %s %x. Previous op = %s",OP,MSK, NAME, offset, md_op2name[md_mask2op[(MSK)+offset]]);}\
  md_mask2op[(MSK)+offset]=(OP);md_opoffset[OP]=offset; max_offset=MAX(max_offset,(MSK)+offset);

#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2)			\
 if ((MSK)+offset >= MD_MAX_MASK) panic("MASK_MAX is too small");	\
  if (md_mask2op[(MSK)+offset]) fatal("DEFLINK: doubly defined opcode %x %x %s %x. Previous op =%s", OP,MSK,NAME,offset, md_op2name[md_mask2op[(MSK)+offset]]);\
  md_mask2op[(MSK)+offset]=(OP); max_offset=MAX(max_offset,(MSK)+offset); 

#define CONNECT(OP)							\
    offset = max_offset+1; md_opoffset[OP] = offset;

#include "machine.def"
}

int carrygenerated(sword_t a, sword_t b)
{
  sword_t c, x, y;
  int i;
  
  if((a==0)||(b==0)) return 0;
  
  c=0;
  for(i=0;i<32;i++){
    
    x = (a>>i)&0x1;
    y = (b>>i)&0x1;
    c = x*y| x*c | y*c;
  }
  return c;
}

int isspNan(qword_t t){
  
qword_t mask=ULL(0x0ff0000000000000);
 
 if( ((t&mask) == mask) && ((t&ULL(0x000fffffffffffff))!=0) ) return 1;
 
 return 0;
 
}

int isdpNan(qword_t t){
  
qword_t mask=ULL(0x7ff0000000000000);
 
 if( ((t&mask) == mask) && ((t&ULL(0x000fffffffffffff))!=0) ) return 1;
 
return 0;
 
}

int isdpSNan(qword_t t){
  
  qword_t mask=ULL(0x7ff0000000000000);
  
  if( ((t&mask) == mask) && ((t&ULL(0x000fffffffffffff))!=0) ){
    
    if ( t & ULL(0x0008000000000000)) return 0;
    else return 1;
    
  }
  else return 0;
  
}

int isspSNan(qword_t t){
  
  qword_t mask=ULL(0x7ff0000000000000);
  
  if( ((t&mask) == mask) && ((t&ULL(0x000fffffffffffff))!=0) ){
    
    if ( t & ULL(0x0008000000000000)) return 0;
    else return 1;
    
  }
  else return 0;
  
} 


int isspQNan(qword_t t){
  
qword_t mask=ULL(0x7ff0000000000000);
 
 if( ((t&mask) == mask) && ((t&ULL(0x000fffffffffffff))!=0) ){
   
   if ( t & ULL(0x0008000000000000)) return 1;
   else return 0;
   
 }
 else return 0;
 
}


int isdpQNan(qword_t t){

  qword_t mask=ULL(0x7ff0000000000000);
  
  if( ((t&mask) == mask) && ((t&ULL(0x000fffffffffffff))!=0) ){         
    
    if ( t & ULL(0x0008000000000000)) return 1;
    else return 0;
    
  }
  else return 0;
  
}

dfloat_t convertDWToDouble(qword_t q){
  
  return *((dfloat_t *)(& q));
  
}


/* disassemble a PowerPC instruction */
void
md_print_insn(md_inst_t inst,		/* instruction to disassemble */
	      md_addr_t pc,		/* addr of inst, used for PC-rels */
	      FILE *stream)		/* output stream */
{
  enum md_opcode op;

  /* use stderr as default output stream */
  if (!stream)
    stream = stderr;

  /* decode the instruction, assumes predecoded text segment */
  MD_SET_OPCODE(op, inst);
  
  /* disassemble the instruction */
  if (op == OP_NA || op >= OP_MAX)
    {
      /* bogus instruction */
      fprintf(stream, "<invalid inst: 0x%08x>", inst);
    }
  else
    {
      const char *s;
      
      fprintf(stream, "%-10s", MD_OP_NAME(op));

      s = MD_OP_FORMAT(op);
      while (*s) {
	switch (*s) {
	case 'a':
	  fprintf(stream, "r%d", RA);
	  break;
	case 'b':
	  fprintf(stream, "r%d", RB);
	  break;
	case 'c':
	  fprintf(stream, "r%d", RC);
	  break;
	case 'd':
	  fprintf(stream, "r%d", RD);
	  break;
	case 'e':
	  fprintf(stream, "r%d", ME);
	  break;
	case 'f':
	  fprintf(stream, "%d", BO);
	  break;
	case 'g':
	  fprintf(stream, "%d", BI);
	  break;
	case 'h':
	  fprintf(stream, "%d", SH);
	  break;
	case 'i':
	  fprintf(stream, "%d", IMM);
	  break;
	case 'j':
	  fprintf(stream, "0x%x", LI);
	  break;
	case 'k':
	  fprintf(stream, "%d", BD);
	  break;
	case 'l':
	  fprintf(stream, "%d", ISSETL);
	  break;
	case 'm':
	  fprintf(stream, "%d", MB);
	  break;
	case 'o':
	  fprintf(stream, "%d", OFS);
	  break;
	case 's':
	  fprintf(stream, "r%d", RS);
	  break;
	case 't':
	  fprintf(stream, "%d", TO);
	case 'u':
	  fprintf(stream, "%u", UIMM);
	  break;
	case 'w':
	  fprintf(stream, "%u", CRFS);
	  break;
	case 'x':
	  fprintf(stream, "%u", CRBD);
	  break;
	case 'y':
	  fprintf(stream, "%u", CRBA);
	  break;
	case 'z':
	  fprintf(stream, "%u", CRBB);
	  break;
	case 'A':
	  fprintf(stream, "r%d", FA);
	  break;
	case 'B':
	  fprintf(stream, "r%d", FB);
	  break;
	case 'C':
	  fprintf(stream, "r%d", FC);
	  break;
	case 'D':
	  fprintf(stream, "f%d", FD);
	  break;
	case 'S':
	  fprintf(stream, "f%d", FS);
	  break;
	case 'N':
	  fprintf(stream, "%d", NB);
	  break;
	case 'M':
	  fprintf(stream, "%d", MTFSFI_FM);
	  break;
	case 'P':
	  fprintf(stream, "%d", SPR);
	  break;
	case 'r':
	  fprintf(stream, "%d", CRFD);
	case 'R':
	  fprintf(stream, "%d", CRM);
	  break;
	case 'U':
	  fprintf(stream, "0x%x", UIMM);
	  break;
	default:
	  /* anything unrecognized, e.g., '.' is just passed through */
	  fputc(*s, stream);
	}
	s++;
      }
    }
}

/*This function takes care of the special extended opcode scenario in instructions with opcode 63*/

unsigned int md_check_mask(md_inst_t inst){

if((MD_TOP_OP(inst))!=63) return 0;

 if(inst&0x20) return 1;/*Use the other opmask of size 6 bits*/

 return 0;/*Use the normal opmask of size 11 bits*/

}



/**********************To be fixed for PowerPC****************************/

/* xor checksum registers */ 
/*Problematic since PowerPC has 64 bit FP registers */


word_t
md_xor_regs(struct regs_t *regs)
{
  int i;
  qword_t checksum = 0;

  for (i=0; i < MD_NUM_IREGS; i++)
    checksum ^= regs->regs_R[i];
   
  for (i=0; i < MD_NUM_FREGS; i++)
    checksum ^= *(qword_t *)(&(regs->regs_F.d[i]));

  checksum ^= regs->regs_C.cr;
  checksum ^= regs->regs_C.xer;
  checksum ^= regs->regs_C.fpscr;
  checksum ^= regs->regs_PC;
  checksum ^= regs->regs_NPC;

  return (word_t)checksum;
}

unsigned int sim_mask32(unsigned int start, unsigned int end)
{
    return(((start > end) ?
            ~(((unsigned int)-1 >> start) ^ ((end >= 31) ? 0 : (unsigned int)-1 >> (end + 1)
)) : 
            (((unsigned int)-1 >> start) ^ ((end >= 31) ? 0 : (unsigned int)-1 >> (end+1)))));
}

unsigned int sim_rotate_left_32(unsigned int source, unsigned int count)
{
    unsigned int value;
    
    if (count == 0) 
        return(source);
    
    value = source & sim_mask32(0, count-1);
    value >>= (32 - count);
    value |= (source << count);
    
    return(value);
}
