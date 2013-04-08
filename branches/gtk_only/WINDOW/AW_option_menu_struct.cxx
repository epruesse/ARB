/* 
 * File:   AW_option_menu_struct.cxx
 * Author: aboeckma
 * 
 * Created on January 15, 2013, 3:41 PM
 */

#include "aw_option_menu_struct.hxx"
#include "aw_gtk_forward_declarations.hxx"


AW_option_menu_struct::AW_option_menu_struct(int numberi, const char *variable_namei,
                                             AW_VARIABLE_TYPE variable_typei,
                                             GtkWidget *menu_widgeti) {
    option_menu_number = numberi;
    variable_name      = strdup(variable_namei);
    variable_type      = variable_typei;
    menu_widget        = menu_widgeti;
}
