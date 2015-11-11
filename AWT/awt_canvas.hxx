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
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#ifndef AW_POSITION_HXX
#include <aw_position.hxx>
#endif


class AWT_canvas;
class AW_device;

enum AWT_COMMAND_MODE {
    AWT_MODE_NONE,
    AWT_MODE_EMPTY, // placeholder (currently used in PARSIMONY)

    // NTREE, PARSIMONY, GENEMAP and SECEDIT:
    AWT_MODE_ZOOM,

    // NTREE, PARSIMONY and GENEMAP:
    AWT_MODE_SELECT,

    // NTREE, PARSIMONY and SECEDIT:
    AWT_MODE_SETROOT,

    // NTREE and GENEMAP:
    AWT_MODE_INFO,

    // NTREE and PARSIMONY:
    AWT_MODE_MOVE,
    AWT_MODE_MARK,
    AWT_MODE_GROUP,
    AWT_MODE_LZOOM,
    AWT_MODE_SWAP,

    // NTREE and SECEDIT:
    AWT_MODE_ROTATE,

    // NTREE only:
    AWT_MODE_LINE,
    AWT_MODE_WWW,
    AWT_MODE_SPREAD,
    AWT_MODE_LENGTH,
    AWT_MODE_MULTIFURC,

    // PARSIMONY only:
    AWT_MODE_KERNINGHAN,
    AWT_MODE_NNI,
    AWT_MODE_OPTIMIZE,

    // SECEDIT only:
    AWT_MODE_FOLD,
    AWT_MODE_CURSOR,
    AWT_MODE_EDIT,
    AWT_MODE_PINFO,
    AWT_MODE_STRETCH,
    AWT_MODE_SET_CURSOR
};

#define STANDARD_PADDING 10

// --------------------------------------------------------------------------------
// AWT_zoom_mode + AWT_fit_mode are correlated, but not strictly coupled

enum AWT_zoom_mode { // bit values!
    AWT_ZOOM_NEVER = 0,
    AWT_ZOOM_X     = 1,
    AWT_ZOOM_Y     = 2,
    AWT_ZOOM_BOTH  = 3,
};

enum AWT_fit_mode {
    AWT_FIT_NEVER,
    AWT_FIT_LARGER, 
    AWT_FIT_SMALLER, 
    AWT_FIT_X, 
    AWT_FIT_Y, 
};

// used combinations are:
// AWT_ZOOM_NEVER + AWT_FIT_NEVER (NDS list, others)
// AWT_ZOOM_X + AWT_FIT_X (dendrogram tree)
// AWT_ZOOM_Y + AWT_FIT_Y
// AWT_ZOOM_BOTH + AWT_FIT_LARGER (radial tree/gene-map; secedit)
// AWT_ZOOM_BOTH + AWT_FIT_SMALLER (book-style gene-map)
//
// other combinations may work as well. some combinations make no sense.
// --------------------------------------------------------------------------------


class AWT_graphic_exports {
    AW_borders default_padding;
    AW_borders padding;

public:
    unsigned int refresh : 1;          // 1 -> do a refresh
    unsigned int resize : 1;           // 1 -> size of graphic might have changed (implies refresh)
    unsigned int structure_change : 1; // 1 -> call update_structure (implies resize) // @@@ rename -> need_reload
    unsigned int zoom_reset : 1;       // 1 -> do a zoom-reset (implies resize)
    unsigned int save : 1;             // 1 -> save structure to DB (implies structure_change)


    AWT_zoom_mode zoom_mode;
    AWT_fit_mode  fit_mode;

    unsigned int dont_scroll : 1; // normally 0 (1 for IRS tree)

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

    AW::Vector zoomVector(double transToFit) const {
        return AW::Vector(zoom_mode&AWT_ZOOM_X ? transToFit : 1.0,
                          zoom_mode&AWT_ZOOM_Y ? transToFit : 1.0);
    }
};

class AWT_graphic_event : virtual Noncopyable {
    AWT_COMMAND_MODE M_cmd;  // currently active mode

    AW_MouseButton M_button;
    AW_key_mod     M_key_modifier;
    AW_key_code    M_key_code;
    char           M_key_char;
    AW_event_type  M_type;

    AW::Position mousepos;

    AW_device_click *click_dev;

public:
    AWT_graphic_event(AWT_COMMAND_MODE cmd_, const AW_event& event, bool is_drag, AW_device_click *click_dev_)
        : M_cmd(cmd_),
          M_button(event.button),
          M_key_modifier(event.keymodifier),
          M_key_code(event.keycode),
          M_key_char(event.character),
          M_type(is_drag ? AW_Mouse_Drag : event.type),
          mousepos(event.x, event.y),
          click_dev(click_dev_)
    {}

    AWT_COMMAND_MODE cmd() const { return M_cmd; }
    AW_MouseButton button() const { return M_button; }

    AW_key_mod key_modifier() const { return M_key_modifier; }
    AW_key_code key_code() const { return M_key_code; }
    char key_char() const { return M_key_char; }

    AW_event_type type() const { return M_type; }

    const AW::Position& position() const { return mousepos; } // screen-coordinates

    const AW_clicked_element *best_click(AW_device_click::ClickPreference prefer = AW_device_click::PREFER_NEARER) {
        return click_dev ? click_dev->best_click(prefer) : NULL;
    }
};

