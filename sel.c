#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <limits.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "libexo/libexo.h"
#include "syscall.h"
#include "sim.h"
#include "endian.h"
#include "sel.h"

#define COMPARE_STR(str1,str2) (strcmp(str1,str2)==0)
#define SEL_LINE_SIZE 128
#define REG_NAME_SIZE 8
#define PAGE_SIZE (4096)
#define INDEX_NOT_FOUND (-1)
#define INVALID_INDEX (0xff)
#define NEXT_INDEX(i) ((i+1==SSI_QUEUE_SIZE)?0:i+1)

#define INTEGER_GPR 0
#define INTEGER_SEG 1

#define MD_SEL_TO_IREG_GPR(VAL, REGS, IDX)                                  \
  ((REGS)->regs_R.d[IDX] = (unsigned long)(VAL))
#define MD_SEL_TO_IREG_SEG(VAL, REGS, IDX)                                  \
  ((REGS)->regs_S.w[IDX] = (unsigned long)(VAL))
#define MD_SEL_TO_FREG(VAL, REGS, IDX)                                  \
  ((REGS)->regs_F.e[IDX] = (efloat_t)(VAL))

#define SEL_LINE(tid)       (threads[tid].sel_line)
#define TH_REGS(tid)  (threads[tid].th_regs)
#define SEL_MARKER(tid) (threads[tid].sel_marker)
#define SEL_ENTRY(tid) (threads[tid].sel_entry)
#define SSI_ENTRY(tid) (threads[tid].ssi_entry)
#define SSI_QUEUE(tid) (threads[tid].ssi_queue)
#define SSI_FNAME(tid) (threads[tid].ssi_fname)
#define SEL_FNAME(tid) (threads[tid].sel_fname)
#define SSI_FD(tid) (threads[tid].ssi_fd)
#define SEL_FD(tid) (threads[tid].sel_fd)

/*-----------------------------------------------------------------------------------------
 local variables 
-----------------------------------------------------------------------------------------*/
static char *cs_fname;  /* context switch file; for mthreads */
static FILE *cs_fd;     /* context switch file descriptor */
static FILE *sec_fd;    /* text section's file descriptor */
static char *sec_fname; /* text section's ranges */
static struct cs_entry cs_entry;

/*-----------------------------------------------------------------------------------------
 exported variables                                                                     
-----------------------------------------------------------------------------------------*/
int sel_flag = FALSE;      /* are we restoring sel files? */
int sel_num_threads = 1;   /* number of threads */
md_addr_t lib_code_sections[MAX_CODE_SECTIONS]; /* text section's ranges for the libraries */
unsigned num_lib_code_sections; /* number of dynamic libraries */
int sel_curr_tid = 0; /* current tid */
struct sel_thread *threads; /* state of each thread being restored */

/*-----------------------------------------------------------------------------------------
  prototypes
-----------------------------------------------------------------------------------------*/
void
sel_restore_registers(FILE *fd, 
		      struct sel_marker *sm, 
		      struct regs_t *regs,
		      char *str);
void 
ssi_read_entries(FILE *fd);

void 
sel_read_cs_entry();

/*-----------------------------------------------------------------------------------------
  local functions
-----------------------------------------------------------------------------------------*/

void sel_restore_ldt_entry(char *str);

/*
 * Save the context of the thread "saved_tid"
 */
static void save_context(int saved_tid, struct regs_t *saved_regs)
{
    memcpy(TH_REGS(saved_tid), saved_regs, sizeof(struct regs_t));
}

/*
 * Restore the context of the thread "restored_tid"
 */
static void restore_context(int restored_tid, struct regs_t *restored_regs)
{
    memcpy(restored_regs, TH_REGS(restored_tid), sizeof(struct regs_t));
}


/*-----------------------------------------------------------------------------------------
  exported functions
-----------------------------------------------------------------------------------------*/

/********************************************************************
 * SEL related functions
 *******************************************************************/

/*
 * Check whether the files exist
 */
int 
sel_valid_files(char *fname)
{
  struct stat buf;
  char tmp_fname[strlen(fname)+10];
  int i;

  sec_fname = (char *)malloc(strlen(fname)+5);
  sprintf(sec_fname, "%s.sec", fname);
  if ( stat(sec_fname, &buf) != 0 )
    return 0;

  if ( sel_num_threads > 1 )
  {
    cs_fname = (char *)malloc(strlen(fname)+5);
    sprintf(cs_fname, "%s.cs", fname);
    if ( stat(cs_fname, &buf) != 0 )
    {
      fprintf(stderr, "Could not open %s file!\n", cs_fname);
      return 0;
    }
  }
  for ( i = 0; i < sel_num_threads; i++ )
  {
    sprintf(tmp_fname, "%s_%d.ssi", fname, i);
    if ( stat(tmp_fname, &buf) != 0 )
    {
      fprintf(stderr, "Could not open %s file!\n", tmp_fname);
      return 0;
    }
  }
  for ( i = 0; i < sel_num_threads; i++ )
  {
    sprintf(tmp_fname, "%s_%d.sel", fname, i);
    if ( stat(tmp_fname, &buf) != 0 )
    {
      fprintf(stderr, "Could not open %s file!\n", tmp_fname);
      return 0;
    }
  }
  return 1;
}

