#ifndef RNA3D_GRAPHICS_HXX
#define RNA3D_GRAPHICS_HXX

#define RNA3D_assert(cond) arb_assert(cond)



enum {
    RNA3D_GC_MOL_BACKBONE,
    RNA3D_GC_BASES_HELIX,
    RNA3D_GC_BASES_UNPAIRED_HELIX,
    RNA3D_GC_BASES_NON_HELIX,
    RNA3D_GC_HELIX,
    RNA3D_GC_HELIX_SKELETON,
    RNA3D_GC_HELIX_MIDPOINT,

    RNA3D_GC_MAX
};

class RNA3D_Graphics: public AWT_graphic {
public:

    GBDATA     *gb_main;
    AW_root    *aw_root;

    RNA3D_Graphics(AW_root *aw_root, GBDATA *gb_main);
    ~RNA3D_Graphics(void);

    AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *ntw,AW_CL);

    void show(AW_device *device);
    void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);
    void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, char key_char,
                 AW_event_type type, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);
    void paint(AW_device *device);

};

#else
#error RNA3D_Graphics.hxx included twice
#endif

