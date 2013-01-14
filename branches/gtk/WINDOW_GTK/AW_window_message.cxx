#include "aw_window.hxx"
#include "aw_window_gtk.hxx"

AW_window_message::AW_window_message() {
}

AW_window_message::~AW_window_message() {
}

void AW_window_message::init(AW_root *root_in, const char *windowname, bool allow_close) {
    root = root_in;
    int width  = 100;
    int height = 100;
    int posx   = 50;
    int posy   = 50;

    set_window_title(windowname);
    set_window_size(width, height);
    allow_delete_window(allow_close);

    gtk_window_set_resizable(prvt->window, false);

    GtkWidget *fixed_size_area = gtk_fixed_new();
    prvt->fixed_size_area = GTK_FIXED(fixed_size_area);
    prvt->areas[AW_INFO_AREA] = new AW_area_management(root, fixed_size_area, fixed_size_area); 
    gtk_container_add(GTK_CONTAINER(prvt->window), fixed_size_area);

    create_devices();
}