/*
 * Returns the index for the integer registers
 */
static int 
get_ireg_index(char *name, int *reg_type)
{
  if ( COMPARE_STR(name, "eax") )
  {
    *reg_type = INTEGER_GPR;
    return MD_REG_EAX;
  }
  if ( COMPARE_STR(name, "ecx") )
  {
    *reg_type = INTEGER_GPR;
    return MD_REG_ECX;
  }
  if ( COMPARE_STR(name, "edx") )
  {
    *reg_type = INTEGER_GPR;
    return MD_REG_EDX;
  }
  if ( COMPARE_STR(name, "ebx") )
  {
    *reg_type = INTEGER_GPR;
    return MD_REG_EBX;
  }
  if ( COMPARE_STR(name, "esp") )
  {
    *reg_type = INTEGER_GPR;
    return MD_REG_ESP;
  }
  if ( COMPARE_STR(name, "ebp") )
  {
    *reg_type = INTEGER_GPR;
    return MD_REG_EBP;
  }
  if ( COMPARE_STR(name, "esi") )
  {
    *reg_type = INTEGER_GPR;
    return MD_REG_ESI;
  }
  if ( COMPARE_STR(name, "edi") )
  {
    *reg_type = INTEGER_GPR;
    return MD_REG_EDI;
  }
  if ( COMPARE_STR(name, "xds") ) 
  {
    *reg_type = INTEGER_SEG;
    return MD_REG_DS;
  }
  if ( COMPARE_STR(name, "xes") ) 
  {
    *reg_type = INTEGER_SEG;
    return MD_REG_ES;
  }
  if ( COMPARE_STR(name, "xfs") )
  {
    *reg_type = INTEGER_SEG;
    return MD_REG_FS;
  }
  if ( COMPARE_STR(name, "xgs") )
  {
    *reg_type = INTEGER_SEG;
    return MD_REG_GS;
  }
  if ( COMPARE_STR(name, "xcs") )
  {
    *reg_type = INTEGER_SEG;
    return MD_REG_CS;
  }
  if ( COMPARE_STR(name, "xss") )
  {
    *reg_type = INTEGER_SEG;
    return MD_REG_SS;
  }
  return INDEX_NOT_FOUND;
}

/*
 * Returns the index for the floating registers
 */
static int 
get_freg_index(char *name)
{
  if ( COMPARE_STR(name, "cwd") ) 
    return INVALID_INDEX;
  if ( COMPARE_STR(name, "swd") ) 
    return INVALID_INDEX;
  if ( COMPARE_STR(name, "twd") ) 
    return INVALID_INDEX;
  if ( COMPARE_STR(name, "fop") ) 
    return INVALID_INDEX;
  if ( COMPARE_STR(name, "fip") ) 
    return INVALID_INDEX;
  if ( COMPARE_STR(name, "fcs") ) 
    return INVALID_INDEX;
  if ( COMPARE_STR(name, "foo") ) 
    return INVALID_INDEX;
  if ( COMPARE_STR(name, "fos") ) 
    return INVALID_INDEX;
  if ( COMPARE_STR(name, "mxcsr") )
    return INVALID_INDEX;
  if ( COMPARE_STR(name, "st0") )
    return MD_REG_ST0;
  if ( COMPARE_STR(name, "st1") )
    return MD_REG_ST1;
  if ( COMPARE_STR(name, "st2") )
    return MD_REG_ST2;
  if ( COMPARE_STR(name, "st3") )
    return MD_REG_ST3;
  if ( COMPARE_STR(name, "st4") )
    return MD_REG_ST4;
  if ( COMPARE_STR(name, "st5") )
    return MD_REG_ST5;
  if ( COMPARE_STR(name, "st6") )
    return MD_REG_ST6;
  if ( COMPARE_STR(name, "st7") )
    return MD_REG_ST7; 

  return INDEX_NOT_FOUND;
}

/*
 *
 */
