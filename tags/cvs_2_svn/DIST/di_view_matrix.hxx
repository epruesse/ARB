// =============================================================== //
//                                                                 //
//   File      : di_view_matrix.hxx                                //
//   Time-stamp: <Thu Jul/03/2008 16:21 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef DI_VIEW_MATRIX_HXX
#define DI_VIEW_MATRIX_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

#define SPECIES_NAME_LEN 10    // only for displaying speciesnames

typedef enum  {
    DI_G_STANDARD,
    DI_G_NAMES,
    DI_G_RULER,
    DI_G_RULER_DISPLAY,
    DI_G_BELOW_DIST,
    DI_G_ABOVE_DIST,
    DI_G_LAST                   // must be last
} DI_gc;

class DI_dmatrix {
    AW_pos  screen_width;         // dimensions of main screen
    AW_pos  screen_height;
    long    cell_width;           // width and height of one cell
    long    cell_height;
    long    cell_offset;          // don't write on base line of cell
    long    horiz_page_start;     // number of first element in this page
    long    vert_page_start;
    long    vert_last_view_start; // last value of scrollbars
    long    horiz_last_view_start;
    long    total_cells_horiz;
    long    total_cells_vert;
    long    horiz_page_size;      // page size in cells (how many cells per page,
    long    vert_page_size;       // vertical and horizontal)
    long    off_dx;               // offset values for devic.shift_dx(y)
    long    off_dy;
    double  min_view_dist;        // m[i][j]<min_view_dist -> ascii ; else small slider
    double  max_view_dist;        // m[i][j]>max_view_dist -> ascii ; else small slider

    void set_scrollbar_steps(long width,long hight,long xinc,long yinc);

public:
    AW_window *awm;
    AW_device *device;          // device to draw in
    DI_MATRIX  *di_matrix;       // the Matrix

    DI_MATRIX *get_matrix();

    void monitor_vertical_scroll_cb(AW_window *);   // vertical and horizontal
    void monitor_horizontal_scroll_cb(AW_window *); // scrollbar movements
    void display(bool clear);                       // display data
    void resized(void);                             // call after resize main window

    // ******************** real public section *******************
    void set_slider_min(double d) { min_view_dist = d; };
    void set_slider_max(double d) { max_view_dist = d; };

    void handle_move(AW_event& event);
    
    DI_dmatrix();
    void init(DI_MATRIX *matrix=0); // set the output matrix

    // if matrix == 0, use DI_MATRIX::root
};

AW_window *DI_create_view_matrix_window(AW_root *awr, DI_dmatrix *dmatrix);


#else
#error di_view_matrix.hxx included twice
#endif // DI_VIEW_MATRIX_HXX
