#ifndef awtc_seq_search_hxx_included
#define awtc_seq_search_hxx_included

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define awtc_assert(bed) arb_assert(bed)

/* hide GNU extensions for non-gnu compilers: */
#ifndef GNU
# ifndef __attribute__
#  define __attribute__(x)
# endif
#endif

#define SEQ_CHARS   26
#define MAX_TRIPLES (SEQ_CHARS*SEQ_CHARS*SEQ_CHARS+1) // one faked triple for all non-char triples
#define GAP_CHARS   ".-~?"      // Characters treated as gaps

void AWTC_message(const char *format, ...) __attribute__((format(printf, 1, 2)));

static inline int max(int i1, int i2) { return i1>i2 ? i1 : i2; }
static inline int AWTC_is_gap(int c)  { return strchr(GAP_CHARS, c)!=0; }

class AWTC_CompactedSubSequence;

class AWTC_Points               // here we store points ('.') which were deleted when compacting sequences
{
    long BeforeBase;         // this position is the "compressed" position
    AWTC_Points *Next;

public:

    AWTC_Points(long before_base) {
        BeforeBase = before_base;
        Next       = NULL;
    }
    ~AWTC_Points() {
        delete Next;
    }

    void append(AWTC_Points *neu);
    long beforeBase(void) const         { return BeforeBase; }
    const AWTC_Points *next(void) const { return Next; }
};

class AWTC_CompactedSequence    // compacts a string (gaps removed, all chars upper)
// (private class - use AWTC_CompactedSubSequence)
{
    char *myText;
    int   myLength;
    int   myStartOffset;        // expanded offsets of first and last position in 'text' (which is given to the c-tor)
    int   myEndOffset;
    int  *expdPositionTab;      // this table contains all 'real' (=not compacted) positions of the bases (len = myLength+1)

    int *gapsBeforePosition;    // gapsBeforePosition[ idx+1 ] = gaps _after_ position 'idx'
    int  referred;

    char *myName;               // sequence name

    AWTC_Points *points;        // Points which were deleted from the sequence are store here

    // -----------------------------------------

    AWTC_CompactedSequence(const char *text, int length, const char *name, int start_offset=0);
    ~AWTC_CompactedSequence();

    int length() const          {return myLength;}
    const char *text(int i=0) const     {return myText+i;}

    char operator[](int i) const        {if (i<0 || i>=length()) i = 0; else i = text()[i]; return i;} // HP compiler does not allow two returns

    int expdPosition(int cPos) const {
        awtc_assert(cPos>=0 && cPos<=myLength); // allowed for all positions plus one following element
        ;                                       // (which contains the total length of the original sequence)
        return expdPositionTab[cPos]+myStartOffset;
    }
    int compPosition(int xPos) const;

    int no_of_gaps_before(int cPos) const {
        int leftMostGap;
        if (cPos>0) {
            leftMostGap = expdPosition(cPos-1)+1;
        }
        else {
            leftMostGap = myStartOffset;
        }
        return expdPosition(cPos)-leftMostGap;
    }
    int no_of_gaps_after(int cPos) const {
        int rightMostGap;
        if (cPos<(length()-1)) {
            rightMostGap = expdPosition(cPos+1)-1;
        }
        else {
            rightMostGap = myEndOffset;
        }
        return rightMostGap-expdPosition(cPos);
    }

    void storePoints(int beforePos) {
        if (points)     points->append(new AWTC_Points(beforePos));
        else            points = new AWTC_Points(beforePos);
    }

    friend class AWTC_CompactedSubSequence;

public:

    const AWTC_Points *getPointlist(void) const { return points; }
};

class AWTC_CompactedSubSequence // smart pointer and substring class for AWTC_CompactedSequence
{
    AWTC_CompactedSequence *mySequence;
    int myPos;                  // offset into mySequence->myText
    //    long myExpdPos;
    int myLength;
    const char *myText;         // only for speed-up
    const AWTC_Points *points;  // just a reference

public:

    AWTC_CompactedSubSequence(const char *seq, int length, const char *name, int start_offset=0);                               // normal c-tor
    AWTC_CompactedSubSequence(const AWTC_CompactedSubSequence& other);
    AWTC_CompactedSubSequence(const AWTC_CompactedSubSequence& other, int rel_pos, int length);         // substr c-tor
    ~AWTC_CompactedSubSequence();                                                               // d-tor
    AWTC_CompactedSubSequence& operator=(const AWTC_CompactedSubSequence& other);               // =-c-tor

    int length() const                  {return myLength;}

    const char *text() const                    { return myText; }
    const char *text(int i) const               { return text()+i; }

