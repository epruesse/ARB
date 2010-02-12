// =============================================================== //
//                                                                 //
//   File      : awtc_seq_search.hxx                               //
//   Purpose   : Fast sequence search for fast aligner             //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 1998      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AWTC_SEQ_SEARCH_HXX
#define AWTC_SEQ_SEARCH_HXX

#ifndef AW_ROOT_HXX
#include <aw_root.hxx>
#endif
#ifndef ARBDB_H
#include <arbdb.h>
#endif


#define fa_assert(bed) arb_assert(bed)

#define SEQ_CHARS   26
#define MAX_TRIPLES (SEQ_CHARS*SEQ_CHARS*SEQ_CHARS+1) // one faked triple for all non-char triples
#define GAP_CHARS   ".-~?"      // Characters treated as gaps

void messagef(const char *format, ...) __ATTR__FORMAT(1);

static inline int max(int i1, int i2) { return i1>i2 ? i1 : i2; }
static inline int is_gap(int c)  { return strchr(GAP_CHARS, c)!=0; }

class CompactedSubSequence;

class Points                                        // here we store points ('.') which were deleted when compacting sequences
{
    long    BeforeBase;                             // this position is the "compressed" position
    Points *Next;

public:

    Points(long before_base) {
        BeforeBase = before_base;
        Next       = NULL;
    }
    ~Points() {
        delete Next;
    }

    void append(Points *neu);
    long beforeBase() const { return BeforeBase; }
    const Points *next() const { return Next; }
};

class CompactedSequence                             // compacts a string (gaps removed, all chars upper)
// (private class - use CompactedSubSequence)
{
    char *myText;
    int   myLength;                                 // number of base positions
    int   myStartOffset;                            // expanded offsets of first and last position in 'text' (which is given to the c-tor)
    int   myEndOffset;
    int  *expdPositionTab;                          // this table contains all 'real' (=not compacted) positions of the bases (len = myLength+1)

    int *gapsBeforePosition;                        // gapsBeforePosition[ idx+1 ] = gaps _after_ position 'idx'
    int  referred;

    char *myName;                                   // sequence name

    Points *points;                                 // Points which were deleted from the sequence are store here

    // -----------------------------------------

    CompactedSequence(const char *text, int length, const char *name, int start_offset=0);
    ~CompactedSequence();

    int length() const          { return myLength; }
    const char *text(int i=0) const     { return myText+i; }

    char operator[](int i) const        { if (i<0 || i>=length()) i = 0; else i = text()[i]; return i; } // HP compiler does not allow two returns

    int expdPosition(int cPos) const {
        fa_assert(cPos>=0 && cPos<=myLength); // allowed for all positions plus one following element
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
        if (points)     points->append(new Points(beforePos));
        else            points = new Points(beforePos);
    }

    friend class CompactedSubSequence;

public:

    const Points *getPointlist() const { return points; }
};

class CompactedSubSequence // smart pointer and substring class for CompactedSequence
{
    CompactedSequence *mySequence;
    int                myPos;                       // offset into mySequence->myText
    int                myLength;                    // number of base positions
    const char        *myText;                      // only for speed-up
    const Points      *points;                      // just a reference

public:

    CompactedSubSequence(const char *seq, int length, const char *name, int start_offset=0);                               // normal c-tor
    CompactedSubSequence(const CompactedSubSequence& other);
    CompactedSubSequence(const CompactedSubSequence& other, int rel_pos, int length);         // substr c-tor
    ~CompactedSubSequence();                                                               // d-tor
    CompactedSubSequence& operator=(const CompactedSubSequence& other);               // =-c-tor

    int length() const                  { return myLength; }

    const char *text() const                    { return myText; }
    const char *text(int i) const               { return text()+i; }

    const char *name() const                    { return mySequence->myName; }

    char operator[](int i) const                { if (i<0 || i>=length()) { i = 0; } else { i = text()[i]; } return i; }

