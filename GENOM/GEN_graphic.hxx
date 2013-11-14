// =============================================================== //
//                                                                 //
//   File      : GEN_graphic.hxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 2001           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GEN_GRAPHIC_HXX
#define GEN_GRAPHIC_HXX

#ifndef AW_COLOR_GROUPS_HXX
#include <aw_color_groups.hxx>
#endif
#ifndef AWT_CANVAS_HXX
#include <awt_canvas.hxx>
#endif

enum {
    GEN_GC_DEFAULT    = 0,
    GEN_GC_FIRST_FONT = GEN_GC_DEFAULT,

    GEN_GC_GENE,
    GEN_GC_MARKED,
    GEN_GC_CURSOR,

    GEN_GC_LAST_FONT = GEN_GC_CURSOR,

    GEN_GC_FIRST_COLOR_GROUP,

    GEN_GC_MAX = GEN_GC_FIRST_COLOR_GROUP+AW_COLOR_GROUPS

};

enum GEN_DisplayStyle {
    GEN_DISPLAY_STYLE_RADIAL,
    GEN_DISPLAY_STYLE_BOOK,
    GEN_DISPLAY_STYLE_VERTICAL,
};

#define GEN_DISPLAY_STYLES (GEN_DISPLAY_STYLE_VERTICAL+1)


// ---------------------
//      GEN_graphic

typedef void (*GEN_graphic_cb_installer)(bool install, AWT_canvas*, GEN_graphic*);

class GEN_graphic : public AWT_nonDB_graphic, virtual Noncopyable {
    AW_root                  *aw_root;
    GBDATA                   *gb_main;
    GEN_graphic_cb_installer  callback_installer;
    int                       window_nr;
    GEN_root                 *gen_root;
    GEN_DisplayStyle          style;
    bool                      want_zoom_reset; // true -> do zoom reset on next refresh

    void delete_gen_root(AWT_canvas *scr);

    void update_structure() {}

protected:

    AW_device *disp_device;     // device for recursive functions

public:
    GEN_graphic(AW_root *aw_root, GBDATA *gb_main, GEN_graphic_cb_installer callback_installer_, int window_nr_);
    virtual ~GEN_graphic() OVERRIDE;

    void reinit_gen_root(AWT_canvas *scr, bool force_reinit);

    void set_display_style(GEN_DisplayStyle type);
    GEN_DisplayStyle get_display_style() const { return style; }

    AW_gc_manager init_devices(AW_window *, AW_device *, AWT_canvas *scr) OVERRIDE;

    virtual void show(AW_device *device) OVERRIDE;
    virtual void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct) OVERRIDE;

    void handle_command(AW_device *device, AWT_graphic_event& event) OVERRIDE;
    virtual int check_update(GBDATA *gbdummy) OVERRIDE;

    AW_root *get_aw_root() const { return aw_root; }
    GBDATA *get_gb_main() const { return gb_main; }
    const GEN_root *get_gen_root() const { return gen_root; }
    AW_device *get_device() const { return disp_device; }
};

#else
#error GEN_graphic.hxx included twice
#endif // GEN_GRAPHIC_HXX
