// =============================================================== //
//                                                                 //
//   File      : seq_search.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de)                   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "seq_search.hxx"

#include <arbdbt.h>
#include <cstdarg>
#include <climits>
#include <cctype>

#define MESSAGE_BUFFERSIZE 300

void messagef(const char *format, ...)
{
    va_list argp;
    char buffer[MESSAGE_BUFFERSIZE];

    va_start(argp, format);

    IF_ASSERTION_USED(int chars =) vsprintf(buffer, format, argp);
    fa_assert(chars<MESSAGE_BUFFERSIZE);

    va_end(argp);
    GB_warning(buffer);
}

void Points::append(Points *neu) {
    if (Next) Next->append(neu);
    else Next = neu;
}

CompactedSequence::CompactedSequence(const char *Text, int Length, const char *name, int start_offset)
{
    long cPos;
    long xPos;

    fa_assert(Length>0);
    
    long *compPositionTab = new long[Length];
    myLength = 0;
    int lastWasPoint = 0;

    myStartOffset = start_offset;
    myEndOffset = start_offset+Length-1;

    fa_assert(name);

    points = NULL;
    myName = strdup(name);

    long firstBase = 0;
    long lastBase = Length-1;

    for (xPos=0; xPos<Length; xPos++) {         // convert point gaps at beginning to dash gaps
        char c = Text[xPos];
        if (!is_gap(c)) {
            firstBase = xPos;
            break;
        }
    }

    for (xPos=Length-1; xPos>=0; xPos--)        // same for end of sequence
    {
        char c = Text[xPos];
        if (!is_gap(c)) {
            lastBase = xPos;
            break;
        }
    }

    for (xPos=0; xPos<Length; xPos++) {
        char c = toupper(Text[xPos]);

        if (is_gap(c)) {
            if (c=='-' || xPos<firstBase || xPos>lastBase) {
                compPositionTab[xPos] = -1;                     // a illegal index
                lastWasPoint = 0;
            }
            else {
                fa_assert(c=='.');

                if (!lastWasPoint) {
                    lastWasPoint = 1;
                    storePoints(myLength);
                }

                compPositionTab[xPos] = -1;
            }

        }
        else {
            compPositionTab[xPos] = myLength++;
            lastWasPoint = 0;
        }
    }

    myText           = new char[myLength+1];
    myText[myLength] = 0;

    expdPositionTab = new int[myLength+1];      // plus one extra element
    expdPositionTab[myLength] = Length;         // which is the original length

    gapsBeforePosition = new int[myLength+1];

    for (xPos=0; xPos<Length; xPos++) {
        cPos = compPositionTab[xPos];
        fa_assert(cPos<myLength);

        if (cPos>=0) {
            myText[cPos] = toupper(Text[xPos]);
            expdPositionTab[cPos] = xPos;
            gapsBeforePosition[cPos] =
                cPos
                ? xPos-expdPositionTab[cPos-1]-1
                : xPos;
        }
    }

    if (myLength>0) {
        gapsBeforePosition[myLength] = Length - expdPositionTab[myLength-1]; // gaps before end of sequence
    }

    referred = 1;

    delete [] compPositionTab;
}

CompactedSequence::~CompactedSequence()
{
    fa_assert(referred==0);

    delete [] myText;

    delete[] expdPositionTab;
    delete[] gapsBeforePosition;

    free(myName);
    
    delete points;
}

int CompactedSequence::compPosition(int xPos) const {
    // returns the number of bases left of 'xPos' (not including bases at 'xPos')

    int l = 0,
        h = length();

    while (l<h) {
        int m = (l+h)/2;
        int cmp = xPos-expdPosition(m);

        if (cmp==0) {
            return m; // found!
        }

        if (cmp<0) { // xPos < expdPosition(m)
            h = m;
        }
        else { // xPos > expdPosition(m)
            l = m+1;
        }
    }

    fa_assert(l == h);
    return l;
}

FastAlignInsertion::~FastAlignInsertion()
{
    if (myNext) delete myNext;
}

FastSearchSequence::FastSearchSequence(const CompactedSubSequence& seq)
{
    memset((char*)myOffset, 0, MAX_TRIPLES*sizeof(*myOffset));
    SequencePosition triple(seq);

    mySequence = &seq;

    while (triple.rightOf()>=3) // enough text for triple?
    {
        int tidx = triple_index(triple.text());
        TripleOffset *top = new TripleOffset(triple.leftOf(), myOffset[tidx]);

        myOffset[tidx] = top;
        ++triple;
    }
}

