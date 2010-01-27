// =============================================================== //
//                                                                 //
//   File      : ClustalV.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Coded based on ClustalV by Ralf Westram (coder@reallysoft.de) //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ClustalV.hxx"
#include "awtc_seq_search.hxx"

#include <cctype>
#include <climits>


/* ---------------------------------------------------------------- */

#define MASTER_GAP_OPEN                 50
#define CHEAP_GAP_OPEN                  20      // penalty for creating a gap (if we have gaps in master)

#define DEFAULT_GAP_OPEN                30      // penalty for creating a gap

#define MASTER_GAP_EXTEND               18
#define CHEAP_GAP_EXTEND                5       // penalty for extending a gap (if we have gaps in master)

#define DEFAULT_GAP_EXTEND              10      // penalty for extending a gap

#define DEFAULT_IMPROBABLY_MUTATION     10      // penalty for mutations
#define DEFAULT_PROBABLY_MUTATION       4       // penalty for special mutations (A<->G,C<->U/T)        (only if 'weighted')

#define DYNAMIC_PENALTIES                       // otherwise you get FIXED PENALTIES    (=no cheap penalties)

/* ---------------------------------------------------------------- */

#define MAX_GAP_OPEN_DISCOUNT           (DEFAULT_GAP_OPEN-CHEAP_GAP_OPEN)       // maximum subtracted from DEFAULT_GAP_OPEN
#define MAX_GAP_EXTEND_DISCOUNT         (DEFAULT_GAP_EXTEND-CHEAP_GAP_EXTEND)   // maximum subtracted from DEFAULT_GAP_EXTEND

#define MAXN    2       /* Maximum number of sequences (both groups) */

#if (MAXN==2)
#define MAXN_2(xxx)             xxx
#define MAXN_2_assert(xxx)      awtc_assert(xxx)
#else
#define MAXN_2(xxx)
#define MAXN_2_assert(xxx)
#endif

#define TRUE    1
#define FALSE   0

static bool module_initialized = false;

typedef int Boolean;

static Boolean dnaflag;
static Boolean is_weight;

#define MAX_BASETYPES 21

static int xover;
static int little_pam;
static int big_pam;
static int pamo[(MAX_BASETYPES-1)*MAX_BASETYPES/2]; 
static int pam[MAX_BASETYPES][MAX_BASETYPES];

static int             pos1;
static int             pos2;
static int           **naa1;                 // naa1[basetype][position]     counts bases for each position of all sequences in group1
static int           **naa2;                 // naa2[basetype][position]     same for group2
static int           **naas;                 // 
static int             seqlen_array[MAXN+1]; // length of all sequences
static unsigned char  *seq_array[MAXN+1];    // the sequences
static int             group[MAXN+1];        // group of sequence
static int             alist[MAXN+1];        // indices of sequences to be aligned
static int             fst_list[MAXN+1];
static int             snd_list[MAXN+1];

static int nseqs;               // # of sequences
static int weights[MAX_BASETYPES][MAX_BASETYPES];     // weights[b1][b2] : penalty for mutation from base 'b1' to base 'b2'

#if defined(DEBUG)
size_t displ_size = 0;
#endif // DEBUG

static int *displ;                                  // displ == 0 -> base in both , displ<0 -> displ gaps in slave, displ>0 -> displ gaps in master
static int *zza;                                    // column (left->right) of align matrix (minimum of all paths to this matrix element)
static int *zzb;                                    // -------------- " ------------------- (minimum of all paths, where gap inserted into slave)
static int *zzc;                                    // column (left<-right) of align matrix (minimum of all paths to this matrix element)
static int *zzd;                                    // -------------- " ------------------- (minimum of all paths, where gap inserted into slave)
static int  print_ptr;
static int  last_print;

static const int *gapsBeforePosition;

#if defined(DEBUG)
// #define MATRIX_DUMP
// #define DISPLAY_DIFF 
#endif // DEBUG

#ifdef MATRIX_DUMP
# define IF_MATRIX_DUMP(xxx) xxx
# define DISPLAY_MATRIX_SIZE 3000

static int vertical             [DISPLAY_MATRIX_SIZE+2][DISPLAY_MATRIX_SIZE+2];
static int verticalOpen         [DISPLAY_MATRIX_SIZE+2][DISPLAY_MATRIX_SIZE+2];
static int diagonal             [DISPLAY_MATRIX_SIZE+2][DISPLAY_MATRIX_SIZE+2];
static int horizontal           [DISPLAY_MATRIX_SIZE+2][DISPLAY_MATRIX_SIZE+2];
static int horizontalOpen       [DISPLAY_MATRIX_SIZE+2][DISPLAY_MATRIX_SIZE+2];

#else
# define IF_MATRIX_DUMP(xxx)
#endif

