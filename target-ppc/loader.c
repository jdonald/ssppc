/*
 * loader.c - program loader routines
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
 * $Id: loader.c,v 1.1.1.1 2001/08/10 14:58:25 karu Exp $
 *
 * $Log: loader.c,v $
 * Revision 1.1.1.1  2001/08/10 14:58:25  karu
 * creating ss3ppc-linux repository. The only difference between this and
 * the original ss3ppc linux is that crosss endian issues are taken
 * care of here.
 *
 *
 * Revision 1.1.1.1  2001/08/08 21:28:56  karu
 * creating ss3ppc linux repository
 *
 * Revision 1.16  2001/08/03 20:45:35  karu
 * checkpoint also works. checkpointing wasnt working previously because, i was always setting the CR register to a constant value at the begining of sim-fast. this is obviously wrong while doing checking pointing. so i removed that. and voila it works. also added the LR, CTR and TBR to the list of miscellaneous registers saved when a checkpoint is created. everything seems to work now.
 *
 * Revision 1.15  2001/08/03 06:12:28  karu
 * added support for EIO traces. checkpointing still does not seem to work
 *
 * Revision 1.14  2001/06/28 02:24:48  karu
 * added support for fclear system call. i havent seen a benchmark really use it so far. just added because i used this as an example for holger
 *
 * Revision 1.13  2001/06/27 03:58:58  karu
 * fixed errno setting in loader.c and added errno support to all syscalls. added 3 more syscalls access, accessx and faccessx to get NAS benchmark MG working. also added MTFB instruction from the patch AK mailed me looooong time back. the NAS benchmarks were also dying becuase of that instruction just like sphinx
 *
 * Revision 1.12  2001/06/05 09:41:03  karu
 * removed warnings and fixed sim-profile. there is still an addressing mode problem unresolved in sim-profile. sim-eio has some system call problems unresolved.
 *
 * Revision 1.11  2000/05/07 17:17:21  karu
 * added syscall_types
 *
 * Revision 1.10  2000/04/17 23:13:48  karu
 * removed out interactive commands from sim-fast and place them in a separate file
 *
 * Revision 1.9  2000/04/11 00:56:01  karu
 * made source platform independent. compiles on linux, solaris and ofcourse aix.
 * powerpc-nonnative.def is the def file with no inline assemble code.
 * manually create the machine.def symlink if compiling on non-aix OS.
 * also syscall.c does not compile because the syscalls haven't been
 * resourced with diff. flags and names for different operating systems.
 *
 * Revision 1.8  2000/04/07 20:50:37  karu
 * fixed problem in loader - argv[0] wasn't working in simulated programs
 * also added showmems to interactive commands
 *
 * Revision 1.7  2000/04/05 01:29:02  karu
 * fixed environ
 *
 * Revision 1.6  2000/04/04 01:31:24  karu
 * added a few more commands to debugger
 * and fixed a problem in system_configuration. thanks to ramdass for finding it out :-)
 *
 * Revision 1.5  2000/04/03 20:03:26  karu
 * entire specf95 working .
 *
 * Revision 1.4  2000/03/28 00:37:13  karu
 * added fstatx and added fdivs implementation
 *
 * Revision 1.3  2000/03/23 01:19:20  karu
 * added syscall lseek and defined errno
 *
 * Revision 1.2  2000/03/07 09:24:34  karu
 * fixed millicode 3280 encoding bug
 *
 * Revision 1.1.1.1  2000/03/07 05:15:21  karu
 * this is the repository created for my own maintanence.
 * created when spec95 (lisp and compress worked).
 * compress still had the scanf("%i") problem
 * DIFF from the repository I am using alongwith ramdass on /projects
 * need to merge the two sometime :-)
 *
 * Revision 1.1.1.1  2000/02/25 21:02:57  karu
 * creating cvs repository for ss3-ppc
 *
 * Revision 1.7  1998/08/27 16:57:46  taustin
 * implemented host interface description in host.h
 * added target interface support
 * added support for register and memory contexts
 * external event I/O (EIO) tracing and checkpointing added
 * improved loader error messages, e.g., loading Alpha binaries on
 *       PISA-configured simulator indicates such
 * fixed a BFD/non-BFD loader problem where tail padding in the text
 *       segment was not correctly allocated in simulator memory; when
 *       padding region happened to cross a page boundary the final text
 *       page has a NULL pointer in mem_table, resulting in a SEGV when
 *       trying to access it in intruction predecoding
 * instruction predecoding moved to loader module
 *
 * Revision 1.6  1997/04/16  22:09:05  taustin
 * added standalone loader support
 *
 * Revision 1.5  1997/03/11  01:12:39  taustin
 * updated copyright
 * swapping supported disabled until it can be tested further
 *
 * Revision 1.4  1997/01/06  15:59:22  taustin
 * stat_reg calls now do not initialize stat variable values
 * ld_prog_fname variable exported
 *
 * Revision 1.3  1996/12/27  15:51:28  taustin
 * updated comments
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "endian.h"
#include "regs.h"
#include "memory.h"
#include "sim.h"
#include "eio.h"
#include "loader.h"
#include "machine.h"

#include "syscall.h"

/* code added by karu begins */
#ifdef TARGET_PPC
/* the following struct is the _system_configuration structure */
/* from file /usr/include/sys/systemcfg.h */
typedef struct {
        int architecture;       /* processor architecture */
        int implementation;     /* processor implementation */
        int version;            /* processor version */
        int width;              /* width (32 || 64) */
        int ncpus;              /* 1 = UP, n = n-way MP */
        int cache_attrib;       /* cache attributes (bit flags)         */
                                /* bit          0/1 meaning             */
                                /* -------------------------------------*/
                                /* 31    no cache / cache present       */
                                /* 30    separate I and D / combined    */
        int icache_size;        /* size of L1 instruction cache */
        int dcache_size;        /* size of L1 data cache */
        int icache_asc;         /* L1 instruction cache associativity */
        int dcache_asc;         /* L1 data cache associativity */
        int icache_block;       /* L1 instruction cache block size */
        int dcache_block;       /* L1 data cache block size */
        int icache_line;        /* L1 instruction cache line size */
        int dcache_line;        /* L1 data cache line size */
        int L2_cache_size;      /* size of L2 cache, 0 = No L2 cache */
        int L2_cache_asc;       /* L2 cache associativity */
        int tlb_attrib;         /* TLB attributes (bit flags)           */
                                /* bit          0/1 meaning             */
                                /* -------------------------------------*/
                                /* 31    no TLB / TLB present           */
                                /* 30    separate I and D / combined    */
        int itlb_size;          /* entries in instruction TLB */
        int dtlb_size;          /* entries in data TLB */
        int itlb_asc;           /* instruction tlb associativity */
        int dtlb_asc;           /* data tlb associativity */
        int resv_size;          /* size of reservation */
        int priv_lck_cnt;       /* spin lock count in supevisor mode */
        int prob_lck_cnt;       /* spin lock count in problem state */
        int rtc_type;           /* RTC type */
        int virt_alias;         /* 1 if hardware aliasing is supported */
        int cach_cong;          /* number of page bits for cache synonym */
        int model_arch;         /* used by system for model determination */
        int model_impl;         /* used by system for model determination */
        int Xint;               /* used by system for time base conversion */
        int Xfrac;              /* used by system for time base conversion */
        int kernel;             /* kernel attributes                        */
                                /* bit          0/1 meaning                 */
                                /* -----------------------------------------*/
                                /* 31   32-bit kernel / 64-bit kernel       */
        long long physmem;      /* bytes of good memory                 */
        int slb_attr;           /* SLB attributes                           */
                                /* bit          0/1 meaning                 */
                                /* -----------------------------------------*/
                                /* 31           Software Manged             */
        int slb_size;           /* size of slb (0 = no slb)                 */
        int original_ncpus;     /* original number of CPUs */

} _system_configuration_struct;

