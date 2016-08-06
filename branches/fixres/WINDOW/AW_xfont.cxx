/* FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1985 by Supoj Sutanthavibul
 * Copyright (c) 1992 by Brian V. Smith
 *
 * "Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation.
 * No representations are made about the suitability of this software for
 * any purpose.  It is provided "as is" without express or implied warranty."
 *
 * This software has been widely modified for usage inside ARB.
 */

#include "aw_xfont.hxx"
#include "aw_root.hxx"

#include <arbdb.h>

#include <cctype>

// --------------------------------------------------------------------------------

#define FONT_EXAMINE_MAX   5000
#define KNOWN_ISO_VERSIONS 3

#if !defined(DEVEL_RELEASE)
// #define DUMP_FONT_LOOKUP
// #define DUMP_FONT_DETAILS
#endif

// --------------------------------------------------------------------------------

typedef XFontStruct *PIX_FONT;

static bool openwinfonts;
static bool is_scalable[AW_NUM_FONTS];     // whether the font is scalable

// defines the preferred xfontsel-'rgstry' values (most wanted first)

#define PREFER_ISO10646

#if defined(DEVEL_RALF)
#if defined(PREFER_ISO10646)
// #warning current iso setting: prefer ISO10646 (this is recommended)
#else
#warning current iso setting: prefer ISO8859
#endif // PREFER_ISO10646
#endif // DEVEL_RALF

#if defined(PREFER_ISO10646)
static const char *known_iso_versions[KNOWN_ISO_VERSIONS] = { "ISO10646", "ISO8859", "*" };
#else
static const char *known_iso_versions[KNOWN_ISO_VERSIONS] = { "ISO8859", "ISO10646", "*" };
#endif

static struct _xfstruct x_fontinfo[] = {
    { "-adobe-times-medium-r-*--",                  "Times",         (xfont*) NULL }, // #0
    { "-adobe-times-medium-i-*--",                  "Times I",       (xfont*) NULL }, // #1
    { "-adobe-times-bold-r-*--",                    "Times B",       (xfont*) NULL }, // #2
    { "-adobe-times-bold-i-*--",                    "Times IB",      (xfont*) NULL }, // #3
    { "-schumacher-clean-medium-r-*--",             "Schumacher",    (xfont*) NULL },      // closest to Avant-Garde
    { "-schumacher-clean-medium-i-*--",             "Schumacher I",  (xfont*) NULL }, // #5
    { "-schumacher-clean-bold-r-*--",               "Schumacher B",  (xfont*) NULL }, // #6
    { "-schumacher-clean-bold-i-*--",               "Schumacher IB", (xfont*) NULL }, // #7
    { "-*-urw bookman l-medium-r-*--",              "Bookman",       (xfont*) NULL },      // closest to Bookman
    { "-*-urw bookman l-medium-i-*--",              "Bookman I",     (xfont*) NULL }, // #9
    { "-*-urw bookman l-bold-r-*--",                "Bookman B",     (xfont*) NULL }, // #10
    { "-*-urw bookman l-bold-i-*--",                "Bookman IB",    (xfont*) NULL }, // #11
    { "-adobe-courier-medium-r-*--",                "Courier",       (xfont*) NULL }, // #12
    { "-adobe-courier-medium-o-*--",                "Courier I",     (xfont*) NULL }, // #13
    { "-adobe-courier-bold-r-*--",                  "Courier B",     (xfont*) NULL }, // #14
    { "-adobe-courier-bold-o-*--",                  "Courier IB",    (xfont*) NULL }, // #15
    { "-adobe-helvetica-medium-r-*--",              "Helvetica",     (xfont*) NULL }, // #16
    { "-adobe-helvetica-medium-o-*--",              "Helvetica I",   (xfont*) NULL }, // #17
    { "-adobe-helvetica-bold-r-*--",                "Helvetica B",   (xfont*) NULL }, // #18
    { "-adobe-helvetica-bold-o-*--",                "Helvetica IB",  (xfont*) NULL }, // #19
    { "-*-liberation sans narrow-medium-r-*--",     "Liberation",    (xfont*) NULL },      // closest to Helv-nar.
    { "-*-liberation sans narrow-medium-o-*--",     "Liberation I",  (xfont*) NULL }, // #21
    { "-*-liberation sans narrow-bold-r-*--",       "Liberation B",  (xfont*) NULL }, // #22
    { "-*-liberation sans narrow-bold-o-*--",       "Liberation IB", (xfont*) NULL }, // #23
    { "-adobe-new century schoolbook-medium-r-*--", "Schoolbook",    (xfont*) NULL }, // #24
    { "-adobe-new century schoolbook-medium-i-*--", "Schoolbook I",  (xfont*) NULL }, // #25
    { "-adobe-new century schoolbook-bold-r-*--",   "Schoolbook B",  (xfont*) NULL }, // #26
    { "-adobe-new century schoolbook-bold-i-*--",   "Schoolbook IB", (xfont*) NULL }, // #27
    { "-*-lucidabright-medium-r-*--",               "LucidaBr",      (xfont*) NULL },      // closest to Palatino
    { "-*-lucidabright-medium-i-*--",               "LucidaBr I",    (xfont*) NULL }, // #29
    { "-*-lucidabright-demibold-r-*--",             "LucidaBr B",    (xfont*) NULL }, // #30
    { "-*-lucidabright-demibold-i-*--",             "LucidaBr IB",   (xfont*) NULL }, // #31
    { "-*-symbol-medium-r-*--",                     "Symbol",        (xfont*) NULL }, // #32
    { "-*-zapfchancery-medium-i-*--",               "zapfchancery",  (xfont*) NULL }, // #33
    { "-*-zapfdingbats-*-*-*--",                    "zapfdingbats",  (xfont*) NULL }, // #34

