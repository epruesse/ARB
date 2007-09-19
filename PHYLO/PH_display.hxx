#ifndef AW_DEVICE_HXX
#include <aw_device.hxx>
#endif

#define SPECIES_NAME_LEN 10    // only for displaying speciesnames
 
extern char **filter_text;

class AP_display{
    AW_device *device;                   // device to draw in
    AW_pos screen_width;                 // dimensions of main screen
    AW_pos screen_height;
    long cell_width;                     // width and height of one cell
    long cell_height;
    long cell_offset;                    // don't write on base line of cell
    long horiz_page_start;               // number of first element in this page 
    long vert_page_start;          
    long vert_last_view_start;           // last value of scrollbars
    long horiz_last_view_start;
    long total_cells_horiz;
    long total_cells_vert;
    display_type display_what;         
    long horiz_page_size;                // page size in cells (how many cells per page,
    long vert_page_size;                 // vertical and horizontal)
    long off_dx;                         // offset values for devic.shift_dx(y)
    long off_dy; 
          
    void set_scrollbar_steps(AW_window *,long,long,long,long); 
    void print(void);                    // print private parameters (debugging)
          

public:
    AP_display(void);               // constructor
    static AP_display *apdisplay;
    void initialize(display_type);
    void display(void);                  // display data (according to display type: matrix ...)
    display_type displayed(void) { return display_what; };
    void clear_window(void) { if(device) device->clear(-1); };
    void resized(void);                  // call after resize main window
    void monitor_vertical_scroll_cb(AW_window *);    // vertical and horizontal
    void monitor_horizontal_scroll_cb(AW_window *);  // scrollbar movements
};


class AP_display_status
{
    AW_device *device;
    short font_width; 
    short font_height;
    AW_pos max_x;                           // screensize
    AW_pos max_y;
    AW_pos x_pos;                           // position of cursor
    AW_pos y_pos;
    AW_pos tab_pos;

public:
    AP_display_status(AW_device *);
    
    void set_origin(void) { device->reset(); device->set_offset(AW::Vector(font_width, font_height)); }
    void newline(void) { x_pos = 0; y_pos+=(y_pos>=max_y) ? 0.0 : 1.0; }
    AW_pos get_size(char c) { return((c=='x') ? max_x : max_y); }
    void set_cursor(AW_pos x,AW_pos y) { x_pos = (x<=max_x) ? x : x_pos; y_pos=(y<=max_y) ? y : y_pos; }
    void move_x(AW_pos d) { x_pos += (x_pos+d<=max_x) ? d : 0; }
    void move_y(AW_pos d) { y_pos+=(y_pos+d<=max_y) ? d : 0; }
    void set_cursor_x(AW_pos x) { x_pos = x; }
    void set_cursor_y(AW_pos y) { y_pos = y; }
    AW_pos get_cursor_pos(char c) { return((c=='x') ? x_pos : y_pos); }
    void set_tab(void) { tab_pos = x_pos; }
    AW_pos get_tab(void) { return tab_pos; }
    
    void write(const char *);   // text to write
    void writePadded(const char *, size_t len);   // write text padded
    void write(long);           // converts long
    void write(double);         // float to text
    void clear(void);
};





