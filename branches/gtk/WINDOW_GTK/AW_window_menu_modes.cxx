


#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"
#include "aw_window_gtk.hxx"

#include <arbdb.h>

void aw_create_help_entry(AW_window *aww) {
    aww->insert_help_topic("Click here and then on the questionable button/menu/...", "P", 0,
                           AWM_ALL, (AW_CB)AW_help_entry_pressed, 0, 0);
}

AW_window_menu_modes::AW_window_menu_modes() {
}

AW_window_menu_modes::~AW_window_menu_modes() {
}

void AW_window_menu_modes::init(AW_root *root_in, const char *wid, const char *windowname, int width, int height) {
#if defined(DUMP_MENU_LIST)
    initMenuListing(windowname);
#endif // DUMP_MENU_LIST
    const char *help_button   = "_HELP"; //underscore + mnemonic 

    root = root_in; // for macro
    set_window_title(windowname);
    set_window_size(width, height);
    window_defaults_name = GBS_string_2_key(wid);


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
    prvt->areas[AW_INFO_AREA] = new AW_area_management(root, GTK_WIDGET(prvt->fixed_size_area), GTK_WIDGET(prvt->fixed_size_area)); 

    //create a table that will contain the drawing area and scrollbars
    GtkWidget *table = gtk_table_new(2, 2, false);

    
    // create main drawing area ('middle area')
    GtkWidget* drawing_area = gtk_drawing_area_new();
  
    //These adjustments will be attached to the scrollbars.
    prvt->hAdjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0));
    prvt->vAdjustment = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 0, 0, 0, 0));
      
    //put drawing area into top left table cell
    gtk_table_attach(GTK_TABLE(table), drawing_area, 0, 1, 0, 1,(GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), 0, 0);
    GtkWidget *hscrollbar = gtk_hscrollbar_new(prvt->hAdjustment); 
    gtk_table_attach(GTK_TABLE(table), hscrollbar, 0, 1, 1, 2, (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL), GTK_FILL, 0, 0);
    GtkWidget *vscrollbar = gtk_vscrollbar_new(prvt->vAdjustment); 
    gtk_table_attach(GTK_TABLE(table), vscrollbar, 1, 2, 0, 1, GTK_FILL, (GtkAttachOptions) (GTK_EXPAND | GTK_SHRINK | GTK_FILL) , 0, 0);
    
    
    
    gtk_widget_realize(GTK_WIDGET(drawing_area));
    prvt->areas[AW_MIDDLE_AREA] = new AW_area_management(root, drawing_area, drawing_area); 
    //FIXME form should be a frame around the area.

    // Layout:
    // fixed_size_area ('info') goes above scrollArea ('middle')
    GtkWidget *vbox2 = gtk_vbox_new(false, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), GTK_WIDGET(prvt->fixed_size_area), false, false, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), table, true, true, 0);   
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
    aw_create_help_entry(this);
    create_window_variables();
}
