#ifndef RNA3D_GRAPHICS_HXX
#define RNA3D_GRAPHICS_HXX

#define RNA3D_assert(cond) arb_assert(cond)



enum {
    RNA3D_GC_FOREGROUND,
    RNA3D_GC_MOL_BACKBONE,    
    RNA3D_GC_MAPPED_SPECIES,
    RNA3D_GC_DELETION,
    RNA3D_GC_MOL_POS,
    RNA3D_GC_BASES_HELIX,
    RNA3D_GC_BASES_UNPAIRED_HELIX,
    RNA3D_GC_BASES_NON_HELIX,
    RNA3D_GC_HELIX,
    RNA3D_GC_HELIX_SKELETON,
    RNA3D_GC_HELIX_MIDPOINT,
    RNA3D_GC_CURSOR_POSITION,

    RNA3D_GC_SBACK_0, // User 1  // Background for search 
    RNA3D_GC_SBACK_1,  // User 2
    RNA3D_GC_SBACK_2,  // Probe
    RNA3D_GC_SBACK_3,  // Primer (local)
    RNA3D_GC_SBACK_4,  // Primer (region)
    RNA3D_GC_SBACK_5,  // Primer (global)
    RNA3D_GC_SBACK_6,  // Signature (local)
    RNA3D_GC_SBACK_7,  // Signature (region)
    RNA3D_GC_SBACK_8,  // Signature (global)

    RNA3D_GC_CBACK_0, 	// Ranges for SAI visualization
    RNA3D_GC_CBACK_1,
    RNA3D_GC_CBACK_2,
    RNA3D_GC_CBACK_3,
    RNA3D_GC_CBACK_4,
    RNA3D_GC_CBACK_5,
    RNA3D_GC_CBACK_6,
    RNA3D_GC_CBACK_7,
    RNA3D_GC_CBACK_8,
    RNA3D_GC_CBACK_9,

    RNA3D_GC_PSEUDOKNOT,
    RNA3D_GC_TRIPLE_BASE,

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

