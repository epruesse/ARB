// ============================================================ //
//                                                              //
//   File      : AW_rgb.cxx                                     //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "aw_rgb.hxx"

static AW_rgb16 CRAZY_PINK("#C0C");
static AW_rgb16 convert_xcolorname(const char *xcolorname, bool& failed);

void AW_rgb16::rescale(uint16_t my_max) {
    float scale = MAX/my_max;

    R = R*scale + 0.5;
    G = G*scale + 0.5;
    B = B*scale + 0.5;
}

AW_rgb16::AW_rgb16(const char *colorname) {
    /*! converts colorname
     * @param colorname  either color hexstring (allowed for orange: '#F80', '#ff8800', '#fFF888000' or '#ffff88880000') or X color name (e.g. "green")
     */

    bool failed = (colorname[0] != '#');
    if (!failed) {
        const char    *end;
        unsigned long  num    = strtoul(colorname+1, const_cast<char**>(&end), 16);
        int            length = end-colorname-1;

        switch (length) {
            case  3: R = num >>  8; G = (num >>  4) & 0xf;    B = num & 0xf;    rescale(0xf);   break;
            case  6: R = num >> 16; G = (num >>  8) & 0xff;   B = num & 0xff;   rescale(0xff);  break;
            case  9: R = num >> 24; G = (num >> 12) & 0xfff;  B = num & 0xfff;  rescale(0xfff); break;
            case 12: R = num >> 32; G = (num >> 16) & 0xffff; B = num & 0xffff; break;

            default:
                aw_assert(0); // hex color spec has invalid length
                failed = true;
                break;
        }
    }

    if (failed) *this = convert_xcolorname(colorname, failed);

    if (failed) {
        fprintf(stderr, "Warning: Failed to interpret color '%s' - fallback to crazy pink\n", colorname);
        *this = CRAZY_PINK;
    }
}

