#define SYMNMLEN 8

#define _LONG32 int
#define _ULONG32 unsigned int

#define uchar char

#define F_EXEC          0x0002 
#define F_DYNLOAD       0x1000

#define STYP_REG        0x00    /* "regular" section */
#define STYP_PAD        0x08    /* "padding" section */
#define STYP_TEXT       0x20    /* section contains text only */
#define STYP_DATA       0x40    /* section contains data only */
#define STYP_BSS        0x80    /* section contains bss only */
#define STYP_EXCEPT     0x0100  /* Exception section */
#define STYP_INFO       0x0200  /* Comment section */
#define STYP_LOADER     0x1000  /* Loader section */
#define STYP_DEBUG      0x2000  /* Debug section */
#define STYP_TYPCHK     0x4000  /* Type check section */
#define STYP_OVRFLO     0x8000  /* Overflow section header
                                   for handling relocation and
                                   line number count overflows */

typedef struct aouthdr {
        short   magic;          /* flags - how to execute               */
                                /* 0x010B: text & data may be paged     */
        short   vstamp;         /* version stamp                        */
                                /* 1 is only valid value                */
        _LONG32 tsize;          /* text size in bytes                   */
        _LONG32 dsize;          /* data size in bytes                   */
        _LONG32 bsize;          /* bss size in bytes                    */
        _LONG32 entry;          /* entry point descriptor address       */
        _ULONG32 text_start;    /* virtual address of beginning of text */
        _ULONG32 data_start;    /* virtual address of beginning of data */
        /* A short 32-bit auxiliary header can end here. */

        _ULONG32 o_toc;         /* address of TOC                       */
        short   o_snentry;      /* section number for entry point       */
        short   o_sntext;       /* section number for text              */
        short   o_sndata;       /* section number for data              */
        short   o_sntoc;        /* section number for toc               */
        short   o_snloader;     /* section number for loader            */
        short   o_snbss;        /* section number for bss               */
        short   o_algntext;     /* log (base 2) of max text alignment   */
        short   o_algndata;     /* log (base 2) of max data alignment   */
        char    o_modtype[2];   /* Module type field, 1L, RE, RO        */
        uchar   o_cpuflag;      /* bit flags - cputypes of objects      */
        uchar   o_cputype;      /* executable cpu type identification   */
        _ULONG32 o_maxstack;    /* max stack size allowed (bytes)       */
        _ULONG32 o_maxdata;     /* max data size allowed (bytes)        */
        unsigned int o_debugger;/* Reserved for use by debugger if it   */
                                /* needs a location for a breakpoint.   */
        _ULONG32 o_resv2[2];    /* reserved fields                      */
} AOUTHDR; 

#define _AOUTHSZ_SHORT          (size_t)&(((AOUTHDR *)0)->o_toc)
#define _AOUTHSZ_EXEC           sizeof(AOUTHDR)

typedef struct filehdr {
        unsigned short  f_magic;        /* magic number */
                                        /* Target machine on which the
                                           object file is executable */
        unsigned short  f_nscns;        /* number of sections */
        _LONG32         f_timdat;       /* time & date stamp:
                                           A value of 0 indicates no timestamp.
                                           Negative values are reserved for
                                           future use and should be treated
                                           as 0. */
        _LONG32         f_symptr;       /* File offset to symbol table. */
        _LONG32         f_nsyms;        /* Number of symbol table entries:
                                           Negative values are reserved for
                                           future use and should be treated
                                           as 0. */
        unsigned short  f_opthdr;       /* Size of the auxiliary header */
        unsigned short  f_flags;        /* flags */
} FILHDR;

struct xcoffhdr
{
        struct filehdr  filehdr;
        struct aouthdr  aouthdr;
};

typedef struct scnhdr {
        char            s_name[8];      /* section name */
        _ULONG32        s_paddr;        /* physical address */
        _ULONG32        s_vaddr;        /* virtual address */
        _ULONG32        s_size;         /* section size */
        _LONG32         s_scnptr;       /* file ptr to raw data for section */
        _LONG32         s_relptr;       /* file ptr to relocation for section */
        _LONG32         s_lnnoptr;      /* file ptr to line numbers for sect. */
        unsigned short  s_nreloc;       /* number of relocation entries */
        unsigned short  s_nlnno;        /* number of line number entries */
        _LONG32         s_flags;        /* flags */
} SCNHDR;

