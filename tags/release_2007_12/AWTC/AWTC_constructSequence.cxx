/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 1998                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ctype.h>
#include <string.h>
// #include <sys/time.h>
#include <time.h> // SuSE 7.3

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>

#include "awtc_fast_aligner.hxx"
#include "awtc_seq_search.hxx"
#include "awtc_constructSequence.hxx"

#define SAME_SEQUENCE (-1),(-1)

static inline int min(int i1, int i2)           {return i1<i2 ? i1 : i2;}

static inline int basesMatch(char c1, char c2)
{
    return toupper(c1)==toupper(c2);
}
static inline int inversBasesMatch(char c1, char c2)
{
    c1 = toupper(c1);
    c2 = toupper(c2);

    switch (c1) {
        case 'A': return c2=='T' || c2=='U';
        case 'C': return c2=='G';
        case 'G': return c2=='C';
        case 'T':
        case 'U': return c2=='A';
        case 'N': return 1;
        default: awtc_assert(0);
    }

    return c1==c2;
}

static inline char *strndup(const char *seq, int length)
{
    char *neu = new char[length+1];

    memcpy(neu, seq, length);
    neu[length] = 0;

    return neu;
}
static char *lstr_ss = 0;
static inline const char *lstr(const char *s, int len)
{

    if (lstr_ss) delete [] lstr_ss;
    lstr_ss = strndup(s,len);
    return lstr_ss;
}
#ifdef DEBUG
static inline void dumpPart(int num, const AWTC_CompactedSubSequence *comp)
{
    printf("[%02i] ",num);

#define SHOWLEN 40

    const char *text = comp->text();
    int len = comp->length();

    awtc_assert(len);

    if (len<=SHOWLEN)
    {
        printf("'%s'\n", lstr(text,len));
    }
    else
    {
        printf("'%s...", lstr(text, SHOWLEN));
        printf("%s'\n", lstr(text+len-SHOWLEN, SHOWLEN));
    }
}
#else
static inline void dumpPart(int, const AWTC_CompactedSubSequence *) {}
#endif


#ifdef DEBUG
# define TEST_UNIQUE_SET
#endif
#define UNIQUE_VALUE (-123)

// ---------
// class Way
// ---------

class Way
{
    int *my_way;        // 1..n for normal parts -1..-n for reverse parts
    int my_length;
    int my_score;
    int my_maxlength;

public:

    Way(int maxlength) : my_length(0), my_score(0), my_maxlength(maxlength) {my_way = new int[maxlength];}
    ~Way()                                  {delete [] my_way;}

    Way(const Way& w)
    {
        my_maxlength    = w.my_maxlength;
        my_length   = w.my_length;
        my_score    = w.my_score;

        my_way      = new int[my_maxlength];

        memcpy(my_way, w.my_way, sizeof(my_way[0])*my_length);
    }

    Way& operator=(const Way& w)
    {
        if (&w!=this)
        {
            if (my_maxlength<w.my_maxlength)
            {
                delete [] my_way;
                my_way = new int[w.my_maxlength];
            }

            my_maxlength    = w.my_maxlength;
            my_length       = w.my_length;
            my_score        = w.my_score;

            memcpy(my_way, w.my_way, sizeof(my_way[0])*my_length);
        }

        return *this;
    }

    void add(int partNum, int reverse, int Score) {
        awtc_assert(my_length<my_maxlength);
        partNum++;  // recode cause partNum==0 cannot be negative
        my_way[my_length++] = reverse ? -partNum : partNum;
        my_score += Score;
    }
    void shorten(int Score) {
        my_length--;
        my_score -= Score;
    }

    int score() const       {return my_score; /*my_length ? my_score/my_length : 0;*/}
    int length() const      {return my_length;}
    int way(int num, int& reverse) const {reverse = my_way[num]<0; return abs(my_way[num])-1;}

    void dump() const {
        int l;

        printf("Way my_length=%2i my_score=%5i score()=%5i:", my_length, my_score, score());

        for (l=0; l<my_length; l++) {
            int reverse;
            int partNum = way(l, reverse);
            printf(" %3i%c", partNum, reverse ? 'r' : ' ');
        }

        printf("\n");
    }
};

// -------------
// class Overlap
// -------------

class Overlap           // matrix which stores overlap data
{
    int parts;          // no of parts
    int *ol;            // how much the both sequence parts overlap
    int *sc;            // which score we calculated for this overlap

    int offset(int f,int fReverse, int t, int tReverse) const
    {
        int off = parts*2 * (t*2 + tReverse)  +  (f*2 + fReverse);
        return off;
    }

