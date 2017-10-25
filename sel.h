#ifndef SEL_H
#define SEL_H

#include "sim.h"

#define USE_SEL 0 // jdonald set to 0 for now, no SEL
#define TRACE_DEBUG 0
#define MAX_CODE_SECTIONS (128*2)
#define MAX_SMIL_THREADS (10)
#define SEL_LINE_SIZE 128
#define MAX_BB_SIZE 2048
#define SSI_QUEUE_SIZE 16 
#define SEL_NUM_MEM(tid) (threads[tid].sel_num_mem)
#define SEL_END_TRACE(tid)  (threads[tid].sel_end_trace)
#define SEL_LDT(tid)  (threads[tid].ldt)
#define SEL_ICOUNT(tid)  (threads[tid].icount)
#define SEL_REP_FLAG(tid)  (threads[tid].sel_rep_flag)

//extern counter_t *sel_num_mem;
extern md_addr_t lib_code_sections[MAX_CODE_SECTIONS];
extern unsigned num_lib_code_sections;
//extern char *ssi_fname;
//extern char *sec_fname;
//extern char *sel_fname;
//extern char *cs_fname;
//extern FILE *ssi_fd;
//extern FILE *sec_fd;
//extern FILE *sel_fd;
//extern FILE *cs_fd;
extern int sel_flag;
//extern int sel_end_trace;
extern int sel_curr_tid;
extern struct sel_thread *threads;
extern int sel_num_threads;   /* number of threads */

/*-----------------------------------------------------------------------------------------
 types
-----------------------------------------------------------------------------------------*/
struct sel_marker {
  int type;
  counter_t mem_count;
  int num_regs;
};

struct sel_entry {
  int type;
  counter_t mem_count;
  md_addr_t addr;
  long int value;
};

struct ssi_entry {
  counter_t mem_count;
  md_addr_t addr;
  unsigned bb_size;
};

struct cs_entry {
  counter_t mem_count;
  int from_tid, to_tid;
};

struct ssi_queue_entry {
   struct ssi_entry entry;
   char bb[MAX_BB_SIZE];
};

struct ssi_queue_t {
   struct ssi_queue_entry queue[SSI_QUEUE_SIZE];
   unsigned head; 
   unsigned index;
};

/* one structure for each thread, with all the state needed to restore the logs
   from this thread */
struct sel_thread {
  counter_t icount; /* instruction count */
  counter_t sel_num_mem; /* memory instruction count */
  int sel_rep_flag; /* memory instruction count */
  char sel_line[SEL_LINE_SIZE];
  int sel_end_trace; /* end of the trace flag */
  struct regs_t *th_regs; /* register state for this thread */
  struct sel_marker sel_marker;
  struct sel_entry sel_entry;
  struct ssi_entry ssi_entry;
  struct ssi_queue_t ssi_queue; 
  char *ssi_fname; /* start image file */
  char *sel_fname; /* system effect logs */
  FILE *ssi_fd;    /* start image file descriptor */
  FILE *sel_fd;    /* system effect log file descriptor */
  md_addr_t ldt[NUM_LDT_ENTRIES];
};

int
ssi_fetch_inst(md_addr_t addr, md_inst_t *inst);

void
ssi_read_entries(FILE *fd);
 
int
ssi_read_bb_header(FILE *fd);

void
ssi_restore_bb(FILE *fd, struct mem_t *mem);

int 
ssi_bb_check_restore(struct mem_t *mem);

void 
sel_init(md_addr_t *ld_text_base, unsigned int *ld_text_size);

int 
sel_check_restore(struct mem_t *mem);

int 
sel_restore_marker(struct regs_t *regs);

int 
sel_valid_files(char *fname);

void
sel_read_marker(char *str, struct sel_marker *sm);

void
sel_read_register(char *str, struct regs_t *regs);

void
sel_read_entry(char *str, struct sel_entry *se);

int
sel_read_line(FILE *fd, char *str);

void
ssi_read(FILE *fd, struct mem_t *mem);

void
sel_print_stats();

void
sel_reg_options(struct opt_odb_t *odb);

int
sel_is_valid_code(md_addr_t addr);

void
sel_load(char *fname, struct mem_t *mem, struct regs_t *regs);

void
sel_end();

int
sel_schedule(struct regs_t *regs, int thread_end);

#endif
