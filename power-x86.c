/* I inclued this copyright since we're using Cacti for some stuff */

/*------------------------------------------------------------
 *  Copyright 1994 Digital Equipment Corporation and Steve Wilton
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-
 * free right and license under any changes, enhancements or extensions
 * made to the core functions of the software, including but not limited to
 * those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to Digital any
 * such changes, enhancements or extensions that they make and inform Digital
 * of noteworthy uses of this software.  Correspondence should be provided
 * to Digital at:
 *
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/

/* #include "smt.h" */
#include <math.h>
#include "power-x86.h"
#include "machine.h"
#include "cache.h"
#include <assert.h>


// #include "delta.h" // jdonald: moved this and the following three other stat extras into stats.h
// #include "cond.h"
// #include "burst.h"
// #include "within.h"
#if 0
#include "dwt.h" 
#endif

#define PIPE_POWER 

#define SensePowerfactor (Mhz)*(Vdd/2)*(Vdd/2)
#define Sense2Powerfactor (Mhz)*(2*.3+.1*Vdd)
#define Powerfactor (Mhz)*Vdd*Vdd
#define LowSwingPowerfactor (Mhz)*.2*.2
/* set scale for crossover (vdd->gnd) currents */
double crossover_scaling = 1.2;
/* set non-ideal turnoff percentage */
double turnoff_factor = 0.2;

#define MSCALE (LSCALE * .624 / .2250)

// jdonald
#if !defined(TEMPERATURE)
	#define TEMPERATURE 1
#endif

#if( TEMPERATURE )
	#define HOTSPOT_VAR
	/* added for HotSpot */
#else
	#define HOTSPOT_VAR static
#endif

extern int verbose;
//=FALSE;

#define DUMP_TRACE
int maxmin_current_cycle=0;
static double max_current=0.0;
static double min_current=1000.0;
static double total_current=0.0;

FILE *fp;
static int voltage_count;
extern void dump_instr_state(FILE *);

/*----------------------------------------------------------------------*/

/* static power model results */
__thread power_result_type power;

int pow2(int x) {
  return((int)pow(2.0,(double)x));
}

double logfour(x)
     double x;
{
  if (x<=0) fprintf(stderr,"%e\n",x);
  return( (double) (log(x)/log(4.0)) );
}

/* safer pop count to validate the fast algorithm */
int pop_count_slow(qword_t bits)
{
  int count = 0; 
  qword_t tmpbits = bits; 
  while (tmpbits) { 
    if (tmpbits & 1) ++count; 
    tmpbits >>= 1; 
  } 
  return count; 
}

/* fast pop count */
int pop_count(qword_t bits)
{
#define T qword_t
#define WATTCH_ONES ((T)(-1)) 
#define TWO(k) ((T)1 << (k)) 
#define CYCL(k) (WATTCH_ONES/(1 + (TWO(TWO(k))))) 
#define BSUM(x,k) ((x)+=(x) >> TWO(k), (x) &= CYCL(k)) 
  qword_t x = bits; 
  x = (x & CYCL(0)) + ((x>>TWO(0)) & CYCL(0)); 
  x = (x & CYCL(1)) + ((x>>TWO(1)) & CYCL(1)); 
  BSUM(x,2); 
  BSUM(x,3); 
  BSUM(x,4); 
  BSUM(x,5); 
  return x; 
}

static double INV_RAND_MAX = 1.0/RAND_MAX;

double randu(void)
{
  return rand() * INV_RAND_MAX;
}

double randn(void)
{
  double x1, x2, w, y;
  
  do {
    x1 = 2.0 * randu() - 1.0;
    x2 = 2.0 * randu() - 1.0;
    w = x1 * x1 + x2 * x2;
  }
  while ( w >= 1.0 );

  w = sqrt( (-2.0 * log( w)) / w);
  y = x1 * w;

  return y;
}



int opcode_length = 8;
int inst_length = 32;

#if 0
extern int ruu_decode_width;
extern int ruu_issue_width;
extern int ruu_commit_width;
extern int RUU_size;
extern int LSQ_size;
extern int data_width;
extern int res_ialu;
extern int res_imult;
extern int res_fpalu;
extern int res_fpmult;
extern int res_memport;
#else
extern int ruu_decode_width;
extern int ruu_issue_width;
extern int ruu_commit_width;
extern int RUU_size;
extern int LSQ_size;
extern int data_width;
extern int res_ialu;
extern int res_imult;
extern int res_fpalu;
extern int res_fpmult;
extern int res_memport;
#endif


extern int int_iq_total;
extern int fp_iq_total;

int nvreg_width;
int npreg_width;

extern int bimod_config[];

extern __thread struct cache_t *cache_dl1;
extern __thread struct cache_t *cache_il1;
extern struct cache_t *cache_dl2; // jdonald: synchronization still needed for cache_dl2

extern __thread struct cache_t *dtlb;
extern __thread struct cache_t *itlb;

/* 2-level predictor config (<l1size> <l2size> <hist_size> <xor>) */
extern int twolev_config[];

/* combining predictor config (<meta_table_size> */
extern int comb_config[];

/* return address stack (RAS) size */
extern int ras_size;

/* BTB predictor config (<num_sets> <associativity>) */
extern int btb_config[];

double __thread global_clockcap;

extern __thread tick_t sim_cycle;
extern __thread md_addr_t current_pc;

static __thread double rename_power=0;
static __thread double bpred_power=0;
static __thread double window_power=0;
static __thread double lsq_power=0;
static __thread double regfile_power=0;
static __thread double icache_power=0;
static __thread double dcache_power=0;
static __thread double dcache2_power=0;
static __thread double alu_power=0;
static __thread double falu_power=0;
static __thread double resultbus_power=0;
static __thread double clock_power=0;

static __thread double rename_power_cc1=0;
static __thread double bpred_power_cc1=0;
static __thread double window_power_cc1=0;
static __thread double lsq_power_cc1=0;
static __thread double regfile_power_cc1=0;
static __thread double icache_power_cc1=0;
static __thread double dcache_power_cc1=0;
static __thread double dcache2_power_cc1=0;
static __thread double alu_power_cc1=0;
static __thread double resultbus_power_cc1=0;
static __thread double clock_power_cc1=0;

static __thread double rename_power_cc2=0;
static __thread double bpred_power_cc2=0;
static __thread double window_power_cc2=0;
static __thread double lsq_power_cc2=0;
static __thread double regfile_power_cc2=0;
static __thread double icache_power_cc2=0;
static __thread double dcache_power_cc2=0;
static __thread double dcache2_power_cc2=0;
static __thread double alu_power_cc2=0;
static __thread double resultbus_power_cc2=0;
static __thread double clock_power_cc2=0;

HOTSPOT_VAR __thread double rename_power_cc3=0;
HOTSPOT_VAR __thread double bpred_power_cc3=0;
HOTSPOT_VAR __thread double window_power_cc3=0;
HOTSPOT_VAR __thread double lsq_power_cc3=0;
HOTSPOT_VAR __thread double regfile_power_cc3=0;
HOTSPOT_VAR __thread double icache_power_cc3=0;
HOTSPOT_VAR __thread double dcache_power_cc3=0;
HOTSPOT_VAR __thread double dcache2_power_cc3=0;
HOTSPOT_VAR __thread double alu_power_cc3=0;
HOTSPOT_VAR __thread double resultbus_power_cc3=0;
HOTSPOT_VAR __thread double clock_power_cc3=0;

static __thread double total_cycle_power;
static __thread double total_cycle_power_cc1;
static __thread double total_cycle_power_cc2;
static __thread double total_cycle_power_cc3;

static __thread double last_single_total_cycle_power_cc1 = 0.0;
static __thread double last_single_total_cycle_power_cc2 = 0.0;
static __thread double last_single_total_cycle_power_cc3 = 0.0;
static __thread double current_total_cycle_power_cc1;
static __thread double current_total_cycle_power_cc2;
static __thread double current_total_cycle_power_cc3;

static __thread double min_cycle_power_cc1 = 10000.0;
static __thread double min_cycle_power_cc2 = 10000.0;
static __thread double min_cycle_power_cc3 = 10000.0;

static __thread double square_total_power_cycle_cc1;
static __thread double square_total_power_cycle_cc2;
static __thread double square_total_power_cycle_cc3;

static __thread double max_cycle_power_cc1 = 0.0;
static __thread double max_cycle_power_cc2 = 0.0;
static __thread double max_cycle_power_cc3 = 0.0;


static __thread double temp_max_cycle_power_cc3=0.0;
static __thread double temp_min_cycle_power_cc3=0.0;

extern __thread counter_t rename_access;
extern __thread counter_t bpred_access;
extern __thread counter_t window_access;
extern __thread counter_t lsq_access;
extern __thread counter_t regfile_access;
extern __thread counter_t icache_access;
extern __thread counter_t dcache_access;
extern __thread counter_t dcache2_access;
extern __thread counter_t alu_access;
extern __thread counter_t ialu_access;
extern __thread counter_t falu_access;
extern __thread counter_t resultbus_access;

extern __thread counter_t int_add_access;
extern __thread counter_t int_mult_access;
extern __thread counter_t int_div_access;
extern __thread counter_t fp_add_access;
extern __thread counter_t fp_mult_access;
extern __thread counter_t fp_div_access;

extern __thread counter_t regread_access;
extern __thread counter_t regwrite_access;

extern __thread counter_t decode_access;
extern __thread counter_t commit_access;

extern __thread counter_t window_selection_access;
extern __thread counter_t window_wakeup_access;
extern __thread counter_t window_preg_access;
extern __thread counter_t lsq_preg_access;
extern __thread counter_t lsq_wakeup_access;
extern __thread counter_t lsq_store_data_access;
extern __thread counter_t lsq_load_data_access;

extern __thread counter_t window_total_pop_count_cycle;
extern __thread counter_t window_num_pop_count_cycle;
extern __thread counter_t lsq_total_pop_count_cycle;
extern __thread counter_t lsq_num_pop_count_cycle;
extern __thread counter_t regfile_total_pop_count_cycle;
extern __thread counter_t regfile_num_pop_count_cycle;
extern __thread counter_t resultbus_total_pop_count_cycle;
extern __thread counter_t resultbus_num_pop_count_cycle;

static __thread counter_t total_rename_access=0;
static __thread counter_t total_bpred_access=0;
static __thread counter_t total_window_access=0;
static __thread counter_t total_lsq_access=0;
static __thread counter_t total_regfile_access=0;
static __thread counter_t total_icache_access=0;
static __thread counter_t total_dcache_access=0;
static __thread counter_t total_dcache2_access=0;
static __thread counter_t total_alu_access=0;
static __thread counter_t total_resultbus_access=0;

static __thread counter_t max_rename_access;
static __thread counter_t max_bpred_access;
static __thread counter_t max_window_access;
static __thread counter_t max_lsq_access;
static __thread counter_t max_regfile_access;
static __thread counter_t max_icache_access;
static __thread counter_t max_dcache_access;
static __thread counter_t max_dcache2_access;
static __thread counter_t max_alu_access;
static __thread counter_t max_resultbus_access;


static __thread double *current_history;
static __thread int current_length;
static __thread int current_position;

typedef struct {
  char name[255];
  int impulse_length;
  double *impulse_response;
  double current_supply_voltage;
  double last_supply_voltage;
  double sum_supply_voltage;
  double square_supply_voltage;
  double min_supply_voltage;
  double max_supply_voltage;
  int low_fault_current_interval_length;
  int high_fault_current_interval_length;
  counter_t low_fault_cycles;
  counter_t high_fault_cycles;
  counter_t low_fault_intervals;
  counter_t high_fault_intervals;
  struct stat_stat_t *voltage_dist;
  struct stat_stat_t *low_fault_dist;
  struct stat_stat_t *high_fault_dist;
} supply_t;

#define DEFAULT_IMPULSE_LENGTH 256
#define MAX_SUPPLY_COUNT 256
__thread supply_t *supply_arr;
extern __thread int supply_count;
extern char *supply_filename[MAX_SUPPLY_COUNT];
extern __thread FILE **supply_stream;

extern __thread int itrace_count;
extern __thread char *itrace_filename;
extern __thread FILE *itrace_stream;
__thread int start_emergency=0;


#define MAX_TRIGGER_COUNT 256
#define MAX_TRIGGER_HISTORY 8
__thread char *trigger_list[MAX_TRIGGER_COUNT];
__thread int trigger_count;

typedef struct {
  double low;
  double high;
  int delay;
  double noise;
  double noise_factor;
  int low_active, high_active;
  char name[16];
  counter_t low_trigger_cycles;
  counter_t low_trigger_events;
  counter_t high_trigger_cycles;
  counter_t high_trigger_events;
  int history_position;
  double trigger_history[MAX_TRIGGER_HISTORY];
} trigger_t;

__thread trigger_t *trigger_arr;

#define MAX_SENSOR_HISTORY 8
__thread int maxmin_power_cycles=0;


__thread int control;
__thread int control_cond;
__thread int control_within;
__thread int control_burst;
__thread int control_delta;
__thread double control_low;
__thread double control_high;

__thread int control_delay;
__thread int control_delay_position;
__thread double control_delay_history[MAX_SENSOR_HISTORY];
__thread int control_freeze;
__thread int control_fire;
__thread int control_scope;
__thread char *control_scope_type;

__thread int mpred_cycle;
__thread int il1fill_cycle;
__thread int il2fill_cycle;
__thread int dl1fill_cycle;
__thread int dl2fill_cycle;
__thread int none_cycle;

__thread int control_step;
__thread int control_step_size;
__thread int control_step_active;
__thread int control_step_trigger;
__thread int control_step_count;
__thread int control_step_timer;
__thread counter_t control_step_intervals;

__thread int busy_analysis;
__thread int busy_active;
__thread int busy_width;
__thread double busy_cutoff;
__thread double *busy_arr;
__thread double busy_interval_sum;
__thread double busy_width_recip;
__thread int busy_position;
__thread int busy_interval_length;
__thread int idle_interval_length;
__thread double total_busy_power;
__thread double total_idle_power;
__thread counter_t busy_interval_count;
__thread counter_t busy_current_interval_length;
__thread counter_t idle_current_interval_length;
__thread struct stat_stat_t *busy_length_dist;
__thread struct stat_stat_t *idle_length_dist;

__thread counter_t control_low_cycles;
__thread counter_t control_high_cycles;
__thread struct stat_stat_t *control_low_length_dist;
__thread struct stat_stat_t *control_high_length_dist;
__thread int control_low_length;
__thread int control_high_length;

__thread struct stat_stat_t *power_cycle_dist;

__thread double min_power = 30.32;
__thread double max_power = 143.16;
__thread double non_clock_power;

#define Vdd_Sim  1.0
__thread double dc_resistance;
__thread double power_dc_offset;
__thread double nominal_supply_voltage = 1.0;



__thread int power_spectra;
__thread int power_spectra_index;
__thread counter_t power_spectra_aggregate_count;
__thread counter_t power_spectra_extreme_count;
__thread int power_spectra_length; ;
__thread float *power_spectra_history;
__thread float *power_spectra_re;
__thread float *power_spectra_im;
__thread double *power_spectra_aggregate_value;
__thread double *power_spectra_extreme_value;
__thread double power_spectra_cutoff;

#if 0
extern char *sim_eio_fname[MD_NUM_THREADS];
#endif


void power_supply_init(void);
void power_supply_compute(void);
void power_supply_reg_stats(struct stat_sdb_t *sdb);

void power_spectra_init(void);
void power_spectra_compute(void);
void power_spectra_reg_stats(struct stat_sdb_t *sdb);

void power_wavelet_init(void);
void power_wavelet_compute(void);
void power_wavelet_reg_stats(struct stat_sdb_t *sdb);

__thread int power_wavelet;
__thread int power_wavelet_level;
__thread int power_wavelet_length;
__thread int power_wavelet_position;
__thread double *power_wavelet_history;
__thread double *wavelet_temp;

__thread double* power_wavelet_mag_sum;
__thread double* power_wavelet_mag_sq;
__thread counter_t* power_wavelet_mag_samples;

__thread counter_t* power_wavelet_stat_large;


struct pipe_rec {
  int depth;
  int width;
  int position;
  int pipelined;
  int controlled;
  int buffer;
  double energy;
  int *pipestage;
};

static __thread struct pipe_rec *il1_pipe;
static __thread struct pipe_rec *dl1_pipe;
static __thread struct pipe_rec *ul2_pipe;
static __thread struct pipe_rec *iadd_pipe;
static __thread struct pipe_rec *imult_pipe;
static __thread struct pipe_rec *idiv_pipe;
static __thread struct pipe_rec *fadd_pipe;
static __thread struct pipe_rec *fmult_pipe;
static __thread struct pipe_rec *fdiv_pipe;
static __thread struct pipe_rec *decode_pipe;
static __thread struct pipe_rec *commit_pipe;
static __thread struct pipe_rec *regread_pipe;
static __thread struct pipe_rec *regwrite_pipe;

void event_tracker_init(void);
void event_tracker_reg_stats(struct stat_sdb_t *);
void event_tracker_update(void);
int event_tracker_find(int, counter_t, int);


const char event_tracker_name[][255] = { // jdonald const
  "mpred",
  "long",
  "il1miss",
  "il1fill",
  "dl1miss",
  "dl1fill",
  "ul2miss",
  "ul2fill"
};

__thread int event_tracker_position;
__thread int event_tracker_length;
__thread int *current_event_count;
__thread counter_t *power_wavelet_event_total;
__thread int **event_tracker_arr;
__thread counter_t **power_wavelet_stat_large_event_near;
__thread counter_t **power_wavelet_stat_large_event_med;
__thread counter_t **power_wavelet_stat_large_event_far;


extern __thread struct stat_cond_t *cond_mpred_low;
extern __thread struct stat_cond_t *cond_il1fill_low;
extern __thread struct stat_cond_t *cond_il2fill_low;
extern __thread struct stat_cond_t *cond_dl1fill_low;
extern __thread struct stat_cond_t *cond_dl2fill_low;


extern __thread struct stat_within_t *within_mpred_low;
extern __thread struct stat_within_t *within_il1fill_low;
extern __thread struct stat_within_t *within_il2fill_low;
extern __thread struct stat_within_t *within_dl1fill_low;
extern __thread struct stat_within_t *within_dl2fill_low;
__thread struct stat_within_t *within_none_low;

__thread struct stat_delta_t *delta_mpred;
__thread struct stat_delta_t *delta_il1fill;
__thread struct stat_delta_t *delta_il2fill;
__thread struct stat_delta_t *delta_dl1fill;
__thread struct stat_delta_t *delta_dl2fill;


#define MAX_BURST 3
__thread struct stat_burst_t *burst_arr[MAX_BURST];

void clear_access_stats()
{
  rename_access=0;
  bpred_access=0;
  window_access=0;
  lsq_access=0;
  regfile_access=0;
  icache_access=0;
  dcache_access=0;
  dcache2_access=0;
  alu_access=0;
  ialu_access=0;
  falu_access=0;
  resultbus_access=0;

  window_preg_access=0;
  window_selection_access=0;
  window_wakeup_access=0;
  lsq_store_data_access=0;
  lsq_load_data_access=0;
  lsq_wakeup_access=0;
  lsq_preg_access=0;

  window_total_pop_count_cycle=0;
  window_num_pop_count_cycle=0;
  lsq_total_pop_count_cycle=0;
  lsq_num_pop_count_cycle=0;
  regfile_total_pop_count_cycle=0;
  regfile_num_pop_count_cycle=0;
  resultbus_total_pop_count_cycle=0;
  resultbus_num_pop_count_cycle=0;

  int_add_access = 0;
  int_mult_access = 0;
  int_div_access = 0;
  fp_add_access = 0;
  fp_mult_access = 0;
  fp_div_access = 0;

  regread_access = 0;
  regwrite_access = 0;

  decode_access = 0;
  commit_access = 0;
}

/* compute bitline activity factors which we use to scale bitline power 
   Here it is very important whether we assume 0's or 1's are
   responsible for dissipating power in pre-charged stuctures. (since
   most of the bits are 0's, we assume the design is power-efficient
   enough to allow 0's to _not_ discharge 
*/
double compute_af(counter_t num_pop_count_cycle,counter_t total_pop_count_cycle,int pop_width) {
  double avg_pop_count;
  double af,af_b;

  if(num_pop_count_cycle)
    avg_pop_count = (double)total_pop_count_cycle / (double)num_pop_count_cycle;
  else
    avg_pop_count = 0;

  af = avg_pop_count / (double)pop_width;
  
  af_b = 1.0 - af;

  /*  printf("af == %f%%, af_b == %f%%, total_pop == %d, num_pop == %d\n",100*af,100*af_b,total_pop_count_cycle,num_pop_count_cycle); */

  return(af_b);
}

/* compute power statistics on each cycle, for each conditional clocking style.  Obviously
most of the speed penalty comes here, so if you don't want per-cycle power estimates
you could post-process 

See README.wattch for details on the various clock gating styles.

*/
__thread double reg_power, il1_power, dl1_power, ul2_power, fu_power, clk_power;

__thread int test_timer = 0;

