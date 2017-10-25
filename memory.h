/*
 * memory.h - flat memory space interfaces
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
 * $Id: memory.h,v 1.1.1.1 2001/08/10 14:58:22 karu Exp $
 *
 * $Log: memory.h,v $
 * Revision 1.1.1.1  2001/08/10 14:58:22  karu
 * creating ss3ppc-linux repository. The only difference between this and
 * the original ss3ppc linux is that crosss endian issues are taken
 * care of here.
 *
 *
 * Revision 1.2  2001/08/10 14:54:05  karu
 * modified SWAP_QWORD macro
 *
 * Revision 1.1.1.1  2001/08/08 21:28:52  karu
 * creating ss3ppc linux repository
 *
 * Revision 1.4  2000/05/31 01:59:51  karu
 * added support for mis-aligned accesses
 *
 * Revision 1.3  2000/04/11 00:55:56  karu
 * made source platform independent. compiles on linux, solaris and ofcourse aix.
 * powerpc-nonnative.def is the def file with no inline assemble code.
 * manually create the machine.def symlink if compiling on non-aix OS.
 * also syscall.c does not compile because the syscalls haven't been
 * resourced with diff. flags and names for different operating systems.
 *
 * Revision 1.2  2000/04/10 23:46:30  karu
 * converted all quad to qword. NO OTHER change made.
 * the tag specfp95-before-quad-qword is a snapshow before this change was made
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
 * Revision 1.6  1998/08/27 15:39:47  taustin
 * implemented host interface description in host.h
 * added target interface support
 * memory module updated to support 64/32 bit address spaces on 64/32
 *       bit machines, now implemented with a dynamically optimized hashed
 *       page table
 * added support for quadword's
 * added fault support
 * added support for register and memory contexts
 *
 * Revision 1.5  1997/03/11  01:16:23  taustin
 * updated copyright
 * long/int tweaks made for ALPHA target support
 * major macro reorganization to support CC portability
 * mem_valid() added, indicates if an address is bogus, used by DLite!
 *
 * Revision 1.4  1997/01/06  16:01:24  taustin
 * HIDE_MEM_TABLE_DEF added to help with sim-fast.c compilation
 *
 * Revision 1.3  1996/12/27  15:53:15  taustin
 * updated comments
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "options.h"
#include "stats.h"

/*
int READMASKSLEFT[] = { 0xFFFFFFFF, 0x00FFFFFF, 0x0000FFFF, 0x000000FF };
int WRITEMASKSLEFT[] = { 0xFFFFFFFF, 0xFF000000, 0xFFFF0000, 0xFFFFFF00 };
int WRITEMASKSRIGHT[] = { 0xFFFFFFFF, 0x00FFFFFF, 0x0000FFFF, 0x000000FF };
*/

extern const int READMASKSLEFT[];
extern const int WRITEMASKSLEFT[];
extern const int WRITEMASKSRIGHT[];

/* number of entries in page translation hash table (must be power-of-two) */
#define MEM_PTAB_SIZE		(32*1024)
#define MEM_LOG_PTAB_SIZE	15

#ifdef TARGET_X86 // jdonald: all this copied from ss-x86-sel/memory.h

#define MAX_LEV                 3

/* memory status */
enum mem_status {
  MEM_RETRY,                           /* The memory request is denied, try again later */
  MEM_KNOWN,                           /* Latency is known, returned in lat */
  MEM_UNKNOWN,                         /* Latency is not known, callback function is called when it becomes known. */
  UNDER_FETCHED,                       /* the blocks requested under being fetched by memory system */  
  MEM_STALL
};

/* dram memory access */
enum _mem_access_t { IREAD, DREAD, DWRITE };
typedef enum _mem_access_t mem_access_t;

typedef struct{
  struct cache_blk_t *repl;
  struct cache_t *cp;
} access_blk_t;

typedef struct{
  int rid;		  /* request id */
  int type;               /* access type */
  int priority;           /* request priority */
  int lev;                /* number of blocks callback later */
  access_blk_t acc_blks[MAX_LEV];
  struct ROB_entry *re;   /* instruction pointer */     
  void (*callback_fn)     /* function to call when latency is known */
       (int rid, int lat); 
} dram_access_t;


typedef struct {
  enum mem_status status;	/* status */
  int lat;	                /* latency, only set when status == MEM_KNOWN */
} mem_status_t; 