    const char *name() const                    { return mySequence->myName; }

    char operator[](int i) const                { if (i<0 || i>=length()) { i= 0; } else { i = text()[i]; } return i; }

    int no_of_gaps_before(int cPos) const       { return mySequence->no_of_gaps_before(myPos+cPos); }
    int no_of_gaps_after(int cPos) const        { return mySequence->no_of_gaps_after(myPos+cPos); }

    int expdStartPosition() const               { return mySequence->expdPosition(myPos); }
    int expdPosition(int cPos) const
    {
        awtc_assert(cPos>=0 && cPos<=myLength);                 // allowed for all positions plus follower
        return mySequence->expdPosition(myPos+cPos);
    }
    int compPosition(int xPos) const            { return mySequence->compPosition(xPos)-myPos; }
    int expdLength() const                      { return expdPosition(length()-1); }
    const int *gapsBefore(int offset=0) const   { return mySequence->gapsBeforePosition + myPos + offset; }

    int thisPointPosition(void) {
        int pos = points->beforeBase()-myPos;

        if (pos>(myLength+1)) {
            points = NULL;
            pos = -1;   // HP Compiler !!
        }

        return pos;
    }

    int firstPointPosition(void) // HP Compiler needs res
    {
        points = mySequence->getPointlist();
        int res = -1;

        while (points && points->beforeBase()<myPos) {
            points = points->next();
        }
        if (points){
            res = thisPointPosition();
        }

        return res;
    }

    int nextPointPosition(void) // HP Compiler !!!
    {
        int res = -1;
        if (points){
            points = points->next();
        }
        if (points){
            res = thisPointPosition();
        }
        return res;
    }
};

class AWTC_SequencePosition     // pointer to character position in AWTC_CompactedSubSequence
{
    const AWTC_CompactedSubSequence *mySequence;
    long myPos;
    const char *myText;

    int distanceTo(const AWTC_SequencePosition& other) const    {awtc_assert(mySequence==other.mySequence); return myPos-other.myPos;}

public:

    AWTC_SequencePosition(const AWTC_CompactedSubSequence& seq, long pos=0)
        : mySequence(&seq),
          myPos(pos),
          myText(seq.text(pos)) {}
    AWTC_SequencePosition(const AWTC_SequencePosition& pos, long offset=0)
        : mySequence(pos.mySequence),
          myPos(pos.myPos+offset),
          myText(pos.myText+offset) {}
    ~AWTC_SequencePosition() {}

    const char *text() const                            { return myText;}
    const AWTC_CompactedSubSequence& sequence() const   { return *mySequence;}

    long expdPosition() const                           { return sequence().expdPosition(myPos); }
    long expdPosition(int offset)                       { return sequence().expdPosition(myPos+offset); }

    AWTC_SequencePosition& operator++()                 { myPos++; myText++; return *this; }
    AWTC_SequencePosition& operator--()                 { myPos--; myText--; return *this; }

    AWTC_SequencePosition& operator+=(long l)           { myPos += l; myText += l; return *this; }
    AWTC_SequencePosition& operator-=(long l)           { myPos -= l; myText -= l; return *this; }

    int operator<(const AWTC_SequencePosition& other) const { return distanceTo(other)<0; }
    int operator>(const AWTC_SequencePosition& other) const { return distanceTo(other)>0; }

    long leftOf() const                                 { return myPos;}
    long rightOf() const                                { return mySequence->length()-myPos;}
};

class AWTC_TripleOffset         // a list of offsets (used in AWTC_FastSearchSequence)
{
    long               myOffset; // compacted offset
    AWTC_TripleOffset *myNext;

    void IV() const     {awtc_assert(myNext!=this);}

public:

    AWTC_TripleOffset(long Offset, AWTC_TripleOffset *next_toff) : myOffset(Offset), myNext(next_toff) { IV(); }
    ~AWTC_TripleOffset();

    AWTC_TripleOffset *next() const { IV(); return myNext; }
    long offset() const { IV(); return myOffset; }
};


class AWTC_alignBuffer                  // alignment result buffer
{
    char *myBuffer;
    char *myQuality;

    // myQuality contains the following chars:
    //
    // '?'      base was just inserted (not aligned)
    // '-'      master and slave are equal
    // '+'      gap in masert excl-or slave
    // '~'      master base and slave base are related
    // '#'      master base and slave base are NOT related (mismatch)

    long mySize;
    long used;

    void set_used(long newVal)
    {
        awtc_assert(newVal<=mySize);
        used = newVal;
        GB_status(double(used)/double(mySize));
    }
    void add_used(long toAdd)
    {
        set_used(toAdd+used);
    }