const char *AW_rgb16::ascii() const {
    const int   FIXEDLEN = 1+3*4;
    static char buffer[FIXEDLEN+1];

#if defined(ASSERTION_USED)
    int printed =
#endif
        sprintf(buffer, "#%04x%04x%04x", R, G, B);

    aw_assert(printed == FIXEDLEN);

    return buffer;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

#define TEST_ASCII_COLOR_CONVERT(i,o) TEST_EXPECT_EQUAL(AW_rgb16(i).ascii(),o)
#define TEST_ASCII_COLOR_IDENT(c)     TEST_ASCII_COLOR_CONVERT(c,c);

#define TEST_NORMALIZED_CONVERSION(c) TEST_EXPECT_EQUAL(AW_rgb16(AW_rgb_normalized(AW_rgb16(c))).ascii(), c);

void TEST_rgb() {
    // Note: more related tests in AW_preset.cxx@RGB_TESTS

    // check construction from diff.-length color specifications
    AW_rgb16 orange4("#f80");
    AW_rgb16 orange8("#ff8800");
    AW_rgb16 orange12("#fff888000");
    AW_rgb16 orange16("#ffff88880000");

    TEST_EXPECT(orange16 == AW_rgb16(65535, 34952, 0));
    TEST_EXPECT(orange12 == orange16);
    TEST_EXPECT(orange8  == orange12);
    TEST_EXPECT(orange4  == orange8);

    AW_rgb16 lblue4("#004");
    AW_rgb16 lblue8("#000044");
    AW_rgb16 lblue12("#000000444");
    AW_rgb16 lblue16("#000000004444");

    TEST_EXPECT(lblue16 == AW_rgb16(0, 0, 17476));
    TEST_EXPECT(lblue12 == lblue16);
    TEST_EXPECT(lblue8  == lblue12);
    TEST_EXPECT(lblue4  == lblue8);

    TEST_EXPECT_EQUAL(lblue8.ascii(), "#000000004444");

    TEST_ASCII_COLOR_CONVERT("#1"   "2"   "3",   "#1111" "2222" "3333");
    TEST_ASCII_COLOR_CONVERT("#45"  "67"  "89",  "#4545" "6767" "8989");
    TEST_ASCII_COLOR_CONVERT("#678" "9ab" "cde", "#6786" "9ab9" "cdec");
    TEST_ASCII_COLOR_CONVERT("#001" "010" "100", "#0010" "0100" "1001");

    TEST_ASCII_COLOR_IDENT("#0123456789ab");
    TEST_ASCII_COLOR_IDENT("#fedcba987654");

    // conversion normalized -> 16bit for special values
    TEST_EXPECT_EQUAL(AW_rgb16(AW_rgb_normalized(0.25,  0.5,    0.75  )).ascii(), "#3fff" "7fff" "bfff");
    TEST_EXPECT_EQUAL(AW_rgb16(AW_rgb_normalized(1./3,  1./5,   1./7  )).ascii(), "#5555" "3333" "2492");
    TEST_EXPECT_EQUAL(AW_rgb16(AW_rgb_normalized(.1,    .01,    .001  )).ascii(), "#1999" "028f" "0041");
    TEST_EXPECT_EQUAL(AW_rgb16(AW_rgb_normalized(.0001, .00005, .00002)).ascii(), "#0006" "0003" "0001");

    // test conversion 16bit -> normalized -> 16bit is stable
    TEST_NORMALIZED_CONVERSION("#000000000000");
    TEST_NORMALIZED_CONVERSION("#0123456789ab");
    TEST_NORMALIZED_CONVERSION("#ffffffffffff");

    // invalid colors
    TEST_EXPECT(AW_rgb16("kashdkjasdh") == CRAZY_PINK);
    TEST_EXPECT(AW_rgb16("")            == CRAZY_PINK);

#ifdef ARB_GTK
    // x-colors (doesn't work w/o GUI in motif version)
    TEST_EXPECT_EQUAL(AW_rgb16("white").ascii(), "#ffffffffffff");
    TEST_EXPECT_EQUAL(AW_rgb16("black").ascii(), "#000000000000");
#endif
}

void TEST_rgb_diff() {
    AW_rgb16 orange("#f80");
    AW_rgb16 blue("#004");

    AW_rgb_diff blue2orange = orange - blue;
    AW_rgb16    calc_orange = blue + blue2orange;
    TEST_EXPECT(calc_orange == orange);

    AW_rgb_diff orange2blue = -blue2orange;
    AW_rgb16    calc_blue   = orange + orange2blue;
    TEST_EXPECT(calc_blue   == blue);

    for (float part = 0.1; part<1.0; part += 0.1) {
        AW_rgb16 mix1 = orange +     part * orange2blue;
        AW_rgb16 mix2 = blue   + (1-part) * blue2orange;
        TEST_EXPECT(mix1 == mix2);
    }

    // check that color calculation does not overflow:
    AW_rgb16    black("#000");
    AW_rgb16    white("#fff");
    AW_rgb_diff black2white = white-black;

    AW_rgb16 whiter = white + black2white;
    TEST_EXPECT_EQUAL(whiter.ascii(), "#ffffffffffff");
    TEST_EXPECT(whiter == white);

    AW_rgb16 blacker = black - black2white;
    TEST_EXPECT_EQUAL(blacker.ascii(), "#000000000000");
    TEST_EXPECT(blacker == black);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
//      WM-dependent code

#if defined(ARB_GTK)

# include <gdk/gdk.h>

#else // ARB_MOTIF

#include "aw_root.hxx"
#include "aw_window.hxx"
#include "aw_window_Xm.hxx"

#endif

static AW_rgb16 convert_xcolorname(const char *xcolorname, bool& failed) {
#if defined(ARB_GTK)
    GdkColor col;
    failed = !gdk_color_parse(xcolorname, &col);
#else // ARB_MOTIF
    XColor   col, unused;
    if (AW_root::SINGLETON) { // only works if window was set (by caller)
        AW_root_Motif *mroot = AW_root::SINGLETON->prvt;
        failed = !XLookupColor(mroot->display,
                               mroot->colormap,
                               xcolorname,
                               &col,     // exact color
                               &unused); // screen-color
    }
    else {
        failed = true;
    }
#endif

    return
        failed
        ? AW_rgb16()
        : AW_rgb16(col.red, col.green, col.blue);
}