// jdonald: enum_mem_cmd no need to be repeated here, so I removed it

#define MAX_REQ         4
typedef struct {
  struct ROB_entry *re;
  unsigned int tag;
} re_info_t;

#endif // TARGET_X86

/* page table entry */
struct mem_pte_t {
  struct mem_pte_t *next;	/* next translation in this bucket */
  md_addr_t tag;		/* virtual page number tag */
  byte_t *page;			/* page pointer */
};

/* memory object */
struct mem_t {
  /* memory object state */
  char *name;				/* name of this memory space */
  struct mem_pte_t *ptab[MEM_PTAB_SIZE];/* inverted page table */

  /* memory object stats */
  counter_t page_count;			/* total number of pages allocated */
  counter_t ptab_misses;		/* total first level page tbl misses */
  counter_t ptab_accesses;		/* total page table accesses */
};

/* memory access command */
enum mem_cmd {
  Read,			/* read memory from target (simulated prog) to host */
  Write			/* write memory from host (simulator) to target */
};

/* memory access function type, this is a generic function exported for the
   purpose of access the simulated vitual memory space */
typedef enum md_fault_type
(*mem_access_fn)(struct mem_t *mem,	/* memory space to access */
		 enum mem_cmd cmd,	/* Read or Write */
		 md_addr_t addr,	/* target memory address to access */
		 void *p,		/* where to copy to/from */
		 int nbytes);		/* transfer length in bytes */

#ifdef TARGET_X86 // jdonald: ACCESS_NEW comes from ss-x86-sel/memory.h

/* access poiter create macro */
#define ACCESS_NEW(ACC, RID, TYPE, PRI, RE, CFN)	        \
{ int idx;                                                  \
  (ACC) = (dram_access_t*) malloc(sizeof(dram_access_t));   \
  (ACC)->rid = (RID);(ACC)->type = (TYPE);			      	\
  (ACC)->priority = (PRI);(ACC)->re = (RE);					\
  (ACC)->lev = 0;                                           \
  for (idx=0;idx<MAX_LEV;idx++)								\
  {															\
  	(ACC)->acc_blks[idx].repl = NULL;                       \
  	(ACC)->acc_blks[idx].cp = NULL;                         \
  }                                                       	\
  (ACC)->callback_fn = (CFN);  		               	 		\
}

#endif // TARGET_X86

/*
 * virtual to host page translation macros
 */

/* compute page table set */
#define MEM_PTAB_SET(ADDR)						\
  (((ADDR) >> MD_LOG_PAGE_SIZE) & (MEM_PTAB_SIZE - 1))

/* compute page table tag */
#define MEM_PTAB_TAG(ADDR)						\
  ((ADDR) >> (MD_LOG_PAGE_SIZE + MEM_LOG_PTAB_SIZE))

/* convert a pte entry at idx to a block address */
#define MEM_PTE_ADDR(PTE, IDX)						\
  (((PTE)->tag << (MD_LOG_PAGE_SIZE + MEM_LOG_PTAB_SIZE))		\
   | ((IDX) << MD_LOG_PAGE_SIZE))

#define MEM_OFFSET(ADDR)	((ADDR) & (MD_PAGE_SIZE - 1))
/* locate host page for virtual address ADDR, returns NULL if unallocated */
#define MEM_PAGE(MEM, ADDR)						\
  (/* first attempt to hit in first entry, otherwise call xlation fn */	\
   ((MEM)->ptab[MEM_PTAB_SET(ADDR)]					\
    && (MEM)->ptab[MEM_PTAB_SET(ADDR)]->tag == MEM_PTAB_TAG(ADDR))	\
   ? (/* hit - return the page address on host */			\
      (MEM)->ptab_accesses++, \
      (MEM)->ptab[MEM_PTAB_SET(ADDR)]->page)				\
   : (/* first level miss - call the translation helper function */	\
      mem_translate((MEM), (ADDR))))

/* compute address of access within a host page */

#ifdef TARGET_X86 // jdonald: MEM_ADDR2HOST comes from ss-x86-sel/memory.h
#define MEM_ADDR2HOST(MEM, ADDR) MEM_PAGE(MEM, ADDR)+MEM_OFFSET(ADDR)
#endif // TARGET_X86

