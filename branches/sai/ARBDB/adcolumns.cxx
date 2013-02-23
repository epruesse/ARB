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
#include <downcast.h>

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

template <typename T>
inline int compare_type(const T& t1, const T& t2) {
    return t1<t2 ? -1 : (t1>t2 ? 1 : 0);
}


// --------------------------------------------------------------------------------

class AliData;
typedef SmartPtr<AliData> AliDataPtr;

// --------------------------------------------------------------------------------

class AliData {
    size_t size;
public:
    AliData(size_t size_) : size(size_) {}
    virtual ~AliData() {}

    virtual size_t unitsize() const = 0;

    enum memop { COPY_TO, COMPARE_WITH };
    virtual int operate_on_mem(void *mem, size_t start, size_t count, memop op) const           = 0;
    virtual int cmp_data(size_t start, const AliData& other, size_t ostart, size_t count) const = 0;

    void copyPartTo(void *mem, size_t start, size_t count) const { operate_on_mem(mem, start, count, COPY_TO); }
    int cmpPartWith(const void *mem, size_t start, size_t count) const {
        gb_assert(is_valid_part(start, count));
        return operate_on_mem(const_cast<void*>(mem), start, count, COMPARE_WITH); // COMPARE_WITH does not modify
    }

    virtual UnitPtr unit_left_of(size_t pos) const  = 0;
    virtual UnitPtr unit_right_of(size_t pos) const = 0;

    virtual AliDataPtr create_gap(size_t gapsize, const UnitPair& gapinfo) const = 0;

    size_t elems() const { return size; }
    size_t memsize() const { return unitsize()*elems(); }
    void copyTo(void *mem) const { copyPartTo(mem, 0, elems()); }
    bool empty() const { return !elems(); }

    int cmp_whole_data(const AliData& other) const {
        int cmp = cmp_data(0, other, 0, std::min(elems(), other.elems()));
        if (cmp == 0) { // prefixes are equal
            return compare_type(elems(), other.elems());
        }
        return cmp;
    }

    bool equals(const AliData& other) const {
        if (&other == this) return true;
        if (elems() != other.elems()) return false;

        return cmp_whole_data(other) == 0;
    }
    bool differs_from(const AliData& other) const { return !equals(other); }

    bool is_valid_pos(size_t pos) const { return pos < elems(); }
    bool is_valid_between(size_t pos) const { return pos <= elems(); } // pos == 0 -> before first base; pos == elems() -> after last base

    bool is_valid_part(size_t start, size_t count) const {
        return is_valid_between(start) && is_valid_between(start+count);
    }
};

// --------------------------------------------------------------------------------

class ComposedAliData : public AliData {
    AliDataPtr left, right;

    ComposedAliData(AliDataPtr l, AliDataPtr r) : AliData(l->elems()+r->elems()), left(l), right(r) {
        gb_assert(l->unitsize() == r->unitsize());
        gb_assert(l->elems());
        gb_assert(r->elems());
    }
    friend AliDataPtr concat(AliDataPtr left, AliDataPtr right); // for above ctor

    void *inc_by_units(void *mem, size_t units) const { return reinterpret_cast<char*>(mem)+units*unitsize(); }

public:
    size_t unitsize() const OVERRIDE { return left->unitsize(); }
    AliDataPtr create_gap(size_t gapsize, const UnitPair& gapinfo) const OVERRIDE {
        return left->create_gap(gapsize, gapinfo);
    }
    int operate_on_mem(void *mem, size_t start, size_t count, memop op) const OVERRIDE {
        size_t left_elems = left->elems();
        size_t take_left  = 0;
        int    res        = 0;
        if (start<left_elems) {
            take_left = min(count, left_elems-start);
            res       = left->operate_on_mem(mem, start, take_left, op);
        }

        if (res == 0) {
            size_t take_right = count-take_left;
            if (take_right) {
                size_t rstart = start>left_elems ? start-left_elems : 0;
                gb_assert(right->is_valid_part(rstart, take_right));
                res = right->operate_on_mem(inc_by_units(mem, take_left), rstart, take_right, op);
            }
        }
        return res;
    }
    int cmp_data(size_t start, const AliData& other, size_t ostart, size_t count) const OVERRIDE {
        size_t left_elems = left->elems();
        size_t take_left  = 0;
        int    cmp        = 0;
        if (start<left_elems) {
            take_left = min(count, left_elems-start);
            cmp       = left->cmp_data(start, other, ostart, take_left);
        }

        if (cmp == 0) {
            size_t take_right = count-take_left;
            if (take_right) {
                size_t rstart  = start>left_elems ? start-left_elems : 0;
                size_t rostart = ostart+take_left;

                gb_assert(is_valid_part(rstart, take_right));
                gb_assert(other.is_valid_part(rostart, take_right));

                cmp = right->cmp_data(rstart, other, rostart, take_right);
            }
        }
        return cmp;
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
    int operate_on_mem(void *mem, size_t start, size_t count, memop op) const OVERRIDE {
        gb_assert(is_valid_part(start, count));
        return from->operate_on_mem(mem, start+offset, count, op);
    }
    UnitPtr unit_left_of(size_t pos) const OVERRIDE {
        gb_assert(is_valid_between(pos));
        return from->unit_left_of(pos+offset);
    }
    UnitPtr unit_right_of(size_t pos) const OVERRIDE {
        gb_assert(is_valid_between(pos));
        return from->unit_right_of(pos+offset);
    }
    int cmp_data(size_t start, const AliData& other, size_t ostart, size_t count) const OVERRIDE {
        gb_assert(is_valid_part(start, count));
        gb_assert(other.is_valid_part(ostart, count));

        return from->cmp_data(start+offset, other, ostart, count);
    }
};

template<typename T>
class TypedAliData : public AliData {
    T gap;

protected:
    static const T *typed_ptr(const UnitPtr& uptr) { return (const T*)uptr.get_pointer(); }

    const T* std_gap_ptr() const { return &gap; }

public:
    TypedAliData(size_t size_, T gap_)
        : AliData(size_),
          gap(gap_)
    {}

    const T& std_gap() const { return gap; }
    virtual const T& tail_gap() const = 0; // @@@ use for refactoring only

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
    const T& tail_gap() const OVERRIDE { gb_assert(0); static T fake = 0; return fake; }
public:
    typedef TypedAliData<T> BaseType;