/* this include contains the code to read the relocation table. required for implementing system calls */
/* the following code is the file aixloader.h. it has been pasted below */

/*
 * Copyright (C) 1996 by the Board of Trustees
 *    of Leland Stanford Junior University.
 * 
 * This file is part of the SimOS distribution. 
 * See LICENSE file for terms of the license. 
 *
 */

/*****************************************************************
 * solo_aix.c
 *
 *****************************************************************/

/* includes required by psim stuff */
// #include <xcoff.h>
#include <stdio.h>
#include <stdlib.h>

// #include <loader.h>
// #include <xcoff.h>
// #include <scnhdr.h>

#include "target-ppc/xcoff.h"

#include "machine.h"
#include "memory.h"
#include "syscall.h"

extern __thread md_addr_t errnoaddress;

void readrelocationtable(char *myfilename, struct mem_t *mem, struct regs_t regs, md_addr_t *sp, _system_configuration_struct _system_configuration) 
{
/* loader symbol table -- saved because it contains the names of imported
   system calls; we refer to it for the same reason as the loader section
   string table above */
	LDSYM *Ldsymtab;
/* the address at which simulatee's errno is stored in the simulator's address space */
	char *Ldrscn_strtbl;
   char *filename;
   FILE *fp;
   struct xcoffhdr xcoff_hdr;
   struct scnhdr section_hdr;
   struct scnhdr *section_hdr_array;
   int text_scnptr = 0, data_scnptr = 0, bss_scnptr = 0, loader_scnptr = 0;
   int text_scnsize = 0, data_scnsize = 0, bss_scnsize = 0, loader_scnsize = 0;
   int text_location = 0, data_location = 0, bss_location = 0;
   int i;
   LDHDR   loader_hdr;
   LDREL   ldrel;
   char *symname;
   char *mydata;
   int tempcount = 0;
   int val, syscalladdress, fnaddress;
   enum md_fault_type mem_ret;
   
   fprintf(stderr, "Inside loader segment\n");
   syscalladdress = PPC_SYSCALL_ADDRESS;
   filename = myfilename;
   if ((fp = fopen(filename, "rb")) == NULL) {
	 fprintf(stderr, "Could not open %s.\n", filename);
	 exit(1);
   }
   
   if (fread(&xcoff_hdr, sizeof(struct xcoffhdr), 1, fp) != 1) {
	 fprintf(stderr, "Could not read xcoff header.\n");
	 exit(1);
   }
   swap_xcoff_hdr(&xcoff_hdr);
	
   if ((xcoff_hdr.filehdr.f_flags & (F_EXEC | F_DYNLOAD)) !=
       (F_EXEC | F_DYNLOAD)) {
	 fprintf(stderr, "%s is not an executable.\n", filename);
	 exit(1);
   }
   
	/* allocate mem for section_hdr array */
   section_hdr_array = (struct scnhdr *) malloc (
												 sizeof(struct scnhdr) * 
												 xcoff_hdr.filehdr.f_nscns );
   
   /* locate relevant sections for further processing */

   for (i = 0;  i < xcoff_hdr.filehdr.f_nscns;  i++) {
	 if (fread(&section_hdr, SCNHSZ, 1, fp) != 1) {
	   fprintf(stderr, "Could not read section header");
	   return;
	 }
	 swap_shdr(&section_hdr);
	 memcpy(&section_hdr_array[i], &section_hdr, SCNHSZ);
	 
	 if ( (section_hdr.s_flags & STYP_TEXT) != 0) {
	   text_scnptr = section_hdr.s_scnptr;
	   text_scnsize = section_hdr.s_size;
	   text_location = section_hdr.s_vaddr;
	 } 
	 if ( (section_hdr.s_flags & STYP_DATA) != 0) {
	   data_scnptr = section_hdr.s_scnptr;
	   data_scnsize = section_hdr.s_size;
	   data_location = section_hdr.s_vaddr;
	 }
	 if ( (section_hdr.s_flags & STYP_BSS) != 0) {
	   bss_scnptr = section_hdr.s_scnptr;
	   bss_scnsize = section_hdr.s_size;
	   bss_location = section_hdr.s_vaddr;
	 }
	 if ( (section_hdr.s_flags & STYP_LOADER) != 0) {
	   loader_scnptr = section_hdr.s_scnptr;
	   loader_scnsize = section_hdr.s_size;
	 }
   }
	
   fseek(fp, data_scnptr, 0);
   mydata = (char *) malloc(data_scnsize);
   if (fread(mydata, data_scnsize, 1, fp) != 1) {
	 fprintf(stderr, "Unable to copy in data section.\n");
	 exit(1);
   } 
   free(mydata);		

   /* allocate and read string table from loader section */
	
   fseek (fp, loader_scnptr, 0);
   fread(&loader_hdr, LDHDRSZ, 1, fp);
   swap_ldhdr(&loader_hdr);
   if ((Ldrscn_strtbl = (char *)malloc(loader_hdr.l_stlen)) == NULL) {
	 fprintf(stderr, "Could not allocate loader string table.\n");
	 exit(1);
   } 
   fseek (fp, loader_scnptr + loader_hdr.l_stoff, 0);
   if (fread ((void *)Ldrscn_strtbl, loader_hdr.l_stlen, 1, fp) != 1) {
	 fprintf(stderr, "Unable to read in loader section string table.\n");
	 return;
   } 
   
   /* allocate symbol table */
   /* remember that there are three implicit symbols at the
      start -- .text, .data, and .bss.  We do not fill those entries in here
      because we do not refer to them */
   if ((Ldsymtab = (LDSYM *)calloc(loader_hdr.l_nsyms+3, LDSYMSZ)) == NULL) {
	 fprintf(stderr, "Could not allocate loader symbol table.\n");
	 return;
   }
   /* read symbol table in */
   fseek(fp, loader_scnptr + LDHDRSZ, 0);
   if (fread(&(Ldsymtab[3]), LDSYMSZ, loader_hdr.l_nsyms, fp) != 
	   loader_hdr.l_nsyms) {
	 fprintf(stderr, "Unable to read in loader section symbol table.\n");
	 return;
   }
   for (i = 0; i < loader_hdr.l_nsyms; i++) {
	 swap_ldsym(&(Ldsymtab[3+i]) ); 
   }   
   /* resolve unresolved imported symbols.  The address is found in the
      relocation table.  NOTE:  this code is only designed to handle
      statically bound executables whose unresolved symbols are imported 
      from /unix */
   for (i = 0;  i < loader_hdr.l_nreloc;  i++) {
	 if (fread(&ldrel, LDRELSZ, 1, fp) != 1) {
	   fprintf(stderr, "Unable to read relocation table entry.\n");
	   return;
	 }
	 swap_ldrel(&ldrel);	
	 if (ldrel.l_symndx >= 3 && Ldsymtab[ldrel.l_symndx].l_value == 0) {
	   tempcount++;
	   Ldsymtab[ldrel.l_symndx].l_value = ldrel.l_vaddr;
	 }
   }

   /* finished with executable file at this point */

   mem_ret = mem_access(mem, Read, 0x20010340, &i, sizeof(md_addr_t) );
   
   for (i = 3; i < loader_hdr.l_nsyms+3; i++ )  {
	 if (Ldsymtab[i]._l._l_l._l_zeroes != 0) {
	   // printf("%d %s %x %x %x\n", i, Ldsymtab[i].l_name, Ldsymtab[i].l_value, Ldsymtab[i].l_scnum, Ldsymtab[i].l_smtype);
	   symname = Ldsymtab[i]._l._l_name;
	 }
	 else  {
	   // printf("%d %s %x %x %x\n", i, (char *) &Ldrscn_strtbl[Ldsymtab[i].l_offset], Ldsymtab[i].l_value, Ldsymtab[i].l_scnum, Ldsymtab[i].l_smtype);
	   symname = (char *) &Ldrscn_strtbl[Ldsymtab[i]._l._l_l._l_offset];
	 }
	 //fprintf(stderr, "%s ", symname);
	 if (strcmp(symname, "environ") == 0) {
	   fprintf(stderr, "setting environ\n");
	   *sp -= sizeof(word_t);
	   /*
	   mem_bcopy(mem_access, mem, Write, *sp, &regs.regs_R[5], sizeof(word_t) );
	   mem_bcopy(mem_access, mem, Write, Ldsymtab[i].l_value, sp, 4);
	   */
	   /* use MEM_WRITE instead of mem_bcopy to handle byte swapping
		  for cross endian support 
	   */
	   MEM_WRITE_WORD(mem, Ldsymtab[i].l_value, (*sp) );
	   MEM_WRITE_WORD(mem, (*sp), ((int) regs.regs_R[5]) );
	   continue;
	 }
	 val = -1; 
	 fnaddress = (int) &syscall_unimplemented;
	 if (strcmp(symname, "sbrk") == 0) {
	   val = SYSCALL_VALUE_SBRK;
	   fnaddress = (int) &syscall_sbrk;
	 } else
	 if (strcmp(symname, "brk") == 0) {
	   val = SYSCALL_VALUE_BRK;
	   fnaddress = (int) &syscall_brk;
	 } else
	 if (strcmp(symname, "kwrite") == 0) {
	   val = SYSCALL_VALUE_KWRITE;
	   fnaddress = (int) &syscall_kwrite;
     } else
      if (strcmp(symname, "kread") == 0) {
         val = SYSCALL_VALUE_KREAD;
			fnaddress = (int) &syscall_kread;
      } else
      if (strcmp(symname, "kioctl") == 0) {
         val = SYSCALL_VALUE_KIOCTL;
			fnaddress = (int) &syscall_kioctl;
      } else
      if (strcmp(symname, "kfcntl") == 0) {
         val = SYSCALL_VALUE_KFCNTL;
			fnaddress = (int) &syscall_kfcntl;
      } else
      if (strcmp(symname, "_exit") == 0) {
         val = SYSCALL_VALUE_EXIT;
			fnaddress = (int) &syscall_exit;
      } else
      if (strcmp(symname, "statx") == 0) {
         val = SYSCALL_VALUE_STATX;
			fnaddress = (int) &syscall_statx;
      } else
		if (strcmp(symname, "open") == 0) {
         val = SYSCALL_VALUE_OPEN;
			fnaddress = (int) &syscall_open;
      } else
      if (strcmp(symname, "close") == 0) {
         val = SYSCALL_VALUE_CLOSE;
			fnaddress = (int) &syscall_close;
      } else 
      if (strcmp(symname, "sigprocmask") == 0) {
		val = SYSCALL_VALUE_SIGPROCMASK;
		fnaddress = (int) &syscall_sigprocmask;
	  } else
	  if (strcmp(symname, "klseek") == 0) {
         val = SYSCALL_VALUE_KLSEEK;
		 fnaddress = (int) &syscall_klseek;
      } else 
      if (strcmp(symname, "sigcleanup") == 0) {
         val = SYSCALL_VALUE_SIGCLEANUP;
			fnaddress = (int) &syscall_sigcleanup;
      } else if (strcmp(symname, "lseek") == 0) {
         val = SYSCALL_VALUE_LSEEK;
         fnaddress = (int) &syscall_lseek;
      } else if (strcmp(symname, "getgidx") == 0) {
         val = SYSCALL_VALUE_GETGIDX;
         fnaddress = (int) &syscall_getgidx;
      } else if (strcmp(symname, "getuidx") == 0) {
         val = SYSCALL_VALUE_GETUIDX;
         fnaddress = (int) &syscall_getuidx;
      } else if (strcmp(symname, "getpid") == 0) {
         val = SYSCALL_VALUE_GETPID;
         fnaddress = (int) &syscall_getpid;
      } else if (strcmp(symname, "creat") == 0) {
         val = SYSCALL_VALUE_CREAT;
         fnaddress = (int) &syscall_creat;
      } else if (strcmp(symname, "kill") == 0) {
         val = SYSCALL_VALUE_KILL;
         fnaddress = (int) &syscall_kill;
      } else if (strcmp(symname, "access") == 0) {
         val = SYSCALL_VALUE_ACCESS;
         fnaddress = (int) &syscall_access;
		} else if (strcmp(symname, "accessx") == 0) {
         val = SYSCALL_VALUE_ACCESSX;
         fnaddress = (int) &syscall_accessx;
		} else if (strcmp(symname, "faccessx") == 0) {
         val = SYSCALL_VALUE_FACCESSX;
         fnaddress = (int) &syscall_faccessx;
		} else if (strcmp(symname, "fclear") == 0) {
         val = SYSCALL_VALUE_FCLEAR;
         fnaddress = (int) &syscall_fclear;
		} else if (strcmp(symname, "errno") == 0) {
		  *sp -= sizeof(md_addr_t);
		  val = 0;
		  /*
		  mem_bcopy(mem_access, mem, Write, Ldsymtab[i].l_value, sp, 4);
		  mem_bcopy(mem_access, mem, Write, *sp, &val, 4);			
		  */
		  MEM_WRITE_WORD(mem, Ldsymtab[i].l_value, (*sp) );
		  MEM_WRITE_WORD(mem, (*sp), ((int) val) );
		  errnoaddress = *sp;
		  /* mem_bcopy(mem_access, mem, Write, errnoaddress, &val, 4);  */
		  MEM_WRITE_WORD(mem, errnoaddress, val);
		  /* set errno to zero */
		  fprintf(stderr, "setting errno %x\n", *sp);
		  continue;
	  } else if (strcmp(symname, "_system_configuration") == 0) {
		*sp -= sizeof(_system_configuration);
		
		mem_bcopy(mem_access, mem, Write, *sp, &_system_configuration, sizeof(_system_configuration) );
		/* mem_bcopy(mem_access, mem, Write, Ldsymtab[i].l_value, sp, 4); */
		MEM_WRITE_WORD(mem, Ldsymtab[i].l_value, (*sp) );
		fprintf(stderr, "system config\n");
		continue;
      } else if (strcmp(symname, "fstatx") == 0) {
         val = SYSCALL_VALUE_FSTATX;
         fnaddress = (int) &syscall_fstatx;
      }
 
	 *sp -= sizeof(md_addr_t)*2;
	 /* write out the R0 and R2 values read in syscall. */
	 /* i eventually ended up no using fnaddress
		at all. I have switch the function to call based on R2
		instead - i could have done fnaddres(). its too confusing 
	 */
		
	 MEM_WRITE_WORD(mem, Ldsymtab[i].l_value, (*sp) );
	 MEM_WRITE_WORD(mem, (*sp), 0 );
	 MEM_WRITE_WORD(mem, (*sp+4), ((int) val) );
	 if (val != -1) fprintf(stderr,"implemented\n" );
	 else fprintf(stderr, "NOT implemented %x\n", Ldsymtab[i].l_value ); 
   }

   fclose(fp);
   return;
}