/* memory tickle function, allocates pages when they are first written */
#define MEM_TICKLE(MEM, ADDR)						\
  (!MEM_PAGE(MEM, ADDR)							\
   ? (/* allocate page at address ADDR */				\
      mem_newpage(MEM, ADDR))						\
   : (/* nada... */ (void)0))

/* memory page iterator */
#define MEM_FORALL(MEM, ITER, PTE)					\
  for ((ITER)=0; (ITER) < MEM_PTAB_SIZE; (ITER)++)			\
    for ((PTE)=(MEM)->ptab[i]; (PTE) != NULL; (PTE)=(PTE)->next)


/*
 * memory accessors macros, fast but difficult to debug...
 */

/* safe version, works only with scalar types */
/* FIXME: write a more efficient GNU C expression for this... */
#define MEM_READ(MEM, ADDR, TYPE)					\
  (MEM_PAGE(MEM, (md_addr_t)(ADDR))					\
   ? *((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR)) + MEM_OFFSET(ADDR)))	\
   : /* page not yet allocated, return zero value */ 0)

/* unsafe version, works with any type */
#define __UNCHK_MEM_READ(MEM, ADDR, TYPE)				\
  (*((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR)) + MEM_OFFSET(ADDR))))

/* safe version, works only with scalar types */
/* FIXME: write a more efficient GNU C expression for this... */
#define MEM_WRITE(MEM, ADDR, TYPE, VAL)					\
  (MEM_TICKLE(MEM, (md_addr_t)(ADDR)),					\
   *((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR)) + MEM_OFFSET(ADDR))) = (VAL))
      
/* unsafe version, works with any type */
#define __UNCHK_MEM_WRITE(MEM, ADDR, TYPE, VAL)				\
  (*((TYPE *)(MEM_PAGE(MEM, (md_addr_t)(ADDR)) + MEM_OFFSET(ADDR))) = (VAL))


/* fast memory accessor macros, typed versions */
#if 0
#define MEM_READ_BYTE(MEM, ADDR)	MEM_READ(MEM, ADDR, byte_t)
#define MEM_READ_SBYTE(MEM, ADDR)	MEM_READ(MEM, ADDR, sbyte_t)
#define MEM_READ_HALF(MEM, ADDR)	MEM_READ(MEM, ADDR, half_t)
#define MEM_READ_SHALF(MEM, ADDR)	MEM_READ(MEM, ADDR, shalf_t)
#define MEM_READ_WORD(MEM, ADDR)	MEM_READ(MEM, ADDR, word_t)
#define MEM_READ_SWORD(MEM, ADDR)	MEM_READ(MEM, ADDR, sword_t)
#define MEM_READ_SFLOAT(MEM, ADDR)	MEM_READ(MEM, ADDR, sfloat_t)
#define MEM_READ_DFLOAT(MEM, ADDR)	MEM_READ(MEM, ADDR, dfloat_t)

#ifdef HOST_HAS_QWORD
#define MEM_READ_QWORD(MEM, ADDR)	MEM_READ(MEM, ADDR, qword_t)
#define MEM_READ_SQWORD(MEM, ADDR)	MEM_READ(MEM, ADDR, sqword_t)
#endif /* HOST_HAS_QWORD */

#define MEM_WRITE_BYTE(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, byte_t, VAL)
#define MEM_WRITE_SBYTE(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, sbyte_t, VAL)
#define MEM_WRITE_HALF(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, half_t, VAL)
#define MEM_WRITE_SHALF(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, shalf_t, VAL)
#define MEM_WRITE_WORD(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, word_t, VAL)
#define MEM_WRITE_SWORD(MEM, ADDR, VAL) MEM_WRITE(MEM, ADDR, sword_t, VAL)
#define MEM_WRITE_SFLOAT(MEM, ADDR, VAL) MEM_WRITE(MEM, ADDR, sfloat_t, VAL)
#define MEM_WRITE_DFLOAT(MEM, ADDR, VAL) MEM_WRITE(MEM, ADDR, dfloat_t, VAL)

#ifdef HOST_HAS_QWORD
#define MEM_WRITE_QWORD(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, qword_t, VAL)
#define MEM_WRITE_SQWORD(MEM, ADDR, VAL)	MEM_WRITE(MEM, ADDR, sqword_t, VAL)
#endif /* HOST_HAS_QWORD */
#endif