#define SCNHSZ  sizeof(SCNHDR)

typedef struct ldrel
{
        _ULONG32        l_vaddr;        /* Address field                */
        _LONG32         l_symndx;       /* Loader symbol table index of */
                                        /* reloc value to apply. This field */
                                        /* is zero-based where 0,1,2 are */
                                        /* text,data,bss and 3 is the first */
                                        /* symbol entry from above */
        unsigned short  l_rtype;        /* relocation type              */
        short           l_rsecnm;       /* section number being relocated */
                                        /* one-based index in scnhdr table */
} LDREL;

#define LDRELSZ sizeof(LDREL)

typedef struct ldhdr {
        _LONG32         l_version;      /* Loader section version number */
        _LONG32         l_nsyms;        /* Qty of loader Symbol table entries */
        _LONG32         l_nreloc;       /* Qty of loader relocation table
                                           entries */
        _ULONG32        l_istlen;       /* Length of loader import file id
                                           strings */
        _LONG32         l_nimpid;       /* Qty of loader import file ids. */
        _ULONG32        l_impoff;       /* Offset to start of loader import
                                           file id strings */
        _ULONG32        l_stlen;        /* Length of loader string table */
        _ULONG32        l_stoff;        /* Offset to start of loader string
                                           table */
} LDHDR;

#define LDHDRSZ sizeof(LDHDR)

typedef struct ldsym {
        union {
                char            _l_name[SYMNMLEN];      /* Symbol name  */
                struct {
                        _LONG32 _l_zeroes;      /* offset if 0          */
                        _LONG32 _l_offset;      /* offset into loader string */
                } _l_l;
                char            *_l_nptr[2];    /* allows for overlaying */
        } _l;
        _ULONG32                l_value;        /* Address field        */
        short                   l_scnum;        /* Section number       */
        char                    l_smtype;       /* type and imp/exp/eps */
                                                /* 0    Unused          */
                                                /* 1    Import          */
                                                /* 2    Entry point     */
                                                /* 3    Export          */
                                                /* 4    Unused          */
                                                /* 5-7  Symbol type     */
        char                    l_smclas;       /* storage class        */
        _LONG32                 l_ifile;        /* import file id       */
                                                /*      string offset   */
        _LONG32                 l_parm;         /* type check offset    */
                                                /*      into loader string */
} LDSYM;

#define LDSYMSZ sizeof(LDSYM)

void swap_int16(unsigned short *p) {
	int q;
	q = *p;
	// *p = (q >> 8) | (q << 8);
	*p = MD_SWAPH(q);
}

void swap_int32(int *p) {
	int q;
	q = *p;
	*p = MD_SWAPW(q);
	/*
	*p = ((q >> 24) & 0xff) | ( ( (q >> 16) & 0xff) << 8 )
			| ( ( (q >> 8) & 0xff) << 16 )
			| ( ( (q >> 0) & 0xff) << 24 );
	*/
}

void swap_fhdr(FILHDR *fhdr) {
	swap_int16(& (fhdr->f_magic) );
	swap_int16(& (fhdr->f_nscns) );	
	swap_int32(& (fhdr->f_timdat) );	
	swap_int32(& (fhdr->f_symptr) );	
	swap_int32(& (fhdr->f_nsyms) );	
	swap_int16(& (fhdr->f_opthdr) );	
	swap_int16(& (fhdr->f_flags) );	
}