static inline int master_gap_open(int beforePosition)
{
#ifdef DYNAMIC_PENALTIES
    long gaps = gapsBeforePosition[beforePosition-1];
    return (gaps) ? MASTER_GAP_OPEN - MAX_GAP_OPEN_DISCOUNT : MASTER_GAP_OPEN;

    /*    return
          gaps >= MAX_GAP_OPEN_DISCOUNT
          ? DEFAULT_GAP_OPEN-MAX_GAP_OPEN_DISCOUNT
          : DEFAULT_GAP_OPEN-gaps; */
#else
    return DEFAULT_GAP_OPEN;
#endif
}
static inline int master_gap_extend(int beforePosition)
{
#ifdef DYNAMIC_PENALTIES
    long gaps = gapsBeforePosition[beforePosition-1];

    return (gaps) ? MASTER_GAP_EXTEND - MAX_GAP_EXTEND_DISCOUNT : MASTER_GAP_EXTEND;
    /*    return
          gaps >= MAX_GAP_EXTEND_DISCOUNT
          ? DEFAULT_GAP_EXTEND-MAX_GAP_EXTEND_DISCOUNT
          : DEFAULT_GAP_EXTEND-gaps; */
#else
    return DEFAULT_GAP_EXTEND;
#endif
}

static inline int master_gapAtWithOpenPenalty(int atPosition, int length, int penalty)
{
    if (length<=0)
        return 0;

    int beforePosition = atPosition,
        afterPosition = atPosition-1;

    while (length--)
    {
        int p1, p2;

        if ((p1=master_gap_extend(beforePosition)) < (p2=master_gap_extend(afterPosition+1)) &&
            beforePosition>1)
        {
            penalty += p1;
            beforePosition--;
        }
        else
        {
            penalty += p2;
            afterPosition++;
        }
    }

    return penalty;
}

static inline int master_gapAt(int atPosition, int length)
{
    return master_gapAtWithOpenPenalty(atPosition, length, master_gap_open(atPosition));
}

static inline int slave_gap_open(int /* beforePosition */)
{
    return DEFAULT_GAP_OPEN;
}

static inline int slave_gap_extend(int /* beforePosition */)
{
    return DEFAULT_GAP_EXTEND;
}

static inline int slave_gapAtWithOpenPenalty(int atPosition, int length, int penalty)
{
    return length<=0 ? 0 : penalty + length*slave_gap_extend(atPosition);
}

static inline int slave_gapAt(int atPosition, int length)
{
    return slave_gapAtWithOpenPenalty(atPosition, length, slave_gap_open(atPosition));
}

#define UNKNOWN_ACID 255
static const char *amino_acid_order   = "XCSTPAGNDEQHRKMILVFYW";

#define NUCLEIDS 16
static const char *nucleic_acid_order = "-ACGTUMRWSYKVHDBN";
static const char *nucleic_maybe_A    = "-A----AAA---AAA-A";
static const char *nucleic_maybe_C    = "--C---C--CC-CC-CC";
static const char *nucleic_maybe_G    = "---G---G-G-GG-GGG";
static const char *nucleic_maybe_T    = "----T---T-TT-TTTT";
static const char *nucleic_maybe_U    = "-----U--U-UU-UUUU";
static const char *nucleic_maybe[6]   = { NULL, nucleic_maybe_A, nucleic_maybe_C, nucleic_maybe_G, nucleic_maybe_T, nucleic_maybe_U };

/*
 *      M = A or C              S = G or C              V = A or G or C         N = A or C or G or T
 *      R = A or G              Y = C or T              H = A or C or T
 *      W = A or T              K = G or T              D = A or G or T
 */

#define cheap_if(cond)  ((cond) ? 1 : 2)
static int baseCmp(unsigned char c1, unsigned char c2)    // c1,c2 == 1=A,2=C (==index of character in nucleic_acid_order[])
// returns  0 for equal
//          1 for probably mutations
//          2 for improbably mutations
{
#define COMPARABLE_BASES 5

    if (c1==c2) return 0;

    if (c2<c1)          // swap
    {
        int c3 = c1;

        c1 = c2;
        c2 = c3;
    }

    if (c2<=COMPARABLE_BASES)
    {
        awtc_assert(c1<=COMPARABLE_BASES);

        switch (c1)
        {
            case 0: return 2;
            case 1: return cheap_if(c2==3);                     // A->G
            case 2: return cheap_if(c2==4 || c2==5);            // C->T/U
            case 3: return cheap_if(c2==1);                     // G->A
            case 4: if (c2==5) return 0;                        // T->U
            case 5: if (c2==4) return 0;                        // U->T
                return cheap_if(c2==2);                         // T/U->C
            default: awtc_assert(0);
        }
    }

    int i;
    int bestMatch = 3;

    if (c1<=COMPARABLE_BASES)
    {
        for (i=1; i<=COMPARABLE_BASES; i++)
        {
            if (isalpha(nucleic_maybe[i][c2]))          // 'c2' maybe a 'i'
            {
                int match = baseCmp(c1, i);
                if (match<bestMatch) bestMatch = match;
            }
        }
    }
    else
    {
        for (i=1; i<=COMPARABLE_BASES; i++)
        {
            if (isalpha(nucleic_maybe[i][c1]))          // 'c1' maybe a 'i'
            {
                int match = baseCmp(i, c2);
                if (match<bestMatch) bestMatch = match;
            }
        }
    }

    awtc_assert(bestMatch>=0 && bestMatch<=2);
    return bestMatch;
}
#undef cheap_if