    // below here are fonts not defined in xfig!
    // on export, they will be mapped to xfig fonts
    // (according to ps_fontinfo.xfontnum, i.e. the number behind the postscript fontname below)
    
    { "-*-lucida-medium-r-*-*-",                    "Lucida",        (xfont*) NULL }, // #35
    { "-*-lucida-medium-i-*-*-",                    "Lucida I",      (xfont*) NULL }, // #36
    { "-*-lucida-bold-r-*-*-",                      "Lucida B",      (xfont*) NULL }, // #37
    { "-*-lucida-bold-i-*-*-",                      "Lucida IB",     (xfont*) NULL }, // #38
    { "-*-lucidatypewriter-medium-r-*-*-",          "Lucida mono",   (xfont*) NULL }, // #39
    { "-*-lucidatypewriter-bold-r-*-*-",            "Lucida mono B", (xfont*) NULL }, // #40

    { "-*-screen-medium-r-*-*-",                    "Screen",        (xfont*) NULL }, // #41
    { "-*-screen-bold-r-*-*-",                      "Screen B",      (xfont*) NULL }, // #42
    { "-*-clean-medium-r-*-*-",                     "Clean",         (xfont*) NULL }, // #43
    { "-*-clean-bold-r-*-*-",                       "Clean B",       (xfont*) NULL }, // #44
    { "-*-terminal-medium-r-*-*-",                  "Terminal",      (xfont*) NULL }, // #45
    { "-*-terminal-bold-r-*-*-",                    "Terminal B",    (xfont*) NULL }, // #46
    
    { "-*-dustismo-medium-r-*-*-",                  "Dustismo",      (xfont*) NULL }, // #47
    { "-*-dustismo-bold-r-*-*-",                    "Dustismo B",    (xfont*) NULL }, // #48
    { "-*-utopia-medium-r-*-*-",                    "Utopia",        (xfont*) NULL }, // #49
    { "-*-utopia-bold-r-*-*-",                      "Utopia B",      (xfont*) NULL }, // #50
    { "-*-dejavu sans-medium-r-*-*-",               "Dejavu",        (xfont*) NULL }, // #51
    { "-*-dejavu sans-bold-r-*-*-",                 "Dejavu B",      (xfont*) NULL }, // #52
    
    { "-*-fixed-medium-r-*-*-",                     "Fixed",         (xfont*) NULL }, // #53
    { "-*-fixed-bold-r-*-*-",                       "Fixed B",       (xfont*) NULL }, // #54
    { "-*-dejavu sans mono-medium-r-*-*-",          "Dejavu mono",   (xfont*) NULL }, // #55
    { "-*-dejavu sans mono-bold-r-*-*-",            "Dejavu mono B", (xfont*) NULL }, // #56
    { "-*-luxi mono-medium-r-*-*-",                 "Luxi mono",     (xfont*) NULL }, // #57
    { "-*-luxi mono-bold-r-*-*-",                   "Luxi mono B",   (xfont*) NULL }, // #58
    { "-*-nimbus mono l-medium-r-*-*-",             "Nimbus mono",   (xfont*) NULL }, // #59
    { "-*-nimbus mono l-bold-r-*-*-",               "Nimbus mono B", (xfont*) NULL }, // #60
    { "-*-latin modern typewriter-medium-r-*-*-",   "Latin mono",    (xfont*) NULL }, // #61
    { "-*-latin modern typewriter-bold-r-*-*-",     "Latin mono B",  (xfont*) NULL }, // #62
};