/* the following lines are from millicode.h. pasted from there */

void writemillicode(struct mem_t *mem) {
	word_t data[] = { /* quous */ 0x3380, 0x7c632396, 0x4e800020, 0,
				/* quoss */ 0x3300, 0x7c6323d6, 0x4e800020, 0,
				/* divus */ 0x3280, 0x7c032396, 0x7c8021d6, 0x7c841850, 0x60600000, 				0x4e800020, 0,
				/* mulh */ 0x3100, 0x7c632096, 0x4e800020, 0,
				/* mull */ 0x3180, 0x7c0321d6,0x7c632096, 0x60800000, 0x4e800020, 0,
	
				/* divss */ 0x3200, 0x7c0323d6, 0x7c8021d6, 0x7c841850, 0x60600000,
            0x4e800020, 0 };		

	int i;
	int x;
	int DATAMAX;

	i = 0;
	DATAMAX = sizeof(data)/sizeof(word_t);
	while (i < DATAMAX) {
		for (x = i+1; data[x] != 0; x++) {
#ifdef PPC_DEBUG
			printf("writing millicode %x %x %x\n",data[i]+sizeof(word_t)*(x-i-1), x-(i+1), data[x]);
#endif
			//data[x] = MD_SWAPW(data[x]);
			mem_access(mem, Write, data[i]+ sizeof(word_t)*(x-i-1), &data[x], sizeof(word_t) );
		}
		i = x+1;
	}
}

