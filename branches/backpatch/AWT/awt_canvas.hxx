#ifndef AWT_CANVAS_HXX
#define AWT_CANVAS_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif
#ifndef AW_DEVICE_HXX
#include <aw_device.hxx>
#endif

class AWT_canvas;
class AW_device;
class AW_clicked_line;
class AW_clicked_text;

typedef enum {
    AWT_MODE_NONE,
    AWT_MODE_SELECT,
    AWT_MODE_MARK,
    AWT_MODE_GROUP,
    AWT_MODE_ZOOM,          // no command
    AWT_MODE_LZOOM,
    AWT_MODE_EDIT, // species info
    AWT_MODE_WWW,
    AWT_MODE_LINE,
    AWT_MODE_ROT,
    AWT_MODE_SPREAD,
    AWT_MODE_SWAP,
    AWT_MODE_LENGTH,
    AWT_MODE_SWAP2,
    AWT_MODE_MOVE,
    AWT_MODE_SETROOT,
    AWT_MODE_RESET,

    AWT_MODE_KERNINGHAN,
    AWT_MODE_NNI,
    AWT_MODE_OPTIMIZE,
    AWT_MODE_PROINFO,
    AWT_MODE_STRETCH
} AWT_COMMAND_MODE;

class AWT_graphic_exports {
public:
    unsigned int zoom_reset : 1;
    unsigned int resize : 1;
    unsigned int refresh : 1;
    unsigned int save : 1;
    unsigned int structure_change : 1; // maybe useless
    unsigned int dont_fit_x : 1;
    unsigned int dont_fit_y : 1;
    unsigned int dont_fit_larger : 1; // if xsize>ysize -> dont_fit_x (otherwise dont_fit_y)
    unsigned int dont_scroll : 1;

    void init();     // like clear, but resets fit/scroll state
    void clear();

    short left_offset;
    short right_offset;
    short top_offset;
    short bottom_offset;
};

class AWT_graphic {
    friend class AWT_canvas;
protected:
    AW_rectangle extends;
    int drag_gc;
public:
    AWT_graphic_exports exports;

    AWT_graphic();
    virtual ~AWT_graphic();

    // pure virtual interface (methods implemented by AWT_nonDB_graphic)

    virtual GB_ERROR load(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2) = 0;
    virtual GB_ERROR save(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2) = 0;
    virtual int  check_update(GBDATA *gb_main)                                     = 0; // check whether anything changed
    virtual void update(GBDATA *gb_main)                                           = 0; // mark the database

    // pure virtual interface (rest)

    virtual void show(AW_device *device) = 0;

    virtual void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct) = 0;     /* double click */
    virtual AW_gc_manager init_devices(AW_window *, AW_device *, AWT_canvas *ntw, AW_CL cd2) = 0;
            /* init gcs, if any gc is changed you may call
                AWT_expose_cb(aw_window, ntw, cd2);
                or AWT_resize_cb(aw_window, ntw, cd2);
                The function may return a pointer to a preset window */

    // implemented interface (most are dummies doing nothing):

    virtual void push_transaction(GBDATA *gb_main);
    virtual void pop_transaction(GBDATA *gb_main);

    virtual void command(AW_device *device, AWT_COMMAND_MODE cmd,
                         int button, AW_key_mod key_modifier, AW_key_code key_code, char key_char,
                         AW_event_type type, AW_pos x, AW_pos y,
                         AW_clicked_line *cl, AW_clicked_text *ct);
    virtual void text(AW_device *device, char *text);
};

// a partly implementation of AWT_graphic
class AWT_nonDB_graphic : public AWT_graphic {
public:
    AWT_nonDB_graphic() {}
    virtual ~AWT_nonDB_graphic();

    // dummy functions, only spittings out warnings:
    GB_ERROR load(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2) __ATTR__USERESULT;
    GB_ERROR save(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2) __ATTR__USERESULT;
    int  check_update(GBDATA *gb_main);
    void update(GBDATA *gb_main);
};


#define EPS               0.0001 /* div zero check */
#define AWT_F_ALL         ((AW_active)-1)
#define CLIP_OVERLAP      15
#define AWT_CATCH_LINE    50    /* pixel */
#define AWT_CATCH_TEXT    5     /* pixel */
#define AWT_ZOOM_OUT_STEP 40    /* (pixel) rand um screen */
#define AWT_MIN_WIDTH     100   /* Minimum center screen (= screen-offset) */
enum {
    AWT_M_LEFT   = 1,
    AWT_M_MIDDLE = 2,
    AWT_M_RIGHT  = 3
};

enum {
    AWT_d_screen = 1
};

class AWT_canvas {
public:
    // too many callbacks -> public
    // in fact: private
    // (in real fact: needs rewrite)