void update_power_stats()
{
  // double window_af_b, lsq_af_b, regfile_af_b, resultbus_af_b; // jdonald unused

#ifdef DYNAMIC_AF
  window_af_b = compute_af(window_num_pop_count_cycle,window_total_pop_count_cycle,data_width);
  lsq_af_b = compute_af(lsq_num_pop_count_cycle,lsq_total_pop_count_cycle,data_width);
  regfile_af_b = compute_af(regfile_num_pop_count_cycle,regfile_total_pop_count_cycle,data_width);
  resultbus_af_b = compute_af(resultbus_num_pop_count_cycle,resultbus_total_pop_count_cycle,data_width);
#endif

  rename_power+=power.rename_power;
  bpred_power+=power.bpred_power;
  window_power+=power.window_power;
  lsq_power+=power.lsq_power;
  regfile_power+=power.regfile_power;
  icache_power+=power.icache_power+power.itlb;
  dcache_power+=power.dcache_power+power.dtlb;
  dcache2_power+=power.dcache2_power;
  alu_power+=power.ialu_power + power.falu_power;
  falu_power+=power.falu_power;
  resultbus_power+=power.resultbus;
  clock_power+=power.clock_power;

  total_rename_access+=rename_access;
  total_bpred_access+=bpred_access;
  total_window_access+=window_access;
  total_lsq_access+=lsq_access;
  total_regfile_access+=regfile_access;
  total_icache_access+=icache_access;
  total_dcache_access+=dcache_access;
  total_dcache2_access+=dcache2_access;
  total_alu_access+=alu_access;
  total_resultbus_access+=resultbus_access;

  max_rename_access=MAX(rename_access,max_rename_access);
  max_bpred_access=MAX(bpred_access,max_bpred_access);
  max_window_access=MAX(window_access,max_window_access);
  max_lsq_access=MAX(lsq_access,max_lsq_access);
  max_regfile_access=MAX(regfile_access,max_regfile_access);
  max_icache_access=MAX(icache_access,max_icache_access);
  max_dcache_access=MAX(dcache_access,max_dcache_access);
  max_dcache2_access=MAX(dcache2_access,max_dcache2_access);
  max_alu_access=MAX(alu_access,max_alu_access);
  max_resultbus_access=MAX(resultbus_access,max_resultbus_access);
      
  if(rename_access) {
    rename_power_cc1+=power.rename_power;
    rename_power_cc2+=((double)rename_access/(double)ruu_decode_width)*power.rename_power;
    rename_power_cc3+=((double)rename_access/(double)ruu_decode_width)*power.rename_power;
  }
  else 
    rename_power_cc3+=turnoff_factor*power.rename_power;


  if(bpred_access) {
    if(bpred_access <= 2)
      bpred_power_cc1+=power.bpred_power;
    else
      bpred_power_cc1+=((double)bpred_access/2.0) * power.bpred_power;
    bpred_power_cc2+=((double)bpred_access/2.0) * power.bpred_power;
    bpred_power_cc3+=((double)bpred_access/2.0) * power.bpred_power;
  }
  else
    bpred_power_cc3+=turnoff_factor*power.bpred_power;

#ifdef STATIC_AF
  if(window_preg_access) {
    if(window_preg_access <= 3*ruu_issue_width)
      window_power_cc1+=power.rs_power;
    else 
      window_power_cc1+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*power.rs_power;
    window_power_cc2+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*power.rs_power;
    window_power_cc3+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*power.rs_power;
  }
  else
    window_power_cc3+=turnoff_factor*power.rs_power;

#elif defined(DYNAMIC_AF)
  if(window_preg_access) {
    if(window_preg_access <= 3*ruu_issue_width)
      window_power_cc1+=power.rs_power_nobit + window_af_b*power.rs_bitline;
    else
      window_power_cc1+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*(power.rs_power_nobit + window_af_b*power.rs_bitline);
    window_power_cc2+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*(power.rs_power_nobit + window_af_b*power.rs_bitline);
    window_power_cc3+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*(power.rs_power_nobit + window_af_b*power.rs_bitline);
  }
  else
    window_power_cc3+=turnoff_factor*power.rs_power;
#else
  panic("no AF-style defined\n");
#endif
  
  if(window_selection_access) {
    if(window_selection_access <= ruu_issue_width)
      window_power_cc1+=power.selection;
    else
      window_power_cc1+=((double)window_selection_access/((double)ruu_issue_width))*power.selection;
    window_power_cc2+=((double)window_selection_access/((double)ruu_issue_width))*power.selection;
    window_power_cc3+=((double)window_selection_access/((double)ruu_issue_width))*power.selection;
  }
  else
    window_power_cc3+=turnoff_factor*power.selection;

  if(window_wakeup_access) {
    if(window_wakeup_access <= ruu_issue_width)
      window_power_cc1+=power.wakeup_power;
    else
      window_power_cc1+=((double)window_wakeup_access/((double)ruu_issue_width))*power.wakeup_power;
    window_power_cc2+=((double)window_wakeup_access/((double)ruu_issue_width))*power.wakeup_power;
    window_power_cc3+=((double)window_wakeup_access/((double)ruu_issue_width))*power.wakeup_power;
  }
  else
    window_power_cc3+=turnoff_factor*power.wakeup_power;

  if(lsq_wakeup_access) {
    if(lsq_wakeup_access <= res_memport)
      lsq_power_cc1+=power.lsq_wakeup_power;
    else
      lsq_power_cc1+=((double)lsq_wakeup_access/((double)res_memport))*power.lsq_wakeup_power;
    lsq_power_cc2+=((double)lsq_wakeup_access/((double)res_memport))*power.lsq_wakeup_power;
    lsq_power_cc3+=((double)lsq_wakeup_access/((double)res_memport))*power.lsq_wakeup_power;
  }
  else
    lsq_power_cc3+=turnoff_factor*power.lsq_wakeup_power;


#ifdef STATIC_AF
  if(lsq_preg_access) {
    if(lsq_preg_access <= res_memport)
      lsq_power_cc1+=power.lsq_rs_power;
    else
      lsq_power_cc1+=((double)lsq_preg_access/((double)res_memport))*power.lsq_rs_power;
    lsq_power_cc2+=((double)lsq_preg_access/((double)res_memport))*power.lsq_rs_power;
    lsq_power_cc3+=((double)lsq_preg_access/((double)res_memport))*power.lsq_rs_power;
  }
  else
    lsq_power_cc3+=turnoff_factor*power.lsq_rs_power;
#else
  if(lsq_preg_access) {
    if(lsq_preg_access <= res_memport)
      lsq_power_cc1+=power.lsq_rs_power_nobit + lsq_af_b*power.lsq_rs_bitline;
    else
      lsq_power_cc1+=((double)lsq_preg_access/((double)res_memport))*(power.lsq_rs_power_nobit + lsq_af_b*power.lsq_rs_bitline);
    lsq_power_cc2+=((double)lsq_preg_access/((double)res_memport))*(power.lsq_rs_power_nobit + lsq_af_b*power.lsq_rs_bitline);
    lsq_power_cc3+=((double)lsq_preg_access/((double)res_memport))*(power.lsq_rs_power_nobit + lsq_af_b*power.lsq_rs_bitline);
  }
  else
    lsq_power_cc3+=turnoff_factor*power.lsq_rs_power;
#endif

#ifdef STATIC_AF
  if(regfile_access) {
    if(regfile_access <= (3.0*ruu_commit_width))
      regfile_power_cc1+=power.regfile_power;
    else
      regfile_power_cc1+=((double)regfile_access/(3.0*(double)ruu_commit_width))*power.regfile_power;
    regfile_power_cc2+=((double)regfile_access/(3.0*(double)ruu_commit_width))*power.regfile_power;
    regfile_power_cc3+=((double)regfile_access/(3.0*(double)ruu_commit_width))*power.regfile_power;
  }
  else
    regfile_power_cc3+=turnoff_factor*power.regfile_power;

  pipe_rec_update(regread_pipe);
  pipe_rec_update(regwrite_pipe);
  pipe_rec_activate(regread_pipe, regread_access);
  pipe_rec_activate(regwrite_pipe, regwrite_access);
//  printf("calling for regread_pipe and regwrite_pipe\n");
  reg_power = pipe_rec_calc(regread_pipe) + pipe_rec_calc(regwrite_pipe);
  regfile_power_cc3+= reg_power;
#else
  if(regfile_access) {
    if(regfile_access <= (3.0*ruu_commit_width))
      regfile_power_cc1+=power.regfile_power_nobit + regfile_af_b*power.regfile_bitline;
    else
      regfile_power_cc1+=((double)regfile_access/(3.0*(double)ruu_commit_width))*(power.regfile_power_nobit + regfile_af_b*power.regfile_bitline);
    regfile_power_cc2+=((double)regfile_access/(3.0*(double)ruu_commit_width))*(power.regfile_power_nobit + regfile_af_b*power.regfile_bitline);
    regfile_power_cc3+=((double)regfile_access/(3.0*(double)ruu_commit_width))*(power.regfile_power_nobit + regfile_af_b*power.regfile_bitline);
  }
  else
    regfile_power_cc3+=turnoff_factor*power.regfile_power;

  pipe_rec_update(regread_pipe);
  pipe_rec_update(regwrite_pipe);
  pipe_rec_activate(regread_pipe, regread_access);
  pipe_rec_activate(regwrite_pipe, regwrite_access);
  reg_power = pipe_rec_calc(regread_pipe) + pipe_rec_calc(regwrite_pipe);
  regfile_power_cc3+= reg_power;
#endif

  if(icache_access) {
    /* don't scale icache because we assume 1 line is fetched, unless fetch stalls */
    icache_power_cc1+=power.icache_power+power.itlb;
    icache_power_cc2+=power.icache_power+power.itlb;
    icache_power_cc3+=power.icache_power+power.itlb;
  }
  else
    icache_power_cc3+=turnoff_factor*(power.icache_power+power.itlb);

  pipe_rec_update(il1_pipe);
  pipe_rec_activate(il1_pipe, (icache_access > 2) ? 2 : icache_access);
//  printf("calling for il1_pipe\n");
  il1_power = pipe_rec_calc(il1_pipe);
  icache_power_cc3+= il1_power;


  if(dcache_access) {
    if(dcache_access <= res_memport)
      dcache_power_cc1+=power.dcache_power+power.dtlb;
    else
      dcache_power_cc1+=((double)dcache_access/(double)res_memport)*(power.dcache_power + power.dtlb);
    dcache_power_cc2+=((double)dcache_access/(double)res_memport)*(power.dcache_power + power.dtlb);
    dcache_power_cc3+=((double)dcache_access/(double)res_memport)*(power.dcache_power + power.dtlb);
  }
  else
    dcache_power_cc3+=turnoff_factor*(power.dcache_power+power.dtlb);
  
  pipe_rec_update(dl1_pipe);
  pipe_rec_activate(dl1_pipe, dcache_access);
//  printf("calling for dl1_pipe\n");

  dl1_power = pipe_rec_calc(dl1_pipe);
  dcache_power_cc3+= dl1_power;


  if(dcache2_access) {
    if(dcache2_access <= res_memport)
      dcache2_power_cc1+=power.dcache2_power;
    else
      dcache2_power_cc1+=((double)dcache2_access/(double)res_memport)*power.dcache2_power;
    dcache2_power_cc2+=((double)dcache2_access/(double)res_memport)*power.dcache2_power;
    dcache2_power_cc3+=((double)dcache2_access/(double)res_memport)*power.dcache2_power;
  }
  else
    dcache2_power_cc3+=turnoff_factor*power.dcache2_power;

  pipe_rec_update(ul2_pipe);
  pipe_rec_activate(ul2_pipe, (dcache2_access) ? 1 : 0);
//  printf("calling for ul2_pipe\n");
  ul2_power = pipe_rec_calc(ul2_pipe);
  dcache2_power_cc3+= ul2_power;

  if(alu_access) {
    if(ialu_access)
      alu_power_cc1+=power.ialu_power;
    else
      alu_power_cc3+=turnoff_factor*power.ialu_power;
    if(falu_access)
      alu_power_cc1+=power.falu_power;
    else
      alu_power_cc3+=turnoff_factor*power.falu_power;

    alu_power_cc2+=((double)ialu_access/(double)res_ialu)*power.ialu_power +
      ((double)falu_access/(double)res_fpalu)*power.falu_power;

    alu_power_cc3+=((double)ialu_access/(double)res_ialu)*power.ialu_power +
      ((double)falu_access/(double)res_fpalu)*power.falu_power;
  }
  else
    alu_power_cc3+=turnoff_factor*(power.ialu_power + power.falu_power);

    pipe_rec_update(iadd_pipe);
    pipe_rec_update(imult_pipe);
    pipe_rec_update(idiv_pipe);
    pipe_rec_update(fadd_pipe);
    pipe_rec_update(fmult_pipe);
    pipe_rec_update(fdiv_pipe);

    pipe_rec_activate(iadd_pipe, int_add_access);
    pipe_rec_activate(imult_pipe, int_mult_access);
    pipe_rec_activate(idiv_pipe, int_div_access);
    pipe_rec_activate(fadd_pipe, fp_add_access);
    pipe_rec_activate(fmult_pipe, fp_mult_access);
    pipe_rec_activate(fdiv_pipe, fp_div_access);
    
//    printf("calling for iadd_pipe\n");

    fu_power = pipe_rec_calc(iadd_pipe) + pipe_rec_calc(imult_pipe) + pipe_rec_calc(idiv_pipe) +
      pipe_rec_calc(fadd_pipe) + pipe_rec_calc(fmult_pipe) + pipe_rec_calc(fdiv_pipe);

alu_power_cc3 += fu_power;


#ifdef STATIC_AF
 if(resultbus_access) {
   assert(ruu_issue_width != 0);
   if(resultbus_access <= ruu_issue_width) {
     resultbus_power_cc1+=power.resultbus;
   }
   else 
     resultbus_power_cc1+=((double)resultbus_access/(double)ruu_issue_width)*power.resultbus;
    
   resultbus_power_cc2+=((double)resultbus_access/(double)ruu_issue_width)*power.resultbus;
   resultbus_power_cc3+=((double)resultbus_access/(double)ruu_issue_width)*power.resultbus;
 }
 else
   resultbus_power_cc3+=turnoff_factor*power.resultbus;
#else
 if(resultbus_access) {
   assert(ruu_issue_width != 0);
   if(resultbus_access <= ruu_issue_width) {
     resultbus_power_cc1+=resultbus_af_b*power.resultbus;
   }
   else {
     resultbus_power_cc1+=((double)resultbus_access/(double)ruu_issue_width)*resultbus_af_b*power.resultbus;
   }
   resultbus_power_cc2+=((double)resultbus_access/(double)ruu_issue_width)*resultbus_af_b*power.resultbus;
   resultbus_power_cc3+=((double)resultbus_access/(double)ruu_issue_width)*resultbus_af_b*power.resultbus;
 }
 else
   resultbus_power_cc3+=turnoff_factor*power.resultbus;
#endif


 pipe_rec_update(decode_pipe);
 pipe_rec_update(commit_pipe);
 pipe_rec_activate(decode_pipe, (decode_access) ? 1 : 0);
 pipe_rec_activate(commit_pipe, (commit_access) ? 1 : 0);
// printf("calling for decode_pipe\n");

 rename_power_cc3 += pipe_rec_calc(decode_pipe) + pipe_rec_calc(commit_pipe);


 total_cycle_power = rename_power + bpred_power + window_power + 
   lsq_power + regfile_power + icache_power + dcache_power +
   alu_power + resultbus_power;
 
 total_cycle_power_cc1 = rename_power_cc1 + bpred_power_cc1 + 
   window_power_cc1 + lsq_power_cc1 + regfile_power_cc1 + 
   icache_power_cc1 + dcache_power_cc1 + alu_power_cc1 + 
   resultbus_power_cc1;

 total_cycle_power_cc2 = rename_power_cc2 + bpred_power_cc2 + 
   window_power_cc2 + lsq_power_cc2 + regfile_power_cc2 + 
   icache_power_cc2 + dcache_power_cc2 + alu_power_cc2 + 
   resultbus_power_cc2;
 
#ifndef PIPE_POWER
 total_cycle_power_cc3 = rename_power_cc3 + bpred_power_cc3 + 
   window_power_cc3 + lsq_power_cc3 + regfile_power_cc3 + 
   icache_power_cc3 + dcache_power_cc3 + alu_power_cc3 + 
   resultbus_power_cc3;
#else
 total_cycle_power_cc3 += pipe_rec_calc(regread_pipe) + 
   pipe_rec_calc(regwrite_pipe) +
   pipe_rec_calc(iadd_pipe) + pipe_rec_calc(imult_pipe) +
   pipe_rec_calc(idiv_pipe) + pipe_rec_calc(fadd_pipe) +
   pipe_rec_calc(fmult_pipe) + pipe_rec_calc(fdiv_pipe) +
   pipe_rec_calc(il1_pipe) + pipe_rec_calc(dl1_pipe) +
   pipe_rec_calc(ul2_pipe) + 
   pipe_rec_calc(decode_pipe) + pipe_rec_calc(commit_pipe);
#endif


 clock_power_cc1+=power.clock_power *
   ((total_cycle_power_cc1 - last_single_total_cycle_power_cc1)/non_clock_power);
  clock_power_cc2+=power.clock_power *
    ((total_cycle_power_cc2 - last_single_total_cycle_power_cc2)/non_clock_power);
#ifndef PIPE_POWER
  clk_power = power.clock_power *
    ((total_cycle_power_cc3 - last_single_total_cycle_power_cc3)/non_clock_power);
  clock_power_cc3 += clk_power;
#endif

  total_cycle_power_cc1 += clock_power_cc1;
  total_cycle_power_cc2 += clock_power_cc2;
#ifndef PIPE_POWER
  total_cycle_power_cc3 += clock_power_cc3;
#endif

  current_total_cycle_power_cc1 = total_cycle_power_cc1
    -last_single_total_cycle_power_cc1;
  current_total_cycle_power_cc2 = total_cycle_power_cc2
    -last_single_total_cycle_power_cc2;
  current_total_cycle_power_cc3 = total_cycle_power_cc3
    -last_single_total_cycle_power_cc3;

  min_cycle_power_cc1 = MIN(min_cycle_power_cc1,current_total_cycle_power_cc1);
  min_cycle_power_cc2 = MIN(min_cycle_power_cc2,current_total_cycle_power_cc2);

#if 1
  if(maxmin_power_cycles < 5){
//    min_cycle_power_cc3 = MIN(min_cycle_power_cc3,current_total_cycle_power_cc3);
    temp_min_cycle_power_cc3=current_total_cycle_power_cc3+temp_min_cycle_power_cc3;
 //   max_cycle_power_cc3 = MAX(max_cycle_power_cc3,current_total_cycle_power_cc3);
    temp_max_cycle_power_cc3=current_total_cycle_power_cc3+temp_max_cycle_power_cc3;
    maxmin_power_cycles++;
  }
  else{
    min_cycle_power_cc3=MIN(min_cycle_power_cc3,(temp_min_cycle_power_cc3/5));
    max_cycle_power_cc3 = MAX(max_cycle_power_cc3,(temp_max_cycle_power_cc3/5));
    maxmin_power_cycles=0;
    temp_max_cycle_power_cc3=0;
    temp_min_cycle_power_cc3=0;
  }
#else
    max_cycle_power_cc3 = MAX(max_cycle_power_cc3,current_total_cycle_power_cc3);
    min_cycle_power_cc3 = MIN(min_cycle_power_cc3,current_total_cycle_power_cc3);

#endif

  max_cycle_power_cc1 = MAX(max_cycle_power_cc1,current_total_cycle_power_cc1);
  max_cycle_power_cc2 = MAX(max_cycle_power_cc2,current_total_cycle_power_cc2);

  square_total_power_cycle_cc1 += current_total_cycle_power_cc1 * current_total_cycle_power_cc1;
  square_total_power_cycle_cc2 += current_total_cycle_power_cc2 * current_total_cycle_power_cc2;
  square_total_power_cycle_cc3 += current_total_cycle_power_cc3 * current_total_cycle_power_cc3;

  last_single_total_cycle_power_cc1 = total_cycle_power_cc1;
  last_single_total_cycle_power_cc2 = total_cycle_power_cc2;
  last_single_total_cycle_power_cc3 = total_cycle_power_cc3;

#if 0
  fprintf(stderr, "%lld\t%f\t%f\n", sim_cycle, 
	  current_total_cycle_power_cc3, 
	  supply_arr[0].current_supply_voltage);
#endif

#if 0
  fprintf(stderr, "%lld\t%f [%lld,%lld,%lld,%lld,%lld,%lld] (%d,%d) ", 
	  sim_cycle, current_total_cycle_power_cc3,
	  int_add_access, int_mult_access, int_div_access,
	  fp_add_access, fp_mult_access, fp_div_access,
	  int_iq_total, fp_iq_total);
  fprintf(stderr, "[%lld,%lld,%lld] [%lld] [%lld,%lld]\n",
	  icache_access, decode_access, regread_access,	
	  dcache_access, regwrite_access, commit_access);
#endif

#if 0
  if (current_total_cycle_power_cc3 > 50) {
    fprintf(stderr, "*** %lld ***\n", sim_cycle);
    fprintf(stderr, "current_total_cycle_power_cc3 = %4.2f\n", current_total_cycle_power_cc3);
    fprintf(stderr, "regread = %4.2f (%4.2f) regwrite = %4.2f (%4.2f)\n", 
	    pipe_rec_calc(regread_pipe), pipe_rec_min(regread_pipe), 
	    pipe_rec_calc(regwrite_pipe), pipe_rec_min(regwrite_pipe));
    fprintf(stderr, "iadd = %4.2f (%4.2f) imult = %4.2f (%4.2f)\n", 
	    pipe_rec_calc(iadd_pipe), pipe_rec_min(iadd_pipe), 
	    pipe_rec_calc(imult_pipe), pipe_rec_min(imult_pipe));
    fprintf(stderr, "idiv = %4.2f (%4.2f) fadd = %4.2f (%4.2f)\n", 
	    pipe_rec_calc(idiv_pipe), pipe_rec_min(idiv_pipe), 
	    pipe_rec_calc(fadd_pipe), pipe_rec_min(fadd_pipe));
    fprintf(stderr, "fmult = %4.2f (%4.2f) fdiv = %4.2f (%4.2f)\n", 
	    pipe_rec_calc(fmult_pipe), pipe_rec_min(fmult_pipe), 
	    pipe_rec_calc(fdiv_pipe), pipe_rec_min(fdiv_pipe));
    fprintf(stderr, "il1 = %4.2f (%4.2f) dl1 = %4.2f (%4.2f)\n", 
	    pipe_rec_calc(il1_pipe), pipe_rec_min(il1_pipe), 
	    pipe_rec_calc(dl1_pipe), pipe_rec_min(dl1_pipe));
    fprintf(stderr, "ul2 = %4.2f (%4.2f)\n",
	    pipe_rec_calc(ul2_pipe), pipe_rec_min(ul2_pipe));
    fprintf(stderr, "decode = %4.2f (%4.2f) commit = %4.2f (%4.2f)\n", 
	    pipe_rec_calc(decode_pipe), pipe_rec_min(decode_pipe), 
	    pipe_rec_calc(commit_pipe), pipe_rec_min(commit_pipe));
  }
#endif


#if 0
  fprintf(stderr, "%8lld\t%4.2f\t%5.3f (%2s)(%2s)  %2d %2d %2d %2d %2d(%d)\n", sim_cycle, current_total_cycle_power_cc3,
	  (supply_count) ? supply_arr[0].current_supply_voltage : 1.0,
	  (control_execute_stall) ?  "LO" : " ",
	  (control_execute_fire) ? "HI" : " ",
	  (icache_access) ? 1 : 0,
	  (decode_access) ? 1 : 0,
	  (int)(int_add_access + int_mult_access + int_div_access +
		fp_add_access + fp_mult_access + fp_div_access + dcache_access),
	  (int)(regwrite_access),
	  (commit_access) ? 1 : 0, (int)(commit_access));

  if (control_cond) {
    if (mpred_cycle) fprintf(stderr, "MPRED\n");
    if (il1fill_cycle) fprintf(stderr, "IL1FILL\n");
    if (il2fill_cycle) fprintf(stderr, "IL2FILL\n");
    if (dl1fill_cycle) fprintf(stderr, "DL1FILL\n");
    if (dl2fill_cycle) fprintf(stderr, "DL2FILL\n");
  }
#endif


  //compute_max_min_current();

  power_supply_compute();

  if (power_spectra)
    power_spectra_compute();

  if (power_wavelet)
    power_wavelet_compute();

#if 0  
  if (control_cond) {

    stat_add_cond_sample(cond_mpred_low, mpred_cycle, control_execute_stall);
    stat_add_cond_sample(cond_il1fill_low, il1fill_cycle, control_execute_stall);
    stat_add_cond_sample(cond_il2fill_low, il2fill_cycle, control_execute_stall);
    stat_add_cond_sample(cond_dl1fill_low, dl1fill_cycle, control_execute_stall);
    stat_add_cond_sample(cond_dl2fill_low, dl2fill_cycle, control_execute_stall);

  }
  if (control_within) {
    int any_cycle = mpred_cycle | il1fill_cycle | il2fill_cycle |
      dl1fill_cycle | dl2fill_cycle;
    stat_add_within_sample(within_mpred_low, mpred_cycle, control_execute_stall);
    stat_add_within_sample(within_il1fill_low, il1fill_cycle, control_execute_stall);
    stat_add_within_sample(within_il2fill_low, il2fill_cycle, control_execute_stall);
    stat_add_within_sample(within_dl1fill_low, dl1fill_cycle, control_execute_stall);
    stat_add_within_sample(within_dl2fill_low, dl2fill_cycle, control_execute_stall);
    stat_add_within_sample(within_none_low, any_cycle, control_execute_stall);
  }

  if (control_burst) {
    int burst_index;
    
    fprintf(stderr, "%lld\t%5.2f (%2s)\t", sim_cycle, 
	    current_total_cycle_power_cc3,
	    ((control_execute_stall) ? "LO" : (control_execute_fire) ? "HI" : " "));
    for(burst_index = 0; burst_index < MAX_BURST; burst_index++)
      stat_add_burst_sample(burst_arr[burst_index], 
			    (control_execute_stall | control_execute_fire),
			    current_total_cycle_power_cc3);
    fprintf(stderr, "\n");
  }
#endif

  if (busy_analysis) {
    busy_interval_sum -= busy_arr[busy_position] * busy_width_recip;
    busy_arr[busy_position] = current_total_cycle_power_cc3;
    busy_interval_sum += current_total_cycle_power_cc3 * busy_width_recip;

    if (busy_position >= (busy_width-1))
      busy_position = 0;
    else
      busy_position++;

    if (busy_interval_sum > busy_cutoff) {
      if (!busy_active) {
	stat_add_sample(idle_length_dist, idle_interval_length);
      }

      busy_active = TRUE;
      idle_interval_length = 0;
      busy_interval_length++;
      busy_current_interval_length++;
      total_busy_power += current_total_cycle_power_cc3;
    }
    else {
      if (busy_active) {
	stat_add_sample(busy_length_dist, busy_interval_length);
	busy_interval_count++;
      }
      
      busy_active = FALSE;
      busy_interval_length = 0;
      idle_interval_length++;
      idle_current_interval_length++;
      total_idle_power += current_total_cycle_power_cc3;
    }
    
  }
  
  if (control_delta) {
    stat_add_delta_sample(delta_mpred, mpred_cycle, current_total_cycle_power_cc3);
    stat_add_delta_sample(delta_il1fill, il1fill_cycle, current_total_cycle_power_cc3);
    stat_add_delta_sample(delta_il2fill, il2fill_cycle, current_total_cycle_power_cc3);
    stat_add_delta_sample(delta_dl1fill, dl1fill_cycle, current_total_cycle_power_cc3);
    stat_add_delta_sample(delta_dl2fill, dl2fill_cycle, current_total_cycle_power_cc3);
  }

  mpred_cycle = 0;
  il1fill_cycle = 0;
  il2fill_cycle = 0;
  dl1fill_cycle = 0;
  dl2fill_cycle = 0;
}

