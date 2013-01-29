#include "aw_window.hxx"
#include "aw_window_gtk.hxx"

AW_window::AW_window_gtk::AW_window_gtk() 
  :   window(GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL))), 
      fixed_size_area(NULL), menu_bar(NULL), help_menu(NULL),
      mode_menu(NULL), radio_last(NULL), toggle_field(NULL), combo_box(NULL), 
      number_of_modes(0), 
      popup_cb(NULL), focus_cb(NULL),modes_f_callbacks(NULL), callback(NULL),
      hAdjustment(NULL),
      vAdjustment(NULL) 
{} 
