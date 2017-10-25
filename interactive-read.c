#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef DOS
#include <bios.h>
#endif


#ifdef UNIX
#include <errno.h>
#endif

#include "interactive-types.h"
#include "interactive-read.h"

#define OB_SIZE 26

static void  InitializeTcshProcessing();
static void  CleanupTcshProcessing();
static void  CleanupObArray();  /* clean up sym table */

//char *strdup(const char *s1);

symtab *ReadBase;
symtab *PrintBase;
symtab *All;



/* The symbol table */
static symtab *ObArray[OB_SIZE];

static primop *PrimopHead = NULL,          /* The head of all primops created */
              *PrimopTail = NULL;          /* The tail of all primops created */

typedef struct { 
          char *filename; 
          int  lineno;
          FILE *fp;
	} InputStream;

#define MAX_INPUT_SP  32
static InputStream InputStack[MAX_INPUT_SP];
static int InputSP = -1;

int GetReadBase()
{ return ReadBase->value.data.val; }

void SetReadBase(int value)
{ ReadBase->value.data.val = value; }

int GetPrintBase()
{ return PrintBase->value.data.val; }

void SetPrintBase(int value)
{ PrintBase->value.data.val = value; }


void ReaderInit()
{ int i;

  for (i = 0; i < OB_SIZE; i = i + 1) ObArray[i] = (symtab *) NULL;

  ReadBase = Intern(MakeSymbol("read-base"));
  ReadBase->value = make_tagged_int(10);

  PrintBase = Intern(MakeSymbol("print-base"));
  PrintBase->value = make_tagged_int(10);


  All       = Intern(MakeSymbol("all"));

  MarkCommands("Builtins");

  AddPrimop("help &opt <category-or-command>",
	    cmd_help,
	    "Provide help on a category of commands or a command",
	    PRIMOP_NO_PRINT | PRIMOP_SPECIAL_FORM,
	    make_tagged_int(TAG_SYMBOL),
	    UNBOUND);
	    
  AddPrimop("include <filename>",
	    cmd_include,
	    "Read commands from the specified file",
	    PRIMOP_NO_PRINT,
	    make_tagged_int(TAG_STRING));
	    
  AddPrimop("quit",
	    cmd_quit,
	    "Exit the interpreter",
	    PRIMOP_NO_PRINT);

  AddPrimop("setq <sym> <value>",
	    cmd_setq,
	    "Assign the indicated symbol to the given value",
	    PRIMOP_NO_PRINT | PRIMOP_SPECIAL_FORM,
	    make_tagged_int(TAG_SYMBOL),
	    make_tagged_int(TAG_ANY),
	    UNBOUND);
	    
  InitializeTcshProcessing();
}

tword cmd_help(tsymbol cat_or_cmd)
{ primop *ptr;

  /* If there wasn't an argument then list all the categories */
  if (cat_or_cmd.tag == TAG_UNBOUND) {
    printf("Type \"help all\"        to see the description of all commands\n");
    printf("Type \"help <category>\" to list commands in a category\n");
    printf("Type \"help <command>\"  for information on given command\n");
    printf("Categories:\n");

    for (ptr = PrimopHead; ptr != NULL; ptr = ptr->next)
      if (!PrimopIsFunc(ptr))
	printf("  %s\n", ptr->usage);
  }

  /* The user typed "help all" or "help <cmd>" or "help <cmd>" */
  else if (cat_or_cmd.tag == TAG_SYMBOL) {
    symtab *sym = (symtab *) cat_or_cmd.data.vptr;

    /* Check for "help all" */
    if (sym == All)
      for (ptr = PrimopHead; ptr != NULL; ptr = ptr->next)
	if (!PrimopIsFunc(ptr)) {
	  int len = strlen(ptr->usage), start = (18 - len) / 2, i;

	  printf("\n******");
	  for (i = start; i > 0; i--) printf(" ");
	  printf("%s", ptr->usage);
	  for (i = 18 - start - len; i > 0; i--) printf(" ");
	  printf("******\n");
	}
	else
	  printf("  %s\n", ptr->usage);
    /* Check for "help <cmd>" */
    else if (sym->value.tag == TAG_PRIMOP) {
      primop *prim = (primop *) sym->value.data.vptr;

      printf("  %s\n", prim->usage);
      printf("  %s\n", prim->descr);
    }
    /* Check for "help <category>" */
    else {
      for (ptr = PrimopHead; ptr != NULL; ptr = ptr->next)
	if (!PrimopIsFunc(ptr) && ptr->name == sym)
	  break;

      if (ptr) {
	int len = strlen(ptr->usage), start = (18 - len) / 2, i;

	printf("******");
	for (i = start; i > 0; i--) printf(" ");
	printf("%s", ptr->usage);
	for (i = 18 - start - len; i > 0; i--) printf(" ");
	printf("******\n");

	for (ptr = ptr->next; ptr && PrimopIsFunc(ptr); ptr = ptr->next)
	  printf("  %s\n", ptr->usage);
      }
      else
	printf("\"%s\" does not have predefined help\n", sym->pname);
    }
  }
  /* Unexpected arg type to help */
  else
    printf("Unexpected argument to help\n");

  return NIL;
}

