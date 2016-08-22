#ifndef AW_XFONT_HXX
#define AW_XFONT_HXX

#ifndef AW_DEF_HXX
#include "aw_def.hxx"
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
    xfont      *xfontlist;      // linked list of X fonts for different point sizes
};

#define DEFAULT      (-1)

#else
#error aw_xfont.hxx included twice
#endif