void
power_reg_stats(struct stat_sdb_t *sdb)	/* stats database */
{
  stat_reg_double(sdb, "rename_power", "total power usage of rename unit", &rename_power, 0, NULL);

  stat_reg_double(sdb, "bpred_power", "total power usage of bpred unit", &bpred_power, 0, NULL);

  stat_reg_double(sdb, "window_power", "total power usage of instruction window", &window_power, 0, NULL);

  stat_reg_double(sdb, "lsq_power", "total power usage of load/store queue", &lsq_power, 0, NULL);

  stat_reg_double(sdb, "regfile_power", "total power usage of arch. regfile", &regfile_power, 0, NULL);

  stat_reg_double(sdb, "icache_power", "total power usage of icache", &icache_power, 0, NULL);

  stat_reg_double(sdb, "dcache_power", "total power usage of dcache", &dcache_power, 0, NULL);

  stat_reg_double(sdb, "dcache2_power", "total power usage of dcache2", &dcache2_power, 0, NULL);

  stat_reg_double(sdb, "alu_power", "total power usage of alu", &alu_power, 0, NULL);

  stat_reg_double(sdb, "falu_power", "total power usage of falu", &falu_power, 0, NULL);

  stat_reg_double(sdb, "resultbus_power", "total power usage of resultbus", &resultbus_power, 0, NULL);

  stat_reg_double(sdb, "clock_power", "total power usage of clock", &clock_power, 0, NULL);

  stat_reg_formula(sdb, "avg_rename_power", "avg power usage of rename unit", "rename_power/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_bpred_power", "avg power usage of bpred unit", "bpred_power/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_window_power", "avg power usage of instruction window", "window_power/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_lsq_power", "avg power usage of lsq", "lsq_power/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_regfile_power", "avg power usage of arch. regfile", "regfile_power/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_icache_power", "avg power usage of icache", "icache_power/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_dcache_power", "avg power usage of dcache", "dcache_power/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_dcache2_power", "avg power usage of dcache2", "dcache2_power/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_alu_power", "avg power usage of alu", "alu_power/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_falu_power", "avg power usage of falu", "falu_power/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_resultbus_power", "avg power usage of resultbus", "resultbus_power/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_clock_power", "avg power usage of clock", "clock_power/sim_cycle",  NULL);

  stat_reg_formula(sdb, "fetch_stage_power", "total power usage of fetch stage", "icache_power + bpred_power", NULL);

  stat_reg_formula(sdb, "dispatch_stage_power", "total power usage of dispatch stage", "rename_power", NULL);

  stat_reg_formula(sdb, "issue_stage_power", "total power usage of issue stage", "resultbus_power + alu_power + dcache_power + dcache2_power + window_power + lsq_power", NULL);

  stat_reg_formula(sdb, "avg_fetch_power", "average power of fetch unit per cycle", "(icache_power + bpred_power)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "avg_dispatch_power", "average power of dispatch unit per cycle", "(rename_power)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "avg_issue_power", "average power of issue unit per cycle", "(resultbus_power + alu_power + dcache_power + dcache2_power + window_power + lsq_power)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "total_power", "total power per cycle","(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power  + resultbus_power + clock_power + alu_power + dcache_power + dcache2_power)", NULL);

  stat_reg_formula(sdb, "avg_total_power_cycle", "average total power per cycle","(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power + dcache2_power)/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_total_power_cycle_nofp_nod2", "average total power per cycle","(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power - falu_power )/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_total_power_insn", "average total power per insn","(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power + dcache2_power)/sim_total_insn", NULL);

  stat_reg_formula(sdb, "avg_total_power_insn_nofp_nod2", "average total power per insn","(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power - falu_power )/sim_total_insn", NULL);

  stat_reg_double(sdb, "rename_power_cc1", "total power usage of rename unit_cc1", &rename_power_cc1, 0, NULL);

  stat_reg_double(sdb, "bpred_power_cc1", "total power usage of bpred unit_cc1", &bpred_power_cc1, 0, NULL);

  stat_reg_double(sdb, "window_power_cc1", "total power usage of instruction window_cc1", &window_power_cc1, 0, NULL);

  stat_reg_double(sdb, "lsq_power_cc1", "total power usage of lsq_cc1", &lsq_power_cc1, 0, NULL);

  stat_reg_double(sdb, "regfile_power_cc1", "total power usage of arch. regfile_cc1", &regfile_power_cc1, 0, NULL);

  stat_reg_double(sdb, "icache_power_cc1", "total power usage of icache_cc1", &icache_power_cc1, 0, NULL);

  stat_reg_double(sdb, "dcache_power_cc1", "total power usage of dcache_cc1", &dcache_power_cc1, 0, NULL);

  stat_reg_double(sdb, "dcache2_power_cc1", "total power usage of dcache2_cc1", &dcache2_power_cc1, 0, NULL);

  stat_reg_double(sdb, "alu_power_cc1", "total power usage of alu_cc1", &alu_power_cc1, 0, NULL);

  stat_reg_double(sdb, "resultbus_power_cc1", "total power usage of resultbus_cc1", &resultbus_power_cc1, 0, NULL);

  stat_reg_double(sdb, "clock_power_cc1", "total power usage of clock_cc1", &clock_power_cc1, 0, NULL);

  stat_reg_formula(sdb, "avg_rename_power_cc1", "avg power usage of rename unit_cc1", "rename_power_cc1/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_bpred_power_cc1", "avg power usage of bpred unit_cc1", "bpred_power_cc1/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_window_power_cc1", "avg power usage of instruction window_cc1", "window_power_cc1/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_lsq_power_cc1", "avg power usage of lsq_cc1", "lsq_power_cc1/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_regfile_power_cc1", "avg power usage of arch. regfile_cc1", "regfile_power_cc1/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_icache_power_cc1", "avg power usage of icache_cc1", "icache_power_cc1/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_dcache_power_cc1", "avg power usage of dcache_cc1", "dcache_power_cc1/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_dcache2_power_cc1", "avg power usage of dcache2_cc1", "dcache2_power_cc1/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_alu_power_cc1", "avg power usage of alu_cc1", "alu_power_cc1/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_resultbus_power_cc1", "avg power usage of resultbus_cc1", "resultbus_power_cc1/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_clock_power_cc1", "avg power usage of clock_cc1", "clock_power_cc1/sim_cycle",  NULL);

  stat_reg_formula(sdb, "fetch_stage_power_cc1", "total power usage of fetch stage_cc1", "icache_power_cc1 + bpred_power_cc1", NULL);

  stat_reg_formula(sdb, "dispatch_stage_power_cc1", "total power usage of dispatch stage_cc1", "rename_power_cc1", NULL);

  stat_reg_formula(sdb, "issue_stage_power_cc1", "total power usage of issue stage_cc1", "resultbus_power_cc1 + alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1 + lsq_power_cc1 + window_power_cc1", NULL);

  stat_reg_formula(sdb, "avg_fetch_power_cc1", "average power of fetch unit per cycle_cc1", "(icache_power_cc1 + bpred_power_cc1)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "avg_dispatch_power_cc1", "average power of dispatch unit per cycle_cc1", "(rename_power_cc1)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "avg_issue_power_cc1", "average power of issue unit per cycle_cc1", "(resultbus_power_cc1 + alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1 + lsq_power_cc1 + window_power_cc1)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "total_power_cycle_cc1", "total power per cycle_cc1","(rename_power_cc1 + bpred_power_cc1 + lsq_power_cc1 + window_power_cc1 + regfile_power_cc1 + icache_power_cc1 + resultbus_power_cc1 + clock_power_cc1 + alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1)", NULL);

  stat_reg_formula(sdb, "avg_total_power_cycle_cc1", "average total power per cycle_cc1","(rename_power_cc1 + bpred_power_cc1 + lsq_power_cc1 + window_power_cc1 + regfile_power_cc1 + icache_power_cc1 + resultbus_power_cc1 + clock_power_cc1 + alu_power_cc1 + dcache_power_cc1 +dcache2_power_cc1)/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_total_power_insn_cc1", "average total power per insn_cc1","(rename_power_cc1 + bpred_power_cc1 + lsq_power_cc1 + window_power_cc1 + regfile_power_cc1 + icache_power_cc1 + resultbus_power_cc1 + clock_power_cc1 +  alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1)/sim_total_insn", NULL);

  stat_reg_double(sdb, "rename_power_cc2", "total power usage of rename unit_cc2", &rename_power_cc2, 0, NULL);

  stat_reg_double(sdb, "bpred_power_cc2", "total power usage of bpred unit_cc2", &bpred_power_cc2, 0, NULL);

  stat_reg_double(sdb, "window_power_cc2", "total power usage of instruction window_cc2", &window_power_cc2, 0, NULL);

  stat_reg_double(sdb, "lsq_power_cc2", "total power usage of lsq_cc2", &lsq_power_cc2, 0, NULL);

  stat_reg_double(sdb, "regfile_power_cc2", "total power usage of arch. regfile_cc2", &regfile_power_cc2, 0, NULL);

  stat_reg_double(sdb, "icache_power_cc2", "total power usage of icache_cc2", &icache_power_cc2, 0, NULL);

  stat_reg_double(sdb, "dcache_power_cc2", "total power usage of dcache_cc2", &dcache_power_cc2, 0, NULL);

  stat_reg_double(sdb, "dcache2_power_cc2", "total power usage of dcache2_cc2", &dcache2_power_cc2, 0, NULL);

  stat_reg_double(sdb, "alu_power_cc2", "total power usage of alu_cc2", &alu_power_cc2, 0, NULL);

  stat_reg_double(sdb, "resultbus_power_cc2", "total power usage of resultbus_cc2", &resultbus_power_cc2, 0, NULL);

  stat_reg_double(sdb, "clock_power_cc2", "total power usage of clock_cc2", &clock_power_cc2, 0, NULL);

  stat_reg_formula(sdb, "avg_rename_power_cc2", "avg power usage of rename unit_cc2", "rename_power_cc2/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_bpred_power_cc2", "avg power usage of bpred unit_cc2", "bpred_power_cc2/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_window_power_cc2", "avg power usage of instruction window_cc2", "window_power_cc2/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_lsq_power_cc2", "avg power usage of instruction lsq_cc2", "lsq_power_cc2/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_regfile_power_cc2", "avg power usage of arch. regfile_cc2", "regfile_power_cc2/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_icache_power_cc2", "avg power usage of icache_cc2", "icache_power_cc2/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_dcache_power_cc2", "avg power usage of dcache_cc2", "dcache_power_cc2/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_dcache2_power_cc2", "avg power usage of dcache2_cc2", "dcache2_power_cc2/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_alu_power_cc2", "avg power usage of alu_cc2", "alu_power_cc2/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_resultbus_power_cc2", "avg power usage of resultbus_cc2", "resultbus_power_cc2/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_clock_power_cc2", "avg power usage of clock_cc2", "clock_power_cc2/sim_cycle",  NULL);

  stat_reg_formula(sdb, "fetch_stage_power_cc2", "total power usage of fetch stage_cc2", "icache_power_cc2 + bpred_power_cc2", NULL);

  stat_reg_formula(sdb, "dispatch_stage_power_cc2", "total power usage of dispatch stage_cc2", "rename_power_cc2", NULL);

  stat_reg_formula(sdb, "issue_stage_power_cc2", "total power usage of issue stage_cc2", "resultbus_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2 + lsq_power_cc2 + window_power_cc2", NULL);

  stat_reg_formula(sdb, "avg_fetch_power_cc2", "average power of fetch unit per cycle_cc2", "(icache_power_cc2 + bpred_power_cc2)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "avg_dispatch_power_cc2", "average power of dispatch unit per cycle_cc2", "(rename_power_cc2)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "avg_issue_power_cc2", "average power of issue unit per cycle_cc2", "(resultbus_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2 + lsq_power_cc2 + window_power_cc2)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "total_power_cycle_cc2", "total power per cycle_cc2","(rename_power_cc2 + bpred_power_cc2 + lsq_power_cc2 + window_power_cc2 + regfile_power_cc2 + icache_power_cc2 + resultbus_power_cc2 + clock_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2)", NULL);

  stat_reg_formula(sdb, "avg_total_power_cycle_cc2", "average total power per cycle_cc2","(rename_power_cc2 + bpred_power_cc2 + lsq_power_cc2 + window_power_cc2 + regfile_power_cc2 + icache_power_cc2 + resultbus_power_cc2 + clock_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2)/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_total_power_insn_cc2", "average total power per insn_cc2","(rename_power_cc2 + bpred_power_cc2 + lsq_power_cc2 + window_power_cc2 + regfile_power_cc2 + icache_power_cc2 + resultbus_power_cc2 + clock_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2)/sim_total_insn", NULL);

  stat_reg_double(sdb, "rename_power_cc3", "total power usage of rename unit_cc3", &rename_power_cc3, 0, NULL);

  stat_reg_double(sdb, "bpred_power_cc3", "total power usage of bpred unit_cc3", &bpred_power_cc3, 0, NULL);

  stat_reg_double(sdb, "window_power_cc3", "total power usage of instruction window_cc3", &window_power_cc3, 0, NULL);

  stat_reg_double(sdb, "lsq_power_cc3", "total power usage of lsq_cc3", &lsq_power_cc3, 0, NULL);

  stat_reg_double(sdb, "regfile_power_cc3", "total power usage of arch. regfile_cc3", &regfile_power_cc3, 0, NULL);

  stat_reg_double(sdb, "icache_power_cc3", "total power usage of icache_cc3", &icache_power_cc3, 0, NULL);

  stat_reg_double(sdb, "dcache_power_cc3", "total power usage of dcache_cc3", &dcache_power_cc3, 0, NULL);

  stat_reg_double(sdb, "dcache2_power_cc3", "total power usage of dcache2_cc3", &dcache2_power_cc3, 0, NULL);

  stat_reg_double(sdb, "alu_power_cc3", "total power usage of alu_cc3", &alu_power_cc3, 0, NULL);

  stat_reg_double(sdb, "resultbus_power_cc3", "total power usage of resultbus_cc3", &resultbus_power_cc3, 0, NULL);

  stat_reg_double(sdb, "clock_power_cc3", "total power usage of clock_cc3", &clock_power_cc3, 0, NULL);

  stat_reg_formula(sdb, "avg_rename_power_cc3", "avg power usage of rename unit_cc3", "rename_power_cc3/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_bpred_power_cc3", "avg power usage of bpred unit_cc3", "bpred_power_cc3/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_window_power_cc3", "avg power usage of instruction window_cc3", "window_power_cc3/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_lsq_power_cc3", "avg power usage of instruction lsq_cc3", "lsq_power_cc3/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_regfile_power_cc3", "avg power usage of arch. regfile_cc3", "regfile_power_cc3/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_icache_power_cc3", "avg power usage of icache_cc3", "icache_power_cc3/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_dcache_power_cc3", "avg power usage of dcache_cc3", "dcache_power_cc3/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_dcache2_power_cc3", "avg power usage of dcache2_cc3", "dcache2_power_cc3/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_alu_power_cc3", "avg power usage of alu_cc3", "alu_power_cc3/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_resultbus_power_cc3", "avg power usage of resultbus_cc3", "resultbus_power_cc3/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_clock_power_cc3", "avg power usage of clock_cc3", "clock_power_cc3/sim_cycle",  NULL);

  stat_reg_formula(sdb, "fetch_stage_power_cc3", "total power usage of fetch stage_cc3", "icache_power_cc3 + bpred_power_cc3", NULL);

  stat_reg_formula(sdb, "dispatch_stage_power_cc3", "total power usage of dispatch stage_cc3", "rename_power_cc3", NULL);

  stat_reg_formula(sdb, "issue_stage_power_cc3", "total power usage of issue stage_cc3", "resultbus_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3 + lsq_power_cc3 + window_power_cc3", NULL);

  stat_reg_formula(sdb, "avg_fetch_power_cc3", "average power of fetch unit per cycle_cc3", "(icache_power_cc3 + bpred_power_cc3)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "avg_dispatch_power_cc3", "average power of dispatch unit per cycle_cc3", "(rename_power_cc3)/ sim_cycle", /* format */NULL);

  stat_reg_formula(sdb, "avg_issue_power_cc3", "average power of issue unit per cycle_cc3", "(resultbus_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3 + lsq_power_cc3 + window_power_cc3)/ sim_cycle", /* format */NULL);

#ifndef PIPE_POWER
  stat_reg_formula(sdb, "total_power_cycle_cc3", "total power per cycle_cc3","(rename_power_cc3 + bpred_power_cc3 + lsq_power_cc3 + window_power_cc3 + regfile_power_cc3 + icache_power_cc3 + resultbus_power_cc3 + clock_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3)", NULL);
  stat_reg_formula(sdb, "avg_total_power_cycle_cc3", "average total power per cycle_cc3","(rename_power_cc3 + bpred_power_cc3 + lsq_power_cc3 + window_power_cc3 + regfile_power_cc3 + icache_power_cc3 + resultbus_power_cc3 + clock_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3)/sim_cycle", NULL);
#else
  stat_reg_double(sdb, "total_power_cycle_cc3", "total power per cycle_cc3", &total_cycle_power_cc3, 0.0, NULL);
  stat_reg_formula(sdb, "avg_total_power_cycle_cc3", "average total power per cycle_cc3", "total_power_cycle_cc3 / sim_cycle", NULL);
#endif

  stat_reg_formula(sdb, "avg_total_power_insn_cc3", "average total power per insn_cc3","(rename_power_cc3 + bpred_power_cc3 + lsq_power_cc3 + window_power_cc3 + regfile_power_cc3 + icache_power_cc3 + resultbus_power_cc3 + clock_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3)/sim_total_insn", NULL);

  stat_reg_counter(sdb, "total_rename_access", "total number accesses of rename unit", &total_rename_access, 0, NULL);

  stat_reg_counter(sdb, "total_bpred_access", "total number accesses of bpred unit", &total_bpred_access, 0, NULL);

  stat_reg_counter(sdb, "total_window_access", "total number accesses of instruction window", &total_window_access, 0, NULL);

  stat_reg_counter(sdb, "total_lsq_access", "total number accesses of load/store queue", &total_lsq_access, 0, NULL);

  stat_reg_counter(sdb, "total_regfile_access", "total number accesses of arch. regfile", &total_regfile_access, 0, NULL);

  stat_reg_counter(sdb, "total_icache_access", "total number accesses of icache", &total_icache_access, 0, NULL);

  stat_reg_counter(sdb, "total_dcache_access", "total number accesses of dcache", &total_dcache_access, 0, NULL);

  stat_reg_counter(sdb, "total_dcache2_access", "total number accesses of dcache2", &total_dcache2_access, 0, NULL);

  stat_reg_counter(sdb, "total_alu_access", "total number accesses of alu", &total_alu_access, 0, NULL);

  stat_reg_counter(sdb, "total_resultbus_access", "total number accesses of resultbus", &total_resultbus_access, 0, NULL);

  stat_reg_formula(sdb, "avg_rename_access", "avg number accesses of rename unit", "total_rename_access/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_bpred_access", "avg number accesses of bpred unit", "total_bpred_access/sim_cycle", NULL);

  stat_reg_formula(sdb, "avg_window_access", "avg number accesses of instruction window", "total_window_access/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_lsq_access", "avg number accesses of lsq", "total_lsq_access/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_regfile_access", "avg number accesses of arch. regfile", "total_regfile_access/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_icache_access", "avg number accesses of icache", "total_icache_access/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_dcache_access", "avg number accesses of dcache", "total_dcache_access/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_dcache2_access", "avg number accesses of dcache2", "total_dcache2_access/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_alu_access", "avg number accesses of alu", "total_alu_access/sim_cycle",  NULL);

  stat_reg_formula(sdb, "avg_resultbus_access", "avg number accesses of resultbus", "total_resultbus_access/sim_cycle",  NULL);

  stat_reg_counter(sdb, "max_rename_access", "max number accesses of rename unit", &max_rename_access, 0, NULL);

  stat_reg_counter(sdb, "max_bpred_access", "max number accesses of bpred unit", &max_bpred_access, 0, NULL);

  stat_reg_counter(sdb, "max_window_access", "max number accesses of instruction window", &max_window_access, 0, NULL);

  stat_reg_counter(sdb, "max_lsq_access", "max number accesses of load/store queue", &max_lsq_access, 0, NULL);

  stat_reg_counter(sdb, "max_regfile_access", "max number accesses of arch. regfile", &max_regfile_access, 0, NULL);

  stat_reg_counter(sdb, "max_icache_access", "max number accesses of icache", &max_icache_access, 0, NULL);

  stat_reg_counter(sdb, "max_dcache_access", "max number accesses of dcache", &max_dcache_access, 0, NULL);

  stat_reg_counter(sdb, "max_dcache2_access", "max number accesses of dcache2", &max_dcache2_access, 0, NULL);

  stat_reg_counter(sdb, "max_alu_access", "max number accesses of alu", &max_alu_access, 0, NULL);

  stat_reg_counter(sdb, "max_resultbus_access", "max number accesses of resultbus", &max_resultbus_access, 0, NULL);

 stat_reg_double(sdb, "min_cycle_power_cc1", "minimum cycle power usage of cc1", &min_cycle_power_cc1, 1000, NULL);

  stat_reg_double(sdb, "min_cycle_power_cc2", "minimum cycle power usage of cc2", &min_cycle_power_cc2, 1000, NULL);

  stat_reg_double(sdb, "min_cycle_power_cc3", "minimum cycle power usage of cc3", &min_cycle_power_cc3, 1000, NULL);

  stat_reg_double(sdb, "max_cycle_power_cc1", "maximum cycle power usage of cc1", &max_cycle_power_cc1, 0, NULL);

  stat_reg_double(sdb, "max_cycle_power_cc2", "maximum cycle power usage of cc2", &max_cycle_power_cc2, 0, NULL);

  stat_reg_double(sdb, "max_cycle_power_cc3", "maximum cycle power usage of cc3", &max_cycle_power_cc3, 0, NULL);

  stat_reg_double(sdb, "min_cycle_current", "minimum cycle current", &min_current, 0, NULL);
  stat_reg_double(sdb, "max_cycle_current", "maximum cycle current", &max_current, 0, NULL);

  stat_reg_double(sdb, "square_total_power_cycle_cc1", "squared sum power of cc1", 
		  &square_total_power_cycle_cc1, 0.0, NULL);

  stat_reg_double(sdb, "square_total_power_cycle_cc2", "squared sum power of cc2", 
		  &square_total_power_cycle_cc2, 0.0, NULL);

  stat_reg_double(sdb, "square_total_power_cycle_cc3", "squared sum power of cc3", 
		  &square_total_power_cycle_cc3, 0.0, NULL);

  stat_reg_formula(sdb, "var_total_power_cycle_cc1", "power variance of cc1",
		   "(square_total_power_cycle_cc1/sim_cycle)-(avg_total_power_cycle_cc1*avg_total_power_cycle_cc1)", 
		   NULL);

  stat_reg_formula(sdb, "var_total_power_cycle_cc2", "power variance of cc2",
		   "(square_total_power_cycle_cc2/sim_cycle)-(avg_total_power_cycle_cc2*avg_total_power_cycle_cc2)", 
		   NULL);

  stat_reg_formula(sdb, "var_total_power_cycle_cc3", "power variance of cc3",
		   "(square_total_power_cycle_cc3/sim_cycle)-(avg_total_power_cycle_cc3*avg_total_power_cycle_cc3)", 
		   NULL);

  stat_reg_counter(sdb, "control_low_cycles", "number of cycles spent in low control mode", 
		   &control_low_cycles, 0, NULL);
  
  stat_reg_counter(sdb, "control_high_cycles", "number of cycles spent in high control mode", 
       &control_high_cycles, 0, NULL);
 
  stat_reg_formula(sdb, "control_low_freq", "percentage of cycles spent in low control mode", 
		   "control_low_cycles / sim_cycle", NULL);

  stat_reg_formula(sdb, "control_high_freq", "percentage of cycles spent in high control mode", 
		   "control_high_cycles / sim_cycle", NULL);

  control_low_length_dist = stat_reg_dist(sdb, "control_low_length_dist",
	"distribution of control low events",
	 /* initial */0, /* array size */15, /* bucket size */1,
	 (PF_COUNT|PF_PDF), NULL, NULL, NULL);

  control_high_length_dist = stat_reg_dist(sdb, "control_high_length_dist",
       "distribution of control high events",
       /* initial */0, /* array size */15, /* bucket size */1,
	(PF_COUNT|PF_PDF), NULL, NULL, NULL);

  if (control_cond) {
    cond_mpred_low = stat_reg_cond(sdb, "mpred", "low", 64);
    cond_il1fill_low = stat_reg_cond(sdb, "il1fill", "low", 64);
    cond_il2fill_low = stat_reg_cond(sdb, "il2fill", "low", 64);
    cond_dl1fill_low = stat_reg_cond(sdb, "dl1fill", "low", 64);
    cond_dl2fill_low = stat_reg_cond(sdb, "dl2fill", "low", 64);
  }

  if (control_within) {
    within_mpred_low = stat_reg_within(sdb, "mpred", "low", 64, FALSE);
    within_il1fill_low = stat_reg_within(sdb, "il1fill", "low", 64, FALSE);
    within_il2fill_low = stat_reg_within(sdb, "il2fill", "low", 64, FALSE);
    within_dl1fill_low = stat_reg_within(sdb, "dl1fill", "low", 64, FALSE);
    within_dl2fill_low = stat_reg_within(sdb, "dl2fill", "low", 64, FALSE);
    within_none_low = stat_reg_within(sdb, "none", "low", 64, TRUE);
  }

  if (control_delta) {
    delta_mpred = stat_reg_delta(sdb, "mpred", 64);
    delta_il1fill = stat_reg_delta(sdb, "il1fill",  64);
    delta_il2fill = stat_reg_delta(sdb, "il2fill",  64);
    delta_dl1fill = stat_reg_delta(sdb, "dl1fill",  64);
    delta_dl2fill = stat_reg_delta(sdb, "dl2fill",  64);
  }

  if (control_burst) {
    int burst_index;

    for(burst_index = 0; burst_index < MAX_BURST; burst_index++)
      burst_arr[burst_index] = stat_reg_burst(sdb, (1+burst_index) * 5, sim_cycle); // jdonald opting to pass sim_cycle into this function, rather than require linking of the ariable
  }

  power_cycle_dist = stat_reg_dist(sdb, "power_cycle_dist",
      "distribution of per cycle power",
      /* initial */0, /* array size */120, /* bucket size */1,
      (PF_COUNT|PF_PDF), NULL, NULL, NULL);

  if (control_step) {
    stat_reg_counter(sdb, "control_step_intervals", "number of step intervals",
		     &control_step_intervals, 0, NULL);
  }

  if (busy_analysis) {
    busy_arr = calloc(busy_width, sizeof(double));
    busy_length_dist = 
      stat_reg_dist(sdb, "busy_length_dist",
		    "distribution of busy lengths",
		    /* initial */0, /* array size */20, /* bucket size */10,
		    (PF_COUNT|PF_PDF), NULL, NULL, NULL);
    idle_length_dist = 
      stat_reg_dist(sdb, "idle_length_dist",
		    "distribution of idle lengths",
		    /* initial */0, /* array size */20, /* bucket size */10,
		    (PF_COUNT|PF_PDF), NULL, NULL, NULL);

    stat_reg_counter(sdb, "busy_interval_count", "length of busy intervals",
		     &busy_interval_count, 0, NULL);

    stat_reg_counter(sdb, "busy_current_interval_length", "cycles in busy intervals",
		     &busy_current_interval_length, 0, NULL);

    stat_reg_counter(sdb, "idle_current_interval_length", "cycles in idle intervals",
		     &idle_current_interval_length, 0, NULL);

    stat_reg_double(sdb, "total_busy_power", "total busy interval power",
		    &total_busy_power, 0.0, NULL);

    stat_reg_double(sdb, "total_idle_power", "total idle interval power",
		    &total_idle_power, 0.0, NULL);

    stat_reg_formula(sdb, "avg_busy_power", "average busy interval power",
		     "total_busy_power / busy_current_interval_length", NULL);

    stat_reg_formula(sdb, "avg_idle_power", "average idle interval power",
		     "total_idle_power / idle_current_interval_length", NULL);
		
    busy_width_recip = 1.0 / busy_width;
  }

  if (power_spectra)
    power_spectra_reg_stats(sdb);

  

  if (power_wavelet) {
    power_wavelet_reg_stats(sdb);
    event_tracker_reg_stats(sdb);
  }

  power_supply_reg_stats(sdb);
}


/* this routine takes the number of rows and cols of an array structure
   and attemps to make it make it more of a reasonable circuit structure
   by trying to make the number of rows and cols as close as possible.
   (scaling both by factors of 2 in opposite directions).  it returns
   a scale factor which is the amount that the rows should be divided
   by and the columns should be multiplied by.
*/
int squarify(int rows, int cols)
{
  int scale_factor = 1;

  if(rows == cols)
    return 1;

  /*
  printf("init rows == %d\n",rows);
  printf("init cols == %d\n",cols);
  */

  while(rows > cols) {
    rows = rows/2;
    cols = cols*2;

    /*
    printf("rows == %d\n",rows);
    printf("cols == %d\n",cols);
    printf("scale_factor == %d (2^ == %d)\n\n",scale_factor,(int)pow(2.0,(double)scale_factor));
    */

    if (rows/2 <= cols)
      return((int)pow(2.0,(double)scale_factor));
    scale_factor++;
  }

  return 1;
}

/* could improve squarify to work when rows < cols */

double squarify_new(int rows, int cols)
{
  double scale_factor = 0.0;

  if(rows==cols)
    return(pow(2.0,scale_factor));

  while(rows > cols) {
    rows = rows/2;
    cols = cols*2;
    if (rows <= cols)
      return(pow(2.0,scale_factor));
    scale_factor++;
  }

  while(cols > rows) {
    rows = rows*2;
    cols = cols/2;
    if (cols <= rows)
      return(pow(2.0,scale_factor));
    scale_factor--;
  }

  return 1;

}

void dump_power_stats(power)
     power_result_type *power;
{
  double total_power;
  double bpred_power;
  double rename_power;
  double rat_power;
  double dcl_power;
  double lsq_power;
  double window_power;
  double wakeup_power;
  double rs_power;
  double lsq_wakeup_power;
  double lsq_rs_power;
  double regfile_power;
  double reorder_power;
  double icache_power;
  double dcache_power;
  double dcache2_power;
  double dtlb_power;
  double itlb_power;
  double ambient_power = 2.0;

  icache_power = power->icache_power;

  dcache_power = power->dcache_power;

  dcache2_power = power->dcache2_power;

  itlb_power = power->itlb;
  dtlb_power = power->dtlb;

  bpred_power = power->btb + power->local_predict + power->global_predict + 
    power->chooser + power->ras;

  rat_power = power->rat_decoder + 
    power->rat_wordline + power->rat_bitline + power->rat_senseamp;

  dcl_power = power->dcl_compare + power->dcl_pencode;

  rename_power = power->rat_power + power->dcl_power + power->inst_decoder_power;

  wakeup_power = power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->wakeup_ormatch;
   
  rs_power = power->rs_decoder + 
    power->rs_wordline + power->rs_bitline + power->rs_senseamp;

  window_power = wakeup_power + rs_power + power->selection;

  lsq_rs_power = power->lsq_rs_decoder + 
    power->lsq_rs_wordline + power->lsq_rs_bitline + power->lsq_rs_senseamp;

  lsq_wakeup_power = power->lsq_wakeup_tagdrive + 
    power->lsq_wakeup_tagmatch + power->lsq_wakeup_ormatch;

  lsq_power = lsq_wakeup_power + lsq_rs_power;

  reorder_power = power->reorder_decoder + 
    power->reorder_wordline + power->reorder_bitline + 
    power->reorder_senseamp;

  regfile_power = power->regfile_decoder + 
    power->regfile_wordline + power->regfile_bitline + 
    power->regfile_senseamp;

  total_power = bpred_power + rename_power + window_power + regfile_power +
    power->resultbus + lsq_power + 
    icache_power + dcache_power + dcache2_power + 
    dtlb_power + itlb_power + power->clock_power + power->ialu_power +
    power->falu_power;

  fprintf(stderr,"\nProcessor Parameters:\n");
  fprintf(stderr,"Issue Width: %d\n",ruu_issue_width);
  fprintf(stderr,"Window Size: %d\n",RUU_size);
  fprintf(stderr,"Number of Virtual Registers: %d\n",MD_NUM_IREGS);
  fprintf(stderr,"Number of Physical Registers: %d\n",RUU_size);
  fprintf(stderr,"Datapath Width: %d\n",data_width);

  fprintf(stderr,"Total Power Consumption: %g\n",total_power+ambient_power);
  fprintf(stderr,"Branch Predictor Power Consumption: %g  (%.3g%%)\n",bpred_power,100*bpred_power/total_power);
  fprintf(stderr," branch target buffer power (W): %g\n",power->btb);
  fprintf(stderr," local predict power (W): %g\n",power->local_predict);
  fprintf(stderr," global predict power (W): %g\n",power->global_predict);
  fprintf(stderr," chooser power (W): %g\n",power->chooser);
  fprintf(stderr," RAS power (W): %g\n",power->ras);
  fprintf(stderr,"Rename Logic Power Consumption: %g  (%.3g%%)\n",rename_power,100*rename_power/total_power);
  fprintf(stderr," Instruction Decode Power (W): %g\n",power->inst_decoder_power);
  fprintf(stderr," RAT decode_power (W): %g\n",power->rat_decoder);
  fprintf(stderr," RAT wordline_power (W): %g\n",power->rat_wordline);
  fprintf(stderr," RAT bitline_power (W): %g\n",power->rat_bitline);
  fprintf(stderr," DCL Comparators (W): %g\n",power->dcl_compare);
  fprintf(stderr,"Instruction Window Power Consumption: %g  (%.3g%%)\n",window_power,100*window_power/total_power);
  fprintf(stderr," tagdrive (W): %g\n",power->wakeup_tagdrive);
  fprintf(stderr," tagmatch (W): %g\n",power->wakeup_tagmatch);
  fprintf(stderr," Selection Logic (W): %g\n",power->selection);
  fprintf(stderr," decode_power (W): %g\n",power->rs_decoder);
  fprintf(stderr," wordline_power (W): %g\n",power->rs_wordline);
  fprintf(stderr," bitline_power (W): %g\n",power->rs_bitline);
  fprintf(stderr,"Load/Store Queue Power Consumption: %g  (%.3g%%)\n",lsq_power,100*lsq_power/total_power);
  fprintf(stderr," tagdrive (W): %g\n",power->lsq_wakeup_tagdrive);
  fprintf(stderr," tagmatch (W): %g\n",power->lsq_wakeup_tagmatch);
  fprintf(stderr," decode_power (W): %g\n",power->lsq_rs_decoder);
  fprintf(stderr," wordline_power (W): %g\n",power->lsq_rs_wordline);
  fprintf(stderr," bitline_power (W): %g\n",power->lsq_rs_bitline);
  fprintf(stderr,"Arch. Register File Power Consumption: %g  (%.3g%%)\n",regfile_power,100*regfile_power/total_power);
  fprintf(stderr," decode_power (W): %g\n",power->regfile_decoder);
  fprintf(stderr," wordline_power (W): %g\n",power->regfile_wordline);
  fprintf(stderr," bitline_power (W): %g\n",power->regfile_bitline);
  fprintf(stderr,"Result Bus Power Consumption: %g  (%.3g%%)\n",power->resultbus,100*power->resultbus/total_power);
  fprintf(stderr,"Total Clock Power: %g  (%.3g%%)\n",power->clock_power,100*power->clock_power/total_power);
  fprintf(stderr,"Int ALU Power: %g  (%.3g%%)\n",power->ialu_power,100*power->ialu_power/total_power);
  fprintf(stderr,"FP ALU Power: %g  (%.3g%%)\n",power->falu_power,100*power->falu_power/total_power);
  fprintf(stderr,"Instruction Cache Power Consumption: %g  (%.3g%%)\n",icache_power,100*icache_power/total_power);
  fprintf(stderr," decode_power (W): %g\n",power->icache_decoder);
  fprintf(stderr," wordline_power (W): %g\n",power->icache_wordline);
  fprintf(stderr," bitline_power (W): %g\n",power->icache_bitline);
  fprintf(stderr," senseamp_power (W): %g\n",power->icache_senseamp);
  fprintf(stderr," tagarray_power (W): %g\n",power->icache_tagarray);
  fprintf(stderr,"Itlb_power (W): %g (%.3g%%)\n",power->itlb,100*power->itlb/total_power);
  fprintf(stderr,"Data Cache Power Consumption: %g  (%.3g%%)\n",dcache_power,100*dcache_power/total_power);
  fprintf(stderr," decode_power (W): %g\n",power->dcache_decoder);
  fprintf(stderr," wordline_power (W): %g\n",power->dcache_wordline);
  fprintf(stderr," bitline_power (W): %g\n",power->dcache_bitline);
  fprintf(stderr," senseamp_power (W): %g\n",power->dcache_senseamp);
  fprintf(stderr," tagarray_power (W): %g\n",power->dcache_tagarray);
  fprintf(stderr,"Dtlb_power (W): %g (%.3g%%)\n",power->dtlb,100*power->dtlb/total_power);
  fprintf(stderr,"Level 2 Cache Power Consumption: %g (%.3g%%)\n",dcache2_power,100*dcache2_power/total_power);
  fprintf(stderr," decode_power (W): %g\n",power->dcache2_decoder);
  fprintf(stderr," wordline_power (W): %g\n",power->dcache2_wordline);
  fprintf(stderr," bitline_power (W): %g\n",power->dcache2_bitline);
  fprintf(stderr," senseamp_power (W): %g\n",power->dcache2_senseamp);
  fprintf(stderr," tagarray_power (W): %g\n",power->dcache2_tagarray);
}

/*======================================================================*/



/* 
 * This part of the code contains routines for each section as
 * described in the tech report.  See the tech report for more details
 * and explanations */

/*----------------------------------------------------------------------*/

double driver_size(double driving_cap, double desiredrisetime) {
  double nsize, psize;
  double Rpdrive; 

  Rpdrive = desiredrisetime/(driving_cap*log(VSINV)*-1.0);
  psize = restowidth(Rpdrive,PCH);
  nsize = restowidth(Rpdrive,NCH);
  if (psize > Wworddrivemax) {
    psize = Wworddrivemax;
  }
  if (psize < 4.0 * LSCALE)
    psize = 4.0 * LSCALE;

  return (psize);

}

/* Decoder delay:  (see section 6.1 of tech report) */

double array_decoder_power(rows,cols,predeclength,rports,wports,cache)
     int rows,cols;
     double predeclength;
     int rports,wports;
     int cache;
{
  double Ctotal=0;
  double Ceq=0;
  int numstack;
  int decode_bits=0;
  int ports;
  double rowsb;

  /* read and write ports are the same here */
  ports = rports + wports;

  rowsb = (double)rows;

  /* number of input bits to be decoded */
  decode_bits=ceil((logtwo(rowsb)));

  /* First stage: driving the decoders */

  /* This is the capacitance for driving one bit (and its complement).
     -There are #rowsb 3->8 decoders contributing gatecap.
     - 2.0 factor from 2 identical sets of drivers in parallel
  */
  Ceq = 2.0*(draincap(Wdecdrivep,PCH,1)+draincap(Wdecdriven,NCH,1)) +
    gatecap(Wdec3to8n+Wdec3to8p,10.0)*rowsb;

  /* There are ports * #decode_bits total */
  Ctotal+=ports*decode_bits*Ceq;

  if(verbose)
    fprintf(stderr,"Decoder -- Driving decoders            == %g\n",.3*Ctotal*Powerfactor);

  /* second stage: driving a bunch of nor gates with a nand 
     numstack is the size of the nor gates -- ie. a 7-128 decoder has
     3-input NAND followed by 3-input NOR  */

  numstack = ceil((1.0/3.0)*logtwo(rows));

  if (numstack<=0) numstack = 1;
  if (numstack>5) numstack = 5;

  /* There are #rowsb NOR gates being driven*/
  Ceq = (3.0*draincap(Wdec3to8p,PCH,1) +draincap(Wdec3to8n,NCH,3) +
	 gatecap(WdecNORn+WdecNORp,((numstack*40)+20.0)))*rowsb;

  Ctotal+=ports*Ceq;

  if(verbose)
    fprintf(stderr,"Decoder -- Driving nor w/ nand         == %g\n",.3*ports*Ceq*Powerfactor);

  /* Final stage: driving an inverter with the nor 
     (inverter preceding wordline driver) -- wordline driver is in the next section*/

  Ceq = (gatecap(Wdecinvn+Wdecinvp,20.0)+
	 numstack*draincap(WdecNORn,NCH,1)+
         draincap(WdecNORp,PCH,numstack));

  if(verbose)
    fprintf(stderr,"Decoder -- Driving inverter w/ nor     == %g\n",.3*ports*Ceq*Powerfactor);

  Ctotal+=ports*Ceq;

  /* assume Activity Factor == .3  */

  return(.3*Ctotal*Powerfactor);
}

double simple_array_decoder_power(rows,cols,rports,wports,cache)
     int rows,cols;
     int rports,wports;
     int cache;
{
  double predeclength=0.0;
  return(array_decoder_power(rows,cols,predeclength,rports,wports,cache));
}


double array_wordline_power(rows,cols,wordlinelength,rports,wports,cache)
     int rows,cols;
     double wordlinelength;
     int rports,wports;
     int cache;
{
  double Ctotal=0;
  double Ceq=0;
  double Cline=0;
  double Cliner, Clinew=0;
  double desiredrisetime,psize,nsize;
  int ports;
  double colsb;

  ports = rports+wports;

  colsb = (double)cols;

  /* Calculate size of wordline drivers assuming rise time == Period / 8 
     - estimate cap on line 
     - compute min resistance to achieve this with RC 
     - compute width needed to achieve this resistance */

  desiredrisetime = Period/16;
  Cline = (gatecappass(Wmemcellr,1.0))*colsb + wordlinelength*CM3metal;
  psize = driver_size(Cline,desiredrisetime);
  
  /* how do we want to do p-n ratioing? -- here we just assume the same ratio 
     from an inverter pair  */
  nsize = psize * Wdecinvn/Wdecinvp; 
  
  if(verbose)
    fprintf(stderr,"Wordline Driver Sizes -- nsize == %f, psize == %f\n",nsize,psize);

  Ceq = draincap(Wdecinvn,NCH,1) + draincap(Wdecinvp,PCH,1) +
    gatecap(nsize+psize,20.0);

  Ctotal+=ports*Ceq;

  if(verbose)
    fprintf(stderr,"Wordline -- Inverter -> Driver         == %g\n",ports*Ceq*Powerfactor);

  /* Compute caps of read wordline and write wordlines 
     - wordline driver caps, given computed width from above
     - read wordlines have 1 nmos access tx, size ~4
     - write wordlines have 2 nmos access tx, size ~2
     - metal line cap
  */

  Cliner = (gatecappass(Wmemcellr,(BitWidth-2*Wmemcellr)/2.0))*colsb+
    wordlinelength*CM3metal+
    2.0*(draincap(nsize,NCH,1) + draincap(psize,PCH,1));
  Clinew = (2.0*gatecappass(Wmemcellw,(BitWidth-2*Wmemcellw)/2.0))*colsb+
    wordlinelength*CM3metal+
    2.0*(draincap(nsize,NCH,1) + draincap(psize,PCH,1));

  if(verbose) {
    fprintf(stderr,"Wordline -- Line                       == %g\n",1e12*Cline);
    fprintf(stderr,"Wordline -- Line -- access -- gatecap  == %g\n",1e12*colsb*2*gatecappass(Wmemcella,(BitWidth-2*Wmemcella)/2.0));
    fprintf(stderr,"Wordline -- Line -- driver -- draincap == %g\n",1e12*draincap(nsize,NCH,1) + draincap(psize,PCH,1));
    fprintf(stderr,"Wordline -- Line -- metal              == %g\n",1e12*wordlinelength*CM3metal);
  }
  Ctotal+=rports*Cliner+wports*Clinew;

  /* AF == 1 assuming a different wordline is charged each cycle, but only
     1 wordline (per port) is actually used */

  return(Ctotal*Powerfactor);
}

double simple_array_wordline_power(rows,cols,rports,wports,cache)
     int rows,cols;
     int rports,wports;
     int cache;
{
  double wordlinelength;
  int ports = rports + wports;
  wordlinelength = cols *  (RegCellWidth + 2 * ports * BitlineSpacing);
  return(array_wordline_power(rows,cols,wordlinelength,rports,wports,cache));
}


double array_bitline_power(rows,cols,bitlinelength,rports,wports,cache)
     int rows,cols;
     double bitlinelength;
     int rports,wports;
     int cache;
{
  double Ctotal=0;
  double Ccolmux=0;
  double Cbitrowr=0;
  double Cbitroww=0;
  double Cprerow=0;
  double Cwritebitdrive=0;
  double Cpregate=0;
  double Cliner=0;
  double Clinew=0;
  int ports;
  double rowsb;
  double colsb;

  double desiredrisetime, Cline, psize, nsize;

  ports = rports + wports;

  rowsb = (double)rows;
  colsb = (double)cols;

  /* Draincaps of access tx's */

  Cbitrowr = draincap(Wmemcellr,NCH,1);
  Cbitroww = draincap(Wmemcellw,NCH,1);

  /* Cprerow -- precharge cap on the bitline
     -simple scheme to estimate size of pre-charge tx's in a similar fashion
      to wordline driver size estimation.
     -FIXME: it would be better to use precharge/keeper pairs, i've omitted this
      from this version because it couldn't autosize as easily.
  */

  desiredrisetime = Period/8;

  Cline = rowsb*Cbitrowr+CM2metal*bitlinelength;
  psize = driver_size(Cline,desiredrisetime);

  /* compensate for not having an nmos pre-charging */
  psize = psize + psize * Wdecinvn/Wdecinvp; 

  if(verbose)
    printf("Cprerow auto   == %g (psize == %g)\n",draincap(psize,PCH,1),psize);

  Cprerow = draincap(psize,PCH,1);

  /* Cpregate -- cap due to gatecap of precharge transistors -- tack this
     onto bitline cap, again this could have a keeper */
  Cpregate = 4.0*gatecap(psize,10.0);
  global_clockcap+=rports*cols*2.0*Cpregate;

  /* Cwritebitdrive -- write bitline drivers are used instead of the precharge
     stuff for write bitlines
     - 2 inverter drivers within each driver pair */

  Cline = rowsb*Cbitroww+CM2metal*bitlinelength;

  psize = driver_size(Cline,desiredrisetime);
  nsize = psize * Wdecinvn/Wdecinvp; 

  Cwritebitdrive = 2.0*(draincap(psize,PCH,1)+draincap(nsize,NCH,1));

  /* 
     reg files (cache==0) 
     => single ended bitlines (1 bitline/col)
     => AFs from pop_count
     caches (cache ==1)
     => double-ended bitlines (2 bitlines/col)
     => AFs = .5 (since one of the two bitlines is always charging/discharging)
  */

#ifdef STATIC_AF
  if (cache == 0) {
    /* compute the total line cap for read/write bitlines */
    Cliner = rowsb*Cbitrowr+CM2metal*bitlinelength+Cprerow;
    Clinew = rowsb*Cbitroww+CM2metal*bitlinelength+Cwritebitdrive;

    /* Bitline inverters at the end of the bitlines (replaced w/ sense amps
       in cache styles) */
    Ccolmux = gatecap(MSCALE*(29.9+7.8),0.0)+gatecap(MSCALE*(47.0+12.0),0.0);
    Ctotal+=(1.0-POPCOUNT_AF)*rports*cols*(Cliner+Ccolmux+2.0*Cpregate);
    Ctotal+=.3*wports*cols*(Clinew+Cwritebitdrive);
  } 
  else { 
    Cliner = rowsb*Cbitrowr+CM2metal*bitlinelength+Cprerow + draincap(Wbitmuxn,NCH,1);
    Clinew = rowsb*Cbitroww+CM2metal*bitlinelength+Cwritebitdrive;
    Ccolmux = (draincap(Wbitmuxn,NCH,1))+2.0*gatecap(WsenseQ1to4,10.0);
    Ctotal+=.5*rports*2.0*cols*(Cliner+Ccolmux+2.0*Cpregate);
    Ctotal+=.5*wports*2.0*cols*(Clinew+Cwritebitdrive);
  }
#else
  if (cache == 0) {
    /* compute the total line cap for read/write bitlines */
    Cliner = rowsb*Cbitrowr+CM2metal*bitlinelength+Cprerow;
    Clinew = rowsb*Cbitroww+CM2metal*bitlinelength+Cwritebitdrive;

    /* Bitline inverters at the end of the bitlines (replaced w/ sense amps
       in cache styles) */
    Ccolmux = gatecap(MSCALE*(29.9+7.8),0.0)+gatecap(MSCALE*(47.0+12.0),0.0);
    Ctotal += rports*cols*(Cliner+Ccolmux+2.0*Cpregate);
    Ctotal += .3*wports*cols*(Clinew+Cwritebitdrive);
  } 
  else { 
    Cliner = rowsb*Cbitrowr+CM2metal*bitlinelength+Cprerow + draincap(Wbitmuxn,NCH,1);
    Clinew = rowsb*Cbitroww+CM2metal*bitlinelength+Cwritebitdrive;
    Ccolmux = (draincap(Wbitmuxn,NCH,1))+2.0*gatecap(WsenseQ1to4,10.0);
    Ctotal+=.5*rports*2.0*cols*(Cliner+Ccolmux+2.0*Cpregate);
    Ctotal+=.5*wports*2.0*cols*(Clinew+Cwritebitdrive);
  }
#endif

  if(verbose) {
    fprintf(stderr,"Bitline -- Precharge                   == %g\n",1e12*Cpregate);
    fprintf(stderr,"Bitline -- Line                        == %g\n",1e12*(Cliner+Clinew));
    fprintf(stderr,"Bitline -- Line -- access draincap     == %g\n",1e12*rowsb*Cbitrowr);
    fprintf(stderr,"Bitline -- Line -- precharge draincap  == %g\n",1e12*Cprerow);
    fprintf(stderr,"Bitline -- Line -- metal               == %g\n",1e12*bitlinelength*CM2metal);
    fprintf(stderr,"Bitline -- Colmux                      == %g\n",1e12*Ccolmux);

    fprintf(stderr,"\n");
  }


  if(cache==0)
    return(Ctotal*Powerfactor);
  else
    return(Ctotal*SensePowerfactor*.4);
  
}


double simple_array_bitline_power(rows,cols,rports,wports,cache)
     int rows,cols;
     int rports,wports;
     int cache;
{
  double bitlinelength;

  int ports = rports + wports;

  bitlinelength = rows * (RegCellHeight + ports * WordlineSpacing);

  return (array_bitline_power(rows,cols,bitlinelength,rports,wports,cache));

}

/* estimate senseamp power dissipation in cache structures (Zyuban's method) */
double senseamp_power(int cols)
{
  return((double)cols * Vdd/8 * .5e-3);
}

/* estimate comparator power consumption (this comparator is similar
   to the tag-match structure in a CAM */
double compare_cap(int compare_bits)
{
  double c1, c2;
  /* bottom part of comparator */
  c2 = (compare_bits)*(draincap(Wcompn,NCH,1)+draincap(Wcompn,NCH,2))+
    draincap(Wevalinvp,PCH,1) + draincap(Wevalinvn,NCH,1);

  /* top part of comparator */
  c1 = (compare_bits)*(draincap(Wcompn,NCH,1)+draincap(Wcompn,NCH,2)+
		       draincap(Wcomppreequ,NCH,1)) +
    gatecap(WdecNORn,1.0)+
    gatecap(WdecNORp,3.0);

  return(c1 + c2);
}

/* power of depency check logic */
double dcl_compare_power(int compare_bits)
{
  double Ctotal;
  int num_comparators;
  
  num_comparators = (ruu_decode_width - 1) * (ruu_decode_width);

  Ctotal = num_comparators * compare_cap(compare_bits);

  return(Ctotal*Powerfactor*ACTIVITY_FACTOR);
}

double simple_array_power(rows,cols,rports,wports,cache)
     int rows,cols;
     int rports,wports;
     int cache;
{
  if(cache==0)
    return( simple_array_decoder_power(rows,cols,rports,wports,cache)+
	    simple_array_wordline_power(rows,cols,rports,wports,cache)+
	    simple_array_bitline_power(rows,cols,rports,wports,cache));
  else
    return( simple_array_decoder_power(rows,cols,rports,wports,cache)+
	    simple_array_wordline_power(rows,cols,rports,wports,cache)+
	    simple_array_bitline_power(rows,cols,rports,wports,cache)+
	    senseamp_power(cols));
}


double cam_tagdrive(rows,cols,rports,wports)
     int rows,cols,rports,wports;
{
  double Ctotal, Ctlcap, Cblcap, Cwlcap;
  double taglinelength;
  double wordlinelength;
  double nsize, psize;
  int ports;
  Ctotal=0;

  ports = rports + wports;

  taglinelength = rows * 
    (CamCellHeight + ports * MatchlineSpacing);

  wordlinelength = cols * 
    (CamCellWidth + ports * TaglineSpacing);

  /* Compute tagline cap */
  Ctlcap = Cmetal * taglinelength + 
    rows * gatecappass(Wcomparen2,2.0) +
    draincap(Wcompdrivern,NCH,1)+draincap(Wcompdriverp,PCH,1);

  /* Compute bitline cap (for writing new tags) */
  Cblcap = Cmetal * taglinelength +
    rows * draincap(Wmemcellr,NCH,2);

  /* autosize wordline driver */
  psize = driver_size(Cmetal * wordlinelength + 2 * cols * gatecap(Wmemcellr,2.0),Period/8);
  nsize = psize * Wdecinvn/Wdecinvp; 

  /* Compute wordline cap (for writing new tags) */
  Cwlcap = Cmetal * wordlinelength + 
    draincap(nsize,NCH,1)+draincap(psize,PCH,1) +
    2 * cols * gatecap(Wmemcellr,2.0);
    
  Ctotal += (rports * cols * 2 * Ctlcap) + 
    (wports * ((cols * 2 * Cblcap) + (rows * Cwlcap)));

  return(Ctotal*Powerfactor*ACTIVITY_FACTOR);
}

double cam_tagmatch(rows,cols,rports,wports)
     int rows,cols,rports,wports;
{
  double Ctotal, Cmlcap;
  double matchlinelength;
  int ports;
  Ctotal=0;

  ports = rports + wports;

  matchlinelength = cols * 
    (CamCellWidth + ports * TaglineSpacing);

  Cmlcap = 2 * cols * draincap(Wcomparen1,NCH,2) + 
    Cmetal * matchlinelength + draincap(Wmatchpchg,NCH,1) +
    gatecap(Wmatchinvn+Wmatchinvp,10.0) +
    gatecap(Wmatchnandn+Wmatchnandp,10.0);

  Ctotal += rports * rows * Cmlcap;

  global_clockcap += rports * rows * gatecap(Wmatchpchg,5.0);
  
  /* noring the nanded match lines */
  if(ruu_issue_width >= 8)
    Ctotal += 2 * gatecap(Wmatchnorn+Wmatchnorp,10.0);

  return(Ctotal*Powerfactor*ACTIVITY_FACTOR);
}

double cam_array(rows,cols,rports,wports)
     int rows,cols,rports,wports;
{
  return(cam_tagdrive(rows,cols,rports,wports) +
	 cam_tagmatch(rows,cols,rports,wports));
}


double selection_power(int win_entries)
{
  double Ctotal, Cor, Cpencode;
  int num_arbiter=1;

  Ctotal=0;

  while(win_entries > 4)
    {
      win_entries = (int)ceil((double)win_entries / 4.0);
      num_arbiter += win_entries;
    }

  Cor = 4 * draincap(WSelORn,NCH,1) + draincap(WSelORprequ,PCH,1);

  Cpencode = draincap(WSelPn,NCH,1) + draincap(WSelPp,PCH,1) + 
    2*draincap(WSelPn,NCH,1) + draincap(WSelPp,PCH,2) + 
    3*draincap(WSelPn,NCH,1) + draincap(WSelPp,PCH,3) + 
    4*draincap(WSelPn,NCH,1) + draincap(WSelPp,PCH,4) + 
    4*gatecap(WSelEnn+WSelEnp,20.0) + 
    4*draincap(WSelEnn,NCH,1) + 4*draincap(WSelEnp,PCH,1);

  Ctotal += ruu_issue_width * num_arbiter*(Cor+Cpencode);

  return(Ctotal*Powerfactor*ACTIVITY_FACTOR);
}

/* very rough clock power estimates */
double total_clockpower(double die_length)
{

  double clocklinelength;
  double Cline,Cline2,Ctotal;
  double pipereg_clockcap=0;
  double global_buffercap = 0;
  double Clockpower;

  double num_piperegs;

  int npreg_width = (int)ceil(logtwo((double)RUU_size));

  /* Assume say 8 stages (kinda low now).
     FIXME: this could be a lot better; user could input
     number of pipestages, etc  */

  /* assume 8 pipe stages and try to estimate bits per pipe stage */
  /* pipe stage 0/1 */
  num_piperegs = ruu_issue_width*inst_length + data_width;
  /* pipe stage 1/2 */
  num_piperegs += ruu_issue_width*(inst_length + 3 * RUU_size);
  /* pipe stage 2/3 */
  num_piperegs += ruu_issue_width*(inst_length + 3 * RUU_size);
  /* pipe stage 3/4 */
  num_piperegs += ruu_issue_width*(3 * npreg_width + pow2(opcode_length));
  /* pipe stage 4/5 */
  num_piperegs += ruu_issue_width*(2*data_width + pow2(opcode_length));
  /* pipe stage 5/6 */
  num_piperegs += ruu_issue_width*(data_width + pow2(opcode_length));
  /* pipe stage 6/7 */
  num_piperegs += ruu_issue_width*(data_width + pow2(opcode_length));
  /* pipe stage 7/8 */
  num_piperegs += ruu_issue_width*(data_width + pow2(opcode_length));

  /* assume 50% extra in control signals (rule of thumb) */
  num_piperegs = num_piperegs * 1.5;

  pipereg_clockcap = num_piperegs * 4*gatecap(10.0,0);

  /* estimate based on 3% of die being in clock metal */
  Cline2 = Cmetal * (.03 * die_length * die_length/BitlineSpacing) * 1e6 * 1e6;

  /* another estimate */
  clocklinelength = die_length*(.5 + 4 * (.25 + 2*(.25) + 4 * (.125)));
  Cline = 20 * Cmetal * (clocklinelength) * 1e6;
  global_buffercap = 12*gatecap(1000.0,10.0)+16*gatecap(200,10.0)+16*8*2*gatecap(100.0,10.00) + 2*gatecap(.29*1e6,10.0);
  /* global_clockcap is computed within each array structure for pre-charge tx's*/
  Ctotal = Cline+global_clockcap+pipereg_clockcap+global_buffercap;

  if(verbose)
    fprintf(stderr,"num_piperegs == %f\n",num_piperegs);

  /* add I_ADD Clockcap and F_ADD Clockcap */
  Clockpower = Ctotal*Powerfactor + res_ialu*I_ADD_CLOCK + res_fpalu*F_ADD_CLOCK;

  if(verbose) {
    fprintf(stderr,"Global Clock Power: %g\n",Clockpower);
    fprintf(stderr," Global Metal Lines   (W): %g\n",Cline*Powerfactor);
    fprintf(stderr," Global Metal Lines (3%%) (W): %g\n",Cline2*Powerfactor);
    fprintf(stderr," Global Clock Buffers (W): %g\n",global_buffercap*Powerfactor);
    fprintf(stderr," Global Clock Cap (Explicit) (W): %g\n",global_clockcap*Powerfactor+I_ADD_CLOCK+F_ADD_CLOCK);
    fprintf(stderr," Global Clock Cap (Implicit) (W): %g\n",pipereg_clockcap*Powerfactor);
  }
  return(Clockpower);

}

/* very rough global clock power estimates */
double global_clockpower(double die_length)
{

  double clocklinelength;
  double Cline,Cline2,Ctotal;
  double global_buffercap = 0;

  Cline2 = Cmetal * (.03 * die_length * die_length/BitlineSpacing) * 1e6 * 1e6;

  clocklinelength = die_length*(.5 + 4 * (.25 + 2*(.25) + 4 * (.125)));
  Cline = 20 * Cmetal * (clocklinelength) * 1e6;
  global_buffercap = 12*gatecap(1000.0,10.0)+16*gatecap(200,10.0)+16*8*2*gatecap(100.0,10.00) + 2*gatecap(.29*1e6,10.0);
  Ctotal = Cline+global_buffercap;

  if(verbose) {
    fprintf(stderr,"Global Clock Power: %g\n",Ctotal*Powerfactor);
    fprintf(stderr," Global Metal Lines   (W): %g\n",Cline*Powerfactor);
    fprintf(stderr," Global Metal Lines (3%%) (W): %g\n",Cline2*Powerfactor);
    fprintf(stderr," Global Clock Buffers (W): %g\n",global_buffercap*Powerfactor);
  }

  return(Ctotal*Powerfactor);

}


double compute_resultbus_power()
{
  double Ctotal, Cline;

  double regfile_height;

  /* compute size of result bus tags */
  int npreg_width = (int)ceil(logtwo((double)RUU_size));

  Ctotal=0;

  regfile_height = RUU_size * (RegCellHeight + 
			       WordlineSpacing * 3 * ruu_issue_width); 

  /* assume num alu's == ialu  (FIXME: generate a more detailed result bus network model*/
  Cline = Cmetal * (regfile_height + .5 * res_ialu * 3200.0 * LSCALE);

  /* or use result bus length measured from 21264 die photo */
  /*  Cline = Cmetal * 3.3*1000;*/

  /* Assume ruu_issue_width result busses -- power can be scaled linearly
     for number of result busses (scale by writeback_access) */
  Ctotal += 2.0 * (data_width + npreg_width) * (ruu_issue_width)* Cline;

#ifdef STATIC_AF
  return(Ctotal*Powerfactor*ACTIVITY_FACTOR);
#else
  return(Ctotal*Powerfactor);
#endif
  
}

void calculate_power(power)
     power_result_type *power;
{
  double clockpower;
  double predeclength, wordlinelength, bitlinelength;
  int ndwl, ndbl, nspd, ntwl, ntbl, ntspd, c,b,a,cache, rowsb, colsb;
  int trowsb, tcolsb, tagsize;
  int va_size = 48;

  int npreg_width = (int)ceil(logtwo((double)RUU_size));

  /* these variables are needed to use Cacti to auto-size cache arrays 
     (for optimal delay) */
  time_result_type time_result;
  time_parameter_type time_parameters;

  /* used to autosize other structures, like bpred tables */
  int scale_factor;

  global_clockcap = 0;

  cache=0;


  /* FIXME: ALU power is a simple constant, it would be better
     to include bit AFs and have different numbers for different
     types of operations */
  power->ialu_power = res_ialu * I_ADD;
  power->falu_power = res_fpalu * F_ADD;

  nvreg_width = (int)ceil(logtwo((double)MD_NUM_IREGS));
  npreg_width = (int)ceil(logtwo((double)RUU_size));


  /* RAT has shadow bits stored in each cell, this makes the
     cell size larger than normal array structures, so we must
     compute it here */

  predeclength = MD_NUM_IREGS * 
    (RatCellHeight + 3 * ruu_decode_width * WordlineSpacing);

  wordlinelength = npreg_width * 
    (RatCellWidth + 
     6 * ruu_decode_width * BitlineSpacing + 
     RatShiftRegWidth*RatNumShift);

  bitlinelength = MD_NUM_IREGS * (RatCellHeight + 3 * ruu_decode_width * WordlineSpacing);

  if(verbose)
    fprintf(stderr,"rat power stats\n");
  power->rat_decoder = array_decoder_power(MD_NUM_IREGS,npreg_width,predeclength,2*ruu_decode_width,ruu_decode_width,cache);
  power->rat_wordline = array_wordline_power(MD_NUM_IREGS,npreg_width,wordlinelength,2*ruu_decode_width,ruu_decode_width,cache);
  power->rat_bitline = array_bitline_power(MD_NUM_IREGS,npreg_width,bitlinelength,2*ruu_decode_width,ruu_decode_width,cache);
  power->rat_senseamp = 0;

  power->dcl_compare = dcl_compare_power(nvreg_width);
  power->dcl_pencode = 0;
  power->inst_decoder_power = ruu_decode_width * simple_array_decoder_power(opcode_length,1,1,1,cache);
  power->wakeup_tagdrive =cam_tagdrive(RUU_size,npreg_width,ruu_issue_width,ruu_issue_width);
  power->wakeup_tagmatch =cam_tagmatch(RUU_size,npreg_width,ruu_issue_width,ruu_issue_width);
  power->wakeup_ormatch =0; 

  power->selection = selection_power(RUU_size);


  predeclength = MD_NUM_IREGS * (RegCellHeight + 3 * ruu_issue_width * WordlineSpacing);

  wordlinelength = data_width * 
    (RegCellWidth + 
     6 * ruu_issue_width * BitlineSpacing);

  bitlinelength = MD_NUM_IREGS * (RegCellHeight + 3 * ruu_issue_width * WordlineSpacing);

  if(verbose)
    fprintf(stderr,"regfile power stats\n");

  power->regfile_decoder = array_decoder_power(MD_NUM_IREGS,data_width,predeclength,2*ruu_issue_width,ruu_issue_width,cache);
  power->regfile_wordline = array_wordline_power(MD_NUM_IREGS,data_width,wordlinelength,2*ruu_issue_width,ruu_issue_width,cache);
  power->regfile_bitline = array_bitline_power(MD_NUM_IREGS,data_width,bitlinelength,2*ruu_issue_width,ruu_issue_width,cache);
  power->regfile_senseamp =0;

  predeclength = RUU_size * (RegCellHeight + 3 * ruu_issue_width * WordlineSpacing);

  wordlinelength = data_width * 
    (RegCellWidth + 
     6 * ruu_issue_width * BitlineSpacing);

  bitlinelength = RUU_size * (RegCellHeight + 3 * ruu_issue_width * WordlineSpacing);

  if(verbose)
    fprintf(stderr,"res station power stats\n");
  power->rs_decoder = array_decoder_power(RUU_size,data_width,predeclength,2*ruu_issue_width,ruu_issue_width,cache);
  power->rs_wordline = array_wordline_power(RUU_size,data_width,wordlinelength,2*ruu_issue_width,ruu_issue_width,cache);
  power->rs_bitline = array_bitline_power(RUU_size,data_width,bitlinelength,2*ruu_issue_width,ruu_issue_width,cache);
  /* no senseamps in reg file structures (only caches) */
  power->rs_senseamp =0;

  /* addresses go into lsq tag's */
  power->lsq_wakeup_tagdrive =cam_tagdrive(LSQ_size,data_width,res_memport,res_memport);
  power->lsq_wakeup_tagmatch =cam_tagmatch(LSQ_size,data_width,res_memport,res_memport);
  power->lsq_wakeup_ormatch =0; 

  wordlinelength = data_width * 
    (RegCellWidth + 
     4 * res_memport * BitlineSpacing);

  bitlinelength = RUU_size * (RegCellHeight + 4 * res_memport * WordlineSpacing);

  /* rs's hold data */
  if(verbose)
    fprintf(stderr,"lsq station power stats\n");
  power->lsq_rs_decoder = array_decoder_power(LSQ_size,data_width,predeclength,res_memport,res_memport,cache);
  power->lsq_rs_wordline = array_wordline_power(LSQ_size,data_width,wordlinelength,res_memport,res_memport,cache);
  power->lsq_rs_bitline = array_bitline_power(LSQ_size,data_width,bitlinelength,res_memport,res_memport,cache);
  power->lsq_rs_senseamp =0;

  power->resultbus = compute_resultbus_power();

  /* Load cache values into what cacti is expecting */
  time_parameters.cache_size = btb_config[0] * (data_width/8) * btb_config[1]; /* C */
  time_parameters.block_size = (data_width/8); /* B */
  time_parameters.associativity = btb_config[1]; /* A */
  time_parameters.number_of_sets = btb_config[0]; /* C/(B*A) */

  /* have Cacti compute optimal cache config */
  calculate_time(&time_result,&time_parameters);
  /* POWER : Have commented out all calls to output data */
  //output_data(&time_result,&time_parameters);

  /* extract Cacti results */
  ndwl=time_result.best_Ndwl;
  ndbl=time_result.best_Ndbl;
  nspd=time_result.best_Nspd;
  ntwl=time_result.best_Ntwl;
  ntbl=time_result.best_Ntbl;
  ntspd=time_result.best_Ntspd;
  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity; 

  cache=1;

  /* Figure out how many rows/cols there are now */
  rowsb = c/(b*a*ndbl*nspd);
  colsb = 8*b*a*nspd/ndwl;

  if(verbose) {
    fprintf(stderr,"%d KB %d-way btb (%d-byte block size):\n",c,a,b);
    fprintf(stderr,"ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    fprintf(stderr,"%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
  }

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"btb power stats\n");
  power->btb = ndwl*ndbl*(array_decoder_power(rowsb,colsb,predeclength,1,1,cache) + array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache) + array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache) + senseamp_power(colsb));

  cache=1;

  scale_factor = squarify(twolev_config[0],twolev_config[2]);
  predeclength = (twolev_config[0] / scale_factor)* (RegCellHeight + WordlineSpacing);
  wordlinelength = twolev_config[2] * scale_factor *  (RegCellWidth + BitlineSpacing);
  bitlinelength = (twolev_config[0] / scale_factor) * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"local predict power stats\n");

  power->local_predict = array_decoder_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,predeclength,1,1,cache) + array_wordline_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,wordlinelength,1,1,cache) + array_bitline_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,bitlinelength,1,1,cache) + senseamp_power(twolev_config[2]*scale_factor);

  scale_factor = squarify(twolev_config[1],3);

  predeclength = (twolev_config[1] / scale_factor)* (RegCellHeight + WordlineSpacing);
  wordlinelength = 3 * scale_factor *  (RegCellWidth + BitlineSpacing);
  bitlinelength = (twolev_config[1] / scale_factor) * (RegCellHeight + WordlineSpacing);


  if(verbose)
    fprintf(stderr,"local predict power stats\n");
  power->local_predict += array_decoder_power(twolev_config[1]/scale_factor,3*scale_factor,predeclength,1,1,cache) + array_wordline_power(twolev_config[1]/scale_factor,3*scale_factor,wordlinelength,1,1,cache) + array_bitline_power(twolev_config[1]/scale_factor,3*scale_factor,bitlinelength,1,1,cache) + senseamp_power(3*scale_factor);

  if(verbose)
    fprintf(stderr,"bimod_config[0] == %d\n",bimod_config[0]);

  scale_factor = squarify(bimod_config[0],2);

  predeclength = bimod_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
  wordlinelength = 2*scale_factor *  (RegCellWidth + BitlineSpacing);
  bitlinelength = bimod_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);


  if(verbose)
    fprintf(stderr,"global predict power stats\n");
  power->global_predict = array_decoder_power(bimod_config[0]/scale_factor,2*scale_factor,predeclength,1,1,cache) + array_wordline_power(bimod_config[0]/scale_factor,2*scale_factor,wordlinelength,1,1,cache) + array_bitline_power(bimod_config[0]/scale_factor,2*scale_factor,bitlinelength,1,1,cache) + senseamp_power(2*scale_factor);

  scale_factor = squarify(comb_config[0],2);

  predeclength = comb_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
  wordlinelength = 2*scale_factor *  (RegCellWidth + BitlineSpacing);
  bitlinelength = comb_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"chooser predict power stats\n");
  power->chooser = array_decoder_power(comb_config[0]/scale_factor,2*scale_factor,predeclength,1,1,cache) + array_wordline_power(comb_config[0]/scale_factor,2*scale_factor,wordlinelength,1,1,cache) + array_bitline_power(comb_config[0]/scale_factor,2*scale_factor,bitlinelength,1,1,cache) + senseamp_power(2*scale_factor);

  if(verbose)
    fprintf(stderr,"RAS predict power stats\n");
  power->ras = simple_array_power(ras_size,data_width,1,1,0);

  tagsize = va_size - ((int)logtwo(cache_dl1->nsets) + (int)logtwo(cache_dl1->bsize));

  if(verbose)
    fprintf(stderr,"dtlb predict power stats\n");
  power->dtlb = res_memport*(cam_array(dtlb->nsets, va_size - (int)logtwo((double)dtlb->bsize),1,1) + simple_array_power(dtlb->nsets,tagsize,1,1,cache));

  tagsize = va_size - ((int)logtwo(cache_il1->nsets) + (int)logtwo(cache_il1->bsize));

  predeclength = itlb->nsets * (RegCellHeight + WordlineSpacing);
  wordlinelength = logtwo((double)itlb->bsize) * (RegCellWidth + BitlineSpacing);
  bitlinelength = itlb->nsets * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"itlb predict power stats\n");
  power->itlb = cam_array(itlb->nsets, va_size - (int)logtwo((double)itlb->bsize),1,1) + simple_array_power(itlb->nsets,tagsize,1,1,cache);


  cache=1;

  time_parameters.cache_size = cache_il1->nsets * cache_il1->bsize * cache_il1->assoc; /* C */
  time_parameters.block_size = cache_il1->bsize; /* B */
  time_parameters.associativity = cache_il1->assoc; /* A */
  time_parameters.number_of_sets = cache_il1->nsets; /* C/(B*A) */

  calculate_time(&time_result,&time_parameters);
  //output_data(&time_result,&time_parameters);

  ndwl=time_result.best_Ndwl;
  ndbl=time_result.best_Ndbl;
  nspd=time_result.best_Nspd;
  ntwl=time_result.best_Ntwl;
  ntbl=time_result.best_Ntbl;
  ntspd=time_result.best_Ntspd;

  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity;

  rowsb = c/(b*a*ndbl*nspd);
  colsb = 8*b*a*nspd/ndwl;

  tagsize = va_size - ((int)logtwo(cache_il1->nsets) + (int)logtwo(cache_il1->bsize));
  trowsb = c/(b*a*ntbl*ntspd);
  tcolsb = a * (tagsize + 1 + 6) * ntspd/ntwl;
 
  if(verbose) {
    fprintf(stderr,"%d KB %d-way cache (%d-byte block size):\n",c,a,b);
    fprintf(stderr,"ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    fprintf(stderr,"%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    fprintf(stderr,"tagsize == %d\n",tagsize);
  }

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"icache power stats\n");
  power->icache_decoder = ndwl*ndbl*array_decoder_power(rowsb,colsb,predeclength,1,1,cache);
  power->icache_wordline = ndwl*ndbl*array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache);
  power->icache_bitline = ndwl*ndbl*array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache);
  power->icache_senseamp = ndwl*ndbl*senseamp_power(colsb);
  power->icache_tagarray = ntwl*ntbl*(simple_array_power(trowsb,tcolsb,1,1,cache));

  power->icache_power = power->icache_decoder + power->icache_wordline + power->icache_bitline + power->icache_senseamp + power->icache_tagarray;

  time_parameters.cache_size = cache_dl1->nsets * cache_dl1->bsize * cache_dl1->assoc; /* C */
  time_parameters.block_size = cache_dl1->bsize; /* B */
  time_parameters.associativity = cache_dl1->assoc; /* A */
  time_parameters.number_of_sets = cache_dl1->nsets; /* C/(B*A) */

  calculate_time(&time_result,&time_parameters);
  //output_data(&time_result,&time_parameters);

  ndwl=time_result.best_Ndwl;
  ndbl=time_result.best_Ndbl;
  nspd=time_result.best_Nspd;
  ntwl=time_result.best_Ntwl;
  ntbl=time_result.best_Ntbl;
  ntspd=time_result.best_Ntspd;
  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity; 

  cache=1;

  rowsb = c/(b*a*ndbl*nspd);
  colsb = 8*b*a*nspd/ndwl;

  tagsize = va_size - ((int)logtwo(cache_dl1->nsets) + (int)logtwo(cache_dl1->bsize));
  trowsb = c/(b*a*ntbl*ntspd);
  tcolsb = a * (tagsize + 1 + 6) * ntspd/ntwl;

  if(verbose) {
    fprintf(stderr,"%d KB %d-way cache (%d-byte block size):\n",c,a,b);
    fprintf(stderr,"ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    fprintf(stderr,"%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    fprintf(stderr,"tagsize == %d\n",tagsize);

    fprintf(stderr,"\nntwl == %d, ntbl == %d, ntspd == %d\n",ntwl,ntbl,ntspd);
    fprintf(stderr,"%d sets of %d rows x %d cols\n",ntwl*ntbl,trowsb,tcolsb);
  }

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"dcache power stats\n");
  power->dcache_decoder = res_memport*ndwl*ndbl*array_decoder_power(rowsb,colsb,predeclength,1,1,cache);
  power->dcache_wordline = res_memport*ndwl*ndbl*array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache);
  power->dcache_bitline = res_memport*ndwl*ndbl*array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache);
  power->dcache_senseamp = res_memport*ndwl*ndbl*senseamp_power(colsb);
  power->dcache_tagarray = res_memport*ntwl*ntbl*(simple_array_power(trowsb,tcolsb,1,1,cache));

  power->dcache_power = power->dcache_decoder + power->dcache_wordline + power->dcache_bitline + power->dcache_senseamp + power->dcache_tagarray;

  clockpower = total_clockpower(.018);
  power->clock_power = clockpower;
  if(verbose) {
    fprintf(stderr,"result bus power == %f\n",power->resultbus);
    fprintf(stderr,"global clock power == %f\n",clockpower);
  }

  time_parameters.cache_size = cache_dl2->nsets * cache_dl2->bsize * cache_dl2->assoc; /* C */
  time_parameters.block_size = cache_dl2->bsize; /* B */
  time_parameters.associativity = cache_dl2->assoc; /* A */
  time_parameters.number_of_sets = cache_dl2->nsets; /* C/(B*A) */

  calculate_time(&time_result,&time_parameters);
  /* POWER : Have commented out all calls to output data */
  //output_data(&time_result,&time_parameters);

  ndwl=time_result.best_Ndwl;
  ndbl=time_result.best_Ndbl;
  nspd=time_result.best_Nspd;
  ntwl=time_result.best_Ntwl;
  ntbl=time_result.best_Ntbl;
  ntspd=time_result.best_Ntspd;
  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity;

  rowsb = c/(b*a*ndbl*nspd);
  colsb = 8*b*a*nspd/ndwl;

  tagsize = va_size - ((int)logtwo(cache_dl2->nsets) + (int)logtwo(cache_dl2->bsize));
  trowsb = c/(b*a*ntbl*ntspd);
  tcolsb = a * (tagsize + 1 + 6) * ntspd/ntwl;

  if(verbose) {
    fprintf(stderr,"%d KB %d-way cache (%d-byte block size):\n",c,a,b);
    fprintf(stderr,"ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    fprintf(stderr,"%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    fprintf(stderr,"tagsize == %d\n",tagsize);
  }

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"dcache2 power stats\n");
  power->dcache2_decoder = array_decoder_power(rowsb,colsb,predeclength,1,1,cache);
  power->dcache2_wordline = array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache);
  power->dcache2_bitline = array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache);
  power->dcache2_senseamp = senseamp_power(colsb);
  power->dcache2_tagarray = simple_array_power(trowsb,tcolsb,1,1,cache);

  power->dcache2_power = power->dcache2_decoder + power->dcache2_wordline + power->dcache2_bitline + power->dcache2_senseamp + power->dcache2_tagarray;

  power->rat_decoder *= crossover_scaling;
  power->rat_wordline *= crossover_scaling;
  power->rat_bitline *= crossover_scaling;

  power->dcl_compare *= crossover_scaling;
  power->dcl_pencode *= crossover_scaling;
  power->inst_decoder_power *= crossover_scaling;
  power->wakeup_tagdrive *= crossover_scaling;
  power->wakeup_tagmatch *= crossover_scaling;
  power->wakeup_ormatch *= crossover_scaling;

  power->selection *= crossover_scaling;

  power->regfile_decoder *= crossover_scaling;
  power->regfile_wordline *= crossover_scaling;
  power->regfile_bitline *= crossover_scaling;
  power->regfile_senseamp *= crossover_scaling;

  power->rs_decoder *= crossover_scaling;
  power->rs_wordline *= crossover_scaling;
  power->rs_bitline *= crossover_scaling;
  power->rs_senseamp *= crossover_scaling;

  power->lsq_wakeup_tagdrive *= crossover_scaling;
  power->lsq_wakeup_tagmatch *= crossover_scaling;

  power->lsq_rs_decoder *= crossover_scaling;
  power->lsq_rs_wordline *= crossover_scaling;
  power->lsq_rs_bitline *= crossover_scaling;
  power->lsq_rs_senseamp *= crossover_scaling;
 
  power->resultbus *= crossover_scaling;

  power->btb *= crossover_scaling;
  power->local_predict *= crossover_scaling;
  power->global_predict *= crossover_scaling;
  power->chooser *= crossover_scaling;

  power->dtlb *= crossover_scaling;

  power->itlb *= crossover_scaling;

  power->icache_decoder *= crossover_scaling;
  power->icache_wordline*= crossover_scaling;
  power->icache_bitline *= crossover_scaling;
  power->icache_senseamp*= crossover_scaling;
  power->icache_tagarray*= crossover_scaling;

  power->icache_power *= crossover_scaling;

  power->dcache_decoder *= crossover_scaling;
  power->dcache_wordline *= crossover_scaling;
  power->dcache_bitline *= crossover_scaling;
  power->dcache_senseamp *= crossover_scaling;
  power->dcache_tagarray *= crossover_scaling;

  power->dcache_power *= crossover_scaling;
  
  power->clock_power *= crossover_scaling;

  power->dcache2_decoder *= crossover_scaling;
  power->dcache2_wordline *= crossover_scaling;
  power->dcache2_bitline *= crossover_scaling;
  power->dcache2_senseamp *= crossover_scaling;
  power->dcache2_tagarray *= crossover_scaling;

  power->dcache2_power *= crossover_scaling;

  power->total_power = power->local_predict + power->global_predict + 
    power->chooser + power->btb +
    power->rat_decoder + power->rat_wordline + 
    power->rat_bitline + power->rat_senseamp + 
    power->dcl_compare + power->dcl_pencode + 
    power->inst_decoder_power +
    power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->selection +
    power->regfile_decoder + power->regfile_wordline + 
    power->regfile_bitline + power->regfile_senseamp +  
    power->rs_decoder + power->rs_wordline +
    power->rs_bitline + power->rs_senseamp + 
    power->lsq_wakeup_tagdrive + power->lsq_wakeup_tagmatch +
    power->lsq_rs_decoder + power->lsq_rs_wordline +
    power->lsq_rs_bitline + power->lsq_rs_senseamp +
    power->resultbus +
    power->clock_power +
    power->icache_power + 
    power->itlb + 
    power->dcache_power + 
    power->dtlb + 
    power->dcache2_power;

  power->total_power_nodcache2 =power->local_predict + power->global_predict + 
    power->chooser + power->btb +
    power->rat_decoder + power->rat_wordline + 
    power->rat_bitline + power->rat_senseamp + 
    power->dcl_compare + power->dcl_pencode + 
    power->inst_decoder_power +
    power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->selection +
    power->regfile_decoder + power->regfile_wordline + 
    power->regfile_bitline + power->regfile_senseamp +  
    power->rs_decoder + power->rs_wordline +
    power->rs_bitline + power->rs_senseamp + 
    power->lsq_wakeup_tagdrive + power->lsq_wakeup_tagmatch +
    power->lsq_rs_decoder + power->lsq_rs_wordline +
    power->lsq_rs_bitline + power->lsq_rs_senseamp +
    power->resultbus +
    power->clock_power +
    power->icache_power + 
    power->itlb + 
    power->dcache_power + 
    power->dtlb + 
    power->dcache2_power;

  power->bpred_power = power->btb + power->local_predict + power->global_predict + power->chooser + power->ras;

  power->rat_power = power->rat_decoder + 
    power->rat_wordline + power->rat_bitline + power->rat_senseamp;

  power->dcl_power = power->dcl_compare + power->dcl_pencode;

  power->rename_power = power->rat_power + 
    power->dcl_power + 
    power->inst_decoder_power;

  power->wakeup_power = power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->wakeup_ormatch;

  power->rs_power = power->rs_decoder + 
    power->rs_wordline + power->rs_bitline + power->rs_senseamp;

  power->rs_power_nobit = power->rs_decoder + 
    power->rs_wordline + power->rs_senseamp;

  power->window_power = power->wakeup_power + power->rs_power + 
    power->selection;

  power->lsq_rs_power = power->lsq_rs_decoder + 
    power->lsq_rs_wordline + power->lsq_rs_bitline + 
    power->lsq_rs_senseamp;

  power->lsq_rs_power_nobit = power->lsq_rs_decoder + 
    power->lsq_rs_wordline + power->lsq_rs_senseamp;
   
  power->lsq_wakeup_power = power->lsq_wakeup_tagdrive + power->lsq_wakeup_tagmatch;

  power->lsq_power = power->lsq_wakeup_power + power->lsq_rs_power;

  power->regfile_power = power->regfile_decoder + 
    power->regfile_wordline + power->regfile_bitline + 
    power->regfile_senseamp;

  power->regfile_power_nobit = power->regfile_decoder + 
    power->regfile_wordline + power->regfile_senseamp;

  non_clock_power = power->total_power - power->clock_power;

  dump_power_stats(power);

  regread_pipe = pipe_rec_init(2, 8, (4.3313 / 6) * 16, 0, TRUE, 
			       CONTROL_SCOPE_FUNC);
  regwrite_pipe = pipe_rec_init(2, 8, (4.3313 / 6) * 8 + 2.986145 * 2, 
				2, TRUE, CONTROL_SCOPE_FUNC_ONLY);

  iadd_pipe = pipe_rec_init(1, res_ialu, res_ialu * I_ADD, 
			    2, TRUE, CONTROL_SCOPE_FUNC_ONLY);
  imult_pipe = pipe_rec_init(3, res_imult, res_imult * I_ADD, 
			     2, TRUE, CONTROL_SCOPE_FUNC_ONLY);
  idiv_pipe = pipe_rec_init(20, 1, I_ADD, 
			    2, FALSE, CONTROL_SCOPE_FUNC_ONLY);
  fadd_pipe = pipe_rec_init(2, res_fpalu, res_fpalu * F_ADD, 
			    2, TRUE, CONTROL_SCOPE_FUNC_ONLY);
  fmult_pipe = pipe_rec_init(4, res_fpmult, res_fpmult * F_ADD, 
			     2, TRUE, CONTROL_SCOPE_FUNC_ONLY);
  fdiv_pipe = pipe_rec_init(24, 1,  F_ADD, 
			    2, FALSE, CONTROL_SCOPE_FUNC_ONLY);

  il1_pipe = pipe_rec_init(2, 2, 14.6217 + 2*0.485143 + 8.8168 / 2, 
			   0, TRUE, CONTROL_SCOPE_FRONT_ONLY);
  dl1_pipe = pipe_rec_init(2, res_memport, 14.6217 + 0.815619 + 10.8907, 
			   0, TRUE, CONTROL_SCOPE_MEM_ONLY);
  ul2_pipe = pipe_rec_init(20, 1, 30, 
			   0, TRUE, CONTROL_SCOPE_NONE);

  decode_pipe = pipe_rec_init(2, 1, 0.978139 * 2 + 5.24705, 
			      0, TRUE, CONTROL_SCOPE_FRONT_ONLY);
  commit_pipe = pipe_rec_init(1, 1, 5.24705,   
			      0, TRUE, CONTROL_SCOPE_NONE);


