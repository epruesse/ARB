// =============================================================== //
//                                                                 //
//   File      : adcolumns.cxx                                     //
//   Purpose   : insert/delete columns                             //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <adGene.h>
#include <arb_progress.h>
#include <arb_defs.h>
#include <cxxforward.h>

#include "gb_local.h"

#include <algorithm>

using namespace std;

// --------------------------------------------------------------------------------
// helper to hold any kind of unit (char, int, float)

class UnitPtr {
    const void *ptr;
public:
    UnitPtr() : ptr(NULL) {}
    UnitPtr(const void *ptr_)
        : ptr(ptr_)
    {
        gb_assert(ptr);
    }

    void set_pointer(const void *ptr_) {
        gb_assert(!ptr);
        ptr = ptr_;
    }
    const void *get_pointer() const { return ptr; }
    const void *expect_pointer() const { gb_assert(ptr); return ptr; }
};
struct UnitPair {
    UnitPtr left, right;
};

// --------------------------------------------------------------------------------

class AliData;
typedef SmartPtr<AliData> AliDataPtr;

class AliData {
    size_t size;
protected:
    void *inc_by_units(void *mem, size_t units) const { return reinterpret_cast<char*>(mem)+units*unitsize(); }
public:
    AliData(size_t size_) : size(size_) {}
    virtual ~AliData() {}

    virtual size_t unitsize() const                                              = 0;
    virtual void copyPartTo(void *mem, size_t start, size_t count) const         = 0;
    virtual UnitPtr unit_left_of(size_t pos) const                               = 0;
    virtual UnitPtr unit_right_of(size_t pos) const                              = 0;
    virtual AliDataPtr create_gap(size_t gapsize, const UnitPair& gapinfo) const = 0;

    size_t elems() const { return size; }
    size_t memsize() const { return unitsize()*elems(); }
    void copyTo(void *mem) const { copyPartTo(mem, 0, elems()); }
    bool empty() const { return !elems(); }

    bool is_valid_pos(size_t pos) const { return pos < elems(); }
    bool is_valid_between(size_t pos) const { return pos <= elems(); } // pos == 0 -> before first base; pos == elems() -> after last base
};

class ComposedAliData : public AliData {
    AliDataPtr left, right;

    ComposedAliData(AliDataPtr l, AliDataPtr r) : AliData(l->elems()+r->elems()), left(l), right(r) {
        gb_assert(l->unitsize() == r->unitsize());
        gb_assert(l->elems());
        gb_assert(r->elems());
    }
    friend AliDataPtr concat(AliDataPtr left, AliDataPtr right);
public:
    size_t unitsize() const OVERRIDE { return left->unitsize(); }
    AliDataPtr create_gap(size_t gapsize, const UnitPair& gapinfo) const OVERRIDE {
        return left->create_gap(gapsize, gapinfo);
    }
    void copyPartTo(void *mem, size_t start, size_t count) const OVERRIDE {
        size_t left_elems = left->elems();
        size_t take_left  = 0;
        if (start<left_elems) {
            take_left = min(count, left_elems-start);
            left->copyPartTo(mem, start, take_left);
        }

        size_t take_right = count-take_left;
        if (take_right) {
            size_t rstart = start>left_elems ? start-left_elems : 0;
            right->copyPartTo(inc_by_units(mem, take_left), rstart, count-take_left);
        }
    }
    UnitPtr unit_left_of(size_t pos) const OVERRIDE {
        gb_assert(is_valid_between(pos));
        if (left->elems() == pos) { // split between left and right
            gb_assert(pos >= 1);
            return left->unit_right_of(pos-1);
        }
        else if (left->elems() < pos) { // split inside or behind 'right'
            return right->unit_left_of(pos-left->elems());
        }
        else { // split inside or frontof 'left'
            return left->unit_left_of(pos);
        }
    }
    UnitPtr unit_right_of(size_t pos) const OVERRIDE {
        gb_assert(is_valid_between(pos));
        if (left->elems() == pos) { // split between left and right
            gb_assert(pos >= 1);
            return right->unit_left_of(0);
        }
        else if (left->elems() < pos) { // split inside or behind 'right'
            return right->unit_right_of(pos-left->elems());
        }
        else { // split inside or frontof 'left'
            return left->unit_right_of(pos);
        }
    }
};

class AliDataSlice : public AliData {
    AliDataPtr from;
    size_t     offset;

    static int fix_amount(AliDataPtr from, size_t offset, size_t amount) {
        if (amount) {
            size_t from_size = from->elems();
            if (offset>from_size) {
                amount = 0;
            }
            else {
                size_t last_pos  = offset+amount-1;
                size_t last_from = from->elems()-1;

                if (last_pos > last_from) {
                    gb_assert(last_from >= offset);
                    amount = last_from-offset+1;
                }
            }
        }
        return amount;
    }

public:
    AliDataSlice(AliDataPtr from_, size_t offset_, size_t amount_)
        : AliData(fix_amount(from_, offset_, amount_)),
          from(from_),
          offset(offset_)
    {}

    size_t unitsize() const OVERRIDE { return from->unitsize(); }
    AliDataPtr create_gap(size_t gapsize, const UnitPair& gapinfo) const OVERRIDE {
        return from->create_gap(gapsize, gapinfo);
    }
    void copyPartTo(void *mem, size_t start, size_t count) const OVERRIDE {
        gb_assert(count <= elems());
        from->copyPartTo(mem, start+offset, count);
    }
    UnitPtr unit_left_of(size_t pos) const OVERRIDE {
        gb_assert(is_valid_between(pos));
        return from->unit_left_of(pos+offset);
    }
    UnitPtr unit_right_of(size_t pos) const OVERRIDE {
        gb_assert(is_valid_between(pos));
        return from->unit_right_of(pos+offset);
    }
};

template<typename T>
class TypedAliData : public AliData {
    T gap;

protected:
    static const T *typed_ptr(const UnitPtr& uptr) { return (const T*)uptr.get_pointer(); }

    const T& std_gap() const { return gap; }
    const T* std_gap_ptr() const { return &gap; }

public:
    TypedAliData(size_t size_, T gap_)
        : AliData(size_),
          gap(gap_)
    {}

    size_t unitsize() const OVERRIDE { return sizeof(T); }
    virtual UnitPtr at_ptr(size_t pos) const = 0;
    AliDataPtr create_gap(size_t gapsize, const UnitPair& /*gapinfo*/) const OVERRIDE;

    UnitPtr unit_left_of(size_t pos) const OVERRIDE {
        gb_assert(is_valid_between(pos));
        return at_ptr(pos-1);
    }
    UnitPtr unit_right_of(size_t pos) const OVERRIDE {
        gb_assert(is_valid_between(pos));
        return at_ptr(pos);
    }
};

template<typename T>
class SpecificGap : public TypedAliData<T> {
public:
    typedef TypedAliData<T> TData;

    SpecificGap(size_t gapsize, const T& gap_)
        : TData(gapsize, gap_)
    {}
    void copyPartTo(void *mem, size_t start, size_t count) const OVERRIDE {
        if (start<TData::elems()) {
            size_t amount = min(start+count, TData::elems()-start);
            for (size_t a = 0; a<amount; ++a) {
                memcpy(mem, TData::std_gap_ptr(), TData::unitsize());
                mem = TData::inc_by_units(mem, 1);
            }
        }
    }

    UnitPtr at_ptr(size_t pos) const OVERRIDE {
        if (pos<TData::elems()) return UnitPtr(TData::std_gap_ptr());
        return UnitPtr();
    }

};

template <typename T> 
AliDataPtr TypedAliData<T>::create_gap(size_t gapsize, const UnitPair& /*gapinfo*/) const {
    return new SpecificGap<T>(gapsize, std_gap());
}

template<typename T>
class SpecificAliData : public TypedAliData<T>, virtual Noncopyable {
    T *data;

public:
    typedef TypedAliData<T> TData;

    SpecificAliData(T*& allocated_data, size_t elements, const T& gap_)
        : TData(elements, gap_),
          data(allocated_data)
    {
        allocated_data = NULL;
    }
    ~SpecificAliData() OVERRIDE { free(data); }

    void copyPartTo(void *mem, size_t start, size_t count) const OVERRIDE {
        gb_assert(start<TData::elems());
        if (count>0) {
            size_t last = start+count-1;
            gb_assert(last<TData::elems());
            size_t msize = TData::unitsize()*count;
            gb_assert(msize>0);
            memcpy(mem, data+start, msize);
        }
    }

    UnitPtr at_ptr(size_t pos) const OVERRIDE {
        if (pos<TData::elems()) return UnitPtr(&data[pos]);
        return UnitPtr();
    }

};

