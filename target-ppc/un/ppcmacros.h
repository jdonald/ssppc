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

/*Dependencies defined here*/

/*Fix Me: This is still incorrect. Will fix it when sim-outorder.c is
done*/

/*This tells that the dependency is not multiple general purpose
registers*/
#define DMGPR			(-1)


/*The once that are here seem to be ok*/
#define DNA                           (0)
#define PPC_DGPR(N)		      (N)
#define PPC_DFPR(N)		      (N+32)
#define PPC_DCR		      (64)
#define PPC_DFPSCR		      (65)
#define PPC_DXER		      (66)
#define PPC_DLR		      (67)
#define PPC_DCNTR		      (68)

#define PPC_DXER_LR_CNTR(N)       ( (N==1)? PPC_DXER :((N==8)? PPC_DLR : PPC_DCNTR) )

/*Macros which help in the reading of the registers are defined here*/

#define PPC_GET_XER_CA 		((XER>>29)&0x1)
#define PPC_GET_XER_SO              ((XER>>31)&0x1)
#define PPC_GET_FPSCR_VE            ((FPSCR>>7)&0x1)

/*FIXME: Speculative mode for these registers have to be defined*/
#define LR	        (regs.regs_L)
#define CNTR	(regs.regs_CNTR)



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
