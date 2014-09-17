
#pragma once

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif

class AW_xfig;

#if defined(ARB_MOTIF)

// Motif misplaces or cripples widgets created beyond the current window limits.
// Workaround: make window huge during setup (applies to windows which resize on show)
// Note: Values below just need to be bigger than any actually created window, should probably be smaller than 32768
// and can be eliminated when we (completely) switch to ARB_GTK
#define WIDER_THAN_SCREEN  10000
#define HIGHER_THAN_SCREEN 6000

#endif

/**
 * A cursor that describes where and how gui elements should be placed
 * in a window.
 */
class AW_at {
private:
    AW_window* window; /** < The window this cursor belongs to */
    AW_xfig* xfig_data;
public:
    char *at_id;

    short length_of_buttons;
    short height_of_buttons;
    int font_width;
    int font_height;
    short length_of_label_for_inputfield;
    bool  highlight;

    char      *helptext_for_next_button;
    AW_active  widget_mask; // sensitivity (expert/novice mode)

    unsigned long int background_color; // gdk Pixel

    char *label_for_inputfield;

    int x_for_next_button;
    int y_for_next_button;
    int max_x_size;
    int max_y_size;

    int  to_position_x;
    int  to_position_y;
    bool to_position_exists;

    bool do_auto_space;
    int  auto_space_x;
    int  auto_space_y;

    bool do_auto_increment;
    int  auto_increment_x;
    int  auto_increment_y;

    int biggest_height_of_buttons;

    short saved_xoff_for_label;

    short saved_x;
    int   correct_for_at_center;
    short x_for_newline;

    bool attach_x;           // attach right side to right form
    bool attach_y;
    bool attach_lx;          // attach left side to right form
    bool attach_ly;
    bool attach_any;

    AW_at(AW_window* pWindow);
    void set_xfig(AW_xfig* xfig);
    void set_mask(AW_active mask);
    void at(const char *at_id);
    void at(int x, int y);
    void at_x(int x);
    void at_y(int y);
    void at_shift(int x, int y);
    void at_newline();
    bool at_ifdef(const char* id);
    void at_set_to(bool attach_x, bool attach_y, int xoff, int yoff);
    void at_unset_to();
    void auto_increment(int x, int y);
    void unset_at_commands();
    void increment_at_commands(int x, int y);
    void at_set_min_size(int xmin, int ymin);
    void auto_space(int x, int y);
    void label_length(int length);
    void button_length(int length);
    int get_button_length() const;
    void get_at_position(int *x, int *y) const;
    int get_at_xposition() const;
    int get_at_yposition() const;
};


class AW_at_size {
protected:
    int  to_offset_x;                               // here we use offsets (not positions like in AW_at)
    int  to_offset_y;
    bool to_position_exists;

    bool attach_x;           // attach right side to right form
    bool attach_y;
    bool attach_lx;          // attach left side to right form
    bool attach_ly;
    bool attach_any;

public:
    AW_at_size()
        : to_offset_x(0),
          to_offset_y(0),
          to_position_exists(false),
          attach_x(false), 
          attach_y(false), 
          attach_lx(false), 
          attach_ly(false), 
          attach_any(false) 
    {}

    void store(const AW_at& at);
    void restore(AW_at& at) const;
};


class AW_at_auto {
    enum { INC, SPACE, OFF } type;
    int x, y;
    int xfn, xfnb, yfnb, bhob;
public:
    AW_at_auto() : type(OFF), x(0), y(0), xfn(0), xfnb(0), yfnb(0), bhob(0) {}

    void store(const AW_at &at);
    void restore(AW_at &at) const;
};

class AW_at_maxsize {
    int maxx;
    int maxy;

public:
    AW_at_maxsize()
        : maxx(0),
          maxy(0)
    {}
    
    void store(const AW_at &at);
    void restore(AW_at &at) const;
};