    int no_of_gaps_before(int cPos) const       { return mySequence->no_of_gaps_before(myPos+cPos); }
    int no_of_gaps_after(int cPos) const        { return mySequence->no_of_gaps_after(myPos+cPos); }

    int expdStartPosition() const               { return mySequence->expdPosition(myPos); }
    int expdPosition(int cPos) const
    {
        fa_assert(cPos>=0 && cPos<=myLength);                 // allowed for all positions plus follower
        return mySequence->expdPosition(myPos+cPos);
    }
    int compPosition(int xPos) const            { return mySequence->compPosition(xPos)-myPos; }
    int expdLength() const                      { return expdPosition(length()-1); }
    const int *gapsBefore(int offset=0) const   { return mySequence->gapsBeforePosition + myPos + offset; }

    int thisPointPosition() {
        int pos = points->beforeBase()-myPos;

        if (pos>(myLength+1)) {
            points = NULL;
            pos = -1;   // HP Compiler !!
        }

        return pos;
    }

    int firstPointPosition()     // HP Compiler needs res
    {
        points = mySequence->getPointlist();
        int res = -1;

        while (points && points->beforeBase()<myPos) {
            points = points->next();
        }
        if (points) {
            res = thisPointPosition();
        }

        return res;
    }

    int nextPointPosition()     // HP Compiler !!!
    {
        int res = -1;
        if (points) {
            points = points->next();
        }
        if (points) {
            res = thisPointPosition();
        }
        return res;
    }
};

class SequencePosition                              // pointer to character position in CompactedSubSequence
{
    const CompactedSubSequence *mySequence;
    long myPos;
    const char *myText;

    int distanceTo(const SequencePosition& other) const    { fa_assert(mySequence==other.mySequence); return myPos-other.myPos; }

public:

    SequencePosition(const CompactedSubSequence& seq, long pos=0)
        : mySequence(&seq),
          myPos(pos),
          myText(seq.text(pos)) {}
    SequencePosition(const SequencePosition& pos, long offset=0)
        : mySequence(pos.mySequence),
          myPos(pos.myPos+offset),
          myText(pos.myText+offset) {}
    ~SequencePosition() {}

    const char *text() const                     { return myText; }
    const CompactedSubSequence& sequence() const { return *mySequence; }

    long expdPosition() const     { return sequence().expdPosition(myPos); }
    long expdPosition(int offset) { return sequence().expdPosition(myPos+offset); }

    SequencePosition& operator++() { myPos++; myText++; return *this; }
    SequencePosition& operator--() { myPos--; myText--; return *this; }

    SequencePosition& operator+=(long l) { myPos += l; myText += l; return *this; }
    SequencePosition& operator-=(long l) { myPos -= l; myText -= l; return *this; }

    int operator<(const SequencePosition& other) const { return distanceTo(other)<0; }
    int operator>(const SequencePosition& other) const { return distanceTo(other)>0; }

    long leftOf() const  { return myPos; }
    long rightOf() const { return mySequence->length()-myPos; }
};

class TripleOffset                                  // a list of offsets (used in FastSearchSequence)
{
    long          myOffset;                         // compacted offset
    TripleOffset *myNext;

    void IV() const     { fa_assert(myNext!=this); }

public:

    TripleOffset(long Offset, TripleOffset *next_toff) : myOffset(Offset), myNext(next_toff) { IV(); }
    ~TripleOffset();

    TripleOffset *next() const { IV(); return myNext; }
    long offset() const { IV(); return myOffset; }
};


class AlignBuffer                                   // alignment result buffer
{
    char *myBuffer;
    char *myQuality;

    // myQuality contains the following chars:
    //
    // '?'      base was just inserted (not aligned)
    // '-'      master and slave are equal
    // '+'      gap in master excl-or slave
    // '~'      master base and slave base are related
    // '#'      master base and slave base are NOT related (mismatch)

    long mySize;
    long used;