class AWT_graphic {
    friend class AWT_canvas;

    void refresh_by_exports(AWT_canvas *scr);
    void postevent_handler(GBDATA *gb_main);

    bool detect_drag_target;

protected:
    int drag_gc;

public:
    AWT_graphic_exports exports;

    AWT_graphic() { exports.init(); }
    virtual ~AWT_graphic() {}

    // pure virtual interface (methods implemented by AWT_nonDB_graphic)

    virtual GB_ERROR load(GBDATA *gb_main, const char *name) = 0;
    virtual GB_ERROR save(GBDATA *gb_main, const char *name) = 0;
    virtual int  check_update(GBDATA *gb_main)               = 0; // check whether anything changed
    virtual void update(GBDATA *gb_main)                     = 0; // mark the database

    // pure virtual interface (rest)

    virtual void show(AW_device *device) = 0;

    /* init gcs, if any gc is changed you may call AWT_expose_cb(NULL, scr); or AWT_resize_cb(NULL, scr); */
    virtual AW_gc_manager init_devices(AW_window *, AW_device *, AWT_canvas *scr) = 0;

    virtual void handle_command(AW_device *device, AWT_graphic_event& event) = 0;
    virtual void update_structure()                                          = 0; // called when exports.structure_change == 1

    bool wants_drag_target() const { return detect_drag_target; }
    void drag_target_detection(bool detect) { detect_drag_target = detect; }
};

class AWT_nonDB_graphic : public AWT_graphic { // @@@ check AWT_nonDB_graphic
    void update_structure() {}
    // a partly implementation of AWT_graphic
public:
    AWT_nonDB_graphic() {}
    virtual ~AWT_nonDB_graphic() OVERRIDE {}

    // dummy functions, only spittings out warnings:
    GB_ERROR load(GBDATA *gb_main, const char *name) OVERRIDE __ATTR__USERESULT;
    GB_ERROR save(GBDATA *gb_main, const char *name) OVERRIDE __ATTR__USERESULT;
    int  check_update(GBDATA *gb_main) OVERRIDE;
    void update(GBDATA *gb_main) OVERRIDE;
};


#define EPS               0.0001 // div zero check
#define CLIP_OVERLAP      15
#define AWT_ZOOM_OUT_STEP 40    // (pixel) rand um screen
#define AWT_MIN_WIDTH     100   // Minimum center screen (= screen-offset)

enum {
    AWT_d_screen = 1
};

class AWT_canvas : virtual Noncopyable {
    bool consider_text_for_size;
    char *gc_base_name;

public:
    // too many callbacks -> public
    // in fact: private
    // (in real fact: needs rewrite)

    char   *user_awar; // contains name of awar (awar contains name of tree displayed in canvas)
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

    void set_scrollbars();
    void set_dragEndpoint(int x, int y);

    void set_horizontal_scrollbar_position(AW_window *aww, int pos);
    void set_vertical_scrollbar_position(AW_window *aww, int pos);


    // public (read only)

    GBDATA      *gb_main;
    AW_window   *aww;
    AW_root     *awr;
    AWT_graphic *gfx;

    AW_gc_manager gc_manager;
    int           drag_gc;

    AWT_COMMAND_MODE mode;

    // real public

    AWT_canvas(GBDATA *gb_main, AW_window *aww, const char *gc_base_name_, AWT_graphic *awd, const char *user_awar);

    inline void push_transaction() const;
    inline void pop_transaction() const;

    void refresh();

    void recalc_size(bool adjust_scrollbars = true);
    void recalc_size_and_refresh() { recalc_size(true); refresh(); }

    void zoom_reset();
    void zoom_reset_and_refresh() { zoom_reset(); refresh(); }

    void set_consider_text_for_zoom_reset(bool consider) { consider_text_for_size = consider; }

    void zoom(AW_device *device, bool zoomIn, const AW::Rectangle& wanted_part, const AW::Rectangle& current_part, int percent);

    void set_mode(AWT_COMMAND_MODE mo) { mode = mo; }

    void scroll(int delta_x, int delta_y, bool dont_update_scrollbars = false);
    void scroll(const AW::Vector& delta, bool dont_update_scrollbars = false) {
        scroll(int(delta.x()), int(delta.y()), dont_update_scrollbars);
    }

    bool handleWheelEvent(AW_device *device, const AW_event& event);

    const char *get_gc_base_name() const { return gc_base_name; }

    void postevent_handler();
};

inline void AWT_graphic::refresh_by_exports(AWT_canvas *scr) {
    if (exports.zoom_reset)   scr->zoom_reset_and_refresh();
    else if (exports.resize)  scr->recalc_size_and_refresh();
    else if (exports.refresh) scr->refresh();
}

void AWT_expose_cb(UNFIXED, AWT_canvas *scr);
void AWT_resize_cb(UNFIXED, AWT_canvas *scr);
void AWT_GC_changed_cb(GcChange whatChanged, AWT_canvas *scr);

void AWT_popup_tree_export_window(AW_window *parent_win, AWT_canvas *scr);
void AWT_popup_sec_export_window (AW_window *parent_win, AWT_canvas *scr);
void AWT_popup_print_window      (AW_window *parent_win, AWT_canvas *scr);


#endif
