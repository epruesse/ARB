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

    arb_assert((source == NULL && source_len == 0) || (source_len == strlen(source)));

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

void TEST_PosRange() {
    // TEST_ASSERT(0);

    PosRange empty = PosRange::empty();
    PosRange whole = PosRange::whole();
    PosRange from7 = PosRange::from(7);
    PosRange till9 = PosRange::till(9);
    PosRange seven2nine(7, 9);

    TEST_ASSERT(!empty.is_whole());      TEST_ASSERT(empty.is_restricted());
    TEST_ASSERT(whole.is_whole());       TEST_ASSERT(!whole.is_restricted());
    TEST_ASSERT(!from7.is_whole());      TEST_ASSERT(from7.is_restricted());
    TEST_ASSERT(!till9.is_whole());      TEST_ASSERT(till9.is_restricted()); 
    TEST_ASSERT(!seven2nine.is_whole()); TEST_ASSERT(seven2nine.is_restricted());  

    TEST_ASSERT(empty.is_explicit());
    TEST_ASSERT(!whole.is_explicit());
    TEST_ASSERT(!from7.is_explicit());
    TEST_ASSERT(till9.is_explicit());
    TEST_ASSERT(seven2nine.is_explicit());

    TEST_ASSERT(empty.is_empty());
    TEST_ASSERT(!whole.is_empty());
    TEST_ASSERT(!from7.is_empty());
    TEST_ASSERT(!till9.is_empty());
    TEST_ASSERT(!seven2nine.is_empty());

    TEST_ASSERT_EQUAL(empty.size(), 0);
    TEST_ASSERT_EQUAL(whole.size(), -1); 
    TEST_ASSERT_EQUAL(from7.size(), -1);
    TEST_ASSERT_EQUAL(till9.size(), 10);
    TEST_ASSERT_EQUAL(seven2nine.size(), 3);
    
    ExplicitRange wholeOf17(whole, 17);
    TEST_ASSERT(wholeOf17.is_explicit());

    TEST_ASSERT(whole == whole);
    TEST_ASSERT(from7 == from7);
    TEST_ASSERT(whole != from7);

    TEST_ASSERT(from7 == PosRange::after(6));
    TEST_ASSERT(till9 == PosRange::prior(10));
}

void TEST_ExplicitRange() {
    PosRange empty;
    PosRange whole  = PosRange::whole();
    PosRange till50 = PosRange::till(50);
    PosRange from30 = PosRange::from(30);
    PosRange f30t50(30, 50);

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


#endif // UNIT_TESTS

// --------------------------------------------------------------------------------