// jdonald changes all the PPC_SWAP checks to check for TARGET_PPC as well
// that way, this is ignored in PISA or Alpha mode
#if defined(PPC_SWAP) && defined(TARGET_PPC)

#undef SWAP_HALF // jdonald
#define SWAP_HALF(X)                                                    \
  (((((half_t)(X)) & 0xff) << 8) | ((((half_t)(X)) & 0xff00) >> 8))
#undef SWAP_WORD // jdonald
#define SWAP_WORD(X)    (((word_t)(X) << 24) |                          \
                         (((word_t)(X) << 8)  & 0x00ff0000) |           \
                         (((word_t)(X) >> 8)  & 0x0000ff00) |           \
                         (((word_t)(X) >> 24) & 0x000000ff))

#define SWAP_QWORD2(X)   (((qword_t)(X) << 56) |                         \
                         (((qword_t)(X) << 40) & ULL(0x00ff000000000000)) |\
                         (((qword_t)(X) << 24) & ULL(0x0000ff0000000000)) |\
                         (((qword_t)(X) << 8)  & ULL(0x000000ff00000000)) |\
                         (((qword_t)(X) >> 8)  & ULL(0x00000000ff000000)) |\
                         (((qword_t)(X) >> 24) & ULL(0x0000000000ff0000)) |\
                         (((qword_t)(X) >> 40) & ULL(0x000000000000ff00)) |\
                         (((qword_t)(X) >> 56) & ULL(0x00000000000000ff)))

#define SWAP_QWORD(X) ({ long long _lqword_temp1 = SWAP_QWORD2(X); \
                        long long _lqword_temp2 = _lqword_temp1; \
                        (_lqword_temp1 >> 32) | \
                                      (_lqword_temp2 << 32); })

#else 

#undef SWAP_HALF // jdonald
#undef SWAP_WORD // jdonald
#define SWAP_HALF(X) (X)
#define SWAP_WORD(X) (X)
#define SWAP_QWORD(X) (X)

#endif

#if !defined(TARGET_ARM) && !defined(TARGET_X86) // jdonald: notices that the ARM (and x86) support defines its own MD_SWAP* macros in machine.h
#define MD_SWAPH(X)             SWAP_HALF(X)
#define MD_SWAPW(X)             SWAP_WORD(X)
#define MD_SWAPQ(X)             SWAP_QWORD(X)
#define MD_SWAPI(X)             SWAP_WORD(X)
#endif // !TARGET_ARM

#define ADDR_REM(ADDR) ((ADDR)%4)
#define LEFTWORD(MEM, ADDR) (MEM_READ(MEM, (ADDR-((ADDR)%4)), word_t) )
#define RIGHTWORD(MEM,ADDR) (MEM_READ(MEM, ((ADDR-((ADDR)%4))+4), word_t) )

#if defined(PPC_SWAP) && defined(TARGET_PPC)
/* just to make sure :-) */

#define MEM_READ_BYTE(MEM, ADDR)        MEM_READ(MEM, ADDR, byte_t)
#define MEM_READ_SBYTE(MEM, ADDR)       MEM_READ(MEM, ADDR, sbyte_t)
#define MEM_READ_HALF2(MEM, ADDR) 	((ADDR%4)==0)?(MEM_READ(MEM, ADDR, half_t)):\
								((half_t) ( ( (MEM_READ_BYTE(MEM,ADDR+1) << 8) | \
									 (MEM_READ_BYTE(MEM,ADDR)) )\
								))
#define MEM_READ_SHALF2(MEM, ADDR)    ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, shalf_t)):\
                        ((shalf_t) ( ( (MEM_READ_BYTE(MEM,ADDR+1) << 8) | \
                            (MEM_READ_BYTE(MEM,ADDR)) )\
                        ))
/*
#define MEM_READ_WORD2(MEM, ADDR) ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, word_t)):\
						((word_t) ( (( (LEFTWORD(MEM,ADDR) & READMASKSLEFT[ADDR_REM(ADDR)]) << (ADDR_REM(ADDR)*8)) |\
							(RIGHTWORD(MEM,ADDR) >> ((4-ADDR_REM(ADDR))*8)) )) )
#define MEM_READ_SWORD2(MEM, ADDR) ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, sword_t)):\
                  ((sword_t) (( (LEFTWORD(MEM,ADDR) & READMASKSLEFT[ADDR_REM(ADDR)]) << (ADDR_REM(ADDR)*8)) |\
                     (RIGHTWORD(MEM, ADDR) >> ((4-ADDR_REM(ADDR))*8)) ))
*/

