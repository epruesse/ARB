#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Graphics.hxx"
#include "RNA3D_Interface.hxx"
#include "RNA3D_OpenGLEngine.hxx"

#include <aw_msg.hxx>
#include <aw_preset.hxx>


using namespace std;


AW_gc_manager *RNA3D_Graphics::init_devices(AW_window *aww, AW_device *device, AWT_canvas *scr) {
    AW_gc_manager *gc_manager =
        AW_manage_GC (aww,
                      scr->get_gc_base_name(),
                      device,
                      RNA3D_GC_FOREGROUND,
                      RNA3D_GC_MAX,
                      AW_GCM_DATA_AREA,
                      makeGcChangedCallback(AWT_GC_changed_cb, scr),
                      false,
                      "#000000",
                      "+-Foreground$#FFFFFF",     "+-MOLECULE Skeleton$#606060", "-Mapped Species$#FF0000",
                      "+-BASE: Deletion$#FF0000", "+-BASE: Insertion$#00FF00",   "-BASE: Positions$#FFAA00",
                      "+-BASE: Helix$#55AAFF",    "+-BASE: Unpaired$#AAFF00",    "-BASE: Non-Helix$#FFAA55",
                      "+-HELIX$#FF0000",          "+-HELIX Skeleton$#606060",    "-HELIX MidPoint$#FFFFFF",

                      // colors used to Paint search patterns
                      // (do not change the names of these gcs)
                      "+-User1$#B8E2F8",          "+-User2$#B8E2F8",         "-Probe$#B8E2F8",
                      "+-Primer(l)$#A9FE54",      "+-Primer(r)$#A9FE54",     "-Primer(g)$#A9FE54",
                      "+-Sig(l)$#DBB0FF",         "+-Sig(r)$#DBB0FF",        "-Sig(g)$#DBB0FF",

                      // Color Ranges to paint SAIs
                      "+-RANGE 0$#FFFFFF",    "+-RANGE 1$#E0E0E0",    "-RANGE 2$#C0C0C0",
                      "+-RANGE 3$#A0A0A0",    "+-RANGE 4$#909090",    "-RANGE 5$#808080",
                      "+-RANGE 6$#808080",    "+-RANGE 7$#505050",    "-RANGE 8$#404040",
                      "+-RANGE 9$#303030",    "+-Pseudoknots$#FFAAFF", "-Triple Bases$#55FF00",

                      "+-Cursor$#FFFFFF",    "+-Comments$#808080",    "-MoleculeMask$#00FF00",
                      NULL);

    return gc_manager;
}

RNA3D_Graphics::RNA3D_Graphics(AW_root *aw_root_, GBDATA *gb_main_) {
    exports.zoom_mode = AWT_ZOOM_NEVER;
    exports.fit_mode  = AWT_FIT_NEVER;
    
    exports.set_standard_default_padding();

    this->aw_root = aw_root_;
    this->gb_main = gb_main_;
}

RNA3D_Graphics::~RNA3D_Graphics() {}

void RNA3D_Graphics::show(AW_device *device) {
    paint(device);
}

void RNA3D_Graphics::paint(AW_device * /* device */) {
    MapDisplayParameters(aw_root);
    RefreshOpenGLDisplay();
}