    void set_used(long newVal)
    {
        fa_assert(newVal<=mySize);
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

    AlignBuffer(long size)
    {
        mySize = size;
        set_used(0);
        myBuffer = new char[size+1];
        myBuffer[size] = 0;
        myQuality = new char[size+1];
        myQuality[size] = 0;
    }
    ~AlignBuffer()
    {
        delete [] myBuffer;
        delete [] myQuality;
    }

    const char *text() const            { fa_assert(free()>=0); myBuffer[used]=0;  return myBuffer; }
    const char *quality() const         { fa_assert(free()>=0); myQuality[used]=0; return myQuality; }
    long length() const                 { return used; }

    long offset() const                 { return used; }
    long free() const                   { return mySize-used; }

    void copy(const char *s, char q, long len)
    {
        fa_assert(len<=free());
        memcpy(myBuffer+used, s, len);
        memset(myQuality+used, q, len);
        add_used(len);
    }
    void set(char c, char q)
    {
        fa_assert(free()>=1);
        myBuffer[used] = c;
        myQuality[used] = q;
        add_used(1);
    }
    void set(char c, char q, long len)
    {
        fa_assert(len<=free());
        memset(myBuffer+used, c, len);
        memset(myQuality+used, q, len);
        add_used(len);
    }
    void reset(long newOffset=0)
    {
        fa_assert(newOffset>=0 && newOffset<mySize);
        set_used(newOffset);
    }

    void correctUnalignedPositions();

    void expandPoints(CompactedSubSequence& slaveSequence);
    void point_ends_of();
};


class FastAlignInsertion
{
    long insert_at_offset;
    long inserted_gaps;

    FastAlignInsertion *myNext;

public:
    FastAlignInsertion(long Offset, long Gaps) {
        insert_at_offset = Offset;
        inserted_gaps    = Gaps;
        myNext           = 0;
    }
    ~FastAlignInsertion();

    FastAlignInsertion *append(long Offset, long Gaps) {
        return myNext = new FastAlignInsertion(Offset, Gaps);
    }
    void dump() const {
        printf(" offset=%5li gaps=%3li\n", insert_at_offset, inserted_gaps);
    }
    void dumpAll() const {
        dump();
        if (myNext) myNext->dumpAll();
    }

    const FastAlignInsertion *next() const { return myNext; }
    long offset() const { return insert_at_offset; }
    long gaps() const { return inserted_gaps; }
};


class FastAlignReport    // alignment report
{
    long  alignedBases;
    long  mismatchedBases;      // in aligned part
    long  unalignedBases;
    char *my_master_name;
    bool  showGapsMessages;     // display messages about missing gaps in master

    FastAlignInsertion *first; // list of insertions (order is important)
    FastAlignInsertion *last;

public:

    FastAlignReport(const char *master_name, bool show_Gaps_Messages) {
        alignedBases     = 0;
        mismatchedBases  = 0;
        unalignedBases   = 0;
        first            = 0;
        last             = 0;
        my_master_name   = strdup(master_name);
        showGapsMessages = show_Gaps_Messages;
    }
    ~FastAlignReport() {
        if (first) delete first;
        free(my_master_name);
    }

    void memorize_insertion(long offset, long gaps) {
        if (last) {
            last = last->append(offset, gaps);
        }
        else {
            if (showGapsMessages) aw_message("---- gaps needed.");
            first = last = new FastAlignInsertion(offset, gaps);
        }

        if (showGapsMessages) {
            char *messi = (char*)malloc(100);

            sprintf(messi, "'%s' needs %li gaps at offset %li.", my_master_name, gaps, offset+1);
            aw_message(messi);
            free(messi);
        }
    }

    const FastAlignInsertion* insertion() const { return first; }

    void count_aligned_base(int mismatched)             { alignedBases++; if (mismatched) mismatchedBases++; }
    void count_unaligned_base(int no_of_bases)          { unalignedBases += no_of_bases; }

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

class FastSearchSequence           // a sequence where search of character triples is very fast
{
    const CompactedSubSequence *mySequence;
    TripleOffset *myOffset[MAX_TRIPLES];

