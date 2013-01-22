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

    window_defaults_name = GBS_string_2_key(wid);

    gtk_window_set_resizable(prvt->window, true);
    set_window_title(windowname);

    
    prvt->areas.reserve(AW_MAX_AREA);
    prvt->areas[0] = NULL;
    prvt->areas[AW_INFO_AREA] = new AW_area_management(root, GTK_WIDGET(prvt->window), GTK_WIDGET(prvt->window));

    create_devices();
}
