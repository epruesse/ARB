#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>
#include <ctype.h>
#include <X11/Xlib.h>

#include <arbdb.h>
#define _AW_COMMON_INCLUDED
#include <aw_root.hxx>
#include "aw_device.hxx"
//#include "aw_window.hxx"
#include "aw_commn.hxx"
#include "aw_xfont.hxx"

#define template templat		// c++ compiler problems
#undef False
#undef True
#define False	AW_FALSE
#define True 	AW_TRUE
#define Boolean AW_BOOL
#define MAXNAMES AW_NUM_FONTS
#define put_msg printf
static struct {
    const char	   *fn;
    int		    s;
} flist[MAXNAMES];

static Boolean	openwinfonts;

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
 */

typedef XFontStruct *PIX_FONT;
appresStruct appres = {
    AW_TRUE,
    AW_FALSE,
    0,
};

struct _xfstruct x_fontinfo[AW_NUM_FONTS] = {
    {"-adobe-times-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-times-medium-i-*--", (struct xfont*) NULL},
    {"-adobe-times-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-times-bold-i-*--", (struct xfont*) NULL},
    {"-schumacher-clean-medium-r-*--", (struct xfont*) NULL},	/* closest to Avant-Garde */
    {"-schumacher-clean-medium-i-*--", (struct xfont*) NULL},
    {"-schumacher-clean-bold-r-*--", (struct xfont*) NULL},
    {"-schumacher-clean-bold-i-*--", (struct xfont*) NULL},
    {"-adobe-times-medium-r-*--", (struct xfont*) NULL},	/* closest to Bookman */
    {"-adobe-times-medium-i-*--", (struct xfont*) NULL},
    {"-adobe-times-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-times-bold-i-*--", (struct xfont*) NULL},
    {"-adobe-courier-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-courier-medium-o-*--", (struct xfont*) NULL},
    {"-adobe-courier-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-courier-bold-o-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-o-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-o-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-medium-r-*--", (struct xfont*) NULL},	/* closest to Helv-nar. */
    {"-adobe-helvetica-medium-o-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-helvetica-bold-o-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-r-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-medium-i-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-r-*--", (struct xfont*) NULL},
    {"-adobe-new century schoolbook-bold-i-*--", (struct xfont*) NULL},
    {"-*-lucidabright-medium-r-*--", (struct xfont*) NULL},	/* closest to Palatino */
    {"-*-lucidabright-medium-i-*--", (struct xfont*) NULL},
    {"-*-lucidabright-demibold-r-*--", (struct xfont*) NULL},
    {"-*-lucidabright-demibold-i-*--", (struct xfont*) NULL},
    {"-*-symbol-medium-r-*--", (struct xfont*) NULL},
    {"-*-zapfchancery-medium-i-*--", (struct xfont*) NULL},
    {"-*-zapfdingbats-*-*-*--", (struct xfont*) NULL},
    // non xfig fonts, will be mapped to xfig fonts on export
    {"-*-lucida-medium-r-*-*-", (struct xfont*) NULL},
    {"-*-lucida-medium-i-*-*-", (struct xfont*) NULL},
    {"-*-lucida-bold-r-*-*-", (struct xfont*) NULL},
    {"-*-lucida-bold-i-*-*-", (struct xfont*) NULL},
    {"-*-lucidatypewriter-medium-*-*-*-", (struct xfont*) NULL},
    {"-*-lucidatypewriter-bold-*-*-*-", (struct xfont*) NULL},
    {"-*-screen-medium-*-*-*-", (struct xfont*) NULL},
    {"-*-screen-bold-*-*-*-", (struct xfont*) NULL},
};

