/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef GEN_GRAPHIC_HXX
#define GEN_GRAPHIC_HXX


enum {
	GEN_GC_DEFAULT    = 0,
    GEN_GC_FIRST_FONT = GEN_GC_DEFAULT,

	GEN_GC_GENE,
	GEN_GC_MARKED,
    GEN_GC_CURSOR,

    GEN_GC_LAST_FONT = GEN_GC_CURSOR,

	GEN_GC_MAX
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
class GEN_graphic : public AWT_graphic {
protected:

    // variables - tree compatibility
    AW_clicked_line rot_cl;
    AW_clicked_text rot_ct;
    AW_clicked_line old_rot_cl;

    AW_device        *disp_device; // device for  rekursiv Funktions
    GEN_DisplayStyle  style;
    void (*callback_installer)(bool install, AWT_canvas*);

public:
    GBDATA   *gb_main;
    AW_root  *aw_root;
    GEN_root *gen_root;
    int       change_flag;

    GEN_graphic(AW_root *aw_root, GBDATA *gb_main, void (*callback_installer_)(bool install, AWT_canvas*));
    virtual ~GEN_graphic();

    void delete_gen_root(AWT_canvas *ntw); // reinit_gen_root is normally called afterwards
    void reinit_gen_root(AWT_canvas *ntw);

    void set_display_style(GEN_DisplayStyle type);
    GEN_DisplayStyle get_display_style() const { return style; }

    virtual	AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *ntw,AW_CL);

    virtual	void show(AW_device *device);
    virtual void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);
    virtual void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_event_type type, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);

    virtual int GEN_graphic::check_update(GBDATA *gbdummy);
};

extern GEN_graphic *GEN_GRAPHIC;

#else
#error GEN_graphic.hxx included twice
#endif // GEN_GRAPHIC_HXX
