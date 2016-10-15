// ============================================================= //
//                                                               //
//   File      : FileContent.cxx                                 //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "FileContent.h"
#include "BufferedFileReader.h"
#include "arb_msg.h"
#include "arb_string.h"
#include "arb_file.h"

using namespace std;

void FileContent::init() {
    FILE *fp = fopen(path, "rb"); // b!
    if (!fp) {
        error = GB_IO_error("loading", path);
    }
    else {
        BufferedFileReader buf(path, fp);

        string line;
        while (buf.getLine(line)) {
            Lines.put(GB_strndup(line.c_str(), line.length()));
        }
    }
}

GB_ERROR FileContent::save() {
    arb_assert(!has_error());

    FILE *out    = fopen(path, "wt");
    bool  failed = !out;
    
    if (out) {
        for (size_t i = 0; i<Lines.size(); ++i) {
            fputs(Lines[i], out);
            fputc('\n', out);
        }
        failed = fclose(out) != 0;
    }
    
    if (failed) error = GB_IO_error("saving", path);
    return error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

static arb_test::match_expectation arrays_equal(const StrArray& expected, const StrArray& got) {
    using namespace   arb_test;
    match_expectation same_size = that(expected.size()).is_equal_to(got.size());
    if (same_size.fulfilled()) {
        for (size_t i = 0; i<expected.size(); ++i) {
            match_expectation eq = that(expected[i]).is_equal_to(got[i]);
            if (!eq.fulfilled()) {
                return all().of(same_size, eq);
            }
        }
    }
    return same_size;
}

#define TEST_EXPECT_READS_SAME(fc,name) do {                     \
        FileContent oc(name);                                    \
        TEST_EXPECTATION(arrays_equal(fc.lines(), oc.lines()));       \
    } while(0)

void TEST_linefeed_conversion() {
    const char *funix   = "general/text.input";      // LF
    const char *fdos    = "general/dos.input";       // CR LF
    const char *fmac    = "general/mac.input";       // CR
    const char *fbroken = "general/broken_LF.input"; // LF CR

    const int LINES = 6;

    FileContent cunix(funix);
    TEST_EXPECT_EQUAL(cunix.lines().size(), LINES);

    TEST_EXPECT_EQUAL(GB_size_of_file(fmac),    GB_size_of_file(funix));
    TEST_EXPECT_EQUAL(GB_size_of_file(fdos),    GB_size_of_file(funix)+LINES);
    TEST_EXPECT_EQUAL(GB_size_of_file(fbroken), GB_size_of_file(fdos));

    TEST_EXPECT_READS_SAME(cunix,fdos);
    TEST_EXPECT_READS_SAME(cunix,fmac);
    TEST_EXPECT_READS_SAME(cunix,fbroken);
}
TEST_PUBLISH(TEST_linefeed_conversion);

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
