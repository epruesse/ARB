#ifndef aw_xfont_hxx_included
#define aw_xfont_hxx_included

#define         NORMAL_FONT     "fixed"
#define         BOLD_FONT       "8x13bold"
#define         BUTTON_FONT     "6x13"

struct xfont {
    int           size;         /* size in points */
    Font          fid;          /* X font id */
    char         *fname;        /* actual name of X font found */
    XFontStruct  *fstruct;      /* X font structure */
    struct xfont *next;         /* next in the list */
};

struct _fstruct {
    const char     *name;       /* Postscript font name */
    int             xfontnum;   /* template for locating X fonts */
};

struct _xfstruct {
    const char     *templat;    /* templat for locating X fonts */
    struct xfont   *xfontlist;  /* linked list of X fonts for different point
                                 * sizes */
};

typedef struct _appres {
    AW_BOOL  SCALABLEFONTS;     /* hns 5 Nov 91 */
    AW_BOOL  debug;             /* hns 5 Nov 91 */
    Display *display;
}               appresStruct, *appresPtr;
extern appresStruct appres;

#define DEFAULT               (-1)
#define DEF_FONTSIZE 12
#define NORMAL_FONT     "fixed"


#endif
