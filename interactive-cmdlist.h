tword cmd_showregs(tword r) { 
	int i, j;
	j = (int) r.data.val;
	if (j == -1) {
		printf("\n");
		for (i = 0; i < MD_NUM_IREGS; i++) {
			printf("%d: 0x%8x\t",i, regs.regs_R[i]);
		}
		printf("\n");
		printf("CR: 0x%8x, LR: 0x%8x, CNTR: 0x%8x, XER: 0x%8x, PC 0x%8x\n",
				regs.regs_C.cr, regs.regs_L, regs.regs_CNTR, regs.regs_C.xer,
				regs.regs_PC);
		return TAGGED_TRUE;	
	}
	printf("%2d: 0x%8x\n", j, regs.regs_R[j]);
	return TAGGED_TRUE;	
	
}

tword cmd_showmem(tword r1, tword r2) {
	int i, j, v;
	int range;
	enum md_fault_type _fault;
	j = (int) r1.data.val;
	range = (int) r2.data.val;
	printf("mem %x %x\n", j, range);
	for (i = 0; i < range; i+=4) {
		v = READ_WORD(j, _fault); j += 4;
		if (_fault != md_fault_none) {
			printf("Read generated fault\n");
		} else {
			printf("%x ", v);
		}
	}
	printf("\n");
}

tword cmd_showmems(tword r1, tword r2) {
   int i, j, v;
   int range;
   enum md_fault_type _fault;
   j = (int) r1.data.val;
   range = (int) r2.data.val;
   printf("mems %x %x\n", j, range);
   for (i = 0; i < range; i++) {
      v = READ_BYTE(j, _fault); j++;
      if (_fault != md_fault_none) {
         printf("Read generated fault\n");
      } else {
         printf("%c", v);
      }
		if (v == 0x0) break;
   }
	printf("\n");
}          

inline void stepi(void) {
#ifdef INTERACTIVE
	register md_inst_t inst;
  	register enum md_opcode op;
	char ch;

	op = (enum md_opcode)( __UNCHK_MEM_READ(dec, regs.regs_PC << 1, word_t) );
  	inst = (word_t)__UNCHK_MEM_READ(dec, (regs.regs_PC << 1)+sizeof(word_t), word_t) );
   printf("INST 0x%x, PC 0x%x\n", inst, regs.regs_PC, regs.regs_C.cr);

	switch (op)
   {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5)\
   case OP:                   \
     SYMCAT(OP,_IMPL);                 \
	  printf("%x %s\n", regs.regs_PC, NAME);	\
     break;
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2)        \
   case OP:                   \
     panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)                 \
     { /* uncaught... */break; }
#include "machine.def"
   default:
		fprintf(stderr, "pc %x inst %x\n",  regs.regs_PC, inst);
     panic("attempted to execute a bogus opcode %x %x", regs.regs_PC, inst);
   }

      /* execute next instruction */
   regs.regs_PC = regs.regs_NPC;
   regs.regs_NPC += sizeof(md_inst_t);
#endif
}

void runstepi() {
#ifdef INTERACTIVE
   register md_inst_t inst;
   register enum md_opcode op;
   char ch;

   op = (enum md_opcode)( __UNCHK_MEM_READ(dec, regs.regs_PC << 1, word_t) );
   inst = (word_t)__UNCHK_MEM_READ(dec, (regs.regs_PC << 1)+sizeof(word_t), word_t);

   switch (op)
   {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5)\
   case OP:                   \
     SYMCAT(OP,_IMPL);                 \
     break;
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2)        \
   case OP:                   \
     panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)                 \
     { /* uncaught... */break; }
#include "machine.def"
   default:

     panic("attempted to execute a bogus opcode %x",op);
   }

      /* execute next instruction */
   regs.regs_PC = regs.regs_NPC;
   regs.regs_NPC += sizeof(md_inst_t);
#endif
}

op = (enum md_opcode) j;
		switch (op) {
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3,O3,O4,O5,I4,I5)\
   	case OP:                   \
     		if (found[j]) printf("%s %d\n", NAME, found[j]); \
			break;
#define DEFLINK(OP,MSK,NAME,SHIFT,MASK1,MASK2)        
#define CONNECT(OP)

#include "machine.def"	
		}
	}
}