    int isGlobalGap(long off)   // TRUE if gap in slave AND master
    {
        return (myBuffer[off]=='-' || myBuffer[off]=='.') && myQuality[off]=='-';
    }
    void moveUnaligned(long from, long to)
    {
        myBuffer[to] = myBuffer[from];
        myQuality[to] = myQuality[from];
        myBuffer[from] = '-';
        myQuality[from] = '-';
    }

public:

    AWTC_alignBuffer(long size)
    {
        mySize = size;
        set_used(0);
        myBuffer = new char[size+1];
        myBuffer[size] = 0;
        myQuality = new char[size+1];
        myQuality[size] = 0;
    }
    ~AWTC_alignBuffer()
    {
        delete [] myBuffer;
        delete [] myQuality;
    }

    const char *text() const            { awtc_assert(free()>=0); myBuffer[used]=0;  return myBuffer; }
    const char *quality() const         { awtc_assert(free()>=0); myQuality[used]=0; return myQuality; }
    long length() const                 { return used; }

    long offset() const                 { return used; }
    long free() const                   { return mySize-used; }

    void copy(const char *s, char q, long len)
    {
        awtc_assert(len<=free());
        memcpy(myBuffer+used, s, len);
        memset(myQuality+used, q, len);
        add_used(len);
    }
    void set(char c, char q)
    {
        awtc_assert(free()>=1);
        myBuffer[used] = c;
        myQuality[used] = q;
        add_used(1);
    }
    void set(char c, char q, long len)
    {
        awtc_assert(len<=free());
        memset(myBuffer+used, c, len);
        memset(myQuality+used, q, len);
        add_used(len);
    }
    void reset(long newOffset=0)
    {
        awtc_assert(newOffset>=0 && newOffset<mySize);
        set_used(newOffset);
    }

    void correctUnalignedPositions(void);

    void expandPoints(AWTC_CompactedSubSequence& slaveSequence);
    void point_ends_of();
};


class AWTC_fast_align_insertion
{
    long insert_at_offset;
    long inserted_gaps;
    
    AWTC_fast_align_insertion *myNext;

public:
    AWTC_fast_align_insertion(long Offset, long Gaps) {
        insert_at_offset = Offset;
        inserted_gaps    = Gaps;
        myNext           = 0;
    }
    ~AWTC_fast_align_insertion();

    AWTC_fast_align_insertion *append(long Offset, long Gaps) {
        return myNext = new AWTC_fast_align_insertion(Offset, Gaps);
    }
    void dump() const {
        printf(" offset=%5li gaps=%3li\n", insert_at_offset, inserted_gaps);
    }
    void dumpAll() const {
        dump();
        if (myNext) myNext->dumpAll();
    }

    const AWTC_fast_align_insertion *next() const { return myNext; }
    long offset() const { return insert_at_offset; }
    long gaps() const { return inserted_gaps; }
};


class AWTC_fast_align_report    // alignment report
{
    long  alignedBases;
    long  mismatchedBases;      // in aligned part
    long  unalignedBases;
    char *my_master_name;
    int   showGapsMessages;

    AWTC_fast_align_insertion *first; // list of insertions (order is important)
    AWTC_fast_align_insertion *last;

public:

    AWTC_fast_align_report(const char *master_name, int show_Gaps_Messages) {
        alignedBases = 0;
        mismatchedBases = 0;
        unalignedBases = 0;
        first = 0;
        last = 0;
        my_master_name = strdup(master_name);
        showGapsMessages = show_Gaps_Messages;
    }
    ~AWTC_fast_align_report() {
        if (first) delete first;
        free(my_master_name);
    }

    void memorize_insertion(long offset, long gaps) {
        if (last) {
            last = last->append(offset,gaps);
        }
        else {
            if (showGapsMessages) {
                aw_message("---- gaps needed.");
            }
            first = last = new AWTC_fast_align_insertion(offset, gaps);
        }

        if (showGapsMessages) {
            char *messi = (char*)malloc(100);

            sprintf(messi, "'%s' needs %li gaps at offset %li.", my_master_name, gaps, offset+1);
            aw_message(messi);
            free(messi);
        }
    }

    const AWTC_fast_align_insertion* insertion() const { return first; }

    void count_aligned_base(int mismatched)             {alignedBases++; if (mismatched) mismatchedBases++;}
    void count_unaligned_base(int no_of_bases)          {unalignedBases += no_of_bases;}

