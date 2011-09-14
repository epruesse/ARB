#ifndef AWT_CANVAS_HXX
#define AWT_CANVAS_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif
#ifndef AW_DEVICE_HXX
#include <aw_device.hxx>
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif

class AWT_canvas;
class AW_device;
class AW_clicked_line;
class AW_clicked_text;

enum AWT_COMMAND_MODE {
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
    AWT_MODE_MOVE,
    AWT_MODE_SETROOT,
    AWT_MODE_RESET,

    AWT_MODE_KERNINGHAN,
    AWT_MODE_NNI,
    AWT_MODE_OPTIMIZE,
    AWT_MODE_PROINFO,
    AWT_MODE_STRETCH
};

#define STANDARD_PADDING 10

class AWT_graphic_exports {
    AW_borders default_padding;
    AW_borders padding;

public:
    unsigned int refresh : 1;          // 1 -> do a refresh
    unsigned int resize : 1;           // 1 -> size of graphic might have changed (implies refresh)
    unsigned int structure_change : 1; // 1 -> call update_structure (implies resize)
    unsigned int zoom_reset : 1;       // 1 -> do a zoom-reset (implies resize)
    unsigned int save : 1;             // 1 -> save structure to DB (implies structure_change)

    unsigned int dont_fit_x : 1;
    unsigned int dont_fit_y : 1;
    unsigned int dont_fit_larger : 1;  // if xsize>ysize -> dont_fit_x (otherwise dont_fit_y)
    unsigned int dont_scroll : 1;

    void init();     // like clear, but resets fit, scroll state and padding
    void clear();

    void set_default_padding(int t, int b, int l, int r) {
        default_padding.t = t;
        default_padding.b = b;
        default_padding.l = l;
        default_padding.r = r;

        padding = default_padding;
    }

    void set_equilateral_default_padding(int pad) { set_default_padding(pad, pad, pad, pad); }
    void set_standard_default_padding() { set_equilateral_default_padding(STANDARD_PADDING); }

    void set_extra_text_padding(const AW_borders& text_padding) {
        padding.t = default_padding.t + text_padding.t;
        padding.b = default_padding.b + text_padding.b;
        padding.l = default_padding.l + text_padding.l;
        padding.r = default_padding.r + text_padding.r;
    }

    int get_x_padding() const { return padding.l+padding.r; }
    int get_y_padding() const { return padding.t+padding.b; }
    int get_top_padding() const { return padding.t; }
    int get_left_padding() const { return padding.l; }
};

enum AWT_Mouse_Button {
    AWT_M_LEFT   = 1,
    AWT_M_MIDDLE = 2,
    AWT_M_RIGHT  = 3
};

class AWT_graphic_event : virtual Noncopyable {
    AWT_COMMAND_MODE M_cmd;  // currently active mode

    AWT_Mouse_Button M_button;
    AW_key_mod       M_key_modifier;
    AW_key_code      M_key_code;
    char             M_key_char;
    AW_event_type    M_type;

    AW::Position mousepos;

    const AW_clicked_line *M_cl; // text and/or
    const AW_clicked_text *M_ct; // line selected by current mouse position

public:
    AWT_graphic_event(AWT_COMMAND_MODE cmd_, const AW_event& event, bool is_drag, const AW_clicked_line  *cl_, const AW_clicked_text  *ct_)
        : M_cmd(cmd_),
          M_button((AWT_Mouse_Button)event.button),
          M_key_modifier(event.keymodifier),
          M_key_code(event.keycode),
          M_key_char(event.character),
          M_type(is_drag ? AW_Mouse_Drag : event.type),
          mousepos(event.x, event.y), 
          M_cl(cl_),
          M_ct(ct_)
    {}

    AWT_COMMAND_MODE cmd() const { return M_cmd; }
    AWT_Mouse_Button button() const { return M_button; }

    AW_key_mod key_modifier() const { return M_key_modifier; }
    AW_key_code key_code() const { return M_key_code; }
    char key_char() const { return M_key_char; }
    
    AW_event_type type() const { return M_type; }

    AW_pos x() const __ATTR__DEPRECATED("use position()") { return mousepos.xpos(); }
    AW_pos y() const __ATTR__DEPRECATED("use position()") { return mousepos.ypos(); }
    const AW::Position& position() const { return mousepos; } // screen-coordinates