static struct _fstruct ps_fontinfo[] = {
    // map window fonts to postscript fonts
    // negative values indicate monospaced fonts
    { "Default",                      -1 },
    { "Times-Roman",                  0 },
    { "Times-Italic",                 1 },
    { "Times-Bold",                   2 },
    { "Times-BoldItalic",             3 },
    { "AvantGarde-Book",              4 },
    { "AvantGarde-BookOblique",       5 },
    { "AvantGarde-Demi",              6 },
    { "AvantGarde-DemiOblique",       7 },
    { "Bookman-Light",                8 },
    { "Bookman-LightItalic",          9 },
    { "Bookman-Demi",                 10 },
    { "Bookman-DemiItalic",           11 },
    { "Courier",                      -12 },
    { "Courier-Oblique",              -13 },
    { "Courier-Bold",                 -14 },
    { "Courier-BoldOblique",          -15 },
    { "Helvetica",                    16 },
    { "Helvetica-Oblique",            17 },
    { "Helvetica-Bold",               18 },
    { "Helvetica-BoldOblique",        19 },
    { "Helvetica-Narrow",             20 },
    { "Helvetica-Narrow-Oblique",     21 },
    { "Helvetica-Narrow-Bold",        22 },
    { "Helvetica-Narrow-BoldOblique", 23 },
    { "NewCenturySchlbk-Roman",       24 },
    { "NewCenturySchlbk-Italic",      25 },
    { "NewCenturySchlbk-Bold",        26 },
    { "NewCenturySchlbk-BoldItalic",  27 },
    { "Palatino-Roman",               28 },
    { "Palatino-Italic",              29 },
    { "Palatino-Bold",                30 },
    { "Palatino-BoldItalic",          31 },
    { "Symbol",                       32 },
    { "ZapfChancery-MediumItalic",    33 },
    { "ZapfDingbats",                 34 },

    // non xfig-fonts below.
    // Need to be mapped to best matching xfig font using a font number between 0 and AW_NUM_FONTS_XFIG-1

    { "LucidaSans",                   16 },
    { "LucidaSans-Italic",            17 },
    { "LucidaSans-Bold",              18 },
    { "LucidaSans-BoldItalic",        19 },
    { "LucidaSansTypewriter",         -12 },
    { "LucidaSansTypewriter-Bold",    -14 },

    { "Screen",                       -16 },
    { "Screen-Bold",                  -18 },
    { "Clean",                        -12 },
    { "Clean-Bold",                   -14 },
    { "Terminal",                     -12 },
    { "Terminal-Bold",                -14 },
    
    { "AvantGarde-Book",              4 },
    { "AvantGarde-Demi",              6 },
    { "Palatino-Roman",               28 },
    { "Palatino-Bold",                30 },
    { "AvantGarde-Book",              4 },
    { "AvantGarde-Demi",              6 },
    
    { "Courier",                      -12 },
    { "Courier-Bold",                 -14 },
    { "Courier",                      -12 },
    { "Courier-Bold",                 -14 },
    { "Courier",                      -12 },
    { "Courier-Bold",                 -14 },
    { "Courier",                      -12 },
    { "Courier-Bold",                 -14 },
    { "Courier",                      -12 },
    { "Courier-Bold",                 -14 },
};

STATIC_ASSERT(ARRAY_ELEMS(x_fontinfo) == AW_NUM_FONTS);
STATIC_ASSERT(ARRAY_ELEMS(ps_fontinfo) == AW_NUM_FONTS+1);

#if defined(ASSERTION_USED)
static void check_ps_fontinfo_valid() {
    // check all fonts do translate into valid xfig fonts:
    for (int f = 0; f<AW_NUM_FONTS; ++f) {
        int xfig_fontnr                = AW_font_2_xfig(f);
        if (xfig_fontnr<0) xfig_fontnr = -xfig_fontnr;
        aw_assert(xfig_fontnr >= 0);
        aw_assert(xfig_fontnr < AW_NUM_FONTS_XFIG);
    }

    // check for unique font-specs and unique font-shortnames:
    for (int f1 = 0; f1<AW_NUM_FONTS; ++f1) {
        const _xfstruct& x1 = x_fontinfo[f1];
        for (int f2 = f1+1; f2<AW_NUM_FONTS; ++f2) {
            const _xfstruct& x2 = x_fontinfo[f2];

            aw_assert(strcmp(x1.templat,     x2.templat)     != 0);
            aw_assert(strcmp(x1.description, x2.description) != 0);
        }
    }
}
#endif