struct _fstruct ps_fontinfo[AW_NUM_FONTS + 1] = {
				// map window fonts to postscript fonts
				// negative values indicate monospaced fonts
    {"Default", -1},
    {"Times-Roman",			0},
    {"Times-Italic",			1},
    {"Times-Bold",			2},
    {"Times-BoldItalic",		3},
    {"AvantGarde-Book",			4},
    {"AvantGarde-BookOblique",		5},
    {"AvantGarde-Demi",			6},
    {"AvantGarde-DemiOblique",		7},
    {"Bookman-Light",			8},
    {"Bookman-LightItalic",		9},
    {"Bookman-Demi",			10},
    {"Bookman-DemiItalic",		11},
    {"Courier",				-12},
    {"Courier-Oblique",			-13},
    {"Courier-Bold",			-14},
    {"Courier-BoldOblique",		-15},
    {"Helvetica",			16},
    {"Helvetica-Oblique",		17},
    {"Helvetica-Bold",			18},
    {"Helvetica-BoldOblique",		19},
    {"Helvetica-Narrow",		20},
    {"Helvetica-Narrow-Oblique",	21},
    {"Helvetica-Narrow-Bold",		22},
    {"Helvetica-Narrow-BoldOblique",	23},
    {"NewCenturySchlbk-Roman",		24},
    {"NewCenturySchlbk-Italic",		25},
    {"NewCenturySchlbk-Bold",		26},
    {"NewCenturySchlbk-BoldItalic",	27},
    {"Palatino-Roman",			28},
    {"Palatino-Italic",			29},
    {"Palatino-Bold",			30},
    {"Palatino-BoldItalic",		31},
    {"Symbol",				32},
    {"ZapfChancery-MediumItalic",	33},
    {"ZapfDingbats",			34},
    {"LucidaSans",			16},
    {"LucidaSans-Italic",		17},
    {"LucidaSans-Bold",			18},
    {"LucidaSans-BoldItalic",		19},
    {"LucidaSansTypewriter",		-12},
    {"LucidaSansTypewriter-Bold",	-14},
    {"Screen",	-16},
    {"Screen-Bold",	-18},
};


/* parse the point size of font 'name' */
/* e.g. -adobe-courier-bold-o-normal--10-100-75-75-m-60-ISO8859-1 */

static int parsesize(char *name)
{
    int		    s;
    char	   *np;

    for (np = name; *(np + 1); np++)
        if (*np == '-' && *(np + 1) == '-')	/* look for the -- */
            break;
    s = 0;
    if (*(np + 1)) {
        np += 2;		/* point past the -- */
        s = atoi(np);		/* get the point size */
    }
    else {
        // no '--' found; search for first - followed by a number
        s = 0;                  // return size == 0 if nothing detected
        for (np = name; ; ++np) {
            if (*np == '-') {
                char *dw;
                for (dw = np+1; isdigit(*dw); ++dw) ;
                if (dw>(np+1) && *dw == '-') { // only digits found
                    s = atoi(np+1);
                    break;
                }
            }
        }
        if (!s) fprintf(stderr, "Can't parse '%s'\n", name);
    }

    return s;
}

