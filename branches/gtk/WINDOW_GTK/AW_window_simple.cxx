#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include <arbdb.h>

AW_window_simple::AW_window_simple() : AW_window() {
}

AW_window_simple::~AW_window_simple() {
}

void AW_window_simple::init(AW_root *root_in, const char *wid, const char *windowname) {
    root = root_in; // for macro
    prvt->fixed_size_area = GTK_FIXED(gtk_fixed_new());
    gtk_container_add(GTK_CONTAINER(prvt->window), GTK_WIDGET(prvt->fixed_size_area));

    //Creates the GDK (windowing system) resources associated with a widget.
    //This is done as early as possible because xfig drawing relies on the gdk stuff.
    gtk_widget_realize(GTK_WIDGET(prvt->window));
    gtk_widget_realize(GTK_WIDGET(prvt->fixed_size_area));
    gtk_widget_show(GTK_WIDGET(prvt->fixed_size_area));


//    int width  = 100;                               // this is only the minimum size!
//    int height = 100;
//    int posx   = 50;
//    int posy   = 50;

    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    gtk_window_set_resizable(prvt->window, true);
    gtk_window_set_title(prvt->window, window_name);

    GTK_PARTLY_IMPLEMENTED;



   // p_w->shell = aw_create_shell(this, true, true, width, height, posx, posy);

    // add this to disable resize or maximize in simple dialogs (avoids broken layouts)
    // XtVaSetValues(p_w->shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE, NULL);

//    Widget form1 = XtVaCreateManagedWidget("forms", xmFormWidgetClass,
//            p_w->shell,
//            NULL);
    prvt->areas.reserve(AW_MAX_AREA);
    prvt->areas[AW_INFO_AREA] = new AW_area_management(root, GTK_WIDGET(prvt->window), GTK_WIDGET(prvt->window));


  //  aw_realize_widget(this);
    create_devices();
}
