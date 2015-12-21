// ================================================================= //
//                                                                   //
//   File      : AWT_hexdump.cxx                                     //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2011   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#include "awt_hexdump.hxx"

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#define DO_HEXDUMP(off,hex,ascii,width,gap,space)       \
    str.erase();                                        \
    MemDump(off, hex, ascii, width, gap,space)          \
        .dump_to(str, buf, len)

#define TEST_HEXDUMP_EQUAL(width,gap,off,hex,ascii,space,expected) do { \
        DO_HEXDUMP(off,hex,ascii,width,gap,space);                      \
        TEST_EXPECT_EQUAL(str.get_data(), expected);                    \
    } while(0)

#define TEST_HEXDUMP_EQUAL__BROKEN(width,gap,off,hex,ascii,space,expected) do { \
        DO_HEXDUMP(off,hex,ascii,width,gap,space);                              \
        TEST_EXPECT_EQUAL__BROKEN(str.get_data(), expected);                    \
    } while(0)

void TEST_hexdump() {
    GBS_strstruct str(200);
    {
        char buf[] = { 0x11, 0x47, 0, 0};
        int len = 4;

        TEST_HEXDUMP_EQUAL(0, 0, false, true,  false, true,  "11 47 00 00\n");        // unwrapped hexdump
        TEST_HEXDUMP_EQUAL(0, 0, false, true,  false, false, "11470000\n");           // unwrapped hexdump (unspaced)
        TEST_HEXDUMP_EQUAL(0, 0, false, false, true,  true,  ".G..\n");               // unwrapped ascii
        TEST_HEXDUMP_EQUAL(0, 0, false, true,  true,  true,  "11 47 00 00 | .G..\n"); // unwrapped hex+ascii

        TEST_HEXDUMP_EQUAL(0, 0, true, false, true,  true, "0000 | .G..\n");               // unwrapped ascii
        TEST_HEXDUMP_EQUAL(0, 0, true, true,  false, true, "0000 | 11 47 00 00\n");        // unwrapped hex
        TEST_HEXDUMP_EQUAL(0, 0, true, true,  true,  true, "0000 | 11 47 00 00 | .G..\n"); // unwrapped hex+ascii

        TEST_HEXDUMP_EQUAL(4, 0, false, true,  false, true, "11 47 00 00\n");
        TEST_HEXDUMP_EQUAL(4, 0, true,  true,  false, true, "0000 | 11 47 00 00\n");
        TEST_HEXDUMP_EQUAL(4, 0, true,  true,  true,  true, "0000 | 11 47 00 00 | .G..\n");

        TEST_HEXDUMP_EQUAL(3, 0, false, true,  false, true, "11 47 00\n00\n");
        TEST_HEXDUMP_EQUAL(3, 0, true,  true,  false, true, "0000 | 11 47 00\n0003 | 00\n");
        TEST_HEXDUMP_EQUAL(3, 0, true,  true,  true,  true, "0000 | 11 47 00 | .G.\n0003 | 00       | .\n");

        TEST_HEXDUMP_EQUAL(2, 0, false, true,  false, true, "11 47\n00 00\n");
        TEST_HEXDUMP_EQUAL(2, 0, true,  true,  false, true, "0000 | 11 47\n0002 | 00 00\n");
        TEST_HEXDUMP_EQUAL(2, 0, true,  true,  true,  true, "0000 | 11 47 | .G\n0002 | 00 00 | ..\n");

        TEST_HEXDUMP_EQUAL(1, 0, false, true,  false, true, "11\n47\n00\n00\n");
        TEST_HEXDUMP_EQUAL(1, 0, true,  true,  false, true, "0000 | 11\n0001 | 47\n0002 | 00\n0003 | 00\n");
        TEST_HEXDUMP_EQUAL(1, 0, true,  true,  true,  true, "0000 | 11 | .\n0001 | 47 | G\n0002 | 00 | .\n0003 | 00 | .\n");
    }

    {
        char buf[] = "\1Smarkerline\1Sposvar_full_all\1Sp";
        int len    = strlen(buf);
        TEST_HEXDUMP_EQUAL(16, 0, true, true, true, true, 
                           "0000 | 01 53 6D 61 72 6B 65 72 6C 69 6E 65 01 53 70 6F | .Smarkerline.Spo\n"
                           "0010 | 73 76 61 72 5F 66 75 6C 6C 5F 61 6C 6C 01 53 70 | svar_full_all.Sp\n");
        TEST_HEXDUMP_EQUAL(16, 4, true, true, true, true, 
                           "0000 | 01 53 6D 61  72 6B 65 72  6C 69 6E 65  01 53 70 6F | .Sma rker line .Spo\n"
                           "0010 | 73 76 61 72  5F 66 75 6C  6C 5F 61 6C  6C 01 53 70 | svar _ful l_al l.Sp\n");
        TEST_HEXDUMP_EQUAL(13, 4, true, true, true, true, 
                           "0000 | 01 53 6D 61  72 6B 65 72  6C 69 6E 65  01 | .Sma rker line .\n"
                           "000D | 53 70 6F 73  76 61 72 5F  66 75 6C 6C  5F | Spos var_ full _\n"
                           "001A | 61 6C 6C 01  53 70                        | all. Sp\n"
                           );
        TEST_HEXDUMP_EQUAL(13, 4, true, false, true, true, 
                           "0000 | .Sma rker line .\n"
                           "000D | Spos var_ full _\n"
                           "001A | all. Sp\n"
                           );
    }
}
TEST_PUBLISH(TEST_hexdump);

#endif // UNIT_TESTS



