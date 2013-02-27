#include "aw_button.hxx"
#include "aw_assert.hxx"
#include "aw_root.hxx" // for legal_mask

#include "gtk/gtk.h"

AW_button::AW_button(AW_active maski, GtkWidget* w) 
    : mask(maski), button(w)
{
  aw_assert(w);
  aw_assert(legal_mask(maski));
}
  
void
AW_button::apply_sensitivity(AW_active maski) 
{
    gtk_widget_set_sensitive(button, (mask & maski) ? true : false);
}