int AWTC_baseMatch(char c1, char c2)    // c1,c2 == ACGUTRS...
// returns  0 for equal
//          1 for probably mutations
//          2 for improbably mutations
//          -1 if one char is illegal
{
    const char *p1 = strchr(nucleic_acid_order, c1);
    const char *p2 = strchr(nucleic_acid_order, c2);

    awtc_assert(c1);
    awtc_assert(c2);

    if (p1 && p2) return baseCmp(p1-nucleic_acid_order, p2-nucleic_acid_order);
    return -1;
}

static int getPenalty(char c1, char c2) /* c1,c2 = A=0,C=1,... s.o. */
{
    switch (baseCmp(c1, c2))
    {
        case 1: return DEFAULT_PROBABLY_MUTATION;
        case 2: return DEFAULT_IMPROBABLY_MUTATION;
        default: break;
    }

    return 0;
}

static char *result[3];         // result buffers

static char pam250mt[] = {
    12,
     0, 2,
    -2, 1, 3,
    -3, 1, 0, 6,
    -2, 1, 1, 1, 2,
    -3, 1, 0, -1, 1, 5,
    -4, 1, 0, -1, 0, 0, 2,
    -5, 0, 0, -1, 0, 1, 2, 4,
    -5, 0, 0, -1, 0, 0, 1, 3, 4,
    -5, -1, -1, 0, 0, -1, 1, 2, 2, 4,
    -3, -1, -1, 0, -1, -2, 2, 1, 1, 3, 6,
    -4, 0, -1, 0, -2, -3, 0, -1, -1, 1, 2, 6,
    -5, 0, 0, -1, -1, -2, 1, 0, 0, 1, 0, 3, 5,
    -5, -2, -1, -2, -1, -3, -2, -3, -2, -1, -2, 0, 0, 6,
    -2, -1, 0, -2, -1, -3, -2, -2, -2, -2, -2, -2, -2, 2, 5,
    -6, -3, -2, -3, -2, -4, -3, -4, -3, -2, -2, -3, -3, 4, 2, 6,
    -2, -1, 0, -1, 0, -1, -2, -2, -2, -2, -2, -2, -2, 2, 4, 2, 4,
    -4, -3, -3, -5, -4, -5, -4, -6, -5, -5, -2, -4, -5, 0, 1, 2, -1, 9,
     0, -3, -3, -5, -3, -5, -2, -4, -4, -4, 0, -4, -4, -2, -1, -1, -2, 7, 10,
    -8, -2, -5, -6, -6, -7, -4, -7, -7, -5, -3, 2, -3, -4, -5, -2, -6, 0, 0, 17
};

static char *matptr = pam250mt;

#if (defined(DISPLAY_DIFF) || defined(MATRIX_DUMP))
static void p_decode(const unsigned char *naseq, unsigned char *seq, int l) {
    int len = strlen(amino_acid_order);

    for (int i=1; i<=l && naseq[i]; i++)
    {
        awtc_assert(naseq[i]<len);
        seq[i] = amino_acid_order[naseq[i]];
    }
}

static void n_decode(const unsigned char *naseq, unsigned char *seq, int l) {
    int len = strlen(nucleic_acid_order);

    for (int i=1; i<=l && naseq[i]; i++)
    {
        awtc_assert(naseq[i]<len);
        seq[i] = nucleic_acid_order[naseq[i]];
    }
}
#endif

static inline ARB_ERROR MAXLENtooSmall() {
    return "AWTC_ClustalV-aligner: MAXLEN is dimensioned to small for this sequence";
}

static inline void *ckalloc(size_t bytes, ARB_ERROR& error) {
    if (error) return NULL;

    void *ret = malloc(bytes);

    if (!ret) error = "out of memory";
    return ret;
}

static ARB_ERROR init_myers(long max_seq_length) {
    ARB_ERROR error;

    naa1 = (int **)ckalloc(MAX_BASETYPES * sizeof (int *), error);
    naa2 = (int **)ckalloc(MAX_BASETYPES * sizeof (int *), error);
    naas = (int **)ckalloc(MAX_BASETYPES * sizeof (int *), error);

    for (int i=0; i<MAX_BASETYPES && !error; i++) {
        naa1[i] = (int *)ckalloc((max_seq_length+1)*sizeof(int), error);
        naa2[i] = (int *)ckalloc((max_seq_length+1)*sizeof(int), error);
        naas[i] = (int *)ckalloc((max_seq_length+1)*sizeof(int), error);
    }
    return error; 
}

static void make_pamo(int nv)
{
    int i, c;

    little_pam=big_pam=matptr[0];
    for (i=0; i<210; ++i) {
        c=matptr[i];
        little_pam=(little_pam<c) ? little_pam : c;
        big_pam=(big_pam>c) ? big_pam : c;
    }
    for (i=0; i<210; ++i)
        pamo[i] = matptr[i]-little_pam;
    nv -= little_pam;
    big_pam -= little_pam;
    xover = big_pam - nv;
    /*
      fprintf(stdout,"\n\nxover= %d, big_pam = %d, little_pam=%d, nv = %d\n\n"
      ,xover,big_pam,little_pam,nv);
    */
}