    SpecificGap(size_t gapsize, const T& gap_)
        : BaseType(gapsize, gap_)
    {}
    int operate_on_mem(void *mem, size_t IF_ASSERTION_USED(start), size_t count, AliData::memop op) const OVERRIDE {
        gb_assert(BaseType::is_valid_part(start, count));
        if (op == AliData::COPY_TO) {
            T *typedMem = (T*)mem;
            for (size_t a = 0; a<count; ++a) {
                typedMem[a] = BaseType::std_gap();
            }
        }
        else {
            gb_assert(op == AliData::COMPARE_WITH);
            const T *typedMem = (const T*)mem;
            for (size_t a = 0; a<count; ++a) {
                int cmp = compare_type(BaseType::std_gap(), typedMem[a]);
                if (cmp) return cmp;
            }
        }
        return 0;
    }
    int cmp_data(size_t start, const AliData& other, size_t ostart, size_t count) const OVERRIDE {
        const SpecificGap<T> *other_is_gap = dynamic_cast<const SpecificGap<T>*>(&other);
        if (other_is_gap) {
            return compare_type(BaseType::std_gap(), other_is_gap->std_gap());
        }
        return -other.cmp_data(ostart, *this, start, count);
    }

    UnitPtr at_ptr(size_t pos) const OVERRIDE {
        if (pos<BaseType::elems()) return UnitPtr(BaseType::std_gap_ptr());
        return UnitPtr();
    }
};

template <typename T>
AliDataPtr TypedAliData<T>::create_gap(size_t gapsize, const UnitPair& /*gapinfo*/) const {
    return new SpecificGap<T>(gapsize, std_gap());
}

class SizeAwarable {
    bool   allows_oversize;
    size_t org_ali_size;
public:
    SizeAwarable(bool allows_oversize_, size_t ali_size_)
        : allows_oversize(allows_oversize_),
          org_ali_size(ali_size_)
    {}

    size_t get_allowed_size(size_t term_size, size_t new_ali_size) const {
        size_t allowed_size = new_ali_size;
        if (allows_oversize && term_size>org_ali_size) {
            size_t oversize = term_size-org_ali_size;
            allowed_size    = new_ali_size+oversize;
        }
        return allowed_size;
    }
};
inline SizeAwarable dontAllowOversize(size_t ali_size) { return SizeAwarable(false, ali_size); }

template<typename T>
class SpecificAliData : public TypedAliData<T>, public SizeAwarable, virtual Noncopyable {
    const T *data;

public:
    typedef TypedAliData<T> BaseType;

    SpecificAliData(const T *static_data, size_t elements, const T& gap_, const SizeAwarable& sizeAware)
        : BaseType(elements, gap_),
          SizeAwarable(sizeAware),
          data(static_data)
    {}

    int operate_on_mem(void *mem, size_t start, size_t count, AliData::memop op) const OVERRIDE {
        if (count>0) {
            gb_assert(BaseType::is_valid_part(start, count));
            if (op == AliData::COPY_TO) {
                size_t msize = BaseType::unitsize()*count;
                gb_assert(msize>0);
                memcpy(mem, data+start, msize);
            }
            else {
                gb_assert(op == AliData::COMPARE_WITH);
                const T *typedMem = (const T*)mem;
                for (size_t a = 0; a<count; ++a) {
                    int cmp = compare_type(data[start+a], typedMem[a]);
                    if (cmp) return cmp;
                }
            }
        }
        return 0;
    }
    int cmp_data(size_t start, const AliData& other, size_t ostart, size_t count) const OVERRIDE {
        gb_assert(BaseType::is_valid_part(start, count));
        gb_assert(other.is_valid_part(ostart, count));

        // if (&other == this && start == ostart) return true; // @@@ why does this fail tests?
        return -other.cmpPartWith(data+start, ostart, count);
    }

    UnitPtr at_ptr(size_t pos) const OVERRIDE {
        if (pos<BaseType::elems()) return UnitPtr(&data[pos]);
        return UnitPtr();
    }

    // @@@ interface to refactor gbt_insert_delete
    const T *get_data() const { return data; }
    const T& tail_gap() const OVERRIDE { return TypedAliData<T>::std_gap(); }
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
    SequenceAliData(const char* static_data, size_t elements, char stdgap, char dotgap, const SizeAwarable& sizeAware)
        : SpecificAliData<char>(static_data, elements, stdgap, sizeAware),
          dot(dotgap)
    {}

    AliDataPtr create_gap(size_t gapsize, const UnitPair& gapinfo) const OVERRIDE {
        char use = preferred_gap(typed_ptr(gapinfo.left), typed_ptr(gapinfo.right));
        return new SpecificGap<char>(gapsize, use);
    }

    const char& tail_gap() const OVERRIDE { return dot; }
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

inline AliDataPtr format(AliDataPtr data, const size_t wanted_len) {
    size_t curr_len = data->elems();
    if (curr_len < wanted_len) {
        data = insert_gap(data, curr_len, wanted_len-curr_len);
    }
    else if (curr_len > wanted_len) {
        data = delete_from(data, wanted_len, curr_len-wanted_len);
    }
    gb_assert(data->elems() == wanted_len);
    return data;
}


template<typename T> AliDataPtr makeAliData(T*& allocated_data, size_t elems, const T& gap) {
    return new SpecificAliData<T>(allocated_data, elems, gap, dontAllowOversize(elems));
}
AliDataPtr makeAliSeqData(char*& allocated_data, size_t elems, char gap, char dot) {
    return new SequenceAliData(allocated_data, elems, gap, dot, dontAllowOversize(elems));
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

template <typename T>
inline T *makeCopy(AliDataPtr d) {
    TEST_EXPECT_EQUAL(d->unitsize(), sizeof(T));
    size_t  size = d->memsize();
    T      *copy = (T*)malloc(size);
    d->copyTo(copy);
    return copy;
}

template <typename T>
static arb_test::match_expectation compare_works(AliDataPtr d1, AliDataPtr d2, int expected_cmp) {
    int brute_force_compare = 0;
    {
        int minSize = std::min(d1->elems(), d2->elems());

        T *copy1 = makeCopy<T>(d1);
        T *copy2 = makeCopy<T>(d2);

        for (int i = 0; i < minSize && brute_force_compare == 0; ++i) { // compare inclusive terminal zero-element
            brute_force_compare = compare_type(copy1[i], copy2[i]);
        }

        if (brute_force_compare == 0) {
            brute_force_compare = compare_type(d1->elems(), d2->elems());
        }

        free(copy2);
        free(copy1);
    }

    int smart_forward_compare  = d1->cmp_whole_data(*d2);
    int smart_backward_compare = d2->cmp_whole_data(*d1);

    using namespace   arb_test;
    expectation_group expected;

    expected.add(that(brute_force_compare).is_equal_to(expected_cmp));
    expected.add(that(smart_forward_compare).is_equal_to(expected_cmp));
    expected.add(that(smart_backward_compare).is_equal_to(-expected_cmp));

    return all().ofgroup(expected);
}

#define TEST_COMPARE_WORKS(d1,d2,expected) TEST_EXPECTATION(compare_works<char>(d1,d2,expected))

#define TEST_COMPARE_WORKS_ALL_TYPES(tid,d1,d2,expected)                                \
    switch (tid) {                                                              \
        case 0: TEST_EXPECTATION(compare_works<char>(d1,d2,expected)); break;           \
        case 1: TEST_EXPECTATION(compare_works<GB_UINT4>(d1,d2,expected)); break;       \
        case 2: TEST_EXPECTATION(compare_works<float>(d1,d2,expected)); break;          \
    }

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
        TEST_EXPECT_COPIES_EQUAL(insert_at(del, 777, empty), del); // append via insert_at
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
            TEST_EXPECT_COPY_EQUALS_STRING(end_gap1,   "A-");      // '-' is ok, since before there was a C behind (but correct would be '.')
            TEST_EXPECT_COPY_EQUALS_STRING(del,        "CGCC-C-GG-C-GG.A.-C------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(del_rest,   "CGC");
            TEST_EXPECT_COPY_EQUALS_STRING(ins,        "CGCAC-C-GG-C-GG.A.-C------GG-.C..UCAGU");
            TEST_EXPECT_COPY_EQUALS_STRING(gap_iseq,   "-----");   // inserted between bases
            TEST_EXPECT_COPY_EQUALS_STRING(gap_iempty, ".....");   // inserted in empty sequence
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

            {
                // test comparability of AliData

                AliDataPtr same_as_start_gap1 = after(start_gap3, 1);

                TEST_COMPARE_WORKS(start_gap1, same_as_start_gap1, 0);

                TEST_EXPECT(start_gap1->differs_from(*start_gap3));
                TEST_EXPECT_EQUAL(strcmp(".CGCC------C-GG-C-GG.A.-C------GG-.C..UCAGU",        // start_gap1
                                         "...CGCC------C-GG-C-GG.A.-C------GG-.C..UCAGU"), 1); // start_gap3

                TEST_EXPECT_EQUAL(start_gap1->cmp_whole_data(*start_gap3),  1);
                TEST_EXPECT_EQUAL(start_gap3->cmp_whole_data(*start_gap1), -1);

                TEST_COMPARE_WORKS(end, end_gap2, -1);
            }
        }

        {
            // test comparability of AliData (for all types)

            TEST_COMPARE_WORKS_ALL_TYPES(t, start_gap1, start_gap3, 1);
            TEST_COMPARE_WORKS_ALL_TYPES(t, gap_iempty, gap_in_gap, -1);
            TEST_COMPARE_WORKS_ALL_TYPES(t, del, ins, 1);
            TEST_COMPARE_WORKS_ALL_TYPES(t, partof(ins_gap, 0, 17), partof(start_gap3, 3, 17), 0);
            TEST_COMPARE_WORKS_ALL_TYPES(t, start_gap3, start_gap3, 0);
        }
    }