void sel_init(md_addr_t *ld_text_base, unsigned int *ld_text_size)
{
  unsigned i;
  num_lib_code_sections = 0;
  char line[128];
  /* if SEL is used, read the first line of the file */
  if ( sel_flag == TRUE )
  {
      if ( ld_text_base && ld_text_size )
      {
        sel_read_line(sec_fd, line);
        if ( sscanf(line, "%x %x %u\n", ld_text_base, ld_text_size, &num_lib_code_sections) != 3 )
        {
            fatal("Unable to restore text information from SEL\n");
        }
        else
        {
            ld_text_bound = *ld_text_base + *ld_text_size;
        }
	for ( i = 0; i < num_lib_code_sections; i++ ) 
	{
            sel_read_line(sec_fd, line);
            if ( sscanf(line, "%x %x\n", &lib_code_sections[2*i], &lib_code_sections[(2*i)+1]) != 2 )
            {
                fatal("Unable to restore text information from SEL\n");
            }
	    lib_code_sections[(2*i)+1] += lib_code_sections[(2*i)];
	}
      }
      for ( i = 0; i < sel_num_threads; i++ ) 
      {
          sel_read_line(SEL_FD(i), SEL_LINE(i));
          if ( SEL_LINE(i)[0] != '0' )
          {
              fatal("Invalid SEL line\n");
          }
          else
          {
	      /* read the first marker header */
	      sel_read_marker(SEL_LINE(i), &SEL_MARKER(i));
          }
      }
      if ( sel_num_threads > 1 )
      {
          sel_read_cs_entry();
      }
  }  
}

/*
 *  Allocate space for the thread's structures
 */
void sel_allocate()
{
  unsigned i, j;
  num_lib_code_sections = 0;
  /* if SEL is used, read the first line of the file */
  threads = (struct sel_thread *)malloc(sel_num_threads*sizeof(struct sel_thread));
  for ( i = 0; i < sel_num_threads; i++ )
  {
      TH_REGS(i) = (struct regs_t *)malloc(sizeof(struct regs_t));
  }
  for ( i = 0; i < sel_num_threads; i++ )
  {
      SEL_ICOUNT(i) = 0;
      SEL_NUM_MEM(i) = 0;
      SEL_REP_FLAG(i) = 0;
      SSI_QUEUE(i).head = 0;
      SSI_QUEUE(i).index = -1;
      SEL_END_TRACE(i) = FALSE;
      for ( j = 0; j < NUM_LDT_ENTRIES; j++ )
      {
          SEL_LDT(0)[j] = 0;
      }
  }
  ldt_p = SEL_LDT(0);
  sel_flag = TRUE;
}

/*
 *
 */
int sel_restore_marker(struct regs_t *regs)
{
  /* if SEL is used, check whether it is time to restore */
  if ( sel_flag == TRUE )
  {
      /* is it time to restore? */
      if ( SEL_NUM_MEM(sel_curr_tid) == SEL_MARKER(sel_curr_tid).mem_count )
      {
  	  //fprintf(stderr, "restoring sel marker at inst_count: %llu, memcont: %llu\n", 
	  //		   sim_num_insn, SEL_NUM_MEM(sel_curr_tid));	
	  /* invalidate mem_count so we don't loop forever */ 
          SEL_MARKER(sel_curr_tid).mem_count = -1;
          /* this is the end of this trace */
          //if ( SEL_MARKER(sel_curr_tid).type == 4 )
	  //{
              //SEL_END_TRACE(sel_curr_tid) = TRUE;
              //return TRUE;
	  //}
	  /* read all register values and restore them in simplescalar */
	  sel_restore_registers(SEL_FD(sel_curr_tid), &SEL_MARKER(sel_curr_tid), regs, SEL_LINE(sel_curr_tid));
          sel_read_line(SEL_FD(sel_curr_tid), SEL_LINE(sel_curr_tid));
	  /* if we read another marker header */
	  if ( SEL_LINE(sel_curr_tid)[0] == '0' )
	  {
	      sel_read_marker(SEL_LINE(sel_curr_tid), &SEL_MARKER(sel_curr_tid));
	  }
	  /* or if we read the next sel entry */
	  if ( SEL_LINE(sel_curr_tid)[0] == '2' || SEL_LINE(sel_curr_tid)[0] == '3' || SEL_LINE(sel_curr_tid)[0] == '4')
	  {
	      sel_read_entry(SEL_LINE(sel_curr_tid), &SEL_ENTRY(sel_curr_tid));
          }
          if ( SEL_LINE(sel_curr_tid)[0] == '5' ) 
          {
              sel_restore_ldt_entry(SEL_LINE(sel_curr_tid));
              sel_read_line(SEL_FD(sel_curr_tid), SEL_LINE(sel_curr_tid));
          }
      }
      else
      {
        fatal("syscall memcount does not match - sim %llu : log %llu", SEL_NUM_MEM(sel_curr_tid), SEL_MARKER(sel_curr_tid).mem_count);
      }
  }
  return FALSE;
}

/*
 *
 */
