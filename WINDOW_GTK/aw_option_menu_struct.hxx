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



struct AW_option_menu_struct : public Noncopyable {
    AW_option_menu_struct(int numberi, const char *variable_namei, AW_VARIABLE_TYPE variable_typei, GtkWidget *label_widgeti, GtkWidget *menu_widgeti, AW_pos xi, AW_pos yi, int correct);

    int               option_menu_number;
    char             *variable_name;
    AW_VARIABLE_TYPE  variable_type;
    GtkWidget        *label_widget;
    GtkWidget        *menu_widget;
    std::vector<std::string> options; /** < Contains all options */
    std::string default_option; /** < This option is selected by default */
    
    AW_pos x;
    AW_pos y;
    int    correct_for_at_center_intern;            // needed for centered and right justified menus (former member of AW_at)
    
    /** Contains a VarUpdateInfo for each entry in this option menu.*/
    std::map<std::string, AW_varUpdateInfo*> valueToUpdateInfo;
    
    AW_option_menu_struct *next;
    
    
    inline void add_option(const std::string& option_name, bool default_option) {
        if (default_option) {
            this->default_option = option_name;
        }
        options.push_back(option_name);
    }
};

