/* mem-interface.c - routines that interface the DRAM memory system to
 * the rest of SimpleScalar */

/* MASE was created by Eric Larson, Seokwoo Lee, Drew Hamel,
 * Saugata Chatterjee, Dan Ernst, and
 * Todd M. Austin at the University of Michigan.
 */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2002 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 2000-2002 by The Regents of The University of Michigan.
 * Copyright (C) 1994-2002 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * Copyright (c) 2002 by David Wang and Bruce Jacob
 * David Wang, Bruce Jacob
 * Systems and Computer Architecture Lab
 * Dept of Electrical & Computer Engineering
 * University of Maryland, College Park
 * All Rights Reserved
 */

#include <stdio.h>
#include "options.h"
#include "memory.h"
#include "cache.h"
#ifndef MEMORY_SYSTEM_H
#include "mem-system.h"
#include "mem-biu.h"
#endif

extern int mem_use_dram;
biu_t *global_biu;

/* request id variable for DRAM system */
int req_id;

/* DRAM system options */
int cpu_frequency;
int dram_frequency;
int channel_count;                      /* total number of logical channels */
int channel_width;                      /* logical channel width in bytes */
int transaction_granularity;            /* how long is a cacheline burst? unit in bytes */
int strict_ordering_flag;
int refresh_interval;                   /* in milliseconds */
int auto_precharge_interval;            /* in dram clock ticks */

int chipset_delay;                      /* latency through chipset, in DRAM ticks */
int biu_depth;				/* how many slots in biu */
int biu_delay;                          /* latency through BIU, in CPU ticks */
int markov_memory_prefetch;             /* secret switch. Don't use yet */
int markov_table_prefetch;              /* secret switch. Don't use yet */
int stream_prefetch;                    /* secret switch. Don't use yet */
int prefetch_depth;			/* how many prefetches should be generated? */

/* dram type {sdram|ddrsdram|drdram|ddr2|2lev} */
char *dram_type_string;

/* address_mapping_policy {burger_base_map|burger_alt_map|sdram_base_map|intel845g_map} */
char *address_mapping_policy_string;

/* row_buffer_management_policy {open_page|close_page|counter} */
char *row_buffer_management_policy_string;

/* DRAM debug options */
int biu_debug_flag;
int transaction_debug_flag;
int dram_debug_flag;
int prefetch_debug_flag;
int all_debug_flag;     /* if this is set, all dram related debug flags are turned on */
int wave_debug_flag;    /* cheesy Text Based waveforms  */

/* spd file input */
char *spd_filein;
FILE *spd_fileptr;

/* dram and bus stat options */
char *biu_stats_fileout;
char *bank_hit_stats_fileout;
char *bank_conflict_stats_fileout;
char *cas_per_ras_stats_fileout;