static void fill_pam()
{
    int i, j, pos;

    pos=0;

    for (i=0; i<20; ++i)
        for (j=0; j<=i; ++j)
            pam[i][j]=pamo[pos++];

    for (i=0; i<20; ++i)
        for (j=0; j<=i; ++j)
            pam[j][i]=pam[i][j];

    if (dnaflag)
    {
        xover=4;
        big_pam=8;
        for (i=0; i<=NUCLEIDS; ++i)
            for (j=0; j<=NUCLEIDS; ++j)
                weights[i][j] = getPenalty(i, j);
    }
    else {
        for (i=1; i<MAX_BASETYPES; ++i) {
            for (j=1; j<MAX_BASETYPES; ++j) {
                weights[i][j] = big_pam - pam[i-1][j-1];
            }
        }
        for (i=0; i<MAX_BASETYPES; ++i) {
            weights[0][i] = xover;
            weights[i][0] = xover;
        }
    }
}

static void exit_myers(void) {
    for (int i=0; i<MAX_BASETYPES; i++) {
        free(naas[i]);
        free(naa2[i]);
        free(naa1[i]);
    }
    free(naas);
    free(naa2);
    free(naa1);
}

static ARB_ERROR init_show_pair(long max_seq_length) {
    ARB_ERROR error;

#if defined(DEBUG)
    displ_size = (2*max_seq_length + 1);
#endif // DEBUG

    displ      = (int *) ckalloc((2*max_seq_length + 1) * sizeof (int), error);
    last_print = 0;

    zza = (int *)ckalloc((max_seq_length+1) * sizeof (int), error);
    zzb = (int *)ckalloc((max_seq_length+1) * sizeof (int), error);

    zzc = (int *)ckalloc((max_seq_length+1) * sizeof (int), error);
    zzd = (int *)ckalloc((max_seq_length+1) * sizeof (int), error);

    return error;
}

void exit_show_pair(void)
{
    freenull(zzd);
    freenull(zzc);
    freenull(zzb);
    freenull(zza);
    freenull(displ);
#if defined(DEBUG)
    displ_size = 0;
#endif // DEBUG
}

inline int set_displ(int offset, int value) {
    awtc_assert(offset >= 0 && offset<int(displ_size));
    displ[offset] = value;
    return displ[offset];
}
inline int decr_displ(int offset, int value) {
    awtc_assert(offset >= 0 && offset<int(displ_size));
    displ[offset] -= value;
    return displ[offset];
}


static inline void add(int v)           // insert 'v' gaps into master ???
{
    if (last_print<0 && print_ptr>0) {
        set_displ(print_ptr-1, v);
        set_displ(print_ptr++, last_print);
    }
    else {
        last_print = set_displ(print_ptr++, v);
    }
}

static MAXN_2(inline) int calc_weight(int iat, int jat, int v1, int v2)
{
#if (MAXN==2)
    awtc_assert(pos1==1 && pos2==1);
    unsigned char j = seq_array[alist[1]][v2+jat-1];
    return j<128 ? naas[j][v1+iat-1] : 0;
#else
    int sum, i, lookn, ret;
    int ipos, jpos;

    ipos = v1 + iat -1;
    jpos = v2 + jat -1;

    ret = 0;
    sum = lookn = 0;

    if (pos1>=pos2)
    {
        for (i=1; i<=pos2; ++i)
        {
            unsigned char j=seq_array[alist[i]][jpos];
            if (j<128) {
                sum += naas[j][ipos];
                ++lookn;
            }
        }
    }
    else
    {
        for (i=1; i<=pos1; ++i)
        {
            unsigned char j = seq_array[alist[i]][ipos];
            if (j<128) {
                sum += naas[j][jpos];
                ++lookn;
            }
        }
    }
    if (sum>0) ret = sum/lookn;
    return ret;
#endif
}

#ifdef MATRIX_DUMP

static inline const unsigned char *lstr(const unsigned char *s, int len) {
    static unsigned char *lstr_ss = 0;

    freeset(lstr_ss, (unsigned char*)strndup((const char*)s, len));
    return lstr_ss;
}

static inline const unsigned char *nstr(unsigned char *cp, int length)
{
    const unsigned char *s = lstr(cp, length);
    (dnaflag ? n_decode : p_decode)(s-1, const_cast<unsigned char *>(s-1), length);
    return s;
}

