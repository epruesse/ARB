#ifndef AW_AT_HXX
#define AW_AT_HXX


class AW_at {
public:
    short	shadow_thickness;
    short	length_of_buttons;
    short	length_of_label_for_inputfield;
    AW_BOOL	highlight;

    char	  *id_for_next_button;
    char	  *helptext_for_next_button;
    AW_active  mask_for_next_button;

    char *background_colorname;
    char *label_for_inputfield;

    int	x_for_next_button;
    int	y_for_next_button;
    int	max_x_size;
    int	max_y_size;

    int		to_position_x;
    int		to_position_y;
    AW_BOOL	to_position_exists;

    AW_BOOL	do_auto_space;
    int		auto_space_x;
    int		auto_space_y;

    AW_BOOL	do_auto_increment;
    int		auto_increment_x;
    int		auto_increment_y;

    int	biggest_height_of_buttons;

    short saved_x_correction_for_label;
//     short correct_for_at_center_intern;

    short	saved_x;
    AW_BOOL	correct_for_at_string;
    int		correct_for_at_center;
    short	x_for_newline;

    AW_BOOL	attach_x;           // attach right side to right form
    AW_BOOL	attach_y;
    AW_BOOL	attach_lx;          // attach left side to right form
    AW_BOOL	attach_ly;
    AW_BOOL	attach_any;

    AW_at(void);
};


#else
#error aw_at.hxx included twice
#endif