tword cmd_include(tstring filename)
{ char *fname = filename.data.cptr, *cfname;
  FILE *fp;

  while ((fp = fopen(fname, "r")) == NULL)
    if (errno == EINTR)
      continue;
    else {
      printf("include: Unable to open file \"%s\"\n", fname);
      return NIL;
    }

  if (InputSP == MAX_INPUT_SP) {
    printf("include: Too many nested files\n");
    return NIL;
  }

  if ((cfname = (char *) malloc(strlen(fname) + 1)) == NULL) {
    printf("include: Unable to copy the filename\n");
    Exit(-1);
  }

  InputSP++;
  InputStack[InputSP].filename = cfname;
  InputStack[InputSP].lineno   =      1;
  InputStack[InputSP].fp       =     fp;

  return NIL;
}

tword cmd_quit()
{ 
  Exit(0);
  return TAGGED_TRUE;  /* This is sleaze to beat big-lameness of compiler */
}

tword cmd_setq(tsymbol symbol, tword slot)
{ tword value = Eval(slot);

  ((symtab *) symbol.data.vptr)->value = value;

  return value;
}

symtab *MakeSymbol(char *pname)
{ symtab *ptr;

/*      printf("Unable to allocate %ld bytes for a symbol table entry\n",*/

  if ((ptr = (symtab *) malloc(sizeof(symtab))) == NULL) {
      printf("Unable to allocate %ld bytes for a symbol table entry\n",
	     (long) sizeof(symtab)); // jdonald
    Exit(-1);
  }

  ptr->next  = NULL;
  ptr->value = UNBOUND;
  ptr->pname = strdup(pname);

  /*    printf("Unable to allocate %ld bytes for a symbol print name in MakeSymbol\n",*/

  if (ptr->pname == NULL) {
    printf("Unable to allocate %ld bytes for a symbol print name in MakeSymbol\n",
	   (long) strlen(pname) + 1); // jdonald long
    Exit(-1);
  }

  return ptr;
}

symtab *Intern(symtab *ptr)
{ symtab **head = &(ObArray[ptr->pname[0] % OB_SIZE]);

  ptr->next = *head;
  *head = ptr;
  return ptr;
}

symtab *FindSymbol(char *pname)
{ symtab *ptr;

  for (ptr = ObArray[pname[0] % OB_SIZE]; ptr != NULL; ptr = ptr->next)
    if (strcmp(pname, ptr->pname) == 0)
      return ptr;

  return Intern(MakeSymbol(pname));
}

/* A primop can be a "real" primop or just a category heading */
int PrimopIsFunc(primop *ptr)
{ return (ptr->func && ptr->descr); }

/* A marker can be placed at the start of any group of commands for "help" */
primop *MarkCommands(char *heading)
{ int length;
  primop *ptr;

  /* Allocate a new primop structure */
  length = sizeof(primop);
  if ((ptr = (primop *) malloc(length)) == NULL) {
    printf("Unable to allocate %d bytes for Command Marker\n", length);
    Exit(-1);
  }

  ptr->next  = NULL;
  ptr->name  = FindSymbol(heading);
  ptr->func  = NULL;
  ptr->usage = heading;
  ptr->descr = NULL;
  ptr->flags = 0; // jdonald

  /* Add the new primop to the list of all primops */
  if (PrimopHead == NULL) PrimopHead = ptr;
  if (PrimopTail != NULL) PrimopTail->next = ptr;
  PrimopTail = ptr;

  /* Return the value */
  return ptr;
}

