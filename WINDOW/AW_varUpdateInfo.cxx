/* 
 * File:   AW_varUpdateInfo.cxx
 * Author: aboeckma
 * 
 * Created on December 20, 2012, 11:18 AM
 */

#include <gtk/gtk.h>

#include "aw_varUpdateInfo.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"
#include "aw_root.hxx"
#include "aw_msg.hxx"
#include "aw_select.hxx"

bool AW_varUpdateInfo::AW_variable_update_callback_event(GtkWidget *widget, GdkEvent */*event*/, gpointer variable_update_struct) {
  DUMP_CALLBACK(widget, "");

    AW_varUpdateInfo *vui = (AW_varUpdateInfo *) variable_update_struct;
    aw_assert(vui);
    
    vui->change_from_widget((gpointer)widget); 
    return false; // must continue on
}


bool AW_varUpdateInfo::AW_variable_update_callback(GtkWidget *widget, gpointer variable_update_struct) {
  DUMP_CALLBACK(widget, "");
    AW_varUpdateInfo *vui = (AW_varUpdateInfo *) variable_update_struct;
    aw_assert(vui);


    if(GTK_IS_TREE_SELECTION(widget)) {
        //if this event is coming from a tree view widget
        //it is called twice, once for deselecting the old item and once for selecting
        //the new one. We don't want to handle the deselect
        if(gtk_tree_selection_count_selected_rows(GTK_TREE_SELECTION(widget)) > 0)
        {
            vui->change_from_widget((gpointer)widget); 
        }
    }
    else if(GTK_IS_RADIO_BUTTON(widget)) {
        //the radio button extends toggle button. 
        if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
            //this method is also called when a radio button is unchecked
            //but we don't need that information.
            vui->change_from_widget((gpointer)widget); 
        }
    }
    else { 
        vui->change_from_widget((gpointer)widget); 
    }
    return false;
}

static void track_awar_change(GBDATA*, int *cl_awar, GB_CB_TYPE IF_ASSERTION_USED(cb_type)) {
    AW_awar *awar = (AW_awar*)cl_awar;
    aw_assert(cb_type == GB_CB_CHANGED);
    awar->root->track_awar_change(awar);
}

void AW_varUpdateInfo::change_from_widget(gpointer call_data) {
    AW_cb_struct::useraction_init();

    GB_ERROR  error = NULL;
    AW_root  *root  = awar->root;

    if (root->value_changed) {
        root->changer_of_variable = widget;
    }

    if (root->is_tracking()) {
        // add a callback which writes macro-code (BEFORE any other callback happens; last added, first calledback)
        GB_transaction ta(awar->gb_var);
        GB_add_callback(awar->gb_var, GB_CB_CHANGED, track_awar_change, (int*)awar);
    }

    bool run_cb = true;
    switch (widget_type) {
        case AW_WIDGET_TEXT_FIELD:
            if (!root->value_changed) {
                run_cb = false;
            }
            else {
                GtkTextBuffer* textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
                GtkTextIter begin, end;
                gtk_text_buffer_get_bounds(textbuffer, &begin, &end);
                char *str = gtk_text_buffer_get_text(textbuffer, &begin, &end, true);
                awar->write_string(str); //FIXME write_string returns an error code that should be handled
                free(str);
            }
            break;

        case AW_WIDGET_INPUT_FIELD:
            if (!root->value_changed) {
                run_cb = false;
            }
            else {
                GtkEditable *field = GTK_EDITABLE(widget);
                char *new_text = gtk_editable_get_chars(field, 0, -1); //-1 = get text until end
                error = awar->write_as_string(new_text);
                free(new_text);
            }
            break;
        case AW_WIDGET_TOGGLE:
            error = awar->toggle_toggle();
            break;

        case AW_WIDGET_TOGGLE_FIELD:
            error = value.write_to(awar);
            break;

        case AW_WIDGET_CHOICE_MENU:
            // fall-through
        case AW_WIDGET_SELECTION_LIST: {
            sellist->update_awar();
            break;
        }

        case AW_WIDGET_LABEL_FIELD:
            FIXME("AW_WIDGET_LABEL_FIELD handling not implemented");
            break;

        default:
            GBK_terminatef("Unknown widget type %i in AW_variable_update_callback", widget_type);
            break;
    }
    
    if (root->is_tracking()) {
        GB_transaction ta(awar->gb_var);
        GB_remove_callback(awar->gb_var, GB_CB_CHANGED, track_awar_change, (int*)awar);
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