    void findWayFrom(Way *w, Way *best, int partNum, int reverse, int *used, int minMatchingBases) const;

public:

    Overlap(int Parts)
    {
        parts = Parts;
        int sizeq = parts*2*parts*2;
        ol = new int[sizeq];
        sc = new int[sizeq];

#ifdef TEST_UNIQUE_SET
        int off;
        for (off=0; off<sizeq; off++)
        {
            ol[off] = UNIQUE_VALUE;
            sc[off] = UNIQUE_VALUE;
        }
#endif
    }

    ~Overlap() { delete [] ol; delete [] sc; }

    int overlap(int f, int fReverse, int t, int tReverse) const             {return ol[offset(f,fReverse,t,tReverse)];}
    int score(int f, int fReverse, int t, int tReverse) const               {return sc[offset(f,fReverse,t,tReverse)];}

    void set(int off, int theOverlap, int theScore)
    {
#ifdef TEST_UNIQUE_SET
        awtc_assert(ol[off]==UNIQUE_VALUE);
        awtc_assert(sc[off]==UNIQUE_VALUE);
#endif
        ol[off] = theOverlap;
        sc[off] = theScore;
    }
    void set(int f,int fReverse, int t, int tReverse, int theOverlap, int theScore)     {set(offset(f,fReverse,t,tReverse), theOverlap, theScore);}

    void setall(int f, int t, int theOverlap, int theScore)
    {
        int off = offset(f,0,t,0);

        set(off,        theOverlap, theScore);
        set(off+1,      theOverlap, theScore);
        set(off+parts*2,    theOverlap, theScore);
        set(off+parts*2+1,  theOverlap, theScore);
    }

    Way findWay(int minMatchingBases) const;
    void dump() const;
};

void Overlap::findWayFrom(Way *w, Way *best, int partNum, int reverse, int *used, int minMatchingBases) const
{
    used[partNum] = reverse ? -1 : 1;

    int l;

    for (l=0; l<parts; l++)
    {
        if (!used[l])
        {
            int rev;

            for (rev=0; rev<2; rev++)
            {
                int scoreForPartialWay = score(partNum, reverse, l, rev);

                if (scoreForPartialWay>=minMatchingBases)
                {
                    w->add(l, rev, scoreForPartialWay);
                    if (w->score() > best->score()) {
                        *best = *w;
                        printf("new best ");
                        best->dump();
                    }

                    findWayFrom(w, best, l, rev, used, minMatchingBases);
                    w->shorten(scoreForPartialWay);
                }
            }
        }
    }

    used[partNum] = 0;
}

Way Overlap::findWay(int minMatchingBases) const
{
    int l;
    int *used = new int[parts];
    Way w(parts);
    Way best(parts);

    for (l=0; l<parts; l++) {
        used[l] = 0;
    }

    for (l=0; l<parts; l++) {
        int rev;
        for (rev=0; rev<2; rev++) {
            w.add(l,rev,0);
            findWayFrom(&w, &best, l, rev, used, minMatchingBases);
            w.shorten(0);
        }
    }

    delete [] used;
    return best;
}

void Overlap::dump() const
{
#ifdef DEBUG
    printf("-------------------------------\n");
    for (int y=-1; y<parts; y++) {
        for (int yR=0; yR<2; yR++) {
            for (int x=-1; x<parts; x++) {
                for (int xR=0; xR<2; xR++) {
                    if (x==-1 || y==-1) {
                        if (!xR && !yR)     printf("%4i   ", x==-1 ? y : x);
                        else        printf("       ");
                    } else {
                        printf("%3i|%-3i", overlap(x,xR,y,yR), score(x,xR,y,yR));
                    }
                }
            }
            printf("\n");
        }
    }
    printf("-------------------------------\n");
#endif
}

// ------------------------------------------------------------------------------------------------------------------------

