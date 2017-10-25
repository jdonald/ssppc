// This is not a typical header file. In Turandot it would have a .macros extension

  /* pre-decode text segment */
  {
		md_inst_t sys_inst[6];
    unsigned i, num_insn = (ld_text_size + 3) / 4;
/* these are the instruction that are part of a system call */
/* vareis from AIX version to version. dont know???? */
  	sys_inst[0] = 0x8182;
   sys_inst[1] = 0x90410014;
   sys_inst[2] = 0x800c0000;
   sys_inst[3] = 0x804c0004;
   sys_inst[4] = 0x7c0903a6;
   sys_inst[5] = 0x4e800420;


    fprintf(stderr, "** pre-decoding %u insts...", num_insn);

    /* allocate decoded text space */
    dec = mem_create("dec");

    for (i=0; i < num_insn; i++)
      {
	enum md_opcode op;
	md_inst_t inst;
	md_addr_t PC;
	static __thread int gotasyscall= 0; // initialize
	int tempi;

	/* compute PC */
	PC = ld_text_base + i * sizeof(md_inst_t);
	/* get instruction from memory */
	inst = MD_SWAPW( __UNCHK_MEM_READ(mem, PC, md_inst_t) );
	/* decode the instruction */

   if ( (inst >> 16) == 0x8182) {
   	gotasyscall = 1;
   	for (tempi = 1; tempi < 6; tempi++) {
   		mem_access(mem, Read, PC+tempi*4, &inst, 4);
   		if (sys_inst[tempi] != inst) {
   			gotasyscall = 0;
   			break;
   		}
   	}
   }
	inst = MD_SWAPW( __UNCHK_MEM_READ(mem, PC, md_inst_t) );

	MD_SET_OPCODE(op, inst);
	/*
	if ( (op == FADDS) || (op == FSUBS) || (op == FMULS)
			|| (op == FDIVS) || (op == FMADDS) || (op == FMSUBS) 
			|| (op == FNMADDS) ) {
		fprintf(stderr, "decode: FP single instruction %x %x\n", inst, PC);
		
	}
	*/
	if ( (gotasyscall) && (inst == 0x4e800420) ) {
#ifdef PPC_DEBUG
		printf("found syscall at %x\n", PC);
#endif
		gotasyscall = 0;
		inst = 0x44000002; op = SC;
	}
	/* insert into decoded opcode space */
	MEM_WRITE_WORD(dec, PC << 1, MD_SWAPW( ((word_t)op) ) );
	MEM_WRITE_WORD(dec, (PC << 1)+sizeof(word_t), MD_SWAPW(inst) );
	MEM_WRITE_WORD(mem, PC, (inst) );
      }
	writemillicodepredecode	(dec);
    fprintf(stderr, "done\n");
  }
