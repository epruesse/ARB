/*
 * FIG : Facility for Interactive Generation of figures
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <X11/Xlib.h>

#include <arbdb.h>
#include <aw_root.hxx>
#include "aw_device.hxx"
#include "aw_commn.hxx"
#include "aw_xfont.hxx"

// --------------------------------------------------------------------------------

#define FONT_EXAMINE_MAX   500
#define KNOWN_ISO_VERSIONS 3

#if defined(DEVEL_RALF)
#warning font debugging is active in release
#endif // DEVEL_RALF
#define DUMP_FONT_LOOKUP
// #define DUMP_FONT_DETAILS

// --------------------------------------------------------------------------------


static AW_BOOL openwinfonts;

typedef XFontStruct *PIX_FONT;
appresStruct appres = {
    AW_TRUE,
    AW_FALSE,
    0,
};

// defines the preferred xfontsel-'rgstry' values (most wanted first)

#define PREFER_ISO10646

#if defined(DEVEL_RALF)
#if defined(PREFER_ISO10646)
#warning current iso setting: prefer ISO10646
#else
#warning current iso setting: prefer ISO8859
#endif // PREFER_ISO10646
#endif // DEVEL_RALF

#if defined(PREFER_ISO10646)
static const char *known_iso_versions[KNOWN_ISO_VERSIONS] = { "ISO10646", "ISO8859", "*" };
#else
static const char *known_iso_versions[KNOWN_ISO_VERSIONS] = { "ISO8859", "ISO10646", "*" };
#endif

struct _xfstruct x_fontinfo[AW_NUM_FONTS] = {
    {"-adobe-times-medium-r-*--",                  (struct xfont*) NULL}, // #0
    {"-adobe-times-medium-i-*--",                  (struct xfont*) NULL}, // #1
    {"-adobe-times-bold-r-*--",                    (struct xfont*) NULL}, // #2
    {"-adobe-times-bold-i-*--",                    (struct xfont*) NULL}, // #3
    {"-schumacher-clean-medium-r-*--",             (struct xfont*) NULL},	/* closest to Avant-Garde */
    {"-schumacher-clean-medium-i-*--",             (struct xfont*) NULL}, // #5
    {"-schumacher-clean-bold-r-*--",               (struct xfont*) NULL}, // #6
    {"-schumacher-clean-bold-i-*--",               (struct xfont*) NULL}, // #7
    {"-adobe-times-medium-r-*--",                  (struct xfont*) NULL},	/* closest to Bookman */
    {"-adobe-times-medium-i-*--",                  (struct xfont*) NULL}, // #9
    {"-adobe-times-bold-r-*--",                    (struct xfont*) NULL}, // #10
    {"-adobe-times-bold-i-*--",                    (struct xfont*) NULL}, // #11
    {"-adobe-courier-medium-r-*--",                (struct xfont*) NULL}, // #12
    {"-adobe-courier-medium-o-*--",                (struct xfont*) NULL}, // #13
    {"-adobe-courier-bold-r-*--",                  (struct xfont*) NULL}, // #14
    {"-adobe-courier-bold-o-*--",                  (struct xfont*) NULL}, // #15
    {"-adobe-helvetica-medium-r-*--",              (struct xfont*) NULL}, // #16
    {"-adobe-helvetica-medium-o-*--",              (struct xfont*) NULL}, // #17
    {"-adobe-helvetica-bold-r-*--",                (struct xfont*) NULL}, // #18
    {"-adobe-helvetica-bold-o-*--",                (struct xfont*) NULL}, // #19
    {"-adobe-helvetica-medium-r-*--",              (struct xfont*) NULL},	/* closest to Helv-nar. */
    {"-adobe-helvetica-medium-o-*--",              (struct xfont*) NULL}, // #21
    {"-adobe-helvetica-bold-r-*--",                (struct xfont*) NULL}, // #22
    {"-adobe-helvetica-bold-o-*--",                (struct xfont*) NULL}, // #23
    {"-adobe-new century schoolbook-medium-r-*--", (struct xfont*) NULL}, // #24
    {"-adobe-new century schoolbook-medium-i-*--", (struct xfont*) NULL}, // #25
    {"-adobe-new century schoolbook-bold-r-*--",   (struct xfont*) NULL}, // #26
    {"-adobe-new century schoolbook-bold-i-*--",   (struct xfont*) NULL}, // #27
    {"-*-lucidabright-medium-r-*--",               (struct xfont*) NULL},	/* closest to Palatino */
    {"-*-lucidabright-medium-i-*--",               (struct xfont*) NULL}, // #29
    {"-*-lucidabright-demibold-r-*--",             (struct xfont*) NULL}, // #30
    {"-*-lucidabright-demibold-i-*--",             (struct xfont*) NULL}, // #31
    {"-*-symbol-medium-r-*--",                     (struct xfont*) NULL}, // #32
    {"-*-zapfchancery-medium-i-*--",               (struct xfont*) NULL}, // #33
    {"-*-zapfdingbats-*-*-*--",                    (struct xfont*) NULL}, // #34
    // non xfig fonts, will be mapped to xfig fonts on export
    {"-*-lucida-medium-r-*-*-",                    (struct xfont*) NULL}, // #35
    {"-*-lucida-medium-i-*-*-",                    (struct xfont*) NULL}, // #36
    {"-*-lucida-bold-r-*-*-",                      (struct xfont*) NULL}, // #37
    {"-*-lucida-bold-i-*-*-",                      (struct xfont*) NULL}, // #38
    {"-*-lucidatypewriter-medium-r-*-*-",          (struct xfont*) NULL}, // #39
    {"-*-lucidatypewriter-bold-r-*-*-",            (struct xfont*) NULL}, // #40
    {"-*-screen-medium-r-*-*-",                    (struct xfont*) NULL}, // #41
    {"-*-screen-bold-r-*-*-",                      (struct xfont*) NULL}, // #42
    {"-*-clean-medium-r-*-*-",                     (struct xfont*) NULL}, // #43
    {"-*-clean-bold-r-*-*-",                       (struct xfont*) NULL}, // #44
    {"-*-terminal-medium-r-*-*-",                  (struct xfont*) NULL}, // #45
    {"-*-terminal-bold-r-*-*-",                    (struct xfont*) NULL}, // #46
};