    void dump() const {
        printf("fast_align_report:\n"
               " alignedBases    = %li (=%f%%)\n"
               " mismatchedBases = %li\n"
               " unalignedBases  = %li\n"
               "inserts in master-sequence:\n",
               alignedBases, (float)alignedBases/(alignedBases+unalignedBases),
               mismatchedBases,
               unalignedBases);
        if (first) first->dumpAll();
    }
};

//class AWTC_FastSearchOccurance;
class AWTC_FastSearchSequence           // a sequence where search of character triples is very fast
{
    const AWTC_CompactedSubSequence *mySequence;
    AWTC_TripleOffset *myOffset[MAX_TRIPLES];

    AWTC_FastSearchSequence(const AWTC_FastSearchSequence&) {} // not allowed
    AWTC_FastSearchSequence& operator=(const AWTC_FastSearchSequence&) { return *this; } // not allowed

    static int triple_index(const char *triple)
    {
        int idx = (int)(triple[0]-'A')*SEQ_CHARS*SEQ_CHARS + (int)(triple[1]-'A')*SEQ_CHARS + (triple[2]-'A');

        return idx>=0 && idx<MAX_TRIPLES ? idx : MAX_TRIPLES-1;

        // if we got any non A-Z characters in sequence, the triple value may overflow.
        // we use (MAX_TRIPLES-1) as index for all illegal indices.
    }

public:

    AWTC_FastSearchSequence(const AWTC_CompactedSubSequence& seq);
    ~AWTC_FastSearchSequence() {}

    GB_ERROR fast_align(const AWTC_CompactedSubSequence& align_to, AWTC_alignBuffer *alignBuffer,
                        int max_seq_length, int matchScore, int mismatchScore,
                        AWTC_fast_align_report *report) const;

    const AWTC_CompactedSubSequence& sequence() const {return *mySequence;}
    const AWTC_TripleOffset *find(const char *triple) const { return myOffset[triple_index(triple)]; }
};


class AWTC_FastSearchOccurance          // iterates through all Occurances of one character triple
{
    const AWTC_FastSearchSequence&      mySequence;
    const AWTC_TripleOffset             *myOffset;

    void IV() const {awtc_assert(!myOffset || myOffset!=myOffset->next());}

public:

    AWTC_FastSearchOccurance(const AWTC_FastSearchSequence& Sequence, const char *triple)
        : mySequence(Sequence)                          { myOffset = mySequence.find(triple); IV();}
    ~AWTC_FastSearchOccurance()                         { IV();}

    int found() const                                   { IV(); return myOffset!=0; }
    void gotoNext()                                     { awtc_assert(myOffset!=0); myOffset = myOffset->next(); IV(); }

    long offset() const                                 { IV(); return myOffset->offset();}
    const AWTC_CompactedSubSequence& sequence() const   { IV(); return mySequence.sequence();}
};

// -----------------------------
//      INLINE-Functions:
// -----------------------------

inline AWTC_CompactedSubSequence::AWTC_CompactedSubSequence(const char *seq, int len, const char *Name, int start_offset) // normal c-tor
{
    mySequence = new AWTC_CompactedSequence(seq, len, Name, start_offset);
    myPos      = 0;
    myLength   = mySequence->length();
    myText     = mySequence->text()+myPos;
}

inline AWTC_CompactedSubSequence::AWTC_CompactedSubSequence(const AWTC_CompactedSubSequence& other)
{
    mySequence = other.mySequence;
    mySequence->referred++;

    myPos    = other.myPos;
    myLength = other.myLength;
    myText   = other.myText;
}

inline AWTC_CompactedSubSequence::AWTC_CompactedSubSequence(const AWTC_CompactedSubSequence& other, int rel_pos, int Length)
// substr c-tor
{
    mySequence = other.mySequence;
    mySequence->referred++;

    myPos    = other.myPos+rel_pos;
    myLength = Length;
    myText   = mySequence->text()+myPos;

    //printf("work on sub-seq: %li-%li\n", pos, pos+length-1);

    awtc_assert((rel_pos+Length)<=mySequence->length());
    awtc_assert(rel_pos>=0);
}

inline AWTC_CompactedSubSequence::~AWTC_CompactedSubSequence() // d-tor
{
    if (mySequence->referred-- == 1) // last reference
        delete mySequence;
}

inline AWTC_CompactedSubSequence& AWTC_CompactedSubSequence::operator=(const AWTC_CompactedSubSequence& other) // =-c-tor
{
    // d-tor part

    if (mySequence->referred-- == 1)
        delete mySequence;

    // c-tor part

    mySequence = other.mySequence;
    mySequence->referred++;

    myPos = other.myPos;
    //    myExpdPos = other.myExpdPos;
    myLength = other.myLength;
    myText = mySequence->text()+myPos;

    return *this;
}



#endif
