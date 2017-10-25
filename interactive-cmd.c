/* 
$Header: /projects/cart/projects/cvsroot/ss3ppc-linux/interactive-cmd.c,v 1.1.1.1 2001/08/10 14:58:21 karu Exp $
$Locker:  $
*/


/*--------------------------------------------------------------------------
 *                                                                          
 *  cmd.c:  
 *
 *--------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h> // jdonald

#include "interactive-types.h"
#include "interactive-read.h"
#include "interactive-cmd.h"

/***************************************************************************
 * 
 * cmd_print_comment
 *   
 ***************************************************************************/

tword cmd_print_comment ( tstring comment ) {
  if ( comment.data.cptr ) {
    printf ( "\n");
    printf ( "Comment: %s\n\n" , comment.data.cptr );
    return TAGGED_TRUE;
  }
  else
    {
    printf ("\n");
    return TAGGED_TRUE;
  }
}


/***************************************************************************
 * 
 * cmd_run:
 *   
 ***************************************************************************/

tword cmd_run() {
  while(!CntlcPollIsWaiting()) {
		runstepi();
	}
  return TAGGED_TRUE;
}


/***************************************************************************
 * 
 * cmd_step:
 *   
 ***************************************************************************/

tword cmd_step(tword num_steps) {
  int i;
  printf("cmd_step %d\n", (int)num_steps.data.val);
  for(i = 0;  (i < num_steps.data.val-1) && (!CntlcPollIsWaiting()); i++) runstepi();
	stepi();
  return TAGGED_TRUE;
}

tword cmd_stepi(void) {
	stepi();
	return TAGGED_TRUE;
}


/***************************************************************************
 * 
 * cmd_break:
 *   
 ***************************************************************************/

tword cmd_break(tword pc_val) {
  printf("cmd_break %d\n", (int)pc_val.data.val);
  return TAGGED_TRUE;
}


/***************************************************************************
 * 
 * InitializeCommonCommands:
 *   
 ***************************************************************************/


void InitializeCommonCommands(tword quietp) {

    AddPrimop("run",
	      cmd_run,
	      "Run system until <ctrl-C> or program termination",
	      PRIMOP_NO_PRINT | PRIMOP_SPECIAL_FORM);
	AddPrimop("stepi",
         cmd_stepi,
         "Step one instruction",
         PRIMOP_NO_PRINT | PRIMOP_SPECIAL_FORM);

	AddPrimop("regs &opt <regnum>",
         cmd_showregs,
         "Show register values",
         PRIMOP_NO_PRINT | PRIMOP_SPECIAL_FORM,
			make_tagged_int(TAG_INT),
			make_tagged_int(-1) );

	AddPrimop("mem <address> &opt <range>",
         cmd_showmem,
         "Examine memory",
         PRIMOP_NO_PRINT | PRIMOP_SPECIAL_FORM,
         make_tagged_int(TAG_INT),
			make_tagged_int(TAG_INT),
			make_tagged_int(4) );

   AddPrimop("mems <address> &opt <range>",
         cmd_showmems,
         "Examine memory",
         PRIMOP_NO_PRINT | PRIMOP_SPECIAL_FORM,
         make_tagged_int(TAG_INT),
         make_tagged_int(TAG_INT),
         make_tagged_int(128) );     

    AddPrimop("step &opt <num_steps>",
	      cmd_step,
	      "Step entire system <num_steps> times",
	      PRIMOP_NO_PRINT | PRIMOP_SPECIAL_FORM,
	      make_tagged_int(TAG_INT),
	      make_tagged_int(1));
    
  
  /* ********************************************************************** */

  MarkCommands("Breakpoints");

  /* ********************************************************************** */

  AddPrimop("break <pc_val>",
	    cmd_break,
	    "Set Breakpoint at program counter pc_val",
	    PRIMOP_NO_PRINT | PRIMOP_SPECIAL_FORM,
	    make_tagged_int(TAG_INT));


}


/***************************************************************************
 * 
 * symtab_get_int:
 *   
 ***************************************************************************/

int symtab_get_int(char *pname) {
  symtab *result;
  result = FindSymbol(pname);
  switch(result->value.tag) {
  case TAG_INT:
    return result->value.data.val;
    break;
  default:
    fprintf(stderr, "Error in symtab_get_int:  %s not bound to integer\n", pname);
    exit(1);
    break;
  }
  return 0;
}


/***************************************************************************
 * 
 * symtab_set_int:
 *   
 ***************************************************************************/

void symtab_set_int(char *pname, int val) {
  symtab *result;
  result = FindSymbol(pname);
  result->value = make_tagged_int(val);
}



/***************************************************************************
 * 
 * symtab_get_tword:
 *   
 ***************************************************************************/

tword symtab_get_tword(char *pname) {
  symtab *result;
  result = FindSymbol(pname);
  return result->value;
}



/***************************************************************************
 * 
 * symtab_set_tword:
 *   
 ***************************************************************************/

void symtab_set_tword(char *pname, tword val) {
  symtab *result;
  result = FindSymbol(pname);
  result->value = val;
}
