// =========================================================== //
//                                                             //
//   File      : PH_display.hxx                                //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef PH_DISPLAY_HXX
#define PH_DISPLAY_HXX

#ifndef AW_DEVICE_HXX
#include <aw_device.hxx>
#endif

#define SPECIES_NAME_LEN 10    // only for displaying speciesnames

extern const char *filter_text[];

class PH_display {
    AW_device    *device;                // device to draw in
    AW_pos        screen_width;          // dimensions of main screen
    AW_pos        screen_height;
    long          cell_width;            // width and height of one cell
    long          cell_height;
    long          cell_offset;           // don't write on base line of cell
    long          horiz_page_start;      // number of first element in this page
    long          vert_page_start;
    long          vert_last_view_start;  // last value of scrollbars
    long          horiz_last_view_start;
    long          total_cells_vert;
    display_type  display_what;
    long          horiz_page_size;       // page size in cells (how many cells per page,
    long          vert_page_size;        // vertical and horizontal)
    long          off_dx;                // offset values for devic.shift_dx(y)
    long          off_dy;

    void set_scrollbar_steps(AW_window *, long, long, long, long);

public:
    PH_display();                   // constructor
    static PH_display *ph_display;

    void initialize_display(display_type);
    void display();                      // display data (according to display_type)
    display_type displayed() { return display_what; };

    void clear_window() { if (device) device->clear(-1); };
    void resized();                      // call after resize main window
    void monitor_vertical_scroll_cb(AW_window *);    // vertical and horizontal
    void monitor_horizontal_scroll_cb(AW_window *);  // scrollbar movements
};


class PH_display_status {
    AW_device *device;
    short font_width;
    short font_height;
    AW_pos max_x;                           // screensize
    AW_pos max_y;
    AW_pos x_pos;                           // position of cursor
    AW_pos y_pos;
    AW_pos tab_pos;

public:
    PH_display_status(AW_device *);

    void set_origin() { device->reset(); device->set_offset(AW::Vector(font_width, font_height)); }
    void newline() { x_pos = 0; y_pos+=(y_pos>=max_y) ? 0.0 : 1.0; }
    AW_pos get_size(char c) { return ((c=='x') ? max_x : max_y); }
    void set_cursor(AW_pos x, AW_pos y) { x_pos = (x<=max_x) ? x : x_pos; y_pos=(y<=max_y) ? y : y_pos; }

    void move_x(AW_pos d) { x_pos += (x_pos+d<=max_x) ? d : 0; }
    void set_cursor_x(AW_pos x) { x_pos = x; }

    void set_tab() { tab_pos = x_pos; }
    AW_pos get_tab() { return tab_pos; }

    void write(const char *);   // text to write
    void writePadded(const char *, size_t len);   // write text padded
    void write(long);           // converts long

    void clear();
};


void ph_view_species_cb();
void ph_calc_filter_cb();
GB_ERROR ph_check_initialized();

#else
#error PH_display.hxx included twice
#endif // PH_DISPLAY_HXX
