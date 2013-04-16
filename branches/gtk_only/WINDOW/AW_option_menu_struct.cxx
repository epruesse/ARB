/* 
 * File:   AW_option_menu_struct.cxx
 * Author: aboeckma
 * 
 * Created on January 15, 2013, 3:41 PM
 */

#include "aw_option_menu_struct.hxx"
#include "aw_gtk_forward_declarations.hxx"


AW_option_menu_struct::AW_option_menu_struct(const char *variable_namei,
                                             GB_TYPES variable_typei,
                                             GtkWidget *menu_widgeti) {
    variable_name      = strdup(variable_namei);
    variable_type      = variable_typei;
    menu_widget        = menu_widgeti;
}
