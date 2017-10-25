#include <signal.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include "interactive-types.h"

//char *strdup(const char *s1);

const tword TAGGED_FALSE = {{0}, TAG_BOOL}, // jdonald made const
      TAGGED_TRUE  = {{1}, TAG_BOOL},
      NIL          = {{0}, TAG_SYM},
      TAGGED_ZERO  = {{0}, TAG_INT},
      TAGGED_ONE   = {{1}, TAG_INT},
      UNBOUND      = {{0}, TAG_UNBOUND},
      NOVALUE      = {{0}, TAG_NOVALUE};

tword make_tagged_word(Tag tag, TInt value)
{ tword result;

  result.tag      = tag;
  result.data.val = value;
  return result;
}

tword make_tagged_wordv(Tag tag, void *ptr)
{ tword result;

  result.tag       = tag;
  result.data.vptr = ptr;
  return result;
}

tword make_tagged_sym(TInt value) 
{ return make_tagged_word(TAG_SYM, value); }

tword make_tagged_int(TInt value) 
{ tword result;
  
  result.tag      = TAG_INT;
  result.data.val = value;
  return result;
}

tword make_tagged_bool(TInt value)
{ return make_tagged_word(TAG_BOOL, value); }


tword make_tagged_vector(tword num_words)
{ tword result, slot = make_tagged_word(TAG_UNBOUND, 0), *ptr;
  int i, length = num_words.data.val + 1;

  if ((ptr = (tword *) malloc(length * sizeof(tword))) == NULL) 
    {
      printf("Unable to allocate %d words for make_vector\n", length);
      Exit(-1);
    }

  ptr[0] = make_tagged_int(length);
  for (i = 1; i < length; i = i + 1) ptr[i] = slot;

  result.tag       = TAG_VECTOR;
  result.data.tptr = ptr;

  return result;
}

tword make_tagged_float(float fval)
{ tword result;

  result.tag       = TAG_FLOAT;
  result.data.fval = fval;
  return result;
}

tword free_tagged_vector(tword vector)
{ 
  free(vector.data.tptr);
  return TAGGED_TRUE;
}

tword tagged_vector_size(tword vector)
{ return make_tagged_int((* (vector.data.tptr)).data.val); }

tword make_tagged_string(char *cstring)
{ return make_tagged_wordv(TAG_STRING, cstring); }

tword make_tagged_stringa(char *cstring)
{ char *buffer = strdup(cstring);

  if ((buffer = strdup(cstring)) == NULL) {
    printf("MakeString: Unable to allocate memory to copy the input string\n");
    return NIL;
  }

  return make_tagged_wordv(TAG_STRING, buffer);
}

tword free_tagged_string(tword string)
{ 
  free(string.data.vptr);
  return TAGGED_TRUE;
}

tword tagged_string_length(tword string)
{ return make_tagged_int(strlen((char *) string.data.vptr)); }

/********************************************************************************
*										*
* Mike Noakes           Handle Interrupts and Cleanup              Jan 27, 1993 *
*                                                                               *
* We wish to catch all interrupts and handle them in a controlled way.  We will *
* generally wish to catch fatal interrupts and perform application dependent    *
* cleanup before exiting.  This code maintains a table of exit handlers that    *
* can be installed during initialization.  An exit handler is passed an exit    *
* status and should not return a value.                                         *
*                                                                               *
* By default ^C is also regarded as a fatal interrupt.  However there are       *
* critical codes in the system that cannot be interrupted.  These regions can   *
* request to delay ^C handling but must indicate when they are finished.        *
* Interactive systems will most likely wish to use ^C as in internal reset and  *
* may chose to make ^C a non-fatal interrupt.  If this is done then the system  *
* must provide a function of no-arguments to call at the end of ^C handling.    *
*                                                                               *
********************************************************************************/

static void HandleCntlcInterrupts();
static void do_Sigint(int ignore);
static void do_FatalSighup(int ignore);
static void do_FatalSigsegv(int ignore);
static void do_FatalSigbus(int ignore);
static void do_FatalSignal(int signum, char *signame);

/* Make sure we clean up the terminal if we are kicked out */
void InitializeInterrupts()
{
  /* Initialize the Fatal interrupts */
#ifdef UNIX
  signal(SIGHUP,  do_FatalSighup);
  signal(SIGSEGV, do_FatalSigsegv);
  signal(SIGBUS,  do_FatalSigbus);
  signal(SIGINT,  do_Sigint);
#endif
}

/* Support a set of exit handlers that can be registered by the application */
#define MAX_EXIT_HANDLERS 16

static void (*ExitFunc[MAX_EXIT_HANDLERS])();
static int ExitHandlers = 0;

void add_exit_handler(void (*func)(int status))
{
  if (ExitHandlers == MAX_EXIT_HANDLERS) {
    printf("Unable to add more exit handler\n");
    Exit(-1);
  }

  ExitFunc[ExitHandlers++] = func;
}