void writemillicodepredecode(struct mem_t *mem) {
   word_t data[] = { /* quous */ 0x3380, 0x7c632396, 0x4e800020, 0,
            /* quoss */ 0x3300, 0x7c6323d6, 0x4e800020, 0,
            /* divus */ 0x3280, 0x7c032396, 0x7c8021d6, 0x7c841850, 0x60600000,
            0x4e800020, 0,
            /* mulh */ 0x3100, 0x7c632096, 0x4e800020, 0,
            /* mull */ 0x3180, 0x7c0321d6,0x7c632096, 0x60800000, 0x4e800020, 0,

            /* divss */ 0x3200, 0x7c0323d6, 0x7c8021d6, 0x7c841850, 0x60600000,
            0x4e800020, 0 };

   int i;
   int x;
	int inst;
   int DATAMAX;
	register enum md_opcode opdec;

   i = 0;
   DATAMAX = sizeof(data)/sizeof(word_t);
   while (i < DATAMAX) {
      for (x = i+1; data[x] != 0; x++) {
#ifdef PPC_DEBUG
         printf("writing millicode %x %x %x\n",data[i]+sizeof(word_t)*(x-i-1), x
-(i+1), data[x]);
#endif
			inst = data[x];
			MD_SET_OPCODE(opdec, inst);
			MEM_WRITE_WORD(mem, (data[i]+ sizeof(word_t)*(x-i-1)) << 1, MD_SWAPW( (word_t)opdec) ); 
        	MEM_WRITE_WORD(mem, ((data[i]+ sizeof(word_t)*(x-i-1)) << 1)+sizeof(word_t), MD_SWAPW(inst) );
			/*
         mem_access(mem, Write, (data[i]+ sizeof(word_t)*(x-i-1)), &data[x], sizeof(word_t) );
         mem_access(mem, Write, (data[i]+ sizeof(word_t)*(x-i-1), &data[x], sizeof(word_t) );
			*/
      }
      i = x+1;
   }
}


#endif
/* code added by karu ends */

#ifdef BFD_LOADER

#ifdef TARGET_PPC
#error BFD_LOADER not supported for PowerPC
#endif

#include <bfd.h>

#elif defined(TARGET_PPC)
// #include<xcoff.h>
#else
#include "target-pisa/ecoff.h"
#endif /* BFD_LOADER */

/* amount of tail padding added to all loaded text segments */
#define TEXT_TAIL_PADDING 128

/* program text (code) segment base */
__thread md_addr_t ld_text_base = 0; // jdonald missed this hunting reentrant bugs

/* program text (code) size in bytes */
__thread unsigned int ld_text_size = 0;

/* program initialized data segment base */
__thread md_addr_t ld_data_base = 0;

/* program initialized ".data" and uninitialized ".bss" size in bytes */
__thread unsigned int ld_data_size = 0;

/* top of the data segment */
__thread md_addr_t ld_brk_point = 0;

/* program stack segment base (highest address in stack) */
__thread md_addr_t ld_stack_base = MD_STACK_BASE;

/* program initial stack size */
__thread unsigned int ld_stack_size = 0;

/* lowest address accessed on the stack */
__thread md_addr_t ld_stack_min = (md_addr_t)-1;

/* program file name */
__thread char *ld_prog_fname = NULL;

/* program entry point (initial PC) */
__thread md_addr_t ld_prog_entry = 0;

/* program environment base address address */
__thread md_addr_t ld_environ_base = 0;

/* target executable endian-ness, non-zero if big endian */
__thread int ld_target_big_endian;