int sel_check_restore(struct mem_t *mem)
{
  int restored = 0;
  /* if SEL is used, check whether it is time to restore */
  if ( sel_flag == TRUE )
  {
      while ( SEL_ENTRY(sel_curr_tid).mem_count == (SEL_NUM_MEM(sel_curr_tid)) ) 
      {
          restored = 1;
          //if ( SEL_ENTRY(sel_curr_tid).mem_count == 124643 )
          //{
          //    fprintf(stderr, "%d:restoring load at %d type %d", sel_curr_tid, SEL_NUM_MEM(sel_curr_tid), SEL_ENTRY(sel_curr_tid).type);
          //    fprintf(stderr, " type %d ", SEL_ENTRY(sel_curr_tid).type);
          //    fprintf(stderr, "addr %x with value %x\n", SEL_ENTRY(sel_curr_tid).addr, SEL_ENTRY(sel_curr_tid).value);
          //}
          //fprintf(stderr, "%d:restoring load at %d addr %x with value %x\n", sel_curr_tid, SEL_NUM_MEM(sel_curr_tid),
          //        SEL_ENTRY(sel_curr_tid).addr, SEL_ENTRY(sel_curr_tid).value);
          /* reset mem skip count and invalidate SEL_ENTRY(sel_curr_tid) record */
          SEL_ENTRY(sel_curr_tid).mem_count = -1;
	  if ( SEL_ENTRY(sel_curr_tid).type == 2 )
	  {
              int i = 0;
              XMEM_WRITE_WORD(mem, SEL_ENTRY(sel_curr_tid).addr, SEL_ENTRY(sel_curr_tid).value);
	  }
	  else if ( SEL_ENTRY(sel_curr_tid).type == 3 )
	  {
              MEM_WRITE_BYTE(mem, SEL_ENTRY(sel_curr_tid).addr, SEL_ENTRY(sel_curr_tid).value);
	  }
          else if ( SEL_ENTRY(sel_curr_tid).type == 4 )
	  {
              SEL_END_TRACE(sel_curr_tid) = TRUE;
              return TRUE;
	  }
          sel_read_line(SEL_FD(sel_curr_tid), SEL_LINE(sel_curr_tid));
          if ( SEL_LINE(sel_curr_tid)[0] == '5' ) 
          {
              sel_restore_ldt_entry(SEL_LINE(sel_curr_tid));
              sel_read_line(SEL_FD(sel_curr_tid), SEL_LINE(sel_curr_tid));
          }
	  /* or if we read next sel entry */
	  if ( SEL_LINE(sel_curr_tid)[0] == '2' || SEL_LINE(sel_curr_tid)[0] == '3' || SEL_LINE(sel_curr_tid)[0] == '4' )
	  {
	      sel_read_entry(SEL_LINE(sel_curr_tid), &SEL_ENTRY(sel_curr_tid));
          }
      }
      /* if we read another marker header */
      if ( restored == 1 )
      {
        if ( SEL_LINE(sel_curr_tid)[0] == '0' )
        {
          /* read the next marker header */
          sel_read_marker(SEL_LINE(sel_curr_tid), &SEL_MARKER(sel_curr_tid));
  	  //fprintf(stderr, "reading marker at inst_count: %llu, memcont: %llu marker memcount: %llu\n", 
	  //sim_num_insn, SEL_NUM_MEM(sel_curr_tid), sel_marker.mem_count);	
        }
      }
  }
  return FALSE;
}

/*
 * Reads the marker header and returns the number of registers to restore
 */
void
sel_read_marker(char *str, struct sel_marker *sm)
{
   assert(str);
   assert(sm);
   //sscanf(str, "%d %llu %d %u\n", &sm->type, &sm->mem_count, &sm->num_regs, &sm->mid); 
   sscanf(str, "%d %llu %d\n", &sm->type, &sm->mem_count, &sm->num_regs); 
}

/*
 *  Reads a register entry
 */