struct _fstruct ps_fontinfo[AW_NUM_FONTS + 1] = {
                                // map window fonts to postscript fonts
                                // negative values indicate monospaced fonts
    {"Default",                         -1},
    {"Times-Roman",                     0},
    {"Times-Italic",                    1},
    {"Times-Bold",                      2},
    {"Times-BoldItalic",                3},
    {"AvantGarde-Book",                 4},
    {"AvantGarde-BookOblique",          5},
    {"AvantGarde-Demi",                 6},
    {"AvantGarde-DemiOblique",          7},
    {"Bookman-Light",                   8},
    {"Bookman-LightItalic",             9},
    {"Bookman-Demi",                    10},
    {"Bookman-DemiItalic",              11},
    {"Courier",                         -12},
    {"Courier-Oblique",                 -13},
    {"Courier-Bold",                    -14},
    {"Courier-BoldOblique",             -15},
    {"Helvetica",                       16},
    {"Helvetica-Oblique",               17},
    {"Helvetica-Bold",                  18},
    {"Helvetica-BoldOblique",           19},
    {"Helvetica-Narrow",                20},
    {"Helvetica-Narrow-Oblique",        21},
    {"Helvetica-Narrow-Bold",           22},
    {"Helvetica-Narrow-BoldOblique",    23},
    {"NewCenturySchlbk-Roman",          24},
    {"NewCenturySchlbk-Italic",         25},
    {"NewCenturySchlbk-Bold",           26},
    {"NewCenturySchlbk-BoldItalic",     27},
    {"Palatino-Roman",                  28},
    {"Palatino-Italic",                 29},
    {"Palatino-Bold",                   30},
    {"Palatino-BoldItalic",             31},
    {"Symbol",                          32},
    {"ZapfChancery-MediumItalic",       33},
    {"ZapfDingbats",                    34},
    {"LucidaSans",                      16},
    {"LucidaSans-Italic",               17},
    {"LucidaSans-Bold",                 18},
    {"LucidaSans-BoldItalic",           19},
    {"LucidaSansTypewriter",            -12},
    {"LucidaSansTypewriter-Bold",       -14},
    {"Screen",                          -16},
    {"Screen-Bold",                     -18},
    {"Clean",                           -12},
    {"Clean-Bold",                      -14},
    {"Terminal",                        -12},
    {"Terminal-Bold",                   -14},
};

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