/* register simulator-specific statistics */
void
ld_reg_stats(struct stat_sdb_t *sdb)	/* stats data base */
{
  stat_reg_addr(sdb, "ld_text_base",
		"program text (code) segment base",
		&ld_text_base, ld_text_base, "  0x%08p");
  stat_reg_uint(sdb, "ld_text_size",
		"program text (code) size in bytes",
		&ld_text_size, ld_text_size, NULL);
  stat_reg_addr(sdb, "ld_data_base",
		"program initialized data segment base",
		&ld_data_base, ld_data_base, "  0x%08p");
  stat_reg_uint(sdb, "ld_data_size",
		"program init'ed `.data' and uninit'ed `.bss' size in bytes",
		&ld_data_size, ld_data_size, NULL);
  stat_reg_addr(sdb, "ld_stack_base",
		"program stack segment base (highest address in stack)",
		&ld_stack_base, ld_stack_base, "  0x%08p");
  stat_reg_uint(sdb, "ld_stack_size",
		"program initial stack size",
		&ld_stack_size, ld_stack_size, NULL);
#if 0 /* FIXME: broken... */
  stat_reg_addr(sdb, "ld_stack_min",
		"program stack segment lowest address",
		&ld_stack_min, ld_stack_min, "  0x%08p");
#endif
  stat_reg_addr(sdb, "ld_prog_entry",
		"program entry point (initial PC)",
		&ld_prog_entry, ld_prog_entry, "  0x%08p");
  stat_reg_addr(sdb, "ld_environ_base",
		"program environment base address address",
		&ld_environ_base, ld_environ_base, "  0x%08p");
  stat_reg_int(sdb, "ld_target_big_endian",
	       "target executable endian-ness, non-zero if big endian",
	       &ld_target_big_endian, ld_target_big_endian, NULL);
}


/* load program text and initialized data into simulated virtual memory
   space and initialize program segment range variables */
