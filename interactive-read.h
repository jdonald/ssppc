#ifndef __read_h__
#define __read_h__

typedef struct symtabt { struct symtabt *next;
			 tword           value;
			 char           *pname;
		       } symtab;

/* The tail of this structure is of arbitrary length */
typedef struct primopt { struct primopt *next;
			 symtab *name;
			 tword (*func)();
			 char   *usage;
			 char   *descr;
			 TInt    flags;
			 tword   values[1];
		       } primop;

#define PrimopMakeFlags(da, np, sp, rest, opt, req)  \
(((da)  << 19) | ((np) << 18) | ((sp) << 17) |        \
((rest) << 16) | ((opt) << 8) | req)

#define PRIMOP_DO_ALLOC     (1 << 19)
#define PRIMOP_DO_PRINT     (0 << 18)
#define PRIMOP_NO_PRINT     (1 << 18)
#define PRIMOP_SPECIAL_FORM (1 << 17)

#define PrimopFlagsDoAlloc(flag)      ((flag >> 19) & 0x01)
#define PrimopFlagsNoPrint(flag)      ((flag >> 18) & 0x01)
#define PrimopFlagsSpecialForm(flag)  ((flag >> 17) & 0x01)
#define PrimopFlagsRestp(flag)        ((flag >> 16) & 0x01)
#define PrimopFlagsNumOptionals(flag) ((flag >>  8) & 0xFF)
#define PrimopFlagsNumRequired(flag)  ((flag >>  0) & 0xFF)

#define PrimopDoAlloc(p)              (PrimopFlagsDoAlloc((p)->flags))
#define PrimopNoPrint(p)              (PrimopFlagsNoPrint((p)->flags))
#define PrimopSpecialForm(p)          (PrimopFlagsSpecialForm((p)->flags))
#define PrimopRestp(p)                (PrimopFlagsRestp((p)->flags))
#define PrimopNumOptionals(p)         (PrimopFlagsNumOptionals((p)->flags))
#define PrimopNumRequired(p)          (PrimopFlagsNumRequired((p)->flags))

tword   cmd_help(tword cat_or_cmd);
tword   cmd_include(tword filename);
tword   cmd_quit();
tword   cmd_setq(tword symbol, tword slot);

void    ReaderInit();
symtab *MakeSymbol(char *pname);
symtab *Intern(symtab *ptr);
symtab *FindSymbol(char *pname);
int     PrimopIsFunc(primop *ptr);
primop *MarkCommands(char *heading);
primop *AddPrimop(char *usage, tword (*func)(), char *descr, int spec, ...);
void    ReadEvalPrintLoop(char *prompt);
void    ReadEvalPrintLoop2(char *prompt);
void    ReadEvalPrint(char *prompt);
tword   Eval(tword value);
tword   Print(tword value);
void    CmdLineInit();
tword   InteractiveRead(char **ptr);

int    GetReadBase();
void   SetReadBase(int value);
int    GetPrintBase();
void   SetPrintBase(int value);

#endif