void aw_root_init_font(Display *tool_d)
{
    struct xfont   *newfont, *nf;
    int		    f, count, i, p, ss;
    char	    templat[200];
    char	**fname;

    if (appres.display) return;
    appres.display = tool_d;

    /*
     * Now initialize the font structure for the X fonts corresponding to the
     * Postscript fonts for the canvas.	 OpenWindows can use any LaserWriter
     * fonts at any size, so we don't need to load anything if we are using
     * it.
     */

    /* if the user hasn't disallowed off scalable fonts, check that the
       server really has them by checking for font of 0-0 size */
    openwinfonts = False;
    if (appres.SCALABLEFONTS) {
        char	  **fontlist;
        /* first look for OpenWindow style font names (e.g. times-roman) */
        if ((fontlist = XListFonts(tool_d, ps_fontinfo[1].name, 1, &count))!=0) {
            openwinfonts = True;	/* yes, use them */
            for (f=0; f<AW_NUM_FONTS; f++)	/* copy the OpenWindow font names */
                x_fontinfo[f].templat = ps_fontinfo[f+1].name;
            XFreeFontNames(fontlist);
        } else {
            strcpy(templat,x_fontinfo[0].templat);  /* nope, check for font size 0 */
            strcat(templat,"0-0-*-*-*-*-*-*");
            if ((fontlist = XListFonts(tool_d, templat, 1, &count))!=0){
                XFreeFontNames(fontlist);
            }else{
                appres.SCALABLEFONTS = False;	/* none, turn off request for them */
            }
        }
    }

    /* no scalable fonts - query the server for all the font
       names and sizes and build a list of them */

    if (!appres.SCALABLEFONTS) {
        char	  **fontlist;
        for (f = 0; f < AW_NUM_FONTS; f++) {
            nf = NULL;
            strcpy(templat,x_fontinfo[f].templat);
            strcat(templat,"*-*-*-*-*-*-");
            /* add ISO8859 (if not Symbol font or ZapfDingbats) to font name */
            if (strstr(templat,"symbol") == NULL && strstr(templat,"zapfdingbats") == NULL) {
                strcat(templat,"ISO8859-*");
            }
            else {
                strcat(templat,"*-*");
            }

            p = 0;
            if ((fontlist = XListFonts(tool_d, templat, MAXNAMES, &count))==0) { /* don't free the Fontlist because we keep pointers into it */
                /* no fonts by that name found, substitute the -normal font name */
                flist[p].fn = NORMAL_FONT;
                flist[p++].s = 12;	/* just set the size to 12 */
            }
            else {
                fname = fontlist; /* go through the list finding point sizes */
                while (count--) {
                    ss = parsesize(*fname); /* get the point size from the name */

                    flist[p].fn = *fname++; /* save name of this size font */
                    flist[p++].s    = ss; /* and save size */
                }
            }
            for (ss = 4; ss <= 50; ss++) {
                for (i = 0; i < p; i++)
                    if (flist[i].s == ss)
                        break;
                if (i < p && flist[i].s == ss) {
                    newfont = (struct xfont *) malloc(sizeof(struct xfont));
                    if (nf == NULL)
                        x_fontinfo[f].xfontlist = newfont;
                    else
                        nf->next = newfont;
                    nf = newfont;	/* keep current ptr */
                    nf->size = ss;	/* store the size here */
                    nf->fname = flist[i].fn;	/* keep actual name */
                    nf->fstruct = NULL;
                    nf->next = NULL;
                }
            } /* next size */
        } /* next font, f */
    } /* !appres.SCALABLEFONTS */
}


/*
 * Lookup an X font, "f" corresponding to a Postscript font style that is
 * close in size to "s"
 */

