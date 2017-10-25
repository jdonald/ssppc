/*
 * syscall.h - proxy system call handler interfaces
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
 * $Id: syscall.h,v 1.1.1.1 2001/08/10 14:58:23 karu Exp $
 *
 * $Log: syscall.h,v $
 * Revision 1.1.1.1  2001/08/10 14:58:23  karu
 * creating ss3ppc-linux repository. The only difference between this and
 * the original ss3ppc linux is that crosss endian issues are taken
 * care of here.
 *
 *
 * Revision 1.1.1.1  2001/08/08 21:28:53  karu
 * creating ss3ppc linux repository
 *
 * Revision 1.8  2001/08/03 06:12:27  karu
 * added support for EIO traces. checkpointing still does not seem to work
 *
 * Revision 1.7  2001/06/28 02:24:48  karu
 * added support for fclear system call. i havent seen a benchmark really use it so far. just added because i used this as an example for holger
 *
 * Revision 1.6  2001/06/27 03:58:58  karu
 * fixed errno setting in loader.c and added errno support to all syscalls. added 3 more syscalls access, accessx and faccessx to get NAS benchmark MG working. also added MTFB instruction from the patch AK mailed me looooong time back. the NAS benchmarks were also dying becuase of that instruction just like sphinx
 *
 * Revision 1.5  2001/06/05 09:41:03  karu
 * removed warnings and fixed sim-profile. there is still an addressing mode problem unresolved in sim-profile. sim-eio has some system call problems unresolved.
 *
 * Revision 1.4  2000/04/03 20:03:22  karu
 * entire specf95 working .
 *
 * Revision 1.3  2000/03/28 00:37:13  karu
 * added fstatx and added fdivs implementation
 *
 * Revision 1.2  2000/03/23 01:19:20  karu
 * added syscall lseek and defined errno
 *
 * Revision 1.1.1.1  2000/03/07 05:15:18  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:53  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.5  1998/08/27 16:49:58  taustin
 * implemented host interface description in host.h
 * added target interface support
 * moved target-dependent definitions to target files
 * added support for register and memory contexts
 *
 * Revision 1.4  1997/03/11  01:36:51  taustin
 * updated copyright
 * long/int tweaks made for ALPHA target support
 * syscall structures are now more portable across platforms
 *
 * Revision 1.3  1996/12/27  15:56:56  taustin
 * updated comments
 *
 * Revision 1.1  1996/12/05  18:50:23  taustin
 * Initial revision
 *
 *
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include <sys/types.h>
#ifdef _MSC_VER
#include <time.h>
#else
#include <sys/time.h>
#endif

#include "host.h"
#include "misc.h"
#include "machine.h"

/*
 * This module implements the system call portion of the SimpleScalar
 * instruction set architecture.  The system call definitions are borrowed
 * from Ultrix.  All system calls are executed by the simulator (the host) on
 * behalf of the simulated program (the target). The basic procedure for
 * implementing a system call is as follows:
 *
 *	1) decode the system call (this is the enum in "syscode")
 *	2) copy system call inputs in target (simulated program) memory
 *	   to host memory (simulator memory), note: the location and
 *	   amount of memory to copy is system call specific
 *	3) the simulator performs the system call on behalf of the target prog
 *	4) copy system call results in host memory to target memory
 *	5) set result register to indicate the error status of the system call
 *
 * That's it...  If you encounter an unimplemented system call and would like
 * to add support for it, first locate the syscode and arguments for the system
 * call when it occurs (do this in the debugger) and then implement a proxy
 * procedure in syscall.c.
 *
 */


/* syscall proxy handler, architect registers and memory are assumed to be
   precise when this function is called, register and memory are updated with
   the results of the sustem call */
void
sys_syscall(struct regs_t *regs,	/* registers to access */
	    mem_access_fn mem_fn,	/* generic memory accessor */
	    struct mem_t *mem,		/* memory space to access */
	    md_inst_t inst,		/* system call inst */
	    int traceable);		/* traceable system call? */

/* this is a systemcall function type. each system call is handled by a separate function unlike alpha and pisa */
typedef void (* ppc_syscall)(struct regs_t *regs,
										struct mem_t *mem);

#ifdef TARGET_PPC

