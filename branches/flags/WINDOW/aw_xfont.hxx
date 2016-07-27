#ifndef AW_XFONT_HXX
#define AW_XFONT_HXX

#ifndef AW_DEF_HXX
#include "aw_def.hxx"
#endif
#ifndef AW_COMMON_XM_HXX
#include "aw_common_xm.hxx"
#endif


struct xfont {
    int          size;          // size in points
    Font         fid;           // X font id
    char        *fname;         // actual name of X font found
    XFontStruct *fstruct;       // X font structure
    xfont       *next;          // next in the list
};

struct _fstruct {
    const char     *name;       // Postscript font name
    int             xfontnum;   // template for locating X fonts
};

struct _xfstruct {
    const char *templat;        // templat for locating X fonts
    const char *description;    // short name (displayed on font button)
    xfont      *xfontlist;      // linked list of X fonts for different point sizes
};

const char *AW_get_font_specification(AW_font font_nr);
const char *AW_get_font_shortname(AW_font font_nr);

int AW_font_2_xfig(AW_font font_nr);

#else
#error aw_xfont.hxx included twice
#endif
