#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Graphics.hxx"
#include "RNA3D_Interface.hxx"

using namespace std;

extern GBDATA *gb_main;

AW_gc_manager RNA3D_Graphics::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw, AW_CL cd2) {
    AW_gc_manager preset_window =
        AW_manage_GC (aww,
                      device,
                      RNA3D_GC_BACKGROUND,
                      RNA3D_GC_MAX,
                      AW_GCM_DATA_AREA,
                      (AW_CB)AWT_resize_cb,
                      (AW_CL)ntw,
                      cd2,
                      false,
                      "#005500",
                      "+-Foreground$#FFAA00",
                      0 );

    return preset_window;
}

RNA3D_Graphics::RNA3D_Graphics(AW_root *aw_root_, GBDATA *gb_main_):AWT_graphic() {
    exports.dont_fit_x      = 1;
    exports.dont_fit_y      = 1;
    exports.dont_fit_larger = 0;
    exports.left_offset     = 20;
    exports.right_offset    = 200;
    exports.top_offset      = 30;
    exports.bottom_offset   = 30;
    exports.dont_scroll     = 0;

    this->aw_root = aw_root_;
    this->gb_main = gb_main_;
}

RNA3D_Graphics::~RNA3D_Graphics(void) {}

void RNA3D_Graphics::command(AW_device */*device*/, AWT_COMMAND_MODE /*cmd*/, int /*button*/, AW_key_mod /*key_modifier*/, char /*key_char*/,
                             AW_event_type /*type*/, AW_pos /*x*/, AW_pos /*y*/, AW_clicked_line *cl, AW_clicked_text *ct)
{
    AWUSE(cl);
    AWUSE(ct);
}

void RNA3D_Graphics::show(AW_device *device){
    paint(device);
}

void RNA3D_Graphics::info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct)
{
    aw_message("INFO MESSAGE");
    AWUSE(device);
    AWUSE(x);
    AWUSE(y);
    AWUSE(cl);
    AWUSE(ct);
}

void RNA3D_Graphics::paint(AW_device *device) {
    RefreshOpenGLDisplay();
    AWUSE(device);    
}