/* parse the point size of font 'name' */
/* e.g. -adobe-courier-bold-o-normal--10-100-75-75-m-60-ISO8859-1 */

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

void aw_root_init_font(Display *tool_d)
{
    if (appres.display) return;

    appres.display = tool_d;
#if defined(DUMP_FONT_LOOKUP)
    appres.debug   = AW_TRUE;
#endif // DUMP_FONT_LOOKUP

    /*
     * Now initialize the font structure for the X fonts corresponding to the
     * Postscript fonts for the canvas.	 OpenWindows can use any LaserWriter
     * fonts at any size, so we don't need to load anything if we are using
     * it.
     */

    /* if the user hasn't disallowed off scalable fonts, check that the
       server really has them by checking for font of 0-0 size */
    openwinfonts = AW_FALSE;
    if (appres.SCALABLEFONTS) {
        char **fontlist;
        int    count;

#if defined(DUMP_FONT_LOOKUP)
	printf("Searching for SCALABLEFONTS\n");
#endif // DUMP_FONT_LOOKUP

        /* first look for OpenWindow style font names (e.g. times-roman) */
        if ((fontlist = XListFonts(tool_d, ps_fontinfo[1].name, 1, &count))!=0) {
            openwinfonts = AW_TRUE;        /* yes, use them */
            for (int f=0; f<AW_NUM_FONTS; f++) {     /* copy the OpenWindow font names */
                x_fontinfo[f].templat = ps_fontinfo[f+1].name;
#if defined(DUMP_FONT_LOOKUP)
                printf("ps_fontinfo[f+1].name='%s'\n",ps_fontinfo[f+1].name);
#endif // DUMP_FONT_LOOKUP
            }
            XFreeFontNames(fontlist);
        } else {
            char templat[200];
            strcpy(templat,x_fontinfo[0].templat); /* nope, check for font size 0 */
            strcat(templat,"0-0-*-*-*-*-*-*");
            if ((fontlist = XListFonts(tool_d, templat, 1, &count))!=0){
#if defined(DUMP_FONT_LOOKUP)
        	printf("Using SCALABLEFONTS!\n");
#endif // DUMP_FONT_LOOKUP
                XFreeFontNames(fontlist);
            }else{
#if defined(DUMP_FONT_LOOKUP)
                printf("Not using SCALABLEFONTS!\n");
#endif // DUMP_FONT_LOOKUP
                appres.SCALABLEFONTS = AW_FALSE;   /* none, turn off request for them */
            }
        }
    }

    /* no scalable fonts - query the server for all the font
       names and sizes and build a list of them */

    struct found_font { const char *fn; int s; };

    if (!appres.SCALABLEFONTS) {
        found_font *flist = new found_font[FONT_EXAMINE_MAX];

        for (int f = 0; f < AW_NUM_FONTS; f++) {
            char **fontlist[KNOWN_ISO_VERSIONS];
            int    iso;
            int    found_fonts      = 0;

            for (iso = 0; iso<KNOWN_ISO_VERSIONS; ++iso) fontlist[iso] = 0;

            for (iso = 0; iso<KNOWN_ISO_VERSIONS; ++iso) {
                char *font_template = GBS_global_string_copy("%s*-*-*-*-*-*-%s-*", x_fontinfo[f].templat, known_iso_versions[iso]);
                int   count;

                fontlist[iso] = XListFonts(tool_d, font_template, FONT_EXAMINE_MAX, &count);
                if (fontlist[iso]) {
                    for (int c = 0; c<count; ++c) {
                        const char *fontname  = fontlist[iso][c];
                        int         size      = parsesize(fontname);
                        flist[found_fonts].fn = fontname; // valid as long fontlist[iso] is not free'd!
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
            struct xfont *nf = NULL;

            for (int size = MIN_FONTSIZE; size <= MAX_FONTSIZE; size++) { // scan all useful sizes
                int i;
                for (i = 0; i < found_fonts; i++) {
                    if (flist[i].s == size) break; // search first font matching current size
                }

                if (i < found_fonts && flist[i].s == size) {
                    struct xfont *newfont = (struct xfont *)malloc(sizeof(struct xfont));

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
                struct xfont *newfont   = (struct xfont *)malloc(sizeof(struct xfont));
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
static void dumpFontInformation(struct xfont *xf) {
    printf("Font information for '%s':\n", xf->fname);
    XFontStruct *xfs = xf->fstruct;
    printf("- max letter ascent  = %2i\n", xfs->max_bounds.ascent);
    printf("- max letter descent = %2i\n", xfs->max_bounds.descent);
    printf("- max letter height  = %2i\n", xfs->max_bounds.ascent + xfs->max_bounds.descent);
    printf("- max letter width   = %2i\n", xfs->max_bounds.width);
}
#endif // DEBUG

/*
 * Lookup an X font, "f" corresponding to a Postscript font style that is
 * close in size to "s"
 */

static bool lookfont(Display *tool_d, int f, int s, int& found_size, bool verboose, bool only_query, PIX_FONT *fontstPtr)
// returns true if appropriate font is available.
//
// 'found_size' is set to the actually found size, which may be bigger or smaller than 's', if the requested size is not available
//
// if 'only_query' is true, then only report availability
// if 'only_query' is false, then actually load the font and store the loaded fontstruct in 'fontstPtr'
{
    // char          fn[128];      memset(fn,0,128);
    AW_BOOL       found;
    struct xfont *newfont, *nf, *oldnf;

#if defined(DEVEL_RALF)
#warning scalability shall be checked for each font -- not only for first
#warning fontdetection is called for each GC -- do some caching ?
#endif // DEVEL_RALF

    found_size = -1;

    if (f == DEFAULT)
        f = 0;          /* pass back the -normal font font */
    if (s < 0)
        s = DEF_FONTSIZE;       /* default font size */

    /* see if we've already loaded that font size 's'
       from the font family 'f' */

    found = AW_FALSE;

    /* start with the basic font name (e.g. adobe-times-medium-r-normal-...
       OR times-roman for OpenWindows fonts) */

    nf = x_fontinfo[f].xfontlist;
    if (!nf) nf = x_fontinfo[0].xfontlist;
    oldnf = nf;
    if (nf != NULL) {
        if (nf->size > s && !appres.SCALABLEFONTS) {
            found = AW_TRUE;
        }
        else {
            while (nf != NULL) {
                if (nf->size == s ||
                    (!appres.SCALABLEFONTS && (nf->size >= s && oldnf->size <= s )))
                {
                    found = AW_TRUE;
                    break;
                }
                oldnf = nf;
                nf = nf->next;
            }
        }
    }
    if (found) {                /* found exact size (or only larger available) */
        // strcpy(fn,nf->fname);  /* put the name in fn */
        if (verboose) {
            if (s < nf->size) fprintf(stderr, "Font size %d not found, using larger %d point\n", s, nf->size);
            if (appres.debug) fprintf(stderr, "Detected font %s\n", nf->fname);
        }
    }
    else if (!appres.SCALABLEFONTS) { /* not found, use largest available */
        nf = oldnf;
        // strcpy(fn,nf->fname);           /* put the name in fn */
        if (verboose) {
            if (s > nf->size) fprintf(stderr, "Font size %d not found, using smaller %d point\n", s, nf->size);
            if (appres.debug) fprintf(stderr, "Using font %s for size %d\n", nf->fname, s);
        }
    }
    else { /* SCALABLE; none yet of that size, alloc one and put it in the list */
        newfont = (struct xfont *) malloc(sizeof(struct xfont));
        /* add it on to the end of the list */

        nf = oldnf ? oldnf->next : 0; // store successor

        if (x_fontinfo[f].xfontlist == NULL) x_fontinfo[f].xfontlist = newfont;
        else oldnf->next                                             = newfont;

        newfont->next    = nf; // old successor in fontlist
        newfont->size    = s;
        newfont->fstruct = NULL;
        newfont->fname   = NULL;

        nf = newfont;

        if (openwinfonts) {
            /* OpenWindows fonts, create font name like times-roman-13 */

            nf->fname = GBS_global_string_copy("%s-%d", x_fontinfo[f].templat, s);
            // sprintf(fn, "%s-%d", x_fontinfo[f].templat, s);
        }
        else {
            // X11 fonts, create a full XLFD font name
            // attach pointsize to font name, use the pixel field instead of points in the fontname so that the
            // font scales with screen size

            for (int iso = 0; iso<KNOWN_ISO_VERSIONS; ++iso) {
                char *fontname = GBS_global_string_copy("%s%d-*-*-*-*-*-%s-*", x_fontinfo[f].templat, s, known_iso_versions[iso]);
                // sprintf(fn, "%s%d-*-*-*-*-*-%s-*", x_fontinfo[f].templat, s, known_iso_versions[iso]);
#if defined(DUMP_FONT_LOOKUP)
                fprintf(stderr, "Checking for '%s' (x_fontinfo[%i].templat='%s')\n", fontname, f, x_fontinfo[f].templat);
#endif

                // PIX_FONT fontst = XQueryFont(tool_d, fontname);
                // PIX_FONT fontst = XLoadQueryFont(tool_d, fontname); // @@@ why loaded here and not below (at least use XQueryFont instead!)

                int    matching_fonts_found;
                char **matching_fonts = XListFonts(tool_d, fontname, 1, &matching_fonts_found);

                if (matching_fonts) {
                    XFreeFontNames(matching_fonts);
                    aw_assert(matching_fonts_found >= 1);
                    nf->fname = fontname;
                    break;
                }

//                 if (fontst) {
//                     XFreeFontInfo(fontst); // only did query -- loading done below
//                     // nf->fstruct = fontst;
//                     nf->fname      = fontname;
//                     break;
//                 }
                free(fontname);
            }
            // @@@ what if nf->fstruct is 0 now ?
        }

        aw_assert(nf->fname);
    } /* scalable */

    bool font_found = true;

    if (nf->fstruct == NULL) {
        if (only_query) {
            ; // assume its available (or use XQueryFont to reduce server-client bandwidth)
        }
        else {
            if (appres.debug && verboose) fprintf(stderr, "Loading font '%s'\n", nf->fname);
            PIX_FONT fontst = XLoadQueryFont(tool_d, nf->fname);
            if (fontst == NULL) {
                fprintf(stderr, "ARB fontpackage: Unexpectedly couldn't load font '%s', falling back to '%s' (f=%i, s=%i)\n", nf->fname, NORMAL_FONT, f, s);
                fontst = XLoadQueryFont(tool_d, NORMAL_FONT); // @@@ may return 0!
                free(nf->fname);
                nf->fname = strdup(NORMAL_FONT);    /* store actual name */
            }
            /* put the structure in the list */
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
        bool     was_found = lookfont(tool_d, f, size, found_size, false, true, &fontst);

        aw_assert(was_found); // because lookfont does fallback

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


const char *AW_root::font_2_ascii(AW_font font_nr)
{
    aw_assert(font_nr >= 0);
    // if (font_nr < 0) return "FIXED";
    if (font_nr<0 || font_nr>=AW_NUM_FONTS ) return 0;

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
    else  {
        readable_fontname = xf.templat;
    }
    return readable_fontname;
    // return (ps_fontinfo[font_nr+1].name);
}

int AW_root::font_2_xfig(AW_font font_nr)
{
    if (font_nr<0 || font_nr>=AW_NUM_FONTS ) return 0;
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
        } else {
            cs = &fs->per_char[col - fs->min_char_or_byte2];
            if (CI_NonExistChar(cs)) cs = def;
        }
    }
}

inline void CI_GetDefaultInfo_1D(const XFontStruct *fs, const XCharStruct*& cs) {
    CI_GetCharInfo_1D(fs, fs->default_char, NULL, cs);
}

/*
 * CI_GET_CHAR_INFO_2D - return the charinfo struct for the indicated row and
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

/*
 * CI_GetRowzeroCharInfo_2D - do the same thing as CI_GetCharInfo_1D,
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
        } else {
            cs = &fs->per_char[(col - fs->min_char_or_byte2)];
            if (CI_NonExistChar(cs)) cs = def;
        }
    }
}


void AW_GC_Xm::set_font(AW_font font_nr, int size, int *found_size)
// if found_size != 0 -> return value for used font size
{
    XFontStruct *xfs;

    {
        int  found_font_size;
        bool was_found = lookfont(common->display, font_nr, size, found_font_size, true, false, &xfs);
        aw_assert(was_found); // because lookfont does fallback

        if (found_size) *found_size = found_font_size;
    }
    XSetFont(common->display, gc, xfs->fid);
    curfont = *xfs;

    const XCharStruct *cs;
    const XCharStruct *def;           /* info about default char */
    Bool         singlerow = (xfs->max_byte1 == 0); /* optimization */

    if (singlerow) {    /* optimization */
        CI_GetDefaultInfo_1D(xfs, def);
    }
    else {
        CI_GetDefaultInfo_2D(xfs, def);
    }

    fontinfo.max_letter.reset();
    fontinfo.max_all_letter.reset();

    aw_assert(AW_FONTINFO_CHAR_MIN < AW_FONTINFO_CHAR_MAX);
    aw_assert(AW_FONTINFO_CHAR_ASCII_MAX <= AW_FONTINFO_CHAR_MAX);
    aw_assert(AW_FONTINFO_CHAR_MIN < AW_FONTINFO_CHAR_ASCII_MAX);

    unsigned int i;
    for (i = AW_FONTINFO_CHAR_MIN; i <= AW_FONTINFO_CHAR_MAX; i++) {
        if (singlerow) {/* optimization */
            CI_GetCharInfo_1D(xfs, i, def, cs);
        } else {
            // cs = def;   // X11R4
            CI_GetRowzeroCharInfo_2D(xfs, i, def, cs);
        }
        if (cs) {
            ascent_of_chars[i]  = cs->ascent;
            descent_of_chars[i] = cs->descent;
            width_of_chars[i]   = cs->width;

            if (i <= AW_FONTINFO_CHAR_ASCII_MAX) {
                fontinfo.max_letter.notify_all(cs->ascent, cs->descent, cs->width);
            }
            fontinfo.max_all_letter.notify_all(cs->ascent, cs->descent, cs->width);
        }
        else {
            width_of_chars[i]   = 0;
            descent_of_chars[i] = 0;
            ascent_of_chars[i]  = 0;
        }
    }

    fontinfo.max_letter.calc_height();
    fontinfo.max_all_letter.calc_height();

    this->fontnr   = font_nr;
    this->fontsize = size;
}

int AW_GC_Xm::get_available_fontsizes(AW_font font_nr, int *available_sizes) {
    return ::get_available_fontsizes(common->display, font_nr, available_sizes);
}