/* Registers dram related options. */
void dram_reg_options(struct opt_odb_t *odb) {

  /* --- dram system options ---- */

  opt_reg_int(odb, "-cpu:frequency",
      "frequency of cpu (in MHz)",
      &cpu_frequency, /* default */100,
      /* print */TRUE, /* format */NULL);

  /* dram type {sdram|ddrsdram|drdram|ddr2} */
  opt_reg_string(odb, "-dram:type",
      "DRAM type ",
      &dram_type_string, /* default */"none",
      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-dram:frequency",
      "frequency of dram relative to cpu",
      &dram_frequency, /* default */INVALID,
      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-dram:channel_count",
      "number of dram channels",
      &channel_count, /* default */INVALID,
      /* print */TRUE, /* format */NULL);

  /* channel width is in bytes */
  opt_reg_int(odb, "-dram:channel_width",
      "width of a logical dram channel in bytes",
      &channel_width, /* default */INVALID,
      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-dram:refresh",
      "Enable refresh and set refresh interval, in miliseconds",
      &refresh_interval, /* default */0,
      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-dram:chipset_delay",
      "latency of transaction through chipset in DRAM ticks",
      &chipset_delay, /* default */INVALID,
      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-biu:depth",
      "Number of slots in the bus queue",
      &biu_depth, /* default */32,
      /* print */TRUE, /* format */NULL);

  opt_reg_int(odb, "-biu:delay",
      "latency of transaction through chipset in CPU ticks",
      &biu_delay, /* default */3,
      /* print */TRUE, /* format */NULL);

  /* row_buffer_management_policy {open_page|close_page|auto_precharge} */
  opt_reg_string(odb, "-dram:row_buffer_management_policy",
      "Row buffer management policy ",
      &row_buffer_management_policy_string, /* default */"open_page",
      /* print */TRUE, /* format */NULL);

  /* overrides row buffer management policy if this is set */
  opt_reg_int(odb, "-dram:auto_precharge",
      "Enable auto precharge and set precharge interval, in clock ticks",
      &auto_precharge_interval, /* default */0,
      /* print */TRUE, /* format */NULL);

  /* address_mapping_policy {burger_map|burger_alt_map|sdram_base_map|intel845g_map} */
  opt_reg_string(odb, "-dram:address_mapping_policy",
      "Address Mapping Policy ",
      &address_mapping_policy_string, /* default */"sdram_base_map",
      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-dram:strict_ordering", "Strict memory transaction ordering or not",
      &strict_ordering_flag, /* default */FALSE,
      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-dram:markov_memory_prefetch", "In memory storage of Markov Table prefetch hints",
      &markov_memory_prefetch, /* default */FALSE,
      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-dram:markov_table_prefetch", "In memory storage of Markov Table prefetch hints",
      &markov_table_prefetch, /* default */FALSE,
      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-dram:stream_prefetch", "Stream Prefetch",
      &stream_prefetch, /* default */FALSE,
      /* print */TRUE, /* format */NULL);

  /* sets prefetch depth */
  opt_reg_int(odb, "-dram:prefetch_depth", "Sets prefetch depth, default is 1",
      &prefetch_depth, /* default */1,
      /* print */TRUE, /* format */NULL);

  /* --- BUS/DRAM debugging options --- */

  opt_reg_flag(odb, "-debug:biu", "Debug Bus Interface to memory controller",
      &biu_debug_flag, /* default */FALSE,
      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-debug:trans", "Debug memory transaction interface",
      &transaction_debug_flag, /* default */FALSE,
      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-debug:dram", "Debug memory system",
      &dram_debug_flag, /* default */FALSE,
      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-debug:prefetch", "Debug Markov Prefetch",
      &prefetch_debug_flag, /* default */FALSE,
      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-debug:all", "Debug all in memory system",
      &all_debug_flag, /* default */FALSE,
      /* print */TRUE, /* format */NULL);

  opt_reg_flag(odb, "-debug:wave", "Display memory system waveform",
      &wave_debug_flag, /* default */FALSE,
      /* print */TRUE, /* format */NULL);

  /* --- DRAM spd file input --- */

  opt_reg_string(odb, "-dram:spd_input",
      "DRAM spd input filename ",
      &spd_filein, /* default */NULL,
      /* print */TRUE, /* format */NULL);

  /* --- DRAM stat gathering options --- */

  opt_reg_string(odb, "-stat:biu",
      "BIU stat output filename ",
      &biu_stats_fileout, /* default */NULL,
      /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-stat:dram:bank_hit",
      "Stat Gathering for dram bank hit interval - output filename",
      &bank_hit_stats_fileout, /* default */NULL,
      /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-stat:dram:bank_conflict",
      "Stat Gathering for dram bank conflict interval - output filename",
      &bank_conflict_stats_fileout, /* default */NULL,
      /* print */TRUE, /* format */NULL);

  opt_reg_string(odb, "-stat:dram:cas_per_ras",
      "Stat Gathering for dram cas per ras - output filename",
      &cas_per_ras_stats_fileout, /* default */NULL,
      /* print */TRUE, /* format */NULL);
}