static inline int calcScore(int overlap, int mismatches)
{
    return overlap-2*mismatches;
}
void overlappingBases(AWTC_CompactedSubSequence *comp1, int reverse1, AWTC_CompactedSubSequence *comp2, int reverse2, int& bestOverlap, int& bestScore)
{
    int len = 1;
    int maxcmp = min(comp1->length(), comp2->length());

    bestOverlap = 0;
    bestScore   = 0;

    if (reverse1)
    {
        if (reverse2)   // both are reversed -> compare reverse start of comp1 with reverse end of comp2
        {
            const char *start1 = comp1->text();
            const char *end2 = comp2->text(comp2->length());

            while (len<=maxcmp)
            {
                int l;
                int mismatches = 0;

                for (l=0; l<len; l++) {
                    if (!basesMatch(start1[-l], end2[-l])) {
                        mismatches++;
                    }
                }

                int score = calcScore(len, mismatches);
                if (score>=bestScore) {
                    bestOverlap = len;
                    bestScore   = score;
                }

                len++;
                start1++;
            }
        }
        else    // comp1 is reversed -> compare reverse start of comp1 with start of comp2
        {
            const char *start1 = comp1->text();
            const char *start2 = comp2->text();

            while (len<=maxcmp)
            {
                int l;
                int mismatches = 0;

                for (l=0; l<len; l++) {
                    if (!inversBasesMatch(start1[-l], start2[l])) {
                        mismatches++;
                    }
                }

                int score = calcScore(len, mismatches);
                if (score>=bestScore) {
                    bestOverlap = len;
                    bestScore   = score;
                }

                len++;
                start1++;
            }
        }
    }
    else if (reverse2)  // comp2 is reversed -> compare end of comp1 with reverse end of comp2
    {
        const char *end1 = comp1->text(comp1->length()-1);
        const char *end2 = comp2->text(comp2->length()-1);

        while (len<=maxcmp)
        {
            int l;
            int mismatches = 0;

            for (l=0; l<len; l++) {
                if (!inversBasesMatch(end1[l],end2[-l])) {
                    mismatches++;
                }
            }

            int score = calcScore(len, mismatches);
            if (score>=bestScore) {
                bestOverlap = len;
                bestScore   = score;
            }

            len++;
            end1--;
        }
    }
    else    // both normal -> compare end of comp1 whith start of comp2
    {
        const char *end1 = comp1->text(comp1->length()-1);
        const char *start2 = comp2->text();

        while (len<=maxcmp)
        {
            int l;
            int mismatches = 0;

            for (l=0; l<len; l++) {
                if (!basesMatch(end1[l], start2[l])) {
                    mismatches++;
                }
            }

            int score = calcScore(len, mismatches);
            if (score>=bestScore) {
                bestOverlap = len;
                bestScore   = score;
            }

            len++;
            end1--;
        }
    }
}

typedef AWTC_CompactedSubSequence *AWTC_CompactedSubSequencePointer;

char *AWTC_constructSequence(int parts, const char **seqs, int minMatchingBases, char **refSeq)
{
    Overlap lap(parts);
    AWTC_CompactedSubSequencePointer *comp = new AWTC_CompactedSubSequencePointer[parts];

    int s;
    for (s=0; s<parts; s++) {
        comp[s] = new AWTC_CompactedSubSequence(seqs[s], strlen(seqs[s]), "contructed by AWTC_constructSequence()");
        if (comp[s]->length()==0) {
            printf("AWTC_constructSequence called with empty sequence (seq #%i)\n", s);
            return NULL;
        }
    }

    for (s=0; s<parts; s++) {
        dumpPart(s,comp[s]);
        lap.setall(s,s,SAME_SEQUENCE);  // set diagonal entries to SAME_SEQUENCE (= "a sequence can't overlap with itself")
        for (int s2=s+1; s2<parts; s2++) {
            awtc_assert(s!=s2);
            for (int sR=0; sR<2; sR++) {
                for (int s2R=0; s2R<2; s2R++) {
                    awtc_assert(s!=s2);
                    int overlap;
                    int score;

                    overlappingBases(comp[s], sR, comp[s2], s2R, overlap, score);

                    lap.set(s, sR, s2, s2R, overlap, score);
                    lap.set(s2, !s2R, s, !sR, overlap, score);
                }
            }
        }
    }

    lap.dump();

    // find the best way through the array

    Way w = lap.findWay(minMatchingBases);
    w.dump();

    // concatenate parts

    int sequenceLength = 0;
    for (s=0; s<w.length(); s++)    // calculate length
    {
        int rev2;
        int part2 = w.way(s,rev2);

        sequenceLength += comp[part2]->length();

        if (s)
        {
            int rev1;
            int part1 = w.way(s-1, rev1);

            sequenceLength -= lap.overlap(part1, rev1, part2, rev2);
        }
    }

    char *resultSeq = new char[sequenceLength+1];
    *refSeq = new char[sequenceLength+1];
    int off = 0;

    for (s=0; s<w.length(); s++)    // construct sequence
    {
        int rev2;
        int part2 = w.way(s,rev2);

        sequenceLength += comp[part2]->length();

        if (s)
        {
            int rev1;
            int part1 = w.way(s-1, rev1);

            sequenceLength -= lap.overlap(part1, rev1, part2, rev2);

            // @@@@ hier fehlt noch fast alles

        }
    }

    // frees

    for (s=0; s<parts; s++) {
        delete comp[s];
    }
    delete [] comp;

    return NULL;
}

