/*
 * loader.h - program loader interfaces
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
 * $Id: loader.h,v 1.1.1.1 2001/08/10 14:58:22 karu Exp $
 *
 * $Log: loader.h,v $
 * Revision 1.1.1.1  2001/08/10 14:58:22  karu
 * creating ss3ppc-linux repository. The only difference between this and
 * the original ss3ppc linux is that crosss endian issues are taken
 * care of here.
 *
 *
 * Revision 1.1.1.1  2001/08/08 21:28:52  karu
 * creating ss3ppc linux repository
 *
 * Revision 1.2  2001/06/05 09:41:00  karu
 * removed warnings and fixed sim-profile. there is still an addressing mode problem unresolved in sim-profile. sim-eio has some system call problems unresolved.
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
 * Revision 1.6  1998/08/27 16:44:14  taustin
 * implemented host interface description in host.h
 * added target interface support
 * added support for register and memory contexts
 *
 * Revision 1.5  1997/03/11  01:13:20  taustin
 * updated copyright
 *
 * Revision 1.4  1997/01/06  16:00:03  taustin
 * ld_prog_fname variable exported
 *
 * Revision 1.3  1996/12/27  15:52:06  taustin
 * updated comments
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef LOADER_H
#define LOADER_H

#include <stdio.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"

/*
 * This module implements program loading.  The program text (code) and
 * initialized data are first read from the program executable.  Next, the
 * program uninitialized data segment is initialized to all zero's.  Finally,
 * the program stack is initialized with command line arguments and
 * environment variables.  The format of the top of stack when the program
 * starts execution is as follows:
 *
 * 0x7fffffff    +----------+
 *               | unused   |
 * 0x7fffc000    +----------+
 *               | envp     |
 *               | strings  |
 *               +----------+
 *               | argv     |
 *               | strings  |
 *               +----------+
 *               | envp     |
 *               | array    |
 *               +----------+
 *               | argv     |
 *               | array    |
 *               +----------+
 *               | argc     |
 * regs_R[29]    +----------+
 * (stack ptr)
 *
 * NOTE: the start of envp is computed in crt0.o (C startup code) using the
 * value of argc and the computed size of the argv array, the envp array size
 * is not specified, but rather it is NULL terminated, so the startup code
 * has to scan memory for the end of the string.
 */

/*
 * program segment ranges, valid after calling ld_load_prog()
 */

/* program text (code) segment base */
extern __thread md_addr_t ld_text_base;

/* program text (code) size in bytes */
extern __thread unsigned int ld_text_size;

/* program initialized data segment base */
extern __thread md_addr_t ld_data_base;

/* program initialized ".data" and uninitialized ".bss" size in bytes */
extern __thread unsigned int ld_data_size;

/* top of the data segment */
extern __thread md_addr_t ld_brk_point;

/* program stack segment base (highest address in stack) */
extern __thread md_addr_t ld_stack_base;

/* program initial stack size */
extern __thread unsigned int ld_stack_size;

/* lowest address accessed on the stack */
extern __thread md_addr_t ld_stack_min;

/* program file name */
extern __thread char *ld_prog_fname;

/* program entry point (initial PC) */
extern __thread md_addr_t ld_prog_entry;

/* program environment base address address */
extern __thread md_addr_t ld_environ_base;

/* target executable endian-ness, non-zero if big endian */
extern __thread int ld_target_big_endian;

/* register simulator-specific statistics */
void
ld_reg_stats(struct stat_sdb_t *sdb);	/* stats data base */

/* load program text and initialized data into simulated virtual memory
   space and initialize program segment range variables */
void
ld_load_prog(char *fname,		/* program to load */
	     int argc, char **argv,	/* simulated program cmd line args */
	     char **envp,		/* simulated program environment */
	     struct regs_t *regs,	/* registers to initialize for load */
	     struct mem_t *mem,		/* memory space to load prog into */
	     int zero_bss_segs);	/* zero uninit data segment? */

#endif /* LOADER_H */

#ifdef TARGET_PPC
/* declare the function prototypes for the functions that write
   the millicode stuff
*/
void writemillicode(struct mem_t *mem);
void writemillicodepredecode(struct mem_t *mem);
#endif
