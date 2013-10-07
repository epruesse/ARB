#include "aw_window.hxx"
#include "aw_window_gtk.hxx"

AW_window_simple::AW_window_simple() : AW_window() {
}

AW_window_simple::~AW_window_simple() {
}

void AW_window_simple::init(AW_root */*root_in*/, const char *window_name_, const char *window_title) {
    init_window(window_name_, window_title, 0, 0, true /*resizable*/);

    gtk_container_add(GTK_CONTAINER(prvt->window), GTK_WIDGET(prvt->fixed_size_area));
}
