#ifndef AW_AT_HXX
#define AW_AT_HXX

#ifndef AW_BASE_HXX
#include "aw_base.hxx"
#endif

#if defined(ARB_MOTIF)

// Motif misplaces or cripples widgets created beyond the current window limits.
// Workaround: make window huge during setup (applies to windows which resize on show)
// Note: Values below just need to be bigger than any actually created window, should probably be smaller than 32768
// and can be eliminated when we (completely) switch to ARB_GTK
#define WIDER_THAN_SCREEN  10000
#define HIGHER_THAN_SCREEN  6000

#endif

/**
 * A cursor that describes where and how gui elements should be placed
 * in a window.
 */
class AW_at {
public:
    short shadow_thickness;
    short length_of_buttons;
    short height_of_buttons;
    short length_of_label_for_inputfield;
    bool  highlight;

    char      *helptext_for_next_button;
    AW_active  widget_mask; // sensitivity (expert/novice mode)

    unsigned long int background_color; // X11 Pixel

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

    AW_at();
};

#else
#error aw_at.hxx included twice
#endif