#define FONT_STRING_PARTS 14

static const char *parseFontString(const char *fontname, int *minus_position) {
    const char *error = 0;
    const char *minus = strchr(fontname, '-');
    int         count = 0;

    for (; minus; minus = strchr(minus+1, '-'), ++count) {
        if (count >= FONT_STRING_PARTS) { error = "too many '-'"; break; }
        minus_position[count] = minus-fontname;
    }
    if (count != FONT_STRING_PARTS) error = "expected 14 '-'";

    return error;
}
static char *getParsedFontPart(const char *fontname, int *minus_position, int idx) {
    aw_assert(idx >= 0 && idx<FONT_STRING_PARTS);

    int   startpos = minus_position[idx]+1; // behind minus
    int   endpos   = (idx == (FONT_STRING_PARTS-1) ? strlen(fontname) : minus_position[idx+1])-1; // in front of minus/string-end
    int   length   = endpos-startpos+1; // excluding minus
    char *result   = new char[length+1];

    memcpy(result, fontname+startpos, length);
    result[length] = 0;

    return result;
}

// parse the point size of font 'name'
// e.g. -adobe-courier-bold-o-normal--10-100-75-75-m-60-ISO8859-1

static int parsesize(const char *fontname) {
    int         pos[14];
    int         size;
    const char *error = parseFontString(fontname, pos);

    if (!error) {
        char *sizeString = getParsedFontPart(fontname, pos, 6);
        size             = atoi(sizeString);
        if (size == 0 && strcmp(sizeString, "0") != 0) {
            error = GBS_global_string("Can't parse size (from '%s')", sizeString);
        }
        delete [] sizeString;
    }

    if (error) {
        fprintf(stderr, "Error parsing size info from '%s' (%s)\n", fontname, error);
        return 0;
    }

    return size;
}