    TEST_EXPECT_CODE_ASSERTION_FAILS(illegal_alidata_composition); // composing different unitsizes shall fail
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

enum TerminalType {
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
    virtual GB_ERROR apply_to_terminal(GBDATA *gb_data, TerminalType term_type, const Alignment& ali) const = 0;

    GB_ERROR apply_recursive(GBDATA *gb_data, TerminalType term_type, const Alignment& ali) const;
    GB_ERROR apply_to_childs_named(GBDATA *gb_item_data, const char *item_field, TerminalType term_type, const Alignment& ali) const;
    GB_ERROR apply_to_secstructs(GBDATA *gb_secstructs, const Alignment& ali) const;

public:
    AliApplicable() {}
    virtual ~AliApplicable() {}

    GB_ERROR apply_to_alignment(GBDATA *gb_main, const Alignment& ali) const;
};

GB_ERROR AliApplicable::apply_recursive(GBDATA *gb_data, TerminalType term_type, const Alignment& ali) const {
    GB_ERROR error = 0;
    GB_TYPES type  = GB_read_type(gb_data);

    if (type == GB_DB) {
        GBDATA *gb_child;
        for (gb_child = GB_child(gb_data); gb_child && !error; gb_child = GB_nextChild(gb_child)) {
            error = apply_recursive(gb_child, term_type, ali);
        }
    }
    else {
        error = apply_to_terminal(gb_data, term_type, ali);
    }

    return error;
}
GB_ERROR AliApplicable::apply_to_childs_named(GBDATA *gb_item_data, const char *item_field, TerminalType term_type, const Alignment& ali) const {
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
                error = apply_recursive(gb_ali, term_type, ali);
                if (error) {
                    const char *item_name = GBT_read_name(gb_item);
                    error = GBS_global_string("%s '%s': %s", targetTypeName[term_type], item_name, error);
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
    GB_ERROR apply_to_terminal(GBDATA *, TerminalType , const Alignment& ) const OVERRIDE { count++; return NULL; }
public:
    AliEntryCounter() : count(0) {}
    size_t get_entry_count() const { return count; }
};

// --------------------------------------------------------------------------------

class AliEditCommand {
public:
    virtual ~AliEditCommand() {}
    virtual AliDataPtr apply(AliDataPtr to) const = 0;
};

class AliInsertCommand : public AliEditCommand {
    size_t pos;
    size_t amount;
public:
    AliInsertCommand(size_t pos_, size_t amount_) : pos(pos_), amount(amount_) {}
    AliDataPtr apply(AliDataPtr to) const OVERRIDE { return insert_gap(to, pos, amount); }
};

class AliDeleteCommand : public AliEditCommand, virtual Noncopyable {
    size_t pos;
    size_t amount;

    const char *delete_chars; // characters allowed to delete (array with 256 entries, value == 0 means deletion allowed)
public:
    AliDeleteCommand(size_t pos_, size_t amount_, const char *delete_chars_)
        : pos(pos_),
          amount(amount_),
          delete_chars(delete_chars_)
    {}

    AliDataPtr apply(AliDataPtr to) const OVERRIDE { return delete_from(to, pos, amount); }
    bool allowed_to_delete(char c) const { return delete_chars[safeCharIndex(c)] == 1; }
    GB_ERROR allowed_to_delete_from(const SequenceAliData& sad) const {
        GB_ERROR    error  = NULL;
        size_t      size   = sad.elems();
        const char *source = sad.get_data();

        size_t after          = pos+amount; // position after deleted part
        if (after>size) after = size;

        for (size_t p = pos; p<after; p++) {
            if (allowed_to_delete(source[p])) {
                error = GBS_global_string("You tried to delete '%c' at position %li  -> Operation aborted", source[p], p);
            }
        }

        return error;
    }
};

class AliFormatCommand : public AliEditCommand {
    size_t wanted_len;

public:
    AliFormatCommand(size_t wanted_len_) : wanted_len(wanted_len_) {}
    AliDataPtr apply(AliDataPtr to) const OVERRIDE {
        SizeAwarable *knows_size = dynamic_cast<SizeAwarable*>(&*to);

        gb_assert(knows_size); // format can only be applied to SpecificAliData
                               // i.e. AliFormatCommand has to be the FIRST of a series of applied commands!

        int allowed_size = knows_size->get_allowed_size(to->elems(), wanted_len);
        return format(to, allowed_size);
    }
};

class AliCompositeCommand : public AliEditCommand, virtual Noncopyable {
    AliEditCommand *first;
    AliEditCommand *second;
public:
    AliCompositeCommand(AliEditCommand *cmd1_, AliEditCommand *cmd2_) // takes ownership of commands
        : first(cmd1_),
          second(cmd2_)
    {}
    ~AliCompositeCommand() OVERRIDE { delete second; delete first; }
    AliDataPtr apply(AliDataPtr to) const OVERRIDE { return second->apply(first->apply(to)); }
};

// --------------------------------------------------------------------------------

class AliEditor : public AliApplicable {
    const AliEditCommand& cmd;
    mutable arb_progress progress;

    GB_ERROR apply_to_terminal(GBDATA *gb_data, TerminalType term_type, const Alignment& ali) const OVERRIDE;

    bool shall_edit(GBDATA *gb_data, TerminalType term_type) const {
        // defines whether specific DB-elements shall be edited by any AliEditor
        // (true for all data, that contains alignment position specific data)

        const char *key   = GB_read_key_pntr(gb_data);
        bool        shall = key[0] != '_';                                    // general case: don't apply to keys starting with '_'
        if (!shall) shall = term_type == IDT_SAI && strcmp(key, "_REF") == 0; // exception (SAI:_REF needs editing)
        return shall;
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

// --------------------------------------------------------------------------------

static char   *insDelBuffer = 0;
static size_t  insDelBuffer_size;

inline void free_insDelBuffer() {
    freenull(insDelBuffer);
}
inline char *provide_insDelBuffer(size_t neededSpace) {
    if (insDelBuffer && insDelBuffer_size<neededSpace) free_insDelBuffer();
    if (!insDelBuffer) {
        insDelBuffer_size = neededSpace+10;
        insDelBuffer      = (char*)malloc(insDelBuffer_size);
    }
    return insDelBuffer;
}

inline GB_CSTR alidata2buffer(const AliData& data) { // @@@ DRY vs copying code (above in this file)
    char *buffer = provide_insDelBuffer(data.memsize()+1);

    data.copyTo(buffer);
    buffer[data.memsize()] = 0; // only needed for strings but does not harm otherwise

    return buffer;
}

// --------------------------------------------------------------------------------

class EditedTerminal : virtual Noncopyable {
    GBDATA     *gb_data;
    GB_TYPES    type;
    AliDataPtr  data;
    GB_ERROR    error;

    void prepare(const AliEditCommand& cmd, const SequenceAliData& sad) { // @@@ rename
        if (!error && type == GB_STRING) {
            const AliDeleteCommand *del_cmd = dynamic_cast<const AliDeleteCommand*>(&cmd);
            if (del_cmd) error = del_cmd->allowed_to_delete_from(sad);
        }
    }

    bool has_key(const char *expected_key) {
        return strcmp(GB_read_key_pntr(gb_data), expected_key) == 0;
    }
    bool does_allow_oversize(TerminalType term_type) {
        bool oversize_allowed = false;
        if (type == GB_STRING) {
            switch (term_type) {
                case IDT_SPECIES:   break;
                case IDT_SECSTRUCT: oversize_allowed = has_key("ref"); break;
                case IDT_SAI:       oversize_allowed = has_key("_REF"); break;
            }
        }
        return oversize_allowed;
    }

public:
    EditedTerminal(GBDATA *gb_data_, GB_TYPES type_, size_t size_, TerminalType term_type, const Alignment& ali)
        : gb_data(gb_data_),
          type(type_),
          error(NULL)
    {
        SizeAwarable oversizable(does_allow_oversize(term_type), ali.get_len());

        // @@@ DRY cases
        switch(type) {
            case GB_STRING: {
                const char *s    = GB_read_char_pntr(gb_data);
                if (!s) error    = GB_await_error();
                else        data = new SequenceAliData(s, size_, '-', '.', oversizable); // @@@ need different gaptype for "ref" and "_REF"
                break;
            }
            case GB_BITS:   {
                const char *b    = GB_read_bits_pntr(gb_data, '-', '+');
                if (!b) error    = GB_await_error();
                else        data = new SpecificAliData<char>(b, size_, '-', oversizable);
                break;
            }
            case GB_BYTES:  {
                const char *b    = GB_read_bytes_pntr(gb_data);
                if (!b) error    = GB_await_error();
                else        data = new SpecificAliData<char>(b, size_, 0, oversizable);
                break;
            }
            case GB_INTS:   {
                const GB_UINT4 *ui   = GB_read_ints_pntr(gb_data);
                if (!ui) error       = GB_await_error();
                else            data = new SpecificAliData<GB_UINT4>(ui, size_, 0, oversizable);
                break;
            }
            case GB_FLOATS: {
                const float *f    = GB_read_floats_pntr(gb_data);
                if (!f) error     = GB_await_error();
                else         data = new SpecificAliData<float>(f, size_, 0.0, oversizable);
                break;
            }

            default:
                error = GBS_global_string("Unhandled type '%i'", type);
                gb_assert(0);
                break;
        }

        gb_assert(implicated(!error, size_ == data->elems()));
    }

    GB_ERROR apply(const AliEditCommand& cmd) {
        if (!error && type == GB_STRING) {
            const SequenceAliData* sad = dynamic_cast<const SequenceAliData*>(&*data);
            prepare(cmd, *sad);
        }
        if (!error) {
            AliDataPtr modified_data = cmd.apply(data);

            if (modified_data->differs_from(*data)) {
                GB_CSTR modified       = alidata2buffer(*modified_data);
                size_t  modified_elems = modified_data->elems();

                switch (type) {
                    case GB_STRING: {
                        gb_assert(strlen(modified) == modified_elems);
                        error = GB_write_string(gb_data, modified);
                        break;
                    }
                    case GB_BITS:   error = GB_write_bits  (gb_data, modified, modified_elems, "-");       break;
                    case GB_BYTES:  error = GB_write_bytes (gb_data, modified, modified_elems);            break;
                    case GB_INTS:   error = GB_write_ints  (gb_data, (GB_UINT4*)modified, modified_elems); break;
                    case GB_FLOATS: error = GB_write_floats(gb_data, (float*)modified, modified_elems);    break;

                    default: gb_assert(0); break;
                }
            }
        }
        return error;
    }
};

GB_ERROR AliEditor::apply_to_terminal(GBDATA *gb_data, TerminalType term_type, const Alignment& ali) const {
    GB_TYPES gbtype  = GB_read_type(gb_data);
    GB_ERROR error = NULL;
    if (gbtype >= GB_BITS && gbtype != GB_LINK) {
        if (shall_edit(gb_data, term_type)) {
            EditedTerminal edited(gb_data, gbtype, GB_read_count(gb_data), term_type, ali);
            error = edited.apply(edit_command());
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

static GB_ERROR format_to_alilen(GBDATA *Main, const char *alignment_name) {
    GB_ERROR  error      = 0;
    GBDATA   *gb_presets = GBT_get_presets(Main);
    GBDATA   *gb_ali;

    for (gb_ali = GB_entry(gb_presets, "alignment");
         gb_ali && !error;
         gb_ali = GB_nextEntry(gb_ali))
    {
        GBDATA *gb_name = GB_find_string(gb_ali, "alignment_name", alignment_name, GB_IGNORE_CASE, SEARCH_CHILD);

        if (gb_name) {
            GBDATA    *gb_len = GB_entry(gb_ali, "alignment_len");
            Alignment  ali(GB_read_char_pntr(gb_name), GB_read_int(gb_len));

            AliFormatCommand fcmd(ali.get_len()); // format to max. existing length

            error = AliEditor(fcmd, "Formatting alignment", countAffectedEntries(Main, ali))
                .apply_to_alignment(Main, ali);
        }
    }
    free_insDelBuffer();
    return error;
}

GB_ERROR GBT_format_alignment(GBDATA *Main, const char *alignment_name) {
    GB_ERROR err = 0;

    if (strcmp(alignment_name, GENOM_ALIGNMENT) != 0) {         // NEVER EVER format 'ali_genom'
        err           = GBT_check_data(Main, alignment_name);   // detect max. length
        if (!err) err = format_to_alilen(Main, alignment_name); // format sequences in alignment
        if (!err) err = GBT_check_data(Main, alignment_name);   // sets state to "formatted"
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
                    Alignment ali(use, len);

                    SmartPtr<AliEditCommand> idcmd;
                    if (count<0) idcmd = new AliDeleteCommand(pos, -count, char_delete_list);
                    else         idcmd = new AliInsertCommand(pos, count);

                    error = AliEditor(*idcmd, "Insert/delete characters", countAffectedEntries(Main, ali))
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

#define APPLY_CMD(str,cmd)                                                                              \
    size_t     str_len = strlen(str);                                                                   \
    AliDataPtr data    = new SequenceAliData(str, str_len, '-', '.', dontAllowOversize(str_len));       \
    AliDataPtr mod     = cmd.apply(data);                                                               \
    GB_CSTR    res     = mod->differs_from(*data) ? alidata2buffer(*mod) : NULL

#define DO_FORMAT(str,wanted_len)               \
    AliFormatCommand     cmd(wanted_len);       \
    APPLY_CMD(str, cmd)

#define DO_INSERT(str,pos,amount)               \
    AliInsertCommand     cmd(pos, amount);      \
    APPLY_CMD(str, cmd)

#define DO_FORMAT_AND_INSERT(str,wanted_len,pos,amount)         \
    AliCompositeCommand  cmd(new AliFormatCommand(wanted_len),  \
                             new AliInsertCommand(pos,amount)); \
    APPLY_CMD(str, cmd)

#define DO_DELETE(str,pos,amount)               \
    AliDeleteCommand     cmd(pos, amount, "-"); \
    APPLY_CMD(str, cmd)

#define TEST_FORMAT(str,wanted_alilen,expected)         do { DO_FORMAT(str,wanted_alilen); TEST_EXPECT_EQUAL(res, expected);         } while(0)
#define TEST_FORMAT__BROKEN(str,wanted_alilen,expected) do { DO_FORMAT(str,wanted_alilen); TEST_EXPECT_EQUAL__BROKEN(res, expected); } while(0)

#define TEST_INSERT(str,pos,amount,expected)         do { DO_INSERT(str,pos,amount); TEST_EXPECT_EQUAL(res, expected);         } while(0)
#define TEST_INSERT__BROKEN(str,pos,amount,expected) do { DO_INSERT(str,pos,amount); TEST_EXPECT_EQUAL__BROKEN(res, expected); } while(0)

#define TEST_DELETE(str,pos,amount,expected)         do { DO_DELETE(str,pos,amount); TEST_EXPECT_EQUAL(res, expected);         } while(0)
#define TEST_DELETE__BROKEN(str,pos,amount,expected) do { DO_DELETE(str,pos,amount); TEST_EXPECT_EQUAL__BROKEN(res, expected); } while(0)

#define TEST_FORMAT_AND_INSERT(str,wanted_alilen,pos,amount,expected)         do { DO_FORMAT_AND_INSERT(str,wanted_alilen,pos,amount); TEST_EXPECT_EQUAL(res, expected);         } while(0)
#define TEST_FORMAT_AND_INSERT__BROKEN(str,wanted_alilen,pos,amount,expected) do { DO_FORMAT_AND_INSERT(str,wanted_alilen,pos,amount); TEST_EXPECT_EQUAL__BROKEN(res, expected); } while(0)

// --------------------------------------------------------------------------------

void TEST_format_insert_delete() {
    // this test is a bit weird.
    //
    // originally it was used to test the function gbt_insert_delete, which is gone now.
    // now it tests AliFormatCommand, AliInsertCommand, AliDeleteCommand and AliCompositeCommand (but quite implicit).

    TEST_FORMAT("xxx",   5, "xxx..");
    TEST_FORMAT(".x.",   5, ".x...");
    TEST_FORMAT(".x..",  5, ".x...");
    TEST_FORMAT(".x...", 5, NULL); // NULL means "result == source"

    // wanted ----------------------------- current
    TEST_FORMAT("xxx--", 3, "xxx");
    TEST_FORMAT("xxx..", 3, "xxx");
    TEST_FORMAT("xxxxx", 3, "xxx");
    TEST_FORMAT("xxx",   0, "");

    // insert/delete in the middle
    TEST_INSERT("abcde", 3, 0, NULL);
    TEST_INSERT("abcde", 3, 1, "abc-de");
    TEST_INSERT("abcde", 3, 2, "abc--de");

    TEST_DELETE("abcde",   3, 0, NULL);
    TEST_DELETE("abc-de",  3, 1, "abcde");
    TEST_DELETE("abc--de", 3, 2, "abcde");

    // insert/delete at end
    TEST_INSERT("abcde", 5, 1, "abcde.");
    TEST_INSERT("abcde", 5, 4, "abcde....");

    TEST_DELETE("abcde-",    5, 1, "abcde");
    TEST_DELETE("abcde----", 5, 4, "abcde");

    // insert/delete at start
    TEST_INSERT("abcde", 0, 1, ".abcde");
    TEST_INSERT("abcde", 0, 4, "....abcde");

    TEST_DELETE("-abcde",    0, 1, "abcde");
    TEST_DELETE("----abcde", 0, 4, "abcde");

    // insert behind end
    TEST_FORMAT_AND_INSERT("abcde", 10, 8, 1, "abcde......");
    TEST_FORMAT_AND_INSERT("abcde", 10, 8, 4, "abcde.........");

    // insert/delete all
    TEST_INSERT("",    0, 3, "...");
    TEST_DELETE("---", 0, 3, "");

    free_insDelBuffer();
}

// ------------------------------

struct arb_unit_test::test_alignment_data TADinsdel[] = {
    { 1, "MtnK1722", "...G-GGC-C-G...--A--G--GAA-CCUG-CGGC-UGG--AUCACCUCC....." },
    { 1, "MhnFormi", "---A-CGA-U-C-----C--G--GAA-CCUG-CGGC-UGG--AUCACCUCCU....." },
    { 1, "MhnT1916", "...A-CGA-A-C.....G--G--GAA-CCUG-CGGC-UGG--AUCACCUCCU----" },
};

struct arb_unit_test::test_alignment_data EXTinsdel[] = {
    { 0, "ECOLI",    "---U-GCC-U-G-----G--C--CCU-UAGC-GCGG-UGG--UCCCACCUGA...." },
    { 0, "HELIX",    ".....[<[.........[..[..[<<.[..].>>]....]..]....].>......]" },
    { 0, "HELIX_NR", ".....1.1.........25.25.34..34.34..34...25.25...1........1" },
};

#define HELIX_REF    ".....x..x........x...x.x....x.x....x...x...x...x.........x"
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
            const char *intsAsStr = "9346740960354855652100942568200611650200211394358998513";
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
            const char *floatsAsStr = "ODu8EJh60e1XYLgxvzrqmeMiMAjB5EJxT6JPiCvQrq4uCLDoHlWV59DW";
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
            if (pass == 1) TEST_ALI_LEN_ALIGNED(56, 1);
            if (pass == 2) TEST_ALI_LEN_ALIGNED(57, 0); // was marked as "not aligned"

            TEST_DATA("...G-GGC-C-G...--A--G--GAA-CCUG-CGGC-UGG--AUCACCUCC.....",
                      "---A-CGA-U-C-----C--G--GAA-CCUG-CGGC-UGG--AUCACCUCCU.....",
                      "...A-CGA-A-C.....G--G--GAA-CCUG-CGGC-UGG--AUCACCUCCU----",
                      "---U-GCC-U-G-----G--C--CCU-UAGC-GCGG-UGG--UCCCACCUGA....",
                      ".....[<[.........[..[..[<<.[..].>>]....]..]....].>......]",
                      ".....1.1.........25.25.34..34.34..34...25.25...1........1",
                      ".....x..x........x...x.x....x.x....x...x...x...x.........x",
                      "9346740960354855652100942568200611650200211394358998513",  // a INTS entry
                      "ODu8EJh60e1XYLgxvzrqmeMiMAjB5EJxT6JPiCvQrq4uCLDoHlWV59DW", // a FLOATS entry
                      HELIX_STRUCT);

            if (pass == 1) TEST_EXPECT_NO_ERROR(GBT_check_data(gb_main, ali_name));
        }

        TEST_EXPECT_NO_ERROR(GBT_format_alignment(gb_main, ali_name));
        TEST_ALI_LEN_ALIGNED(57, 1);
        TEST_DATA("...G-GGC-C-G...--A--G--GAA-CCUG-CGGC-UGG--AUCACCUCC......",
                  "---A-CGA-U-C-----C--G--GAA-CCUG-CGGC-UGG--AUCACCUCCU.....",
                  "...A-CGA-A-C.....G--G--GAA-CCUG-CGGC-UGG--AUCACCUCCU-----", // @@@ <- should convert '-' to '.'
                  "---U-GCC-U-G-----G--C--CCU-UAGC-GCGG-UGG--UCCCACCUGA.....",
                  ".....[<[.........[..[..[<<.[..].>>]....]..]....].>......]",
                  ".....1.1.........25.25.34..34.34..34...25.25...1........1",
                  ".....x..x........x...x.x....x.x....x...x...x...x.........x",
                  "934674096035485565210094256820061165020021139435899851300",
                  "ODu8EJh60e1XYLgxvzrqmeMiMAjB5EJxT6JPiCvQrq4uCLDoHlWV59DW!",
                  HELIX_STRUCT);

// text-editor column -> alignment column
#define COL(col) ((col)-19)

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(64), 2, "")); // insert in middle
        TEST_ALI_LEN_ALIGNED(59, 1);
        TEST_DATA("...G-GGC-C-G...--A--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCC......",
                  "---A-CGA-U-C-----C--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU.....",
                  "...A-CGA-A-C.....G--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU-----",
                  "---U-GCC-U-G-----G--C--CCU-UAGC-GCGG-UGG--UCC--CACCUGA.....",
                  ".....[<[.........[..[..[<<.[..].>>]....]..]......].>......]",
                  ".....1.1.........25.25.34..34.34..34...25.25.....1........1",
                  ".....x..x........x...x.x....x.x....x...x...x.....x.........x",
                  "93467409603548556521009425682006116502002113900435899851300",
                  "ODu8EJh60e1XYLgxvzrqmeMiMAjB5EJxT6JPiCvQrq4uC!!LDoHlWV59DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(75), 2, "")); // insert near end
        TEST_ALI_LEN_ALIGNED(61, 1);
        TEST_DATA("...G-GGC-C-G...--A--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCC........",
                  "---A-CGA-U-C-----C--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU.......",
                  "...A-CGA-A-C.....G--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU-------",
                  "---U-GCC-U-G-----G--C--CCU-UAGC-GCGG-UGG--UCC--CACCUGA.......",
                  ".....[<[.........[..[..[<<.[..].>>]....]..]......].>........]",
                  ".....1.1.........25.25.34..34.34..34...25.25.....1..........1",
                  ".....x..x........x...x.x....x.x....x...x...x.....x...........x",
                  "9346740960354855652100942568200611650200211390043589985100300",
                  "ODu8EJh60e1XYLgxvzrqmeMiMAjB5EJxT6JPiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(20), 2, "")); // insert near start
        TEST_ALI_LEN_ALIGNED(63, 1);
        TEST_DATA(".....G-GGC-C-G...--A--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCC........",
                  "-----A-CGA-U-C-----C--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU.......",
                  ".....A-CGA-A-C.....G--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU-------",
                  "-----U-GCC-U-G-----G--C--CCU-UAGC-GCGG-UGG--UCC--CACCUGA.......",
                  ".......[<[.........[..[..[<<.[..].>>]....]..]......].>........]",
                  ".......1.1.........25.25.34..34.34..34...25.25.....1..........1",
                  ".......x..x........x...x.x....x.x....x...x...x.....x...........x",
                  "900346740960354855652100942568200611650200211390043589985100300",
                  "O!!Du8EJh60e1XYLgxvzrqmeMiMAjB5EJxT6JPiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);


        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(26), 2, "")); // insert at left helix start
        TEST_ALI_LEN_ALIGNED(65, 1);
        TEST_DATA(".....G---GGC-C-G...--A--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCC........",
                  "-----A---CGA-U-C-----C--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU.......",
                  ".....A---CGA-A-C.....G--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU-------",
                  "-----U---GCC-U-G-----G--C--CCU-UAGC-GCGG-UGG--UCC--CACCUGA.......",
                  ".........[<[.........[..[..[<<.[..].>>]....]..]......].>........]",
                  ".........1.1.........25.25.34..34.34..34...25.25.....1..........1",
                  ".........x..x........x...x.x....x.x....x...x...x.....x...........x",
                  "90034670040960354855652100942568200611650200211390043589985100300",
                  "O!!Du8E!!Jh60e1XYLgxvzrqmeMiMAjB5EJxT6JPiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(29), 2, "")); // insert behind left helix start
        TEST_ALI_LEN_ALIGNED(67, 1);
        TEST_DATA(".....G---G--GC-C-G...--A--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCC........",
                  "-----A---C--GA-U-C-----C--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU.......",
                  ".....A---C--GA-A-C.....G--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU-------",
                  "-----U---G--CC-U-G-----G--C--CCU-UAGC-GCGG-UGG--UCC--CACCUGA.......",
                  ".........[--<[.........[..[..[<<.[..].>>]....]..]......].>........]", // @@@ <- should insert dots
                  ".........1...1.........25.25.34..34.34..34...25.25.....1..........1",
                  ".........x....x........x...x.x....x.x....x...x...x.....x...........x",
                  "9003467004000960354855652100942568200611650200211390043589985100300",
                  "O!!Du8E!!J!!h60e1XYLgxvzrqmeMiMAjB5EJxT6JPiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(32), 2, "")); // insert at left helix end
        TEST_ALI_LEN_ALIGNED(69, 1);
        TEST_DATA(".....G---G--G--C-C-G...--A--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCC........",
                  "-----A---C--G--A-U-C-----C--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU.......",
                  ".....A---C--G--A-A-C.....G--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU-------",
                  "-----U---G--C--C-U-G-----G--C--CCU-UAGC-GCGG-UGG--UCC--CACCUGA.......",
                  ".........[--<--[.........[..[..[<<.[..].>>]....]..]......].>........]", // @@@ <- should insert dots
                  ".........1.....1.........25.25.34..34.34..34...25.25.....1..........1",
                  ".........x......x........x...x.x....x.x....x...x...x.....x...........x",
                  "900346700400000960354855652100942568200611650200211390043589985100300",
                  "O!!Du8E!!J!!h!!60e1XYLgxvzrqmeMiMAjB5EJxT6JPiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(35), 2, ""));    // insert behind left helix end
        TEST_ALI_LEN_ALIGNED(71, 1);
        TEST_DATA(".....G---G--G--C---C-G...--A--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----C--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....G--G--GAA-CCUG-CGGC-UGG--AUC--ACCUCCU-------",
                  "-----U---G--C--C---U-G-----G--C--CCU-UAGC-GCGG-UGG--UCC--CACCUGA.......",
                  ".........[--<--[...........[..[..[<<.[..].>>]....]..]......].>........]",
                  ".........1.....1...........25.25.34..34.34..34...25.25.....1..........1",
                  ".........x........x........x...x.x....x.x....x...x...x.....x...........x", // @@@ _REF gets destroyed here! (see #159)
                  //               ^ ^
                  "90034670040000090060354855652100942568200611650200211390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLgxvzrqmeMiMAjB5EJxT6JPiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);


        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(59), 2, "")); // insert at right helix start
        TEST_ALI_LEN_ALIGNED(73, 1);
        TEST_DATA(".....G---G--G--C---C-G...--A--G--GAA-CCU--G-CGGC-UGG--AUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----C--G--GAA-CCU--G-CGGC-UGG--AUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....G--G--GAA-CCU--G-CGGC-UGG--AUC--ACCUCCU-------",
                  "-----U---G--C--C---U-G-----G--C--CCU-UAG--C-GCGG-UGG--UCC--CACCUGA.......",
                  ".........[--<--[...........[..[..[<<.[....].>>]....]..]......].>........]",
                  ".........1.....1...........25.25.34..34...34..34...25.25.....1..........1",
                  ".........x........x........x...x.x....x...x....x...x...x.....x...........x",
                  "9003467004000009006035485565210094256820000611650200211390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLgxvzrqmeMiMAjB5E!!JxT6JPiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(62), 2, ""));       // insert behind right helix start
        TEST_ALI_LEN_ALIGNED(75, 1);
        TEST_DATA(".....G---G--G--C---C-G...--A--G--GAA-CCU--G---CGGC-UGG--AUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----C--G--GAA-CCU--G---CGGC-UGG--AUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....G--G--GAA-CCU--G---CGGC-UGG--AUC--ACCUCCU-------",
                  "-----U---G--C--C---U-G-----G--C--CCU-UAG--C---GCGG-UGG--UCC--CACCUGA.......",
                  ".........[--<--[...........[..[..[<<.[....]...>>]....]..]......].>........]",
                  ".........1.....1...........25.25.34..34...3--4..34...25.25.....1..........1", // @@@ <- helix nr destroyed + shall use dots
                  //                                         ^^^^
                  ".........x........x........x...x.x....x...x......x...x...x.....x...........x",
                  "900346700400000900603548556521009425682000000611650200211390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLgxvzrqmeMiMAjB5E!!J!!xT6JPiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(67), 2, ""));         // insert at right helix end
        TEST_ALI_LEN_ALIGNED(77, 1);
        TEST_DATA(".....G---G--G--C---C-G...--A--G--GAA-CCU--G---CG--GC-UGG--AUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----C--G--GAA-CCU--G---CG--GC-UGG--AUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....G--G--GAA-CCU--G---CG--GC-UGG--AUC--ACCUCCU-------",
                  "-----U---G--C--C---U-G-----G--C--CCU-UAG--C---GC--GG-UGG--UCC--CACCUGA.......",
                  ".........[--<--[...........[..[..[<<.[....]...>>--]....]..]......].>........]", // @@@ <- shall insert dots
                  ".........1.....1...........25.25.34..34...3--4....34...25.25.....1..........1",
                  ".........x........x........x...x.x....x...x........x...x...x.....x...........x",
                  "90034670040000090060354855652100942568200000061100650200211390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLgxvzrqmeMiMAjB5E!!J!!xT6!!JPiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(70), 2, ""));           // insert behind right helix end
        TEST_ALI_LEN_ALIGNED(79, 1);
        TEST_DATA(".....G---G--G--C---C-G...--A--G--GAA-CCU--G---CG--G--C-UGG--AUC--ACCUCC........",
                  "-----A---C--G--A---U-C-----C--G--GAA-CCU--G---CG--G--C-UGG--AUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.....G--G--GAA-CCU--G---CG--G--C-UGG--AUC--ACCUCCU-------",
                  "-----U---G--C--C---U-G-----G--C--CCU-UAG--C---GC--G--G-UGG--UCC--CACCUGA.......",
                  ".........[--<--[...........[..[..[<<.[....]...>>--]......]..]......].>........]",
                  ".........1.....1...........25.25.34..34...3--4....3--4...25.25.....1..........1", // @@@ <- helix nr destroyed + shall use dots
                  ".........x........x........x...x.x....x...x..........x...x...x.....x...........x", // @@@ _REF gets destroyed here! (see #159)
                  "9003467004000009006035485565210094256820000006110060050200211390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLgxvzrqmeMiMAjB5E!!J!!xT6!!J!!PiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);



        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(44), 2, ""));           // insert at gap border (between different gap types)
        TEST_ALI_LEN_ALIGNED(81, 1);
        TEST_DATA(".....G---G--G--C---C-G...----A--G--GAA-CCU--G---CG--G--C-UGG--AUC--ACCUCC........", // now prefers '-' here
                  "-----A---C--G--A---U-C-------C--G--GAA-CCU--G---CG--G--C-UGG--AUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.......G--G--GAA-CCU--G---CG--G--C-UGG--AUC--ACCUCCU-------",
                  "-----U---G--C--C---U-G-------G--C--CCU-UAG--C---GC--G--G-UGG--UCC--CACCUGA.......",
                  ".........[--<--[.............[..[..[<<.[....]...>>--]......]..]......].>........]",
                  ".........1.....1.............25.25.34..34...3--4....3--4...25.25.....1..........1",
                  ".........x........x..........x...x.x....x...x..........x...x...x.....x...........x",
                  "900346700400000900603548500565210094256820000006110060050200211390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYLg!!xvzrqmeMiMAjB5E!!J!!xT6!!J!!PiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);


        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(42), -6, "-.")); // delete gaps
        TEST_ALI_LEN_ALIGNED(75, 1);
        TEST_DATA(".....G---G--G--C---C-G.A--G--GAA-CCU--G---CG--G--C-UGG--AUC--ACCUCC........",
                  "-----A---C--G--A---U-C-C--G--GAA-CCU--G---CG--G--C-UGG--AUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.G--G--GAA-CCU--G---CG--G--C-UGG--AUC--ACCUCCU-------",
                  "-----U---G--C--C---U-G-G--C--CCU-UAG--C---GC--G--G-UGG--UCC--CACCUGA.......",
                  ".........[--<--[.......[..[..[<<.[....]...>>--]......]..]......].>........]",
                  ".........1.....1.......25.25.34..34...3--4....3--4...25.25.....1..........1",
                  ".........x........x....x...x.x....x...x..........x...x...x.....x...........x",
                  "900346700400000900603545210094256820000006110060050200211390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYzrqmeMiMAjB5E!!J!!xT6!!J!!PiCvQrq4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);

        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(74), -1, "-.")); // delete gap destroying helix nr
        TEST_ALI_LEN_ALIGNED(74, 1);
        TEST_DATA(".....G---G--G--C---C-G.A--G--GAA-CCU--G---CG--G--C-UGG-AUC--ACCUCC........",
                  "-----A---C--G--A---U-C-C--G--GAA-CCU--G---CG--G--C-UGG-AUC--ACCUCCU.......",
                  ".....A---C--G--A---A-C.G--G--GAA-CCU--G---CG--G--C-UGG-AUC--ACCUCCU-------",
                  "-----U---G--C--C---U-G-G--C--CCU-UAG--C---GC--G--G-UGG-UCC--CACCUGA.......",
                  ".........[--<--[.......[..[..[<<.[....]...>>--]......].]......].>........]",
                  ".........1.....1.......25.25.34..34...3--4....3--4...2525.....1..........1", // @@@ helix nr destroyed ('25.25' -> '2525')
                  ".........x........x....x...x.x....x...x..........x...x..x.....x...........x",
                  "90034670040000090060354521009425682000000611006005020021390043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYzrqmeMiMAjB5E!!J!!xT6!!J!!PiCvQr4uC!!LDoHlWV59!!DW!",
                  HELIX_STRUCT);


        TEST_EXPECT_NO_ERROR(GBT_insert_character(gb_main, ali_name, COL(73), -5, "%")); // delete anything
        TEST_ALI_LEN_ALIGNED(69, 1);
        TEST_DATA(".....G---G--G--C---C-G.A--G--GAA-CCU--G---CG--G--C-UGG-ACCUCC........",
                  "-----A---C--G--A---U-C-C--G--GAA-CCU--G---CG--G--C-UGG-ACCUCCU.......",
                  ".....A---C--G--A---A-C.G--G--GAA-CCU--G---CG--G--C-UGG-ACCUCCU-------",
                  "-----U---G--C--C---U-G-G--C--CCU-UAG--C---GC--G--G-UGG-CACCUGA.......",
                  ".........[--<--[.......[..[..[<<.[....]...>>--]......]...].>........]",
                  ".........1.....1.......25.25.34..34...3--4....3--4...2...1..........1",
                  ".........x........x....x...x.x....x...x..........x...x...x...........x",
                  "900346700400000900603545210094256820000006110060050200043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYzrqmeMiMAjB5E!!J!!xT6!!J!!PiCvQ!LDoHlWV59!!DW!",
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
            TEST_EXPECT_EQUAL(GBT_insert_character(gb_main, ali_name, COL(58), -3, "-."), // illegal delete
                              "SAI 'HELIX_NR': You tried to delete '4' at position 41  -> Operation aborted");
            ta.close("xxx");
        }
        {
            GB_transaction ta(gb_main);
            TEST_EXPECT_EQUAL(GBT_insert_character(gb_main, ali_name, 4711, 3, "-."), // illegal insert
                              "Can't insert at position 4711 (exceeds length 69 of alignment 'ali_mini')");
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
        TEST_DATA(".....G---G--G--C---C-G.A--G--GAA-CCU--G---CG--G--C-UGG-ACCUCC........",
                  "-----A---C--G--A---U-C-C--G--GAA-CCU--G---CG--G--C-UGG-ACCUCCU.......",
                  ".....A---C--G--A---A-C.G--G--GAA-CCU--G---CG--G--C-UGG-ACCUCCU-------",
                  "-----U---G--C--C---U-G-G--C--CCU-UAG--C---GC--G--G-UGG-CACCUGA.......",
                  ".........[--<--[.......[..[..[<<.[....]...>>--]......]...].>........]",
                  ".........1.....1.......25.25.34..34...3--4....3--4...2...1..........1",
                  ".........x........x....x...x.x....x...x..........x...x...x...........x",
                  "900346700400000900603545210094256820000006110060050200043589985100300",
                  "O!!Du8E!!J!!h!!6!!0e1XYzrqmeMiMAjB5E!!J!!xT6!!J!!PiCvQ!LDoHlWV59!!DW!",
                  HELIX_STRUCT);
    }

    GB_close(gb_main);
    TEST_EXPECT_NO_ERROR(error.deliver());
}

#endif // UNIT_TESTS