#define PART_POWER(t,p,s) ((t*(1-turnoff_factor)*((double)p/(double)s))+(t*turnoff_factor))

  /*
  fprintf(stderr, "il1:\t%5.2f\ndecode:\t%5.2f\nregread:\t%5.2f\n",
	  PART_POWER(pipe_rec_max(il1_pipe), 1, 2),
	  pipe_rec_max(decode_pipe),	  
	  pipe_rec_min(regread_pipe));
  fprintf(stderr, "func:\t%5.2f\ndl1:\t%5.2f\nul2:\t%5.2f\n",
	  pipe_rec_min(iadd_pipe) + 
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(idiv_pipe) +
	  pipe_rec_min(fadd_pipe) + 
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(fdiv_pipe),
	  pipe_rec_max(dl1_pipe),	  
	  pipe_rec_max(ul2_pipe));
  fprintf(stderr, "regwrite:\t%5.2f\ncommit:\t%5.2f\n",
	  PART_POWER(pipe_rec_max(regwrite_pipe), 4, 8),
	  pipe_rec_max(commit_pipe));
  */

  fprintf(stderr, "min_unconstrained_pipe_power = %4.2f\n", 
	  pipe_rec_min(il1_pipe) + 
	  pipe_rec_min(decode_pipe) + 
	  pipe_rec_min(regread_pipe) + 
	  pipe_rec_min(iadd_pipe) + 
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(idiv_pipe) +
	  pipe_rec_min(fadd_pipe) + 
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(fdiv_pipe) + 
	  pipe_rec_min(dl1_pipe) +
	  pipe_rec_min(ul2_pipe) +
	  pipe_rec_min(regwrite_pipe) +
	  pipe_rec_min(commit_pipe));

  fprintf(stderr, "max_unconstrained_pipe_power = %4.2f\n", 
	  pipe_rec_max(il1_pipe) + 
	  pipe_rec_max(decode_pipe) + 
	  pipe_rec_max(regread_pipe) + 
	  pipe_rec_max(iadd_pipe) + 
	  pipe_rec_max(imult_pipe) +
	  pipe_rec_max(idiv_pipe) +
	  pipe_rec_max(fadd_pipe) + 
	  pipe_rec_max(imult_pipe) +
	  pipe_rec_max(fdiv_pipe) + 
	  pipe_rec_max(dl1_pipe) +
	  pipe_rec_max(ul2_pipe) +
	  pipe_rec_max(regwrite_pipe) +
	  pipe_rec_max(commit_pipe));



  fprintf(stderr, "max_constrained_pipe_power = %4.2f\n",
	  PART_POWER(pipe_rec_max(il1_pipe), 1, 2) +
	  pipe_rec_max(decode_pipe) + 
	  pipe_rec_max(regread_pipe) + 
	  PART_POWER(pipe_rec_max(iadd_pipe), 5, 6) +
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(idiv_pipe) +
	  PART_POWER(pipe_rec_max(fadd_pipe), 2, 2) +
	  PART_POWER(pipe_rec_max(fmult_pipe), 1, 1) +
	  pipe_rec_min(fdiv_pipe) +
	  PART_POWER(pipe_rec_max(dl1_pipe), 4, 4) +
	  pipe_rec_min(ul2_pipe) +
	  pipe_rec_max(regwrite_pipe) +
	  pipe_rec_max(commit_pipe));

  fprintf(stderr, "min_func_control = %4.2f\n",
	  PART_POWER(pipe_rec_max(il1_pipe), 1, 2) +
	  pipe_rec_max(decode_pipe) +
	  pipe_rec_min(regread_pipe) + 
	  pipe_rec_min(iadd_pipe) + 
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(idiv_pipe) +
	  pipe_rec_min(fadd_pipe) + 
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(fdiv_pipe) + 
	  pipe_rec_max(dl1_pipe) +
	  pipe_rec_max(ul2_pipe) +
	  PART_POWER(pipe_rec_max(regwrite_pipe), 4, 8) +
	  pipe_rec_max(commit_pipe));

  fprintf(stderr, "max_func_control = %4.2f\n",
	  pipe_rec_min(il1_pipe) +
	  pipe_rec_min(decode_pipe) +
	  PART_POWER(pipe_rec_max(regread_pipe), 4, 8) + 
	  pipe_rec_max(iadd_pipe) + 
	  pipe_rec_max(imult_pipe) +
	  pipe_rec_max(idiv_pipe) +
	  pipe_rec_max(fadd_pipe) + 
	  pipe_rec_max(imult_pipe) +
	  pipe_rec_max(fdiv_pipe) + 
	  pipe_rec_min(dl1_pipe) +
	  pipe_rec_min(ul2_pipe) +
	  PART_POWER(pipe_rec_max(regwrite_pipe), 4, 8) +
	  pipe_rec_min(commit_pipe));

  fprintf(stderr, "min_exec_control = %4.2f\n",
	  PART_POWER(pipe_rec_max(il1_pipe), 1, 2) +
	  pipe_rec_max(decode_pipe) +
	  pipe_rec_min(regread_pipe) +
	  pipe_rec_min(iadd_pipe) + 
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(idiv_pipe) +
	  pipe_rec_min(fadd_pipe) + 
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(fdiv_pipe) + 
	  pipe_rec_min(dl1_pipe) +
	  pipe_rec_max(ul2_pipe) +
	  pipe_rec_min(regwrite_pipe) +
	  pipe_rec_min(commit_pipe));

  fprintf(stderr, "max_exec_control = %4.2f\n",
	  PART_POWER(pipe_rec_max(il1_pipe), 1, 2) +
	  pipe_rec_min(il1_pipe) +
	  pipe_rec_min(decode_pipe) +
	  pipe_rec_max(regread_pipe) +
	  pipe_rec_max(iadd_pipe) + 
	  pipe_rec_max(imult_pipe) +
	  pipe_rec_max(idiv_pipe) +
	  pipe_rec_max(fadd_pipe) + 
	  pipe_rec_max(imult_pipe) +
	  pipe_rec_max(fdiv_pipe) + 
	  pipe_rec_max(dl1_pipe) +
	  pipe_rec_min(ul2_pipe) +
	  pipe_rec_max(regwrite_pipe) + 
	  pipe_rec_min(commit_pipe));

  fprintf(stderr, "min_core_control = %4.2f\n",
	  pipe_rec_min(il1_pipe) +
	  pipe_rec_min(decode_pipe) +
	  pipe_rec_min(regread_pipe) +
	  pipe_rec_min(iadd_pipe) + 
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(idiv_pipe) +
	  pipe_rec_min(fadd_pipe) + 
	  pipe_rec_min(imult_pipe) +
	  pipe_rec_min(fdiv_pipe) + 
	  pipe_rec_min(dl1_pipe) +
	  pipe_rec_max(ul2_pipe) +
	  pipe_rec_min(regwrite_pipe) +
	  pipe_rec_min(commit_pipe));

  fprintf(stderr, "max_core_control = %4.2f\n",
	  pipe_rec_max(il1_pipe) +
	  pipe_rec_max(decode_pipe) +
	  pipe_rec_max(regread_pipe) +
	  pipe_rec_max(iadd_pipe) + 
	  pipe_rec_max(imult_pipe) +
	  pipe_rec_max(idiv_pipe) +
	  pipe_rec_max(fadd_pipe) + 
	  pipe_rec_max(imult_pipe) +
	  pipe_rec_max(fdiv_pipe) + 
	  pipe_rec_max(dl1_pipe) +
	  pipe_rec_min(ul2_pipe) +
	  pipe_rec_max(regwrite_pipe) + 
	  pipe_rec_min(commit_pipe));

  power_supply_init();

  if (power_spectra) 
    power_spectra_init();


  if (power_wavelet) {
    power_wavelet_init();
    event_tracker_init();
  }

}