primop *AddPrimop(char *usage, tword (*func)(), char *descr, int spec, ...)
{ int required = 0,
      optional = 0,
      restp    = 0,
      np = PrimopFlagsNoPrint(spec),
      sf = PrimopFlagsSpecialForm(spec),
      da = PrimopFlagsDoAlloc(spec),
      length,
      i;
  primop *ptr;
  char    name[64], *fptr, *tptr;
  va_list ap;

  /* Generate the name from the usage */
  for (fptr = usage, tptr = name; *fptr && *fptr != ' '; fptr++, tptr++)
    *tptr = *fptr;
  *tptr = '\0';

  /* Count the required and optional counts from the usage */
  { int count = 0, optsp = 0, restp = 0;

    for ( ; *fptr != '\0'; fptr++)
      if (*fptr == '<') 
	count++;
      else if (*fptr == '&') {
	if (strncmp(fptr, "&opt", 4) == 0) {
	  required = count;
	  count    = 0;
	  optsp    = 1;
	}
	else if (strncmp(fptr, "&rest", 5) == 0) {
	  if (optsp == 0)
	    required = count;
	  else
	    optional = count;

	  restp = 1;

	  break;
	}
	else {
	  printf("AddPrimop(%s).  Unexpected & escape\n", name);
	  return NULL;
	}
      }

    if (optsp == 0)
      required = count;
    else
      optional = count;
  }

  /* Allocate a new primop */
  length = sizeof(primop) + (required + 2 * optional - 1) * sizeof(tword);
  if ((ptr = (primop *) malloc(length)) == NULL) {
    printf("Unable to allocate %d bytes for AddPrimop\n", length);
    Exit(-1);
  }

  ptr->next  = NULL;
  ptr->name  = FindSymbol(name);
  ptr->func  = func;
  ptr->usage = usage;
  ptr->descr = descr;
  ptr->flags = PrimopMakeFlags(da, np, sf, restp, optional, required);

  va_start(ap, spec);

  for (i = 0; i < required + 2 * optional; i = i + 1)
    ptr->values[i] = va_arg(ap, tword);

  va_end(ap);

  /* Add the new primop to the list of all primops */
  if (PrimopHead == NULL) PrimopHead = ptr;
  if (PrimopTail != NULL) PrimopTail->next = ptr;
  PrimopTail = ptr;

  /* Make the symbol for this function point to this primop */
  (ptr->name)->value = make_tagged_wordv(TAG_PRIMOP, ptr);
    
  /* Return the value */
  return ptr;
}
  
/********************************************************************************
*										*
*                        NEW  Read Eval Print Loop                              *
*      										*
* This code is similar in style to the classic Lisp Read-Eval-Print loop.       *
*										*
*   1) Read a line of text                                                      *
*   2) Evaluate each token and apply the command to the args                    *
*   3) Print the result                                                         *
*										*
* This supports a new simplified command line interface.                        *
*										*
********************************************************************************/

#define EVAL_STACK_SIZE   64
#define INBUF_SIZE      1024

static tword EvalStack[EVAL_STACK_SIZE];
static char  InputBuffer[INBUF_SIZE];

static void ReadLine(char *prompt, char *buffer);

void ReadEvalPrintLoop(char *prompt)
{ while (1) ReadEvalPrint(prompt); }

void ReadEvalPrint(char *prompt)
{ int   sp = 0, i, j;
  char *ptr;
  tword result;

  /* Read a single line */
  ReadLine(prompt, InputBuffer);

  /* Parse the line into a set of atoms on the stack */
  for (sp = 0, ptr = InputBuffer; ptr; sp++)
    EvalStack[sp] = InteractiveRead(&ptr);

  /* Evaluate the first argument */
  result = Eval(EvalStack[0]);

  /* If the first value is a primop */
  if (result.tag == TAG_PRIMOP) {
    primop *pptr = (primop *) result.data.vptr;
    int num_args = sp - 1,
        num_req  = PrimopNumRequired(pptr),
        num_opt  = PrimopNumOptionals(pptr),
        num_par  = num_req + num_opt,
        restp    = PrimopRestp(pptr);

    if (num_req > num_args) {
      printf("Primop \"%s\" requires %d args but was passed %d\n",
	     (pptr->name)->pname,
	     num_req,
	     num_args);
      return;
    }
    else if (num_args > num_par && !restp) {
      printf("Primop \"%s\" takes up to %d args but was passed %d\n",
	     (pptr->name)->pname,
	     num_par,
	     num_args);
      return;
    }

    /* If it's not a special form then evaluate the delivered args */
    if (!PrimopSpecialForm(pptr))
      for (i = 1; i < sp; i = i + 1)
	EvalStack[i] = Eval(EvalStack[i]);

    /* Provide defaults for all the optionals */
    for (i = sp, j = num_opt + sp - 1; i <= num_req + num_opt; i++, j++)
      EvalStack[i] = pptr->values[j];
  
    /* Apply the function to the args */
    result = pptr->func(EvalStack[1], 
			EvalStack[2], 
			EvalStack[3], 
			EvalStack[4], 
			EvalStack[5],
			EvalStack[6],
			EvalStack[7],
			EvalStack[8],
			EvalStack[9]);

    if (PrimopNoPrint(pptr))
      result = NOVALUE;
  }
  else if (sp > 1)
    printf("Unexpected values after non-function\n");

  Print(result);
  if (result.tag != TAG_NOVALUE) printf("\n");
}

