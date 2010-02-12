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

#include <cstdarg>
#include <climits>
#include <cctype>

#define MESSAGE_BUFFERSIZE 300

void messagef(const char *format, ...)
{
    va_list argp;
    char buffer[MESSAGE_BUFFERSIZE];

    va_start(argp, format);

#if defined(ASSERTION_USED)
    int chars =
#endif // ASSERTION_USED
        vsprintf(buffer, format, argp);
    fa_assert(chars<MESSAGE_BUFFERSIZE);

    va_end(argp);
    aw_message(buffer);
}

void Points::append(Points *neu) {
    if (Next) Next->append(neu);
    else Next = neu;
}

CompactedSequence::CompactedSequence(const char *Text, int Length, const char *name, int start_offset)
{
    long cPos;
    long xPos;
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

    delete[] expdPositionTab;
    if (points) delete points;

    free(myName);
}

int CompactedSequence::compPosition(int xPos) const
{
    int l = 0,
        h = length();

    while (l<h) {
        int m = (l+h)/2;
        int cmp = xPos-expdPosition(m);

        if (cmp==0) {
            return m; // found!
        }

        if (cmp<0) { // xPos < expdPosition(m)
            h = m-1;
        }
        else { // xPos > expdPosition(m)
            l = m+1;
        }
    }

    fa_assert(l==h);
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

void AlignBuffer::correctUnalignedPositions()
{
    long off = 0;
    long rest = used;

    while (rest)
    {
        char *found = (char*)memchr(myQuality+off, '?', rest);  // search for next '?' in myQuality
        if (!found || (found-myQuality)>=rest) break;

        long cnt;
        for (cnt=0; found[cnt]=='?'; cnt++) found[cnt]='+';     // count # of unaligned positions and change them to '+'
        long from  = found-myQuality;
        long b_off = from-1;                                    // position before unaligned positions
        long a_off = from+cnt;                                  // positions after unaligned positions

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
