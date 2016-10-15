// =============================================================== //
//                                                                 //
//   File      : di_view_matrix.hxx                                //
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
#ifndef DI_MATR_HXX
#include "di_matr.hxx"
#endif

enum DI_gc {
    DI_G_STANDARD,
    DI_G_NAMES,
    DI_G_RULER,
    DI_G_RULER_DISPLAY,
    DI_G_BELOW_DIST,
    DI_G_ABOVE_DIST,
    DI_G_LAST                   // must be last
};

class MatrixDisplay {
    int screen_width;             // dimensions of main screen
    int screen_height;

    int cell_width;               // width and height of one cell
    int cell_height;

    int cell_paddx;               // padding between cells
    int cell_paddy;

    bool leadZero;                // show leading zero
    int  digits;                  // digits after '.'
    char format_string[50];       // for digits

    int horiz_page_start;         // number of first element in this page
    int vert_page_start;

    int vert_last_view_start;     // last value of scrollbars
    int horiz_last_view_start;

    int total_cells_horiz;
    int total_cells_vert;

    int horiz_page_size;          // page size in cells (how many cells per page,
    int vert_page_size;           // vertical and horizontal)

    int off_dx;                   // offset values for scrollable area
    int off_dy;

    double min_view_dist;         // m[i][j]<min_view_dist -> ascii ; else small slider
    double max_view_dist;         // m[i][j]>max_view_dist -> ascii ; else small slider

    void set_scrollbar_steps(long width, long hight, long xinc, long yinc);
    void scroll_to(int sxpos, int sypos);

public:
    enum UpdateFlag { // bits
        NEED_NOTHING = 0,
        NEED_CLEAR   = 1,
        NEED_RESIZE  = 2,
        NEED_SETUP   = 4,
    };
private:
    UpdateFlag beforeUpdate;

    void setup();
    void adapt_to_canvas_size();                                 // call after resize main window
    void draw();

    enum LastAutoPop { UNKNOWN, UP, DOWN };
    LastAutoPop lastautopop;

    bool autopop(bool show) {
        // handle automatic hide/show of matrix view
        // - avoid popup if was not auto-hidden
        if (!awm) return false;
        if (!show) {
            if (awm->is_shown()) {
                awm->hide(); lastautopop = DOWN;
            }
        }
        else if (!awm->is_shown() && lastautopop == DOWN) {
            awm->show(); lastautopop = UP;
        }
        return awm->is_shown();
    }

public:
    AW_window *awm;
    AW_device *device;          // device to draw in

    MatrixDisplay()
        : screen_width(0),
          screen_height(0),
          cell_width(0),
          cell_height(0),
          cell_paddx(0),
          cell_paddy(0),
          leadZero(false),
          digits(0),
          horiz_page_start(0),
          vert_page_start(0),
          vert_last_view_start(0),
          horiz_last_view_start(0),
          total_cells_horiz(0),
          total_cells_vert(0),
          horiz_page_size(0),
          vert_page_size(0),
          off_dx(0),
          off_dy(0),
          min_view_dist(0.0),
          max_view_dist(0.0),
          beforeUpdate(NEED_SETUP),
          lastautopop(UNKNOWN),
          awm(NULL),
          device(NULL)
    {}

    DI_MATRIX *get_matrix() { return GLOBAL_MATRIX.get(); }

    bool willShow() {
        if (!awm) return false;
        return awm->is_shown() || lastautopop == DOWN;
    }

    void monitor_vertical_scroll_cb(AW_window *);   // vertical and horizontal
    void monitor_horizontal_scroll_cb(AW_window *); // scrollbar movements

    void mark(UpdateFlag needed) { beforeUpdate = UpdateFlag(beforeUpdate|needed); }

    void update_display() {
        if (beforeUpdate&NEED_SETUP) setup();
        if (beforeUpdate&NEED_RESIZE) adapt_to_canvas_size();
        draw();
        beforeUpdate = NEED_NOTHING;
    }

    // ******************** real public section *******************
    void set_slider_min(double d) { min_view_dist = d; };
    void set_slider_max(double d) { max_view_dist = d; };

    void handle_move(AW_event& event);
    void scroll_cells(int cells_x, int cells_y);
};

AW_window *DI_create_view_matrix_window(AW_root *awr, MatrixDisplay *disp, save_matrix_params *sparam);


#else
#error di_view_matrix.hxx included twice
#endif // DI_VIEW_MATRIX_HXX