void power_supply_init(void)
{
  int supply_index, position_index;
  int trigger_index;
  float temp_float;

  if (power_dc_offset < min_power) {
    power_dc_offset = min_power;
  }

#if defined(SWEEP)

#else 
   
  supply_arr = malloc(sizeof(supply_t) * supply_count);
  for(supply_index = 0; supply_index < supply_count; supply_index++) {
    supply_t *current_supply = supply_arr + supply_index;
    int block_count = DEFAULT_IMPULSE_LENGTH;
    int time_index;

    strncpy(current_supply->name, supply_filename[supply_index],
	    sizeof(current_supply->name));

    current_supply->current_supply_voltage = 
      current_supply->last_supply_voltage = Vdd_Sim;

    current_supply->impulse_length = 0;
    current_supply->impulse_response = malloc(block_count * sizeof(double));

    /* read the inputs from the file pds */
    while (fscanf(supply_stream[supply_index], "%f", &temp_float) == 1) {
      current_supply->impulse_response[current_supply->impulse_length] = temp_float;
      current_supply->impulse_length++;
      if (current_supply->impulse_length >= block_count) {
	block_count = block_count << 1;
	current_supply->impulse_response = 
	  realloc(current_supply->impulse_response, block_count * sizeof(double));
      }
    }
    current_length = MAX(current_length, current_supply->impulse_length);

    fprintf(stderr, "Loaded supply description \"%s\" of length %d\n",
	    current_supply->name, current_supply->impulse_length);
    dc_resistance = 0.0;
    for(time_index = 0; time_index < current_supply->impulse_length; time_index++)
      dc_resistance += current_supply->impulse_response[time_index];
    fprintf(stderr, "'%s' has DC resistance = %f\n",
	    current_supply->name, dc_resistance);
  }
#endif

  current_history = malloc(sizeof(double) * current_length);
  for(position_index = 0; position_index < current_length; position_index++)
    current_history[position_index] = 0.0;
  current_position = 0;

  for(position_index = 0; position_index < control_delay; position_index++)
    control_delay_history[position_index] = Vdd_Sim;

  if (trigger_count > 0)
    trigger_arr = malloc(sizeof(trigger_t) * trigger_count);

  for(trigger_index = 0; trigger_index < trigger_count; trigger_index++) {
    int history_position;

    if (sscanf(trigger_list[trigger_index], "%[^:]:%lf:%lf:%d:%lf", 
	       trigger_arr[trigger_index].name, // jdonald: looked like a serious bug with the extra & ampersand, removed
	       &trigger_arr[trigger_index].low,
	       &trigger_arr[trigger_index].high,
	       &trigger_arr[trigger_index].delay,
	       &trigger_arr[trigger_index].noise) != 5) 
      fatal("bad trigger parameters: %s\n");

    if (trigger_arr[trigger_index].noise == 0.0)
      trigger_arr[trigger_index].noise_factor = 0.0;
    else
      trigger_arr[trigger_index].noise_factor = 0.111111 * 
	trigger_arr[trigger_index].noise * trigger_arr[trigger_index].noise;

    trigger_arr[trigger_index].history_position = 0;
    for(history_position = 0; history_position < MAX_TRIGGER_HISTORY; 
	history_position++) 
      trigger_arr[trigger_index].trigger_history[history_position] = Vdd_Sim;
  }

  if (!strncmp(control_scope_type, "func", sizeof("func"))) {
    control_scope = CONTROL_SCOPE_FUNC;
  }
  else if (!strncmp(control_scope_type, "exec", sizeof("func"))) {
    control_scope = CONTROL_SCOPE_EXEC;
  }
  else if (!strncmp(control_scope_type, "core", sizeof("func"))) {
    control_scope = CONTROL_SCOPE_CORE;
  }
  else if (!strncmp(control_scope_type, "none", sizeof("func"))) {
    control_scope = CONTROL_SCOPE_NONE;
  }

  else {
    fatal("unrecognized control scope: %s\n", control_scope_type);
  }

      
}