#define MAX_TOKEN_SIZE 256
static char ReadToken[MAX_TOKEN_SIZE];

tword InteractiveRead(char **startptr)
{ char *fptr, *tptr;
  TInt  base         = ReadBase->value.data.val,
        tag          = 1;

  TInt  intval       = 0;
  int   colonp       = 0,
        isint        = 1,
        isfloat      = 0,
        minusp       = 0,
        expon_flag   = 0,
        expon_minusp = 0,
        stringp      = 0;
  float fval         = 0.0,
        fraction     = 0.1;

  /* Read leading whitespace */
  for (fptr = *startptr; *fptr && *fptr == ' '; fptr++);

  /* Perhaps there is no token */
  if (*fptr == '\0') {
    *startptr = '\0';
    return NOVALUE;
  }

  /* Check for a leading minus sign */
  if (*fptr == '-') {
    minusp = 1;
    fptr++;
  }

  /* Read characters until end of string or a space or a comment char */
  for (tptr = ReadToken; *fptr && *fptr != ' ' && *fptr != ';' && *fptr != '\n'; 
       fptr++) {
    char c = *fptr;

    *tptr++ = c;

    if (c == '$') 
      base = 16;
    else if (c == '0' && *(fptr + 1) == 'x') {
      base = 16;
      fptr++;
    }
    else if (c == '"')
      if (stringp) {
	*(tptr - 1) = '\0';
	fptr++;
	break;
      }
      else {
	tptr--;
	stringp = 1;
	isint   = 0;
      }
    else if (c == ':')
      if (isint && !colonp)
	if (minusp) {
	  printf("Tag can't be negative\n");
	  tag    = TAG_SYM;
	  intval =       0;
	  break;
	}
	else if (intval > 15) {
	  printf("Tag must be in the range 0 .. 15.  Setting to 0\n");
	  tag    = TAG_SYM;
	  intval =       0;
	  break;
	}
	else if (*(fptr + 1) == '\0') {
	  printf("Tagged pointer must have a value field\n");
	  tag    = TAG_SYM;
	  intval =       0;
	  break;
	}
	else {
	  colonp =      1;
	  tag    = intval;
	  intval =      0;
	  if (*(fptr + 1) == '-') {
	    minusp = 1;
	    fptr++;
	  }
	}
      else
	isint = 0;
    else if (isint)
      if (c >= '0' && c <= '9')
	intval = intval * base + (c - '0');
      else if (base == 16 && (c >= 'a' && c <= 'f'))
	intval = intval * base + (10 + c - 'a');
      else if (base == 16 && (c >= 'A' && c <= 'F'))
	intval = intval * base + (10 + c - 'A');
      else if (c == '.')
	{
	  isint      = 0;
	  isfloat    = 1;
	  fval       = intval;
	  expon_flag = 0;
	  intval     = 0;
	}
      else if (c == 'e' || c == 'E')
	{
	  isint      = 0;
	  isfloat    = 1;
	  fval       = intval;
	  expon_flag = 1;
	  intval     = 0;
	}
      else
	isint = 0;
    else if (isfloat) {
      if (expon_flag == 0)
	{
	  if (c >= '0' && c <= '9')
	    {
	      fval = fval + (c - '0') * fraction;
	      fraction = fraction * 0.1;
	    }
	  else if (c == 'e' || c == 'E')
	    {
	      expon_flag = 1;
	      intval     = 0;
	    }
	  else
	    isfloat = 0;
	}
      else if (expon_flag == 1 && (c == '+' || c == '-'))
	{ 
	  expon_flag   = 2;
	  expon_minusp = (c == '-');
	}
      else if (c >= '0' && c <= '9') 
	{
	  expon_flag = 2;
	  intval     = intval * 10 + c - '0';
	}
      else
	isfloat = 0;
    }
  }

  *tptr = '\0';

  /* Read trailing whitespace */
  for ( ; *fptr && *fptr == ' '; fptr++);

  /* If current character is a ';' then read to end of line */
  if (*fptr == ';') for (fptr++; *fptr; fptr++);

  /* Check for empty string */
  *startptr = (*fptr == '\0') ? NULL : fptr;

  { 

    if (isint)
      return make_tagged_word(tag, (minusp) ? intval * -1 : intval);
    else if (stringp)
      return make_tagged_stringa(ReadToken);
    else if (isfloat && (expon_flag == 0 || expon_flag == 2))
      { float scale = 1.0;

	if (expon_flag == 2)
	  {
	    if (expon_minusp == 0)
	      {
		while (intval > 100) 
		  {
		    scale  = scale * 1.0e100;
		    intval = intval - 100;
		  }
		while (intval > 10)
		  {
		    scale  = scale * 1.0e10;
		    intval = intval -  10;
		  }
		while (intval > 0)
		  {
		    scale  = scale * 10.0;
		    intval = intval -   1;
		  }
	      }
	    else
	      {
		while (intval > 100) 
		  {
		    scale  = scale / 1.0e100;
		    intval = intval - 100;
		  }
		while (intval > 10)
		  {
		    scale  = scale / 1.0e10;
		    intval = intval -  10;
		  }
		while (intval > 0)
		  {
		    scale  = scale / 10.0;
		    intval = intval -   1;
		  }
	      }
	  }

	return make_tagged_float(scale * ((minusp) ? fval * -1 : fval));
      }
    else
      return make_tagged_wordv(TAG_SYMBOL, FindSymbol(ReadToken));
  }
}

