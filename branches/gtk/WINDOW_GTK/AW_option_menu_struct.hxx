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

#include "AW_gtk_forward_declarations.hxx"
#include "aw_scalar.hxx"
#include "aw_base.hxx"

struct AW_widget_value_pair : virtual Noncopyable {
    template<typename T> explicit AW_widget_value_pair(T t, GtkWidget* w) : value(t), widget(w), next(NULL) {}
    ~AW_widget_value_pair() { aw_assert(!next); } // has to be unlinked from list BEFORE calling dtor

    AW_scalar  value;
    GtkWidget* widget;

    AW_widget_value_pair *next;
};

struct AW_option_menu_struct {
    AW_option_menu_struct(int numberi, const char *variable_namei, AW_VARIABLE_TYPE variable_typei, GtkWidget *label_widgeti, GtkWidget *menu_widgeti, AW_pos xi, AW_pos yi, int correct);

    int               option_menu_number;
    char             *variable_name;
    AW_VARIABLE_TYPE  variable_type;
    GtkWidget        *label_widget;
    GtkWidget        *menu_widget;

    AW_widget_value_pair *first_choice;
    AW_widget_value_pair *last_choice;
    AW_widget_value_pair *default_choice;
    
    AW_pos x;
    AW_pos y;
    int    correct_for_at_center_intern;            // needed for centered and right justified menus (former member of AW_at)

    AW_option_menu_struct *next;
};

