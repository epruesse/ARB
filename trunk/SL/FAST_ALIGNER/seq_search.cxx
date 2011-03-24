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
    bool test_comp;
    bind_css(CompactedSubSequence& css_) : css(css_), test_comp(true) {}

    int operator()(int i) const {
        if (test_comp) return css.compPosition(i);
        return css.expdPosition(i);
    }
};

#define TEST_ASSERT_CSS_SELF_REFLEXIVE(css) do {    \
        for (int b = 0; b<css.length(); ++b) {  \
            int x = css.expdPosition(b);        \
            int c = css.compPosition(x);        \
            TEST_ASSERT_EQUAL(c,b);             \
        }                                       \
    } while(0)

#define TEST_CS_START(in)                                               \
    int len  = strlen(in);                                              \
    fprintf(stderr, "in='%s'\n", in);                                   \
    CompactedSubSequence css(in, len, "noname", 0);                     \
    TEST_ASSERT_CSS_SELF_REFLEXIVE(css);                                \
    bind_css bound_css(css);                                            \
    char *comp = collectIntFunResults(bound_css, 0, css.expdLength()-1, 3, 0, 1); \
    bound_css.test_comp = false;                                        \
    char *expd = collectIntFunResults(bound_css, 0, css.length(), 3, 0, 0);
    
#define TEST_CS_END()                                                   \
    free(expd);                                                         \
    free(comp);
    
#define TEST_CS_EQUALS(in,exp_comp,exp_expd) do {                       \
        TEST_CS_START(in);                                              \
        TEST_ASSERT_EQUAL(comp, exp_comp);                              \
        TEST_ASSERT_EQUAL(expd, exp_expd);                              \
        TEST_CS_END();                                                  \
} while(0)

#define TEST_CS_CBROKN(in,exp_comp,exp_expd) do {                       \
        TEST_CS_START(in);                                              \
        TEST_ASSERT_EQUAL__BROKEN(comp, exp_comp);                      \
        TEST_ASSERT_EQUAL(expd, exp_expd);                              \
        TEST_CS_END();                                                  \
} while(0)

void TEST_CompactedSequence() {
    // reproduce a bug in compPosition

    // no base
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
}

#endif // UNIT_TESTS
