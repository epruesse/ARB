// ============================================================== //
//                                                                //
//   File      : AW_option_toggle.cxx                             //
//   Purpose   : option-menu- and toggle-code                     //
//                                                                //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de in Jul 2012   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include "aw_at.hxx"
#include "aw_select.hxx"
#include "aw_choice.hxx"

#include <gtk/gtk.h>

// ----------------------
//      Options-Menu

AW_selection_list *AW_window::create_option_menu(const char *awar_name, bool fallback2default){
    AW_awar* awar = get_root()->awar_no_error(awar_name);
    aw_return_val_if_fail(awar, NULL);

    GtkWidget *combo_box = gtk_combo_box_new();
    AW_selection_list *slist = new AW_selection_list(awar, fallback2default);
    slist->bind_widget(combo_box);

    prvt->combo_box = combo_box;
    prvt->selection_list = slist; // define as current (for subsequent inserts)
    return slist;
}

void AW_window::clear_option_menu(AW_selection_list *sel) {
    prvt->selection_list = sel; // define as current (for subsequent inserts)
    sel->clear();
}

// ----------------------------------------------------------------------
// force-diff-sync 917381378912 (remove after merging back to trunk)
// ----------------------------------------------------------------------

template <class T>
void AW_window::insert_option_internal(const char *option_name, const char */*mnemonic*/,
                                       T           var_value,
                                       bool default_option)
{
    aw_assert(prvt->selection_list != NULL); // "current" option menu has to be set (insert-functions may only be used between create_option_menu/clear_option_menu and update_option_menu)

    //aw_return_if_fail(prvt->selection_list->variable_type == AW_TypeCheck::getVarType(var_value)); //type missmatch
    FIXME("check for distinct per-option callbacks");
    FIXME("setting sensitivity of option menu entries not implemented");

    if(default_option) {
        prvt->selection_list->insert_default(option_name, var_value);
    }
    else {
        prvt->selection_list->insert(option_name, var_value);
    }
}

// ----------------------------------------------------------------------
// force-diff-sync 917234982173 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::insert_option        (const char *on, const char *mn, const char *vv) { insert_option_internal(on, mn, vv, false); }
void AW_window::insert_default_option(const char *on, const char *mn, const char *vv) { insert_option_internal(on, mn, vv, true);  }
void AW_window::insert_option        (const char *on, const char *mn, int vv)         { insert_option_internal(on, mn, vv, false); }
void AW_window::insert_default_option(const char *on, const char *mn, int vv)         { insert_option_internal(on, mn, vv, true);  }
void AW_window::insert_option        (const char *on, const char *mn, float vv)       { insert_option_internal(on, mn, vv, false); }
void AW_window::insert_default_option(const char *on, const char *mn, float vv)       { insert_option_internal(on, mn, vv, true);  }
// (see insert_option_internal for longer parameter names)

void AW_window::update_option_menu() {
    aw_return_if_fail(prvt->selection_list);
    prvt->selection_list->update();
    put_with_label(prvt->combo_box);
    prvt->selection_list = NULL;
}

AW_selection_list* AW_window::create_selection_list(const char *awar_name, int columns, int rows, bool fallback2default) {
    AW_awar* awar = get_root()->awar_no_error(awar_name);
    aw_return_val_if_fail(awar, NULL);
    aw_warn_if_fail(!_at->label_for_inputfield); // labels have no effect for selection lists


    GtkWidget *tree = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_win), tree);

    if (!_at->to_position_exists) {
        // set size
        int char_width, char_height;
        prvt->get_font_size(char_width, char_height);
        gtk_widget_set_size_request(scrolled_win, char_width * columns, char_height * rows);
    }

    AW_selection_list *slist = new AW_selection_list(awar, fallback2default);
    slist->bind_widget(tree);

    awar->dclicked += prvt->action_template.dclicked;
    prvt->action_template = AW_action();

    put_with_label(scrolled_win);
    return slist;
}

// -------------------------------------------------------
//      toggle field (actually this are radio buttons)

void AW_window::create_toggle_field(const char *var_name, const char *labeli, const char *mnemonic) {
    /*!
     * Begins a radio button group with a label
     */
    if (labeli) {
        char *lab = aw_convert_mnemonic(labeli, mnemonic);
        this->label(lab);
        free(lab);
    }
    create_toggle_field(var_name);
}

void AW_window::create_toggle_field(const char *var_name, int orientation /*= 0*/) {
    /*!
     * Begins a radio button group
     * @param var_name    name of awar
     * @param orientation 0 -> vertical, != 0 horizontal layout
     */
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_if_fail(awar != NULL);

    if (orientation == 0) {
        prvt->toggle_field = gtk_vbox_new(true, 2);
    }
    else {
        prvt->toggle_field = gtk_hbox_new(true, 2);
    }

    prvt->toggle_field_awar_name = ARB_strdup(var_name);
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// force-diff-sync 189273814273 (remove after merging back to trunk)
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------


template <class T>
void AW_window::insert_toggle_internal(const char *toggle_label, const char *mnemonic, T var_value) {
    AW_awar* awar = get_root()->awar_no_error(prvt->toggle_field_awar_name);
    aw_return_if_fail(awar != NULL);

    prvt->radio_last = GTK_RADIO_BUTTON(gtk_radio_button_new_from_widget(prvt->radio_last));

    gtk_container_add(GTK_CONTAINER(prvt->radio_last),
                      make_label(toggle_label, 0, mnemonic));

    gtk_box_pack_start(GTK_BOX(prvt->toggle_field),
                       GTK_WIDGET(prvt->radio_last), true, true, 2);

    prvt->action_template.set_label(toggle_label); // fixme mnemonic

    AW_choice *choice = awar->add_choice(prvt->action_template, var_value);
    choice->bind(GTK_WIDGET(prvt->radio_last), "clicked");
    choice->set_label(toggle_label);
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// force-diff-sync 381892714273 (remove after merging back to trunk)
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------


void AW_window::insert_toggle        (const char *toggle_label, const char *mnemonic, const char *var_value) { insert_toggle_internal(toggle_label, mnemonic, var_value); }
void AW_window::insert_default_toggle(const char *toggle_label, const char *mnemonic, const char *var_value) { insert_toggle_internal(toggle_label, mnemonic, var_value); }
void AW_window::insert_toggle        (const char *toggle_label, const char *mnemonic, int var_value)         { insert_toggle_internal(toggle_label, mnemonic, var_value); }
void AW_window::insert_default_toggle(const char *toggle_label, const char *mnemonic, int var_value)         { insert_toggle_internal(toggle_label, mnemonic, var_value); }
void AW_window::insert_toggle        (const char *toggle_label, const char *mnemonic, float var_value)       { insert_toggle_internal(toggle_label, mnemonic, var_value); }
void AW_window::insert_default_toggle(const char *toggle_label, const char *mnemonic, float var_value)       { insert_toggle_internal(toggle_label, mnemonic, var_value); }

void AW_window::update_toggle_field() { 
    gtk_widget_show_all(prvt->toggle_field);
    GtkAlignment* align = GTK_ALIGNMENT(gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f));
    put_with_label(prvt->toggle_field, align);

    // select the toggle associated with current awar value:
    AW_awar* awar = get_root()->awar_no_error(prvt->toggle_field_awar_name);
    aw_assert(awar);
    if (awar) awar->update_choices();

    prvt->radio_last   = NULL; // end of radio group
    prvt->toggle_field = NULL;
    freenull(prvt->toggle_field_awar_name);
}