void aw_root_init_font(Display *display) {
    static bool initialized = false;
    if (initialized) return;

    initialized = true;

#if defined(ASSERTION_USED)
    check_ps_fontinfo_valid();
#endif

    /* Now initialize the font structure for the X fonts corresponding to the
     * Postscript fonts for the canvas.  OpenWindows can use any LaserWriter
     * fonts at any size, so we don't need to load anything if we are using
     * it.
     */

    // check for OpenWindow-style fonts, otherwise check for scalable font
    openwinfonts = false;
    {
        char **fontlist;
        int    count;

        // first look for OpenWindow style font names (e.g. times-roman)
        if ((fontlist = XListFonts(display, ps_fontinfo[1].name, 1, &count))!=0) {
            openwinfonts = true;
            for (int f = 0; f<AW_NUM_FONTS; f++) { // copy the OpenWindow font names ( = postscript fontnames?)
                x_fontinfo[f].templat = ps_fontinfo[f+1].name;
                is_scalable[f]        = true;
#if defined(DUMP_FONT_LOOKUP)
                printf("ps_fontinfo[f+1].name='%s'\n", ps_fontinfo[f+1].name);
#endif // DUMP_FONT_LOOKUP
            }
            XFreeFontNames(fontlist);
#if defined(DUMP_FONT_LOOKUP)
            printf("Using OpenWindow fonts\n");
#endif
        }
        else {
            for (int f = 0; f<AW_NUM_FONTS; f++) {
                char templat[200];
                strcpy(templat, x_fontinfo[f].templat);
                strcat(templat, "0-0-*-*-*-*-*-*");

                if ((fontlist = XListFonts(display, templat, 1, &count)) != 0) {
                    // font with size zero exists -> this font is scalable
                    is_scalable[f] = true;
                    XFreeFontNames(fontlist);
                }
                else {
                    is_scalable[f] = false;
                }
#if defined(DUMP_FONT_LOOKUP)
                printf("Using %s font for '%s'\n", is_scalable[f] ? "scalable" : "fixed", x_fontinfo[f].templat);
#endif // DUMP_FONT_LOOKUP
            }
        }
    }

    struct found_font { const char *fn; int s; };

    if (!openwinfonts) {
        found_font *flist = new found_font[FONT_EXAMINE_MAX];

        for (int f = 0; f<AW_NUM_FONTS; f++) {
            if (is_scalable[f]) continue;

            // for all non-scalable fonts:
            // query the server for all the font names and sizes and build a list of them
            
            char **fontlist[KNOWN_ISO_VERSIONS];
            int    iso;
            int    found_fonts = 0;

            for (iso = 0; iso<KNOWN_ISO_VERSIONS; ++iso) fontlist[iso] = 0;

            for (iso = 0; iso<KNOWN_ISO_VERSIONS; ++iso) {
                char *font_template = GBS_global_string_copy("%s*-*-*-*-*-*-%s-*", x_fontinfo[f].templat, known_iso_versions[iso]);
                int   count;

                fontlist[iso] = XListFonts(display, font_template, FONT_EXAMINE_MAX, &count);
                if (fontlist[iso]) {
                    if ((found_fonts+count) >= FONT_EXAMINE_MAX) {
                        printf("Warning: Too many fonts found for '%s..%s' - ARB can't examine all fonts\n", x_fontinfo[f].templat, known_iso_versions[iso]);
                        count = FONT_EXAMINE_MAX-found_fonts;
                    }
                    for (int c = 0; c<count; ++c) {
                        const char *fontname  = fontlist[iso][c];
                        int         size      = parsesize(fontname);
                        flist[found_fonts].fn = fontname; // valid as long fontlist[iso] is not freed!
                        flist[found_fonts].s  = size;
                        found_fonts++;
                    }
                }

                free(font_template);
            }

#if defined(DUMP_FONT_LOOKUP)
            printf("Considering %i fonts for '%s'\n", found_fonts, x_fontinfo[f].templat);
#endif // DUMP_FONT_LOOKUP

            aw_assert(found_fonts <= FONT_EXAMINE_MAX);
            xfont *nf = NULL;

            for (int size = MIN_FONTSIZE; size <= MAX_FONTSIZE; size++) { // scan all useful sizes
                int i;
                for (i = 0; i < found_fonts; i++) {
                    if (flist[i].s == size) break; // search first font matching current size
                }

                if (i < found_fonts && flist[i].s == size) {
                    xfont *newfont = (xfont *)ARB_alloc(sizeof(xfont));

                    (nf ? nf->next : x_fontinfo[f].xfontlist) = newfont;
                    nf                                        = newfont;

                    // store size and actual fontname :
                    nf->size    = size;
                    nf->fname   = strdup(flist[i].fn);
                    nf->fstruct = NULL;
                    nf->next    = NULL;
                }
            }

            if (!nf) { // no font has been found -> fallback to "fixed 12pt"
                aw_assert(x_fontinfo[f].xfontlist == 0);
                xfont *newfont   = (xfont *)ARB_alloc(sizeof(xfont));
                x_fontinfo[f].xfontlist = newfont;

                newfont->size    = DEF_FONTSIZE;
                newfont->fname   = strdup(NORMAL_FONT);
                newfont->fstruct = NULL;
                newfont->next    = NULL;
            }

#if defined(DUMP_FONT_LOOKUP)
            nf = x_fontinfo[f].xfontlist;
            printf("Fonts used for '%s':\n", x_fontinfo[f].templat);
            while (nf) {
                printf("- %2i pt: '%s'\n", nf->size, nf->fname);
                nf = nf->next;
            }
#endif // DUMP_FONT_LOOKUP

            for (iso = 0; iso<KNOWN_ISO_VERSIONS; ++iso) XFreeFontNames(fontlist[iso]);
        }

        delete [] flist;
    }
}

#if defined(DUMP_FONT_DETAILS)
static void dumpFontInformation(xfont *xf) {
    printf("Font information for '%s'", xf->fname);
    XFontStruct *xfs = xf->fstruct;
    if (xfs) {
        printf(":\n");
        printf("- max letter ascent  = %2i\n", xfs->max_bounds.ascent);
        printf("- max letter descent = %2i\n", xfs->max_bounds.descent);
        printf("- max letter height  = %2i\n", xfs->max_bounds.ascent + xfs->max_bounds.descent);
        printf("- max letter width   = %2i\n", xfs->max_bounds.width);
    }
    else {
        printf(" (not available, font is not loaded)\n");
    }
}
#endif // DEBUG

/* Lookup an X font, "f" corresponding to a Postscript font style that is
 * close in size to "s"
 */

#define DEFAULT (-1)