static inline void dumpMatrix(int x0, int y0, int breite, int hoehe, int mitte_vert)
{
    int b;
    int h;
    char *sl = (char*)malloc(hoehe+3);
    char *ma = (char*)malloc(breite+3);

    sprintf(ma, "-%s-", nstr(seq_array[1]+x0, breite));
    sprintf(sl, "-%s-", nstr(seq_array[2]+y0, hoehe));

    printf("                 ");

    for (b=0; b<=mitte_vert; b++)
        printf("%5c", ma[b]);
    printf("  MID");
    for (b++; b<=breite+1+1; b++)
        printf("%5c", ma[b-1]);

    for (h=0; h<=hoehe+1; h++)
    {
        printf("\n%c vertical:      ", sl[h]);
        for (b=0; b<=breite+1+1; b++) printf("%5i", vertical[b][h]);
        printf("\n  verticalOpen:  ");
        for (b=0; b<=breite+1+1; b++) printf("%5i", verticalOpen[b][h]);
        printf("\n  diagonal:      ");
        for (b=0; b<=breite+1+1; b++) printf("%5i", diagonal[b][h]);
        printf("\n  horizontal:    ");
        for (b=0; b<=breite+1+1; b++) printf("%5i", horizontal[b][h]);
        printf("\n  horizontalOpen:");
        for (b=0; b<=breite+1+1; b++) printf("%5i", horizontalOpen[b][h]);
        printf("\n");
    }

    printf("--------------------\n");

    free(ma);
    free(sl);
}
#endif

