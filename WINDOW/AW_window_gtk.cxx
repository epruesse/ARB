#include "aw_window.hxx"
#include "aw_window_gtk.hxx"

AW_window::AW_window_gtk::AW_window_gtk() 
  :   window(GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL))), 
      fixed_size_area(NULL), menu_bar(NULL), help_menu(NULL),
      mode_menu(NULL), radio_last(NULL), toggle_field(NULL), 
      combo_box(NULL),       
      accel_group(gtk_accel_group_new()),
      areas(AW_MAX_AREA, NULL),
      popup_cb(NULL), focus_cb(NULL), callback(NULL), d_callback(NULL),
      drawing_area(NULL), delete_event_handler_id(-1)
{
    gtk_window_add_accel_group(window, accel_group);
    aw_assert(areas.size() == AW_MAX_AREA);
} 

void AW_window::AW_window_gtk::show() {
    gtk_widget_show_all(GTK_WIDGET(window));
}

void AW_window::AW_window_gtk::set_title(const char* title) {
    gtk_window_set_title(window, title);
}

void AW_window::AW_window_gtk::set_size(int width, int height) {
    gtk_window_set_default_size(window, width, height);
}

void AW_window::AW_window_gtk::get_size(int& width, int& height) {
    gtk_window_get_size(window, &width, &height);
}

void AW_window::AW_window_gtk::set_resizable(bool resizable) {
    gtk_window_set_resizable(window, resizable);
}

void AW_window::AW_window_gtk::get_font_size(int &width, int& height) {
    PangoContext  *pc = gtk_widget_get_pango_context(GTK_WIDGET(window));
    PangoFontMetrics *pfm = pango_context_get_metrics(pc, NULL, NULL);
    height = PANGO_PIXELS(pango_font_metrics_get_ascent(pfm) +
                          pango_font_metrics_get_descent(pfm));
    width = PANGO_PIXELS(pango_font_metrics_get_approximate_char_width(pfm));
    pango_font_metrics_unref(pfm);
}