static bool lookfont(Display *tool_d, int f, int s, int& found_size, bool verboose, bool only_query, PIX_FONT *fontstPtr)
// returns true if appropriate font is available.
//
// 'found_size' is set to the actually found size, which may be bigger or smaller than 's', if the requested size is not available
//
// if 'only_query' is true, then only report availability
// if 'only_query' is false, then actually load the font and store the loaded fontstruct in 'fontstPtr'
{
    bool   found;
    xfont *newfont, *nf, *oldnf;

#if defined(WARN_TODO)
#warning scalability shall be checked for each font -- not only for first
#warning fontdetection is called for each GC -- do some caching ?
#endif

    found_size = -1;

    if (f == DEFAULT)
        f = 0;          // pass back the -normal font font
    if (s < 0)
        s = DEF_FONTSIZE;       // default font size

    /* see if we've already loaded that font size 's'
       from the font family 'f' */

    found = false;

    /* start with the basic font name (e.g. adobe-times-medium-r-normal-...
       OR times-roman for OpenWindows fonts) */

    nf = x_fontinfo[f].xfontlist;
    if (!nf) nf = x_fontinfo[0].xfontlist;
    oldnf = nf;
    if (nf != NULL) {
        if (nf->size > s && !is_scalable[f]) {
            found = true;
        }
        else {
            while (nf != NULL) {
                if (nf->size == s ||
                    (!is_scalable[f] && (nf->size >= s && oldnf->size <= s)))
                {
                    found = true;
                    break;
                }
                oldnf = nf;
                nf = nf->next;
            }
        }
    }
    if (found) {                // found exact size (or only larger available)
        if (verboose) {
            if (s < nf->size) fprintf(stderr, "Font size %d not found, using larger %d point\n", s, nf->size);
#if defined(DUMP_FONT_LOOKUP)
            fprintf(stderr, "Detected font %s\n", nf->fname);
#endif
        }
    }
    else if (!is_scalable[f]) { // not found, use largest available
        nf = oldnf;
        if (verboose) {
            if (s > nf->size) fprintf(stderr, "Font size %d not found, using smaller %d point\n", s, nf->size);
#if defined(DUMP_FONT_LOOKUP)
            fprintf(stderr, "Using font %s for size %d\n", nf->fname, s);
#endif
        }
    }
    else { // SCALABLE; none yet of that size, alloc one and put it in the list
        newfont = (xfont *)ARB_alloc(sizeof(xfont));
        // add it on to the end of the list

        nf = oldnf ? oldnf->next : 0; // store successor

        if (x_fontinfo[f].xfontlist == NULL) x_fontinfo[f].xfontlist = newfont;
        else oldnf->next                                             = newfont;

        newfont->next    = nf; // old successor in fontlist
        newfont->size    = s;
        newfont->fstruct = NULL;
        newfont->fname   = NULL;

        nf = newfont;

        if (openwinfonts) {
            // OpenWindows fonts, create font name like times-roman-13

            nf->fname = GBS_global_string_copy("%s-%d", x_fontinfo[f].templat, s);
        }
        else {
            // X11 fonts, create a full XLFD font name
            // attach pointsize to font name, use the pixel field instead of points in the fontname so that the
            // font scales with screen size

            for (int iso = 0; iso<KNOWN_ISO_VERSIONS; ++iso) {
                char *fontname = GBS_global_string_copy("%s%d-*-*-*-*-*-%s-*", x_fontinfo[f].templat, s, known_iso_versions[iso]);
#if defined(DUMP_FONT_LOOKUP)
                fprintf(stderr, "Checking for '%s' (x_fontinfo[%i].templat='%s')\n", fontname, f, x_fontinfo[f].templat);
#endif

                int    matching_fonts_found;
                char **matching_fonts = XListFonts(tool_d, fontname, 1, &matching_fonts_found);

                if (matching_fonts) {
                    XFreeFontNames(matching_fonts);
                    aw_assert(matching_fonts_found >= 1);
                    nf->fname = fontname;
                    break;
                }

                free(fontname);
            }
            // @@@ what if nf->fstruct is 0 now ?
        }
        // aw_assert(nf->fname); // fails e.g. for screen-medium-r, but font nevertheless seems to be usable
    } // scalable

    bool font_found = true;

    if (nf->fstruct == NULL) {
        if (only_query) {
            ; // assume its available (or use XQueryFont to reduce server-client bandwidth)
        }
        else {
#if defined(DUMP_FONT_LOOKUP)
            if (verboose) fprintf(stderr, "Loading font '%s'\n", nf->fname);
#endif
            PIX_FONT fontst = XLoadQueryFont(tool_d, nf->fname);
            if (fontst == NULL) {
                fprintf(stderr, "ARB fontpackage: Unexpectedly couldn't load font '%s', falling back to '%s' (f=%i, s=%i)\n", nf->fname, NORMAL_FONT, f, s);
                fontst = XLoadQueryFont(tool_d, NORMAL_FONT); // @@@ may return 0!
                freedup(nf->fname, NORMAL_FONT);    // store actual name
            }
            // put the structure in the list
            nf->fstruct = fontst;
        }
    }

#if defined(DUMP_FONT_DETAILS)
    dumpFontInformation(nf);
#endif // DEBUG

    found_size = nf->size;      // report used size
    *fontstPtr = nf->fstruct;

    return font_found;
}