#ifdef DEBUG
# define TEST_THIS_MODULE
#endif

#ifdef TEST_THIS_MODULE
// typedef const char *cstr;
// typedef char *str;

#define PARTS           10
#define OVERLAPPING_BASES   100

static inline int startOf(int len, int partNum)
{
    awtc_assert(len>0);
    awtc_assert(partNum>0);
    int s = (len/PARTS)*partNum - rand()%OVERLAPPING_BASES;
    return s>0 ? (s<len ? s : len-1) : 0;
}

static inline int lengthOf(int start, int len)
{
    awtc_assert(len>0);
    awtc_assert(start>=0);
    int s = len/PARTS + rand()%(2*OVERLAPPING_BASES);
    return s>0 ? ((start+s)<=len ? s : len-start) : 1;
}

char *AWTC_testConstructSequence(const char *testWithSequence)
{
    int basesInSeq = 0;
    char *compressed = strdup(testWithSequence);

    {
        const char *s = testWithSequence;
        while(*s)
        {
            if (*s!='-' && *s!='.')
            {
                compressed[basesInSeq++] = *s;
            }
            s++;
        }
    }

    int    parts = PARTS;
    char **part  = new char*[PARTS];
    int    p;

    printf("AWTC_testConstructSequence: len(=no of bases) = %5i\n", basesInSeq);

    long randSeed = time(NULL);
    //    randSeed = 22061965;
    //    randSeed = 860485236;
    printf("randSeed=%li\n", randSeed);
    srand(randSeed);

    int last_end = -1;

    for (p=0; p<parts; p++)
    {
        int start = p==0 ? 0 : last_end - (25+(rand()*75)/RAND_MAX);
        if (start<0)            start = 0;
        else if (start>=basesInSeq)     start = basesInSeq-1;

        int llen = p<PARTS-1 ? (rand()*(basesInSeq/parts+200))/RAND_MAX : basesInSeq-start;
        if (start+llen > basesInSeq)    llen = basesInSeq-start;

        int end = start+llen-1;
        awtc_assert(end<basesInSeq);

        int overlap = last_end-start+1;
        awtc_assert(overlap>0 || p==0);

        int count = 0;
        int l;
        for (l=0; l<overlap; l++)
        {
            if (strchr("-.",compressed[last_end+l])==NULL)
                count++;
        }

        printf("[%02i] start=%-5i llen=%-5i end=%-5i overlap=%-5i basesInOverlap=%-5i", p, start, llen, end, overlap, count);
        last_end = end;
        awtc_assert(start+llen<=basesInSeq);
        part[p] = strndup(compressed+start, llen);
        if (rand()%2)
        {
            char     T_or_U;
            GB_ERROR error = GBT_determine_T_or_U(GB_AT_RNA, &T_or_U, "reverse-complement");
            if (error) aw_message(error);
            GBT_reverseComplementNucSequence(part[p], llen, T_or_U);
            printf(" (reverted)");
        }
        printf("\n");
    }

    for (p=0; p<parts; p++)
    {
        int   l;
        int   llen = strlen(part[p]);
        char *s    = part[p];

        for (l=0; l<llen; l++)
        {
            if (s[l]!='-' && s[l]!='.')
            {
                break;
            }
        }

        if (l==llen)    // seq is empty
        {
            int q;

            for (q=p+1; q<parts; q++)
            {
                part[q-1] = part[q];
            }
            parts--;    // skip it
            p--;
        }

        part[p] = s;
    }

    if (parts<PARTS) printf("deleted %i empty parts\n", PARTS-parts);

    for (p=0; p<parts; p++) // insert some errors into sequences
    {
        int   changes = 0;
        char *s       = part[p];

        while (*s)
        {
            if (strchr("-.", *s)==NULL)
            {
                if ((double(rand())/double(RAND_MAX)) < 0.05 )      // error-probability
                {
                    *s = "ACGT"[rand()%4];
                    changes++;
                }
            }
            s++;
        }
        printf("[%02i] base-errors = %i\n", p, changes);
    }

    char *neu = AWTC_constructSequence(parts, (const char**)part, 10, NULL);

    for (p=0; p<parts; p++) delete [] part[p];
    delete [] part;

    if (!neu) printf("AWTC_constructSequence() returned NULL\n");

    return neu;
}

#endif
/* TEST_THIS_MODULE */