void Exit(int status)
{ int i;

  for (i = 0; i < ExitHandlers; i++) ExitFunc[i](status);

  exit(status);
}

/* Support a set of ^C handlers and permit them to be delayed */
#define MAX_CNTLC_HANDLERS 16

static void (*CntlcFunc[MAX_CNTLC_HANDLERS])();
static int CntlcHandlers          = 0;

static int  CntlcInterruptsFatal   = 1;
static void (*CntlcContinue)();
static int  CntlcInterruptsPending = 0;
static int  CntlcInterruptsDelayed = 0;

void add_cntlc_handler(void (*func)(int status))
{
  if (CntlcHandlers == MAX_CNTLC_HANDLERS) {
    printf("Unable to add more cntlc handler\n");
    Exit(-1);
  }

  CntlcFunc[CntlcHandlers++] = func;
}

void delay_cntlc()
{
  CntlcInterruptsDelayed++;
}

void undelay_cntlc()
{
  if (CntlcInterruptsDelayed == 0) {
    printf("undelay_cntlc called with none delayed\n");
    Exit(-2);
  }

  if (--CntlcInterruptsDelayed == 0)
    if (CntlcInterruptsPending > 0)
      HandleCntlcInterrupts();
}

void cntlc_continue_handler(void (*func)())
{
  CntlcContinue        = func;
  CntlcInterruptsFatal = 0;
}

static void HandleCntlcInterrupts()
{ int i;

  for (i = 0; i < CntlcHandlers; i++) CntlcFunc[i](0);
  if (CntlcInterruptsFatal == 1)
    Exit(-1);
  else
    CntlcContinue();
}

/* Handle a ^C interrupt */
static void do_Sigint(int ignore)
{
  if (CntlcInterruptsDelayed > 0)
    CntlcInterruptsPending++;
  else 
    HandleCntlcInterrupts();

#ifdef UNIX
  signal(SIGINT,  do_Sigint);
#endif
}

static void do_FatalSighup(int ignore)
{ do_FatalSignal(SIGHUP, "sighup");   }

static void do_FatalSigsegv(int ignore)
{ do_FatalSignal(SIGSEGV, "sigsegv"); }

static void do_FatalSigbus(int ignore)
{ do_FatalSignal(SIGBUS, "sigbus");   }

static void do_FatalSignal(int signum, char *signame)
{
  /* Avoid nasty recursive signals */
#ifdef UNIX
  signal(signum,  SIG_DFL);
#endif
 
  fflush(stdout);
  fprintf(stderr,"\nReceived fatal %s signal, cleaning up.\n", signame);
  fflush(stderr);
  Exit(1);
}

/********************************************************************************
*										*
* A useful default ^C behavior is to simply set a flag when a ^C is signalled   *
* by the user and then poll this flag in the main loop of the program.  The     *
* attraction of this is that is simple to use and avoids atomicity problems.    *
*                                                                               *
* These set of functions are layered on the general utilities to provide this   *
* support.                                                                      *
*                                                                               *
********************************************************************************/

static int  CntlcInterruptsSignalled = 0;

void CntlcPollingHandler()
{ 
  CntlcInterruptsSignalled = 1; 
}

/* Return 1 if a ^C has been signalled and clear the flag */
int CntlcPollIsWaiting()
{ int value = CntlcInterruptsSignalled;

  CntlcInterruptsSignalled = 0;
  return value;
}

void CntlcInstallPollingHandler()
{
  cntlc_continue_handler(CntlcPollingHandler);
}

/*----------------------------------------------------------------------------
 * Fatal error related to a system call.  Print a message and terminate.
 * Don't dump core, but do print the system's errno value and its associated
 * message.
 *    err_sys(fmt, arg1, arg2, ....)
 *----------------------------------------------------------------------------*/
static char *sys_err_str();
static char emesgstr[255] = {0};
static void my_perror();

void err_sys(char *fmt, ...)
{ va_list ap;
  
  va_start(ap, fmt);
  vsprintf(emesgstr, fmt, ap);
  va_end(ap);

  my_perror();

  Exit(1);
}

static void my_perror()
{ int len = strlen(emesgstr);

  sprintf(emesgstr + len, " %s", sys_err_str());
}

// extern int sys_nerr;		/* # of error messages in system table */
#ifdef linux
// extern __const char *__const _sys_errlist[];
#else 
// extern char *sys_errlist[];	/* The system error message table */
#endif

static char *sys_err_str()
{
  static char msgstr[200];

  if (errno != 0) {
    if (errno > 0)
      sprintf(msgstr, "(%s)", strerror(errno));
    else
      sprintf(msgstr, "(errno = %d)", errno);
  }
  else
    msgstr[0] = '\0';

  return msgstr;
}