class SequenceAliData : public SpecificAliData<char> {
    char dot;

    char preferred_gap(const char *s1, const char *s2) const {
        if (s1 && s2) {
            if (*s1 == std_gap() || *s2 == std_gap()) {
                return std_gap();
            }
            if (*s1 == dot || *s2 == dot) {
                return dot;
            }
            return std_gap();
        }
        else if (s1) {
            gb_assert(!s2);
            return *s1 == std_gap() ? std_gap() : dot;
        }
        else if (s2) {
            gb_assert(!s1);
            return *s2 == std_gap() ? std_gap() : dot;
        }
        else {
            gb_assert(!s1 && !s2);
            return dot;
        }
    }

public:
    SequenceAliData(char*& allocated_data, size_t elements, char stdgap, char dotgap)
        : SpecificAliData<char>(allocated_data, elements, stdgap),
          dot(dotgap)
    {}

    AliDataPtr create_gap(size_t gapsize, const UnitPair& gapinfo) const OVERRIDE {
        char use = preferred_gap(typed_ptr(gapinfo.left), typed_ptr(gapinfo.right));
        return new SpecificGap<char>(gapsize, use);
    }
};

// --------------------------------------------------------------------------------
// @@@ move things below into a class ?

inline AliDataPtr concat(AliDataPtr left, AliDataPtr right) {
    return left->empty() ? right : (right->empty() ? left : new ComposedAliData(left, right));
}
inline AliDataPtr concat(AliDataPtr left, AliDataPtr mid, AliDataPtr right) {
    return concat(left, concat(mid, right));
}

inline AliDataPtr partof(AliDataPtr data, size_t pos, size_t amount) { return new AliDataSlice(data, pos, amount); }
inline AliDataPtr before(AliDataPtr data, size_t pos) { return partof(data, 0, pos); }
inline AliDataPtr after(AliDataPtr data, size_t pos) { return partof(data, pos+1, data->elems()-pos-1); }

inline AliDataPtr delete_from(AliDataPtr from, size_t pos, size_t amount) { return concat(before(from, pos), after(from, pos+amount-1)); }
inline AliDataPtr insert_at(AliDataPtr dest, size_t pos, AliDataPtr src) {
    return concat(before(dest, pos), src, after(dest, pos-1));
}

inline AliDataPtr insert_gap(AliDataPtr data, size_t pos, size_t count) {
    UnitPair gapinfo;

    gb_assert(data->unitsize() <= sizeof(gapinfo.left));

    gapinfo.left  = data->unit_left_of(pos);
    gapinfo.right = data->unit_right_of(pos);

    AliDataPtr gap = data->create_gap(count, gapinfo);
    return insert_at(data, pos, gap);
}

template<typename T> AliDataPtr makeAliData(T*& allocated_data, size_t elems, const T& gap) {
    return new SpecificAliData<T>(allocated_data, elems, gap);
}
AliDataPtr makeAliSeqData(char*& allocated_data, size_t elems, char gap, char dot) {
    return new SequenceAliData(allocated_data, elems, gap, dot);
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

template<typename T>
inline T*& copyof(const T* const_data, size_t elemsize, size_t elements) {
    static T *copy = NULL;

    size_t memsize = elemsize*elements;
    gb_assert(!copy);
    copy = (T*)malloc(memsize);
    gb_assert(copy);
    memcpy(copy, const_data, memsize);
    return copy;
}


#define COPYOF(typedarray) copyof(typedarray, sizeof(*(typedarray)), ARRAY_ELEMS(typedarray))
#define SIZEOF(typedarray) (sizeof(*(typedarray))*ARRAY_ELEMS(typedarray))

#define TEST_EXPECT_COPIES_EQUAL(d1,d2) do{                     \
        size_t  s1 = (d1)->memsize();                           \
        size_t  s2 = (d2)->memsize();                           \
        TEST_EXPECT_EQUAL(s1, s2);                              \
        void   *copy1 = malloc(s1+s2);                          \
        void   *copy2 = reinterpret_cast<char*>(copy1)+s1;      \
        (d1)->copyTo(copy1);                                    \
        (d2)->copyTo(copy2);                                    \
        TEST_EXPECT_MEM_EQUAL(copy1, copy2, s1);                \
        free(copy1);                                            \
    }while(0)

#define TEST_EXPECT_COPY_EQUALS_ARRAY(adp,typedarray,asize) do{ \
        size_t  size    = (adp)->memsize();                     \
        TEST_EXPECT_EQUAL(size, asize);                         \
        void   *ad_copy = malloc(size);                         \
        (adp)->copyTo(ad_copy);                                 \
        TEST_EXPECT_MEM_EQUAL(ad_copy, typedarray, size);       \
        free(ad_copy);                                          \
    }while(0)

#define TEST_EXPECT_COPY_EQUALS_STRING(adp,str) do{             \
        size_t  size    = (adp)->memsize();                     \
        char   *ad_copy = (char*)malloc(size+1);                \
        (adp)->copyTo(ad_copy);                                 \
        ad_copy[size]   = 0;                                    \
        TEST_EXPECT_EQUAL(ad_copy, str);                        \
        free(ad_copy);                                          \
    }while(0)

#if defined(ENABLE_CRASH_TESTS)
static void illegal_alidata_composition() {
    const int ELEMS = 5;

    int  *i = (int*)malloc(sizeof(int)*ELEMS);
    char *c = (char*)malloc(sizeof(char)*ELEMS);

    concat(makeAliData(i, ELEMS, 0), makeAliData(c, ELEMS, '-'));
}
#endif