static int diff(int v1, int v2, int v3, int v4, int st, int en)
/* v1,v3    master sequence (v1=offset, v3=length)
 * v2,v4    slave sequence (v2=offset, v4=length)
 * st,en    gap_open-penalties for start and end of the sequence
 *
 * returns  costs for inserted gaps
 */
{
    int ctrc, ctri, ctrj=0,
        i, j, k, l, m, n, p,
        flag;

#ifdef DEBUG
# ifdef MATRIX_DUMP
    int display_matrix = 0;

    awtc_assert(v3<=(DISPLAY_MATRIX_SIZE+2));   // width
    awtc_assert(v4<=(DISPLAY_MATRIX_SIZE));             // height

# define DISPLAY_MATRIX_ELEMENTS ((DISPLAY_MATRIX_SIZE+2)*(DISPLAY_MATRIX_SIZE+2))

    memset(vertical, -1, DISPLAY_MATRIX_ELEMENTS*sizeof(int));
    memset(verticalOpen, -1, DISPLAY_MATRIX_ELEMENTS*sizeof(int));
    memset(diagonal, -1, DISPLAY_MATRIX_ELEMENTS*sizeof(int));
    memset(horizontal, -1, DISPLAY_MATRIX_ELEMENTS*sizeof(int));
    memset(horizontalOpen, -1, DISPLAY_MATRIX_ELEMENTS*sizeof(int));

# endif
#endif
    static int deep;
    deep++;

#if (defined (DEBUG) && 0)
    {
        char *d;

        d = lstr(seq_array[1]+v1, v3);
        
        (dnaflag ? n_decode : p_decode)(d-1, d-1, v3);

        for (int cnt=0; cnt<deep; cnt++) putchar(' ');
        printf("master = '%s' (penalties left=%i right=%i)\n", d, st, en);

        d = lstr(seq_array[2]+v2, v4);
        (dnaflag ? n_decode : p_decode)(d-1, d-1, v4);

        for (cnt=0; cnt<deep; cnt++) putchar(' ');
        printf("slave  = '%s'\n", d);
    }
#endif

    if (v4<=0) {                                                // if slave sequence is empty
        if (v3>0) {
            if (last_print<0 && print_ptr>0) {
                last_print = decr_displ(print_ptr-1, v3);         // add ..
            }
            else {
                last_print = set_displ(print_ptr++, -(v3));       // .. or insert gap of length 'v3' into slave
            }
        }

        deep--;
        return master_gapAt(v1, v3);
    }

    if (v3<=1) {
        if (v3<=0) {                                            // if master sequence is empty
            add(v4);                                            // ??? insert gap length 'v4' into master ???
            deep--;
            return slave_gapAt(v2, v4);
        }

        awtc_assert(v3==1);
        // if master length == 1

        if (v4==1)
        {
            if (st>en)
                st=en;

            /*!*************if(!v4)*********BUG********************************/

            ctrc = slave_gapAtWithOpenPenalty(v2, v4, st);
            ctrj = 0;

            for (j=1; j<=v4; ++j)
            {
                k = slave_gapAt(v2, j-1) + calc_weight(1, j, v1, v2) + slave_gapAt(v2+j, v4-j);
                if (k<ctrc) { ctrc = k; ctrj = j; }
            }

            if (!ctrj)
            {
                add(v4);
                if (last_print<0 && print_ptr>0)
                    last_print = decr_displ(print_ptr-1, 1);
                else
                    last_print = set_displ(print_ptr++, -1);
            }
            else
            {
                if (ctrj>1) add(ctrj-1);
                set_displ(print_ptr++, last_print = 0);
                if (ctrj<v4) add(v4-ctrj);
            }

            deep--;
            return ctrc;
        }
    }

    awtc_assert(v3>=1 && v4>=1);

    // slave length  >= 1
    // master length >= 1

# define AF 0

    // first column of matrix (slave side):
    IF_MATRIX_DUMP(vertical[0][0]=)
        zza[0] = 0;
    p = master_gap_open(v1);
    for (j=1; j<=v4; j++)
    {
        p += master_gap_extend(v1);
        IF_MATRIX_DUMP(vertical[0][j]=)
            zza[j] = p;
        IF_MATRIX_DUMP(verticalOpen[0][j]=)
            zzb[j] = p + master_gap_open(v1);
    }

    // left half of the matrix
    p = st;
    ctri = v3 / 2;
    for (i=1; i<=ctri; i++)
    {
        n = zza[0];
        p += master_gap_extend(v1+i+AF);
        k = p;
        IF_MATRIX_DUMP(vertical[i][0]=)
            zza[0] = k;
        l = p + master_gap_open(v1+i+AF);

        for (j=1; j<=v4; j++)
        {
            // from above (gap in master (behind position i))
            IF_MATRIX_DUMP(verticalOpen[i][j]=)         k += master_gap_open(v1+i+AF)+master_gap_extend(v1+i+AF);       // (1)
            IF_MATRIX_DUMP(vertical[i][j]=)             l += master_gap_extend(v1+i+AF);                                // (2)
            if (k<l) l = k;     // l=min((1),(2))

            // from left (gap in slave (behind position j))
            IF_MATRIX_DUMP(horizontalOpen[i][j]=)       k = zza[j] + slave_gap_open(v2+j+AF)+slave_gap_extend(v2+j+AF);         // (3)
            IF_MATRIX_DUMP(horizontal[i][j]=)           m = zzb[j] + slave_gap_extend(v2+j+AF);                                 // (4)
            if (k<m) m = k;     // m=min((3),(4))

            // diagonal (no gaps)
            IF_MATRIX_DUMP(diagonal[i][j]=)             k = n + calc_weight(i, j, v1, v2);                              // (5)
            if (l<k) k = l;
            if (m<k) k = m;     // k = minimum of all paths

            n = zza[j];         // minimum of same row; one column to the left
            zza[j] = k;         // minimum of all paths to this matrix position
            zzb[j] = m;         // minimum of those two paths, where gap was inserted into slave
        }
    }

# define MHO 1  // X-Offset for second half of matrix-arrays (only used to insert MID-column into dumpMatrix())
# define BO  1  // Offset for second half of matrix (cause now the indices i and j are smaller as above)

    // last column of matrix:

    zzb[0]=zza[0];
    IF_MATRIX_DUMP(vertical[v3+1+MHO][v4+1]=)
        zzc[v4]=0;
    p = master_gap_open(v1+v3);
    for (j=v4-1; j>-1; j--)
    {
        p += master_gap_extend(v1+v3);
        IF_MATRIX_DUMP(vertical[v3+1+MHO][j+BO]=)
            zzc[j] = p;
        IF_MATRIX_DUMP(verticalOpen[v3+1+MHO][j+BO]=)
            zzd[j] = p+master_gap_open(v1+v3);
    }

    // right half of matrix (backwards):
    p = en;
    for (i=v3-1; i>=ctri; i--)
    {
        n = zzc[v4];
        p += master_gap_extend(v1+i);
        k = p;
        IF_MATRIX_DUMP(vertical[i+BO+MHO][v4+1]=)
            zzc[v4] = k;
        l = p+master_gap_open(v1+i);

        for (j=v4-1; j>=0; j--)
        {
            // from below (gap in master (in front of position (i+BO)))
            IF_MATRIX_DUMP(verticalOpen[i+BO+MHO][j+BO]=)       k += master_gap_open(v1+i)+master_gap_extend(v1+i);     // (1)
            IF_MATRIX_DUMP(vertical[i+BO+MHO][j+BO]=)           l += master_gap_extend(v1+i);                           // (2)
            if (k<l) l = k;     // l=min((1),(2))

            // from right (gap in slave (in front of position (j+BO)))
            IF_MATRIX_DUMP(horizontalOpen[i+BO+MHO][j+BO]=)     k = zzc[j] + slave_gap_open(v2+j) + slave_gap_extend(v2+j);     // (3)
            IF_MATRIX_DUMP(horizontal[i+BO+MHO][j+BO]=)         m = zzd[j] + slave_gap_extend(v2+j);                            // (4)
            if (k<m) m = k;     // m=min((3),(4))

            // diagonal (no gaps)
            IF_MATRIX_DUMP(diagonal[i+BO+MHO][j+BO]=)           k = n + calc_weight(i+BO, j+BO, v1, v2);                // (5)
            if (l<k) k = l;
            if (m<k) k = m;     // k = minimum of all paths

            n = zzc[j];         // minimum of same row; one column to the right
            zzc[j] = k;         // minimum of all paths to this matrix position
            zzd[j] = m;         // minimum of those two paths, where gap was inserted into slave
        }
    }

#undef BO

    // connect rightmost column of first half (column ctri)
    // with leftmost column of second half (column ctri+1)

    zzd[v4] = zzc[v4];

    ctrc = INT_MAX;
    flag = 0;

    for (j=(ctri ? 0 : 1); j<=v4; j++)
    {
        IF_MATRIX_DUMP(vertical[ctri+MHO][j]=)
            k = zza[j] + zzc[j];                // sum up both calculations (=diagonal=no gaps)

        if (k<ctrc || ((k==ctrc) && (zza[j]!=zzb[j]) && (zzc[j]==zzd[j])))
        {
            ctrc = k;
            ctrj = j;
            flag = 1;
        }
    }

    for (j=v4; j>=(ctri ? 0 : 1); j--)
    {
        IF_MATRIX_DUMP(verticalOpen[ctri+MHO][j]=)
            k = zzb[j] + zzd[j]                 // paths where gaps were inserted into slave (left and right side!)
            - slave_gap_open(j);                // subtract gap_open-penalty which was added twice (at left and right end of gap)

        if (k<ctrc)
        {
            ctrc = k;
            ctrj = j;
            flag = 2;
        }
    }

    awtc_assert(flag>=1 && flag<=2);

#undef MHO

#ifdef MATRIX_DUMP
    if (display_matrix)
        dumpMatrix(v1, v2, v3, v4, ctri);
#endif

    /* Conquer recursively around midpoint  */

    if (flag==1)                /* Type 1 gaps  */
    {
        diff(v1, v2, ctri, ctrj, st, master_gap_open(v1+ctri));                 // includes midpoint ctri and ctrj
        diff(v1+ctri, v2+ctrj, v3-ctri, v4-ctrj, master_gap_open(v1+ctri), en);
    }
    else
    {
        diff(v1, v2, ctri-1, ctrj, st, 0);                                      // includes midpoint ctrj

        if (last_print<0 && print_ptr>0) /* Delete 2 */
            last_print = decr_displ(print_ptr-1, 2);
        else
            last_print = set_displ(print_ptr++, -2);

        diff(v1+ctri+1, v2+ctrj, v3-ctri-1, v4-ctrj, 0, en);
    }

    deep--;
    return ctrc;       /* Return the score of the best alignment */
}

