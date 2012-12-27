/* 
 * File:   AW_varUpdateInfo.cxx
 * Author: aboeckma
 * 
 * Created on December 20, 2012, 11:18 AM
 */

#include <gtk-2.0/gtk/gtktreeview.h>
#include <gtk-2.0/gtk/gtktreeselection.h>

#include "aw_varUpdateInfo.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"
#include "aw_root.hxx"
#include "aw_msg.hxx"


void AW_varUpdateInfo::AW_variable_update_callback(GtkWidget *widget, gpointer variable_update_struct) {
    AW_varUpdateInfo *vui = (AW_varUpdateInfo *) variable_update_struct;
    aw_assert(vui);


    if(GTK_IS_TREE_SELECTION(widget)) {
        //if this event is coming from a tree view widget
        //it is called twice, once for deselecting the old item and once for selecting
        //the new one. We dont want to handle the deselect
        if(gtk_tree_selection_count_selected_rows(GTK_TREE_SELECTION(widget)) > 0)
        {
            vui->change_from_widget((gpointer)widget); 
        }
    }
    else
    { //for now forward all other events.
        vui->change_from_widget((gpointer)widget); 
    }
    
}

void AW_varUpdateInfo::change_from_widget(gpointer call_data) {
    
    
    AW_cb_struct::useraction_init();

    GB_ERROR  error = NULL;
    AW_root  *root  = awar->root;

    if (root->value_changed) {
        root->changer_of_variable = widget;
    }

    if (root->is_recording_macro()) {
        // add a callback which writes macro-code (BEFORE any other callback happens; last added, first calledback)
//        GB_transaction ta(awar->gb_var);
//        GB_add_callback(awar->gb_var, GB_CB_CHANGED, record_awar_change, (int*)awar);
        FIXME("macro recording not implemented");
    }

    bool run_cb = true;
    switch (widget_type) {
        case AW_WIDGET_INPUT_FIELD:
            FIXME("AW_WIDGET_INPUT_FIELD handling not implemented");
        case AW_WIDGET_TEXT_FIELD:
            FIXME("AW_WIDGET_TEXT_FIELD handling not implemented");
//            if (!root->value_changed) {
//                run_cb = false;
//            }
//            else {
//                char *new_text = XmTextGetString((widget));
//                error          = awar->write_as_string(new_text);
//                XtFree(new_text);
//            }
//            break;
//
//        case AW_WIDGET_TOGGLE:
//            root->changer_of_variable = 0;
//            error = awar->toggle_toggle();
            break;

        case AW_WIDGET_TOGGLE_FIELD:
            FIXME("AW_WIDGET_TOGGLE_FIELD  handling not implemented");
//            if (XmToggleButtonGetState(widget) == False) break; // no toggle is selected (?)
            // fall-through
        case AW_WIDGET_CHOICE_MENU:
            FIXME("AW_WIDGET_CHOICE_MENU handling not implemented");
//            error = value.write_to(awar);
            break;

        case AW_WIDGET_SELECTION_LIST: {
            char *selected;
            GtkTreeSelection* selection = GTK_TREE_SELECTION(call_data);
            GtkTreeIter iter;
            GtkTreeModel *model;
            
            if(gtk_tree_selection_get_selected(selection, &model, &iter))
                
            
            //if this assert fails no rows are selected, which should be impossible because
            //this method is called when the selection changes.
            aw_assert(gtk_tree_selection_get_selected(selection, &model, &iter));
            gtk_tree_model_get(model, &iter, 0, &selected,  -1);
      
            AW_selection_list_entry *entry = sellist->list_table;
            while (entry && strcmp(entry->get_displayed(), selected) != 0) {
                entry = entry->next;
            }

            if (!entry) {   
                entry = sellist->default_select; // use default selection
                if (!entry) GBK_terminate("no default specified for selection list"); // or die
            }
            entry->value.write_to(awar);
            break;
        }

        case AW_WIDGET_LABEL_FIELD:
            FIXME("AW_WIDGET_LABEL_FIELD handling not implemented");
            break;

        default:
            GBK_terminatef("Unknown widget type %i in AW_variable_update_callback", widget_type);
            break;
    }
    
    if (root->is_recording_macro()) {
//        GB_transaction ta(awar->gb_var);
//        GB_remove_callback(awar->gb_var, GB_CB_CHANGED, record_awar_change, (int*)awar);
        FIXME("macro recording not implemented");
    }

    if (error) {
        root->changer_of_variable = 0;
        awar->update();
        aw_message(error);
    }
    else {
        if (cbs && run_cb) cbs->run_callback();
        root->value_changed = false;

        if (GB_have_error()) aw_message(GB_await_error()); // show error exported by awar-change-callback
    }

    AW_cb_struct::useraction_done(aw_parent);
}

void AW_varUpdateInfo::set_widget(GtkWidget* w) {
    widget = w; 
}
void AW_varUpdateInfo::set_sellist(AW_selection_list *asl) {
    sellist = asl; 
}


