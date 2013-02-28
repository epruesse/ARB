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

    TEST_REJECT(empty.is_whole());      TEST_REJECT(empty.is_part());      TEST_EXPECT(empty.is_empty());
    TEST_EXPECT(whole.is_whole());      TEST_REJECT(whole.is_part());      TEST_REJECT(whole.is_empty());
    TEST_REJECT(from7.is_whole());      TEST_EXPECT(from7.is_part());      TEST_REJECT(from7.is_empty());
    TEST_REJECT(till9.is_whole());      TEST_EXPECT(till9.is_part());      TEST_REJECT(till9.is_empty());
    TEST_REJECT(seven2nine.is_whole()); TEST_EXPECT(seven2nine.is_part()); TEST_REJECT(seven2nine.is_empty());

    TEST_EXPECT(empty.is_limited());
    TEST_REJECT(whole.is_limited());
    TEST_REJECT(from7.is_limited());
    TEST_EXPECT(till9.is_limited());
    TEST_EXPECT(seven2nine.is_limited());

    // test size
    TEST_EXPECT_EQUAL(empty.size(), 0);
    TEST_EXPECT(whole.size() < 0);
    TEST_EXPECT(from7.size() < 0);
    TEST_EXPECT_EQUAL(till9.size(), 10);
    TEST_EXPECT_EQUAL(seven2nine.size(), 3);

    TEST_EXPECT(wellDefined(empty));
    TEST_EXPECT(wellDefined(whole));
    TEST_EXPECT(wellDefined(from7));
    TEST_EXPECT(wellDefined(till9));
    TEST_EXPECT(wellDefined(seven2nine));

    // test equality
    TEST_EXPECT(empty == empty);
    TEST_EXPECT(whole == whole);
    TEST_EXPECT(from7 == from7);
    TEST_EXPECT(whole != from7);

    TEST_EXPECT(from7 == PosRange::after(6));
    TEST_EXPECT(till9 == PosRange::prior(10));

    // test containment
    for (int pos = -3; pos<12; ++pos) {
        TEST_ANNOTATE_ASSERT(GBS_global_string("pos=%i", pos));

        TEST_REJECT(empty.contains(pos));
        TEST_EXPECT(correlated(whole.contains(pos),      pos >= 0));
        TEST_EXPECT(correlated(from7.contains(pos),      pos >= 7));
        TEST_EXPECT(correlated(till9.contains(pos),      pos >= 0 && pos <= 9));
        TEST_EXPECT(correlated(seven2nine.contains(pos), pos >= 7 && pos <= 9));
    }
    TEST_EXPECT(whole.contains(INT_MAX));
    TEST_EXPECT(from7.contains(INT_MAX));
    TEST_REJECT(empty.contains(INT_MAX));
}

void TEST_ExplicitRange() {
    PosRange empty;
    PosRange whole  = PosRange::whole();
    PosRange till50 = PosRange::till(50);
    PosRange from30 = PosRange::from(30);
    PosRange f30t50(30, 50);

    ExplicitRange wholeOf17(whole, 17);
    TEST_EXPECT(wholeOf17.is_limited());

    TEST_EXPECT_EQUAL(ExplicitRange(whole, 100).size(), 100);
    TEST_EXPECT_EQUAL(ExplicitRange(till50, 100).size(), 51); // 0-50
    TEST_EXPECT_EQUAL(ExplicitRange(till50, 40).size(), 40);
    TEST_EXPECT_EQUAL(ExplicitRange(from30, 100).size(), 70);
    TEST_EXPECT_EQUAL(ExplicitRange(from30, 20).size(), 0);
    TEST_EXPECT_EQUAL(ExplicitRange(f30t50, 100).size(), 21);
    TEST_EXPECT_EQUAL(ExplicitRange(f30t50, 40).size(), 10);  // 30-39

    TEST_EXPECT_EQUAL(ExplicitRange(empty, 20).size(), 0);
}