#define MEM_READ_WORD2(MEM, ADDR) ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, word_t)):\
                    ( (word_t) ( (MEM_READ_BYTE(MEM, ADDR+3) << 24) | \
								   (MEM_READ_BYTE(MEM, ADDR+2) << 16) | \
								   (MEM_READ_BYTE(MEM, ADDR+1) << 8) | \
								   (MEM_READ_BYTE(MEM, ADDR) ) ) )

#define MEM_READ_SWORD2(MEM, ADDR) ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, sword_t)):\
                    ( (sword_t) ( (MEM_READ_BYTE(MEM, ADDR+3) << 24) | \
								   (MEM_READ_BYTE(MEM, ADDR+2) << 16) | \
								   (MEM_READ_BYTE(MEM, ADDR+1) << 8) | \
								   (MEM_READ_BYTE(MEM, ADDR) ) ) )

#define MEM_READ_HALF(MEM, ADDR)        MD_SWAPH( MEM_READ_HALF2(MEM, ADDR) )
#define MEM_READ_SHALF(MEM, ADDR)       MD_SWAPH( MEM_READ_SHALF2(MEM, ADDR) )
#define MEM_READ_WORD(MEM, ADDR)        MD_SWAPW( MEM_READ_WORD2(MEM, ADDR) )
#define MEM_READ_SWORD(MEM, ADDR)       MD_SWAPW( MEM_READ_SWORD2(MEM, ADDR) )

#else

#define MEM_READ_BYTE(MEM, ADDR)        MEM_READ(MEM, ADDR, byte_t)
#define MEM_READ_SBYTE(MEM, ADDR)       MEM_READ(MEM, ADDR, sbyte_t)
/*
#define MEM_READ_HALF(MEM, ADDR)        MD_SWAPH(MEM_READ(MEM, ADDR, half_t))
#define MEM_READ_SHALF(MEM, ADDR)       MD_SWAPH(MEM_READ(MEM, ADDR, shalf_t))
#define MEM_READ_WORD(MEM, ADDR)        MD_SWAPW(MEM_READ(MEM, ADDR, word_t))
#define MEM_READ_SWORD(MEM, ADDR)       MD_SWAPW(MEM_READ(MEM, ADDR, sword_t))
*/

#define MEM_READ_HALF(MEM, ADDR) 	((ADDR%4)==0)?(MEM_READ(MEM, ADDR, half_t)):\
								((half_t) ( ( (MEM_READ_BYTE(MEM,ADDR) << 8) | \
									 (MEM_READ_BYTE(MEM,ADDR+1)) )\
								))
#define MEM_READ_SHALF(MEM, ADDR)    ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, shalf_t)):\
                        ((shalf_t) ( ( (MEM_READ_BYTE(MEM,ADDR) << 8) | \
                            (MEM_READ_BYTE(MEM,ADDR+1)) )\
                        ))

#define MEM_READ_WORD(MEM, ADDR) ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, word_t)):\
						((word_t) (( (LEFTWORD(MEM,ADDR) & READMASKSLEFT[ADDR_REM(ADDR)]) << (ADDR_REM(ADDR)*8)) |\
							(RIGHTWORD(MEM,ADDR) >> ((4-ADDR_REM(ADDR))*8)) ))
#define MEM_READ_SWORD(MEM, ADDR) ((ADDR%4)==0)?(MEM_READ(MEM, ADDR, sword_t)):\
                  ((sword_t) (( (LEFTWORD(MEM,ADDR) & READMASKSLEFT[ADDR_REM(ADDR)]) << (ADDR_REM(ADDR)*8)) |\
                     (RIGHTWORD(MEM, ADDR) >> ((4-ADDR_REM(ADDR))*8)) ))

#endif

#ifdef HOST_HAS_QWORD
#define MEM_READ_QWORD(MEM, ADDR)       MD_SWAPQ( ( (qword_t) (\
			( ((qword_t) (MEM_READ(MEM, ADDR, word_t))) << 32) |					\
			((MEM_READ(MEM, ADDR+4, word_t)))	\
			) ) )