/* Everything but a symbol is self-evaluating */
tword Eval(tword value)
{
  if (value.tag == TAG_SYMBOL)
    return ((symtab *) value.data.vptr)->value;
  else
    return value;
}

/* Print standard things */
tword Print(tword value)
{ 
  if (value.tag == TAG_SYM)
    printf((value.data.val == 0) ? "NIL" : "SYM:$%08x", (unsigned) value.data.val); // jdonald unsigned
  else if (value.tag == TAG_INT) {
    printf((PrintBase->value.data.val == 10) ? "%d\n" : "0x%x\n", (int) value.data.val); // jdonald int
  }
  else if (value.tag == TAG_BOOL)
    printf((value.data.val == 0) ? "FALSE" : "TRUE");
  else if (value.tag == TAG_FLOAT)
    printf("%f", value.data.fval);
  else if (value.tag == TAG_STRING)
    printf("\"%s\"", value.data.cptr);
  else if (value.tag == TAG_UNBOUND)
    printf("#Unbound");
  else if (value.tag == TAG_NOVALUE)
    ;
  else
    printf("$%x:$%08x", (int)value.tag, (int)value.data.val);

  return value;
}

/********************************************************************************
*										*
*                       EMACS-like Command Line editing                         *
*      										*
* This code is based on code provided by Randy Sargent.				*
*										*
********************************************************************************/

#define MAXLEN                1000
#define MAX_COMMAND_LINE_SIZE  256
#define MAXHISTORY              50

FILE *cmd_in;
FILE *cmd_out;
char  command_line[MAX_COMMAND_LINE_SIZE],
      cmd_old_line[MAXLEN], 
     *cmd_history[MAXHISTORY],
     *cmd_line = (char *) NULL,
     *cmd_prompt;
int   cmd_line_pos        = 0,
      cmd_no_reverse_wrap = 0,
      cmd_ansi            = 1,
      cmd_current         = 0,
      cmd_pos             = 0, 
      cmd_old_pos         = 0,
      cmd_char_pos        = 0;

static int TERM_is_emacs  = 0;

static void ResetCmdLine();

#define cmd_char_len(c) (((c) & 127) < 32 ? 2 : 1)

static void InitializeTcshProcessing()
{ int i;

#ifdef UNIX
  TERM_is_emacs = !strcmp((char *) getenv("TERM"), "emacs");
#endif

  /* If not emacs then make stdin be non-buffered */
  if (!TERM_is_emacs) {
#ifdef UNIX
      if (isatty(STDIN_FILENO)) {
	/* system("/usr/ucb/stty cbreak -echo"); */
      }
#endif
  }

  /* Clear the current line */
  ResetCmdLine();

  /* Clear the command line history */
  for (i = 0; i < MAXHISTORY; i++) cmd_history[i] = (char *) NULL;

  /* Add an exit handler */
  add_exit_handler(CleanupTcshProcessing);
  add_exit_handler(CleanupObArray);  /* clean up sym table */
}

static void ResetCmdLine()
{
  command_line[0] = '\0';
  cmd_line_pos    =   0;
}

static void CleanupTcshProcessing(void)
{
#if 0
  int stdin_fd = 0, old_fl;

  fcntl(stdin_fd, F_SETOWN, getpid());
  old_fl = fcntl(stdin_fd, F_GETFL);
  fcntl(stdin_fd, F_SETFL, old_fl & ~FASYNC);
#endif

#ifdef UNIX
/*  if (isatty(STDIN_FILENO))
    system("/usr/bin/stty cooked echo -nl"); */
#endif
}



static void CleanupObArray(void)  /* clean up sym table */
{
  int i;
  symtab *sym, *next_sym;
  primop *prim, *next_prim;
  for(i = 0; i < OB_SIZE; i++) {
    sym = ObArray[i];
    while(sym != NULL) {
      next_sym = sym->next;
      free((char*)sym->pname);
      free((char*)sym);
      sym = next_sym;
    }
  }
  prim = PrimopHead;
  while(prim != NULL) {
    next_prim = prim->next;
    free((char*)prim);    
    prim = next_prim;
  }
}

