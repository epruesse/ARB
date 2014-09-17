// =============================================================== //
//                                                                 //
//   File      : aw_xfig.hxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#pragma once

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif


const int XFIG_DEFAULT_FONT_WIDTH  = 8;
const int XFIG_DEFAULT_FONT_HEIGHT = 13;

const int MAX_LINE_WIDTH  = 20;
const int MAX_XFIG_LENGTH = 100000;

class AW_device;

char *aw_get_font_from_xfig(int fontnr);

struct AW_xfig_line {
    struct AW_xfig_line *next;

    short x0, y0;
    short x1, y1;
    short color;
    int   gc;
};

struct AW_xfig_text {
    struct AW_xfig_text *next;

    short    x, y;
    short    pix_len;
    char    *text;
    AW_font  font;
    short    fontsize;
    int      center;
    short    color;
    int      gc;
};

struct AW_xfig_pos {
    short x, y;
    int   center; // 0=left, 1=center, 2=right
};

class AW_xfig : virtual Noncopyable {
    void calc_scaling(int font_width, int font_height);

    void init(int font_width, int font_height) {
        text        = NULL;
        at_pos_hash = NULL;

        memset(line, 0, sizeof(line));

        minx = 0; maxx = 0; centerx = 0;
        miny = 0; maxy = 0; centery = 0;

        size_x = size_y = 0;

        calc_scaling(font_width, font_height);
    }

public:
    struct AW_xfig_text *text;
    struct AW_xfig_line *line[MAX_LINE_WIDTH];

    GB_HASH *at_pos_hash;                         // hash table for named positions

    int minx, miny;
    int maxx, maxy;
    int size_x, size_y;
    int centerx, centery;

    double font_scale;
    double dpi_scale;

    AW_xfig(const char *filename, int font_width, int font_height);
    AW_xfig(int font_width, int font_height);      // creates an empty drawing area

    ~AW_xfig();
    void print(AW_device *device); // you can scale it
    /**
     * initializes the graphics contexts
     */
    void create_gcs(AW_device *device);

    void add_line(int x1, int y1, int x2, int y2, int width); // add a line to xfig
};


/* xfig format ::

text::

4 0 font(-1== default) fontsize depth color  ?? angle justified(4=left) flags width x y text^A

lines::

2 art depth width color ....
x y x y 9999 9999

*/