void 
sel_read_register(char *str, struct regs_t *regs)
{
   int type;
   unsigned ireg_value;
   efloat_t freg_value;
   char reg_name[REG_NAME_SIZE];
   int index, reg_type;

   assert(str);
   sscanf(str, "%d %s ", &type, reg_name); 

   if ( COMPARE_STR(reg_name, "eip") ) 
   {
     sscanf(str, "%d %s %u\n", &type, reg_name, &ireg_value); 
     //fprintf(stderr, "%s %x\n", reg_name, ireg_value);
     regs->regs_NPC = regs->regs_PC = ireg_value;
     return;
   }
   if ( COMPARE_STR(reg_name, "eflags") ) 
   {
     sscanf(str, "%d %s %u\n", &type, reg_name, &ireg_value); 
     //fprintf(stderr, "%s %x\n", reg_name, ireg_value);
     regs->regs_C.aflags = (word_t)ireg_value;
     return;
   }
   if ( COMPARE_STR(reg_name, "swd") ) 
   {
     sscanf(str, "%d %s %u\n", &type, reg_name, &ireg_value); 
     //fprintf(stderr, "%s %x\n", reg_name, ireg_value);
     regs->regs_C.fsw = (word_t)ireg_value;
     return;
   }
   if ( COMPARE_STR(reg_name, "cwd") ) 
   {
     sscanf(str, "%d %s %u\n", &type, reg_name, &ireg_value); 
     //fprintf(stderr, "%s %x\n", reg_name, ireg_value);
     regs->regs_C.cwd = (word_t)ireg_value;
     return;
   }
   /* INT register */
   if ( (index = get_ireg_index(reg_name, &reg_type)) != INDEX_NOT_FOUND )
   {
     sscanf(str, "%d %s %u\n", &type, reg_name, &ireg_value); 
     //fprintf(stderr, "%s %x\n", reg_name, ireg_value);
     if ( index != INVALID_INDEX )
     {
         if (reg_type == INTEGER_GPR )
         {
             MD_SEL_TO_IREG_GPR(ireg_value, regs, index);
         }
         else if (reg_type == INTEGER_SEG )
         {
             MD_SEL_TO_IREG_SEG(ireg_value, regs, index);
         }
     }
   }
   /* FP register */
   else if ( (index = get_freg_index(reg_name)) != INDEX_NOT_FOUND )
   {
     //char byte_str[3];
     //char fpreg_str[21];
     //int i, j;
     //unsigned char *p = (unsigned char *)&freg_value;
     //sscanf(str, "%d %s %s\n", &type, reg_name, &fpreg_str); 
     sscanf(str, "%d %s %Le\n", &type, reg_name, &freg_value); 
     if ( index != INVALID_INDEX )
     {
       //fprintf(stderr, "%d %s %Le\n", index, reg_name, freg_value);
       MD_SEL_TO_FREG(freg_value, regs, (((regs->regs_C.fsw>>11)&0x7)+index)&0x7);
       //fprintf(stderr, "%s %Le\n", reg_name, regs->regs_F.e[(((regs->regs_C.fsw>>11)&0x7)+index)&0x7]);
       //fprintf(stderr, "%d %d\n", index, (((regs->regs_C.fsw>>11)&0x7)+index)&0x7);
     }
   }
   else
   {
     fatal("invalid register name `%s'", reg_name);
   }
}

/* 
 * Read a sel entry
 */
void
sel_read_entry(char *str, struct sel_entry *se)
{
   assert(str);
   sscanf(str, "%d %llu %x %lx", &se->type, &se->mem_count, &se->addr, &se->value);
}

/* 
 * Read a sel entry
 */
void
sel_restore_ldt_entry(char *str)
{
   int sel_type;
   int entry_number;
   md_addr_t base_addr;
    
   assert(str);
   sscanf(str, "%d %x %x", &sel_type, &entry_number, &base_addr);
   /* restore the ldt entry in the proper place */
   SEL_LDT(sel_curr_tid)[entry_number] = base_addr;
   //fprintf(stderr, "******* ldt entry: %d %x %lx\n", sel_type, entry_number, base_addr);
}


/*
 * Returns the type and the line read from sel file
 */
int 
sel_read_line(FILE *fd, char *str)
{
  if ( !feof(fd) )
  {
    fgets(str, SEL_LINE_SIZE, fd);
    return 1;
  } 
  return 0;
}

/*
 * Read all entries between two SEL markers
 */
void
sel_restore_registers(FILE *fd, 
		      struct sel_marker *sm, 
		      struct regs_t *regs,
		      char *str)
{
   int i;
   /* restore registers */
   for ( i = 0; i < sm->num_regs; i++ )
   {
     sel_read_line(fd, str);
     if ( str[0] != '1' )
       fatal("Invalid SEL register line\n");
     sel_read_register(str, regs);
   }
   /* restore memory */
   //sel_read_line(fd, str);
   //while (str[0] == '2')
   //{
   //    sel_read_entry(str, &se);
   //    //fprintf(stderr, "Writting %x to %x\n", se.value, se.addr);
   //    MEM_WRITE_BYTE(mem, se.addr, se.value);
   //    sel_read_line(fd, str);
   //}
}

int
sel_is_valid_code(md_addr_t addr)
{
  unsigned i;
  int valid = 0;
  if ( sel_flag == TRUE )
  {
      for ( i = 0; i < num_lib_code_sections; i++ )
      {
          valid |= (lib_code_sections[i*2] <= addr && addr <= lib_code_sections[(i*2)+1]);
          if ( valid )
          {
	      break;
          }
      }
      return valid;
  }
  else 
  {
      return 1;
  }
}

/********************************************************************
 * SSI related functions
 *******************************************************************/
/*
 *  Maintain a circular queue with SSI_QUEUE_SIZE entries from the 
 *  SSI file. 
 */
