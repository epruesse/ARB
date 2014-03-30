// ============================================================= //
//                                                               //
//   File      : RangeList.cxx                                   //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "RangeList.h"

void RangeList::add_combined(const PosRange& range, iterator found) {
    PosRange combined(std::min(range.start(), found->start()), std::max(range.end(), found->end()));
    ranges.erase(found);
    add(combined);
}

RangeList build_RangeList_from_string(const char *SAI_data, const char *set_bits) {
    RangeList rlist;
    int       pos   = 0;
    bool      isSet = strchr(set_bits, SAI_data[pos]);

    while (SAI_data[pos]) {
        if (isSet) {
            size_t setCount = strspn(SAI_data+pos, set_bits);
            rl_assert(setCount>0);
            rlist.add(PosRange(pos, pos+setCount-1));
            pos += setCount;
        }
        else {
            size_t clrCount = strcspn(SAI_data+pos, set_bits);
            pos += clrCount;
        }
        isSet = !isSet;
    }

    return rlist;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif
#include <arb_strbuf.h>

static const char *dump(const RangeList& rlist) {
    static GBS_strstruct buf;
    buf.reset_pos();
    for (RangeList::iterator i = rlist.begin(); i != rlist.end(); ++i) {
        buf.nprintf(40, "%i-%i,", i->start(), i->end());
    }
    buf.cut_tail(1);
    return buf.get_data();
}
static const char *dump_reverse(const RangeList& rlist) {
    static GBS_strstruct buf;
    buf.reset_pos();
    for (RangeList::reverse_iterator i = rlist.rbegin(); i != rlist.rend(); ++i) {
        buf.nprintf(40, "%i-%i,", i->start(), i->end());
    }
    buf.cut_tail(1);
    return buf.get_data();
}

void TEST_RangeList() {
    RangeList ranges;

    TEST_ASSERT_EQUAL(ranges.size(), 0);

    ranges.add(PosRange(10, 20));
    TEST_ASSERT_EQUAL(ranges.size(), 1);
    TEST_ASSERT_EQUAL(dump(ranges), "10-20");

    ranges.add(PosRange(30, 40));
    TEST_ASSERT_EQUAL(ranges.size(), 2);
    TEST_ASSERT_EQUAL(dump(ranges), "10-20,30-40");

    ranges.add(PosRange(5, 6));
    TEST_ASSERT_EQUAL(ranges.size(), 3);
    TEST_ASSERT_EQUAL(dump(ranges), "5-6,10-20,30-40");
    
    // add range inbetween
    ranges.add(PosRange(25, 27));
    TEST_ASSERT_EQUAL(ranges.size(), 4);
    TEST_ASSERT_EQUAL(dump(ranges), "5-6,10-20,25-27,30-40");

    // add already-set range
    ranges.add(PosRange(12, 17));
    TEST_ASSERT_EQUAL(ranges.size(), 4);
    TEST_ASSERT_EQUAL(dump(ranges), "5-6,10-20,25-27,30-40");
    
    TEST_ASSERT_EQUAL(dump_reverse(ranges), "30-40,25-27,10-20,5-6"); // test reverse iterator

    // expand an already-set range (upwards; overlapping)
    ranges.add(PosRange(38, 42));
    TEST_ASSERT_EQUAL(ranges.size(), 4);
    TEST_ASSERT_EQUAL(dump(ranges), "5-6,10-20,25-27,30-42");

    // expand an already-set range (upwards; adjacent)
    ranges.add(PosRange(43, 45));
    TEST_ASSERT_EQUAL(ranges.size(), 4);
    TEST_ASSERT_EQUAL(dump(ranges), "5-6,10-20,25-27,30-45");

    // expand an already-set range (downwards; overlapping)
    ranges.add(PosRange(8, 12));
    TEST_ASSERT_EQUAL(ranges.size(), 4);
    TEST_ASSERT_EQUAL(dump(ranges), "5-6,8-20,25-27,30-45");

    // expand an already-set range (downwards; adjacent)
    ranges.add(PosRange(24, 24));
    TEST_ASSERT_EQUAL(ranges.size(), 4);
    TEST_ASSERT_EQUAL(dump(ranges), "5-6,8-20,24-27,30-45");

    // aggregate ranges (overlapping)
    ranges.add(PosRange(19, 25));
    TEST_ASSERT_EQUAL(ranges.size(), 3);
    TEST_ASSERT_EQUAL(dump(ranges), "5-6,8-27,30-45");

    // aggregate ranges (adjacent)
    ranges.add(PosRange(28, 29));
    TEST_ASSERT_EQUAL(ranges.size(), 2);
    TEST_ASSERT_EQUAL(dump(ranges), "5-6,8-45");
}

static const char *convert(const char *saistring, const char *set_bits) {
    RangeList   rlist = build_RangeList_from_string(saistring, set_bits);
    return dump(rlist);
}

void TEST_string2RangeList() {
    TEST_ASSERT_EQUAL(convert("xxxxx-----xxxxx-----xxxxx-----", "x"),   "0-4,10-14,20-24");
    TEST_ASSERT_EQUAL(convert("xxxxx-----yyyyy-----xxzzx-----", "xyz"), "0-4,10-14,20-24");
    TEST_ASSERT_EQUAL(convert("xxxxx-----xxxxx-----xxxxx-----", "-"),   "5-9,15-19,25-29");

    TEST_ASSERT_EQUAL(convert("x-x-x", "x"), "0-0,2-2,4-4");
    TEST_ASSERT_EQUAL(convert("x-x-x", "-"), "1-1,3-3");
    TEST_ASSERT_EQUAL(convert("---", "x"), "");

    
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