/************************************************************************
*    									*
*	      The core of the command line reader 			*
*									*
************************************************************************/

static char ReadChar(FILE *fp);
static void cmd_get_line(char *pin, FILE *in_str, FILE *out_str, char *res_buf);
static void cmd_putc_repeat(int c, int n);
static void cmd_put_char(int c);
static void cmd_update_line(int match_parens);
static int  cmd_string_len(char *s);
static void cmd_beep(void);

#ifdef DOS
static int io_getchar(int fd, int *c);
#endif

static void ReadLine(char *prompt, char *ptr)
{ char c, *start = ptr;
  FILE *in = (InputSP == -1) ? stdin : InputStack[InputSP].fp;

  if (InputSP == -1) printf("%s", prompt);

  for (c = ReadChar(in); c != '\n' && c != (char)EOF; c = ReadChar(in))
    *ptr++ = c;

  *ptr = '\0';

  /* If reading from a file stream then handle EOF condition */
  if (InputSP >= 0) {
    if (c == (char)EOF) {
      while (fclose(InputStack[InputSP].fp) == EOF)
	if (errno == EINTR)
	  continue;
	else {
	  printf("Unable to close \"%s\"\n", InputStack[InputSP].filename);
	  break;
	}	  
	
      free(InputStack[InputSP].filename);
      InputSP--;

      /* If nothing read from the stream then read from previous one */
      if (start == ptr) ReadLine(prompt, ptr);
    }
    else {
      InputStack[InputSP].lineno++;
    }
  }
}

/*
  Input is taken from either a file or from the emacs-like command-line editor.
  The line editor is not-used if we are running under emacs so as to avoid 
  screen turds.
*/
static char ReadChar(FILE *fp) 
{
  /* Read from "terminal" */

  if (fp == stdin) {
    if (TERM_is_emacs) { 
      return fgetc(fp);
    }
    else {
      char next_char = command_line[cmd_line_pos++];

      if (next_char) {
	return next_char;
      }
      else {
	command_line[0] = '\0';
	cmd_line_pos = 0;
	cmd_get_line("", fp, stdout, command_line);
	strcat(command_line, "\n");
	printf("\n");
	return ReadChar(fp);
      }
    }
  }
  else {
    return fgetc(fp);
  }
}

/* 
  This function reads a line of input from in_stream and saves the result
  into the buffer result_buf.  The user may enter tcsh-history commands
  to recapture/edit previous commands.  Redisplay occurs on out_stream as
  required.
*/

#define Cntl(c) ((c) -  64)
#define Meta(c) ((c) + 128)
#define ESC     (27)

#include <termio.h>
#include <termios.h>
#include <sys/types.h>
#include <fcntl.h>