void TEST_range_copying() {
    char          dest[100];
    const char   *source     = "0123456789";
    const size_t  source_len = strlen(source);

    PosRange::till(2).copy_corresponding_part(dest, source, source_len);
    TEST_EXPECT_EQUAL(dest, "012");

    PosRange(2, 5).copy_corresponding_part(dest, source, source_len);
    TEST_EXPECT_EQUAL(dest, "2345");

    PosRange::from(7).copy_corresponding_part(dest, source, source_len);
    TEST_EXPECT_EQUAL(dest, "789");

    PosRange(9, 1000).copy_corresponding_part(dest, source, source_len);
    TEST_EXPECT_EQUAL(dest, "9");

    PosRange(900, 1000).copy_corresponding_part(dest, source, source_len);
    TEST_EXPECT_EQUAL(dest, "");
    
    // make sure dest and source may overlap:
    strcpy(dest, source);
    PosRange::whole().copy_corresponding_part(dest+1, dest, source_len);
    TEST_EXPECT_EQUAL(dest+1, source);

    strcpy(dest, source);
    PosRange::whole().copy_corresponding_part(dest, dest+1, source_len-1);
    TEST_EXPECT_EQUAL(dest, source+1);

    TEST_EXPECT_EQUAL(PosRange::empty().dup_corresponding_part(source, source_len), ""); // empty range
    TEST_EXPECT_EQUAL(PosRange(2, 5).dup_corresponding_part(NULL, 0), ""); // empty source
}

void TEST_range_intersection() {
    PosRange empty;
    PosRange whole  = PosRange::whole();
    PosRange till50 = PosRange::till(50);
    PosRange from30 = PosRange::from(30);
    PosRange part(30, 50);

    TEST_EXPECT(intersection(empty, empty)  == empty);
    TEST_EXPECT(intersection(empty, whole)  == empty);
    TEST_EXPECT(intersection(empty, till50) == empty);
    TEST_EXPECT(intersection(empty, from30) == empty);
    TEST_EXPECT(intersection(empty, part)   == empty);

    TEST_EXPECT(intersection(whole, empty)  == empty);
    TEST_EXPECT(intersection(whole, whole)  == whole);
    TEST_EXPECT(intersection(whole, till50) == till50);
    TEST_EXPECT(intersection(whole, from30) == from30);
    TEST_EXPECT(intersection(whole, part)   == part);
    
    TEST_EXPECT(intersection(till50, empty)  == empty);
    TEST_EXPECT(intersection(till50, whole)  == till50);
    TEST_EXPECT(intersection(till50, till50) == till50);
    TEST_EXPECT(intersection(till50, from30) == part);
    TEST_EXPECT(intersection(till50, part)   == part);

    TEST_EXPECT(intersection(from30, empty)  == empty);
    TEST_EXPECT(intersection(from30, whole)  == from30);
    TEST_EXPECT(intersection(from30, till50) == part);
    TEST_EXPECT(intersection(from30, from30) == from30);
    TEST_EXPECT(intersection(from30, part)   == part);

    TEST_EXPECT(intersection(part, empty)  == empty);
    TEST_EXPECT(intersection(part, whole)  == part);
    TEST_EXPECT(intersection(part, till50) == part);
    TEST_EXPECT(intersection(part, from30) == part);
    TEST_EXPECT(intersection(part, part)   == part);

    TEST_EXPECT(intersection(PosRange(20, 40), till50) == PosRange(20, 40));
    TEST_EXPECT(intersection(PosRange(40, 60), till50) == PosRange(40, 50));
    TEST_EXPECT(intersection(PosRange(60, 80), till50) == empty);

    TEST_EXPECT(intersection(PosRange(0,  20), from30) == empty);
    TEST_EXPECT(intersection(PosRange(20, 40), from30) == PosRange(30, 40));
    TEST_EXPECT(intersection(PosRange(40, 60), from30) == PosRange(40, 60));
    
    TEST_EXPECT(intersection(PosRange(40, 60), PosRange(50, 70)) == PosRange(50, 60));
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------



