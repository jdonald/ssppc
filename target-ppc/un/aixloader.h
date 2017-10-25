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
#include <xcoff.h>
#include <stdio.h>
#include <stdlib.h>

#include <loader.h>
#include <xcoff.h>
#include <scnhdr.h>

#include "machine.h"
#include "memory.h"
#include "syscall.h"

void readrelocationtable(char *myfilename, struct mem_t *mem, md_addr_t *sp) 
{
/* loader symbol table -- saved because it contains the names of imported
   system calls; we refer to it for the same reason as the loader section
   string table above */
	LDSYM *Ldsymtab;
/* the address at which simulatee's errno is stored in the simulator's address space */
	int *Errno_address;
	char *Ldrscn_strtbl;
   char *filename;
   FILE *fp;
   struct xcoffhdr xcoff_hdr;
   struct scnhdr section_hdr;
	struct scnhdr *section_hdr_array;
   int text_scnptr = 0, data_scnptr = 0, bss_scnptr = 0, loader_scnptr = 0;
   int text_scnsize = 0, data_scnsize = 0, bss_scnsize = 0, loader_scnsize = 0;
   int text_location = 0, data_location = 0, bss_location = 0;
   int i, j;
   unsigned *stack_top;
   unsigned *p, *q;
   char **argp;
   int nenv;
   unsigned int argsize;
   char **new_arg_and_env_p;
   char *new_argument_start;
   char *arg_and_env_str_p;
   struct func_desc *entry;
   LDHDR   loader_hdr;
   LDREL   ldrel;
	RELOC   snrel;
   char *symname;
	char *mydata;
	int tempcount = 0;
	int val, syscalladdress;
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

   if ((xcoff_hdr.filehdr.f_flags & (F_EXEC | F_DYNLOAD)) !=
       (F_EXEC | F_DYNLOAD)) {
      fprintf(stderr, "%s is not an executable.\n", filename);
      exit(1);
   }

	/* allocate mem for section_hdr array */
	section_hdr_array = (struct scnhdr *) malloc (
			sizeof(struct scnhdr) * xcoff_hdr.filehdr.f_nscns );

   /* locate relevant sections for further processing */

   for (i = 0;  i < xcoff_hdr.filehdr.f_nscns;  i++) {
      if (fread(&section_hdr, SCNHSZ, 1, fp) != 1) {
         fprintf(stderr, "Could not read section header");
         return;
      }
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

   /* executable must be bound with proper text and data starting addresses
      in order to work with this simulator */
	/*
   if ((text_location >> SEGSHIFT) != ((ulong)SOLO_TEXTORG >> SEGSHIFT)) {
      fprintf(stderr, "Executable was not bound with text starting at 0x%x\n",
              (unsigned int) SOLO_TEXTORG);
      fprintf(stderr, "Use -bpT:0x%x flag with binder\n", (unsigned int) SOLO_TEXTORG);
      exit(1);
   }
   if ((data_location >> SEGSHIFT) != ((ulong)SOLO_DATAORG >> SEGSHIFT)) {
      fprintf(stderr, "Executable was not bound with data starting at 0x%x\n",
              (unsigned int) SOLO_DATAORG);
      fprintf(stderr, "Use -bpD:0x%x flag with binder\n", (unsigned int) SOLO_DATAORG);
      exit(1);
   }
	*/


   /* allocate area for text and copy text over */
	/*
   fseek(fp, text_scnptr, 0);
   if (fread((void *)text_location, text_scnsize, 1, fp) != 1) {
      fprintf(stderr, "Unable to copy in text section.\n");
      exit(1);
   } else printf("read text segment\n");
	*/

   /* allocate area for initialized data and bss and 
      copy initialized data over */
	
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
   
   /* resolve unresolved imported symbols.  The address is found in the
      relocation table.  NOTE:  this code is only designed to handle
      statically bound executables whose unresolved symbols are imported 
      from /unix */
   for (i = 0;  i < loader_hdr.l_nreloc;  i++) {
      if (fread(&ldrel, LDRELSZ, 1, fp) != 1) {
         fprintf(stderr, "Unable to read relocation table entry.\n");
         return;
      }
		
      if (ldrel.l_symndx >= 3 && Ldsymtab[ldrel.l_symndx].l_value == 0) {
			tempcount++;
         Ldsymtab[ldrel.l_symndx].l_value = ldrel.l_vaddr;
			/*
			if (Ldsymtab[ldrel.l_symndx].l_zeroes != 0) 
				printf("%d in loader segment %d %s %x %x %x %d\n", ldrel.l_symndx, i, Ldsymtab[ldrel.l_symndx].l_name, ldrel.l_vaddr, ldrel.l_rtype, ldrel.l_rsecnm, ldrel.l_symndx);
			else {
				printf("%d in loader segment %d %s %x %x %x %d \n",ldrel.l_symndx, i, (char *) &Ldrscn_strtbl[Ldsymtab[ldrel.l_symndx].l_offset],ldrel.l_vaddr, ldrel.l_rtype, ldrel.l_rsecnm, ldrel.l_symndx);
			}
			*/
				
		}/* else {
			printf("outside %d %s %x %x %x %x\n", i, Ldsymtab[ldrel.l_symndx].l_name, ldrel.l_vaddr, ldrel.l_symndx, ldrel.l_rtype, ldrel.l_rsecnm);
		} */
		/*
		if (ldrel.l_vaddr >= xcoff_hdr.aouthdr.o_toc) {
			printf("in TOC\n");
		}
		*/
   }

   /* finished with executable file at this point */

	// fprintf(stderr, "%d symbols imported\nListing them out on stderr....\n", tempcount);
	mem_ret = mem_access(mem, Read, 0x20010340, &i, sizeof(md_addr_t) );

	for (i = 3; i < loader_hdr.l_nsyms+3; i++ )  {
		if (Ldsymtab[i].l_zeroes != 0) {
			// printf("%d %s %x %x %x\n", i, Ldsymtab[i].l_name, Ldsymtab[i].l_value, Ldsymtab[i].l_scnum, Ldsymtab[i].l_smtype);
			symname = Ldsymtab[i].l_name;
		}
		else  {
			// printf("%d %s %x %x %x\n", i, (char *) &Ldrscn_strtbl[Ldsymtab[i].l_offset], Ldsymtab[i].l_value, Ldsymtab[i].l_scnum, Ldsymtab[i].l_smtype);
			symname = (char *) &Ldrscn_strtbl[Ldsymtab[i].l_offset];
		}
		//fprintf(stderr, "%s ", symname);
		if (strcmp(symname, "environ") == 0) {
			fprintf(stderr, " Got environ.... Not implemented yet. simulator may not work\n");
			continue;
		}
		val = -1;
		if (strcmp(symname, "sbrk") == 0) {
			val = SYSCALL_VALUE_SBRK;
			/*
			*sp -= sizeof(md_addr_t);
			mem_bcopy(mem_access, mem, Write, *sp, &syscalladdress, 4);
			mem_bcopy(mem_access, mem, Write, Ldsymtab[i].l_value, sp, 4);
			*sp -= sizeof(md_addr_t);
			mem_bcopy(mem_access, mem, Write, *sp, &val, 4);
			fprintf(stderr,"implemented\n" );
			continue;
			*/
		} else
      if (strcmp(symname, "brk") == 0) {
         val = SYSCALL_VALUE_BRK;
      } else
      if (strcmp(symname, "kwrite") == 0) {
         val = SYSCALL_VALUE_KWRITE;
      } else
      if (strcmp(symname, "kread") == 0) {
         val = SYSCALL_VALUE_KREAD;
      } else
      if (strcmp(symname, "kioctl") == 0) {
         val = SYSCALL_VALUE_KIOCTL;
      } else
      if (strcmp(symname, "kfcntl") == 0) {
         val = SYSCALL_VALUE_KFCNTL;
      } else
      if (strcmp(symname, "_exit") == 0) {
         val = SYSCALL_VALUE_EXIT;
      } else
      if (strcmp(symname, "statx") == 0) {
         val = SYSCALL_VALUE_STATX;
      } else
		if (strcmp(symname, "open") == 0) {
         val = SYSCALL_VALUE_OPEN;
      } else
      if (strcmp(symname, "close") == 0) {
         val = SYSCALL_VALUE_CLOSE;
      } else 
		if (strcmp(symname, "sigprocmask") == 0) {
			val = SYSCALL_VALUE_SIGPROCMASK;
		} else
		if (strcmp(symname, "klseek") == 0) {
         val = SYSCALL_VALUE_KLSEEK;
      }
 
		*sp -= sizeof(md_addr_t);
      mem_bcopy(mem_access, mem, Write, *sp, &syscalladdress, 4);
      mem_bcopy(mem_access, mem, Write, Ldsymtab[i].l_value, sp, 4);
      *sp -= sizeof(md_addr_t);
      mem_bcopy(mem_access, mem, Write, *sp, &val, 4);
      if (val != -1) fprintf(stderr,"implemented\n" );
			else fprintf(stderr, "NOT implemented\n" ); 
	}

	/*
	for (i = 0; i < xcoff_hdr.filehdr.f_nscns; i++) {
		printf("reading %d from %s\n", section_hdr_array[i].s_nreloc,
			section_hdr_array[i].s_name);
		fseek(fp, section_hdr_array[i].s_relptr, 0);
		for (j = 0; j < section_hdr_array[i].s_nreloc; j++) {
			fread(&snrel, sizeof(RELOC), 1, fp);
			printf("%0x %0x %0x %0x \n", snrel.r_vaddr, snrel.r_symndx, snrel.r_rsize, snrel.r_rtype);
			if (snrel.r_vaddr > xcoff_hdr.aouthdr.o_toc) {
				printf("in TOC\n");
			}
		}
	}

	*/

   fclose(fp);
	return;

#ifdef 0

   /* here we allocate and initialize storage for variables and descriptors 
      that would normally be found either in 1) the ublock, in the case of
      errno; 2) on the stack, in the case of envp; and 3) in the kernel
      exports list, in the case of system calls */
   for (i = 3, stack_top = (unsigned *)(*new_argv);  
        i < loader_hdr.l_nsyms+3;  
        i++) {
      if (Ldsymtab[i].l_zeroes == 0)
         symname = &(Ldrscn_strtbl[Ldsymtab[i].l_offset]);
      else
         symname = Ldsymtab[i].l_name;
      if (!strncmp(symname,"errno",5)) {
         /* allocate a storage location for errno, initialize to 0,
            and point TOC entry to it */
         p = stack_top - 1;
         stack_top -= 1;
         /* save away errno address so that emulated system calls can set it */
         Errno_address = (int *)p;
         *p = (unsigned) 0;
         q = (unsigned *) (unsigned long) Ldsymtab[i].l_value;
         *q = (unsigned) (unsigned long) p;
      } else if (!strncmp(symname,"environ",7)) {
         /* allocate a storage location for environ, initialize to envp vector,
            and point TOC entry to it */
         p = stack_top - 1;
         stack_top -= 1;
         *p = (unsigned) (unsigned long) *new_envp;
         q = (unsigned *) (unsigned long) Ldsymtab[i].l_value;
         *q = (unsigned) (unsigned long) p;
      } else if (Ldsymtab[i].l_smclas == XMC_DS &&
                 strncmp("__start",symname,7)) {
         /* __start already has a function descriptor allocated */
         /* descriptor class -- points to a system call func descriptor */
         /* allocate a function descriptor, initialize to have branch target 0, 
            index into emulated_syscall table, and point TOC entry to it */
         p = stack_top - 1;
         *p = (unsigned) N_emulated_syscalls;
         stack_top -= 1;
         p = stack_top - 1;
         *p = (unsigned) 0;
         stack_top -= 1;
         q = (unsigned *) (unsigned long) Ldsymtab[i].l_value;
         *q = (unsigned) (unsigned long) p;
         emulated_syscall[N_emulated_syscalls].name = symname;
         /* set to invalid syscall for now; fix up emulated syscalls next */
         if (!strncmp(symname,"kreadv",5))
            emulated_syscall[N_emulated_syscalls].emulator =
               SoloEmulateAix_kreadv;
         else if (!strncmp(symname,"kwritev",7))
            emulated_syscall[N_emulated_syscalls].emulator = 
               SoloEmulateAix_kwritev;
         else if (!strncmp(symname,"_exit",5))
            emulated_syscall[N_emulated_syscalls].emulator =
               SoloEmulateAix__exit;
         else
            emulated_syscall[N_emulated_syscalls].emulator =
               SoloStubbedAixEmulator;
         N_emulated_syscalls++;
         if (N_emulated_syscalls >= MAX_EMULATED_SYSCALLS) {
            fprintf(stderr, "Number of system calls exceeded maximum.\n");
            fprintf(stderr, "Increase MAX_EMULATED_SYSCALLS.\n");
            exit(1);
         }
      }
   }

   /* return entry point, toc, and initial stack pointer */
   entry = (struct func_desc *) (unsigned long) xcoff_hdr.aouthdr.entry;
   *entry_instruction_VA = (VA) *((uint32 *) &entry->entry_point);
   *toc_VA  = (VA) xcoff_hdr.aouthdr.o_toc;
   /* update stack top to reserve a minimum stack */
   *stack_top_VA = (VA) (unsigned long) (stack_top - STKMIN/sizeof(*stack_top));
   *stack_top_VA = (VA) (((int64) *stack_top_VA) & -8LL);  /* round to 8-byte boundary */

#ifdef DEBUG_SOLO
   printf("First instruction address:  %x\n", *entry_instruction_VA);
   printf("TOC address:  %x\n", *toc_VA);
   printf("Top (low address) of initial stack frame:  %x\n", *stack_top_VA);
#endif

#endif

}

#ifdef 0

void
SoloAixInitCPU(VA pc, VA stackTop, VA toc, int argc, VA argv, VA envp)
{
   curPE = PE[0];
   bzero(curPE, sizeof(PowerPCState));

   WriteMSR (curPE, MSR_PR_BIT | MSR_FP_BIT | MSR_ME_BIT);
   curPE->PC = pc;
   curPE->gpr[1] = stackTop;
   curPE->gpr[2] = toc;
   curPE->gpr[3] = argc;
   curPE->gpr[4] = argv;
   curPE->gpr[5] = envp;
   curPE->cpuState = cpu_running;

   if (simosCPUType == PPC_BLOCK)
       curPE->currentSimulator = PPC_BLOCK;
   else
       curPE->currentSimulator = PPC_SIMPLE;
}

static
void
PrintAoutHdr(struct aouthdr *aout_hdr)
{
   printf("AOUT HEADER:\n");
   printf("\tmagic      = %x ", aout_hdr->magic );
   switch ( aout_hdr->magic ) {
      case 0x0107:    printf ("(text & data contiguous, both writabl)\n"); break;
      case 0x0108:    printf ("(text is R/O, data in next section)\n"); break;
      case 0x010b:    printf ("(text & data aligned and may be paged)\n"); break;
      default:        printf ("(UNKNOWN)\n"); break;
   }
   printf("\tvstamp     = %x\n", aout_hdr->vstamp );
   printf("\ttsize      = %d Bytes\n", aout_hdr->tsize );
   printf("\tdsize      = %d Bytes\n", aout_hdr->dsize );
   printf("\tbsize      = %d Bytes\n", aout_hdr->bsize );
   printf("\tentry      = %08x\n", aout_hdr->entry );
   printf("\ttext_start = %08x\n", aout_hdr->text_start );
   printf("\tdata_start = %08x\n", aout_hdr->data_start );
   printf("\to_toc      = %08x\n", aout_hdr->o_toc );
   printf("\to_snentry  = %d (Section number of entry point)\n", aout_hdr->o_snentry );
   printf("\to_sntext   = %d (Section number of text)\n", aout_hdr->o_sntext );
   printf("\to_sndata   = %d (Section number of data)\n", aout_hdr->o_sndata );
   printf("\to_sntoc    = %d (Section number of toc)\n", aout_hdr->o_sntoc );
   printf("\to_snloader = %d (Section number of loader)\n", aout_hdr->o_snloader );
   printf("\to_snbss    = %d (Section number of bss)\n", aout_hdr->o_snbss );
   printf("\to_algntext = %08x (Maximum alignment of text)\n", aout_hdr->o_algntext );
   printf("\to_algndata = %08x (Maximum alignment of data)\n", aout_hdr->o_algndata );
   printf("\to_modtype  = %c%c\n", aout_hdr->o_modtype[0], aout_hdr->o_modtype[1]);
   printf("\to_cpuflag  = %x\n", aout_hdr->o_cpuflag);
   printf("\to_cputype  = %x\n", aout_hdr->o_cputype);
   printf("\to_maxstack = %08x (Maximum size of stack)\n", aout_hdr->o_maxstack );
   printf("\to_maxdata  = %08x (Maximum size of data)\n", aout_hdr->o_maxdata );
}

static
void PrintFileHdr(struct filehdr *file_hdr)
{
   printf("FILE HEADER:\n");
   printf("\tf_magic  = %o\n", file_hdr->f_magic );
   printf("\tf_nscns  = %ld\n", file_hdr->f_nscns );
   printf("\tf_timdat = %ld\n", file_hdr->f_timdat );
   printf("\tf_symptr = %ld\n", file_hdr->f_symptr );
   printf("\tf_nsyms  = %ld\n", file_hdr->f_nsyms );
   printf("\tf_opthdr = %ld\n", file_hdr->f_opthdr );
   PrintFFlags(file_hdr->f_flags);
}

static
void PrintFFlags(unsigned short f_flags)
{
   printf("\tf_flags  = 0x%08x :\n", f_flags);

   if ( f_flags & F_RELFLG )
      printf("\t\tF_RELFLG:  relocation info stripped from file\n");
   if ( f_flags & F_EXEC )
      printf("\t\tF_EXEC:    executable (no unresolved external references)\n");
   if ( f_flags & F_LNNO )
      printf("\t\tF_LNNO:    line nunbers stripped from file\n");
   if ( f_flags & F_LSYMS )
      printf("\t\tF_LSYMS:   local symbols stripped from file\n");
   if ( f_flags & F_AR16WR )
      printf("\t\tF_AR16WR:  has the byte ordering of an AR16WR\n");
   if ( f_flags & F_AR32WR )
      printf("\t\tF_AR32WR:  has the byte ordering of an AR32WR machine\n");
   if ( f_flags & F_AR32W )
      printf("\t\tF_AR32W:   has the byte ordering of an AR32W machine\n");
   if ( f_flags & F_DYNLOAD )
      printf("\t\tF_DYNLOAD: dynamically loadable and executable\n");
   if ( f_flags & F_SHROBJ )
      printf("\t\tF_SHROBJ:  shared object\n");
}

static
void PrintSectionHdr( struct scnhdr *section_hdr )
{
   printf("SECTION : %s\n", section_hdr->s_name);
   printf("\ts_paddr   = 0x%08x (physical address)\n", section_hdr->s_paddr);
   printf("\ts_vaddr   = 0x%08x (virtual address)\n", section_hdr->s_vaddr);
   printf("\ts_size    = %d (section size)\n", section_hdr->s_size);
   printf("\ts_scnptr  = 0x%08x (file ptr to raw data for section)\n", 
          section_hdr->s_scnptr);
   printf("\ts_relptr  = 0x%08x (file ptr to relocation)\n", 
          section_hdr->s_relptr);
   printf("\ts_lnnoptr = 0x%08x (file ptr to line numbers)\n", 
          section_hdr->s_lnnoptr);
   printf("\ts_nreloc  = 0x%08x (number of relocation entries)\n", 
          section_hdr->s_nreloc);
   printf("\ts_nlnno   = 0x%08x (number of line number entries)\n", 
          section_hdr->s_nlnno);
   printf("\ts_flags   = 0x%08x :\n", section_hdr->s_flags);

   if ( section_hdr->s_flags & STYP_REG )
      printf("\t\tSTYP_REG    : \"regular\" section (allocated, relocated, loaded)\n");

   if ( section_hdr->s_flags & STYP_PAD )
      printf("\t\tSTYP_PAD    : \"padding\" section (not allocated, not relocated)\n");

   if ( section_hdr->s_flags & STYP_TEXT ) {
      printf("\t\tSTYP_TEXT   : section contains instructions and data only");
      printf("\t\t\tRANGE (0x%08lx:0x%08lx)\n", 
             section_hdr->s_vaddr, 
             section_hdr->s_vaddr + section_hdr->s_size);
   }

   if ( section_hdr->s_flags & STYP_DATA ) {
      printf("\t\tSTYP_DATA   : section contains data only\n");
      printf("\t\t\tRANGE (0x%08lx:0x%08lx)\n",
             section_hdr->s_vaddr,
             section_hdr->s_vaddr + section_hdr->s_size);
   }

   if ( section_hdr->s_flags & STYP_BSS )
      printf("\t\tSTYP_BSS    : section contains bss only\n");

   if ( section_hdr->s_flags & STYP_EXCEPT )
      printf("\t\tSTYP_EXCEPT : Exception section\n");

   if ( section_hdr->s_flags & STYP_LOADER )
      printf("\t\tSTYP_LOADER : Loader section\n");

   if ( section_hdr->s_flags & STYP_DEBUG )
      printf("\t\tSTYP_DEBUG  : Debug section\n");

   if ( section_hdr->s_flags & STYP_TYPCHK )
      printf("\t\tSTYP_TYPCHK : Type check section\n");

   if ( section_hdr->s_flags & STYP_OVRFLO )
      printf("\t\tSTYP_OVRFLO : RLD and line number overflow sec hdr\n");
}

static
void
PrintLoaderScn(FILE *fp, long loader_scnptr, ulong loader_scnsize)
{
   LDHDR   loader_hdr;
   LDSYM   ldsym;
   LDREL   ldrel;
   char *strtbl, *imptbl;
   char *c;
   int i;
   char *symname;

   fseek (fp, loader_scnptr, 0);
   fread(&loader_hdr, LDHDRSZ, 1, fp);

   printf("\nLOADER HEADER:\n");
   printf("Number of symbol table entries                   : %d\n",
          loader_hdr.l_nsyms );
   printf("Number of relocation table entries               : %d\n",
          loader_hdr.l_nreloc );
   printf("Length of loader import file id strings          : %d\n",
          loader_hdr.l_istlen );
   printf("Qty of loader import file ids                    : %d\n",
          loader_hdr.l_nimpid );
   printf("Offset to start of loader import file id strings : %d\n",
          loader_hdr.l_impoff );
   printf("Length of loader string table                    : %d\n",
          loader_hdr.l_stlen );
   printf("Offset to start of loader string table           : %d\n",
          loader_hdr.l_stoff );

   if ((strtbl = (char *)malloc(loader_hdr.l_stlen)) == NULL) {
      fprintf(stderr, "Could not allocate loader string table.\n");
      exit(1);
   }
   fseek (fp, loader_scnptr + loader_hdr.l_stoff, 0);
   fread (strtbl, 1, loader_hdr.l_stlen, fp);
 
   if ((imptbl = (char *)malloc(loader_hdr.l_istlen)) == NULL) {
      fprintf(stderr, "Could not allocate import file string table.\n");
      exit(1);
   }
   fseek (fp, loader_scnptr + loader_hdr.l_impoff, 0);
   fread (imptbl, 1, loader_hdr.l_istlen, fp);
   printf("\nIMPORT FILE TABLE:\n");
   for (i = 0, c = imptbl;  i < loader_hdr.l_istlen;  c++, i++) {
      if (*c == NULL)
         putchar ('\n');
      else
         putchar(*c);
   }

   printf("\nSYMBOL TABLE:\n");
   fseek(fp, loader_scnptr + LDHDRSZ, 0);
   /* remember first three entries of table are implicitly .text, .data,
      and .bss */
   for (i = 3;  i < loader_hdr.l_nsyms+3;  i++) {
      fread(&ldsym, LDSYMSZ, 1, fp);
      if (ldsym.l_zeroes == 0) {
         printf("Symbol name:  %s\n", &(strtbl[ldsym.l_offset]));
         symname = &(strtbl[ldsym.l_offset]);
      } else {
         printf("Symbol name:  %8s\n", ldsym.l_name);
         symname = ldsym.l_name;
      }
      printf("[%d] Value:  %x\n", i, ldsym.l_value);
      printf("[%d] Section Number:  %d\n", i, ldsym.l_scnum);
      printf("[%d] Type and Imp/Exp:  %d\n", i, ldsym.l_smtype);
      printf("[%d] Class:  %d\n", i, ldsym.l_smclas);
      printf("[%d] First entry of import file:  %s\n", i, &(imptbl[ldsym.l_ifile]));
      printf("[%d] Typecheck offset into loader string:  %d\n", i, ldsym.l_parm);
   }

   printf("\nRELOCATION TABLE:\n");
   for (i = 0;  i < loader_hdr.l_nreloc;  i++) {
      fread(&ldrel, LDRELSZ, 1, fp);
      printf("Virtual address:  %x\n", ldrel.l_vaddr);
      printf("Symbol table index:  %d\n", ldrel.l_symndx);
      printf("Type:  %x\n", (ldrel.l_rtype & 0xff));
      printf("Section Number:  %hd\n", ldrel.l_rsecnm);
   }

   free(strtbl);
   free(imptbl);
}

#endif

/*
void main(char argc, char *argv[]) {
	struct mem_t *mem;
	md_addr_t sp = 0;

	mem = (struct mem_t *) malloc(sizeof(struct mem_t) * 100);
	printf("loading %s\n", argv[1]);
	//SoloAixLoadXcoff	(argv[1]);
	readrelocationtable(argv[1], mem, &sp);
	printf("ended\n");

}
*/