FastSearchSequence::~FastSearchSequence() {
    for (int tidx = 0; tidx<MAX_TRIPLES; ++tidx) {
        delete myOffset[tidx];
    }
}

void AlignBuffer::correctUnalignedPositions()
{
    long off  = 0;
    long rest = used;

    while (rest) {
        const char *qual  = quality();
        char       *found = (char*)memchr(qual+off, '?', rest); // search for next '?' in myQuality
        
        if (!found || (found-qual) >= rest) break;

        long cnt;
        for (cnt=0; found[cnt]=='?'; cnt++) found[cnt] = '+';   // count # of unaligned positions and change them to '+'

        long from  = found-myQuality;
        long b_off = from-1;   // position before unaligned positions
        long a_off = from+cnt; // positions after unaligned positions

        long before, after;
        for (before=0; b_off>=0 && isGlobalGap(b_off); before++, b_off--) ;             // count free positions before unaligned positions
        for (after=0; a_off<used && isGlobalGap(a_off); after++, a_off++) ;             // count free positions after unaligned positions

        if (b_off<=0) before = LONG_MAX;
        if (a_off>=used) after = LONG_MAX;

        if (before<after)               // move to the left
        {
            if (before)
            {
                long to = from-before;
                while (cnt--) moveUnaligned(from++, to++);
            }
        }
        else if (after!=LONG_MAX)       // move to the right
        {
            if (after)
            {
                from += cnt-1;          // copy from right to left!
                long to = from+after;

                while (cnt--) moveUnaligned(from--, to--);
            }
        }

        rest -= (a_off-off);
        off = a_off;
    }
}

void AlignBuffer::expandPoints(CompactedSubSequence& slaveSequence)
{
    long rest = used;
    long off = 0;               // real position in sequence
    long count = 0;             // # of bases left of this position
    long nextPoint = slaveSequence.firstPointPosition();

    while (nextPoint!=-1 && rest)
    {
        unsigned char gap = myBuffer[off];
        if (gap=='-' || gap=='.') {
            if (nextPoint==count) {
                myBuffer[off] = '.';
            }
        }
        else
        {
            count++;
            if (nextPoint==count) {
                messagef("Couldn't insert point-gap at offset %li of '%s' (gap removed by aligner)",
                         off, slaveSequence.name());
            }
            if (nextPoint<count) {
                nextPoint = slaveSequence.nextPointPosition();
            }
        }

        off++;
        rest--;
    }
}

void AlignBuffer::point_ends_of()
{
    int i;

    for (i=0; (myBuffer[i]=='.' || myBuffer[i]=='-') && i<used; i++) {
        myBuffer[i] = '.';
    }

    for (i=used-1; (myBuffer[i]=='.' || myBuffer[i]=='-') && i>=0; i--) {
        myBuffer[i] = '.';
    }
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>
#include <test_helpers.h>

struct bind_css {
    CompactedSubSequence& css;
    int test_mode;
    bind_css(CompactedSubSequence& css_) : css(css_), test_mode(-1) {}

    int operator()(int i) const {
        switch (test_mode) {
            case 0: return css.compPosition(i);
            case 1: return css.expdPosition(i);
            case 2: return css[i];
            case 3: return css.no_of_gaps_before(i);
            case 4: return css.no_of_gaps_after(i);
        }
        fa_assert(0);
        return -666;
    }
};

#define TEST_ASSERT_CSS_SELF_REFLEXIVE(css) do {        \
        for (int b = 0; b<css.length(); ++b) {          \
            int x = css.expdPosition(b);                \
            int c = css.compPosition(x);                \
            TEST_ASSERT_EQUAL(c,b);                     \
        }                                               \
    } while(0)

#define CSS_COMMON(in,offset)                                           \
    int len  = strlen(in);                                              \
    fprintf(stderr, "in='%s'\n", in);                                   \
    CompactedSubSequence css(in, len, "noname", offset);                \
    TEST_ASSERT_CSS_SELF_REFLEXIVE(css);                                \
    bind_css bound_css(css)

#define GEN_COMP_EXPD()                                                 \
    bound_css.test_mode = 0;                                            \
    char *comp = collectIntFunResults(bound_css, 0, css.expdLength()-1, 3, 0, 1); \
    bound_css.test_mode = 1;                                            \
    char *expd = collectIntFunResults(bound_css, 0, css.length(), 3, 0, 0)

#define GEN_TEXT(in)                                                    \
    bound_css.test_mode = 2;                                            \
    char *text = collectIntFunResults(bound_css, 0, css.length()-1, 3, 0, 0)
    