    char   *user_awar;
    void    init_device(AW_device *device);
    AW_pos  trans_to_fit;
    AW_pos  shift_x_to_fit;
    AW_pos  shift_y_to_fit;

    int old_hor_scroll_pos;
    int old_vert_scroll_pos;
    AW_rectangle rect;  // screen coordinates
    AW_world worldinfo; // real coordinates without transform.
    AW_world worldsize;
    int zoom_drag_sx;
    int zoom_drag_sy;
    int zoom_drag_ex;
    int zoom_drag_ey;
    int drag;
    AW_clicked_line clicked_line;
    AW_clicked_text clicked_text;

    void set_scrollbars();
    void set_dragEndpoint(int x, int y);

    void set_horizontal_scrollbar_position(AW_window *aww, int pos);
    void set_vertical_scrollbar_position(AW_window *aww, int pos);


    // public (read only)

    GBDATA      *gb_main;
    AW_window   *aww;
    AW_root     *awr;
    AWT_graphic *tree_disp;

    AW_gc_manager gc_manager;
    int           drag_gc;

    AWT_COMMAND_MODE mode;

    // real public

    AWT_canvas(GBDATA *gb_main, AW_window *aww, AWT_graphic *awd, AW_gc_manager &gc_manager, const char *user_awar);

    void refresh();
    void recalc_size();         // Calculate the size of the sb
    void zoom_reset();          // Calculate all
    void zoom(AW_device *device, bool zoomIn, const AW::Rectangle& wanted_part, const AW::Rectangle& current_part);

    void set_mode(AWT_COMMAND_MODE mo) { mode = mo; }

    void scroll(AW_window *aww, int delta_x, int delta_y, bool dont_update_scrollbars = false);
    void scroll(AW_window *aw, const AW::Vector& delta, bool dont_update_scrollbars = false) {
        scroll(aw, int(delta.x()), int(delta.y()), dont_update_scrollbars);
    }
};

void AWT_expose_cb(AW_window *dummy, AWT_canvas *ntw, AW_CL cl2);
void AWT_resize_cb(AW_window *dummy, AWT_canvas *ntw, AW_CL cl2);

#define AWAR_PRINT_TREE                "NT/print/"
#define AWAR_PRINT_TREE_LANDSCAPE      AWAR_PRINT_TREE "landscape"
#define AWAR_PRINT_TREE_MAGNIFICATION  AWAR_PRINT_TREE "magnification"
#define AWAR_PRINT_TREE_CLIP           AWAR_PRINT_TREE "clip"
#define AWAR_PRINT_TREE_HANDLES        AWAR_PRINT_TREE "handles"
#define AWAR_PRINT_TREE_COLOR          AWAR_PRINT_TREE "color"
#define AWAR_PRINT_TREE_DEST           AWAR_PRINT_TREE "dest"
#define AWAR_PRINT_TREE_PRINTER        AWAR_PRINT_TREE "printer"
#define AWAR_PRINT_TREE_OVERLAP        AWAR_PRINT_TREE "overlap"
#define AWAR_PRINT_TREE_OVERLAP_AMOUNT AWAR_PRINT_TREE "amount"

#define AWAR_PRINT_TREE_TMP    "tmp/" AWAR_PRINT_TREE
#define AWAR_PRINT_TREE_GSIZEX AWAR_PRINT_TREE_TMP "gsizex" // graphic size
#define AWAR_PRINT_TREE_GSIZEY AWAR_PRINT_TREE_TMP "gsizey"
#define AWAR_PRINT_TREE_PSIZEX AWAR_PRINT_TREE_TMP "psizex" // print size
#define AWAR_PRINT_TREE_PSIZEY AWAR_PRINT_TREE_TMP "psizey"
#define AWAR_PRINT_TREE_SIZEX  AWAR_PRINT_TREE_TMP "sizex" // size in pages
#define AWAR_PRINT_TREE_SIZEY  AWAR_PRINT_TREE_TMP "sizey"
#define AWAR_PRINT_TREE_PAGES  AWAR_PRINT_TREE_TMP "pages"

#define AWAR_PRINT_TREE_FILE_BASE   AWAR_PRINT_TREE_TMP "file"
#define AWAR_PRINT_TREE_FILE_NAME   AWAR_PRINT_TREE_FILE_BASE "/file_name"
#define AWAR_PRINT_TREE_FILE_DIR    AWAR_PRINT_TREE_FILE_BASE "/directory"
#define AWAR_PRINT_TREE_FILE_FILTER AWAR_PRINT_TREE_FILE_BASE "/filter"

void AWT_popup_tree_export_window(AW_window *parent_win, AW_CL cl_canvas, AW_CL);
void AWT_popup_sec_export_window (AW_window *parent_win, AW_CL cl_canvas, AW_CL);
void AWT_popup_print_window      (AW_window *parent_win, AW_CL cl_canvas, AW_CL);


#endif