PIX_FONT
lookfont(Display *tool_d, int f, int s)
{
    PIX_FONT        fontst;
    char		fn[128];	memset(fn,0,128);
    char		templat[200];
    Boolean		found;
    struct xfont   *newfont, *nf, *oldnf;


    if (f == DEFAULT)
	f = 0;		/* pass back the -normal font font */
    if (s < 0)
	s = DEF_FONTSIZE;	/* default font size */

    /* see if we've already loaded that font size 's'
       from the font family 'f' */

    found = False;

    /* start with the basic font name (e.g. adobe-times-medium-r-normal-...
       OR times-roman for OpenWindows fonts) */

    nf = x_fontinfo[f].xfontlist;
    if (!nf) nf = x_fontinfo[0].xfontlist;
    oldnf = nf;
    if (nf != NULL) {
	if (nf->size > s && !appres.SCALABLEFONTS)
	    found = True;
	else while (nf != NULL){
	    if (nf->size == s || (!appres.SCALABLEFONTS &&
				  (nf->size >= s && oldnf->size <= s ))) {
		found = True;
		break;
	    }
	    oldnf = nf;
	    nf = nf->next;
	}
    }
    if (found) {		/* found exact size (or only larger available) */
	strcpy(fn,nf->fname);  /* put the name in fn */
	if (s < nf->size)
	    put_msg("Font size %d not found, using larger %d point\n",s,nf->size);
	if (appres.debug)
	    fprintf(stderr, "Located font %s\n", fn);
    } else if (!appres.SCALABLEFONTS) {	/* not found, use largest available */
	nf = oldnf;
	strcpy(fn,nf->fname);  		/* put the name in fn */
	if (s > nf->size)
	    put_msg("Font size %d not found, using smaller %d point\n",s,nf->size);
	if (appres.debug)
	    fprintf(stderr, "Using font %s for size %d\n", fn, s);
    } else { /* SCALABLE; none yet of that size, alloc one and put it in the list */
	newfont = (struct xfont *) malloc(sizeof(struct xfont));
	/* add it on to the end of the list */
	if (x_fontinfo[f].xfontlist == NULL)
	    x_fontinfo[f].xfontlist = newfont;
	else
	    oldnf->next = newfont;
	nf = newfont;		/* keep current ptr */
	nf->size = s;		/* store the size here */
	nf->fstruct = NULL;
	nf->next = NULL;

	if (openwinfonts) {
	    /* OpenWindows fonts, create font name like times-roman-13 */
	    sprintf(fn, "%s-%d", x_fontinfo[f].templat, s);
	} else {
	    /* X11 fonts, create a full XLFD font name */
	    strcpy(templat,x_fontinfo[f].templat);
	    /* attach pointsize to font name */
	    strcat(templat,"%d-*-*-*-*-*-");
	    /* add ISO8859 (if not Symbol font or ZapfDingbats) to font name */
// 	    if (strstr(templat,"symbol")          == NULL &&
//             strstr(templat,"zapfdingbats") == NULL)
            strcat(templat,"%s-*");
// 	    else
//             strcat(templat,"*-*");
	    /* use the pixel field instead of points in the fontname so that the
	       font scales with screen size */

	    sprintf(fn, templat, s, "ISO8859");
        fontst = XLoadQueryFont(tool_d, fn);
        if (fontst == NULL) {
            sprintf(fn, templat, s, "ISO10646");
            fontst = XLoadQueryFont(tool_d, fn);
            if (fontst == NULL) {
                sprintf(fn, templat, s, "*");
            }
        }
	}
	/* allocate space for the name and put it in the structure */
	nf->fname = strdup(fn);
    } /* scalable */
    if (nf->fstruct == NULL) {
	if (appres.debug)
	    fprintf(stderr, "Loading font %s\n", fn);
	//	    set_temp_cursor(wait_cursor);
	//	    app_flush();
	fontst = XLoadQueryFont(tool_d, fn);
	//	    reset_cursor();
	if (fontst == NULL) {
	    fprintf(stderr, "ARB fontpackage: Can't load font %s ?!, using %s\n",
		    fn, NORMAL_FONT);
	    fontst = XLoadQueryFont(tool_d, NORMAL_FONT);
	    nf->fname = strdup(NORMAL_FONT);	/* keep actual name */
	}
	/* put the structure in the list */
	nf->fstruct = fontst;
    } /* if (nf->fstruct == NULL) */

    return (nf->fstruct);
}


const char *AW_root::font_2_ascii(AW_font font_nr)
{
    if (font_nr <0) return "FIXED";
    if (font_nr >= AW_NUM_FONTS ) return 0;
    return (ps_fontinfo[font_nr+1].name);
}

int AW_root::font_2_xfig(AW_font font_nr)
{
    if (font_nr <0) return 0;
    if (font_nr >= AW_NUM_FONTS ) return 0;
    return (ps_fontinfo[font_nr+1].xfontnum);
}

#define CI_NONEXISTCHAR(cs) (((cs)->width == 0) && \
			     (((cs)->rbearing|(cs)->lbearing| \
			       (cs)->ascent|(cs)->descent) == 0))

#define CI_GET_CHAR_INFO_1D(fs,col,def,cs)				\
{									\
    cs = def;								\
    if (col >= fs->min_char_or_byte2 && col <= fs->max_char_or_byte2) {	\
	if (fs->per_char == NULL) {					\
	    cs = &fs->min_bounds;					\
	} else {							\
	    cs = &fs->per_char[(col - fs->min_char_or_byte2)];		\
	    if (CI_NONEXISTCHAR(cs)) cs = def;				\
	}								\
    }									\
}