/* This should not normally be called directly */
static void cmd_get_line(char *pin, FILE* in_str, FILE *out_str, char *res_buf)
{ static FILE *in;
  static FILE *out;
  static int  did_command, c, oldc, newc, i, repeat, cmd_new, iter;
  static int  meta, cursor, quote;
  static char *prompt, *l, *line;
  static unsigned char lc;

  prompt          = pin;
  in              = in_str;
  out             = out_str;
  line            = res_buf;
  cmd_new         = cmd_current;

  cmd_prompt      = prompt;
  cmd_char_pos    = strlen(cmd_prompt);
	
  cmd_in          = in;  
  cmd_out         = out;
  cmd_pos         = 0;
  cmd_old_pos     = 0;
  cmd_line        = "";
  cmd_old_line[0] = '\0';
  cmd_line        = line; 
  cmd_pos         = strlen(line);
  cmd_update_line(1);
  repeat          = 1;
  meta            = 0;
  cursor          = 0;
  quote           = 0;

  printf("%s", prompt);
  fflush(stdout);

  if (isatty(STDIN_FILENO)) {
#ifdef __svr4__
    system("/usr/bin/stty -icanon -echo min 1 time 0");
#endif
  }

  while (1) {
    /* Fetch the next character */
#if DOS
    while (io_getchar(in, &c) == 0);
#else
    fread(&lc, sizeof(unsigned char), 1, in);
    c = lc;
#endif

    if (c == EOF) {
      if (isatty(STDIN_FILENO)) {
#ifdef __svr4__
	system("/usr/bin/stty icanon");
#endif
      }
      return ;
    }

    /* Meta-<char> may also be specified as <ESC> <char> */
    if (meta) {
      c |= 128;
      meta = 0;
    }
    
    if (cursor) {
      switch(c) {
      case 'A': c = 16; break;  /* up    arrow */
      case 'B': c = 14; break;  /* down  arrow */
      case 'C': c =  6; break;  /* right arrow */
      case 'D': c =  2; break;  /* left  arrow */
      }
      cursor = 0;
    }
    
    /* Save the current state */
    strcpy(cmd_old_line, cmd_line);
    cmd_old_pos = cmd_pos;
    did_command = 1;
    
    for (iter = 0; iter < repeat; iter ++) {
      /* If last character was ^Q then store next one verbatim */
      if (quote) {
	quote = 0; 
	goto normal_char;
      }
      
      switch(c) {
	
      case ESC:   /* esc */
	meta = 1;
	did_command = 0;
	break;
	
      case Meta('['):
	cursor = 1;
	did_command = 0;
	break;
      case Meta('d'):
      case Meta('D'):
	for (i = cmd_pos; cmd_line[i] && !isalnum(cmd_line[i]); i++);
	for (; cmd_line[i] && isalnum(cmd_line[i]); i++);
	strcpy(&cmd_line[cmd_pos], &cmd_line[i]);
	break;
      case Meta(  8):
      case Meta(127):
	for (i = cmd_pos - 1; i >= 0 && !isalnum(cmd_line[i]); i--);
	for (; i >= 0 && isalnum(cmd_line[i]); i--);
	strcpy(&cmd_line[i + 1], &cmd_line[cmd_pos]);
	cmd_pos = i + 1;
	break;
	
      case '\n': /* return */
      case '\r': /* return */

        /* Make sure paren highlighting is turned off before returning string */
	cmd_pos = strlen(cmd_line);
	cmd_update_line(0);
	
	if (cmd_pos) {
	  /* If the next cmd_history entry is in use then free it */
	  if (cmd_history[cmd_current]) free(cmd_history[cmd_current]);
	  
	  /* Copy the new command line into a freshly allocated slot */
	  cmd_history[cmd_current] = malloc(strlen(cmd_line) + 1);
	  strcpy(cmd_history[cmd_current], cmd_line);
	  
	  /* Increment the history counter */
	  cmd_current = (cmd_current + 1) % MAXHISTORY;
	}
	if (isatty(STDIN_FILENO)) {
#ifdef __svr4__
	  system("/usr/bin/stty icanon");
#endif
	}
	return;
	
	
      case Cntl('A'):
	cmd_pos = 0;
	break;
      case Cntl('B'):
	if (cmd_pos) cmd_pos--; else goto error;
	break;
#ifdef DOS
      case Cntl('C'):
	raise(SIGINT);
	break;
#endif
      case Cntl('D'):
	if (cmd_line[cmd_pos])
	  strcpy(&cmd_line[cmd_pos], &cmd_line[cmd_pos + 1]);
	else 
	  goto error;
	break;
      case Cntl('F'):
	if (cmd_line[cmd_pos]) cmd_pos++;
	else goto error;
	break;
      case Cntl('H'):
      case 127:         /* del */
	if (cmd_pos) {
	  cmd_pos--;
	  strcpy(&cmd_line[cmd_pos], &cmd_line[cmd_pos + 1]);
	} 
	else 
	  goto error;
	break;
      case Cntl('K'):
	cmd_line[cmd_pos] = 0;
	break;
      case Cntl('Q'):
	quote = 1;
	did_command = 0;
	break;
      case Cntl('U'):
	repeat *= 4;
	did_command = 0;
	break;
	
	/* Be careful here.  There are some fall through cases */
      case '!':
	if (cmd_pos == 1 && cmd_line[0] == '!' && !cmd_line[1]) c = 16;
	else goto normal_char;
      case Cntl('N'):
      case Cntl('P'):
	cmd_new = (cmd_new + MAXHISTORY + 15 - c) % MAXHISTORY;
	if (!cmd_history[cmd_new]) {
	  cmd_new = (cmd_new - 15 + c) % MAXHISTORY;
	  goto error;
	}
      set_line:
	strcpy(cmd_line, cmd_history[cmd_new]);
	/* FALL THROUGH TO END OF LINE */
      case Cntl('E'):
	cmd_pos = strlen(cmd_line);
	break;
	
      case ' ':
	if (cmd_pos &&
	    cmd_line[0] == '!' &&
	    !cmd_line[cmd_pos] &&
	    !strchr(cmd_line, ' ')) {
	  int i;
	  for (i = (cmd_current - 1) % MAXHISTORY;
	       cmd_history[i] && i != cmd_current;
	       i = (i - 1) % MAXHISTORY) {
	    if (!strncmp(cmd_history[i], &cmd_line[1], cmd_pos - 1)) {
	      cmd_new = i;
	      goto set_line;
	    }
	  }
	}
	/* FALL THROUGH TO CHARACTER */
      default:
      normal_char:
	if (c < 32 || (c & 128)) goto error; 
	for (newc = c, l = &cmd_line[cmd_pos]; (oldc = newc); l++) {
	  newc = *l; 
	  *l   = oldc;
	}
	*l = 0;
	cmd_pos++;
	break;
      error:
	cmd_beep();
	iter = repeat;
	break;
      }
    }
    
    if (did_command) {
      repeat = 1;
      meta   = 0;
      cursor = 0;
    }
    
    cmd_update_line(1);
  }
  if (isatty(STDIN_FILENO)) {
#ifdef __svr4__
    system("/usr/bin/stty icanon");
#endif
  }
}
  
