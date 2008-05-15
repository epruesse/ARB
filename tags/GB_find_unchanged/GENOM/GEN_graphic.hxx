/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef GEN_GRAPHIC_HXX
#define GEN_GRAPHIC_HXX

#include <aw_color_groups.hxx>

enum {
    GEN_GC_DEFAULT    = 0,
    GEN_GC_FIRST_FONT = GEN_GC_DEFAULT,

    GEN_GC_GENE,
    GEN_GC_MARKED,
    GEN_GC_CURSOR,

    GEN_GC_LAST_FONT = GEN_GC_CURSOR,

    GEN_GC_FIRST_COLOR_GROUP,

    GEN_GC_MAX = GEN_GC_FIRST_COLOR_GROUP+AW_COLOR_GROUPS

};                              // AW_gc

typedef enum {
    GEN_DISPLAY_STYLE_RADIAL,
    GEN_DISPLAY_STYLE_BOOK,
    GEN_DISPLAY_STYLE_VERTICAL,

    GEN_DISPLAY_STYLES // counter
} GEN_DisplayStyle;


//  -----------------------------------------------
//      class GEN_graphic : public AWT_graphic
//  -----------------------------------------------

typedef void (*GEN_graphic_cb_installer)(bool install, AWT_canvas*, GEN_graphic*);

class GEN_graphic : public AWT_nonDB_graphic {
    AW_root                  *aw_root;
    GBDATA                   *gb_main;
    GEN_graphic_cb_installer  callback_installer;
    int                       window_nr;
    GEN_root                 *gen_root;
    GEN_DisplayStyle          style;
    bool                      want_zoom_reset; // true -> do zoom reset on next refresh

    void delete_gen_root(AWT_canvas *ntw);

protected:

    // variables - tree compatibility
    AW_clicked_line rot_cl;
    AW_clicked_text rot_ct;
    AW_clicked_line old_rot_cl;

    AW_device *disp_device;     // device for recursive functions

public:
    GEN_graphic(AW_root *aw_root, GBDATA *gb_main, GEN_graphic_cb_installer callback_installer_, int window_nr_);
    virtual ~GEN_graphic();

    void reinit_gen_root(AWT_canvas *ntw, bool force_reinit);

    void set_display_style(GEN_DisplayStyle type);
    GEN_DisplayStyle get_display_style() const { return style; }

    virtual	AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *ntw,AW_CL);

    virtual	void show(AW_device *device);
    virtual void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);
    virtual void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, AW_key_code key_code, char key_char, AW_event_type type, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);

    virtual int check_update(GBDATA *gbdummy);

    AW_root *get_aw_root() const { return aw_root; }
    GBDATA *get_gb_main() const { return gb_main; }
    const GEN_root *get_gen_root() const { return gen_root; }
    AW_device *get_device() const { return disp_device; }
};

#else
#error GEN_graphic.hxx included twice
#endif // GEN_GRAPHIC_HXX