#define GEN_GAPS()                                                      \
    bound_css.test_mode = 3;                                            \
    char *gaps_before = collectIntFunResults(bound_css, 0, css.length(), 3, 0, 0); \
    bound_css.test_mode = 4;                                            \
    char *gaps_after = collectIntFunResults(bound_css, 0, css.length()-1, 3, 0, 1)

#define FREE_COMP_EXPD()                                                \
    free(expd);                                                         \
    free(comp)
    
#define FREE_GAPS()                                                     \
    free(gaps_before);                                                  \
    free(gaps_after)
    
#define COMP_EXPD_CHECK(exp_comp,exp_expd)                              \
    GEN_COMP_EXPD();                                                    \
    TEST_ASSERT_EQUAL(comp, exp_comp);                                  \
    TEST_ASSERT_EQUAL(expd, exp_expd);                                  \
    FREE_COMP_EXPD()

#define GAPS_CHECK(exp_before,exp_after)                                \
    GEN_GAPS();                                                         \
    TEST_ASSERT_EQUAL(gaps_before, exp_before);                         \
    TEST_ASSERT_EQUAL(gaps_after, exp_after);                           \
    FREE_GAPS()

// ------------------------------------------------------------
        
#define TEST_CS_EQUALS(in,exp_comp,exp_expd) do {                       \
        CSS_COMMON(in, 0);                                              \
        COMP_EXPD_CHECK(exp_comp,exp_expd);                             \
    } while(0)

#define TEST_CS_EQUALS_OFFSET(in,offset,exp_comp,exp_expd) do {         \
        CSS_COMMON(in, offset);                                         \
        COMP_EXPD_CHECK(exp_comp,exp_expd);                             \
    } while(0)
        
#define TEST_GAPS_EQUALS_OFFSET(in,offset,exp_before,exp_after) do {    \
        CSS_COMMON(in, offset);                                         \
        GAPS_CHECK(exp_before,exp_after);                               \
    } while(0)

#define TEST_CS_TEXT(in,exp_text) do {                                  \
        CSS_COMMON(in, 0);                                              \
        GEN_TEXT(in);                                                   \
        TEST_ASSERT_EQUAL(text, exp_text);                              \
        free(text);                                                     \
    } while(0)

#define TEST_CS_CBROKN(in,exp_comp,exp_expd) do {                       \
        CSS_COMMON(in, 0);                                              \
        GEN_COMP_EXPD();                                                \
        TEST_ASSERT_EQUAL__BROKEN(comp, exp_comp);                      \
        TEST_ASSERT_EQUAL(expd, exp_expd);                              \
        FREE_COMP_EXPD();                                               \
    } while(0)