void
ssi_read_entries(FILE *fd)
{
   unsigned bb_size;
   unsigned next_ssi_queue_index = NEXT_INDEX(SSI_QUEUE(sel_curr_tid).index); 
   while ( next_ssi_queue_index != SSI_QUEUE(sel_curr_tid).head || SSI_QUEUE(sel_curr_tid).index == -1 )
   {
       SSI_QUEUE(sel_curr_tid).index = next_ssi_queue_index;

       if ( !feof(fd) ) 
       {
           if ( feof(fd) ) 
           {
               fprintf(stderr, "Unexpected end of file!\n");
               exit(1);
           }
           fscanf(fd, "%llu %x %u\n", &SSI_QUEUE(sel_curr_tid).queue[SSI_QUEUE(sel_curr_tid).index].entry.mem_count, 
			   &SSI_QUEUE(sel_curr_tid).queue[SSI_QUEUE(sel_curr_tid).index].entry.addr, 
			   &SSI_QUEUE(sel_curr_tid).queue[SSI_QUEUE(sel_curr_tid).index].entry.bb_size);
           bb_size = (SSI_QUEUE(sel_curr_tid).queue[SSI_QUEUE(sel_curr_tid).index].entry.bb_size * 2) + 1; // two characteres per byte
	   if ( bb_size > MAX_BB_SIZE )
	   {
		   fatal("BB is too big (%x:%d), needs to increase buffer size!", SSI_QUEUE(sel_curr_tid).queue[SSI_QUEUE(sel_curr_tid).index].entry.addr, bb_size*2);
	   }
           fgets(SSI_QUEUE(sel_curr_tid).queue[SSI_QUEUE(sel_curr_tid).index].bb, bb_size, fd);
       }
       else
       {
	   break;
       }
       next_ssi_queue_index = NEXT_INDEX(SSI_QUEUE(sel_curr_tid).index);
   } 
}

/*
 *
 */
int
ssi_fetch_inst(md_addr_t addr, md_inst_t *inst)
{
    if ( sel_flag )
    {
	    int i = SSI_QUEUE(sel_curr_tid).head;
	    do 
	    {
	    if ( SSI_QUEUE(sel_curr_tid).queue[i].entry.addr <= addr && 
		 addr < (SSI_QUEUE(sel_curr_tid).queue[i].entry.addr + SSI_QUEUE(sel_curr_tid).queue[i].entry.bb_size))
	    {
		    char str_byte[3];
		    int j, k, l = 0;
		    //fprintf(stderr, "Restoring %x during fetch (%x:%x)\n", addr, SSI_QUEUE(sel_curr_tid).queue[i].entry.addr,
		    // 	       SSI_QUEUE(sel_curr_tid).queue[i].entry.addr + SSI_QUEUE(sel_curr_tid).queue[i].entry.bb_size);
		    for ( j = addr - SSI_QUEUE(sel_curr_tid).queue[i].entry.addr; l < MD_MAX_ILEN && 
				  j < SSI_QUEUE(sel_curr_tid).queue[i].entry.bb_size; j++, l++ )
		    {
			    for ( k = 0; k < 2; k++ )
			    {
				    str_byte[k] = SSI_QUEUE(sel_curr_tid).queue[i].bb[(2*j)+k];
			    }
			    str_byte[2] = 0;
			    inst->code[l] = (byte_t)strtol(str_byte, NULL, 16);
		    }
		    return 1;
	    }
	    i = NEXT_INDEX(i);
	    } while ( i != SSI_QUEUE(sel_curr_tid).head );
    }
    return 0; 
}

/*
 * Read bb header
 */
int
ssi_read_bb_header(FILE *fd)
{
   //fscanf(fd, "%llu %x %u\n", &SSI_ENTRY(sel_curr_tid).mem_count, &SSI_ENTRY(sel_curr_tid).addr, &SSI_ENTRY(sel_curr_tid).bb_size);
   SSI_ENTRY(sel_curr_tid).mem_count = SSI_QUEUE(sel_curr_tid).queue[SSI_QUEUE(sel_curr_tid).head].entry.mem_count;
   SSI_ENTRY(sel_curr_tid).addr = SSI_QUEUE(sel_curr_tid).queue[SSI_QUEUE(sel_curr_tid).head].entry.addr;
   SSI_ENTRY(sel_curr_tid).bb_size = SSI_QUEUE(sel_curr_tid).queue[SSI_QUEUE(sel_curr_tid).head].entry.bb_size;

   //fprintf(stderr, "Reading header: %llu %x %u\n", SSI_ENTRY(sel_curr_tid).mem_count, SSI_ENTRY(sel_curr_tid).addr, SSI_ENTRY(sel_curr_tid).bb_size);
   if ( !feof(fd) )
   {
       //fprintf(stderr, "%llu %x\n", *insn_count, *addr);
       return 1;
   }
   return 0;
}

/*
 * 
 */
