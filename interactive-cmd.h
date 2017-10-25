
#ifndef __cmd_h__
#define __cmd_h__



void InitializeCommonCommands(tword quietp);
int symtab_get_int(char *pname);
void symtab_set_int(char *pname, int);
tword symtab_get_tword(char *pname);
void symtab_set_tword(char *pname, tword);
tword cmd_showregs(tword r);
tword cmd_showmem(tword r1,tword r2 );
tword cmd_showmems(tword r1,tword r2 );
void stepi(void);
void runstepi(void);

#endif
