#pragma once

#include "aw_gtk_forward_declarations.hxx"
#include "aw_base.hxx"

class AW_button {
private:
  AW_active mask;
  GtkWidget* button;
public: 
  AW_button(AW_active maski, GtkWidget* w);
  
  void apply_sensitivity(AW_active mask);
};
