#ifndef AW_AT_HXX
#define AW_AT_HXX

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

    short saved_x_correction_for_label;

    short saved_x;
    bool  correct_for_at_string;
    int   correct_for_at_center;
    short x_for_newline;

    bool attach_x;           // attach right side to right form
    bool attach_y;
    bool attach_lx;          // attach left side to right form
    bool attach_ly;
    bool attach_any;

    AW_at(void);
};


#else
#error aw_at.hxx included twice
#endif