#if 0
void compute_max_min_current(void)
{

 double instantaneous_current=(current_total_cycle_power_cc3-power_dc_offset)/Vdd_Sim;

 if(instantaneous_current > max_current){
   max_current= instantaneous_current;
   fprintf(stderr, "*** maximum current %5.3f at sim_cycle = %8lld ***\n",max_current, sim_cycle);
 }

 if(instantaneous_current < min_current){
   min_current= instantaneous_current;
   fprintf(stderr, "*** minimum current %5.3f at sim_cycle = %8lld ***\n",min_current, sim_cycle);
 }

}
#endif



void power_supply_compute(void)
{
  int supply_index, impulse_time_index, current_time_index;
  int trigger_index;
  static double invVdd = 1 / Vdd_Sim;
  
  if (supply_count == 0)
    return;

  /* get the instantaneous current */

  current_history[current_position] = (current_total_cycle_power_cc3-power_dc_offset) * invVdd;
  
  if(maxmin_current_cycle < 5){
    total_current=total_current+current_history[current_position];
    maxmin_current_cycle++;
  }
  else{
    min_current=MIN(min_current,(total_current)/5);
    max_current=MAX(max_current,(total_current)/5);
    total_current=0;
    maxmin_current_cycle=0;
  }

  for(supply_index = 0; supply_index < supply_count; supply_index++) {
    int voltage_index;
    supply_t *current_supply = supply_arr + supply_index;

    current_supply->current_supply_voltage = Vdd_Sim;
        
    /* perform the convolution */
    for(impulse_time_index = 0, current_time_index = current_position; 
	impulse_time_index < current_supply->impulse_length &&
	  current_time_index >= 0;
	impulse_time_index++, current_time_index--) 
      current_supply->current_supply_voltage -= current_history[current_time_index] * 
	current_supply->impulse_response[impulse_time_index];

    for(current_time_index = current_length - 1;
	impulse_time_index < current_supply->impulse_length;
	impulse_time_index++, current_time_index--)
      current_supply->current_supply_voltage -=
	current_history[current_time_index] * current_supply->impulse_response[impulse_time_index];

    if (current_supply->max_supply_voltage < current_supply->current_supply_voltage)
      current_supply->max_supply_voltage = current_supply->current_supply_voltage;

    if (current_supply->min_supply_voltage > current_supply->current_supply_voltage) {
      current_supply->min_supply_voltage = current_supply->current_supply_voltage;
      fprintf(stderr, "*** minimum voltage %5.3f at sim_cycle = %8lld ***\n", 
	      current_supply->min_supply_voltage, sim_cycle);
    }

    current_supply->sum_supply_voltage += current_supply->current_supply_voltage;
    current_supply->square_supply_voltage += current_supply->current_supply_voltage * 
      current_supply->current_supply_voltage;

    voltage_index = (current_supply->current_supply_voltage - nominal_supply_voltage) * 1000 + 100; 
    stat_add_sample( current_supply->voltage_dist, voltage_index);

    /* marks the hard faults */
    if (current_supply->current_supply_voltage < 0.95) {
      if (current_supply->low_fault_current_interval_length == 0)
	current_supply->low_fault_intervals++;
      current_supply->low_fault_current_interval_length++;
      current_supply->low_fault_cycles++;
    }
    else {
      if (current_supply->low_fault_current_interval_length)
	stat_add_sample( current_supply->low_fault_dist, current_supply->low_fault_current_interval_length);
      current_supply->low_fault_current_interval_length = 0;
    }

    if (current_supply->current_supply_voltage > 1.05) {
      if (current_supply->high_fault_current_interval_length == 0)
	current_supply->high_fault_current_interval_length++;
      current_supply->high_fault_current_interval_length++;
      current_supply->high_fault_cycles++;
    }
    else {
      if (current_supply->high_fault_current_interval_length)
	stat_add_sample( current_supply->high_fault_dist, current_supply->high_fault_current_interval_length);
      current_supply->high_fault_current_interval_length = 0;
    }

    
//    fprintf(itrace_stream,"%lf %lf %x\n",current_supply->current_supply_voltage,current_history[current_position],current_pc);

  }
  

  current_position = (current_position + 1) % current_length;

  if (control) {
    double sensor_reading = control_delay_history[control_delay_position];

    control_delay_history[control_delay_position] = supply_arr[0].current_supply_voltage;
    control_delay_position++;
    if (control_delay_position >= control_delay)
      control_delay_position = 0;

    if (control_delay == 0)
      sensor_reading = control_delay_history[0];

    control_freeze = control_fire = CONTROL_SCOPE_NONE;

    if (sensor_reading < control_low) {
      control_low_cycles++;
      control_low_length++;

      control_freeze = control_scope;
    }
    else {
      if (control_low_length)
	stat_add_sample( control_low_length_dist, control_low_length);
      control_low_length = 0;
    }

    if (sensor_reading > control_high) {
      control_high_cycles++;
      control_high_length++;

      control_fire = control_scope;
    }
    else {      if (control_high_length)
	stat_add_sample( control_high_length_dist, control_high_length);    
      control_high_length = 0;
    }


    if (control_step) {
#if 0
      fprintf(stderr, "%8lld %5.2f %5.3f %2s %2s %8s %2d\n",
	      sim_cycle, current_total_cycle_power_cc3,
	      supply_arr[0].current_supply_voltage,
	      (control_execute_stall) ? "LO" : " ",
	      (control_execute_fire) ? "HI" : " ",
	      (control_step_active) ? "ACTIVE" : " ",
	      control_step_count);
#endif
      if (!control_step_active && control_step_trigger) {
	control_step_intervals++;
	control_step_active = TRUE;
	control_step_count = 2;
	control_step_timer = 0;
      }
      else if (control_step_active) {
	control_step_timer++;
	if (control_step_timer == control_step_size) {
	  control_step_timer = 0;
	  control_step_count += 2;
	}
	if (control_step_count >= ruu_issue_width) {
	  control_step_active = FALSE;
	  control_step_timer = 0;
	  control_step_count = 0;
	}
      }
      control_step_trigger = FALSE;

    }
   
   if(voltage_count > 500000){
     if( (sensor_reading > control_high) || (sensor_reading < control_low) ) { 
     //  fprintf(stderr," exceeding thresholds\n");
         //fprintf(itrace_stream,  "%d) %lf\n", voltage_count, sensor_reading);
         //dump_instr_state(itrace_stream);
         //fprintf(itrace_stream,  "%d) %lf\n", voltage_count, sensor_reading);
     }
   }
   voltage_count++;



  }

  stat_add_sample(power_cycle_dist, (int) current_total_cycle_power_cc3);

  /*
  fprintf(stderr, "%10lld\t%5.2f\t%5.3f\t%1s %1s  || ",
	  sim_cycle, current_total_cycle_power_cc3, supply_arr[0].current_supply_voltage,
	  (control_freeze) ? "Z" : "-", 
	  (control_fire) ? "F" : "-");
  fprintf(stderr, "\t%4.1f %4.1f %4.1f %4.1f %4.1f %4.1f\n",
	  il1_power, dl1_power, ul2_power, reg_power, fu_power, clk_power);
  */

  for(trigger_index = 0; trigger_index < trigger_count; trigger_index++) {
    double trigger_reading;
    trigger_t *current_trigger = trigger_arr + trigger_index;
    
    current_trigger->trigger_history[current_trigger->history_position] =
      supply_arr[0].current_supply_voltage;

    trigger_reading = 
      current_trigger->trigger_history[(current_trigger->history_position + 
					MAX_TRIGGER_HISTORY - 
					current_trigger->delay) 
				       % MAX_TRIGGER_HISTORY];
    if (current_trigger->noise != 0.0){
      trigger_reading += randn() * current_trigger->noise_factor;
      printf("current_trigger->noise != 0.0\n");
    }

    /*
    fprintf(stderr, "%f\t%f  [%f %f]\n",
	    supply_arr[0].current_supply_voltage,
	    trigger_reading,
	    current_trigger->low, current_trigger->high);
    */

    if (trigger_reading < current_trigger->low) {
      current_trigger->low_trigger_cycles++;
      if (!current_trigger->low_active)
	current_trigger->low_trigger_events++;
      current_trigger->low_active = TRUE;
    }
    else 
      current_trigger->low_active = FALSE;


    if (trigger_reading > current_trigger->high) {
      current_trigger->high_trigger_cycles++;
      if (!current_trigger->high_active)
	current_trigger->high_trigger_events++;
      current_trigger->high_active = TRUE;
    }
    else 
      current_trigger->high_active = FALSE;

    current_trigger->history_position = 
      (current_trigger->history_position + 1)  % MAX_TRIGGER_HISTORY;
  }
}