    const AW_clicked_line *cl() const __ATTR__DEPRECATED("use best_click()") { return M_cl; } 
    const AW_clicked_text *ct() const __ATTR__DEPRECATED("use best_click()") { return M_ct; } 
    const AW_clicked_element *best_click() const { return AW_getBestClick(M_cl, M_ct); }
};

class AWT_graphic {
    friend class AWT_canvas;

    void refresh_by_exports(AWT_canvas *ntw);

protected:
    int drag_gc;

public:
    AWT_graphic_exports exports;

    AWT_graphic() { exports.init(); }
    virtual ~AWT_graphic() {}
    
    // pure virtual interface (methods implemented by AWT_nonDB_graphic)

    virtual GB_ERROR load(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2) = 0;
    virtual GB_ERROR save(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2) = 0;
    virtual int  check_update(GBDATA *gb_main)                                     = 0; // check whether anything changed
    virtual void update(GBDATA *gb_main)                                           = 0; // mark the database

    // pure virtual interface (rest)

    virtual void show(AW_device *device) = 0;

    virtual void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct) = 0;     // double click
    virtual AW_gc_manager init_devices(AW_window *, AW_device *, AWT_canvas *ntw, AW_CL cd2) = 0;
            /* init gcs, if any gc is changed you may call
                AWT_expose_cb(aw_window, ntw, cd2);
                or AWT_resize_cb(aw_window, ntw, cd2);
                The function may return a pointer to a preset window */

    virtual void handle_command(AW_device *device, AWT_graphic_event& event) = 0;
    virtual void update_structure()                                          = 0; // called when exports.structure_change == 1
};

class AWT_nonDB_graphic : public AWT_graphic {
    void update_structure() {}
    // a partly implementation of AWT_graphic
public:
    AWT_nonDB_graphic() {}
    virtual ~AWT_nonDB_graphic() {}

    // dummy functions, only spittings out warnings:
    GB_ERROR load(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2) __ATTR__USERESULT;
    GB_ERROR save(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2) __ATTR__USERESULT;
    int  check_update(GBDATA *gb_main);
    void update(GBDATA *gb_main);
};


#define EPS          0.0001      // div zero check
#define CLIP_OVERLAP 15

#define AWT_ZOOM_OUT_STEP 40    // (pixel) rand um screen
#define AWT_MIN_WIDTH     100   // Minimum center screen (= screen-offset)
enum {
    AWT_d_screen = 1
};

class AWT_canvas : virtual Noncopyable {
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
    AW_screen_area rect;  // screen coordinates
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

    inline void push_transaction() const;
    inline void pop_transaction() const;

    void refresh();

    void recalc_size(bool adjust_scrollbars = true);
    void recalc_size_and_refresh() { recalc_size(true); refresh(); }

    void zoom_reset();
    void zoom_reset_and_refresh() { zoom_reset(); refresh(); }

    void refresh_by_exports() { tree_disp->refresh_by_exports(this); }

    void zoom(AW_device *device, bool zoomIn, const AW::Rectangle& wanted_part, const AW::Rectangle& current_part);

    void set_mode(AWT_COMMAND_MODE mo) { mode = mo; }

    void scroll(AW_window *aww, int delta_x, int delta_y, bool dont_update_scrollbars = false);
    void scroll(AW_window *aw, const AW::Vector& delta, bool dont_update_scrollbars = false) {
        scroll(aw, int(delta.x()), int(delta.y()), dont_update_scrollbars);
    }
};

inline void AWT_graphic::refresh_by_exports(AWT_canvas *ntw) {
    if (exports.zoom_reset)   ntw->zoom_reset_and_refresh();
    else if (exports.resize)  ntw->recalc_size_and_refresh();
    else if (exports.refresh) ntw->refresh();
}


void AWT_expose_cb(AW_window *dummy, AWT_canvas *ntw, AW_CL cl2);
void AWT_resize_cb(AW_window *dummy, AWT_canvas *ntw, AW_CL cl2);

void AWT_popup_tree_export_window(AW_window *parent_win, AW_CL cl_canvas, AW_CL);
void AWT_popup_sec_export_window (AW_window *parent_win, AW_CL cl_canvas, AW_CL);
void AWT_popup_print_window      (AW_window *parent_win, AW_CL cl_canvas, AW_CL);


#endif