static int get_available_fontsizes(Display *tool_d, int f, int *available_sizes) {
    // returns number of available sizes
    // specific sizes are stored in available_sizes[]

    int size_count = 0;
    for (int size = MAX_FONTSIZE; size >= MIN_FONTSIZE; --size) {
        int      found_size;
        PIX_FONT fontst;

        ASSERT_TRUE(lookfont(tool_d, f, size, found_size, false, true, &fontst)); // lookfont should do fallback 

        if (found_size<size) size = found_size;
        if (found_size == size) available_sizes[size_count++] = size;
    }

    // reverse order of found fontsizes
    if (size_count>1) {
        for (int reverse = size_count/2-1; reverse >= 0; --reverse) {
            int o = size_count-1-reverse;
            aw_assert(o >= 0 && o<size_count);

            int s                    = available_sizes[reverse];
            available_sizes[reverse] = available_sizes[o];
            available_sizes[o]       = s;
        }
    }

    return size_count;
}

static char *caps(char *sentence) {
    bool doCaps = true;
    for (int i = 0; sentence[i]; ++i) {
        if (isalpha(sentence[i])) {
            if (doCaps) {
                sentence[i] = toupper(sentence[i]);
                doCaps      = false;
            }
        }
        else {
            doCaps = true;
        }
    }
    return sentence;
}


const char *AW_get_font_specification(AW_font font_nr) {
    //! converts fontnr to string
    //
    // @return 0 if font is not available

    aw_assert(font_nr >= 0);
    if (font_nr<0 || font_nr>=AW_NUM_FONTS) return 0;

    const char        *readable_fontname = 0;
    struct _xfstruct&  xf                = x_fontinfo[font_nr];

    if (xf.xfontlist) {
        const char *fontname = xf.xfontlist->fname;

        if (strcmp(fontname, "fixed") == 0) {
            readable_fontname = GBS_global_string("[not found: %s]", xf.templat);
        }
        else {
            int         pos[14];
            const char *error = parseFontString(fontname, pos);
            if (error) {
                readable_fontname = GBS_global_string("[%s - parse-error (%s)]", fontname, error);
            }
            else {
                char *fndry  = caps(getParsedFontPart(fontname, pos, 0));
                char *fmly   = caps(getParsedFontPart(fontname, pos, 1));
                char *wght   = getParsedFontPart(fontname, pos, 2); wght[3] = 0;
                char *slant  = getParsedFontPart(fontname, pos, 3);
                char *rgstry = getParsedFontPart(fontname, pos, 12);

                readable_fontname = GBS_global_string("%s %s %s,%s,%s",
                                                      fndry, fmly,
                                                      wght, slant,
                                                      rgstry);
                delete [] rgstry;
                delete [] slant;
                delete [] wght;
                delete [] fmly;
                delete [] fndry;
            }
        }
    }
    else {
        readable_fontname = xf.templat;
    }
    return readable_fontname;
}

const char *AW_get_font_shortname(AW_font font_nr) {
    aw_assert(font_nr>=0 && font_nr<AW_NUM_FONTS);
    return x_fontinfo[font_nr].description;
}

int AW_font_2_xfig(AW_font font_nr) {
    //! converts fontnr to xfigid
    //
    // negative values indicate monospaced f.

    if (font_nr<0 || font_nr>=AW_NUM_FONTS) return 0;
    return (ps_fontinfo[font_nr+1].xfontnum);
}

inline bool CI_NonExistChar(const XCharStruct *cs) {
    return
        cs->width                                          == 0 &&
        (cs->rbearing|cs->lbearing|cs->ascent|cs->descent) == 0;
}

inline void CI_GetCharInfo_1D(const XFontStruct *fs, unsigned col, const XCharStruct *def, const XCharStruct*& cs)
{
    cs = def;
    aw_assert(fs->min_byte1 == 0 && fs->max_byte1 == 0); // otherwise CI_GetCharInfo_1D is not the appropriate function
    if (col >= fs->min_char_or_byte2 && col <= fs->max_char_or_byte2) {
        if (fs->per_char == NULL) {
            cs = &fs->min_bounds;
        }
        else {
            cs = &fs->per_char[col - fs->min_char_or_byte2];
            if (CI_NonExistChar(cs)) cs = def;
        }
    }
}