void
ssi_restore_bb(FILE *fd, struct mem_t *mem)
{
  char str_byte[3];
  byte_t data; 
  unsigned i, j;
  char bb[MAX_BB_SIZE];
  
  if ( SSI_QUEUE(sel_curr_tid).index == NEXT_INDEX(SSI_QUEUE(sel_curr_tid).head) ) 
  {
    fprintf(stderr, "Unexpected end of file %d!\n", i);
    exit(1);
  }

  //fprintf(stderr, "Restoring bb %x\n", SSI_ENTRY(sel_curr_tid).addr);
  memcpy(bb, SSI_QUEUE(sel_curr_tid).queue[SSI_QUEUE(sel_curr_tid).head].bb, MAX_BB_SIZE);
  SSI_QUEUE(sel_curr_tid).head = NEXT_INDEX(SSI_QUEUE(sel_curr_tid).head);
  ssi_read_entries(SSI_FD(sel_curr_tid));

  for ( i = 0; i < SSI_ENTRY(sel_curr_tid).bb_size; i++ )
  {
    for ( j = 0; j < 2; j++ )
    {
      str_byte[j] = bb[(2*i)+j];
    }
    str_byte[2] = 0;
    data = (byte_t)strtol(str_byte, NULL, 16);
    if ( mem )
    {
      //fprintf(stderr, "%x ", data);
      MEM_WRITE_BYTE(mem, SSI_ENTRY(sel_curr_tid).addr+i, data);
    }
    //fprintf(stderr, "\n");
  }
}

/*
 *
 */
int 
ssi_bb_check_restore(struct mem_t *mem)
{
  /* if SEL is used, check whether it is time to restore */
  if ( sel_flag == TRUE )
  {
      //fprintf(stderr, "ssi_bb_check_restore: %llu %llu\n", SSI_ENTRY(sel_curr_tid).mem_count, SEL_NUM_MEM(sel_curr_tid));
      while ( SSI_ENTRY(sel_curr_tid).mem_count == SEL_NUM_MEM(sel_curr_tid) ) 
      {
          SSI_ENTRY(sel_curr_tid).mem_count = -1;
	  // restore the bb corresponding to the last header read 
          ssi_restore_bb(SSI_FD(sel_curr_tid), mem);
	  // read the next header
          ssi_read_bb_header(SSI_FD(sel_curr_tid));
      }
  }
  return FALSE;
}

/*
 * Read page header
 */
int
ssi_read_page_header(FILE *fd, counter_t *insn_count, md_addr_t *addr)
{
   fscanf(fd, "%llu %x\n", insn_count, addr);
   if ( !feof(fd) )
   {
       //fprintf(stderr, "%llu %x\n", *insn_count, *addr);
       return 1;
   }
   return 0;
}

void
sel_load(char *fname, struct mem_t *mem, struct regs_t *regs)
{
  unsigned i;

  sel_allocate();

  fprintf(stderr, "Loading pinsel files\n");

  if ( sel_num_threads > 1 )
  {
    cs_fd = fopen(cs_fname, "r");
    if ( !cs_fd )
    {
        fatal("Cannot open file `%s' ", cs_fname);
    }
  }

  sec_fd = fopen(sec_fname, "r");
  if ( !sec_fd )
  {
      fatal("Cannot open file `%s' ", sec_fname);
  }

  for ( i = 0; i < sel_num_threads; i++ )
  {
    SEL_END_TRACE(i) = FALSE;

    /* ssi files */
    SSI_FNAME(i) = (char *)malloc(strlen(fname)+10);
    sprintf(SSI_FNAME(i), "%s_%d.ssi", fname, i);
    SSI_FD(i) = fopen(SSI_FNAME(i), "r");
    if ( !SSI_FD(i) )
    {
        fatal("Cannot open file `%s' ", SSI_FNAME(i));
    }
    /* sel files */
    SEL_FNAME(i) = (char *)malloc(strlen(fname)+10);
    sprintf(SEL_FNAME(i), "%s_%d.sel", fname, i);
    SEL_FD(i) = fopen(SEL_FNAME(i), "r");
    if ( !SEL_FD(i) )
    {
        fatal("Cannot open file `%s' ", SEL_FNAME(i));
    }
  }

  /* read pages */
  //ssi_read(SSI_FD(sel_curr_tid), mem);
    
  ssi_read_entries(SSI_FD(sel_curr_tid)); 
     
  /* read first bb header */
  ssi_read_bb_header(SSI_FD(sel_curr_tid));

  /* restore all bbs with memcount equal to zero */
  ssi_bb_check_restore(mem);

  /* initialize sel variables */
  sel_init(&ld_text_base, &ld_text_size);

  /* restore the first SEL record */
  sel_restore_marker(regs);

  free(sec_fname);
  free(cs_fname);
  for ( i = 0; i < sel_num_threads; i++ )
  {
      free(SEL_FNAME(i));
      free(SSI_FNAME(i));
  }
}

/*
 * Clean up the house
 */
