


#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include "aw_drawing_area.hxx"
#include <arbdb.h>

void aw_insert_default_help_entries(AW_window *aww) {
    aww->insert_help_topic("Click here and then on the questionable button/menu/...", "P", 0, AWM_ALL, (AW_CB)AW_help_entry_pressed, 0, 0);

    aww->insert_help_topic("How to use help", "H", "help.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"help.hlp", 0);
    aww->insert_help_topic("ARB help",        "A", "arb.hlp",  AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"arb.hlp",  0);
}

AW_window_menu_modes::AW_window_menu_modes() {
}

AW_window_menu_modes::~AW_window_menu_modes() {
}

void AW_window_menu_modes::init(AW_root */*root_in*/, const char *window_name, const char *window_title, 
                                int width, int height) {
    init_window(window_name, window_title, width, height, true /*resizable*/);
    
#if defined(DUMP_MENU_LIST)
    initMenuListing(windowname);
#endif // DUMP_MENU_LIST
    const char *help_button   = "_HELP"; //underscore + mnemonic 

    // create menu bar
    prvt->menu_bar = (GtkMenuBar*) gtk_menu_bar_new();
    prvt->menus.push(GTK_MENU_SHELL(prvt->menu_bar)); //The menu bar is the top level menu shell.
    //create help menu
    GtkMenuItem *help_item = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(help_button));
    prvt->help_menu = GTK_MENU_SHELL(gtk_menu_new());
    gtk_menu_item_set_submenu(help_item, GTK_WIDGET(prvt->help_menu));
    gtk_menu_item_set_right_justified(help_item, true);
    gtk_menu_shell_append(GTK_MENU_SHELL(prvt->menu_bar), GTK_WIDGET(help_item));

    // create vertical toolbar ('mode menu')
    prvt->mode_menu = GTK_TOOLBAR(gtk_toolbar_new());
    gtk_toolbar_set_orientation(prvt->mode_menu, GTK_ORIENTATION_VERTICAL);

    // create area for buttons at top ('info area')
    prvt->fixed_size_area = GTK_FIXED(gtk_fixed_new());
    FIXME("form should be a frame around area?!");
    prvt->areas[AW_INFO_AREA] = new AW_area_management(GTK_WIDGET(prvt->fixed_size_area), GTK_WIDGET(prvt->fixed_size_area)); 

    //create scroll window around drawing area
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    //only show scrollbars if they are needed
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledWindow),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    
    // create main drawing area ('middle area')
    prvt->drawing_area = AW_DRAWING_AREA(aw_drawing_area_new());
    gtk_container_add(GTK_CONTAINER(scrolledWindow), GTK_WIDGET(prvt->drawing_area));

    GtkWidget* scrolledTree = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolledTree), prvt->drawing_area);
    

    gtk_widget_realize(GTK_WIDGET(prvt->drawing_area));
    //FIXME form should be a frame around the area.
    prvt->areas[AW_MIDDLE_AREA] = new AW_area_management(GTK_WIDGET(prvt->drawing_area), GTK_WIDGET(prvt->drawing_area)); 
    gtk_widget_show(scrolledTree);
    gtk_widget_show(GTK_WIDGET(drawing_area));


    // Layout:
    // fixed_size_area ('info') goes above scrollArea ('middle')
    GtkWidget *vbox2 = gtk_vbox_new(false, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), GTK_WIDGET(prvt->fixed_size_area), false, false, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), scrolledTree, true, true, 0);
    

    // Both go right of the mode_menu / vert. toolbar
    GtkWidget *hbox = gtk_hbox_new(false, 0);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(prvt->mode_menu), false, false, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox2, true, true, 0);
    // And above those goes the menu          
    GtkWidget *vbox = gtk_vbox_new(false, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(prvt->menu_bar), false, false, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hbox), true, true, 0);
    gtk_container_add(GTK_CONTAINER(prvt->window), vbox);

    // make-it-so:
    gtk_widget_realize(GTK_WIDGET(prvt->window)); 
    create_devices();
    aw_insert_default_help_entries(this);
    create_window_variables();
}