#define MEM_READ_SQWORD(MEM, ADDR)      MD_SWAPQ( ( (qword_t) (\
         ( ((qword_t) (MEM_READ(MEM, ADDR, word_t))) << 32) |              \
         ((MEM_READ(MEM, ADDR+4, word_t)))   \
         ) ) )

#endif /* HOST_HAS_QWORD */

#if defined(PPC_SWAP) && defined(TARGET_PPC)
#define MEM_WRITE_BYTE(MEM, ADDR, VAL)  MEM_WRITE(MEM, ADDR, byte_t, VAL)
#define MEM_WRITE_SBYTE(MEM, ADDR, VAL) MEM_WRITE(MEM, ADDR, sbyte_t, VAL)
/*
#define MEM_WRITE_HALF(MEM, ADDR, VAL)                                  \
                                MEM_WRITE(MEM, ADDR, half_t, MD_SWAPH(VAL))
#define MEM_WRITE_SHALF(MEM, ADDR, VAL)                                 \
                                MEM_WRITE(MEM, ADDR, shalf_t, MD_SWAPH(VAL))
*/
#define MEM_WRITE_HALF(MEM, ADDR, VAL) (((ADDR)%4)==0)?MEM_WRITE(MEM, ADDR, half_t, MD_SWAPH(VAL)):\
									(\
										(MEM_WRITE_BYTE(MEM, ADDR, ((VAL)>>8)) ),\
										(MEM_WRITE_BYTE(MEM, ((ADDR)+1), ((VAL)&0xFF)) )\
									)
#define MEM_WRITE_SHALF(MEM, ADDR, VAL) (((ADDR)%4)==0)?MEM_WRITE(MEM, ADDR, shalf_t, MD_SWAPH(VAL)):\
                           (\
                              (MEM_WRITE_BYTE(MEM, ADDR, ((VAL)>>8)) ),\
                              (MEM_WRITE_BYTE(MEM, ((ADDR)+1), ((VAL)&0xFF)) )\
                           )

/*
#define MEM_WRITE_WORD(MEM, ADDR, VAL) (((ADDR)%4)==0)?\
										MEM_WRITE(MEM, (ADDR), word_t, MD_SWAPW(VAL)):\
										(\
										(\
										MEM_WRITE(MEM, ((ADDR)-((ADDR)%4)), word_t, ((LEFTWORD(MEM, (ADDR) ) &  WRITEMASKSLEFT[ADDR_REM(ADDR)]) | (VAL >> (ADDR_REM(ADDR)*8))) )	\
										),	\
										(\
										MEM_WRITE(MEM, ((ADDR)-((ADDR)%4)+4), word_t, ((RIGHTWORD(MEM, (ADDR)) &  WRITEMASKSRIGHT[ADDR_REM(ADDR)]) | (VAL << ((4-ADDR_REM(ADDR))*8))) )									\
										)		\
										)
*/

#define MEM_WRITE_WORD(MEM, ADDR, VAL) (( (ADDR)%4)==0)? \
                   (MEM_WRITE(MEM, (ADDR), word_t, MD_SWAPW(VAL))):\
                   ( MEM_WRITE_BYTE(MEM, (ADDR), (VAL) >> 24) , \
				   MEM_WRITE_BYTE(MEM, (ADDR)+1, ((VAL) >> 16) & 0xff ) , \
				   MEM_WRITE_BYTE(MEM, (ADDR)+2, ((VAL) >> 8) & 0xff ) , \
				   MEM_WRITE_BYTE(MEM, (ADDR)+3, (VAL) & 0xff ) )

/* FIXME: SWAPPING not done for WRITE_SWORD because I do not use
   it anywhere in the PPC's machine.def
*/

#define MEM_WRITE_SWORD(MEM, ADDR, VAL) (((ADDR)%4)==0)?\
                              MEM_WRITE(MEM, ADDR, sword_t, MD_SWAPW(VAL)):\
                              MEM_WRITE_WORD(MEM, ADDR, VAL)
 

											

#define MEM_WRITE_SFLOAT(MEM, ADDR, VAL)                                \
                                MEM_WRITE(MEM, ADDR, sfloat_t, MD_SWAPW(VAL))
#define MEM_WRITE_DFLOAT(MEM, ADDR, VAL)                                \
                                MEM_WRITE(MEM, ADDR, dfloat_t, MD_SWAPQ(VAL))


#else