/* Checks dram related options. */
void dram_check_options(struct opt_odb_t *odb) {

  /* initialize the BIU before DRAM memory system*/
  
  global_biu = get_biu_address();
  init_biu(global_biu);

  set_cpu_frequency(global_biu, cpu_frequency);            /* input value in MHz */

  /* initialize the DRAM memory system */
  init_dram_system();

  if (!mystricmp(dram_type_string, "sdram")){
    set_dram_type(SDRAM);                      /* SDRAM, DDRSDRAM, DRDRAM etc */
  } else if (!mystricmp(dram_type_string, "ddrsdram")){
    set_dram_type(DDRSDRAM);
  } else if (!mystricmp(dram_type_string, "drdram")){
    set_dram_type(DRDRAM);
  } else if (!mystricmp(dram_type_string, "ddr2")){
    set_dram_type(DDR2);
  } else {
    set_dram_type(SDRAM);
  }

  set_dram_transaction_granularity(64);		/* How many bytes are fetched per transaction? */

  /* the spd file, if any, will become the default values that can be overridden by command line switches */

  if((spd_filein != NULL) && ((spd_fileptr = fopen(spd_filein, "r+")) != NULL)){
	read_dram_config_from_file(spd_fileptr,get_dram_system_config());
  }

  /* Finally, allow the command line switches to override defaults */
  if(dram_frequency != INVALID){
    set_dram_frequency(dram_frequency);			/* units in MHz */
  }
  if(channel_count != INVALID){
    set_dram_channel_count(channel_count);	/* single channel of memory */
  }
  if(channel_width != INVALID){
    set_dram_channel_width(channel_width);	/* data bus width, in bytes */
  }

  if(refresh_interval > 0){
    enable_auto_refresh(TRUE,refresh_interval); /* set auto refresh, cycle through every "refresh_interval" ms */
  } else {
    enable_auto_refresh(FALSE,refresh_interval);
  }

  if(markov_memory_prefetch){
    init_prefetch_cache(MARKOV_MEMORY_PREFETCH);
  } else if(markov_table_prefetch){
    init_prefetch_cache(MARKOV_TABLE_PREFETCH);
  } else if(stream_prefetch){
    init_prefetch_cache(STREAM_PREFETCH);
  } else {
    init_prefetch_cache(NO_PREFETCH);
  }
  set_prefetch_depth(prefetch_depth);

  set_strict_ordering(strict_ordering_flag);

  if (!mystricmp(address_mapping_policy_string, "burger_base_map")){
    set_dram_address_mapping_policy(BURGER_BASE_MAP);  /* How to map physical address to memory address */
  } else if (!mystricmp(address_mapping_policy_string, "burger_alt_map")){
    set_dram_address_mapping_policy(BURGER_ALT_MAP);
  } else if (!mystricmp(address_mapping_policy_string, "sdram_base_map")){
    set_dram_address_mapping_policy(SDRAM_BASE_MAP);
  } else if (!mystricmp(address_mapping_policy_string, "sdram_hiperf_map")){
    set_dram_address_mapping_policy(SDRAM_HIPERF_MAP);
  } else if (!mystricmp(address_mapping_policy_string, "intel845g_map")){
    set_dram_address_mapping_policy(INTEL845G_MAP);
  }

  if (!mystricmp(row_buffer_management_policy_string, "open_page")){
    set_dram_row_buffer_management_policy(OPEN_PAGE);               /* OPEN_PAGE, CLOSED_PAGE etc */
  } else if (!mystricmp(row_buffer_management_policy_string, "close_page")){
    set_dram_row_buffer_management_policy(CLOSE_PAGE);
  } else if (!mystricmp(row_buffer_management_policy_string, "perfect_page")){
    set_dram_row_buffer_management_policy(PERFECT_PAGE);
  }

  set_auto_precharge_interval(auto_precharge_interval);
  if(auto_precharge_interval > 0){  /* override row buffer management policy if the precharge counter is set */
    set_dram_row_buffer_management_policy(AUTO_PAGE);
  }

  if(chipset_delay != INVALID){      /* someone overrode default, so let us set the delay for them */
    set_chipset_delay(chipset_delay);
  }
  set_biu_delay(global_biu,biu_delay);
  set_biu_depth(global_biu,biu_depth);
  set_biu_debug(global_biu, biu_debug_flag);
  set_dram_debug(dram_debug_flag);
  set_transaction_debug(transaction_debug_flag);
  set_prefetch_debug(prefetch_debug_flag);
  set_wave_debug(wave_debug_flag);

  if(all_debug_flag == TRUE){
    set_biu_debug(global_biu, TRUE);
    set_dram_debug(TRUE);
    set_transaction_debug(TRUE);
    set_prefetch_debug(TRUE);
    set_wave_debug(FALSE);
  } else if (wave_debug_flag == TRUE){
    set_biu_debug(global_biu, FALSE);
    set_dram_debug(FALSE);
    set_transaction_debug(FALSE);
    set_prefetch_debug(FALSE);
    set_wave_debug(TRUE);
  }
	  mem_gather_stat_init(GATHER_BUS_STAT,       biu_stats_fileout);
	  mem_gather_stat_init(GATHER_BANK_HIT_STAT,  bank_hit_stats_fileout);
	  mem_gather_stat_init(GATHER_BANK_CONFLICT_STAT,bank_conflict_stats_fileout);
	  mem_gather_stat_init(GATHER_CAS_PER_RAS_STAT,cas_per_ras_stats_fileout);
}

/* Initiates a new memory request. */
mem_status_t
dram_access_memory(enum mem_cmd cmd,
    md_addr_t baddr, 
    tick_t now, 
    dram_access_t *access)    /* data uses by dram system */
{
  mem_status_t result;
  int slot_id;
  int access_type;
  int thread_id;

  thread_id = 0;

  result.lat = 0;

  /* access main memory */
  switch (access->type) {
  case IREAD:
    access_type = MEMORY_IFETCH_COMMAND;
    break;
  case DREAD:
    access_type = MEMORY_READ_COMMAND;
    break;
  case DWRITE:
    if(cmd == Read) { access_type = MEMORY_READ_COMMAND; }
    else { access_type = MEMORY_WRITE_COMMAND; }
    break;
  default:
    fatal("unknown access type");
}

  if((cmd != Write) && active_prefetch() && pfcache_hit_check(baddr)){
    remove_pfcache_entry((unsigned int)baddr);
    result.status = MEM_KNOWN;
    result.lat = pfcache_hit_latency();
    return result;
  } else if ((cmd != Write) && active_prefetch() && (biu_hit_check(global_biu, access_type, (unsigned int) baddr, 0, access->rid, now) == TRUE)) {
    result.status = MEM_UNKNOWN;
    return result;
  } else {
    slot_id = find_free_biu_slot(global_biu, INVALID);  /* make sure this has highest priority */
    if (slot_id == INVALID){
      /* can't get a free slot, retry later */
      result.status = MEM_RETRY;
      return result;
    } else {  
      fill_biu_slot(global_biu, slot_id, thread_id, now, access->rid, (unsigned int) baddr,
	  access_type, access->priority, access->callback_fn);
      result.status = MEM_UNKNOWN;
      return result;
    }
  }
  return result;
}

