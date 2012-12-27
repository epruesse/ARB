/* 
 * File:   AW_varUpdateInfo.hxx
 * Author: aboeckma
 *
 * Created on December 20, 2012, 11:18 AM
 */

#pragma once
#include "AW_gtk_forward_declarations.hxx"
#include "aw_awar.hxx"
#include "aw_scalar.hxx"
#include "aw_gtk_migration_helpers.hxx"
#ifndef ARBDB_H
#include <arbdb.h>
#endif

class AW_window;
class AW_cb_struct;
class AW_selection_list;


class AW_varUpdateInfo : virtual Noncopyable { // used to refresh single items on change
    AW_window         *aw_parent;
    GtkWidget         *widget;
    AW_widget_type     widget_type;
    AW_awar           *awar;
    AW_scalar          value;
    AW_cb_struct      *cbs;
    AW_selection_list *sellist;

public:
    AW_varUpdateInfo(AW_window *aw, GtkWidget *w, AW_widget_type wtype, AW_awar *a, AW_cb_struct *cbs_)
        : aw_parent(aw), widget(w), widget_type(wtype), awar(a),
          value(a),
          cbs(cbs_), sellist(NULL)
    {
    }
    template<typename T>
    AW_varUpdateInfo(AW_window *aw, GtkWidget *w, AW_widget_type wtype, AW_awar *a, T t, AW_cb_struct *cbs_)
        : aw_parent(aw), widget(w), widget_type(wtype), awar(a),
          value(t),
          cbs(cbs_), sellist(NULL)
    {
    }

    void change_from_widget(gpointer call_data);

    void set_widget(GtkWidget *w);
    void set_sellist(AW_selection_list *asl);
    
    static void AW_variable_update_callback(GtkWidget *selection, gpointer variable_update_struct);
    
};