#define MEM_WRITE_BYTE(MEM, ADDR, VAL)  MEM_WRITE(MEM, ADDR, byte_t, VAL)
#define MEM_WRITE_SBYTE(MEM, ADDR, VAL) MEM_WRITE(MEM, ADDR, sbyte_t, VAL)
/*
#define MEM_WRITE_HALF(MEM, ADDR, VAL)                                  \
                                MEM_WRITE(MEM, ADDR, half_t, MD_SWAPH(VAL))
#define MEM_WRITE_SHALF(MEM, ADDR, VAL)                                 \
                                MEM_WRITE(MEM, ADDR, shalf_t, MD_SWAPH(VAL))
*/
#define MEM_WRITE_HALF(MEM, ADDR, VAL) (((ADDR)%4)==0)?MEM_WRITE(MEM, ADDR, half_t, MD_SWAPH(VAL)):\
									(\
										(MEM_WRITE_BYTE(MEM, ADDR, ((VAL)>>8)) ),\
										(MEM_WRITE_BYTE(MEM, ((ADDR)+1), ((VAL)&0xFF)) )\
									)
#define MEM_WRITE_SHALF(MEM, ADDR, VAL) (((ADDR)%4)==0)?MEM_WRITE(MEM, ADDR, shalf_t, MD_SWAPH(VAL)):\
                           (\
                              (MEM_WRITE_BYTE(MEM, ADDR, ((VAL)>>8)) ),\
                              (MEM_WRITE_BYTE(MEM, ((ADDR)+1), ((VAL)&0xFF)) )\
                           )

/*
#define MEM_WRITE_WORD(MEM, ADDR, VAL)                                  \
                                MEM_WRITE(MEM, ADDR, word_t, MD_SWAPW(VAL))
#define MEM_WRITE_SWORD(MEM, ADDR, VAL)                                 \
                                MEM_WRITE(MEM, ADDR, sword_t, MD_SWAPW(VAL))
*/
#define MEM_WRITE_WORD(MEM, ADDR, VAL) (((ADDR)%4)==0)?\
										MEM_WRITE(MEM, (ADDR), word_t, MD_SWAPW(VAL)):\
										(\
										(\
										MEM_WRITE(MEM, ((ADDR)-((ADDR)%4)), word_t, ((LEFTWORD(MEM, (ADDR) ) &  WRITEMASKSLEFT[ADDR_REM(ADDR)]) | (VAL >> (ADDR_REM(ADDR)*8))) )	\
										),	\
										(\
										MEM_WRITE(MEM, ((ADDR)-((ADDR)%4)+4), word_t, ((RIGHTWORD(MEM, (ADDR)) &  WRITEMASKSRIGHT[ADDR_REM(ADDR)]) | (VAL << ((4-ADDR_REM(ADDR))*8))) )									\
										)		\
										)
#define MEM_WRITE_SWORD(MEM, ADDR, VAL) (((ADDR)%4)==0)?\
                              MEM_WRITE(MEM, ADDR, sword_t, MD_SWAPW(VAL)):\
                              MEM_WRITE_WORD(MEM, ADDR, VAL)
 

											

#define MEM_WRITE_SFLOAT(MEM, ADDR, VAL)                                \
                                MEM_WRITE(MEM, ADDR, sfloat_t, MD_SWAPW(VAL))
#define MEM_WRITE_DFLOAT(MEM, ADDR, VAL)                                \
                                MEM_WRITE(MEM, ADDR, dfloat_t, MD_SWAPQ(VAL))

#endif

#ifdef HOST_HAS_QWORD
/*
#define MEM_WRITE_QWORD(MEM, ADDR, VAL)                                 \
                                MEM_WRITE(MEM, ADDR, qword_t, MD_SWAPQ(VAL))
#define MEM_WRITE_SQWORD(MEM, ADDR, VAL)                                \
                                MEM_WRITE(MEM, ADDR, sqword_t, MD_SWAPQ(VAL))
*/
#define MEM_WRITE_QWORD(MEM, ADDR, VAL) \
									(MEM_WRITE_WORD(MEM, ADDR, (((qword_t) (VAL))>>32))  , MEM_WRITE_WORD(MEM, ADDR, ((VAL)&0xFFFFFFFF)))
#define MEM_WRITE_SQWORD(MEM, ADDR, VAL) \
									MEM_WRITE_QWORD(MEM, ADDR, VAL)