void power_supply_reg_stats(struct stat_sdb_t *sdb)
{
  int supply_index;
  int trigger_index;

  for(supply_index = 0; supply_index < supply_count; supply_index++) {
    char sum_name[255], square_name[255];
    // char var_name[255], var_formula[255]; // jdonald unused
    // char avg_formula[255], avg_name[255]; // jdonald unused
    char min_name[255], max_name[255], dist_name[255];
    char low_name[255], high_name[255];
    char low_intervals[255], high_intervals[255];
    char low_cycles[255], high_cycles[255];
    supply_t *current_supply = supply_arr + supply_index;

    sprintf(sum_name, "%s.sum_supply_voltage", current_supply->name);
    sprintf(square_name, "%s.square_supply_voltage", current_supply->name);
    sprintf(min_name, "%s.min_supply_voltage", current_supply->name);
    sprintf(max_name, "%s.max_supply_voltage", current_supply->name);
    sprintf(dist_name, "%s.voltage_dist", current_supply->name);
    sprintf(low_name, "%s.low_fault_dist", current_supply->name);
    sprintf(high_name, "%s.high_fault_dist", current_supply->name);
    sprintf(low_cycles, "%s.low_fault_cycles", current_supply->name);
    sprintf(high_cycles, "%s.high_fault_cycles", current_supply->name);
    sprintf(low_intervals, "%s.low_fault_intervals", current_supply->name);
    sprintf(high_intervals, "%s.high_fault_intervals", current_supply->name);
    

    stat_reg_double(sdb, sum_name, "sum of supply voltage", 
		    &current_supply->sum_supply_voltage, /* default*/0.0, NULL);
    stat_reg_double(sdb, square_name, "sum of squared supply voltage", 
		    &current_supply->square_supply_voltage, /* default*/0.0, NULL);
    stat_reg_double(sdb, min_name, "minimum supply voltage", 
		    &current_supply->min_supply_voltage, /* default*/10e6, NULL);
    stat_reg_double(sdb, max_name, "maximum supply voltage", 
		    &current_supply->max_supply_voltage, /* default*/-10e6, NULL);

    stat_reg_counter(sdb, low_cycles, "low fault cycles",
		     &current_supply->low_fault_cycles, /* default */0, NULL);
    stat_reg_counter(sdb, high_cycles, "high fault cycles",
		     &current_supply->high_fault_cycles, /* default */0, NULL);

    stat_reg_counter(sdb, low_intervals, "low fault intervals",
		     &current_supply->low_fault_intervals, /* default */0, NULL);
    stat_reg_counter(sdb, high_intervals, "high fault intervals",
		     &current_supply->high_fault_intervals, /* default */0, NULL);


    current_supply->voltage_dist = stat_reg_dist(sdb, dist_name, "distribution of supply voltage",
                               /* initial */0, /* array size */200, /* bucket size */1,
                               (PF_COUNT|PF_PDF), NULL, NULL, NULL);

    current_supply->low_fault_dist = stat_reg_dist(sdb, low_name, "distribution of faults",
                               /* initial */0, /* array size */25, /* bucket size */1,
                               (PF_COUNT|PF_PDF), NULL, NULL, NULL);

    current_supply->high_fault_dist = stat_reg_dist(sdb, high_name, "distribution of faults",
                               /* initial */0, /* array size */25, /* bucket size */1,
                               (PF_COUNT|PF_PDF), NULL, NULL, NULL);

    current_supply->low_fault_current_interval_length = current_supply->high_fault_current_interval_length = 0;
  }

  for(trigger_index = 0; trigger_index < trigger_count; trigger_index++) {
    char low_events[32], high_events[32];
    char low_cycles[32], high_cycles[32];

    sprintf(low_events, "%s.trigger_low_events", trigger_arr[trigger_index].name);
    sprintf(high_events, "%s.trigger_high_events", trigger_arr[trigger_index].name);
    sprintf(low_cycles, "%s.trigger_low_cycles", trigger_arr[trigger_index].name);
    sprintf(high_cycles, "%s.trigger_high_cycles", trigger_arr[trigger_index].name);

    stat_reg_counter(sdb, low_events, 
		     "low trigger events",
		     &trigger_arr[trigger_index].low_trigger_events,
		     /* default */0, NULL);
    stat_reg_counter(sdb, high_events, 
		     "high trigger events",
		     &trigger_arr[trigger_index].high_trigger_events,
		     /* default */0, NULL);

    stat_reg_counter(sdb, low_cycles, 
		     "low trigger cycles",
		     &trigger_arr[trigger_index].low_trigger_cycles,
		     /* default */0, NULL);
    stat_reg_counter(sdb, high_cycles, 
		     "high trigger cycles",
		     &trigger_arr[trigger_index].high_trigger_cycles,
		     /* default */0, NULL);
  }
}