void
sel_end()
{
  int i;
  if ( sel_num_threads > 1 )
  {
      fclose(cs_fd);
  }
  fclose(sec_fd);
  for ( i = 0; i < sel_num_threads; i++ )
  {
      fclose(SSI_FD(i));
      fclose(SEL_FD(i));
  }
}

/*
 * Read the next entry from the context switch file
 */
void 
sel_read_cs_entry()
{
    if ( fscanf(cs_fd, "%d %llu %d\n", &cs_entry.from_tid, &cs_entry.mem_count, &cs_entry.to_tid) != 3 )
    {
        fatal("Could not read record from CS file\n");
    }
    //fprintf(stderr, "cs_entry: %d %llu %d\n", cs_entry.from_tid, cs_entry.mem_count, cs_entry.to_tid);
}

/*
 * Context switch if the time to do so has arrived
 */
int
sel_schedule(struct regs_t *regs, int thread_end)
{
   if ( sel_num_threads > 1 && sel_flag == TRUE )
   {
       /* compare memory count of curr_tid with curr_mem_count from cs file */
       if ( (sel_curr_tid == cs_entry.from_tid && 
             SEL_NUM_MEM(sel_curr_tid) >= cs_entry.mem_count ) || thread_end )
       {
           //fprintf(stderr, "Switching from %d[%llu] to %d\n", 
           //    sel_curr_tid, SEL_NUM_MEM(sel_curr_tid), cs_entry.to_tid);

           sel_curr_tid = cs_entry.to_tid;
           save_context(cs_entry.from_tid, regs);
           if ( SEL_NUM_MEM(sel_curr_tid) == 0 )
           {
               ssi_read_entries(SSI_FD(sel_curr_tid)); 
     
               /* read first bb header */
               ssi_read_bb_header(SSI_FD(sel_curr_tid));

               /* this function uses sel_curr_tid internally, so don't move it before the 
                  assignment to sel_curr_tid above */
               sel_restore_marker(regs);
           }
           else 
           {
               restore_context(cs_entry.to_tid, regs);
           }
           ldt_p = SEL_LDT(sel_curr_tid);
           /* read next cs entry */
           sel_read_cs_entry();
           return 1;
       }
   }
   return 0;
}

#if 0
/*
 * Restore a page into simulated memory
 */
#define STR_PAGE_SIZE ((PAGE_SIZE*2)+1)
void
ssi_restore_page(FILE *fd, md_addr_t addr, struct mem_t *mem)
{
  char str_byte[3];
  byte_t data; 
  unsigned i, j;
  char page[STR_PAGE_SIZE];

  fgets(page, STR_PAGE_SIZE, fd);
  if ( feof(fd) ) 
  {
    fprintf(stderr, "Unexpected end of file %d!\n", i);
    exit(1);
  }
  for ( i = 0; i < PAGE_SIZE; i++ )
  {
    for ( j = 0; j < 2; j++ )
    {
      //if ( feof(fd) ) 
      //{
      //  fprintf(stderr, "Unexpected end of file %d!\n", i);
      //  exit(1);
      //}
      //str_byte[j] = fgetc(fd);
      str_byte[j] = page[(2*i)+j];
    }
    str_byte[2] = 0;
    data = (byte_t)strtol(str_byte, NULL, 16);
    if ( mem )
    {
      MEM_WRITE_BYTE(mem, addr+i, data);
    }
  }
  /* read the newline */
  //fgetc(fd);
} 

/* 
 * Read a page from ssi file until a non-zero instruction count is found. 
 * If instruction count is 0 store to memory, otherwise, stop reading until 
 * the instruction count is reached
 */
void
ssi_read(FILE *fd, struct mem_t *mem)
{
  unsigned num_pages = 0;
  /* read all the pages */
  while (1)
  {
    counter_t page_insn_count;
    md_addr_t addr;

    if ( !ssi_read_page_header(fd, &page_insn_count, &addr) )
      break;

    /* copy page to memory */
    if ( page_insn_count == 0 )
    {
       ssi_restore_page(fd, addr, mem);
    }
    /* store page and restore when the instruction count is reached */
    else
    {
       /* TODO: either save the page somewhere or stop reading from the
	* file and start again when the IC is reached */ 
       assert(0);
       ssi_restore_page(fd, addr, NULL);
       break;
    }
    num_pages++;
  }
  fprintf(stderr, "%d pages restored\n", num_pages);
}
#endif

/********************************************************************
 * Other functions, related to simplescalar structures
 *******************************************************************/
/* 
 * Print sel statistics
 */
void
sel_print_stats()
{
  fprintf(stderr, "Memcount: %llu\n", SEL_NUM_MEM(sel_curr_tid));
}

/* 
 * SEL command line options
 */
void
sel_reg_options(struct opt_odb_t *odb)
{
  /* interval size */
  opt_reg_int(odb, "-sel_num_threads", "number of threads to execute from the sel logs",
              &sel_num_threads, /* default */1,
              /* print */TRUE, /* format */NULL);
}