    FastSearchSequence(const FastSearchSequence&) {} // not allowed
    FastSearchSequence& operator=(const FastSearchSequence&) { return *this; } // not allowed

    static int triple_index(const char *triple)
    {
        int idx = (int)(triple[0]-'A')*SEQ_CHARS*SEQ_CHARS + (int)(triple[1]-'A')*SEQ_CHARS + (triple[2]-'A');

        return idx>=0 && idx<MAX_TRIPLES ? idx : MAX_TRIPLES-1;

        // if we got any non A-Z characters in sequence, the triple value may overflow.
        // we use (MAX_TRIPLES-1) as index for all illegal indices.
    }

public:

    FastSearchSequence(const CompactedSubSequence& seq);
    ~FastSearchSequence() {}

    ARB_ERROR fast_align(const CompactedSubSequence& align_to, AlignBuffer *alignBuffer,
                         int max_seq_length, int matchScore, int mismatchScore,
                         FastAlignReport *report) const;

    const CompactedSubSequence& sequence() const { return *mySequence; }
    const TripleOffset *find(const char *triple) const { return myOffset[triple_index(triple)]; }
};


class FastSearchOccurrence                          // iterates through all occurrences of one character triple
{
    const FastSearchSequence&  mySequence;
    const TripleOffset        *myOffset;

    void assertValid() const { fa_assert(!myOffset || myOffset!=myOffset->next()); }

public:

    FastSearchOccurrence(const FastSearchSequence& Sequence, const char *triple)
        : mySequence(Sequence)
    {
            myOffset = mySequence.find(triple);
            assertValid();
    }
    ~FastSearchOccurrence() { assertValid(); }

    int found() const { assertValid(); return myOffset!=0; }
    void gotoNext() { fa_assert(myOffset!=0); myOffset = myOffset->next(); assertValid(); }

    long offset() const { assertValid(); return myOffset->offset(); }
    const CompactedSubSequence& sequence() const   { assertValid(); return mySequence.sequence(); }
};

// -----------------------------
//      INLINE-Functions:
// -----------------------------

inline CompactedSubSequence::CompactedSubSequence(const char *seq, int len, const char *Name, int start_offset) // normal c-tor
{
    mySequence = new CompactedSequence(seq, len, Name, start_offset);
    myPos      = 0;
    myLength   = mySequence->length();
    myText     = mySequence->text()+myPos;
}

inline CompactedSubSequence::CompactedSubSequence(const CompactedSubSequence& other)
{
    mySequence = other.mySequence;
    mySequence->referred++;

    myPos    = other.myPos;
    myLength = other.myLength;
    myText   = other.myText;
}

inline CompactedSubSequence::CompactedSubSequence(const CompactedSubSequence& other, int rel_pos, int Length)
{
    mySequence = other.mySequence;
    mySequence->referred++;

    myPos    = other.myPos+rel_pos;
    myLength = Length;
    myText   = mySequence->text()+myPos;

    fa_assert((rel_pos+Length)<=mySequence->length());
    fa_assert(rel_pos>=0);
}

inline CompactedSubSequence::~CompactedSubSequence()
{
    if (mySequence->referred-- == 1) // last reference
        delete mySequence;
}

inline CompactedSubSequence& CompactedSubSequence::operator=(const CompactedSubSequence& other) // =-c-tor
{
    // d-tor part

    if (mySequence->referred-- == 1)
        delete mySequence;

    // c-tor part

    mySequence = other.mySequence;
    mySequence->referred++;

    myPos    = other.myPos;
    myLength = other.myLength;
    myText   = mySequence->text()+myPos;

    return *this;
}

#else
#error awtc_seq_search.hxx included twice
#endif // AWTC_SEQ_SEARCH_HXX