/* seems that this is not called anywhere */
double power_supply_convolve(int offset, int width)
{
  int time_index, imp_index;
  double result = 0.0;
  
  for(time_index = 0; time_index < width; time_index++) {
    imp_index = offset + time_index;

    if (imp_index >= 0 && imp_index < supply_arr[0].impulse_length)
      result += supply_arr[0].impulse_response[imp_index];
  }

  return result;
}

void power_spectra_init(void)
{
  power_spectra_index = 0;
  power_spectra_history = malloc(sizeof(float) * power_spectra_length);

  power_spectra_re = malloc(sizeof(float) * power_spectra_length);
  power_spectra_im = malloc(sizeof(float) * power_spectra_length);

  power_spectra_aggregate_value = 
    malloc(sizeof(double)*(power_spectra_length/2 + 1));

  power_spectra_extreme_value = 
    malloc(sizeof(double)*(power_spectra_length/2 + 1));
}

void power_spectra_reg_stats(struct stat_sdb_t *sdb)
{
  int index;
  char name[64];

  stat_reg_counter(sdb, "power_spectra_aggregate_count",
		   "number of spectra samples taken for aggregate",
		   &power_spectra_aggregate_count,
		    /* default */0, NULL);

  for(index = 0; index < (power_spectra_length/2 + 1); index++) {
    sprintf(name, "power_spectra_aggregate_value[%d]", index);
    stat_reg_double(sdb, name, "power spectra aggregate",
		    power_spectra_aggregate_value + index, 
		    /* default */0.0, NULL);
  }

  stat_reg_counter(sdb, "power_spectra_extreme_count",
		   "number of spectra samples taken for extreme",
		   &power_spectra_extreme_count,
		    /* default */0, NULL);

  for(index = 0; index < (power_spectra_length/2 + 1); index++) {
    sprintf(name, "power_spectra_extreme_value[%d]", index);
    stat_reg_double(sdb, name, "power spectra extreme",
		    power_spectra_extreme_value + index, 
		    /* default */0.0, NULL);
  }
}

void power_spectra_compute(void)
{
  power_spectra_history[power_spectra_index] = current_total_cycle_power_cc3;
  if (power_spectra_index == (power_spectra_length-1)) {
    int freq_index;
    double scale_factor = 1.0 / power_spectra_length;
    int stop_freq = (2.0e2 / 3.0e3) * power_spectra_length;
    double spec_density = 0.0;

    power_spectra_index = 0;

/* POWER  don't need this as of now*/
#if 0
    fft_float(power_spectra_length, 0, power_spectra_history, NULL,
	      power_spectra_re, power_spectra_im);
#endif

#if 0
    fprintf(stdout, "<--- Frequency Block %d at (%lld)--->\n", power_spectra_count, sim_cycle);
    for(freq_index = 0; freq_index < power_spectra_length / 2; freq_index++) {
      fprintf(stdout, "%8.0f\t%6.4f\n",
	      freq_index * scale_factor * 3.0e9,
	      scale_factor * (freq_index ? 2 : 1) * 
	      sqrt(power_spectra_re[freq_index]*power_spectra_re[freq_index] +
		   power_spectra_im[freq_index]*power_spectra_im[freq_index]));
    }
#endif

    for(freq_index = 0; freq_index <= power_spectra_length / 2; freq_index++) {
      double mag_sq = 
	power_spectra_re[freq_index]*power_spectra_re[freq_index] +
	power_spectra_im[freq_index]*power_spectra_im[freq_index];

      if (freq_index != 0 && freq_index <= stop_freq)
	spec_density += scale_factor * mag_sq;

      power_spectra_aggregate_value[freq_index] += scale_factor * 
	sqrt(mag_sq);

    }
    power_spectra_aggregate_count++;


#if 0
    fprintf(stderr, "%f\t(%d)\n", spec_density, stop_freq);
#endif

    if (0) {
      int time_index;
      
      fprintf(stderr, "Cycle Count = %lld\n", 
	      sim_cycle - power_spectra_length);
      for(time_index = 0; time_index < power_spectra_length; time_index++)
	fprintf(stderr, "%8d %8.2f\n", time_index, // jdonald: changed %8ld to %8d. it's just an int after all
		power_spectra_history[time_index]);
    }

    if (spec_density > power_spectra_cutoff) {
      power_spectra_extreme_count++;
      for(freq_index = 0; freq_index <= power_spectra_length / 2; freq_index++) {      
	double mag_sq = 
	  power_spectra_re[freq_index]*power_spectra_re[freq_index] +
	  power_spectra_im[freq_index]*power_spectra_im[freq_index];

	power_spectra_extreme_value[freq_index] += scale_factor * 
	  sqrt(mag_sq);
      }
    }
  }
  else 
    power_spectra_index++;

}

struct pipe_rec* pipe_rec_init(int depth, int width, double energy, 
			       int buffer, int pipelined, int controlled)
{
  struct pipe_rec *pipe;

  pipe = calloc(1, sizeof(struct pipe_rec) + (buffer+depth) * sizeof(int));
  pipe->pipestage = (int *) (((char *) pipe) + sizeof(struct pipe_rec));
  pipe->depth = depth;
  pipe->width = width;
  pipe->position = 0;
  pipe->energy = energy;
  pipe->buffer = buffer;
  pipe->pipelined = pipelined;
  pipe->controlled = controlled;

  return pipe;
}

void pipe_rec_activate(struct pipe_rec *pipe, int act)
{
  int insert_index = pipe->position + pipe->buffer;
  if (insert_index >= (pipe->depth + pipe->buffer))
    insert_index -= pipe->depth + pipe->buffer;

  pipe->pipestage[insert_index] = (act < pipe->width) ? act : pipe->width;
}

void pipe_rec_inc(struct pipe_rec *pipe)
{
  pipe->pipestage[pipe->position]++;
}

void pipe_rec_dec(struct pipe_rec *pipe)
{
  pipe->pipestage[pipe->position]--;
}

void pipe_rec_update(struct pipe_rec *pipe)
{
  if (!(control_freeze & pipe->controlled)) {
    int insert_index;

    pipe->position = (pipe->position + 1) % (pipe->depth + pipe->buffer);

    insert_index = pipe->position + pipe->buffer;
    if (insert_index >= (pipe->depth + pipe->buffer))
      insert_index -= pipe->depth + pipe->buffer;
    
    pipe->pipestage[insert_index] = 0;
  }
}

double pipe_rec_calc(struct pipe_rec *pipe)
{
  int index, count = 0;

  if (control_freeze & pipe->controlled) {
 //   printf("pipe_rec_calc Freeze \n");
    return turnoff_factor * pipe->energy;
  }

  if (control_fire & pipe->controlled){
//    printf("pipe_rec_calc Fire \n");
    return pipe->energy;
  }

  for(index = 0; index < pipe->depth; index++)
    count += pipe->pipestage[(index+pipe->position) % (pipe->depth+pipe->buffer)];

 return ((1-turnoff_factor)*(((double)count)/(pipe->width*pipe->depth)) + turnoff_factor) * pipe->energy;
}

double pipe_rec_max(struct pipe_rec *pipe)
{
  return (pipe->pipelined) ? pipe->energy : pipe->energy / ((double)pipe->depth);
}

double pipe_rec_min(struct pipe_rec *pipe)
{
  return ((turnoff_factor)*pipe->energy);
}

int pipe_rec_peek(struct pipe_rec *pipe)
{
  return pipe->pipestage[(pipe->position+pipe->depth-1) % pipe->depth];
}

int pipe_rec_population(struct pipe_rec *pipe)
{
  int index, count = 0;
  
  for(index = 0; index < pipe->depth; index++)
    count += pipe->pipestage[(index+pipe->position) % (pipe->depth+pipe->buffer)];

  return count;
}

void pipe_dump(void)
{
  double current_total;

  current_total = pipe_rec_calc(regread_pipe) + pipe_rec_calc(regwrite_pipe) +
    pipe_rec_calc(iadd_pipe) + pipe_rec_calc(imult_pipe) +
    pipe_rec_calc(idiv_pipe) + pipe_rec_calc(fadd_pipe) +
    pipe_rec_calc(fmult_pipe) + pipe_rec_calc(fdiv_pipe) +
    pipe_rec_calc(il1_pipe) + pipe_rec_calc(dl1_pipe) +
    pipe_rec_calc(ul2_pipe) + 
    pipe_rec_calc(decode_pipe) + pipe_rec_calc(commit_pipe);


  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "il1", pipe_rec_min(il1_pipe), pipe_rec_max(il1_pipe), pipe_rec_calc(il1_pipe));
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "decode", pipe_rec_min(decode_pipe), pipe_rec_max(decode_pipe), pipe_rec_calc(decode_pipe));
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "regread", pipe_rec_min(regread_pipe), pipe_rec_max(regread_pipe), pipe_rec_calc(regread_pipe));
  fprintf(stderr, "\n");
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "iadd", pipe_rec_min(iadd_pipe), pipe_rec_max(iadd_pipe), pipe_rec_calc(iadd_pipe));
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "imult", pipe_rec_min(imult_pipe), pipe_rec_max(imult_pipe), pipe_rec_calc(imult_pipe));
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "idiv", pipe_rec_min(idiv_pipe), pipe_rec_max(idiv_pipe), pipe_rec_calc(idiv_pipe));
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "fadd", pipe_rec_min(fadd_pipe), pipe_rec_max(fadd_pipe), pipe_rec_calc(fadd_pipe));
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "fmult", pipe_rec_min(fmult_pipe), pipe_rec_max(fmult_pipe), pipe_rec_calc(fmult_pipe));
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "fdiv", pipe_rec_min(fdiv_pipe), pipe_rec_max(fdiv_pipe), pipe_rec_calc(fdiv_pipe));
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "dl1", pipe_rec_min(dl1_pipe), pipe_rec_max(dl1_pipe), pipe_rec_calc(dl1_pipe));
  fprintf(stderr, "\n");
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "ul2", pipe_rec_min(ul2_pipe), pipe_rec_max(ul2_pipe), pipe_rec_calc(ul2_pipe));
  fprintf(stderr, "\n");

  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "regwrite", pipe_rec_min(regwrite_pipe), pipe_rec_max(regwrite_pipe), pipe_rec_calc(regwrite_pipe));
  fprintf(stderr, "%16s%8.2f%8.2f%8.2f\n", "commit", pipe_rec_min(commit_pipe), pipe_rec_max(commit_pipe), pipe_rec_calc(commit_pipe));
}

double power_wavelet_stat_thld[] = {
  0.0,
  6.116664,
  10.448049,
  18.349546,
  32.585290,
  45.390453,
  62.841538,
  86.660613,
  92.619893
};

void power_wavelet_init(void)
{
  int index;

  power_wavelet_length = 1 << power_wavelet_level;
  power_wavelet_history = malloc(sizeof(double) * power_wavelet_length);
  for(index = 0; index < power_wavelet_length; index++)
    power_wavelet_history[index] = 0.0;

  wavelet_temp = malloc(sizeof(double) * power_wavelet_length);

  power_wavelet_stat_large = 
    malloc(sizeof(double) * (1 + power_wavelet_level));
}

void power_wavelet_reg_stats(struct stat_sdb_t *sdb)
{
  int scale_index;
  char sum_name[255], sq_name[255], samples_name[255];
  char large_name[255];
  
  power_wavelet_mag_sum = malloc(sizeof(double) * (power_wavelet_level+1));
  power_wavelet_mag_sq = malloc(sizeof(double) * (power_wavelet_level+1));
  power_wavelet_mag_samples = malloc(sizeof(double) * (power_wavelet_level+1));

  power_wavelet_stat_large = malloc(sizeof(double) * (power_wavelet_level+1));

  for(scale_index = 1; scale_index <= power_wavelet_level; scale_index++) {
    sprintf(sum_name, "power_wavelet_mag_sum[%d]", scale_index);
    stat_reg_double(sdb, sum_name, "wavelet mag sum", 
		    power_wavelet_mag_sum + scale_index, 
		    /* default */0.0, NULL);
  }

  for(scale_index = 1; scale_index <= power_wavelet_level; scale_index++) {
    sprintf(sq_name, "power_wavelet_mag_sq[%d]", scale_index);
    stat_reg_double(sdb, sq_name, "wavelet mag sq", 
		    power_wavelet_mag_sq + scale_index, 
		    /* default */0.0, NULL);
  }

  for(scale_index = 1; scale_index <= power_wavelet_level; scale_index++) {
    sprintf(samples_name, "power_wavelet_mag_samples[%d]", scale_index);
    stat_reg_counter(sdb, samples_name, "wavelet mag samples", 
		    power_wavelet_mag_samples + scale_index, 
		    /* default */0, NULL);
  }

  for(scale_index = 1; scale_index <= power_wavelet_level; scale_index++) {
    sprintf(large_name, "power_wavelet_stat_large[%d]", scale_index);
    stat_reg_counter(sdb, large_name, "wavelet stat large coeffs",
		    power_wavelet_stat_large + scale_index, 
		    /* default */0, NULL);
  }
}



void power_wavelet_compute(void)
{
  power_wavelet_history[power_wavelet_position] = 
    current_total_cycle_power_cc3;

  power_wavelet_position++;
  event_tracker_update();

  if (power_wavelet_position >= power_wavelet_length) {
    int scale_index, time_index;
    // int level_index; // jdonald unused
    double *detail;
    // double *approx; // jdonald unused
    
    /* MS: fwt(power_wavelet_history, wavelet_temp, 
	power_wavelet_length, power_wavelet_level);
	*/

    for(scale_index = 1; scale_index <= power_wavelet_level; scale_index++) {
      /* MS: 
      detail = wavedec(wavelet_temp, power_wavelet_length, scale_index);
      */

      for(time_index = 0; 
          time_index < (power_wavelet_length >> scale_index);
          time_index++) { 
	double mag = fabs(detail[time_index]);

	power_wavelet_mag_sum[scale_index] += mag;
	power_wavelet_mag_sq[scale_index] += mag * mag;
	power_wavelet_mag_samples[scale_index]++;

	if (mag > power_wavelet_stat_thld[scale_index]) {
	  int event_index; 
	  int start_time;
	  power_wavelet_stat_large[scale_index]++;
	  
	  start_time = sim_cycle - power_wavelet_length +
	    time_index * (1 << scale_index);

	  for(event_index = 0; event_index < EVENT_TRACKER_NUM_EVENTS; event_index++) {

	    if (event_tracker_find(event_index, start_time, 5))
	      power_wavelet_stat_large_event_near[scale_index][event_index]++;

	    if (event_tracker_find(event_index, start_time, 10))
	      power_wavelet_stat_large_event_med[scale_index][event_index]++;
	    
	    if (event_tracker_find(event_index, start_time,  20))
	      power_wavelet_stat_large_event_far[scale_index][event_index]++;
	  }
	}
      }
    }

    power_wavelet_position = 0;
  }
}


void event_tracker_init(void)
{
  int event_index;

  current_event_count = malloc(sizeof(int) * EVENT_TRACKER_NUM_EVENTS);
  power_wavelet_event_total = malloc(sizeof(counter_t) * EVENT_TRACKER_NUM_EVENTS);
  event_tracker_arr = malloc(sizeof(int) * EVENT_TRACKER_NUM_EVENTS);
  power_wavelet_stat_large_event_near = malloc(sizeof(counter_t*) * EVENT_TRACKER_NUM_EVENTS);
  power_wavelet_stat_large_event_med = malloc(sizeof(counter_t*) * EVENT_TRACKER_NUM_EVENTS);
  power_wavelet_stat_large_event_far = malloc(sizeof(counter_t*) * EVENT_TRACKER_NUM_EVENTS);

  for(event_index = 0; event_index < EVENT_TRACKER_NUM_EVENTS; 
      event_index++){
    current_event_count[event_index] = 0;

    event_tracker_arr[event_index] = 
      calloc(sizeof(int), 2 * power_wavelet_length);

    power_wavelet_stat_large_event_near[event_index] = malloc(sizeof(counter_t) * 
					    (1+power_wavelet_level));
    power_wavelet_stat_large_event_med[event_index] = malloc(sizeof(counter_t) * 
					    (1+power_wavelet_level));
    power_wavelet_stat_large_event_far[event_index] = malloc(sizeof(counter_t) * 
					    (1+power_wavelet_level));
  }
  
  event_tracker_position = 0;
}

void event_tracker_reg_stats(struct stat_sdb_t *sdb)
{
  int scale_index, event_index;
  char total[255], near[255], med[255], far[255];

  for(event_index = 0; event_index < EVENT_TRACKER_NUM_EVENTS;
      event_index++) {
    sprintf(total, "power_wavelet_%s_total", event_tracker_name[event_index]);
    stat_reg_counter(sdb, total, "wavelet stat large coeffs", 
		     &power_wavelet_event_total[event_index],  /* default */0, NULL);

    for(scale_index = 1; scale_index <= power_wavelet_level; 
	scale_index++) {
      sprintf(near, "power_wavelet_stat_large_%s_near[%d]", 
	      event_tracker_name[event_index], scale_index);

      stat_reg_counter(sdb, near, "wavelet stat large coeffs", 
		       &power_wavelet_stat_large_event_near[event_index][scale_index],
		       /* default */0, NULL);      
    }

    for(scale_index = 1; scale_index <= power_wavelet_level; 
	scale_index++) {
      sprintf(med, "power_wavelet_stat_large_%s_med[%d]", 
	      event_tracker_name[event_index], scale_index);

      stat_reg_counter(sdb, med, "wavelet stat large coeffs", 
		       &power_wavelet_stat_large_event_med[event_index][scale_index],
		       /* default */0, NULL);      
    }

    for(scale_index = 1; scale_index <= power_wavelet_level; 
	scale_index++) {
      sprintf(far, "power_wavelet_stat_large_%s_far[%d]", 
	      event_tracker_name[event_index], scale_index);

      stat_reg_counter(sdb, far, "wavelet stat large coeffs", 
		       &power_wavelet_stat_large_event_far[event_index][scale_index],
		       /* default */0, NULL);      
    }
    
  }
}

void event_tracker_update(void)
{
  int event_index;

  for(event_index = 0; event_index < EVENT_TRACKER_NUM_EVENTS; event_index++) {
    event_tracker_arr[event_index][event_tracker_position] +=
      current_event_count[event_index];

    power_wavelet_event_total[event_index] += current_event_count[event_index];
    
    current_event_count[event_index] = 0;
  }

  event_tracker_position++;
  if (event_tracker_position >= event_tracker_length)
    event_tracker_position = 0;
}

int event_tracker_find(int event, counter_t when, int distance)
{
  int time_index;
  int lookup_index, found = 0;

  for(time_index = 0; time_index < distance; time_index++) {
    lookup_index = event_tracker_position  - 
      ((sim_cycle - power_wavelet_length) - when);
    lookup_index = (lookup_index < 0) ? lookup_index + 2 * power_wavelet_length :
      lookup_index;

    found += event_tracker_arr[event][lookup_index];
  }
  
  return found;
}