inline void CI_GetDefaultInfo_1D(const XFontStruct *fs, const XCharStruct*& cs) {
    CI_GetCharInfo_1D(fs, fs->default_char, NULL, cs);
}

/* CI_GET_CHAR_INFO_2D - return the charinfo struct for the indicated row and
 * column.  This is used for fonts that have more than row zero.
 */

inline void CI_GetCharInfo_2D(const XFontStruct *fs, unsigned row, unsigned col, const XCharStruct *def, const XCharStruct*& cs)
{
    cs = def;
    if (row >= fs->min_byte1 && row <= fs->max_byte1 &&
        col >= fs->min_char_or_byte2 && col <= fs->max_char_or_byte2)
    {
        if (fs->per_char == NULL) {
            cs = &fs->min_bounds;
        }
        else {
            cs = &fs->per_char[((row - fs->min_byte1) *
                                (fs->max_char_or_byte2 -
                                 fs->min_char_or_byte2 + 1)) +
                               (col - fs->min_char_or_byte2)];
            if (CI_NonExistChar(cs)) cs = def;
        }
    }
}

inline void CI_GetDefaultInfo_2D(const XFontStruct *fs, const XCharStruct*& cs)
{
    unsigned int r = (fs->default_char >> 8);
    unsigned int c = (fs->default_char & 0xff);
    CI_GetCharInfo_2D (fs, r, c, NULL, cs);
}

/* CI_GetRowzeroCharInfo_2D - do the same thing as CI_GetCharInfo_1D,
 * except that the font has more than one row.  This is special case of more
 * general version used in XTextExt16.c since row == 0.  This is used when
 * max_byte2 is not zero.  A further optimization would do the check for
 * min_byte1 being zero ahead of time.
 */

inline void CI_GetRowzeroCharInfo_2D(const XFontStruct *fs, unsigned col, const XCharStruct *def, const XCharStruct*& cs)
{
    cs = def;
    if (fs->min_byte1 == 0 &&
        col >= fs->min_char_or_byte2 && col <= fs->max_char_or_byte2) {
        if (fs->per_char == NULL) {
            cs = &fs->min_bounds;
        }
        else {
            cs = &fs->per_char[(col - fs->min_char_or_byte2)];
            if (CI_NonExistChar(cs)) cs = def;
        }
    }
}

void AW_GC_Xm::wm_set_font(const AW_font font_nr, const int size, int *found_size) {
    // if found_size != 0 -> return value for used font size
    XFontStruct *xfs;
    {
        int  found_font_size;
        ASSERT_TRUE(lookfont(get_common()->get_display(), font_nr, size, found_font_size, true, false, &xfs)); // lookfont should do fallback
        if (found_size) *found_size = found_font_size;
    }
    XSetFont(get_common()->get_display(), gc, xfs->fid);
    curfont = *xfs;

    const XCharStruct *cs;
    const XCharStruct *def;     // info about default char
    bool               singlerow = (xfs->max_byte1 == 0); // optimization

    if (singlerow) {    // optimization
        CI_GetDefaultInfo_1D(xfs, def);
    }
    else {
        CI_GetDefaultInfo_2D(xfs, def);
    }

    aw_assert(AW_FONTINFO_CHAR_ASCII_MIN < AW_FONTINFO_CHAR_ASCII_MAX);

    unsigned int i;
    for (i = AW_FONTINFO_CHAR_ASCII_MIN; i <= AW_FONTINFO_CHAR_ASCII_MAX; i++) {
        if (singlerow) CI_GetCharInfo_1D(xfs, i, def, cs); // optimization 
        else           CI_GetRowzeroCharInfo_2D(xfs, i, def, cs);
        if (cs) set_char_size(i, cs->ascent, cs->descent, cs->width);
        else    set_no_char_size(i);
    }
}

void AW_GC::set_font(const AW_font font_nr, const int size, int *found_size) {
    font_limits.reset();
    wm_set_font(font_nr, size, found_size);
    font_limits.calc_height();
    fontnr   = font_nr;
    fontsize = size;
}

int AW_GC_Xm::get_available_fontsizes(AW_font font_nr, int *available_sizes) const {
    return ::get_available_fontsizes(get_common()->get_display(), font_nr, available_sizes);
}