#endif /* HOST_HAS_QWORD */

/* create a flat memory space */
struct mem_t *
mem_create(char *name);			/* name of the memory space */
	   
/* translate address ADDR in memory space MEM, returns pointer to host page */
byte_t *
mem_translate(struct mem_t *mem,	/* memory space to access */
	      md_addr_t addr);		/* virtual address to translate */

/* allocate a memory page */
void
mem_newpage(struct mem_t *mem,		/* memory space to allocate in */
	    md_addr_t addr);		/* virtual address to allocate */

#ifdef TARGET_X86 // jdonald: 3 functions taken from ss-x86-sel/memory.h
/* given a set of pages, this creates a set of new page mappings,
 * try to use addr as the suggested addresses
 */
md_addr_t
mem_newmap(struct mem_t *mem,	         /* memory space to access */
           md_addr_t    addr,            /* virtual address to map to */
           size_t       length);

/* given a set of pages, this creates a set of new page mappings,
 * try to use addr as the suggested addresses, but point to something
 * we allocated ourselves.
 */
md_addr_t
mem_newmap2(struct mem_t *mem,	         /* memory space to access */
           md_addr_t    addr,            /* virtual address to map to */
           md_addr_t    our_addr,        /* our stuff (return value from mmap) */
           size_t       length);

/* given a set of pages, this removes them from our map.  Necessary
 * for munmap.
 */
void
mem_delmap(struct mem_t *mem,	         /* memory space to access */
           md_addr_t    addr,            /* virtual address to delete */
           size_t       length);
#endif // jdonald: TARGET_X86
/* generic memory access function, it's safe because alignments and permissions
   are checked, handles any natural transfer sizes; note, faults out if nbytes
   is not a power-of-two or larger then MD_PAGE_SIZE */
enum md_fault_type
mem_access(struct mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes);			/* number of bytes to access */

/* register memory system-specific statistics */
void
mem_reg_stats(struct mem_t *mem,	/* memory space to declare */
	      struct stat_sdb_t *sdb);	/* stats data base */

/* initialize memory system, call before loader.c */
void
mem_init(struct mem_t *mem);	/* memory space to initialize */

/* dump a block of memory, returns any faults encountered */
enum md_fault_type
mem_dump(struct mem_t *mem,		/* memory space to display */
	 md_addr_t addr,		/* target address to dump */
	 int len,			/* number bytes to dump */
	 FILE *stream);			/* output stream */


/*
 * memory accessor routines, these routines require a memory access function
 * definition to access memory, the memory access function provides a "hook"
 * for programs to instrument memory accesses, this is used by various
 * simulators for various reasons; for the default operation - direct access
 * to the memory system, pass mem_access() as the memory access function
 */

/* copy a '\0' terminated string to/from simulated memory space, returns
   the number of bytes copied, returns any fault encountered */
enum md_fault_type
mem_strcpy(mem_access_fn mem_fn,	/* user-specified memory accessor */
	   struct mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   char *s);			/* host memory string buffer */

/* copy NBYTES to/from simulated memory space, returns any faults */
enum md_fault_type
mem_bcopy(mem_access_fn mem_fn,		/* user-specified memory accessor */
	  struct mem_t *mem,		/* memory space to access */
	  enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	  md_addr_t addr,		/* target address to access */
	  void *vp,			/* host memory address to access */
	  int nbytes);			/* number of bytes to access */

/* copy NBYTES to/from simulated memory space, NBYTES must be a multiple
   of 4 bytes, this function is faster than mem_bcopy(), returns any
   faults encountered */
enum md_fault_type
mem_bcopy4(mem_access_fn mem_fn,	/* user-specified memory accessor */
	   struct mem_t *mem,		/* memory space to access */
	   enum mem_cmd cmd,		/* Read (from sim mem) or Write */
	   md_addr_t addr,		/* target address to access */
	   void *vp,			/* host memory address to access */
	   int nbytes);			/* number of bytes to access */

/* zero out NBYTES of simulated memory, returns any faults encountered */
enum md_fault_type
mem_bzero(mem_access_fn mem_fn,		/* user-specified memory accessor */
	  struct mem_t *mem,		/* memory space to access */
	  md_addr_t addr,		/* target address to access */
	  int nbytes);			/* number of bytes to clear */

#endif /* MEMORY_H */
