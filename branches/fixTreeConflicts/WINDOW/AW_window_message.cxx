#include "aw_window.hxx"
#include "aw_window_gtk.hxx"

AW_window_message::AW_window_message() {
}

AW_window_message::~AW_window_message() {
}

void AW_window_message::init(AW_root */*root_in*/, const char *window_title, bool allow_close) {
    init_window(window_title, window_title, 0, 0, false /*not resizable*/);

    allow_delete_window(allow_close);

    gtk_container_add(GTK_CONTAINER(prvt->window), GTK_WIDGET(prvt->fixed_size_area));

    create_devices();
}

