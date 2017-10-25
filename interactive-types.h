#ifndef __types_h__
#define __types_h__


#ifdef DOS
typedef unsigned short ushort;
typedef unsigned int   uint;
#endif

typedef long TInt;
typedef unsigned long TUInt;
typedef int Tag;            /* The tag field of a tagged word */

#define TAG_SYM        0x00
#define TAG_INT        0x01
#define TAG_BOOL       0x02
#define TAG_TAG8       0x08
#define TAG_TAG9       0x09
#define TAG_TAGA       0x0A
#define TAG_TAGB       0x0B

#define TAG_UNBOUND    0x10
#define TAG_NOVALUE    0x11
#define TAG_ERROR      0x12
#define TAG_STRING     0x13
#define TAG_VECTOR     0x14
#define TAG_SYMBOL     0x15
#define TAG_PRIMOP     0x16
#define TAG_FLOAT      0x17
#define TAG_ANY        0x19


/****************************************************/

typedef struct twordt { union { TInt    val;
				TUInt   uval;
				char   *cptr;
				struct  twordt *tptr;
				float   fval;
				double *dval;
				void   *vptr;
			      } data;
			long tag;
		      } tword;

/* Aliases to enhance source documentation */
typedef tword tint, tfloat, tstring, tsymbol;

extern const tword TAGGED_FALSE, TAGGED_TRUE, NIL, TAGGED_ZERO, TAGGED_ONE, // jdonald made const
             UNBOUND, NOVALUE;

#define MakeTInt(slot, val0)                                              \
{                                                                         \
  slot.tag      = TAG_INT;                                                \
  slot.data.val = val0;                                                   \
}

#define MakeTWord(slot, type, val0)                                       \
{                                                                         \
  slot.tag      = type;                                                   \
  slot.data.val = val0;                                                   \
}

#define MakeTWordV(slot, type, val0)                                      \
{                                                                         \
  slot.tag       = type;                                                  \
  slot.data.vptr = val0;                                                  \
}

tword make_tagged_word(Tag tag, TInt value);
tword make_tagged_wordv(Tag tag, void *ptr);
tword make_tagged_sym(TInt value);
tword make_tagged_int(TInt value);
tword make_tagged_bool(TInt value);
tword make_tagged_vector(tword num_words);
tword free_tagged_vector(tword vector);
tword tagged_vector_size(tword vector);
tword make_tagged_string(char *cstring);
tword make_tagged_stringa(char *cstring);
tword free_tagged_string(tword string);
tword tagged_string_length(tword string);
tword make_tagged_float(float fval);

void  InitializeInterrupts();
void  add_exit_handler(void (*func)(int status));
void  Exit(int status);
void  add_cntlc_handler(void (*func)(int status));
void  delay_cntlc();
void  undelay_cntlc();
void  cntlc_continue_handler(void (*func)());
void  CntlcPollingHandler();
int   CntlcPollIsWaiting();
void  CntlcInstallPollingHandler();

void  err_sys(char *fmt, ...);

#endif