static void cmd_update_line(int match_parens)
{ int  i, x, clear_len, level;
  char *s;

  for (i = 0; cmd_line[i]; i++) cmd_line[i] &= 127;

  if (match_parens) {
    /* If the last character in the line is one of the close parens */
    if (cmd_pos && strchr(")]}", cmd_line[cmd_pos - 1])) {

      /* Work back looking for the matching open */
      for (level = 0, i = cmd_pos - 1; i >= 0; i--) {
	if (strchr(")]}", cmd_line[i])) level++;
	if (strchr("([{", cmd_line[i])) level--;
	if (!level) {
	  if (cmd_ansi) {
	    cmd_line[cmd_pos - 1] |= 128;
	    cmd_line[i] |= 128;
	  }
	  break;
	}
      }
    }
  }

  /* Is cmd_line a substring of cmd_old_line? */
  for (i = 0; cmd_old_line[i] == cmd_line[i] && cmd_line[i]; i++);
  
  if (cmd_old_line[i] != cmd_line[i]) {
    for (x = cmd_old_pos; x < i; x++)
      cmd_put_char(cmd_line[x]);
    for (x = i; x < cmd_old_pos; x++)
      cmd_putc_repeat('\b', cmd_char_len(cmd_old_line[x]));
    for (s = &cmd_line[i]; *s; s++) cmd_put_char(*s);
    clear_len = cmd_string_len(&cmd_old_line[i]) - 
                cmd_string_len(&cmd_line[i]);
    cmd_putc_repeat(' ',  clear_len);
    cmd_putc_repeat('\b', clear_len);
    cmd_putc_repeat('\b', cmd_string_len(&cmd_line[cmd_pos]));
  } 
  else {
    for (x = cmd_old_pos; x < cmd_pos; x++)
      cmd_put_char(cmd_line[x]);
    for (x = cmd_pos; x < cmd_old_pos; x++)
      cmd_putc_repeat('\b', cmd_char_len(cmd_old_line[x]));
  }
  fflush(cmd_out);
}

static void cmd_put_char(int c)
{
  if (cmd_ansi && (c & 128)) fprintf(cmd_out, "\033[4m");
  if ((c & 127) < 32) {
    cmd_char_pos += 1;
    fputc('^', cmd_out);
    fputc((c & 127) +'@', cmd_out);
  } 
  else 
    fputc(c & 127, cmd_out);
  cmd_char_pos += 1;
  if (cmd_ansi && (c & 128)) fprintf(cmd_out, "\033[m");
}

static void cmd_putc_repeat(int c, int n)
{
  while(n-- > 0) {
    if (c == '\b') {
      if (cmd_no_reverse_wrap && !(cmd_char_pos % 80) && cmd_ansi) {
	int i;
	/* up line */
	fputc(27, cmd_out); 
	fputc('[', cmd_out); 
	fputc('A', cmd_out);

	/* end of line */
	for (i = 0; i < 79; i++) {
	  fputc(ESC, cmd_out); 
	  fputc('[', cmd_out); 
	  fputc('C', cmd_out);
	}
      } 
      else
	fputc(c, cmd_out);
      cmd_char_pos--;
    } 
    else {
      cmd_char_pos++;
      fputc(c, cmd_out);
    }
  }
}

static int cmd_string_len(char *s)
{ int ret = 0;
  
  while (*s) ret += cmd_char_len(*s++);
  return ret;
}

static void cmd_beep(void)
{
  fputc(Cntl('G'), cmd_out);
}

#ifdef DOS
/* Potentially unbuffered character read */
static int io_getchar(int fd, int *c)
{
  if (fd == 0)
    if (_bios_keybrd(_KEYBRD_READY)) {
      *c = (int) _bios_keybrd(_KEYBRD_READ) & 0x7f;
      return 1;
    }
    else
      return 0;
  else
    return read(fd, c, sizeof(int));
}
#endif