static void do_align( /* int v1, */ int *score, long act_seq_length)
{
    int i, j, k, l1, l2, n;
    int t_arr[MAX_BASETYPES];

    l1=l2=pos1=pos2=0;

    // clear statistics

    for (i=1; i<=act_seq_length; ++i) {
        for (j=0; j<MAX_BASETYPES; ++j) {
            naa1[j][i] = naa2[j][i]=0;
            naas[j][i]=0;
        }
    }

    // create position statistics for each group
    // [here every group contains only one seq]

    for (i=1; i<=nseqs; ++i) {
        if (group[i]==1) {
            fst_list[++pos1]=i;
            for (j=1; j<=seqlen_array[i]; ++j) {
                unsigned char b = seq_array[i][j];
                if (b<128) {
                    ++naa1[b][j];
                    ++naa1[0][j];
                }
            }
            if (seqlen_array[i]>l1) l1=seqlen_array[i];
        }
        else if (group[i]==2)
        {
            snd_list[++pos2]=i;
            for (j=1; j<=seqlen_array[i]; ++j) {
                unsigned char b = seq_array[i][j];
                if (b<128) {
                    ++naa2[b][j];
                    ++naa2[0][j];
                }
            }
            if (seqlen_array[i]>l2) l2=seqlen_array[i];
        }
    }

    if (pos1>=pos2) {
        for (i=1; i<=pos2; ++i) alist[i]=snd_list[i];
        for (n=1; n<=l1; ++n)
        {
            for (i=1; i<MAX_BASETYPES; ++i) t_arr[i]=0;
            for (i=1; i<MAX_BASETYPES; ++i) {
                if (naa1[i][n]>0) {
                    for (j=1; j<MAX_BASETYPES; ++j) {
                        t_arr[j] += (weights[i][j]*naa1[i][n]);
                    }
                }
            }
            k = naa1[0][n];
            if (k>0) {
                for (i=1; i<MAX_BASETYPES; ++i) {
                    naas[i][n]=t_arr[i]/k;
                }
            }
        }
    }
    else {
        MAXN_2_assert(0);       // should never occur if MAXN==2
#if (MAXN!=2)
        for (i=1; i<=pos1; ++i) alist[i]=fst_list[i];
        for (n=1; n<=l2; ++n) {
            for (i=1; i<MAX_BASETYPES; ++i) t_arr[i]=0;
            for (i=1; i<MAX_BASETYPES; ++i)
                if (naa2[i][n]>0)
                    for (j=1; j<MAX_BASETYPES; ++j)
                        t_arr[j] += (weights[i][j]*naa2[i][n]);
            k = naa2[0][n];
            if (k>0)
                for (i=1; i<MAX_BASETYPES; ++i)
                    naas[i][n]=t_arr[i]/k;
        }
#endif
    }

    *score=diff(1, 1, l1, l2, master_gap_open(1), master_gap_open(l1+1)); /* Myers and Miller alignment now */
}