void TEST_EARLY_CompactedSequence() {
    // reproduce a bug in compPosition

    // no base
    TEST_CS_EQUALS("-",          "  0  [0]",       "  1");
    TEST_CS_EQUALS("--",         "  0  0  [0]",    "  2");
    TEST_CS_EQUALS("---",        "  0  0  0  [0]", "  3");

    TEST_CS_TEXT("---", "");
    TEST_CS_TEXT("-A-", " 65");    // "A"
    TEST_CS_TEXT("A-C", " 65 67"); // "AC"

    TEST_CS_EQUALS("----------", "  0  0  0  0  0  0  0  0  0  0  [0]", " 10");

    // one base
    TEST_CS_EQUALS("A---------", "  0  1  1  1  1  1  1  1  1  1  [1]", "  0 10");
    TEST_CS_EQUALS("-A--------", "  0  0  1  1  1  1  1  1  1  1  [1]", "  1 10"); 
    TEST_CS_EQUALS("---A------", "  0  0  0  0  1  1  1  1  1  1  [1]", "  3 10"); 
    TEST_CS_EQUALS("-----A----", "  0  0  0  0  0  0  1  1  1  1  [1]", "  5 10"); 
    TEST_CS_EQUALS("-------A--", "  0  0  0  0  0  0  0  0  1  1  [1]", "  7 10"); 
    TEST_CS_EQUALS("---------A", "  0  0  0  0  0  0  0  0  0  0  [1]", "  9 10"); 

    // two bases
    TEST_CS_EQUALS("AC--------", "  0  1  2  2  2  2  2  2  2  2  [2]", "  0  1 10");

    TEST_CS_EQUALS("A-C-------", "  0  1  1  2  2  2  2  2  2  2  [2]", "  0  2 10");
    TEST_CS_EQUALS("A--------C", "  0  1  1  1  1  1  1  1  1  1  [2]", "  0  9 10");
    TEST_CS_EQUALS("-AC-------", "  0  0  1  2  2  2  2  2  2  2  [2]", "  1  2 10");
    TEST_CS_EQUALS("-A------C-", "  0  0  1  1  1  1  1  1  1  2  [2]", "  1  8 10");
    TEST_CS_EQUALS("-A-------C", "  0  0  1  1  1  1  1  1  1  1  [2]", "  1  9 10");
    TEST_CS_EQUALS("-------A-C", "  0  0  0  0  0  0  0  0  1  1  [2]", "  7  9 10");
    TEST_CS_EQUALS("--------AC", "  0  0  0  0  0  0  0  0  0  1  [2]", "  8  9 10");

    // 3 bases
    TEST_CS_EQUALS("ACG-------", "  0  1  2  3  3  3  3  3  3  3  [3]", "  0  1  2 10");
    TEST_CS_EQUALS("AC---G----", "  0  1  2  2  2  2  3  3  3  3  [3]", "  0  1  5 10"); 
    TEST_CS_EQUALS("A-C--G----", "  0  1  1  2  2  2  3  3  3  3  [3]", "  0  2  5 10"); 
    TEST_CS_EQUALS("A-C--G----", "  0  1  1  2  2  2  3  3  3  3  [3]", "  0  2  5 10"); 
    TEST_CS_EQUALS("A--C-G----", "  0  1  1  1  2  2  3  3  3  3  [3]", "  0  3  5 10"); 
    TEST_CS_EQUALS("A---CG----", "  0  1  1  1  1  2  3  3  3  3  [3]", "  0  4  5 10");

    TEST_CS_EQUALS("A---C---G-", "  0  1  1  1  1  2  2  2  2  3  [3]", "  0  4  8 10");
    TEST_CS_EQUALS("A---C----G", "  0  1  1  1  1  2  2  2  2  2  [3]", "  0  4  9 10");

    // 4 bases
    TEST_CS_EQUALS("-AC-G--T--", "  0  0  1  2  2  3  3  3  4  4  [4]", "  1  2  4  7 10"); 

    // 9 bases
    TEST_CS_EQUALS("-CGTACGTAC", "  0  0  1  2  3  4  5  6  7  8  [9]", "  1  2  3  4  5  6  7  8  9 10");
    TEST_CS_EQUALS("A-GTACGTAC", "  0  1  1  2  3  4  5  6  7  8  [9]", "  0  2  3  4  5  6  7  8  9 10");
    TEST_CS_EQUALS("ACGTA-GTAC", "  0  1  2  3  4  5  5  6  7  8  [9]", "  0  1  2  3  4  6  7  8  9 10");
    TEST_CS_EQUALS("ACGTACGT-C", "  0  1  2  3  4  5  6  7  8  8  [9]", "  0  1  2  3  4  5  6  7  9 10");
    TEST_CS_EQUALS("ACGTACGTA-", "  0  1  2  3  4  5  6  7  8  9  [9]", "  0  1  2  3  4  5  6  7  8 10");

    // all 10 bases
    TEST_CS_EQUALS("ACGTACGTAC", "  0  1  2  3  4  5  6  7  8  9 [10]", "  0  1  2  3  4  5  6  7  8  9 10");

    TEST_CS_EQUALS_OFFSET("A--C-G-", 0, "  0  1  1  1  2  2  3  [3]",             "  0  3  5  7");
    TEST_CS_EQUALS_OFFSET("A--C-G-", 2, "  0  0  0  1  1  1  2  2  3  [3]",       "  2  5  7  9");
    TEST_CS_EQUALS_OFFSET("A--C-G-", 3, "  0  0  0  0  1  1  1  2  2  3  [3]",    "  3  6  8 10");
    TEST_CS_EQUALS_OFFSET("A--C-G-", 4, "  0  0  0  0  0  1  1  1  2  2  3  [3]", "  4  7  9 11");

    // test no_of_gaps_before() and no_of_gaps_after() 
    TEST_GAPS_EQUALS_OFFSET("-AC---G",     0, "  1  0  3  0", "  0  3  0 [-1]");
    TEST_GAPS_EQUALS_OFFSET(".AC..-G",     0, "  1  0  3  0", "  0  3  0 [-1]");
    TEST_GAPS_EQUALS_OFFSET("A--C-G-",     0, "  0  2  1  1", "  2  1  1 [-1]");
    TEST_GAPS_EQUALS_OFFSET("A--C-G-",  1000, "  0  2  1  1", "  2  1  1 [-1]"); // is independent from offset
}

#endif // UNIT_TESTS
