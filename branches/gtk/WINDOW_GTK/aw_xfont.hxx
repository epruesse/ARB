#pragma once

#ifndef AW_DEF_HXX
#include "aw_def.hxx"
#endif
#include <Xm/Xm.h>

typedef XFontStruct *PIX_FONT;

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

void aw_root_init_font(Display *display);

bool lookfont(Display *tool_d, int f, int s, int& found_size, bool verboose, bool only_query, PIX_FONT *fontstPtr);

#define DEFAULT      (-1)

