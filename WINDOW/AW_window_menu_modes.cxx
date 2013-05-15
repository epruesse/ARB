#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include "aw_drawing_area.hxx"
#include "gdk/gdkkeysyms.h"

void aw_insert_default_help_entries(AW_window *aww) {
    aww->insert_help_topic("Click here and then on the questionable button/menu/...", "P", 0, AWM_ALL, (AW_CB)AW_help_entry_pressed, 0, 0);

    aww->insert_help_topic("How to use help", "H", "help.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"help.hlp", 0);
    aww->insert_help_topic("ARB help",        "A", "arb.hlp",  AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"arb.hlp",  0);
}

AW_window_menu_modes::AW_window_menu_modes() {
}

AW_window_menu_modes::~AW_window_menu_modes() {
}

void AW_window_menu_modes::init(AW_root */*root_in*/, const char *window_name_, const char *window_title, 
                                int width, int height) {
    init_window(window_name_, window_title, width, height, true /*resizable*/);
    
#if defined(DUMP_MENU_LIST)
    initMenuListing(window_name_);
#endif // DUMP_MENU_LIST
    const char *help_button   = "_HELP"; //underscore + mnemonic 

    // create menu bar
    prvt->menu_bar = (GtkMenuBar*) gtk_menu_bar_new();
    prvt->menus.push(GTK_MENU_SHELL(prvt->menu_bar)); //The menu bar is the top level menu shell.
    //create help menu
    GtkMenuItem *help_item = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(help_button));
    prvt->help_menu = GTK_MENU_SHELL(gtk_menu_new());
    gtk_menu_item_set_submenu(help_item, GTK_WIDGET(prvt->help_menu));
    gtk_menu_shell_append(GTK_MENU_SHELL(prvt->menu_bar), GTK_WIDGET(help_item));

    // create vertical toolbar ('mode menu')
    prvt->mode_menu = GTK_TOOLBAR(gtk_toolbar_new());
    gtk_toolbar_set_orientation(prvt->mode_menu, GTK_ORIENTATION_VERTICAL);
    gtk_toolbar_set_show_arrow(prvt->mode_menu, false);
    
    GtkWidget *scrolledWindow = gtk_scrolled_window_new(NULL, NULL);
    // Scrollbars must be shown alwyas because dynamic display will cause 
    // a resize event which will cause a zoom reset in @@@AWT/AWT_canvas.cxx
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(scrolledWindow),
                                    GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);

    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledWindow), GTK_SHADOW_ETCHED_IN);

    // create main drawing area ('middle area')
    prvt->drawing_area = AW_DRAWING_AREA(aw_drawing_area_new());
    prvt->areas[AW_MIDDLE_AREA] = new AW_area_management(GTK_WIDGET(prvt->drawing_area), this);

    // create secondary drawing area ('bottom_area')
    prvt->bottom_area = AW_DRAWING_AREA(aw_drawing_area_new());
    prvt->areas[AW_BOTTOM_AREA] = new AW_area_management(GTK_WIDGET(prvt->bottom_area), this);


    aw_assert(NULL != prvt->drawing_area);
    gtk_container_add(GTK_CONTAINER(scrolledWindow), GTK_WIDGET(prvt->drawing_area));

   


    // Layout:
    // fixed_size_area ('info') goes above scrollArea ('middle')
    GtkWidget *vbox2 = gtk_vbox_new(false, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), GTK_WIDGET(prvt->fixed_size_area), false, false, 0);
    gtk_box_pack_start(GTK_BOX(vbox2), scrolledWindow, true, true, 0);   
    gtk_box_pack_start(GTK_BOX(vbox2), GTK_WIDGET(prvt->bottom_area), false, false, 0);
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
    gtk_widget_realize(GTK_WIDGET(prvt->drawing_area));
    gtk_widget_realize(GTK_WIDGET(prvt->bottom_area));

    create_devices();
    aw_insert_default_help_entries(this);
    create_window_variables();
}

void AW_window_menu_modes::select_mode(int mode) {
    GtkToolItem* button = gtk_toolbar_get_nth_item(prvt->mode_menu, mode);
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(button), true);
}

void AW_window_menu_modes::create_mode(const char *pixmap, const char *helpText, AW_active mask, 
                           void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2){
    
    aw_assert(legal_mask(mask));
    
    // create radio button
    GtkToolItem *button;
    if (!gtk_toolbar_get_n_items(prvt->mode_menu)) {
      // first toolbar entry
      button = gtk_radio_tool_button_new(NULL);
    } else {
      GtkToolItem *first = gtk_toolbar_get_nth_item(prvt->mode_menu,0);
      button = gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(first));
      
    }
    gtk_toolbar_insert(prvt->mode_menu, button, -1); //-1 = append

    // add image
    const char *path  = GB_path_in_ARBLIB("pixmaps", pixmap);
    GtkWidget *icon = gtk_image_new_from_file(path);
    gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(button), icon);
       
    // register clicked callback
    AW_cb_struct *cbs = new AW_cb_struct(this, f, cd1, cd2, 0);
    cbs->help_text = helpText;
    g_signal_connect((gpointer)button, "toggled", G_CALLBACK(AW_window::click_handler), (gpointer)cbs);

    // register F1 - Fn accelerators
    // jumping through some hoops here. the "toggled" signal doesn't work, so we create
    // a closure that calls gtk_toggle_tool_button_set_active
    // this /might/ be a bug... I don't understand where gtk...set_active get's its 
    // second parameter from. it works, though... valgrinders beware...
    guint accel_key =  GDK_F1 + gtk_toolbar_get_n_items(prvt->mode_menu) - 1;
    GClosure *gclosure = g_cclosure_new_swap(G_CALLBACK(gtk_toggle_tool_button_set_active), button, NULL);
    gtk_accel_group_connect(prvt->accel_group, accel_key,
                            (GdkModifierType)0, GTK_ACCEL_VISIBLE,
                            gclosure);

    // put the accelerator name into the tooltip
    gtk_widget_set_tooltip_text(GTK_WIDGET(button), gtk_accelerator_name(accel_key, (GdkModifierType)0));

    get_root()->register_widget(GTK_WIDGET(button), mask);
}