#define SYSCALL_VALUE_SBRK 1
#define SYSCALL_VALUE_BRK 2 
#define SYSCALL_VALUE_KWRITE 3
#define SYSCALL_VALUE_KIOCTL 4 
#define SYSCALL_VALUE_KFCNTL 5 
#define SYSCALL_VALUE_EXIT 6 
#define SYSCALL_VALUE_KREAD 7 
#define SYSCALL_VALUE_STATX 8 
#define SYSCALL_VALUE_OPEN 9 
#define SYSCALL_VALUE_CLOSE 10 
#define SYSCALL_VALUE_SIGPROCMASK 11
#define SYSCALL_VALUE_KLSEEK 12
#define SYSCALL_VALUE_SIGCLEANUP 13
#define SYSCALL_VALUE_LSEEK 14 
#define SYSCALL_VALUE_FSTATX 15 
#define SYSCALL_VALUE_GETGIDX 16 
#define SYSCALL_VALUE_GETUIDX 17 
#define SYSCALL_VALUE_GETPID 18 
#define SYSCALL_VALUE_KILL 19 
#define SYSCALL_VALUE_CREAT 20
#define SYSCALL_VALUE_ACCESS 21
#define SYSCALL_VALUE_ACCESSX 22
#define SYSCALL_VALUE_FACCESSX 23
#define SYSCALL_VALUE_FCLEAR 24

void syscall_kwrite(struct regs_t *regs, struct mem_t *mem, 
					mem_access_fn mem_fn);
void syscall_kread(struct regs_t *regs, struct mem_t *mem, 
				   mem_access_fn mem_fn);
void syscall_sbrk(struct regs_t *regs, struct mem_t *mem, 
				  mem_access_fn mem_fn);
void syscall_brk(struct regs_t *regs, struct mem_t *mem, 
				 mem_access_fn mem_fn);
void syscall_kioctl(struct regs_t *regs, struct mem_t *mem,
					mem_access_fn mem_fn);
void syscall_exit(struct regs_t *regs, struct mem_t *mem, 
				  mem_access_fn mem_fn);
void syscall_statx(struct regs_t *regs, struct mem_t *mem, 
				   mem_access_fn mem_fn);
void syscall_open(struct regs_t *regs, struct mem_t *mem, 
				  mem_access_fn mem_fn);
void syscall_close(struct regs_t *regs, struct mem_t *mem, 
				   mem_access_fn mem_fn);
void syscall_sigprocmask(struct regs_t *regs, struct mem_t *mem, 
						 mem_access_fn mem_fn);
void syscall_klseek(struct regs_t *regs, struct mem_t *mem, 
					mem_access_fn mem_fn);
void syscall_sigcleanup(struct regs_t *regs, struct mem_t *mem, 
						mem_access_fn mem_fn);
void syscall_kfcntl(struct regs_t *regs, struct mem_t *mem, 
					mem_access_fn mem_fn);
void syscall_lseek(struct regs_t *regs, struct mem_t *mem, 
				   mem_access_fn mem_fn);
void syscall_fstatx(struct regs_t *regs, struct mem_t *mem, 
					mem_access_fn mem_fn);
void syscall_getgidx(struct regs_t *regs, struct mem_t *mem,
					 mem_access_fn mem_fn);
void syscall_getuidx(struct regs_t *regs, struct mem_t *mem, 
					 mem_access_fn mem_fn);
void syscall_getpid(struct regs_t *regs, struct mem_t *mem, 
					mem_access_fn mem_fn);
void syscall_kill(struct regs_t *regs, struct mem_t *mem, 
				  mem_access_fn mem_fn);
void syscall_creat(struct regs_t *regs, struct mem_t *mem, 
				   mem_access_fn mem_fn);
void syscall_access(struct regs_t *regs, struct mem_t *mem, 
					mem_access_fn mem_fn);
void syscall_accessx(struct regs_t *regs, struct mem_t *mem, 
					 mem_access_fn mem_fn);
void syscall_faccessx(struct regs_t *regs, struct mem_t *mem, 
					  mem_access_fn mem_fn);
void syscall_fclear(struct regs_t *regs, struct mem_t *mem, 
					mem_access_fn mem_fn);

void syscall_unimplemented(struct regs_t *regs, struct mem_t *mem, 
						   mem_access_fn mem_fn);

void dosyscall(int r, struct regs_t *regs, struct mem_t *mem, mem_access_fn mem_fn);

#endif

#endif /* SYSCALL_H */
