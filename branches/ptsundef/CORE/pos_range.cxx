// =============================================================== //
//                                                                 //
//   File      : pos_range.cxx                                     //
//   Purpose   : range of positions (e.g. part of seq)             //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2011   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "pos_range.h"


void PosRange::copy_corresponding_part(char *dest, const char *source, size_t source_len) const {
    // dest and source may overlap

    arb_assert((source == NULL && source_len == 0) || (source && source_len == strlen(source)));

    ExplicitRange range(*this, source_len);
    int           Size = range.size();

    if (Size) memmove(dest, source+start(), Size);
    dest[Size] = 0;
}

char *PosRange::dup_corresponding_part(const char *source, size_t source_len) const {
    ExplicitRange  range(*this, source_len);
    int            Size = range.size();
    char          *dup  = (char*)malloc(Size+1);

    copy_corresponding_part(dup, source, source_len);
    return dup;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS

#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif
#include "arb_msg.h"
#include <climits>

inline bool exactly_one_of(bool b1, bool b2, bool b3) { return (b1+b2+b3) == 1; }
inline bool wellDefined(PosRange range) {
    return
        exactly_one_of(range.is_empty(), range.is_part(), range.is_whole()) &&
        contradicted(range.is_limited(), range.is_unlimited());
}

void TEST_PosRange() {
    PosRange empty = PosRange::empty();
    PosRange whole = PosRange::whole();
    PosRange from7 = PosRange::from(7);
    PosRange till9 = PosRange::till(9);
    PosRange seven2nine(7, 9);

    TEST_ASSERT(!empty.is_whole());      TEST_ASSERT(!empty.is_part());      TEST_ASSERT( empty.is_empty());
    TEST_ASSERT( whole.is_whole());      TEST_ASSERT(!whole.is_part());      TEST_ASSERT(!whole.is_empty());     
    TEST_ASSERT(!from7.is_whole());      TEST_ASSERT( from7.is_part());      TEST_ASSERT(!from7.is_empty());     
    TEST_ASSERT(!till9.is_whole());      TEST_ASSERT( till9.is_part());      TEST_ASSERT(!till9.is_empty());     
    TEST_ASSERT(!seven2nine.is_whole()); TEST_ASSERT( seven2nine.is_part()); TEST_ASSERT(!seven2nine.is_empty()); 

    TEST_ASSERT( empty.is_limited());     
    TEST_ASSERT(!whole.is_limited());     
    TEST_ASSERT(!from7.is_limited());
    TEST_ASSERT( till9.is_limited());
    TEST_ASSERT( seven2nine.is_limited());

    // test size
    TEST_ASSERT_EQUAL(empty.size(), 0);
    TEST_ASSERT(whole.size() < 0);
    TEST_ASSERT(from7.size() < 0);
    TEST_ASSERT_EQUAL(till9.size(), 10);
    TEST_ASSERT_EQUAL(seven2nine.size(), 3);

    TEST_ASSERT(wellDefined(empty));
    TEST_ASSERT(wellDefined(whole));
    TEST_ASSERT(wellDefined(from7));
    TEST_ASSERT(wellDefined(till9));
    TEST_ASSERT(wellDefined(seven2nine));

    // test equality
    TEST_ASSERT(empty == empty);
    TEST_ASSERT(whole == whole);
    TEST_ASSERT(from7 == from7);
    TEST_ASSERT(whole != from7);

    TEST_ASSERT(from7 == PosRange::after(6));
    TEST_ASSERT(till9 == PosRange::prior(10));

    // test containment
    for (int pos = -3; pos<12; ++pos) {
        TEST_ANNOTATE_ASSERT(GBS_global_string("pos=%i", pos));

        TEST_ASSERT(!empty.contains(pos));
        TEST_ASSERT(correlated(whole.contains(pos),      pos >= 0));
        TEST_ASSERT(correlated(from7.contains(pos),      pos >= 7));
        TEST_ASSERT(correlated(till9.contains(pos),      pos >= 0 && pos <= 9));
        TEST_ASSERT(correlated(seven2nine.contains(pos), pos >= 7 && pos <= 9));
    }
    TEST_ASSERT(whole.contains(INT_MAX));
    TEST_ASSERT(from7.contains(INT_MAX));
    TEST_ASSERT(!empty.contains(INT_MAX));
}

void TEST_ExplicitRange() {
    PosRange empty;
    PosRange whole  = PosRange::whole();
    PosRange till50 = PosRange::till(50);
    PosRange from30 = PosRange::from(30);
    PosRange f30t50(30, 50);

    ExplicitRange wholeOf17(whole, 17);
    TEST_ASSERT(wholeOf17.is_limited());

    TEST_ASSERT_EQUAL(ExplicitRange(whole, 100).size(), 100);
    TEST_ASSERT_EQUAL(ExplicitRange(till50, 100).size(), 51); // 0-50
    TEST_ASSERT_EQUAL(ExplicitRange(till50, 40).size(), 40);
    TEST_ASSERT_EQUAL(ExplicitRange(from30, 100).size(), 70);
    TEST_ASSERT_EQUAL(ExplicitRange(from30, 20).size(), 0);
    TEST_ASSERT_EQUAL(ExplicitRange(f30t50, 100).size(), 21);
    TEST_ASSERT_EQUAL(ExplicitRange(f30t50, 40).size(), 10);  // 30-39

    TEST_ASSERT_EQUAL(ExplicitRange(empty, 20).size(), 0);
}

void TEST_range_copying() {
    char          dest[100];
    const char   *source     = "0123456789";
    const size_t  source_len = strlen(source);

    PosRange::till(2).copy_corresponding_part(dest, source, source_len);
    TEST_ASSERT_EQUAL(dest, "012");

    PosRange(2, 5).copy_corresponding_part(dest, source, source_len);
    TEST_ASSERT_EQUAL(dest, "2345");

    PosRange::from(7).copy_corresponding_part(dest, source, source_len);
    TEST_ASSERT_EQUAL(dest, "789");

    PosRange(9, 1000).copy_corresponding_part(dest, source, source_len);
    TEST_ASSERT_EQUAL(dest, "9");

    PosRange(900, 1000).copy_corresponding_part(dest, source, source_len);
    TEST_ASSERT_EQUAL(dest, "");
    
    // make sure dest and source may overlap:
    strcpy(dest, source);
    PosRange::whole().copy_corresponding_part(dest+1, dest, source_len);
    TEST_ASSERT_EQUAL(dest+1, source);

    strcpy(dest, source);
    PosRange::whole().copy_corresponding_part(dest, dest+1, source_len-1);
    TEST_ASSERT_EQUAL(dest, source+1);

    TEST_ASSERT_EQUAL(PosRange::empty().dup_corresponding_part(source, source_len), ""); // empty range
    TEST_ASSERT_EQUAL(PosRange(2, 5).dup_corresponding_part(NULL, 0), ""); // empty source
}

void TEST_range_intersection() {
    PosRange empty;
    PosRange whole  = PosRange::whole();
    PosRange till50 = PosRange::till(50);
    PosRange from30 = PosRange::from(30);
    PosRange part(30, 50);

    TEST_ASSERT(intersection(empty, empty)  == empty);
    TEST_ASSERT(intersection(empty, whole)  == empty);
    TEST_ASSERT(intersection(empty, till50) == empty);
    TEST_ASSERT(intersection(empty, from30) == empty);
    TEST_ASSERT(intersection(empty, part)   == empty);

    TEST_ASSERT(intersection(whole, empty)  == empty);
    TEST_ASSERT(intersection(whole, whole)  == whole);
    TEST_ASSERT(intersection(whole, till50) == till50);
    TEST_ASSERT(intersection(whole, from30) == from30);
    TEST_ASSERT(intersection(whole, part)   == part);
    
    TEST_ASSERT(intersection(till50, empty)  == empty);
    TEST_ASSERT(intersection(till50, whole)  == till50);
    TEST_ASSERT(intersection(till50, till50) == till50);
    TEST_ASSERT(intersection(till50, from30) == part);
    TEST_ASSERT(intersection(till50, part)   == part);

    TEST_ASSERT(intersection(from30, empty)  == empty);
    TEST_ASSERT(intersection(from30, whole)  == from30);
    TEST_ASSERT(intersection(from30, till50) == part);
    TEST_ASSERT(intersection(from30, from30) == from30);
    TEST_ASSERT(intersection(from30, part)   == part);

    TEST_ASSERT(intersection(part, empty)  == empty);
    TEST_ASSERT(intersection(part, whole)  == part);
    TEST_ASSERT(intersection(part, till50) == part);
    TEST_ASSERT(intersection(part, from30) == part);
    TEST_ASSERT(intersection(part, part)   == part);

    TEST_ASSERT(intersection(PosRange(20, 40), till50)    == PosRange(20, 40));
    TEST_ASSERT(intersection(PosRange(40, 60), till50)    == PosRange(40, 50));
    TEST_ASSERT(intersection(PosRange(60, 80), till50) == empty);

    TEST_ASSERT(intersection(PosRange(0,  20), from30) == empty);
    TEST_ASSERT(intersection(PosRange(20, 40), from30) == PosRange(30, 40));
    TEST_ASSERT(intersection(PosRange(40, 60), from30) == PosRange(40, 60));
    
    TEST_ASSERT(intersection(PosRange(40, 60), PosRange(50, 70)) == PosRange(50, 60));
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------



