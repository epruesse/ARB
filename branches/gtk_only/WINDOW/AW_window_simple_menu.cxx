#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include <arbdb.h>

AW_window_simple_menu::AW_window_simple_menu() {
}

AW_window_simple_menu::~AW_window_simple_menu() {
}

void AW_window_simple_menu::init(AW_root */*root_in*/, const char *window_name, const char *window_title) {
    init_window(window_name, window_title, 0, 0, true /*resizable*/);
    const char *help_button   = "_HELP"; //underscore + mnemonic  

    // create menu bar
    prvt->menu_bar = (GtkMenuBar*) gtk_menu_bar_new();
    prvt->menus.push(GTK_MENU_SHELL(prvt->menu_bar));
    GtkMenuItem *help_item = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(help_button));
    prvt->help_menu = GTK_MENU_SHELL(gtk_menu_new());
    gtk_menu_item_set_submenu(help_item, GTK_WIDGET(prvt->help_menu));
    gtk_menu_shell_append(GTK_MENU_SHELL(prvt->menu_bar), GTK_WIDGET(help_item));

    // create drawing/info area
    GtkWidget *fixed_size_area = gtk_fixed_new();
    prvt->fixed_size_area = GTK_FIXED(fixed_size_area);
    prvt->areas[AW_INFO_AREA] = new AW_area_management(fixed_size_area, fixed_size_area); 

    // put menu+drawing area into vbox into window
    GtkWidget *vbox = gtk_vbox_new(false, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(prvt->menu_bar), false, false, 0);
    gtk_box_pack_start(GTK_BOX(vbox), fixed_size_area, false, false, 0);
    gtk_container_add(GTK_CONTAINER(prvt->window), vbox);

    gtk_widget_realize(GTK_WIDGET(prvt->window));
    create_devices();
    g_signal_connect (prvt->window, "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
    /*
    g_signal_connect((gpointer)prvt->window, 
                     "delete_event", 
                     G_CALLBACK(&AW_window_simple_menu::hide),
                     (gpointer)this);
    */

    FIXME("AW_window_simple_menu::init partially redundant with AW_window_simple::init");
}
