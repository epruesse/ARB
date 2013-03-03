// ============================================================= //
//                                                               //
//   File      : aw_common.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 20, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_common.hxx"

AW_common::AW_common(AW_rgb*& fcolors,
                     AW_rgb*& dcolors,
                     long&    dcolors_count)
  : frame_colors(fcolors),
    data_colors(dcolors),
    data_colors_size(dcolors_count)
{
    screen.t = 0;
    screen.b = -1;
    screen.l = 0;
    screen.r = -1;
}

void AW_common::new_gc(int gc) {
    //remove old gc
    if(gcmap.find(gc) != gcmap.end()) {
        delete gcmap[gc];
    }
    gcmap[gc] = create_gc();
}

bool AW_common::gc_mapable(int gc) const {
    return gcmap.find(gc) != gcmap.end();
}

const AW_GC *AW_common::map_gc(int gc) const {
    return  gcmap.find(gc)->second;
}

AW_GC *AW_common::map_mod_gc(int gc) {
    return gcmap[gc];
}

const AW_font_limits& AW_common::get_font_limits(int gc, char c) const {
    // for one characters (c == 0 -> for all characters)
    return c
        ? map_gc(gc)->get_font_limits(c)
        : map_gc(gc)->get_font_limits();
}

AW_rgb AW_common::get_XOR_color() const {
    return data_colors ? data_colors[AW_DATA_BG] : frame_colors[AW_WINDOW_BG];
}

AW_rgb AW_common::get_data_color(int i) const {
    return data_colors[i];
}

int AW_common::get_data_color_size() const {
    return data_colors_size;
}

void AW_common::set_screen(const AW_screen_area& screen_) {
    // set clipping coordinates
    screen = screen_;
    aw_assert(screen.t == 0 && screen.l == 0);
}

void AW_common::set_screen_size(unsigned int width, unsigned int height) {
    screen.t = 0;               // set clipping coordinates
    screen.b = height;
    screen.l = 0;
    screen.r = width;
}

void AW_common::reset_style()
{
    std::map<int, AW_GC*>::iterator it;
    for(it = gcmap.begin(); it != gcmap.end(); it++) {
        it->second->reset();
    }
}

void AW_GC_config::set_grey_level(AW_grey_level grey_level_) {
    // <0 = don't fill, 0.0 = white, 1.0 = black
    grey_level = grey_level_;
}


int AW_GC::get_string_size(const char *str, long textlen) const {
    // calculate display size of 'str'
    // 'str' and/or 'textlen' may be 0
    // 'str'     == 0 -> calculate max width of any text with length 'textlen'
    // 'textlen' == 0 -> calls strlen when needed
    // both 0 -> return 0

    int width = 0;
    if (font_limits.is_monospaced() || !str) {
        if (!textlen && str) textlen = strlen(str);
        width = textlen * font_limits.width;
    }
    else {
        for (int c = *(str++); c; c = *(str++)) width += width_of_chars[c];
    }
    return width;
}