/* Updates the DRAM memory system, called each cycle.  Replaces dram_cb_check. */
void dram_update_system(tick_t now) {
  int sid, access_type;
  unsigned int    address;
  int thread_id;

  thread_id = 0;
  set_current_cpu_time(global_biu,now);
  if(bus_queue_status_check(global_biu, thread_id) == BUSY){
    if (!update_dram_system(now)) return;
  }
  else {
    return;
  }

  /* look for loads that have already returned the critical word */
  sid = find_critical_word_ready_slot(global_biu, thread_id);
  if (sid != INVALID) { 
    biu_invoke_callback_fn(global_biu, sid);

  /* retire at most 1 transaction per call back check */
  } else {
    sid = find_completed_slot(global_biu, thread_id);    
    if(sid != INVALID){
      access_type = get_access_type(global_biu, sid);
      address = get_address(global_biu, sid);
      if (!callback_done(global_biu, sid)) {
        fatal("completed slot found without executed callback_fn");
      }
      if(access_type == MEMORY_PREFETCH){
	update_prefetch_cache(address);
      }
      if(active_prefetch() && ((access_type == MEMORY_IFETCH_COMMAND) || (access_type == MEMORY_READ_COMMAND))){
        update_recent_access_history(access_type, address);
        check_prefetch_algorithm(access_type, address, now);
      } 
      release_biu_slot(global_biu, sid);  
    }
  }
}

/* Prints the dram stats, called from sim_aux_stats. */
void dram_print_stats_and_close()
{
  mem_print_stat(GATHER_BUS_STAT);
  mem_print_stat(GATHER_BANK_HIT_STAT);
  mem_print_stat(GATHER_BANK_CONFLICT_STAT);
  mem_print_stat(GATHER_CAS_PER_RAS_STAT);
  mem_close_stat_fileptrs();
  print_biu_access_count(global_biu);
}

#if 0
void dram_dump_config(FILE *stream)
{
  fprintf(stream, "** DRAM config **\n");
  fprintf(stream, "  type: %d\n  freq: %d\n  mfreq: %d\n  mpol: %d\n", 
      dram_system.dram_type, dram_system.cpu_frequency, dram_system.memory_frequency,
      dram_system.mapping_policy);
  fprintf(stream, "  cpu2mem: %f\n  mem2cpu: %f\n  clk_gran: %d\n  tran_delay: %d\n",
      dram_system.cpu2mem_clock_ratio, dram_system.mem2cpu_clock_ratio, 
      dram_system.dram_clock_granularity, dram_system.transaction_queue_delay);
  fprintf(stream, "  bq_delay: %d\n  rb_cnt: %d\n  channels: %d\n  banks: %d\n  burstlen: %d\n", 
      dram_system.bus_queue_delay, dram_system.row_buffer_count, dram_system.channel_count, 
      dram_system.bank_count, dram_system.dram_burst_size);
  fprintf(stream, "  linesz: %d\n  packets: %d\n  buswidth: %d\n  rc_dur: %d\n  cc_dur: %d\n",
      dram_system.cacheline_size, dram_system.packet_count, dram_system.data_bus_width,
      dram_system.row_command_duration, dram_system.col_command_duration);
  fprintf(stream, "  t_rc: %d\n  t_rcd: %d\n  t_cac: %d\n  t_ras: %d\n  t_rp: %d\n  t_pp: %d\n",
      dram_system.t_rc, dram_system.t_rcd, dram_system.t_cac, dram_system.t_ras,
      dram_system.t_rp, dram_system.t_pp);
  fprintf(stream, "  t_rr: %d\n  t_cwd: %d\n  t_rtr: %d\n  t_packet: %d\n  t_dqs: %d\n\n",
      dram_system.t_rr, dram_system.t_cwd, dram_system.t_rtr, dram_system.t_packet, 
      dram_system.t_dqs_penalty);
}
#endif



