/* mem-interface.h - routines that interface the DRAM memory system to
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

#ifndef MEM_INTERFACE_H
#define MEM_INTERFACE_H

#include "options.h"
#include "memory.h"
#include "cache.h"

#define RID_INVALID                     -1

/* request id variable for DRAM system */
extern int req_id;

/* Registers dram related options. */
void dram_reg_options(struct opt_odb_t *odb);

/* Checks dram related options. */
void dram_check_options(struct opt_odb_t *odb);

/* Initiates a new memory request. */
mem_status_t
dram_access_memory(enum mem_cmd cmd,
		   md_addr_t baddr, 
		   tick_t now, 
		   dram_access_t *access);

/* Updates the DRAM memory system, called each cycle.  Replaces dram_cb_check. */
void dram_update_system(tick_t now);

/* Prints the dram stats, called from sim_aux_stats. */
void dram_print_stats_and_close();

/* Squashes bid entry from memory system. */
void dram_squash_entry(int rid);

#if 0
void dram_dump_config(FILE *stream);
#endif

#endif