void swap_ahdr(AOUTHDR *ahdr) {
	swap_int16((unsigned short*) & (ahdr->magic) );
	swap_int16((unsigned short*) & (ahdr->vstamp) );
	swap_int32(& (ahdr->tsize) );
	swap_int32(& (ahdr->dsize) );
	swap_int32(& (ahdr->bsize) );
	swap_int32(& (ahdr->entry) );
	swap_int32((int*) & (ahdr->text_start) );
	swap_int32((int*) & (ahdr->data_start) );
	swap_int32((int*) & (ahdr->o_toc) );
	swap_int16((unsigned short*) & (ahdr->o_snentry) );
	swap_int16((unsigned short*) & (ahdr->o_sntext) );
	swap_int16((unsigned short*) & (ahdr->o_sndata) );
	swap_int16((unsigned short*) & (ahdr->o_snloader) );
	swap_int16((unsigned short*) & (ahdr->o_snbss) );
	swap_int16((unsigned short*) & (ahdr->o_algntext) );
	swap_int16((unsigned short*) & (ahdr->o_algndata) );
	/* swap_int16((int *) &(ahdr->o_modtype) ); */ /* dont swap this ! char * type */
	swap_int32((int*) & (ahdr->o_maxstack) );
	swap_int32((int*) & (ahdr->o_maxdata) );
	swap_int32((int*) & (ahdr->o_debugger) );
	swap_int32((int*) & (ahdr->o_resv2[0]) );
	swap_int32((int*) & (ahdr->o_resv2[1]) );
}

void swap_shdr(SCNHDR *shdr) {
	swap_int32((int*) & (shdr->s_paddr) );
	swap_int32((int*) & (shdr->s_vaddr) );
	swap_int32((int*) & (shdr->s_size) );
	swap_int32((int*) & (shdr->s_scnptr) );
	swap_int32((int*) & (shdr->s_relptr) );
	swap_int32((int*) & (shdr->s_lnnoptr) );
	swap_int16(& (shdr->s_nreloc) );
	swap_int16(& (shdr->s_nlnno) );
	swap_int32(& (shdr->s_flags) );
}

void swap_buf(int *p, int size) {
	int i, s;
	//assert( (size % 4) == 0);
	s = ((size / 4) * 4);
	for (i = 0; i < s; i+=4) {
		swap_int32(&p[i / 4]);
	}
}

void swap_xcoff_hdr(struct xcoffhdr *xcoff_hdr) {
	swap_fhdr((FILHDR *) &(xcoff_hdr->filehdr));
	swap_ahdr((AOUTHDR *) &(xcoff_hdr->aouthdr));
}

void swap_ldhdr(LDHDR *ldhdr) {
	swap_int32(& (ldhdr->l_version) );
	swap_int32(& (ldhdr->l_nsyms) );
	swap_int32(& (ldhdr->l_nreloc) );
	swap_int32((int*) & (ldhdr->l_istlen) );
	swap_int32((int*) & (ldhdr->l_nimpid) );
	swap_int32((int*) & (ldhdr->l_impoff) );
	swap_int32((int*) & (ldhdr->l_stlen) );
	swap_int32((int*) & (ldhdr->l_stoff) );
}

void swap_ldsym(LDSYM *ldsym) {
	/*
	swap_int32( (int *) &(ldsym->_l._l_name[0]) );
	swap_int32( (int *) &(ldsym->_l._l_name[4]) );
	swap_int32(& (ldsym->_l._l_l._l_zeroes) );
	swap_int32(& (ldsym->_l._l_l._l_offset) );
	swap_int32(& (ldsum->_l._l_nptr[0]) );
	swap_int32(& (ldsum->_l._l_nptr[1]) );
	*/
	if (ldsym->_l._l_l._l_zeroes == 0) {
		swap_int32(& (ldsym->_l._l_l._l_offset) );
	}
	swap_int32((int*) & (ldsym->l_value) );
	swap_int16((unsigned short*) & (ldsym->l_scnum) );
	swap_int32(& (ldsym->l_ifile) );
	swap_int32(& (ldsym->l_parm) );
}

void swap_ldrel(LDREL *ldrel) {
	swap_int32((int*) & (ldrel->l_vaddr));
	swap_int32(& (ldrel->l_symndx) );
	swap_int16(& (ldrel->l_rtype) );
	swap_int16((unsigned short*) & (ldrel->l_rsecnm) );
}

void swap_datasegment(struct mem_t *mem, int dstart, int size) {
	int start;
	int i, w;
	start = dstart + (4 - (dstart % 4));
	for (i = 0; i < size; i+=4) {
		w = MEM_READ_WORD(mem, start+i);
		swap_int32(&w); 
		MEM_WRITE_WORD(mem, start+i, w);
	}
}
