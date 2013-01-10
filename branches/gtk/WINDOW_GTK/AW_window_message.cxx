#include "aw_window.hxx"
#include "aw_window_gtk.hxx"

AW_window_message::AW_window_message() {
    GTK_NOT_IMPLEMENTED;
}

AW_window_message::~AW_window_message() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window_message::init(AW_root */*root_in*/, const char */*windowname*/, bool /*allow_close*/) {
    GTK_NOT_IMPLEMENTED;
//    root = root_in; // for macro
//
//    int width  = 100;
//    int height = 100;
//    int posx   = 50;
//    int posy   = 50;
//
//    window_name = strdup(windowname);
//    window_defaults_name = GBS_string_2_key(window_name);
//
//    // create shell for message box
//    p_w->shell = aw_create_shell(this, true, allow_close, width, height, posx, posy);
//
//    // disable resize or maximize in simple dialogs (avoids broken layouts)
//    XtVaSetValues(p_w->shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
//            NULL);
//
//    p_w->areas[AW_INFO_AREA] = new AW_area_management(root, p_w->shell, XtVaCreateManagedWidget("info_area",
//                    xmDrawingAreaWidgetClass,
//                    p_w->shell,
//                    XmNheight, 0,
//                    XmNbottomAttachment, XmATTACH_NONE,
//                    XmNtopAttachment, XmATTACH_FORM,
//                    XmNleftAttachment, XmATTACH_FORM,
//                    XmNrightAttachment, XmATTACH_FORM,
//                    NULL));
//
//    aw_realize_widget(this);
}

