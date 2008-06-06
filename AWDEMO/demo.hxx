class Box {
public:
    // for lines
    AW_pos  line_x0[5], line_y0[5];
    AW_pos  line_x1[5], line_y1[5];
    char   *name[5];
    int     gc[5];
    // for text
    AW_pos  text_x, text_y;

    AW_BOOL drag;
    AW_BOOL drag_line;
    int     line_number;
    AW_BOOL drag_text;

    // eigentlich AW_window
    AW_pos start_x, start_y;
    AW_pos last_x, last_y;
    AW_pos shift_x, shift_y;
    AW_pos zoom;
    // eigentlich AW_window :ende:

    Box( void );
    ~Box( void );
    void draw( AW_device *device, int slider_pos_horizontal, int slider_pos_vertical );
    void draw_drag_line( AW_device *device, int slider_pos_horizontal, int slider_pos_vertical, AW_pos x, AW_pos y, int number );
    void draw_drag_text( AW_device *device, int slider_pos_horizontal, int slider_pos_vertical, AW_pos x, AW_pos y );
};


void big_button_cb( AW_window *aw, AW_CL cd1, AW_CL cd2 );