static int add_ggaps(long /* max_seq_length */)
{
    int i, j, k, pos, to_do;

    pos=1;
    to_do=print_ptr;

    for (i=0; i<to_do; ++i)     // was: 1 .. <=to_do
    {
        if (displ[i]==0)
        {
            result[1][pos]=result[2][pos]='*';
            ++pos;
        }
        else
        {
            if ((k=displ[i])>0)
            {
                for (j=0; j<=k-1; ++j)
                {
                    result[2][pos+j]='*';
                    result[1][pos+j]='-';
                }
                pos += k;
            }
            else
            {
                k = (displ[i]<0) ? displ[i] * -1 : displ[i];
                for (j=0; j<=k-1; ++j)
                {
                    result[1][pos+j]='*';
                    result[2][pos+j]='-';
                }
                pos += k;
            }
        }
    }

#ifdef DEBUG
    result[1][pos] = 0;
    result[2][pos] = 0;
#endif

    return pos-1;
}


static int res_index(const char *t, char c)
{
    if (t[0]==c) return UNKNOWN_ACID;
    for (int i=1; t[i]; i++) {
        if (t[i]==c) return i;
    }
    return 0;
}

static void p_encode(const unsigned char *seq, unsigned char *naseq, int l) /* code seq as ints .. use -2 for gap */ {
    bool warned = false;

    for (int i=1; i<=l; i++) {
        int c = res_index(amino_acid_order, seq[i]);

        if (!c) {
            if (seq[i] == '*') {
                c = -2;
            }
            else {
                if (!warned && seq[i] != '?') { // consensus contains ? and it means X
                    char buf[100];
                    sprintf(buf, "Illegal character '%c' in sequence data", seq[i]);
                    aw_message(buf);
                    warned = true;
                }
                c = res_index(amino_acid_order, 'X');
            }
        }

        awtc_assert(c>0 || c == -2);
        naseq[i] = c;
    }
}

static void n_encode(const unsigned char *seq, unsigned char *naseq, int l)
{                                       /* code seq as ints .. use -2 for gap */
    int i;
    int warned = 0;

    for (i=1; i<=l; i++) {
        int c = res_index(nucleic_acid_order, seq[i]);

        if (!c) {
            if (!warned && seq[i] != '?') {
                char buf[100];
                sprintf(buf, "Illegal character '%c' in sequence data", seq[i]);
                aw_message(buf);
                warned = 1;
            }
            c = res_index(nucleic_acid_order, 'N');
        }

        awtc_assert(c>0);
        naseq[i] = c;
    }
}

ARB_ERROR AWTC_ClustalV_align(int          is_dna,
                              int          weighted,
                              const char  *seq1,
                              int          length1,
                              const char  *seq2,
                              int          length2,
                              const int   *gapsBefore1,
                              int          max_seq_length,
                              char       **resultPtr1,
                              char       **resultPtr2,
                              int         *resLengthPtr,
                              int *score)
{
    ARB_ERROR error;
    gapsBeforePosition = gapsBefore1;

    if (!module_initialized) { // initialize only once
        dnaflag = is_dna;
        is_weight = weighted;

        error             = init_myers(max_seq_length);
        if (!error) error = init_show_pair(max_seq_length);
        make_pamo(0);
        fill_pam();

        for (int i=1; i<=2 && !error; i++) {
            seq_array[i] = (unsigned char*)ckalloc((max_seq_length+2)*sizeof(char), error);
            result[i] = (char*)ckalloc((max_seq_length+2)*sizeof(char), error);
            group[i] = i;
        }

        if (!error) module_initialized = true;
    }
    else {
        if (dnaflag!=is_dna || is_weight!=weighted) {
            error = "Please call AWTC_ClustalV_align_exit() between calls that differ in\n"
                "one of the following parameters:\n"
                "       is_dna, weighted";
        }
    }

    if (!error) {
        nseqs = 2;
        print_ptr = 0;

#if defined(DEBUG) && 1
        memset(&seq_array[1][1], 0, max_seq_length*sizeof(seq_array[1][1]));
        memset(&seq_array[2][1], 0, max_seq_length*sizeof(seq_array[2][1]));
        memset(&displ[1], 0xff, max_seq_length*sizeof(displ[1]));
        seq_array[1][0] = '_';
        seq_array[2][0] = '_';
#endif

        {
            void (*encode)(const unsigned char*, unsigned char*, int) = dnaflag ? n_encode : p_encode;

            encode((const unsigned char*)(seq1-1), seq_array[1], length1);
            seqlen_array[1] = length1;
            encode((const unsigned char*)(seq2-1), seq_array[2], length2);
            seqlen_array[2] = length2;

            do_align(score, max(length1, length2));
            int alignedLength = add_ggaps(max_seq_length);

            *resultPtr1   = result[1]+1;
            *resultPtr2   = result[2]+1;
            *resLengthPtr = alignedLength;
        }
    }

    return error;
}

void AWTC_ClustalV_align_exit(void)
{
    if (module_initialized) {
        module_initialized = false;

        for (int i=1; i<=2; i++) {
            free(result[i]);
            free(seq_array[i]);
        }
        exit_show_pair();
        exit_myers();
    }
}