#define CI_GET_DEFAULT_INFO_1D(fs,cs) \
  CI_GET_CHAR_INFO_1D (fs, fs->default_char, NULL, cs)

/*
 * CI_GET_CHAR_INFO_2D - return the charinfo struct for the indicated row and
 * column.  This is used for fonts that have more than row zero.
 */
#define CI_GET_CHAR_INFO_2D(fs,row,col,def,cs) \
{ \
    cs = def; \
    if (row >= fs->min_byte1 && row <= fs->max_byte1 && \
	col >= fs->min_char_or_byte2 && col <= fs->max_char_or_byte2) { \
	if (fs->per_char == NULL) { \
	    cs = &fs->min_bounds; \
	} else { \
	    cs = &fs->per_char[((row - fs->min_byte1) * \
			        (fs->max_char_or_byte2 - \
				 fs->min_char_or_byte2 + 1)) + \
			       (col - fs->min_char_or_byte2)]; \
	    if (CI_NONEXISTCHAR(cs)) cs = def; \
        } \
    } \
}

#define CI_GET_DEFAULT_INFO_2D(fs,cs) \
{ \
    unsigned int r = (fs->default_char >> 8); \
    unsigned int c = (fs->default_char & 0xff); \
    CI_GET_CHAR_INFO_2D (fs, r, c, NULL, cs); \
}

/*
 * CI_GET_ROWZERO_CHAR_INFO_2D - do the same thing as CI_GET_CHAR_INFO_1D,
 * except that the font has more than one row.  This is special case of more
 * general version used in XTextExt16.c since row == 0.  This is used when
 * max_byte2 is not zero.  A further optimization would do the check for
 * min_byte1 being zero ahead of time.
 */

#if 0	// X11R6!!!
#define CI_GET_ROWZERO_CHAR_INFO_2D(fs,col,def,cs) \
{ \
    cs = def; \
    if (fs->min_byte1 == 0 && \
	col >= fs->min_byte2 && col <= fs->max_byte2) { \
	if (fs->per_char == NULL) { \
	    cs = &fs->min_bounds; \
	} else { \
	    cs = &fs->per_char[(col - fs->min_byte2)]; \
	    if (CI_NONEXISTCHAR(cs)) cs = def; \
	} \
    } \
}
#endif

void            AW_GC_Xm::
set_font(AW_font font_nr, int size)
{
    XFontStruct    *xfs;
    xfs = lookfont(common->display, font_nr, size);
    XSetFont(common->display, gc, xfs->fid);
    curfont = *xfs;
    register XCharStruct *cs;
    XCharStruct    *def;	/* info about default char */
    Bool            singlerow = (xfs->max_byte1 == 0);	/* optimization */
    if (singlerow) {	/* optimization */
	CI_GET_DEFAULT_INFO_1D(xfs, def);
    } else {
	CI_GET_DEFAULT_INFO_2D(xfs, def);
    }
    unsigned int i;
    for (i = 0; i < 256; i++) {
	if (singlerow) {/* optimization */
	    CI_GET_CHAR_INFO_1D(xfs, i, def, cs);
	} else {
	    cs = def;	// X11R4
	    //CI_GET_ROWZERO_CHAR_INFO_2D(xfs, i, def, cs);
	}
	if (cs) {
	    width_of_chars[i] = cs ->width;
	    descent_of_chars[i] = cs->descent;
	    ascent_of_chars[i] = cs->ascent;
	}else{
	    width_of_chars[i] = 0;
	    descent_of_chars[i] = 0;
	    ascent_of_chars[i] = 0;
	}
    }
    fontinfo.max_letter_ascent = xfs->max_bounds.ascent;
    fontinfo.max_letter_descent = xfs->max_bounds.descent;
    fontinfo.max_letter_height = xfs->max_bounds.ascent + xfs->max_bounds.descent;
    fontinfo.max_letter_width = xfs->max_bounds.width;

    this->fontnr = font_nr;
    this->fontsize = size;
}
