/* 
 * File:   AW_option_menu_struct.hxx
 * Author: aboeckma
 *
 * Created on January 15, 2013, 3:41 PM
 */

#pragma once


#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

#include "aw_gtk_forward_declarations.hxx"
#include "aw_scalar.hxx"
#include "aw_base.hxx"
#include "aw_varUpdateInfo.hxx"
#include <map>
#include <string>
#include <vector>



struct AW_option_menu_struct : virtual Noncopyable {
    AW_option_menu_struct(int numberi, const char *variable_namei, GB_TYPES variable_typei, GtkWidget *menu_widgeti);

    int        option_menu_number;
    char      *variable_name;
    GB_TYPES   variable_type;
    GtkWidget *menu_widget;
    std::vector<std::string> options; /** < Contains all options */
    std::string default_option; /** < This option is selected by default */
    
    /** Contains a VarUpdateInfo for each entry in this option menu.*/
    std::map<std::string, AW_varUpdateInfo*> valueToUpdateInfo;
    
    AW_option_menu_struct *next;
    
    
    inline void add_option(const std::string& option_name, bool as_default_option) {
        if (as_default_option) {
            default_option = option_name;
        }
        options.push_back(option_name);
    }
};