void
ld_load_prog(char *fname,		/* program to load */
	     int argc, char **argv,	/* simulated program cmd line args */
	     char **envp,		/* simulated program environment */
	     struct regs_t *regs,	/* registers to initialize for load */
	     struct mem_t *mem,		/* memory space to load prog into */
	     int zero_bss_segs)		/* zero uninit data segment? */
{
  int i;
  md_addr_t sp, originalsp, currsp, zeroptr, r4ptr, r5ptr, data_break = 0; 
  int spdiff;
  char name[255];
  enum md_fault_type mem_ret;
  char **tempenvp;
	_system_configuration_struct _system_configuration; 

	zeroptr = 0;  /* constant zero for null padding */
  if (eio_valid(fname))
    {
      if (argc != 1)
	{
	  fprintf(stderr, "error: EIO file has arguments\n");
	  exit(1);
	}
      fprintf(stderr, "sim: loading EIO file: %s\n", fname);

      sim_eio_fname = mystrdup(fname);

      /* open the EIO file stream */
      sim_eio_fd = eio_open(fname);

      /* load initial state checkpoint */
      if (eio_read_chkpt(regs, mem, sim_eio_fd) != -1)
	fatal("bad initial checkpoint in EIO file");

      /* load checkpoint? */
      if (sim_chkpt_fname != NULL)
	{
	  counter_t restore_icnt;

	  FILE *chkpt_fd;

	  fprintf(stderr, "sim: loading checkpoint file: %s\n",
		  sim_chkpt_fname);

	  if (!eio_valid(sim_chkpt_fname))
	    fatal("file `%s' does not appear to be a checkpoint file",
		  sim_chkpt_fname);

	  /* open the checkpoint file */
	  chkpt_fd = eio_open(sim_chkpt_fname);

	  /* load the state image */
	  restore_icnt = eio_read_chkpt(regs, mem, chkpt_fd);

	  /* fast forward the baseline EIO trace to checkpoint location */
	  fprintf(stderr, "sim: fast forwarding to instruction %d\n",
		  (int)restore_icnt);
	  eio_fast_forward(sim_eio_fd, restore_icnt);
	  fprintf(stderr, "starting from %d\n", (int) sim_num_insn);
	}

      /* computed state... */
      ld_environ_base = regs->regs_R[MD_REG_SP];
      ld_prog_entry = regs->regs_PC;

      /* fini... */
      return;
    }

  if (sim_chkpt_fname != NULL)
    fatal("checkpoints only supported while EIO tracing");

#ifdef BFD_LOADER

#ifdef TARGET_PPC
#error BFD_LOADER not implemented for PowerPC
#endif
  {
    bfd *abfd;
    asection *sect;

    /* set up a local stack pointer, this is where the argv and envp
       data is written into program memory */
    ld_stack_base = MD_STACK_BASE;
    sp = ROUND_DOWN(MD_STACK_BASE - MD_MAX_ENVIRON, sizeof(dfloat_t));
    ld_stack_size = ld_stack_base - sp;

    /* initial stack pointer value */
    ld_environ_base = sp;

    /* load the program into memory, try both endians */
    if (!(abfd = bfd_openr(argv[0], "ss-coff-big")))
      if (!(abfd = bfd_openr(argv[0], "ss-coff-little")))
	fatal("cannot open executable `%s'", argv[0]);

    /* this call is mainly for its side effect of reading in the sections.
       we follow the traditional behavior of `strings' in that we don't
       complain if we don't recognize a file to be an object file.  */
    if (!bfd_check_format(abfd, bfd_object))
      {
	bfd_close(abfd);
	fatal("cannot open executable `%s'", argv[0]);
      }

    /* record profile file name */
    ld_prog_fname = argv[0];

    /* record endian of target */
    ld_target_big_endian = abfd->xvec->byteorder_big_p;

    debug("processing %d sections in `%s'...",
	  bfd_count_sections(abfd), argv[0]);

    /* read all sections in file */
    for (sect=abfd->sections; sect; sect=sect->next)
      {
	char *p;

	debug("processing section `%s', %d bytes @ 0x%08x...",
	      bfd_section_name(abfd, sect), bfd_section_size(abfd, sect),
	      bfd_section_vma(abfd, sect));

	/* read the section data, if allocated and loadable and non-NULL */
	if ((bfd_get_section_flags(abfd, sect) & SEC_ALLOC)
	    && (bfd_get_section_flags(abfd, sect) & SEC_LOAD)
	    && bfd_section_vma(abfd, sect)
	    && bfd_section_size(abfd, sect))
	  {
	    /* allocate a section buffer */
	    p = calloc(bfd_section_size(abfd, sect), sizeof(char));
	    if (!p)
	      fatal("cannot allocate %d bytes for section `%s'",
		    bfd_section_size(abfd, sect),
		    bfd_section_name(abfd, sect));

	    if (!bfd_get_section_contents(abfd, sect, p, (file_ptr)0,
					  bfd_section_size(abfd, sect)))
	      fatal("could not read entire `%s' section from executable",
		    bfd_section_name(abfd, sect));

	    /* copy program section it into simulator target memory */
	    mem_bcopy(mem_access, mem, Write, bfd_section_vma(abfd, sect),
		      p, bfd_section_size(abfd, sect));

	    /* release the section buffer */
	    free(p);
	  }
	/* zero out the section if it is loadable but not allocated in exec */
	else if (zero_bss_segs
		 && (bfd_get_section_flags(abfd, sect) & SEC_LOAD)
		 && bfd_section_vma(abfd, sect)
		 && bfd_section_size(abfd, sect))
	  {
	    /* zero out the section region */
	    mem_bzero(mem_access, mem,
		      bfd_section_vma(abfd, sect),
		      bfd_section_size(abfd, sect));
	  }
	else
	  {
	    /* else do nothing with this section, it's probably debug data */
	    debug("ignoring section `%s' during load...",
		  bfd_section_name(abfd, sect));
	  }

	/* expected text section */
	if (!strcmp(bfd_section_name(abfd, sect), ".text"))
	  {
	    /* .text section processing */
	    ld_text_size =
	      ((bfd_section_vma(abfd, sect) + bfd_section_size(abfd, sect))
	       - MD_TEXT_BASE)
		+ /* for speculative fetches/decodes */TEXT_TAIL_PADDING;

	    /* create tail padding and copy into simulator target memory */
	    mem_bzero(mem_access, mem,
		      bfd_section_vma(abfd, sect)
		      + bfd_section_size(abfd, sect),
		      TEXT_TAIL_PADDING);
	  }
	/* expected data sections */
	else if (!strcmp(bfd_section_name(abfd, sect), ".rdata")
		 || !strcmp(bfd_section_name(abfd, sect), ".data")
		 || !strcmp(bfd_section_name(abfd, sect), ".sdata")
		 || !strcmp(bfd_section_name(abfd, sect), ".bss")
		 || !strcmp(bfd_section_name(abfd, sect), ".sbss"))
	  {
	    /* data section processing */
	    if (bfd_section_vma(abfd, sect) + bfd_section_size(abfd, sect) >
		data_break)
	      data_break = (bfd_section_vma(abfd, sect) +
			    bfd_section_size(abfd, sect));
	  }
	else
	  {
	    /* what is this section??? */
	    fatal("encountered unknown section `%s', %d bytes @ 0x%08x",
		  bfd_section_name(abfd, sect), bfd_section_size(abfd, sect),
		  bfd_section_vma(abfd, sect));
	  }
      }

    /* compute data segment size from data break point */
    ld_text_base = MD_TEXT_BASE;
    ld_data_base = MD_DATA_BASE;
    ld_prog_entry = bfd_get_start_address(abfd);
    ld_data_size = data_break - ld_data_base;

    /* done with the executable, close it */
    if (!bfd_close(abfd))
      fatal("could not close executable `%s'", argv[0]);
  }

#elif  defined( TARGET_PPC)
  {
    FILE *fobj;
    long floc;
    FILHDR fhdr;
    AOUTHDR ahdr;
    SCNHDR shdr;
	 enum md_fault_type mem_ret;

    byte_t *ptemp = (byte_t *) malloc(sizeof(byte_t));
	
    /* set up a local stack pointer, this is where the argv and envp
       data is written into program memory */
    ld_stack_base = MD_STACK_BASE;
    sp = ROUND_DOWN(MD_STACK_BASE - MD_MAX_ENVIRON, sizeof(word_t));
    ld_stack_size = ld_stack_base - sp;

    /* initial stack pointer value */
    ld_environ_base = sp;

    /* record profile file name */
    ld_prog_fname = argv[0];

        /* load the program into memory, try both endians */
#if defined(__CYGWIN32__) || defined(_MSC_VER)
    fobj = fopen(argv[0], "rb");
#else
    fobj = fopen(argv[0], "r");
#endif
    if (!fobj)
      fatal("cannot open executable `%s'", argv[0]);

    if (fread(&fhdr, sizeof(FILHDR), 1, fobj) < 1)
      fatal("cannot read header from executable `%s'", argv[0]);
    swap_fhdr(&fhdr);
 
    /* record endian of target */
    if (fhdr.f_magic == 0x1df)
      ld_target_big_endian = TRUE;
    else
      fatal("bad magic number 0x%x in executable `%s'",fhdr.f_magic, argv[0]);

    if (fread(&ahdr, sizeof(AOUTHDR), 1, fobj) < 1)
      fatal("cannot read AOUT header from executable `%s'", argv[0]);
	 swap_ahdr(&ahdr);
    data_break = ahdr.data_start + ahdr.dsize + ahdr.bsize;

    
    debug("processing %d sections in `%s'...", fhdr.f_nscns, argv[0]);

    /*Read text section*/
    if (fread(&shdr, sizeof(SCNHDR), 1, fobj) < 1)
      fatal("could not read section %d from executable", i);
    else{
      
      char *p;
	   swap_shdr(&shdr); 
      floc =ftell(fobj);
      ld_text_size = ((shdr.s_vaddr + shdr.s_size) - ahdr.text_start) 
	/*+ TEXT_TAIL_PADDING*/;
      
      p = calloc(shdr.s_size, sizeof(char));
      if (!p)
	fatal("out of virtual memory");

      if (fseek(fobj, shdr.s_scnptr, 0) == -1)
	fatal("could not read `.text' from executable", i);
      if (fread(p, shdr.s_size, 1, fobj) < 1) {
	fatal("could not read text section from executable 1265");
	}
     
      /* copy program section into simulator target memory */
      mem_bcopy(mem_access, mem, Write, shdr.s_vaddr, p, shdr.s_size);

      /* create tail padding and copy into simulator target memory */
      mem_bzero(mem_access, mem,
		shdr.s_vaddr + shdr.s_size, TEXT_TAIL_PADDING);
      
      /* release the section buffer */
      free(p);
    }
    if (fseek(fobj, floc, 0) == -1)
      fatal("could not reset location in executable");
    /*Read ".pad" section*/
    
    if (fread(&shdr, sizeof(SCNHDR), 1, fobj) < 1)
      { fatal("could not read section %d from executable", i);}
	/* code added by karu begins */
	/* for some reasons the actual data segment is not read 
	from the file and written to the simulated memory.
	the following code does that! needs to have some error checking maybe
	to be dont later! ??? */
	/* for some reason the next section is labelled Read Data section.
	but it actually reads the .bss (uninitialised data).
	confirm with manish, doug, steve????
	*/
	else {
		char *p;
		swap_shdr(&shdr); 
		p = calloc(shdr.s_size, sizeof(char) );

		floc = ftell(fobj);  /* save the current pos in file
		so that we can come back to it to read the .bss section */

		if (!p) fatal("out of virtual memory %s", shdr.s_name);
		if (fseek(fobj, shdr.s_scnptr, 0) == -1)
		    fatal("could not read `.data' from executable", i);
      	if (fread(p, shdr.s_size, 1, fobj) < 1)
		    fatal("could not read data section from executable");
 
	     /* copy data section it into simulator target memory */
    	mem_ret = mem_bcopy(mem_access, mem, Write, shdr.s_vaddr, p, shdr.s_size);
		free(p);
	}

	if (fseek(fobj, floc, 0) == -1)
  		fatal("could not reset location in executable to .bss after reading .data");

	/* code added by karu ends */	  
 
    /*Read bss section*/
	 
    if (fread(&shdr, sizeof(SCNHDR), 1, fobj) < 1){
      fatal("could not read section %d from executable", i);
    }
    else{
		/*
      char* p;
      p = calloc(shdr.s_size, sizeof(char));
      if (!p)
	fatal("out of virtual memory %s", shdr.s_name);
			
      floc = ftell(fobj);      
      if (fseek(fobj, shdr.s_scnptr, 0) == -1)
	fatal("could not read `.text' from executable", i);
      if (fread(p, shdr.s_size, 1, fobj) < 1) {
	fatal("could not read text section from executable 1330");
		}
      mem_ret = mem_bcopy(mem_access, mem, Write, shdr.s_vaddr, p, shdr.s_size);
      free(p);
	  fseek(fobj, floc, 0);
		*/	
		/* dont need to read from binary. just initiliase bss to zero */
	  swap_shdr( &shdr);
      mem_ret = mem_bzero(mem_access, mem, shdr.s_vaddr, shdr.s_size);
    }
	
    
    if (fread(&shdr, sizeof(SCNHDR), 1, fobj) < 1){
      fatal("could not read section %d from executable", i);
    }
    else{
      char* p;
		swap_shdr(&shdr); 
      p = calloc(shdr.s_size, sizeof(char));
      if (!p)
        fatal("out of virtual memory %s", shdr.s_name);
       
      floc = ftell(fobj);
      if (fseek(fobj, shdr.s_scnptr, 0) == -1)
        fatal("could not read `.loader' from executable", i);
      if (fread(p, shdr.s_size, 1, fobj) < 1)
        fatal("could not read loader section from executable");
       
      /* copy program section it into simulator target memory */
      mem_ret = mem_bcopy(mem_access, mem, Write, shdr.s_vaddr, p, shdr.s_size);

      /* release the section buffer */
      free(p);

    }	
    /* compute data segment size from data break point */
    ld_text_base = ahdr.text_start;
    ld_data_base = ahdr.data_start;
    ld_prog_entry = ahdr.text_start;
    ld_data_size = data_break - ld_data_base;

    /* stuff added by karu begins */
    *((byte_t *)ptemp) = MEM_READ_BYTE(mem, ld_text_base);
    *((byte_t *)ptemp) = MEM_READ_BYTE(mem, ld_text_base+1);
    *((byte_t *)ptemp) = MEM_READ_BYTE(mem, ld_text_base+2);
    *((byte_t *)ptemp) = MEM_READ_BYTE(mem, ld_text_base+3);
    *((byte_t *)ptemp) = MEM_READ_BYTE(mem, ld_data_base);

	/* the next line is a hack to make environ reference in loader work */
    // mem_bcopy(mem_access, mem, Write, 0x20010288+1064, &sp, 4);

    *((byte_t *)ptemp) = MEM_READ_BYTE(mem, 0x20010288+1064); 
    *((byte_t *)ptemp) = MEM_READ_BYTE(mem, 0x20010288+1064+1); 
    *((byte_t *)ptemp) = MEM_READ_BYTE(mem, 0x20010288+1064+2); 
    *((byte_t *)ptemp) = MEM_READ_BYTE(mem, 0x20010288+1064+3); 
    *((byte_t *)ptemp) = MEM_READ_BYTE(mem, 0x20011848); 

    /* stuff added by karu ends */
    /* done with the executable, close it */
    if (fclose(fobj))
      fatal("could not close executable `%s'", argv[0]);
 
	/* code added by karu begins */
	/* this loads the required toc value in R2. this is found in ahdr.o_toc field. required only for PPC. this code should eventually be moved somewhere else... */
	#ifdef TARGET_PPC
	regs->regs_R[2] = ahdr.o_toc;
	#endif
	/* code added by karu ends */


  }

#else

  {
    FILE *fobj;
    long floc;
    struct ecoff_filehdr fhdr;
    struct ecoff_aouthdr ahdr;
    struct ecoff_scnhdr shdr;

    /* set up a local stack pointer, this is where the argv and envp
       data is written into program memory */
    ld_stack_base = MD_STACK_BASE;
    sp = ROUND_DOWN(MD_STACK_BASE - MD_MAX_ENVIRON, sizeof(dfloat_t));
    ld_stack_size = ld_stack_base - sp;

    /* initial stack pointer value */
    ld_environ_base = sp;

    /* record profile file name */
    ld_prog_fname = argv[0];

    /* load the program into memory, try both endians */
#if defined(__CYGWIN32__) || defined(_MSC_VER)
    fobj = fopen(argv[0], "rb");
#else
    fobj = fopen(argv[0], "r");
#endif
    if (!fobj)
      fatal("cannot open executable `%s'", argv[0]);

    if (fread(&fhdr, sizeof(struct ecoff_filehdr), 1, fobj) < 1)
      fatal("cannot read header from executable `%s'", argv[0]);

    /* record endian of target */
    if (fhdr.f_magic == ECOFF_EB_MAGIC)
      ld_target_big_endian = TRUE;
    else if (fhdr.f_magic == ECOFF_EL_MAGIC)
      ld_target_big_endian = FALSE;
    else if (fhdr.f_magic == ECOFF_EB_OTHER || fhdr.f_magic == ECOFF_EL_OTHER)
      fatal("PISA binary `%s' has wrong endian format", argv[0]);
    else if (fhdr.f_magic == ECOFF_ALPHAMAGIC)
      fatal("PISA simulator cannot run Alpha binary `%s'", argv[0]);
    else
      fatal("bad magic number 0x%x in executable `%s' (not an executable?)",fhdr.f_magic, argv[0]);

    if (fread(&ahdr, sizeof(struct ecoff_aouthdr), 1, fobj) < 1)
      fatal("cannot read AOUT header from executable `%s'", argv[0]);

    data_break = MD_DATA_BASE + ahdr.dsize + ahdr.bsize;

    /* seek to the beginning of the first section header, the file header comes
       first, followed by the optional header (this is the aouthdr), the size
       of the aouthdr is given in Fdhr.f_opthdr */
    fseek(fobj, sizeof(struct ecoff_filehdr) + fhdr.f_opthdr, 0);

    debug("processing %d sections in `%s'...", fhdr.f_nscns, argv[0]);

    /* loop through the section headers */
    floc = ftell(fobj);
    for (i = 0; i < fhdr.f_nscns; i++)
      {
	char *p;

	if (fseek(fobj, floc, 0) == -1)
	  fatal("could not reset location in executable");
	if (fread(&shdr, sizeof(struct ecoff_scnhdr), 1, fobj) < 1)
	  fatal("could not read section %d from executable", i);
	floc = ftell(fobj);

	switch (shdr.s_flags)
	  {
	  case ECOFF_STYP_TEXT:
	    ld_text_size = ((shdr.s_vaddr + shdr.s_size) - MD_TEXT_BASE) 
	      + TEXT_TAIL_PADDING;

	    p = calloc(shdr.s_size, sizeof(char));
	    if (!p)
	      fatal("out of virtual memory");

	    if (fseek(fobj, shdr.s_scnptr, 0) == -1)
	      fatal("could not read `.text' from executable", i);
	    if (fread(p, shdr.s_size, 1, fobj) < 1)
	      fatal("could not read text section from executable 1483");

	    /* copy program section into simulator target memory */
	    mem_bcopy(mem_access, mem, Write, shdr.s_vaddr, p, shdr.s_size);

	    /* create tail padding and copy into simulator target memory */
	    mem_bzero(mem_access, mem,
		      shdr.s_vaddr + shdr.s_size, TEXT_TAIL_PADDING);
  
	    /* release the section buffer */
	    free(p);

	    break;

	  case ECOFF_STYP_RDATA:
	    /* The .rdata section is sometimes placed before the text
	     * section instead of being contiguous with the .data section.
	     */

	    /* fall through */
	  case ECOFF_STYP_DATA:
	    /* fall through */
	  case ECOFF_STYP_SDATA:

	    p = calloc(shdr.s_size, sizeof(char));
	    if (!p)
	      fatal("out of virtual memory");

	    if (fseek(fobj, shdr.s_scnptr, 0) == -1)
	      fatal("could not read `.text' from executable", i);
	    if (fread(p, shdr.s_size, 1, fobj) < 1)
	      fatal("could not read text section from executable 1514");

	    /* copy program section it into simulator target memory */
	    mem_bcopy(mem_access, mem, Write, shdr.s_vaddr, p, shdr.s_size);

	    /* release the section buffer */
	    free(p);

	    break;

	  case ECOFF_STYP_BSS:
	    break;

	  case ECOFF_STYP_SBSS:
	    break;
	  }
      }

    /* compute data segment size from data break point */
    ld_text_base = MD_TEXT_BASE;
    ld_data_base = MD_DATA_BASE;
    ld_prog_entry = ahdr.entry;
    ld_data_size = data_break - ld_data_base;

    /* done with the executable, close it */
    if (fclose(fobj))
      fatal("could not close executable `%s'", argv[0]);
  }

#endif /* BFD_LOADER */

  /* perform sanity checks on segment ranges */
  if (!ld_text_base || !ld_text_size)
    fatal("executable is missing a `.text' section");
  if (!ld_data_base || !ld_data_size)
    fatal("executable is missing a `.data' section");
  if (!ld_prog_entry)
    fatal("program entry point not specified");

  /* determine byte/words swapping required to execute on this host */
  sim_swap_bytes = (endian_host_byte_order() != endian_target_byte_order());
  if (sim_swap_bytes)
    {
		/* FIXME: disabled until further notice... */
      /* cross-endian is never reliable, why this is so is beyond the scope
	 of this comment, e-mail me for details... */
      fprintf(stderr, "sim: *WARNING*: swapping bytes to match host...\n");
      fprintf(stderr, "sim: *WARNING*: swapping may break your program!\n");
    }
  sim_swap_words = (endian_host_word_order() != endian_target_word_order());
  if (sim_swap_words)
    {
		/* FIXME: disabled until further notice... */
      /* cross-endian is never reliable, why this is so is beyond the scope
	 of this comment, e-mail me for details... */
      fprintf(stderr, "sim: *WARNING*: swapping words to match host...\n");
      fprintf(stderr, "sim: *WARNING*: swapping may break your program!\n");
    }

/* the previous segment was written assuming stack grows down */
/* the current code assume stack grows up */

	_system_configuration.architecture = MD_SWAPW(0x02);
	_system_configuration.implementation = MD_SWAPW(0x10); /* POWER_604 */

	/*
	readrelocationtable(argv[0], mem, &sp, _system_configuration);
	writemillicode(mem);
	*/

	tempenvp = (char **) malloc(sizeof(char *) * 4);
	tempenvp[0] = (char *) malloc(256);
	tempenvp[1] = (char *) malloc(256);
	tempenvp[2] = (char *) malloc(256);
	tempenvp[3] = (char *) malloc(256);
	strcpy(tempenvp[0], "s1");
	strcpy(tempenvp[1], "s1");
	strcpy(tempenvp[2], "s1");
	tempenvp[3] = NULL;
        
	envp = tempenvp;


	originalsp = sp;
	for (i = 0; envp[i]; i++) {
		sp -= (strlen(envp[i]) +1);
		mem_ret = mem_strcpy(mem_access, mem, Write, sp, envp[i]);
		//printf("%s\n", envp[i]);
		if (mem_ret == md_fault_alignment) {
			printf("unaligned mem_strcpy\n");
		}
	}
	for (i = argc-1; i >= 0 ; i--) {
		sp -= (strlen(argv[i]) + 1);
		mem_ret = mem_strcpy(mem_access, mem, Write, sp, argv[i]);
	}
	currsp = sp; sp = originalsp;
	spdiff = (currsp % 4);
	if (spdiff) 
		currsp -= spdiff;

   currsp -= sizeof(md_addr_t);
   mem_access(mem, Write, currsp, &zeroptr , sizeof(md_addr_t) );

	for (i = 0; envp[i]; i++) {
		currsp -= sizeof(md_addr_t);
		sp -= (strlen(envp[i]) + 1);
		//mem_ret = mem_access(mem, Write, currsp, &sp , sizeof(md_addr_t) );
		MEM_WRITE_WORD(mem, currsp, sp);
	}

	r5ptr = currsp;
	currsp -= sizeof(md_addr_t);
	//mem_access(mem, Write, currsp, &zeroptr , sizeof(md_addr_t) );
	MEM_WRITE_WORD(mem, currsp, zeroptr);

   for (i = argc-1; i >= 0; i--) {
		currsp -= sizeof(md_addr_t);
      sp -= (strlen(argv[i]) + 1);
		// mem_ret = mem_access(mem, Write, currsp, &sp, sizeof(md_addr_t) );
		MEM_WRITE_WORD(mem, currsp, sp);
   }

	r4ptr = currsp;

	sp = currsp;
   for (i = 0; i < 18; i++) {
      sp -= sizeof(md_addr_t);
      //mem_access(mem, Write, sp, &zeroptr, sizeof(md_addr_t) );
		MEM_WRITE_WORD(mem, sp, zeroptr);
   }
	sp -= sizeof(md_addr_t);

	mem_access(mem, Read, 0x20010288+1064, &currsp, 4);
	mem_access(mem, Read, currsp, &originalsp, 4);	
	mem_access(mem, Read, originalsp, &currsp, 4);	
	mem_ret = mem_strcpy(mem_access, mem, Read, currsp, name);

/* code added by karu ends for creating the stack */
	regs->regs_R[5] = r5ptr;
	readrelocationtable(argv[0], mem, *regs, &sp, _system_configuration);
   writemillicode(mem);


  /* did we tromp off the stop of the stack? */
  if (sp > ld_stack_base)
    {
      /* we did, indicate to the user that MD_MAX_ENVIRON must be increased,
	 alternatively, you can use a smaller environment, or fewer
	 command line arguments */
      fatal("environment overflow, increase MD_MAX_ENVIRON in ss.h");
    }

  /* initialize the bottom of heap to top of data segment */
  ld_brk_point = ROUND_UP(ld_data_base + ld_data_size, MD_PAGE_SIZE);

  /* set initial minimum stack pointer value to initial stack value */
  ld_stack_min = regs->regs_R[MD_REG_SP];

  /* set up initial register state */
  regs->regs_R[MD_REG_SP] = ld_environ_base;
  /* code added by karu begins */
  regs->regs_R[MD_REG_SP] = sp;
  regs->regs_R[3] = argc;
  regs->regs_R[4] = r4ptr;
  regs->regs_R[5] = r5ptr; /* redundat here. see 10 lines back */

  /* code added by karu ends */
  regs->regs_PC = ld_prog_entry;

  debug("ld_text_base: 0x%08x  ld_text_size: 0x%08x",
	ld_text_base, ld_text_size);
  debug("ld_data_base: 0x%08x  ld_data_size: 0x%08x",
	ld_data_base, ld_data_size);
  debug("ld_stack_base: 0x%08x  ld_stack_size: 0x%08x",
	ld_stack_base, ld_stack_size);
  debug("ld_prog_entry: 0x%08x", ld_prog_entry);
}