void TEST_AliData() {
#define SEQDATA "CGCAC-C-GG-C-GG.A.-C------GG-.C..UCAGU"
    char      chr_src[] = SEQDATA; // also contains trailing 0-byte!
    GB_CUINT4 int_src[] = { 0x01, 0x1213, 0x242526, 0x37383930, 0xffffffff };
    float     flt_src[] = { 0.0, 0.5, 1.0, -5.0, 20.1 };

    AliDataPtr type[] = {
        makeAliSeqData(COPYOF(chr_src), ARRAY_ELEMS(chr_src)-1, '-', '.'),
        makeAliData(COPYOF(int_src), ARRAY_ELEMS(int_src), 0U),
        makeAliData(COPYOF(flt_src), ARRAY_ELEMS(flt_src), 0.0F)
    };
    TEST_EXPECT_COPY_EQUALS_ARRAY(type[0], chr_src, SIZEOF(chr_src)-1);
    TEST_EXPECT_COPY_EQUALS_STRING(type[0], chr_src);
    TEST_EXPECT_COPY_EQUALS_ARRAY(type[1], int_src, SIZEOF(int_src));
    TEST_EXPECT_COPY_EQUALS_ARRAY(type[2], flt_src, SIZEOF(flt_src));

    for (size_t t = 0; t<ARRAY_ELEMS(type); ++t) {
        AliDataPtr data  = type[t];
        AliDataPtr dup = concat(data, data);
        TEST_EXPECT_EQUAL(dup->elems(), 2*data->elems());

        AliDataPtr start = before(data, 3);
        TEST_EXPECT_EQUAL(start->elems(), 3U);

        AliDataPtr end = after(data, 3);
        TEST_EXPECT_EQUAL(end->elems(), data->elems()-4);

        AliDataPtr mid = partof(data, 3, 1);
        TEST_EXPECT_COPIES_EQUAL(concat(start, mid, end), data);

        AliDataPtr del = delete_from(data, 3, 1);
        TEST_EXPECT_EQUAL(del->elems(), data->elems()-1);
        TEST_EXPECT_COPIES_EQUAL(concat(start, end), del);

        AliDataPtr empty = before(data, 0);
        TEST_EXPECT_EQUAL(empty->elems(), 0U);

        TEST_EXPECT_COPIES_EQUAL(data, concat(data, empty));
        TEST_EXPECT_COPIES_EQUAL(data, concat(empty, data));
        TEST_EXPECT_COPIES_EQUAL(empty, concat(empty, empty));

        AliDataPtr del_rest = delete_from(data, 3, 999);
        TEST_EXPECT_COPIES_EQUAL(start, del_rest);

        AliDataPtr ins = insert_at(del, 3, mid);
        TEST_EXPECT_COPIES_EQUAL(data, ins);
        TEST_EXPECT_COPIES_EQUAL(del, delete_from(ins, 3, 1));

        TEST_EXPECT_COPIES_EQUAL(insert_at(del, 3, empty), del);
        TEST_EXPECT_COPIES_EQUAL(insert_at(del, 777, empty), del);      // append via insert_at
        TEST_EXPECT_COPIES_EQUAL(insert_at(start, 777, end), del); // append via insert_at

        AliDataPtr ins_gap = insert_gap(del, 4, 5);
        TEST_EXPECT_EQUAL(ins_gap->elems(), del->elems()+5);

        AliDataPtr gap_iseq = partof(ins_gap, 4, 5);

        TEST_EXPECT_COPIES_EQUAL(ins_gap, insert_gap(ins_gap, 7, 0)); // insert empty gap

        AliDataPtr start_gap1 = insert_gap(ins_gap, 0, 1); // insert gap at start
        AliDataPtr start_gap3 = insert_gap(ins_gap, 0, 3); // insert gap at start

        AliDataPtr gap_iempty = insert_gap(empty, 0, 5);
        TEST_EXPECT_EQUAL(gap_iempty->elems(), 5U);

        AliDataPtr gap_in_gap = insert_gap(gap_iempty, 3, 2);
        TEST_EXPECT_EQUAL(gap_in_gap->elems(), 7U);

        AliDataPtr end_gap1 = insert_gap(mid, 1, 1);
        TEST_EXPECT_EQUAL(end_gap1->elems(), 2U);

        if (t == 0) {
            AliDataPtr end_gap2 = insert_gap(end, 34, 2);

            TEST_EXPECT_COPY_EQUALS_STRING(start,      "CGC");
            TEST_EXPECT_COPY_EQUALS_STRING(end,        "C-C-GG-C-GG.A.-C------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(end_gap2,   "C-C-GG-C-GG.A.-C------GG-.C..UCAGU..");
            TEST_EXPECT_COPY_EQUALS_STRING(mid,        "A");
            TEST_EXPECT_COPY_EQUALS_STRING(end_gap1,   "A-");    // '-' is ok, since before there was a C behind (but correct would be '.')
            TEST_EXPECT_COPY_EQUALS_STRING(del,        "CGCC-C-GG-C-GG.A.-C------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(del_rest,   "CGC");
            TEST_EXPECT_COPY_EQUALS_STRING(ins,        "CGCAC-C-GG-C-GG.A.-C------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(gap_iseq,   "-----"); // inserted between bases
            TEST_EXPECT_COPY_EQUALS_STRING(gap_iempty, "....."); // inserted in empty sequence
            TEST_EXPECT_COPY_EQUALS_STRING(gap_in_gap, "......."); // inserted gap in gap
            TEST_EXPECT_COPY_EQUALS_STRING(ins_gap,    "CGCC------C-GG-C-GG.A.-C------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(start_gap1, ".CGCC------C-GG-C-GG.A.-C------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(start_gap3, "...CGCC------C-GG-C-GG.A.-C------GG-.C..UCAGU");

            AliDataPtr bef_dot     = insert_gap(ins, 15, 2);
            AliDataPtr aft_dot     = insert_gap(ins, 16, 2);
            AliDataPtr bet_dots    = insert_gap(ins, 32, 2);
            AliDataPtr bet_dashes  = insert_gap(ins, 23, 2);
            AliDataPtr bet_dashdot = insert_gap(ins, 29, 2);
            AliDataPtr bet_dotdash = insert_gap(ins, 18, 2);

            TEST_EXPECT_COPY_EQUALS_STRING(ins,        "CGCAC-C-GG-C-GG.A.-C------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(bef_dot,    "CGCAC-C-GG-C-GG...A.-C------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(aft_dot,    "CGCAC-C-GG-C-GG...A.-C------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(bet_dots,   "CGCAC-C-GG-C-GG.A.-C------GG-.C....UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(bet_dashes, "CGCAC-C-GG-C-GG.A.-C--------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(bet_dashdot,"CGCAC-C-GG-C-GG.A.-C------GG---.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(bet_dotdash,"CGCAC-C-GG-C-GG.A.---C------GG-.C..UCAGU");
        }

    }

    TEST_EXPECT_CODE_ASSERTION_FAILS(illegal_alidata_composition); // composing different unitsizes shall fail
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

static char   *insDelBuffer = 0;
static size_t  insDelBuffer_size;

inline void free_insDelBuffer() {
    freenull(insDelBuffer);
}

static const char *gbt_insert_delete(const char *source, long srclen, long destlen, size_t& newlen, long pos, long nchar, long mod, char insert_what, char insert_tail, int extraByte) {
    /* removes elems from or inserts elems into an array
     *
     * srclen           len of source
     * destlen          if != 0, then cut or append characters to get this len, otherwise keep srclen
     * newlenPtr        the resulting len
     * pos              where to insert/delete
     * nchar            and how many items
     * mod              size of an item
     * insert_what      insert this character (mod times)
     * insert_tail      append this character (if destlen>srclen)
     * extraByte        0 or 1. append extra zero byte at end? use 1 for strings!
     *
     * resulting array has destlen+nchar elements
     *
     * 1. array size is corrected to 'destlen' (by appending/cutting tail)
     * 2. part is deleted inserted
     *
     * return NULL, if nothing modified
     */

    const char *result;

    pos     *= mod;
    nchar   *= mod;
    srclen  *= mod;
    destlen *= mod;

    if (!destlen) destlen                       = srclen; // if no destlen is set then keep srclen
    if ((nchar<0) && (pos-nchar>destlen)) nchar = pos-destlen; // clip maximum characters to delete at end of array

    if (destlen == srclen && (pos>srclen || nchar == 0)) { // length stays same and clip-range is empty or behind end of sequence
        /* before 26.2.09 the complete data was copied in this case - but nevertheless NULL(=failure) was returned.
         * I guess this was some test accessing complete data w/o writing anything back to DB,
         * but AFAIK it was not used anywhere --ralf
         */
        result = NULL;
        newlen = 0;
    }
    else {
        newlen = destlen+nchar;                // length of result (w/o trailing zero-byte)
        if (newlen == 0) {
            result = "";
        }
        else {
            size_t neededSpace = newlen+extraByte;

            if (insDelBuffer && insDelBuffer_size<neededSpace) freenull(insDelBuffer);
            if (!insDelBuffer) {
                insDelBuffer_size = neededSpace;
                insDelBuffer      = (char*)malloc(neededSpace);
            }

            char *dest = insDelBuffer;
            gb_assert(dest);

            if (pos>srclen) {                       // insert/delete happens inside appended range
                insert_what = insert_tail;
                pos         = srclen;               // insert/delete directly after source, to avoid illegal access below
            }

            gb_assert(pos >= 0);
            if (pos>0) {                            // copy part left of pos
                // @@@ need to check vs smaller newlen
                memcpy(dest, source, (size_t)pos);
                dest   += pos;
                source += pos; srclen -= pos;
            }

            if (nchar>0) {                          // insert
                memset(dest, insert_what, (size_t)nchar);
                dest += nchar;
            }
            else if (nchar<0) {                     // delete
                source += -nchar; srclen -= -nchar;
            }

            if (srclen>0) {                         // copy rest of source
                // @@@ need to check vs smaller newlen
                memcpy(dest, source, (size_t)srclen);
                dest   += srclen;
                source += srclen; srclen = 0;
            }

            long rest = newlen-(dest-insDelBuffer);
            // gb_assert(rest >= 0); // @@@ disabled (fails in test-code below)

            if (rest>0) {                           // append tail
                memset(dest, insert_tail, rest);
                dest += rest;
            }

            if (extraByte) dest[0] = 0;             // append zero byte (used for strings)

            result = insDelBuffer;
        }
        newlen = newlen/mod;                    // report result length
    }
    return result;
}

enum TargetType {
    IDT_SPECIES = 0,
    IDT_SAI,
    IDT_SECSTRUCT,
};

static GB_CSTR targetTypeName[] = {
    "Species",
    "SAI",
    "SeceditStruct",
};

class Alignment {
    SmartCharPtr name; // name of alignment
    size_t       len;  // length of alignment
public:
    Alignment(const char *name_, size_t len_) : name(strdup(name_)), len(len_) {}

    const char *get_name() const { return &*name; }
    size_t get_len() const { return len; }
};

// --------------------------------------------------------------------------------

class AliApplicable { // something that can be appied to the whole alignment
    virtual GB_ERROR apply_to_terminal(GBDATA *gb_data, TargetType ttype, const Alignment& ali) const = 0;

    GB_ERROR apply_recursive(GBDATA *gb_data, TargetType ttype, const Alignment& ali) const;
    GB_ERROR apply_to_childs_named(GBDATA *gb_item_data, const char *item_field, TargetType ttype, const Alignment& ali) const;
    GB_ERROR apply_to_secstructs(GBDATA *gb_secstructs, const Alignment& ali) const;

public:
    AliApplicable() {}
    virtual ~AliApplicable() {}

    GB_ERROR apply_to_alignment(GBDATA *gb_main, const Alignment& ali) const;
};

GB_ERROR AliApplicable::apply_recursive(GBDATA *gb_data, TargetType ttype, const Alignment& ali) const {
    GB_ERROR error = 0;
    GB_TYPES type  = GB_read_type(gb_data);

    if (type == GB_DB) {
        GBDATA *gb_child;
        for (gb_child = GB_child(gb_data); gb_child && !error; gb_child = GB_nextChild(gb_child)) {
            error = apply_recursive(gb_child, ttype, ali);
        }
    }
    else {
        error = apply_to_terminal(gb_data, ttype, ali);
    }

    return error;
}
GB_ERROR AliApplicable::apply_to_childs_named(GBDATA *gb_item_data, const char *item_field, TargetType ttype, const Alignment& ali) const {
    GBDATA   *gb_item;
    GB_ERROR  error      = 0;
    long      item_count = GB_number_of_subentries(gb_item_data);

    if (item_count) {
        for (gb_item = GB_entry(gb_item_data, item_field);
             gb_item && !error;
             gb_item = GB_nextEntry(gb_item))
        {
            GBDATA *gb_ali = GB_entry(gb_item, ali.get_name());
            if (gb_ali) {
                error = apply_recursive(gb_ali, ttype, ali);
                if (error) {
                    const char *item_name = GBT_read_name(gb_item);
                    error = GBS_global_string("%s '%s': %s", targetTypeName[ttype], item_name, error);
                }
            }
        }
    }
    return error;
}
GB_ERROR AliApplicable::apply_to_secstructs(GBDATA *gb_secstructs, const Alignment& ali) const {
    GB_ERROR  error  = 0;
    GBDATA   *gb_ali = GB_entry(gb_secstructs, ali.get_name());

    if (gb_ali) {
        long item_count = GB_number_of_subentries(gb_ali)-1;
        if (item_count<1) item_count = 1;

        GBDATA *gb_item;
        for (gb_item = GB_entry(gb_ali, "struct");
             gb_item && !error;
             gb_item = GB_nextEntry(gb_item))
        {
            GBDATA *gb_ref = GB_entry(gb_item, "ref");
            if (gb_ref) {
                error = apply_recursive(gb_ref, IDT_SECSTRUCT, ali);
                if (error) {
                    const char *item_name = GBT_read_name(gb_item);
                    error = GBS_global_string("%s '%s': %s", targetTypeName[IDT_SECSTRUCT], item_name, error);
                }
            }
        }
    }
    return error;
}

GB_ERROR AliApplicable::apply_to_alignment(GBDATA *gb_main, const Alignment& ali) const {
    GB_ERROR error    = apply_to_childs_named(GBT_find_or_create(gb_main, "species_data",  7), "species",  IDT_SPECIES, ali);
    if (!error) error = apply_to_childs_named(GBT_find_or_create(gb_main, "extended_data", 7), "extended", IDT_SAI,     ali);
    if (!error) error = apply_to_secstructs(GB_search(gb_main, "secedit/structs", GB_CREATE_CONTAINER), ali);
    return error;
}

// --------------------------------------------------------------------------------

class AliEntryCounter : public AliApplicable {
    mutable size_t count;
    GB_ERROR apply_to_terminal(GBDATA *, TargetType , const Alignment& ) const OVERRIDE { count++; return NULL; }
public:
    AliEntryCounter() : count(0) {}
    size_t get_entry_count() const { return count; }
};

// --------------------------------------------------------------------------------

class AliEditCommand : virtual Noncopyable {
    long pos;        // start position of insert/delete
    long nchar;      // number of elements to insert/delete

    const char *delete_chars; // characters allowed to delete (array with 256 entries, value == 0 means deletion allowed)
public: 
    AliEditCommand(long pos_, long nchar_, const char *delete_chars_)
        : pos(pos_),
          nchar(nchar_),
          delete_chars(delete_chars_)
    {
        gb_assert(pos >= 0);
        // gb_assert(correlated(nchar<0, delete_chars)); // @@@
    }
    virtual ~AliEditCommand() {}

    virtual bool might_modify(size_t , const Alignment& ) const { return nchar != 0; }

    long get_pos() const { return pos; }
    long get_amount() const { return nchar; }
    bool allowed_to_delete(char c) const { return delete_chars[safeCharIndex(c)] == 1; }
};

struct AliFormat : public AliEditCommand {
    AliFormat() : AliEditCommand(0, 0, 0) {}
    bool might_modify(size_t term_size, const Alignment& ali) const OVERRIDE { return term_size != ali.get_len(); }
};

// --------------------------------------------------------------------------------

class AliEditor : public AliApplicable {
    const AliEditCommand& cmd;
    mutable arb_progress  progress;

    GB_ERROR apply_to_terminal(GBDATA *gb_data, TargetType ttype, const Alignment& ali) const OVERRIDE;

    bool shall_edit(GBDATA *gb_data, TargetType ttype) const {
        // defines whether specific DB-elements shall be edited by any AliEditor
        // (true for all data, that contains alignment position specific data)

        const char *key   = GB_read_key_pntr(gb_data);
        bool        apply = key[0] != '_';                                // general case: don't apply to keys starting with '_'
        if (!apply) apply = ttype == IDT_SAI && strcmp(key, "_REF") == 0; // exception (SAI:_REF needs editing)
        return apply;
    }

public:
    AliEditor(const AliEditCommand& cmd_, const char *progress_title, size_t progress_count)
        : cmd(cmd_),
          progress(progress_title, progress_count)
    {
    }
    ~AliEditor() OVERRIDE {
        progress.done();
    }

    const AliEditCommand& edit_command() const { return cmd; }
};

inline int alignment_oversize(GBDATA *gb_data, TargetType ttype, long alilen, long found_size) {
    int oversize = 0;
    const char *key = GB_read_key_pntr(gb_data);
    switch (ttype) {
        case IDT_SPECIES:
            break;
        case IDT_SECSTRUCT:
            if (strcmp(key, "ref") == 0) { // 'ref' may be one byte longer than alignment size
                oversize = found_size-alilen;
            }
            break;
        case IDT_SAI:
            if (strcmp(key, "_REF") == 0) { // _REF may be one byte longer than alignment size
                oversize = found_size-alilen;
            }
            break;
    }
    return oversize;
}

class EditedTerminal {
    GBDATA   *gb_data;
    GB_TYPES  type;

    GB_CSTR source;
    long    size;
    long    modulo;

    char insert_what; // @@@ information about insert is specific for ONE explicit insert, i.e. information is misplaced here

    char insert_tail;

    char extraByte; // special flag for zero-terminated strings

    GB_ERROR error;

    void prepare(const AliEditCommand& cmd) {
        if (!error && source && type == GB_STRING) {
            if (cmd.get_amount() > 0) { // insert
                if (cmd.get_pos()<size) { // otherwise insert position is behind (old and too short) sequence -> dots are inserted at tail
                    if ((cmd.get_pos()>0 && source[cmd.get_pos()-1] == '.') || source[cmd.get_pos()] == '.') { // dot at insert position?
                        insert_what = '.'; // insert dots
                    }
                }
            }
            else { // delete
                long after            = cmd.get_pos()+(-cmd.get_amount()); // position after deleted part
                if (after>size) after = size;

                for (long p = cmd.get_pos(); p<after; p++) {
                    if (cmd.allowed_to_delete(source[p])) {
                        error = GBS_global_string("You tried to delete '%c' at position %li  -> Operation aborted", source[p], p);
                    }
                }
            }
        }
    }

public:
    EditedTerminal(GBDATA *gb_data_, GB_TYPES type_, long size_)
        : gb_data(gb_data_),
          type(type_),
          source(NULL),
          size(size_),
          modulo(sizeof(char)),
          insert_what(0),
          insert_tail(0),
          extraByte(0),
          error(NULL)
    {
        switch(type) {
            case GB_STRING: source = GB_read_char_pntr(gb_data);            insert_what = '-'; insert_tail = '.'; extraByte = 1; break;
            case GB_BITS:   source = GB_read_bits_pntr(gb_data, '-', '+');  insert_what = '-'; insert_tail = '-'; break;
            case GB_BYTES:  source = GB_read_bytes_pntr(gb_data);           break;
            case GB_INTS:   source = (GB_CSTR)GB_read_ints_pntr(gb_data);   modulo = sizeof(GB_UINT4); break;
            case GB_FLOATS: source = (GB_CSTR)GB_read_floats_pntr(gb_data); modulo = sizeof(float); break;

            default:
                error = GBS_global_string("Unhandled type '%i'", type);
                gb_assert(0);
                break;
        }
    }

    GB_ERROR apply(const AliEditCommand& cmd, TargetType ttype, const Alignment& ali) {
        if (!error) prepare(cmd);
        if (!error) {
            if (!source) error = GB_await_error();
            else {
                size_t  modified_len;
                int     oversize   = alignment_oversize(gb_data, ttype, ali.get_len(), size);
                long    wanted_len = ali.get_len() + oversize;
                GB_CSTR modified   = gbt_insert_delete(source, size, wanted_len, modified_len, cmd.get_pos(), cmd.get_amount(), modulo, insert_what, insert_tail, extraByte);

                if (modified) {
                    gb_assert(modified_len == (ali.get_len()+cmd.get_amount()+oversize));

                    switch (type) {
                        case GB_STRING: error = GB_write_string(gb_data, modified);                          break;
                        case GB_BITS:   error = GB_write_bits  (gb_data, modified, modified_len, "-");       break;
                        case GB_BYTES:  error = GB_write_bytes (gb_data, modified, modified_len);            break;
                        case GB_INTS:   error = GB_write_ints  (gb_data, (GB_UINT4*)modified, modified_len); break;
                        case GB_FLOATS: error = GB_write_floats(gb_data, (float*)modified, modified_len);    break;

                        default: gb_assert(0); break;
                    }
                }
            }
        }
        return error;
    }
};

GB_ERROR AliEditor::apply_to_terminal(GBDATA *gb_data, TargetType ttype, const Alignment& ali) const {
    GB_TYPES gbtype  = GB_read_type(gb_data);
    GB_ERROR error = NULL;
    if (gbtype >= GB_BITS && gbtype != GB_LINK) {
        long size = GB_read_count(gb_data);

        if (edit_command().might_modify(size, ali)) {
            if (shall_edit(gb_data, ttype)) {
                error = EditedTerminal(gb_data, gbtype, size).apply(edit_command(), ttype, ali);
            }
        }
    }
    progress.inc_and_check_user_abort(error);
    return error;
}

// --------------------------------------------------------------------------------

static size_t countAffectedEntries(GBDATA *Main, const Alignment& ali) {
    AliEntryCounter counter;
    counter.apply_to_alignment(Main, ali);
    return counter.get_entry_count();
}

static GB_ERROR GBT_check_lengths(GBDATA *Main, const char *alignment_name) {
    GB_ERROR  error      = 0;
    GBDATA   *gb_presets = GBT_get_presets(Main);
    GBDATA   *gb_ali;

    AliFormat op;

    for (gb_ali = GB_entry(gb_presets, "alignment");
         gb_ali && !error;
         gb_ali = GB_nextEntry(gb_ali))
    {
        GBDATA *gb_name = GB_find_string(gb_ali, "alignment_name", alignment_name, GB_IGNORE_CASE, SEARCH_CHILD);

        if (gb_name) {
            GBDATA    *gb_len = GB_entry(gb_ali, "alignment_len");
            Alignment  ali(GB_read_char_pntr(gb_name), GB_read_int(gb_len));

            error = AliEditor(op, "Formatting alignment", countAffectedEntries(Main, ali))
                .apply_to_alignment(Main, ali);
        }
    }
    free_insDelBuffer();
    return error;
}

GB_ERROR GBT_format_alignment(GBDATA *Main, const char *alignment_name) {
    GB_ERROR err = 0;

    if (strcmp(alignment_name, GENOM_ALIGNMENT) != 0) { // NEVER EVER format 'ali_genom'
        err           = GBT_check_data(Main, alignment_name); // detect max. length
        if (!err) err = GBT_check_lengths(Main, alignment_name); // format sequences in alignment
        if (!err) err = GBT_check_data(Main, alignment_name); // sets state to "formatted"
    }
    else {
        err = "It's forbidden to format '" GENOM_ALIGNMENT "'!";
    }
    return err;
}


GB_ERROR GBT_insert_character(GBDATA *Main, const char *alignment_name, long pos, long count, const char *char_delete)
{
    /* if count > 0     insert 'count' characters at pos
     * if count < 0     delete pos to pos+|count|
     *
     * Note: deleting is only performed, if found characters in deleted range are listed in 'char_delete'
     *       otherwise function returns with error
     *
     * This affects all species' and SAIs having data in given 'alignment_name' and
     * modifies several data entries found there
     * (see shall_edit() for details which fields are affected).
     */

    GB_ERROR error = 0;

    if (pos<0) {
        error = GBS_global_string("Illegal sequence position %li", pos);
    }
    else {
        GBDATA *gb_ali;
        GBDATA *gb_presets = GBT_get_presets(Main);
        char    char_delete_list[256];

        if (strchr(char_delete, '%')) {
            memset(char_delete_list, 0, 256);
        }
        else {
            int ch;
            for (ch = 0; ch<256; ch++) {
                if (strchr(char_delete, ch)) char_delete_list[ch] = 0;
                else                         char_delete_list[ch] = 1;
            }
        }

        for (gb_ali = GB_entry(gb_presets, "alignment");
             gb_ali && !error;
             gb_ali = GB_nextEntry(gb_ali))
        {
            GBDATA *gb_name = GB_find_string(gb_ali, "alignment_name", alignment_name, GB_IGNORE_CASE, SEARCH_CHILD);

            if (gb_name) {
                GBDATA *gb_len = GB_entry(gb_ali, "alignment_len");
                long    len    = GB_read_int(gb_len);
                char   *use    = GB_read_string(gb_name);

                if (pos > len) {
                    error = GBS_global_string("Can't insert at position %li (exceeds length %li of alignment '%s')", pos, len, use);
                }
                else {
                    if (count < 0 && pos-count > len) count = pos - len;
                    error = GB_write_int(gb_len, len + count);
                }

                if (!error) {
                    Alignment      ali(use, len);
                    AliEditCommand op(pos, count, char_delete_list);

                    error = AliEditor(op, "Insert/delete characters", countAffectedEntries(Main, ali))
                        .apply_to_alignment(Main, ali);
                }
                free(use);
            }
        }

        free_insDelBuffer();

        if (!error) GB_disable_quicksave(Main, "a lot of sequences changed");
    }
    return error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif
#include <arb_unit_test.h>

#define DO_INSERT(str,alilen,pos,amount)                                                                \
    size_t      newlen;                                                                                 \
    const char *res = gbt_insert_delete(str, strlen(str), alilen, newlen, pos, amount, 1, '-', '.', 1)

#define TEST_INSERT(str,alilen,pos,amount,expected)         do { DO_INSERT(str,alilen,pos,amount); TEST_EXPECT_EQUAL(res, expected);         } while(0)
#define TEST_INSERT__BROKEN(str,alilen,pos,amount,expected) do { DO_INSERT(str,alilen,pos,amount); TEST_EXPECT_EQUAL__BROKEN(res, expected); } while(0)

#define TEST_FORMAT(str,alilen,expected)         TEST_INSERT(str,alilen,0,0,expected)
#define TEST_FORMAT__BROKEN(str,alilen,expected) TEST_INSERT__BROKEN(str,alilen,0,0,expected)

#define TEST_DELETE(str,alilen,pos,amount,expected)         TEST_INSERT(str,alilen,pos,-(amount),expected)
#define TEST_DELETE__BROKEN(str,alilen,pos,amount,expected) TEST_INSERT__BROKEN(str,alilen,pos,-(amount),expected)

void TEST_insert_delete() {
    TEST_FORMAT("xxx",   5, "xxx..");
    TEST_FORMAT(".x.",   5, ".x...");
    TEST_FORMAT(".x..",  5, ".x...");
    TEST_FORMAT(".x...", 5, NULL); // NULL means "result == source"

    TEST_FORMAT__BROKEN("xxx--", 3, "xxx");
    TEST_FORMAT__BROKEN("xxx..", 3, "xxx");
    TEST_FORMAT__BROKEN("xxxxx", 3, "xxx");
    TEST_FORMAT__BROKEN("xxx",   0, "");

    // insert/delete in the middle
    TEST_INSERT("abcde", 5, 3, 0, NULL);
    TEST_INSERT("abcde", 5, 3, 1, "abc-de");
    TEST_INSERT("abcde", 5, 3, 2, "abc--de");

    TEST_DELETE("abcde",   5, 3, 0, NULL);
    TEST_DELETE("abc-de",  5, 3, 1, "abcde");
    TEST_DELETE("abc--de", 5, 3, 2, "abcde");

    // insert/delete at end
    TEST_INSERT("abcde", 5, 5, 1, "abcde-");
    TEST_INSERT("abcde", 5, 5, 4, "abcde----");

    TEST_DELETE__BROKEN("abcde-",    5, 5, 1, "abcde");
    TEST_DELETE__BROKEN("abcde----", 5, 5, 4, "abcde");

    // insert/delete at start
    TEST_INSERT("abcde", 5, 0, 1, "-abcde");
    TEST_INSERT("abcde", 5, 0, 4, "----abcde");

    TEST_DELETE("-abcde",    5, 0, 1, "abcde");
    TEST_DELETE("----abcde", 5, 0, 4, "abcde");


    // insert behind end
    TEST_INSERT__BROKEN("abcde", 10, 8, 1, "abcde...-.."); // expected_behavior?
    TEST_INSERT        ("abcde", 10, 8, 1, "abcde......"); // current_behavior

    TEST_INSERT__BROKEN("abcde", 10, 8, 4, "abcde...----.."); // expected_behavior?
    TEST_INSERT        ("abcde", 10, 8, 4, "abcde........."); // current_behavior

    // insert/delete all
    TEST_INSERT("",    0, 0, 3, "---");
    TEST_DELETE("---", 3, 0, 3, "");

    free_insDelBuffer();
}

// ------------------------------

struct arb_unit_test::test_alignment_data TADinsdel[] = {
    { 1, "MtnK1722", "...G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUCACCUCC....." },
    { 1, "MhnFormi", "---A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUCACCUCCU....." },
    { 1, "MhnT1916", "...A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUCACCUCCU----" },
};

struct arb_unit_test::test_alignment_data EXTinsdel[] = {
    { 0, "ECOLI",    "---U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCCCACCUGA...." },
    { 0, "HELIX",    ".....[<[.........[[..[<<.[..].>>]....]]....].>......]" },
    { 0, "HELIX_NR", ".....1.1.........22..34..34.34..34...22....1........1" },
};

#define HELIX_REF    ".....x..x........x.x.x....x.x....x...x.x...x.........x"
#define HELIX_STRUCT "VERSION=3\nLOOP={etc.pp\n}\n"

static const char *read_item_entry(GBDATA *gb_item, const char *ali_name, const char *entry_name) {
    const char *result = NULL;
    if (gb_item) {
        GBDATA *gb_ali = GB_find(gb_item, ali_name, SEARCH_CHILD);
        if (gb_ali) {
            GBDATA *gb_entry = GB_entry(gb_ali, entry_name);
            if (gb_entry) {
                result = GB_read_char_pntr(gb_entry);
            }
        }
    }
    if (!result) TEST_EXPECT_NO_ERROR(GB_await_error());
    return result;
}
static char *ints2string(const GB_UINT4 *ints, size_t count) {
    char *str = (char*)malloc(count+1);
    for (size_t c = 0; c<count; ++c) {
        str[c] = (ints[c]<10) ? ints[c]+'0' : '?';
    }
    str[count] = 0;
    return str;
}
static GB_UINT4 *string2ints(const char *str, size_t count) {
    GB_UINT4 *ints = (GB_UINT4*)malloc(sizeof(GB_UINT4)*count);
    for (size_t c = 0; c<count; ++c) {
        ints[c] = int(str[c]-'0');
    }
    return ints;
}
static char *floats2string(const float *floats, size_t count) {
    char *str = (char*)malloc(count+1);
    for (size_t c = 0; c<count; ++c) {
        str[c] = char(floats[c]*64.0+0.5)+' '+1;
    }
    str[count] = 0;
    return str;
}
static float *string2floats(const char *str, size_t count) {
    float *floats = (float*)malloc(sizeof(float)*count);
    for (size_t c = 0; c<count; ++c) {
        floats[c] = float(str[c]-' '-1)/64.0;
    }
    return floats;
}

static GBDATA *get_ali_entry(GBDATA *gb_item, const char *ali_name, const char *entry_name) {
    GBDATA *gb_entry = NULL;
    if (gb_item) {
        GBDATA *gb_ali = GB_find(gb_item, ali_name, SEARCH_CHILD);
        if (gb_ali) gb_entry = GB_entry(gb_ali, entry_name);
    }
    return gb_entry;
}

static char *read_item_ints_entry_as_string(GBDATA *gb_item, const char *ali_name, const char *entry_name) {
    char   *result   = NULL;
    GBDATA *gb_entry = get_ali_entry(gb_item, ali_name, entry_name);
    if (gb_entry) {
        GB_UINT4 *ints = GB_read_ints(gb_entry);
        result         = ints2string(ints, GB_read_count(gb_entry));
        free(ints);
    }
    if (!result) TEST_EXPECT_NO_ERROR(GB_await_error());
    return result;
}
static char *read_item_floats_entry_as_string(GBDATA *gb_item, const char *ali_name, const char *entry_name) {
    char   *result   = NULL;
    GBDATA *gb_entry = get_ali_entry(gb_item, ali_name, entry_name);
    if (gb_entry) {
        float *floats = GB_read_floats(gb_entry);
        result         = floats2string(floats, GB_read_count(gb_entry));
        free(floats);
    }
    if (!result) TEST_EXPECT_NO_ERROR(GB_await_error());
    return result;
}

#define TEST_ITEM_HAS_ENTRY(find,name,ename,expected)                                   \
    TEST_EXPECT_EQUAL(read_item_entry(find(gb_main, name), ali_name, ename), expected)

#define TEST_ITEM_HAS_INTSENTRY(find,name,ename,expected)                                                               \
    TEST_EXPECT_EQUAL(&*SmartCharPtr(read_item_ints_entry_as_string(find(gb_main, name), ali_name, ename)), expected)

#define TEST_ITEM_HAS_FLOATSENTRY(find,name,ename,expected)                                                             \
    TEST_EXPECT_EQUAL(&*SmartCharPtr(read_item_floats_entry_as_string(find(gb_main, name), ali_name, ename)), expected)

#define TEST_ITEM_HAS_DATA(find,name,expected) TEST_ITEM_HAS_ENTRY(find,name,"data",expected)

#define TEST_SPECIES_HAS_DATA(ad,sd)    TEST_ITEM_HAS_DATA(GBT_find_species,ad.name,sd)
#define TEST_SAI_HAS_DATA(ad,sd)        TEST_ITEM_HAS_DATA(GBT_find_SAI,ad.name,sd)
#define TEST_SAI_HAS_ENTRY(ad,ename,sd) TEST_ITEM_HAS_ENTRY(GBT_find_SAI,ad.name,ename,sd)

#define TEST_SPECIES_HAS_INTS(ad,id)   TEST_ITEM_HAS_INTSENTRY(GBT_find_species,ad.name,"NN",id)
#define TEST_SPECIES_HAS_FLOATS(ad,fd) TEST_ITEM_HAS_FLOATSENTRY(GBT_find_species,ad.name,"FF",fd)

#define TEST_DATA(sd0,sd1,sd2,ed0,ed1,ed2,ref,ints,floats,struct) do {                          \
        TEST_SPECIES_HAS_DATA(TADinsdel[0], sd0);                                               \
        TEST_SPECIES_HAS_DATA(TADinsdel[1], sd1);                                               \
        TEST_SPECIES_HAS_DATA(TADinsdel[2], sd2);                                               \
        TEST_SAI_HAS_DATA(EXTinsdel[0], ed0);                                                   \
        TEST_SAI_HAS_DATA(EXTinsdel[1], ed1);                                                   \
        TEST_SAI_HAS_DATA(EXTinsdel[2], ed2);                                                   \
        TEST_SAI_HAS_ENTRY(EXTinsdel[1], "_REF", ref);                                          \
        GBDATA *gb_ref = GB_search(gb_main, "secedit/structs/ali_mini/struct/ref", GB_FIND);    \
        TEST_EXPECT_EQUAL(GB_read_char_pntr(gb_ref), ref);                                      \
        TEST_SPECIES_HAS_INTS(TADinsdel[0], ints);                                              \
        TEST_SPECIES_HAS_FLOATS(TADinsdel[0], floats);                                          \
        TEST_SAI_HAS_ENTRY(EXTinsdel[1], "_STRUCT", struct);                                    \
    } while(0)

static int get_alignment_aligned(GBDATA *gb_main, const char *aliname) { // former GBT_get_alignment_aligned
    GBDATA *gb_alignment = GBT_get_alignment(gb_main, aliname);
    return gb_alignment ? *GBT_read_int(gb_alignment, "aligned") : -1;
}

#define TEST_ALI_LEN_ALIGNED(len,aligned) do {                                  \
        TEST_EXPECT_EQUAL(GBT_get_alignment_len(gb_main, ali_name), len);       \
        TEST_EXPECT_EQUAL(get_alignment_aligned(gb_main, ali_name), aligned);   \
    } while(0)

void TEST_insert_delete_DB() {
    GB_shell    shell;
    ARB_ERROR   error;
    const char *ali_name = "ali_mini";
    GBDATA     *gb_main  = TEST_CREATE_DB(error, ali_name, TADinsdel, false);

    arb_suppress_progress noProgress;

    if (!error) {
        GB_transaction ta(gb_main);
        TEST_DB_INSERT_SAI(gb_main, error, ali_name, EXTinsdel);

        // add secondary structure to "HELIX"
        GBDATA *gb_helix     = GBT_find_SAI(gb_main, "HELIX");
        if (!gb_helix) error = GB_await_error();
        else {
            GBDATA *gb_struct     = GBT_add_data(gb_helix, ali_name, "_STRUCT", GB_STRING);
            if (!gb_struct) error = GB_await_error();
            else    error         = GB_write_string(gb_struct, HELIX_STRUCT);

            GBDATA *gb_struct_ref     = GBT_add_data(gb_helix, ali_name, "_REF", GB_STRING);
            if (!gb_struct_ref) error = GB_await_error();
            else    error             = GB_write_string(gb_struct_ref, HELIX_REF);
        }

        // add stored secondary structure
        GBDATA *gb_ref     = GB_search(gb_main, "secedit/structs/ali_mini/struct/ref", GB_STRING);
        if (!gb_ref) error = GB_await_error();
        else    error      = GB_write_string(gb_ref, HELIX_REF);

        // create one INTS and one FLOATS entry for first species
        GBDATA *gb_spec = GBT_find_species(gb_main, TADinsdel[0].name);
        {
            GBDATA     *gb_ints   = GBT_add_data(gb_spec, ali_name, "NN", GB_INTS);
            const char *intsAsStr = "934674096035485565009425682006116502001394358998513";
            size_t      len       = strlen(intsAsStr);
            GB_UINT4   *ints      = string2ints(intsAsStr, len);
            {
                char *asStr = ints2string(ints, len);
                TEST_EXPECT_EQUAL(intsAsStr, asStr);
                free(asStr);
            }
            error = GB_write_ints(gb_ints, ints, len);
            free(ints);
        }
        {
            GBDATA     *gb_ints     = GBT_add_data(gb_spec, ali_name, "FF", GB_FLOATS);
            const char *floatsAsStr = "ODu8EJh60e1XYLgxvzmeMiMAjB5EJxT6JPiCvQ4uCLDoHlWV59DW";
            size_t      len         = strlen(floatsAsStr);
            float      *floats      = string2floats(floatsAsStr, len);
            {
                char *asStr = floats2string(floats, len);
                TEST_EXPECT_EQUAL(floatsAsStr, asStr);
                free(asStr);
            }
            error = GB_write_floats(gb_ints, floats, len);
            free(floats);
        }
    }

    if (!error) {
        GB_transaction ta(gb_main);

        for (int pass = 1; pass <= 2; ++pass) {
            if (pass == 1) TEST_ALI_LEN_ALIGNED(52, 1);
            if (pass == 2) TEST_ALI_LEN_ALIGNED(53, 0); // was marked as "not aligned"

            TEST_DATA("...G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUCACCUCC.....",
                      "---A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUCACCUCCU.....",
                      "...A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUCACCUCCU----",
                      "---U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCCCACCUGA....",
                      ".....[<[.........[[..[<<.[..].>>]....]]....].>......]",
                      ".....1.1.........22..34..34.34..34...22....1........1",
                      ".....x..x........x.x.x....x.x....x...x.x...x.........x",
                      "934674096035485565009425682006116502001394358998513",  // a INTS entry
                      "ODu8EJh60e1XYLgxvzmeMiMAjB5EJxT6JPiCvQ4uCLDoHlWV59DW", // a FLOATS entry
                      HELIX_STRUCT);

            if (pass == 1) TEST_EXPECT_NO_ERROR(GBT_check_data(gb_main, ali_name));
        }

        TEST_EXPECT_NO_ERROR(GBT_format_alignment(gb_main, ali_name));
        TEST_ALI_LEN_ALIGNED(53, 1);
        TEST_DATA("...G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUCACCUCC......",
                  "---A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUCACCUCCU.....",
                  "...A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUCACCUCCU----.", // @@@ <- should convert '-' to '.' or append '-' 
                  "---U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCCCACCUGA.....",
                  ".....[<[.........[[..[<<.[..].>>]....]]....].>......]",
                  ".....1.1.........22..34..34.34..34...22....1........1",
                  ".....x..x........x.x.x....x.x....x...x.x...x.........x",
                  "93467409603548556500942568200611650200139435899851300",
                  "ODu8EJh60e1XYLgxvzmeMiMAjB5EJxT6JPiCvQ4uCLDoHlWV59DW!",
                  HELIX_STRUCT);

// editor column -> alignment column
#define COL(col) ((col)-19)

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(60), 2, "")); // insert in middle
        TEST_ALI_LEN_ALIGNED(55, 1);
        TEST_DATA("...G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC......",
                  "---A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.....",
                  "...A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU----.",
                  "---U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.....",
                  ".....[<[.........[[..[<<.[..].>>]....]]......].>......]",
                  ".....1.1.........22..34..34.34..34...22......1........1",
                  ".....x..x........x.x.x....x.x....x...x.x.....x.........x",
                  "9346740960354855650094256820061165020013900435899851300",
                  "ODu8EJh60e1XYLgxvzmeMiMAjB5EJxT6JPiCvQ4uC!!LDoHlWV59DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(71), 2, "")); // insert near end
        TEST_ALI_LEN_ALIGNED(57, 1);
        TEST_DATA("...G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "---A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  "...A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "---U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".....[<[.........[[..[<<.[..].>>]....]]......].>........]",
                  ".....1.1.........22..34..34.34..34...22......1..........1",
                  ".....x..x........x.x.x....x.x....x...x.x.....x...........x",
                  "934674096035485565009425682006116502001390043589985100300",
                  "ODu8EJh60e1XYLgxvzmeMiMAjB5EJxT6JPiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(20), 2, "")); // insert near start
        TEST_ALI_LEN_ALIGNED(59, 1);
        TEST_DATA(".....G-GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "-----A-CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A-CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U-GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".......[<[.........[[..[<<.[..].>>]....]]......].>........]",
                  ".......1.1.........22..34..34.34..34...22......1..........1",
                  ".......x..x........x.x.x....x.x....x...x.x.....x...........x",
                  "90034674096035485565009425682006116502001390043589985100300",
                  "O!!Du8EJh60e1XYLgxvzmeMiMAjB5EJxT6JPiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);


        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(26), 2, "")); // insert at left helix start
        TEST_ALI_LEN_ALIGNED(61, 1);
        TEST_DATA(".....G---GGC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "-----A---CGA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---CGA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---GCC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".........[<[.........[[..[<<.[..].>>]....]]......].>........]",
                  ".........1.1.........22..34..34.34..34...22......1..........1",
                  ".........x..x........x.x.x....x.x....x...x.x.....x...........x",
                  "9003467004096035485565009425682006116502001390043589985100300",
                  "O!!Du8E!!Jh60e1XYLgxvzmeMiMAjB5EJxT6JPiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(29), 2, "")); // insert behind left helix start
        TEST_ALI_LEN_ALIGNED(63, 1);
        TEST_DATA(".....G---G--GC-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "-----A---C--GA-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---C--GA-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---G--CC-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".........[--<[.........[[..[<<.[..].>>]....]]......].>........]", // @@@ <- should insert dots
                  ".........1...1.........22..34..34.34..34...22......1..........1",
                  ".........x....x........x.x.x....x.x....x...x.x.....x...........x",
                  "900346700400096035485565009425682006116502001390043589985100300",
                  "O!!Du8E!!J!!h60e1XYLgxvzmeMiMAjB5EJxT6JPiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(32), 2, "")); // insert at left helix end
        TEST_ALI_LEN_ALIGNED(65, 1);
        TEST_DATA(".....G---G--G--C-C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "-----A---C--G--A-U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A-A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C-U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".........[--<--[.........[[..[<<.[..].>>]....]]......].>........]", // @@@ <- should insert dots
                  ".........1.....1.........22..34..34.34..34...22......1..........1",
                  ".........x......x........x.x.x....x.x....x...x.x.....x...........x",
                  "90034670040000096035485565009425682006116502001390043589985100300",
                  "O!!Du8E!!J!!h!!60e1XYLgxvzmeMiMAjB5EJxT6JPiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(35), 2, ""));    // insert behind left helix end
        TEST_ALI_LEN_ALIGNED(67, 1);
        TEST_DATA(".....G---G--G--C---C-G...--AGGGGAA-CCUG-CGGC-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----CGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....GGGGGAA-CCUG-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-----GCGGCCU-UAGC-GCGG-UGGUCC--CACCUGA.......",
                  ".........[--<--[...........[[..[<<.[..].>>]....]]......].>........]",
                  ".........1.....1...........22..34..34.34..34...22......1..........1",
                  ".........x........x........x.x.x....x.x....x...x.x.....x...........x", // @@@ _REF gets destroyed here! (see #159)
                  "9003467004000009006035485565009425682006116502001390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLgxvzmeMiMAjB5EJxT6JPiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);


        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(57), 2, "")); // insert at right helix start
        TEST_ALI_LEN_ALIGNED(69, 1);
        TEST_DATA(".....G---G--G--C---C-G...--AGGGGAA-CCU--G-CGGC-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----CGGGGAA-CCU--G-CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....GGGGGAA-CCU--G-CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-----GCGGCCU-UAG--C-GCGG-UGGUCC--CACCUGA.......",
                  ".........[--<--[...........[[..[<<.[....].>>]....]]......].>........]",
                  ".........1.....1...........22..34..34...34..34...22......1..........1",
                  ".........x........x........x.x.x....x...x....x...x.x.....x...........x",
                  "900346700400000900603548556500942568200006116502001390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLgxvzmeMiMAjB5E!!JxT6JPiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(60), 2, ""));       // insert behind right helix start
        TEST_ALI_LEN_ALIGNED(71, 1);
        TEST_DATA(".....G---G--G--C---C-G...--AGGGGAA-CCU--G---CGGC-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----CGGGGAA-CCU--G---CGGC-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....GGGGGAA-CCU--G---CGGC-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-----GCGGCCU-UAG--C---GCGG-UGGUCC--CACCUGA.......",
                  ".........[--<--[...........[[..[<<.[....]...>>]....]]......].>........]",
                  ".........1.....1...........22..34..34...3--4..34...22......1..........1", // @@@ <- helix nr destroyed + shall use dots
                  ".........x........x........x.x.x....x...x......x...x.x.....x...........x",
                  "90034670040000090060354855650094256820000006116502001390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLgxvzmeMiMAjB5E!!J!!xT6JPiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(65), 2, ""));         // insert at right helix end
        TEST_ALI_LEN_ALIGNED(73, 1);
        TEST_DATA(".....G---G--G--C---C-G...--AGGGGAA-CCU--G---CG--GC-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----CGGGGAA-CCU--G---CG--GC-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....GGGGGAA-CCU--G---CG--GC-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-----GCGGCCU-UAG--C---GC--GG-UGGUCC--CACCUGA.......",
                  ".........[--<--[...........[[..[<<.[....]...>>--]....]]......].>........]", // @@@ <- shall insert dots
                  ".........1.....1...........22..34..34...3--4....34...22......1..........1",
                  ".........x........x........x.x.x....x...x........x...x.x.....x...........x",
                  "9003467004000009006035485565009425682000000611006502001390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLgxvzmeMiMAjB5E!!J!!xT6!!JPiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(68), 2, ""));           // insert behind right helix end
        TEST_ALI_LEN_ALIGNED(75, 1);
        TEST_DATA(".....G---G--G--C---C-G...--AGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----CGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....GGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-----GCGGCCU-UAG--C---GC--G--G-UGGUCC--CACCUGA.......",
                  ".........[--<--[...........[[..[<<.[....]...>>--]......]]......].>........]",
                  ".........1.....1...........22..34..34...3--4....3--4...22......1..........1", // @@@ <- helix nr destroyed + shall use dots
                  ".........x........x........x.x.x....x...x..........x...x.x.....x...........x", // @@@ _REF gets destroyed here! (see #159)
                  "900346700400000900603548556500942568200000061100600502001390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLgxvzmeMiMAjB5E!!J!!xT6!!J!!PiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);



        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(44), 2, ""));           // insert at gap border
        TEST_ALI_LEN_ALIGNED(77, 1);
        TEST_DATA(".....G---G--G--C---C-G.....--AGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-------CGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.......GGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-------GCGGCCU-UAG--C---GC--G--G-UGGUCC--CACCUGA.......",
                  ".........[--<--[.............[[..[<<.[....]...>>--]......]]......].>........]",
                  ".........1.....1.............22..34..34...3--4....3--4...22......1..........1",
                  ".........x........x..........x.x.x....x...x..........x...x.x.....x...........x",
                  "90034670040000090060354850056500942568200000061100600502001390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLg!!xvzmeMiMAjB5E!!J!!xT6!!J!!PiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);


        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(42), -6, "-.")); // delete gaps
        TEST_ALI_LEN_ALIGNED(71, 1);
        TEST_DATA(".....G---G--G--C---C-G.AGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCC........",
                  "-----A---C--G--A---U-C-CGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.GGGGGAA-CCU--G---CG--G--C-UGGAUC--ACCUCCU------.",
                  "-----U---G--C--C---U-G-GCGGCCU-UAG--C---GC--G--G-UGGUCC--CACCUGA.......",
                  ".........[--<--[.......[[..[<<.[....]...>>--]......]]......].>........]",
                  ".........1.....1.......22..34..34...3--4....3--4...22......1..........1",
                  ".........x........x....x.x.x....x...x..........x...x.x.....x...........x",
                  "90034670040000090060354500942568200000061100600502001390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYzmeMiMAjB5E!!J!!xT6!!J!!PiCvQ4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);


        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(71), -4, "%")); // delete anything
        TEST_ALI_LEN_ALIGNED(67, 1);
        TEST_DATA(".....G---G--G--C---C-G.AGGGGAA-CCU--G---CG--G--C-UGG-ACCUCC........",
                  "-----A---C--G--A---U-C-CGGGGAA-CCU--G---CG--G--C-UGG-ACCUCCU.......",
                  ".....A---C--G--A---A-C.GGGGGAA-CCU--G---CG--G--C-UGG-ACCUCCU------.",
                  "-----U---G--C--C---U-G-GCGGCCU-UAG--C---GC--G--G-UGG-CACCUGA.......",
                  ".........[--<--[.......[[..[<<.[....]...>>--]......]...].>........]",
                  ".........1.....1.......22..34..34...3--4....3--4...2...1..........1",
                  ".........x........x....x.x.x....x...x..........x...x...x...........x",
                  "9003467004000009006035450094256820000006110060050200043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYzmeMiMAjB5E!!J!!xT6!!J!!PiCvQ!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

    }

    if (!error) {
        {
            GB_transaction ta(gb_main);
            TEST_EXPECT_EQUAL(GBT_insert_character(gb_main, ali_name, COL(35), -3, "-."), // illegal delete
                              "SAI 'HELIX': You tried to delete 'x' at position 18  -> Operation aborted");
            ta.close("xxx");
        }
        {
            GB_transaction ta(gb_main);
            TEST_EXPECT_EQUAL(GBT_insert_character(gb_main, ali_name, COL(56), -3, "-."), // illegal delete
                              "SAI 'HELIX_NR': You tried to delete '4' at position 39  -> Operation aborted");
            ta.close("xxx");
        }
        {
            GB_transaction ta(gb_main);
            TEST_EXPECT_EQUAL(GBT_insert_character(gb_main, ali_name, 4711, 3, "-."), // illegal insert
                              "Can't insert at position 4711 (exceeds length 67 of alignment 'ali_mini')");
            ta.close("xxx");
        }
        {
            GB_transaction ta(gb_main);
            TEST_EXPECT_EQUAL(GBT_insert_character(gb_main, ali_name, -1, 3, "-."), // illegal insert
                              "Illegal sequence position -1");
            ta.close("xxx");
        }
    }
    if (!error) {
        GB_transaction ta(gb_main);
        TEST_DATA(".....G---G--G--C---C-G.AGGGGAA-CCU--G---CG--G--C-UGG-ACCUCC........",
                  "-----A---C--G--A---U-C-CGGGGAA-CCU--G---CG--G--C-UGG-ACCUCCU.......",
                  ".....A---C--G--A---A-C.GGGGGAA-CCU--G---CG--G--C-UGG-ACCUCCU------.",
                  "-----U---G--C--C---U-G-GCGGCCU-UAG--C---GC--G--G-UGG-CACCUGA.......",
                  ".........[--<--[.......[[..[<<.[....]...>>--]......]...].>........]",
                  ".........1.....1.......22..34..34...3--4....3--4...2...1..........1",
                  ".........x........x....x.x.x....x...x..........x...x...x...........x",
                  "9003467004000009006035450094256820000006110060050200043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYzmeMiMAjB5E!!J!!xT6!!J!!PiCvQ!LDoHlWV59!!DW!",
                  HELIX_STRUCT);
    }

    GB_close(gb_main);
    TEST_EXPECT_NO_ERROR(error.deliver());
}

#endif // UNIT_TESTS
