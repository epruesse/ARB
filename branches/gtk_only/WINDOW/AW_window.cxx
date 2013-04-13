// ============================================================= //
//                                                               //
//   File      : Aw_window.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Jul 26, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //
   
#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include "aw_area_management.hxx"
#include "aw_xfig.hxx" 
#include "aw_root.hxx"
#include "aw_device_click.hxx"  
#include "aw_device_size.hxx"
#include "aw_at.hxx"
#include "aw_at_layout.hxx"
#include "aw_msg.hxx"
#include "aw_awar.hxx"
#include "aw_common.hxx"
#include "aw_motif_gtk_conversion.hxx"
#include "aw_modal.hxx"
#include "aw_help.hxx"
#include "aw_varUpdateInfo.hxx" 
#include "aw_option_menu_struct.hxx"
#include "aw_type_checking.hxx"
#include "aw_select.hxx"
#include <gtk/gtk.h>

#include <string>

#ifndef ARBDB_H
#include <arbdb.h>
#endif
//#define DUMP_BUTTON_CREATION

// proxy functions handing down stuff to AW_at
void AW_window::at(int x, int y){ _at.at(x,y); }
void AW_window::at_x(int x) { _at.at_x(x); }
void AW_window::at_y(int y){ _at.at_y(y); }
void AW_window::at_shift(int x, int y){ _at.at_shift(x,y); }
void AW_window::at_newline(){ _at.at_newline(); }
bool AW_window::at_ifdef(const  char *id){  return _at.at_ifdef(id); }
void AW_window::at_set_to(bool attach_x, bool attach_y, int xoff, int yoff) {
  _at.at_set_to(attach_x, attach_y, xoff, yoff);
}
void AW_window::at_unset_to() { _at.at_unset_to(); }
void AW_window::unset_at_commands() { 
    _at.unset_at_commands();     
    prvt->callback = NULL;
    //  prvt->d_callback = NULL;
}
void AW_window::at_set_min_size(int xmin, int ymin) { _at.at_set_min_size(xmin, ymin); }
void AW_window::auto_space(int x, int y){ _at.auto_space(x,y); }
void AW_window::label_length(int length){ _at.label_length(length); }
void AW_window::button_length(int length){ _at.button_length(length); }
int AW_window::get_button_length() const { return _at.get_button_length(); }
void AW_window::get_at_position(int *x, int *y) const { _at.get_at_position(x,y); }
int AW_window::get_at_xposition() const { return _at.get_at_xposition(); }
int AW_window::get_at_yposition() const { return _at.get_at_yposition(); }
void AW_window::at(const char *at_id) { _at.at(at_id); }
void AW_window::sens_mask(AW_active mask){  _at.set_mask(mask); }
void AW_window::restore_at_size_and_attach(const AW_at_size *at_size){
    at_size->restore(_at);
}
void AW_window::store_at_size_and_attach(AW_at_size *at_size) {
    at_size->store(_at);
}

// FIXME: callback ownership not handled, possible memory leaks
void AW_window::callback(void (*f)(AW_window*)){
    prvt->callback = new AW_cb_struct(this, (AW_CB)f);
}
void AW_window::callback(void (*f)(AW_window*, AW_CL), AW_CL cd1){
    prvt->callback = new AW_cb_struct(this, (AW_CB)f, cd1);
}
void AW_window::callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2){
    prvt->callback = new AW_cb_struct(this, (AW_CB)f, cd1, cd2);
}
void AW_window::callback(AW_cb_struct * /* owner */ awcbs) {
    prvt->callback = awcbs;
}

void AW_window::d_callback(void (*f)(AW_window*)) {
    prvt->d_callback = new AW_cb_struct(this, (AW_CB)f);
}
void AW_window::d_callback(void (*f)(AW_window*, AW_CL), AW_CL cd1){
    prvt->d_callback = new AW_cb_struct(this, (AW_CB)f, cd1);
}
void AW_window::d_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2){
    prvt->d_callback = new AW_cb_struct(this, (AW_CB)f, cd1, cd2);
}

/**
 * handler for click-callbacks
 * takes care of recording actions and displaying help on ?-cursor
 */
void AW_window::click_handler(GtkWidget* /*wgt*/, gpointer aw_cb_struct) {
    GTK_PARTLY_IMPLEMENTED;
    printf("in click handler\n");

    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;
    
    AW_root *root = AW_root::SINGLETON;
     
    if (root->is_help_active()) {
        root->set_help_active(false);
        root->normal_cursor();

        if (cbs->help_text && ((GBS_string_matches(cbs->help_text, "*.ps", GB_IGNORE_CASE)) ||
                               (GBS_string_matches(cbs->help_text, "*.hlp", GB_IGNORE_CASE)) ||
                               (GBS_string_matches(cbs->help_text, "*.help", GB_IGNORE_CASE))))
        {
            AW_POPUP_HELP(cbs->aw, (AW_CL)cbs->help_text);
        }
        else {
            aw_message("Sorry no help available");
        }
        return;
    }

    if (root->is_tracking()) root->track_action(cbs->id);

    if (cbs->f == AW_POPUP) {
        cbs->run_callback();
    }
    else {
        root->wait_cursor();
        
        cbs->run_callback();
        FIXME("destruction of old events not implemented");
//        XEvent event; // destroy all old events !!!
//        while (XCheckMaskEvent(XtDisplay(p_global->toplevel_widget),
//        ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
//        KeyPressMask|KeyReleaseMask|PointerMotionMask, &event)) {
//        }
    }

}

void AW_window::_set_activate_callback(GtkWidget *widget) {
    if (prvt->callback && (long)prvt->callback != 1) {
        if (!prvt->callback->help_text && _at.helptext_for_next_button) {
            prvt->callback->help_text = _at.helptext_for_next_button;
            _at.helptext_for_next_button = 0;
        }
        
        g_signal_connect((gpointer)widget, "clicked", G_CALLBACK(AW_window::click_handler),
                         (gpointer)prvt->callback);
    }
    prvt->callback = NULL;
}

AW_area_management* AW_window::get_area(int index) {   
    if(index < AW_MAX_AREA) {
        return prvt->areas[index];
    }   
    return NULL;
}

GtkWidget* AW_window::make_label(const char* label_text, short label_length) {
    aw_assert(label_text);
    GtkWidget *widget;
    char *str;
    
    AW_awar *is_awar = get_root()->label_is_awar(label_text);
    if (is_awar) 
        str = is_awar->read_as_string();
    else 
        str = strdup(label_text);

    if (label_text[0] == '#') {
        widget = gtk_image_new_from_file(GB_path_in_ARBLIB("pixmaps", str+1));
    }
    else {
        widget = gtk_label_new_with_mnemonic(str);
        if (label_length)
            gtk_label_set_width_chars(GTK_LABEL(widget), label_length);
    }
    
    if (is_awar) 
        is_awar->tie_widget(0, widget, AW_WIDGET_LABEL_FIELD, this);

    free(str);
    return widget;
}


/**
 * Places @param *widget according to _at in prvt->fixedArea
 * - creates label if defined in _at (using hbox)
 **/
void AW_window::put_with_label(GtkWidget *widget) {
    #define SPACE_BEHIND 5
    #define SPACE_BELOW 5
    GtkWidget *hbox, *label = 0;

    // create label from label text
    if (_at.label_for_inputfield) {
        label = make_label(_at.label_for_inputfield, _at.length_of_label_for_inputfield);
    }
    
    // pack widget and label into hbox
    // (having/not having the hbox changes appearance!)
    hbox = gtk_hbox_new(false,0);
    if (label) {
        // label does not expand (hence "false,false")
        gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
        //gtk_label_set_mnemonic_widget(
    }
    gtk_box_pack_start(GTK_BOX(hbox), widget, true, true, 0);

    // if size given by xfig, scale hbox
    if (_at.to_position_exists) { 
        aw_assert(_at.to_position_x >_at.x_for_next_button);               
        aw_assert(_at.to_position_y >_at.y_for_next_button);         
        gtk_widget_set_size_request(hbox, 
                                    _at.to_position_x - _at.x_for_next_button,
                                    _at.to_position_y - _at.y_for_next_button);
    }

    GtkRequisition hbox_req;
    gtk_widget_show_all(hbox);
    gtk_widget_size_request(GTK_WIDGET(hbox), &hbox_req);

    aw_at_layout_put(prvt->fixed_size_area, hbox, &_at);

    _at.increment_at_commands(hbox_req.width * (2-_at.correct_for_at_center) / 2. + SPACE_BEHIND, 
                                  hbox_req.height + SPACE_BELOW);
    unset_at_commands();
    prvt->last_widget = hbox;
}

/**
 * Create a button or text display.
 * If a callback was given via at->callback(), creates a button;
 * otherwise creates a label.
 */ 
void AW_window::create_button(const char *macro_name, const char *button_text, 
                              const char */*mnemonic*/, const char */*color*/) {
#if defined(DUMP_BUTTON_CREATION)
    printf("------------------------------ Button '%s'\n", button_text);
    printf("x_for_next_button=%i y_for_next_button=%i\n", _at.x_for_next_button, _at.y_for_next_button);
#endif // DUMP_BUTTON_CREATION

    GtkWidget *button_label, *button;

    button_label = make_label(button_text, _at.length_of_buttons);

    gtk_widget_show(button_label);

    // make 'button' a real button if we've got a callback to run on click
    if (prvt->callback) {
        // register macro
        if (((long)prvt->callback != 1)) {
            if (macro_name) {
              prvt->callback->id = GBS_global_string_copy("%s/%s", this->window_defaults_name, macro_name);
              get_root()->define_remote_command(prvt->callback);
            }
            else {
              prvt->callback->id = 0;
            }
        }

        button = gtk_button_new();
        gtk_container_add(GTK_CONTAINER(button), button_label);
       
        this->_set_activate_callback(button);
        
        if (_at.highlight) {
            gtk_widget_grab_default(button);
        }

        get_root()->register_widget(button, _at.widget_mask);
    } 
    else {
        button = button_label;
    }

    put_with_label(button);
}

void aw_detect_text_size(const char *text, size_t& width, size_t& height) {
    size_t text_width = strcspn(text, "\n");

    if (text[text_width]) {
        aw_assert(text[text_width] == '\n');

        aw_detect_text_size(text+text_width+1, width, height);
        if (text_width>width) width = text_width;
        height++;
    }
    else { // EOS
        width  = text_width;
        height = 1;
    }
}

void AW_window::create_autosize_button(const char *macro_name, const char *buttonlabel, 
                                       const char *mnemonic /* = 0*/, unsigned xtraSpace /* = 1 */){
    aw_assert(buttonlabel[0] != '#');    // use create_button for graphical buttons!
    aw_assert(!_at.to_position_exists); // wont work if to-position exists

    AW_awar *is_awar = get_root()->label_is_awar(buttonlabel);
    size_t   width, height;
    if (is_awar) {
        char *content = is_awar->read_as_string();
        aw_assert(content[0]); /* you need to fill the awar before calling create_autosize_button,
                                * otherwise size cannot be detected */
        aw_detect_text_size(content, width, height);
    }
    else {
        aw_detect_text_size(buttonlabel, width, height);
    }

    int   len               = width+(xtraSpace*2);
    short length_of_buttons = _at.length_of_buttons;
    short height_of_buttons = _at.height_of_buttons;

    _at.length_of_buttons = len+1;
    _at.height_of_buttons = height;
    create_button(macro_name, buttonlabel, mnemonic);
    _at.length_of_buttons = length_of_buttons;
    _at.height_of_buttons = height_of_buttons;
}

GtkWidget* AW_window::get_last_widget() const{
    return prvt->last_widget;
}

void AW_window::create_toggle(const char */*var_name*/){
    GtkWidget* checkButton = gtk_check_button_new();
    put_with_label(checkButton);

    if (prvt->callback) {
      _set_activate_callback(checkButton);
    }
}

void AW_window::create_toggle(const char */*awar_name*/, const char */*nobitmap*/, const char */*yesbitmap*/, int /*buttonWidth*/ /* = 0 */){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_text_toggle(const char */*var_name*/, const char */*noText*/, const char */*yesText*/, int /*buttonWidth*/ /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::update_toggle(GtkWidget */*widget*/, const char */*var*/, AW_CL /*cd_toggle_data*/) {
//    aw_toggle_data *tdata = (aw_toggle_data*)cd_toggle_data;
//    const char     *text  = tdata->bitmapOrText[(var[0] == '0' || var[0] == 'n') ? 0 : 1];
//
//    if (tdata->isTextToggle) {
//        XtVaSetValues(widget, RES_CONVERT(XmNlabelString, text), NULL);
//    }
//    else {
//        char *path = pixmapPath(text+1);
//        XtVaSetValues(widget, RES_CONVERT(XmNlabelPixmap, path), NULL);
//        free(path);
//    }
    GTK_NOT_IMPLEMENTED;
}


static gboolean noop_signal_handler(GtkWidget* /*wgt*/, gpointer /*user_data*/) {
  return true; // stop signal
}

void AW_window::allow_delete_window(bool allow_close) {
    gtk_window_set_deletable(prvt->window, allow_close);
    // the window manager might still show the close button
    // => do nothing if clicked
    g_signal_connect(prvt->window, "delete-event", G_CALLBACK(noop_signal_handler), NULL);
}


static bool AW_value_changed_callback(GtkWidget* /*wgt*/,  gpointer rooti) {
    AW_root::SINGLETON->value_changed = true;
    return false; //this event should propagate further
}

void AW_window::create_input_field(const char *var_name,   int columns) {
    GtkWidget *entry;
    AW_awar *vs = get_root()->awar(var_name);
    aw_assert(NULL != vs);

    // create entry field
    entry = gtk_entry_new();
    if (columns) {
      gtk_entry_set_width_chars(GTK_ENTRY(entry), columns);
    } 
    else {
      gtk_entry_set_width_chars(GTK_ENTRY(entry), _at.length_of_buttons);
    }
    char *str   = vs->read_as_string();
    gtk_entry_set_text(GTK_ENTRY(entry), str);
    free(str);

    put_with_label(entry);

    AW_varUpdateInfo *vui;
    vui = new AW_varUpdateInfo(this, entry, AW_WIDGET_INPUT_FIELD, vs, prvt->callback);

    // callback for enter
    g_signal_connect(G_OBJECT(entry), "activate",
                     G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback),
                     (gpointer) vui);
    
    if (prvt->d_callback) {
        g_signal_connect(G_OBJECT(entry), "activate",
                         G_CALLBACK(AW_window::click_handler),
                         (gpointer) prvt->d_callback);
        prvt->d_callback->id = GBS_global_string_copy("INPUT:%s", var_name);
        get_root()->define_remote_command(prvt->d_callback);
    }
   
    // callback for losing focus
    g_signal_connect(G_OBJECT(entry), "focus-out-event",
                     G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback_event),
                     (gpointer) vui);  
    // callback for value changed
    g_signal_connect(G_OBJECT(entry), "changed",
                     G_CALLBACK(AW_value_changed_callback),
                     (gpointer) get_root());
    vs->tie_widget(0, entry, AW_WIDGET_INPUT_FIELD, this);

    get_root()->register_widget(entry, _at.widget_mask);
}

void AW_window::create_text_field(const char */*awar_name*/, int /*columns*/ /* = 20 */, int /*rows*/ /*= 4*/){
    GTK_NOT_IMPLEMENTED;
}


void AW_window::create_menu(const char *name, const char *mnemonic, AW_active mask /*= AWM_ALL*/){
    aw_assert(legal_mask(mask));
        
    GTK_PARTLY_IMPLEMENTED;

//    #if defined(DUMP_MENU_LIST)
//        dumpCloseAllSubMenus();
//    #endif // DUMP_MENU_LIST
        
    // The user might leave sub menus open. Close them before creating a new top level menu.
    while(prvt->menus.size() > 1) {
        close_sub_menu();
    }
    
    insert_sub_menu(name, mnemonic, mask);
}

void AW_window::close_sub_menu(){
    arb_assert(NULL != prvt);
    arb_assert(prvt->menus.size() > 1);
    
    prvt->menus.pop();
    
}


char *AW_window::align_string(const char *label_text, int columns) {
    // shortens or expands 'label_text' to 'columns' columns
    // if label_text contains '\n', each "line" is handled separately

    const char *lf     = strchr(label_text, '\n');
    char       *result = 0;

    if (!lf) {
        result = (char*)malloc(columns+1);

        int len              = strlen(label_text);
        if (len>columns) len = columns;

        memcpy(result, label_text, len);
        if (len<columns) memset(result+len, ' ', columns-len);
        result[columns] = 0;
    }
    else {
        char *part1    = GB_strpartdup(label_text, lf-1);
        char *aligned1 = align_string(part1, columns);
        char *aligned2 = align_string(lf+1, columns);

        result = GBS_global_string_copy("%s\n%s", aligned1, aligned2);

        free(aligned2);
        free(aligned1);
        free(part1);
    }
    return result;
}

/**
 * Is called whenever the selection of a combobox changes
 */
void AW_combo_box_changed(GtkComboBox* box, gpointer user_data) {
    
    AW_option_menu_struct *oms = (AW_option_menu_struct*) user_data;
    aw_assert(NULL != oms);
    
    gchar *selected_option_name = gtk_combo_box_get_active_text(box);
    
    aw_assert(NULL != selected_option_name);
    aw_assert(oms->valueToUpdateInfo.find(selected_option_name) != oms->valueToUpdateInfo.end());
    
    AW_varUpdateInfo *vui = oms->valueToUpdateInfo[selected_option_name];
    
    //invoke callback
    AW_varUpdateInfo::AW_variable_update_callback(GTK_WIDGET(box), vui);
    
    
}

AW_option_menu_struct *AW_window::create_option_menu(const char *awar_name, 
                                                     const char *tmp_label, 
                                                     const char *){
    AW_awar *vs = get_root()->awar(awar_name);
    aw_assert(vs);

    if (tmp_label) this->label(tmp_label);
    prvt->combo_box = gtk_combo_box_new_text();
    gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(prvt->combo_box), GTK_SENSITIVITY_ON);
 
    get_root()->number_of_option_menus++;


    AW_option_menu_struct *next =
        new AW_option_menu_struct(get_root()->number_of_option_menus,
                                  awar_name,
                                  vs->variable_type,
                                  prvt->combo_box);

    if (get_root()->option_menu_list) {
        get_root()->last_option_menu->next = next;
        get_root()->last_option_menu = get_root()->last_option_menu->next;
    }
    else {
        get_root()->last_option_menu = get_root()->option_menu_list = next;
    }
    get_root()->current_option_menu = get_root()->last_option_menu;

    vs->tie_widget((AW_CL)get_root()->current_option_menu, prvt->combo_box, AW_WIDGET_CHOICE_MENU, this);
    
    //connect changed signal
    g_signal_connect(G_OBJECT(prvt->combo_box), "changed", G_CALLBACK(AW_combo_box_changed), (gpointer) next);
   
    get_root()->register_widget(prvt->combo_box, _at.widget_mask);
  
    return get_root()->current_option_menu;
}

void AW_window::insert_option(const char *on, const char *mn, const char *vv, const char *noc) {
        insert_option_internal(on, mn, vv, noc, false); 
}

void AW_window::insert_default_option(const char *on, const char *mn, const char *vv, const char *noc) { 
    insert_option_internal(on, mn, vv, noc, true); 

}
void AW_window::insert_option(const char *on, const char *mn, int vv, const char *noc) { 
    insert_option_internal(on, mn, vv, noc, false);
}

void AW_window::insert_default_option(const char *on, const char *mn, int vv, const char *noc) { 
    insert_option_internal(on, mn, vv, noc, true); 

}
void AW_window::insert_option(const char *on, const char *mn, float vv, const char *noc) { 
    insert_option_internal(on, mn, vv, noc, false); 
}

void AW_window::insert_default_option(const char *on, const char *mn, float vv, const char *noc) { 
    insert_option_internal(on, mn, vv, noc, true); 
}


template <class T>
void AW_window::insert_option_internal(const char *option_name, const char *mnemonic, T var_value, const char *name_of_color, bool default_option) {
    AW_option_menu_struct *oms = get_root()->current_option_menu;
    
    aw_assert(NULL != oms); //current option menu has to be set
    aw_assert(oms->variable_type == AW_TypeCheck::getVarType(var_value)); //type missmatch
    
    if (oms->variable_type == AW_TypeCheck::getVarType(var_value)) {
        
        FIXME("background color not implemented");
        gtk_combo_box_append_text(GTK_COMBO_BOX(oms->menu_widget), option_name);
        

        AW_cb_struct *cbs   = prvt->callback; // user-own callback

        //create VarUpdateInfo for new choice and add it to the oms.
        //AW_variable_update_callback() will be called with this VarUpdateInfo
        //This rather complicated structure is needed because the old interface
        //allows for every entry to have it's own callback method.
        AW_varUpdateInfo* vui = new AW_varUpdateInfo(this,
                                               NULL,
                                               AW_WIDGET_CHOICE_MENU,
                                               get_root()->awar(oms->variable_name),
                                               var_value, cbs);
        //check for duplicate option name
        aw_assert(oms->valueToUpdateInfo.find(option_name) == oms->valueToUpdateInfo.end());
        
        oms->valueToUpdateInfo[option_name] = vui;
        oms->add_option(option_name, default_option);
        
        if (default_option) {
          gtk_combo_box_set_active(GTK_COMBO_BOX(oms->menu_widget), 
                                   oms->options.size()-1);
        }

       
        FIXME("setting sensitivity of option menu entries not implemented");
        // get_root()->register_widget(entry, _at->widget_mask);
        this->unset_at_commands();
    } else {
      printf("arg\n");
    }
}

void AW_window::update_option_menu() {
    put_with_label(prvt->combo_box);
}


void AW_window::refresh_option_menu(AW_option_menu_struct */*oms*/) {
    GTK_NOT_IMPLEMENTED;
//    if (get_root()->changer_of_variable != oms->label_widget) {
//        AW_widget_value_pair *active_choice = oms->first_choice;
//        {
//            AW_scalar global_var_value(get_root()->awar(oms->variable_name));
//            while (active_choice && global_var_value != active_choice->value) {
//                active_choice = active_choice->next;
//            }
//        }
//
//        if (!active_choice) active_choice = oms->default_choice;
//        //if (active_choice) XtVaSetValues(oms->label_widget, XmNmenuHistory, active_choice->widget, NULL);
//    }
  
}

void AW_window::clear_option_menu(AW_option_menu_struct */*oms*/) {
    GTK_NOT_IMPLEMENTED;
}



GType AW_window::convert_aw_type_to_gtk_type(AW_VARIABLE_TYPE aw_type){

    switch(aw_type) {
        //inconvertable types
        case AW_NONE:
        case AW_TYPE_MAX:
        case AW_DB:
            return G_TYPE_INVALID;
        case AW_BIT:
            FIXME("Not sure if AW_BIT and G_TYPE_FLAGS are the same");
            return G_TYPE_FLAGS;
        case AW_BYTE:
            return G_TYPE_UCHAR;
        case AW_INT:
            return G_TYPE_INT;
        case AW_FLOAT:
            return G_TYPE_FLOAT;
        case AW_POINTER:
            return G_TYPE_POINTER;
        case AW_BITS:
            return G_TYPE_FLAGS;
        case AW_BYTES:
            return G_TYPE_BYTE_ARRAY;
        case AW_INTS:
            FIXME("Warning: AW_INTS converted to G_TYPE_ARRAY.");
            return G_TYPE_ARRAY;
        case AW_FLOATS:
            FIXME("Warning: AW_FLOATS converted to G_TYPE_ARRAY.");
            return G_TYPE_ARRAY;      
        case AW_STRING:
            return G_TYPE_STRING;
        default:
            aw_assert(false);
            return G_TYPE_INVALID;
    }
}

static void  row_activated_cb (GtkTreeView       *tree_view,
                        GtkTreePath       *path,
                        GtkTreeViewColumn *column,
                        gpointer           user_data) {
  AW_cb_struct *cbs = (AW_cb_struct *) user_data;
  cbs->run_callback();
}

AW_selection_list* AW_window::create_selection_list(const char *var_name, int columns, int rows) {
    aw_assert(var_name); 
    aw_assert(!_at.label_for_inputfield); // labels have no effect for selection lists

    GTK_PARTLY_IMPLEMENTED;
    GtkListStore *store;
    GtkWidget *tree;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    AW_awar *vs = get_root()->awar(var_name);
    aw_assert(vs);
    AW_varUpdateInfo *vui = 0;
    AW_cb_struct  *cbs = 0;
    
    tree = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

    renderer = gtk_cell_renderer_text_new ();
    g_object_set(renderer, "family", "monospace", NULL);
    column = gtk_tree_view_column_new_with_attributes("List Items",
             renderer, "text", 0, NULL); //column headers are disabled, text does not matter
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

    store = gtk_list_store_new(1, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));

    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), tree);

    
    int width_of_list;
    int height_of_list;
    int width_of_last_widget  = 0;
    int height_of_last_widget = 0;

    int char_width, char_height;
    prvt->get_font_size(char_width, char_height);
    gtk_widget_set_size_request(scrolled_win, char_width * columns, char_height * rows);

    put_with_label(scrolled_win);

    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);

    int type = vs->variable_type;
    get_root()->append_selection_list(new AW_selection_list(var_name, type, GTK_TREE_VIEW(tree)));

    vui = new AW_varUpdateInfo(this, tree, AW_WIDGET_SELECTION_LIST, vs, prvt->callback);
    vui->set_sellist(get_root()->get_last_selection_list());

    g_signal_connect(G_OBJECT(selection), "changed", 
                     G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback), 
                     (gpointer) vui);

    if (prvt->d_callback) {
        g_signal_connect(G_OBJECT(tree), "row-activated", 
                         G_CALLBACK(row_activated_cb),
                         (gpointer) prvt->d_callback);
    }

    vs->tie_widget((AW_CL)get_root()->get_last_selection_list(), tree, AW_WIDGET_SELECTION_LIST, this);
    get_root()->register_widget(tree, _at.widget_mask);
    
    return get_root()->get_last_selection_list();
}

// BEGIN TOGGLE FIELD STUFF

void AW_window::create_toggle_field(const char *awar_name, int orientation /*= 0*/){
    // orientation = 0 -> vertical else horizontal layout

    if (orientation == 0) {
      prvt->toggle_field = gtk_vbox_new(true, 2);
    } 
    else {
      prvt->toggle_field = gtk_hbox_new(true, 2);
    }
    
    prvt->toggle_field_awar_name = awar_name;
    
    get_root()->register_widget(prvt->toggle_field, _at.widget_mask);
}

void AW_window::create_toggle_field(const char *var_name, const char *labeli, const char */*mnemonic*/) {
    if (labeli) this->label(labeli);
    create_toggle_field(var_name);
}

template <class T>
void AW_window::insert_toggle_internal(const char *toggle_label, const char *mnemonic, 
                                       T var_value, bool default_toggle) {
    GtkWidget *radio;  
    std::string mnemonicLabel = AW_motif_gtk_conversion::convert_mnemonic(toggle_label, mnemonic);
    // create and chain radio button
    radio = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(prvt->radio_last), 
                                                           mnemonicLabel.c_str());
    prvt->radio_last = radio;
    
    gtk_box_pack_start(GTK_BOX(prvt->toggle_field), radio, true, true, 2);
    
    AW_varUpdateInfo *vui = new AW_varUpdateInfo(this, NULL, AW_WIDGET_TOGGLE_FIELD,
                                                 get_root()->awar(prvt->toggle_field_awar_name), 
                                                 var_value, prvt->callback);
    g_signal_connect((gpointer)radio, "clicked", 
                     G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback),
                     (gpointer)vui);
    
    if(default_toggle){
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), true);
    }
}

void AW_window::insert_toggle        (const char *toggle_label, const char *mnemonic, const char *var_value)
{ insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(const char *toggle_label, const char *mnemonic, const char *var_value)
{ insert_toggle_internal(toggle_label, mnemonic, var_value, true); }
void AW_window::insert_toggle        (const char *toggle_label, const char *mnemonic, int var_value)           
{ insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(const char *toggle_label, const char *mnemonic, int var_value)           
{ insert_toggle_internal(toggle_label, mnemonic, var_value, true); }
void AW_window::insert_toggle        (const char *toggle_label, const char *mnemonic, float var_value)         
{ insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(const char *toggle_label, const char *mnemonic, float var_value)        
{ insert_toggle_internal(toggle_label, mnemonic, var_value, true); }


void AW_window::update_toggle_field() { 
    gtk_widget_show_all(prvt->toggle_field);
    put_with_label(prvt->toggle_field);

    prvt->radio_last = NULL; // end of radio group
    prvt->toggle_field = NULL;
}

void AW_window::refresh_toggle_field(int /*toggle_field_number*/) {
    GTK_NOT_IMPLEMENTED;
}
// END TOGGLE FIELD STUFF



void AW_window::draw_line(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, int /*width*/, bool /*resize*/){
   GTK_NOT_IMPLEMENTED;
}


AW_device_click *AW_window::get_click_device(AW_area area, int mousex, int mousey,
                                             AW_pos max_distance_linei, AW_pos max_distance_texti, AW_pos radi) {
    AW_area_management *aram         = prvt->areas[area];
    AW_device_click    *click_device = NULL;

    if (aram) {
        click_device = aram->get_click_device();
        click_device->init(mousex, mousey, max_distance_linei,
                           max_distance_texti, radi, AW_ALL_DEVICES);
    }
    return click_device;
}
AW_device *AW_window::get_device(AW_area area){
    AW_area_management *aram   = prvt->areas[area];
    arb_assert(NULL != aram);
    return (AW_device *)aram->get_screen_device();
}
  
void AW_window::get_event(AW_event *eventi) const {
    *eventi = event;
}

AW_device_print *AW_window::get_print_device(AW_area /*area*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_device_size *AW_window::get_size_device(AW_area area){
    AW_area_management *aram        = prvt->areas[area];
    AW_device_size     *size_device = NULL;

    if (aram) {
        size_device = aram->get_size_device();
        size_device->forget_tracked();
        size_device->reset(); // @@@ hm
    }
    return size_device;
}

void AW_window::get_window_size(int& width, int& height){
    prvt->get_size(width,height);
}

void AW_window::help_text(const char */*id*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::hide(){
    gtk_widget_hide(GTK_WIDGET(prvt->window));
}

void AW_window::hide_or_notify(const char */*error*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::highlight(){
    GTK_NOT_IMPLEMENTED;
}




void AW_window::insert_help_topic(const char *name, const char */*mnemonic*/, const char */*help_text_*/, AW_active /*mask*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/){
    aw_assert(NULL != prvt->help_menu);
    
    FIXME("mnemonic not working");
    FIXME("help entry callback not working");
    FIXME("Help text not working");
    GtkWidget *item = gtk_menu_item_new_with_label(name);
    gtk_menu_shell_append(prvt->help_menu, item);
    
        
}


void AW_window::insert_menu_topic(const char *topic_id, const char *name, const char *mnemonic, const char *helpText, AW_active mask, AW_CB f, AW_CL cd1, AW_CL cd2){
   aw_assert(legal_mask(mask));
   aw_assert(prvt->menus.size() > 0); //closed too many menus
  
   std::string topicName = AW_motif_gtk_conversion::convert_mnemonic(name, mnemonic);
   
   if (!topic_id) topic_id = name; // hmm, due to this we cannot insert_menu_topic w/o id. Change? @@@

   GtkWidget *label = make_label(name, 0);
   GtkWidget *alignment = gtk_alignment_new(0.f, 0.5f, 0.f, 0.f);
   gtk_container_add(GTK_CONTAINER(alignment), label);
   GtkWidget *item = gtk_menu_item_new();
   gtk_container_add(GTK_CONTAINER(item),alignment);

   gtk_menu_shell_append(prvt->menus.top(), item);  
   
#if defined(DUMP_MENU_LIST)
 //   dumpMenuEntry(name);
#endif // DUMP_MENU_LIST

    AW_cb_struct *cbs = new AW_cb_struct(this, f, cd1, cd2, helpText);
    
    
    g_signal_connect((gpointer)item, "activate", G_CALLBACK(AW_window::click_handler), (gpointer)cbs);

    cbs->id = strdup(topic_id);
    get_root()->define_remote_command(cbs);
    get_root()->register_widget(item, mask);
}

void AW_window::insert_sub_menu(const char *name, const char *mnemonic, AW_active mask /*= AWM_ALL*/){
    aw_assert(legal_mask(mask));
    aw_assert(NULL != prvt->menus.top());
    
    //construct mnemonic string
    std::string menuName = AW_motif_gtk_conversion::convert_mnemonic(name, mnemonic);
    
    //create new menu item with attached submenu
    GtkMenu *submenu  = GTK_MENU(gtk_menu_new());
    GtkMenuItem *item = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(menuName.c_str()));
    
    gtk_menu_item_set_submenu(item, GTK_WIDGET(submenu));

    if (prvt->menus.size() == 1) { // Insert new menu second-to-last (last is HELP)
      // Count entries in menu
      GList *menu_items =  gtk_container_get_children(GTK_CONTAINER(prvt->menus.top()));
      int menu_item_cnt = g_list_length(menu_items); 
      g_list_free(menu_items);
      // Insert at n-1
      gtk_menu_shell_insert(prvt->menus.top(), GTK_WIDGET(item), menu_item_cnt-1);
    } 
    else {
      // Append the new submenu to the current menu shell
      gtk_menu_shell_append(prvt->menus.top(), GTK_WIDGET(item));
    }
    
    // use the new submenu as current menu shell.
    prvt->menus.push(GTK_MENU_SHELL(submenu));

    // add tearof item
    gtk_menu_shell_append(prvt->menus.top(), gtk_tearoff_menu_item_new());
       
    #if defined(DUMP_MENU_LIST)
        dumpOpenSubMenu(name);
    #endif // DUMP_MENU_LIST

    get_root()->register_widget(GTK_WIDGET(item), mask);
}



bool AW_window::is_shown() const{
    return gtk_widget_get_visible(GTK_WIDGET(prvt->window));
}

/* set label for next button 
 */
void AW_window::label(const char *_label){
    freedup(_at.label_for_inputfield, _label);
}

void AW_window::update_text_field(GtkWidget */*widget*/, const char */*var_value*/) {
   // XtVaSetValues(widget, XmNvalue, var_value, NULL);
    GTK_NOT_IMPLEMENTED;
}

void AW_window::update_input_field(GtkWidget */*widget*/, const char */*var_value*/) {
    //XtVaSetValues(widget, XmNvalue, var_value, NULL);
    GTK_NOT_IMPLEMENTED;
}



static void AW_xfigCB_info_area(AW_window *aww, AW_xfig *xfig) {
    AW_device *device = aww->get_device(AW_INFO_AREA);
    device->reset();
    device->set_offset(AW::Vector(-xfig->minx, -xfig->miny));

    xfig->print(device);
}

void AW_window::get_font_size(int& width, int& height) {
  prvt->get_font_size(width, height);
}

void AW_window::load_xfig(const char *file, bool resize /*= true*/){
    int width, height;
    prvt->get_font_size(width, height);
  
    FIXME("scaling to 80% for unknown reason -- just fits that way");
    if (file)   xfig_data = new AW_xfig(file, width*0.8, height*0.8);
    else        xfig_data = new AW_xfig(width, height); // create an empty xfig

    xfig_data->create_gcs(get_device(AW_INFO_AREA)); 

    _at.set_xfig(xfig_data);

    AW_device *device = get_device(AW_INFO_AREA);

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
        set_window_size(_at.max_x_size, _at.max_y_size);

        FIXME("this call should not be necessary. The screen size should have been set by the resize_callback...");
        device->get_common()->set_screen_size(_at.max_x_size, _at.max_y_size);

    }

    set_expose_callback(AW_INFO_AREA, (AW_CB)AW_xfigCB_info_area, (AW_CL)xfig_data, 0);
}

const char *AW_window::local_id(const char */*id*/) const{
    GTK_NOT_IMPLEMENTED;
    return 0;
}





void AW_window::sep______________() {
    
    aw_assert(NULL != prvt);
    aw_assert(prvt->menus.size() > 0);
    
    GtkWidget *item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(prvt->menus.top(), item);
    
}

void AW_window::set_bottom_area_height(int /*height*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_expose_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1 /*= 0*/, AW_CL cd2 /*= 0*/) {
    AW_area_management *aram = prvt->areas[area];
    if (aram) aram->set_expose_callback(this, f, cd1, cd2);
}


static void AW_focusCB(GtkWidget* /*wgt*/, gpointer cl_aww) {
    AW_window *aww = (AW_window*)cl_aww;
    aww->run_focus_callback();
}

void AW_window::run_focus_callback() {

    FIXME("i have never actually seen this. But set_focus_callback is called several times.");
    if (prvt->focus_cb) prvt->focus_cb->run_callback();
}

void AW_window::set_focus_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    if (!prvt->focus_cb) {
        g_signal_connect(G_OBJECT(prvt->areas[AW_MIDDLE_AREA]->get_area()),
                "focus", G_CALLBACK(AW_focusCB), (gpointer) this);
    }
    if (!prvt->focus_cb || !prvt->focus_cb->contains(f)) {
        prvt->focus_cb = new AW_cb_struct(this, f, cd1, cd2, 0, prvt->focus_cb);
    }
}


void AW_window::set_info_area_height(int /*height*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_input_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    AW_area_management *aram = prvt->areas[area];
    if (!aram)
        return;
    aram->set_input_callback(this, f, cd1, cd2);
}

void AW_window::set_motion_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    AW_area_management *aram = prvt->areas[area];
    if (!aram)
        return;
    aram->set_motion_callback(this, f, cd1, cd2);
}

void AW_window::set_popup_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_resize_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1 /*= 0*/, AW_CL cd2 /*= 0*/) {
    AW_area_management *aram = prvt->areas[area];
    if (!aram)
        return;
    aram->set_resize_callback(this, f, cd1, cd2);
}



// SCROLL BAR STUFF

void AW_window::tell_scrolled_picture_size(AW_screen_area rectangle) {
    picture->l = rectangle.l;
    picture->r = rectangle.r;
    picture->t = rectangle.t;
    picture->b = rectangle.b;
}

void AW_window::tell_scrolled_picture_size(AW_world rectangle) {
    picture->l = (int)rectangle.l;
    picture->r = (int)rectangle.r;
    picture->t = (int)rectangle.t;
    picture->b = (int)rectangle.b;
}

AW_pos AW_window::get_scrolled_picture_width() const {
    return (picture->r - picture->l);
}

AW_pos AW_window::get_scrolled_picture_height() const {
    return (picture->b - picture->t);
}

void AW_window::reset_scrolled_picture_size() {
    picture->l = 0;
    picture->r = 0;
    picture->t = 0;
    picture->b = 0;
}

void AW_window::_get_area_size(AW_area area, AW_screen_area *square) {
    AW_area_management *aram = prvt->areas[area];
    *square = aram->get_common()->get_screen();
}

void AW_window::get_scrollarea_size(AW_screen_area *square) {
    _get_area_size(AW_MIDDLE_AREA, square);
    square->r -= left_indent_of_horizontal_scrollbar;
    square->b -= top_indent_of_vertical_scrollbar;
}

void AW_window::calculate_scrollbars(){

    AW_screen_area scrollArea;
    get_scrollarea_size(&scrollArea);

    const int width = (int)get_scrolled_picture_width();
    const int height = (int)get_scrolled_picture_height();
    const int scrollArea_width = scrollArea.r - scrollArea.l;
    const int scrollArea_height= scrollArea.b - scrollArea.t;
    
    aw_drawing_area_set_picture_size(prvt->drawing_area, width, height, scrollArea_width, scrollArea_height);
    
    char buffer[200];
     sprintf(buffer, "window/%s/horizontal_page_increment", window_defaults_name);   
    const int hpage_increment = scrollArea.r * get_root()->awar(buffer)->read_int() / 100;
    sprintf(buffer, "window/%s/scroll_width_horizontal", window_defaults_name);
    const int hstep_increment = get_root()->awar(buffer)->read_int();
    sprintf(buffer, "window/%s/vertical_page_increment", window_defaults_name);   
    const int vpage_increment = scrollArea.b * get_root()->awar(buffer)->read_int() / 100;
    sprintf(buffer, "window/%s/scroll_width_vertical", window_defaults_name);
    const int vstep_increment = get_root()->awar(buffer)->read_int();

    aw_drawing_area_set_increments(prvt->drawing_area, hstep_increment, hpage_increment,
                                   vstep_increment, vpage_increment);
}
 

static void vertical_scrollbar_redefinition_cb(AW_root*, AW_CL cd) {
    GTK_NOT_IMPLEMENTED;
/*
    AW_window *aw = (AW_window *)cd;
    aw->update_scrollbar_settings_from_awars(AW_VERTICAL);
*/
}

static void value_changed_scroll_bar_vertical(GtkAdjustment *adjustment, gpointer user_data){
    AW_cb_struct *cbs = (AW_cb_struct *) user_data;
    cbs->aw->slider_pos_vertical = gtk_adjustment_get_value(adjustment);
    cbs->run_callback();

}
static void horizontal_scrollbar_redefinition_cb(AW_root*, AW_CL cd) {
    GTK_NOT_IMPLEMENTED;
/*
    AW_window *aw = (AW_window *)cd;
    aw->update_scrollbar_settings_from_awars(AW_HORIZONTAL);
*/
}
static void value_changed_scroll_bar_horizontal(GtkAdjustment *adjustment, gpointer user_data){
    AW_cb_struct *cbs = (AW_cb_struct *) user_data;
    (cbs->aw)->slider_pos_horizontal = gtk_adjustment_get_value(adjustment);
    cbs->run_callback();
}


void AW_window::set_vertical_scrollbar_position(int position){
#if defined(DEBUG) && 0
    fprintf(stderr, "set_vertical_scrollbar_position to %i\n", position);
#endif
    
    slider_pos_vertical = position;
    aw_drawing_area_set_vertical_slider(prvt->drawing_area, position);
}

void AW_window::set_horizontal_scrollbar_position(int position) {
#if defined(DEBUG) && 0
    fprintf(stderr, "set_horizontal_scrollbar_position to %i\n", position);
#endif
    slider_pos_horizontal = position;
    aw_drawing_area_set_horizontal_slider(prvt->drawing_area, position);
}


void AW_window::set_vertical_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    aw_assert(NULL != prvt->drawing_area);
    
    GtkAdjustment *hAdj = aw_drawing_area_get_vertical_adjustment(prvt->drawing_area);
    
    g_signal_connect((gpointer)hAdj, "value_changed",
                     G_CALLBACK(value_changed_scroll_bar_vertical),
                     (gpointer)new AW_cb_struct(this, f, cd1, cd2, ""));

}

void AW_window::set_horizontal_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    aw_assert(NULL != prvt->drawing_area);
    GtkAdjustment *hAdj = aw_drawing_area_get_horizontal_adjustment(prvt->drawing_area);
    g_signal_connect((gpointer)hAdj, "value_changed",
                     G_CALLBACK(value_changed_scroll_bar_horizontal),
                     (gpointer)new AW_cb_struct(this, f, cd1, cd2, ""));

}

void AW_window::set_vertical_scrollbar_top_indent(int indent) {
  top_indent_of_vertical_scrollbar =  indent;
}

void AW_window::set_horizontal_scrollbar_left_indent(int indent) {
  left_indent_of_horizontal_scrollbar = indent;
}


// END SCROLLBAR STUFF



void AW_window::set_window_size(int width, int height) {
    // only used from GDE once (looks like a hack) -- delete?
    prvt->set_size(width, height); 
}

void AW_window::set_window_title(const char *title){
    prvt->set_title(title);
    freedup(window_name, title);
}

void AW_window::shadow_width (int /*shadow_thickness*/) {
    // GTK_NOT_IMPLEMENTED;
    // won't implement
}

void AW_window::show() {
    prvt->show();
}

void AW_window::button_height(int height) {
    _at.height_of_buttons = height>1 ? height : 0; 
}

void AW_window::show_modal() {
    recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    get_root()->current_modal_window = this;
    activate();
}
bool AW_window::is_expose_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL)) {
    AW_area_management *aram = prvt->areas[area];
    return aram && aram->is_expose_callback(this, f);
}

bool AW_window::is_focus_callback(void (*f)(AW_window*, AW_CL, AW_CL)) {
    return prvt->focus_cb && prvt->focus_cb->contains(f);
}

bool AW_window::is_resize_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL)) {
    AW_area_management *aram = prvt->areas[area];
    return aram && aram->is_resize_callback(this, f);
}

void AW_window::update_label(GtkWidget* widget, const char *var_value) {
    if (get_root()->changer_of_variable != widget) {
        FIXME("this will break if the labels content should switch from text to image or vice versa");
        if (GTK_IS_HBOX(widget)) {
          widget = GTK_WIDGET(gtk_container_get_children(GTK_CONTAINER(widget))->data);
        }

        if(GTK_IS_BUTTON(widget)) {
            gtk_button_set_label(GTK_BUTTON(widget), var_value);   
        }
        else if(GTK_IS_LABEL(widget)) {
            gtk_label_set_text_with_mnemonic(GTK_LABEL(widget), var_value);
        }
        else {
            aw_assert(false); //unable to set label of unknown type
        }
        
    }
    else {
        get_root()->changer_of_variable = 0;
    }
}

void AW_window::window_fit() {
    // let gtk do the sizing based on content
    // (default size will be ignored if requisition of 
    //  children is greater)
    set_window_size(0,0);
}

/* pops window to front */
void AW_window::wm_activate() {
    gtk_window_present(prvt->window);
}

void AW_window::create_inverse_toggle(const char */*awar_name*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::auto_increment(int /*dx*/, int /*dy*/) {
    GTK_NOT_IMPLEMENTED;
}

AW_window::AW_window() 
  : recalc_size_at_show(AW_KEEP_SIZE),
    left_indent_of_horizontal_scrollbar(0),
    top_indent_of_vertical_scrollbar(0),
    prvt(new AW_window::AW_window_gtk()),
    _at(this),
    event(),
    click_time(0),
    color_table_size(0),
    color_table(NULL),
    number_of_timed_title_changes(0),
    xfig_data(NULL),
    window_name(NULL),
    window_defaults_name(NULL),
    window_is_shown(false),
    slider_pos_vertical(0),
    slider_pos_horizontal(0),
    main_drag_gc(0),
    picture(new AW_screen_area)
{
    aw_assert(AW_root::SINGLETON); // must have AW_root
  
    reset_scrolled_picture_size();
    
    prvt = new AW_window::AW_window_gtk();
}

AW_window::~AW_window() {
    delete picture;
    delete prvt;
}

void AW_window::init_window(const char *window_name, const char* window_title,
                            int width, int height, bool resizable) {
    // clean unwanted chars from window_name 
    window_defaults_name = GBS_string_2_key(window_name);

    // get (and create if necessary) x,y,w,h awars for window
    std::string prop_root = std::string("window/windows/") + window_defaults_name;
    awar_posx   = get_root()->awar_int((prop_root + "/posx").c_str(), 0);
    awar_posy   = get_root()->awar_int((prop_root + "/posy").c_str(), 0);
    awar_width  = get_root()->awar_int((prop_root + "/height").c_str(), width);
    awar_height = get_root()->awar_int((prop_root + "/width").c_str(), height);
        
    // register window by name
    if (!GBS_read_hash(get_root()->hash_for_windows, window_defaults_name)) {
        GBS_write_hash(get_root()->hash_for_windows, window_defaults_name, (long)this);
    }
    set_window_title(window_title);
    
    set_window_size(awar_width->read_int(), awar_height->read_int());
    prvt->set_resizable(resizable);

    // set minimum window size to size provided by init
    if (width  > _at.max_x_size) _at.max_x_size = width;
    if (height > _at.max_y_size) _at.max_y_size = height;

    // manage transience:
    // the first created window is considered the main application
    // window. should it be closed, the next created window supersedes it.
    // all other windows are "transient", i.e. dialogs relative to it.
    // (relevant for window placement by window manager)
    if (!get_root()->root_window || !get_root()->root_window->is_shown()) {
        // no root window yet, or root window not visible => I'm root
        get_root()->root_window = this;
    }
    else {
        // there is a root, we're a transient to it
        gtk_window_set_transient_for(prvt->window, 
                                     get_root()->root_window->prvt->window);
    }

    
    // try setting a window icon 
    GtkIconTheme *theme = gtk_icon_theme_get_default();
    const char* icon_name = NULL;
    if (gtk_icon_theme_has_icon(theme, window_defaults_name)) {
      icon_name = window_defaults_name;
    }
    else if (gtk_icon_theme_has_icon(theme, get_root()->program_name)) {
      icon_name = get_root()->program_name;
    }
    if (icon_name) {
      // sadly, this doesn't work:
      // gtk_window_set_icon_name(prvt->window, get_root()->program_name);
      // => do it manually
      GtkIconInfo *ii = gtk_icon_theme_lookup_icon(theme, get_root()->program_name, 
                                                   128, GTK_ICON_LOOKUP_GENERIC_FALLBACK);
      GError *err = NULL;
      gtk_window_set_icon_from_file(prvt->window, gtk_icon_info_get_filename(ii), &err);
      if (err) fprintf(stderr, "Setting icon for window %s failed with message '%s'",
                       window_defaults_name, err->message);
    } 

    // create area for buttons at top ('info area')
    prvt->fixed_size_area = AW_AT_LAYOUT(aw_at_layout_new());
    // we want this to have its own GdkWindow
    gtk_widget_set_has_window(GTK_WIDGET(prvt->fixed_size_area),true);
    prvt->areas[AW_INFO_AREA] = new AW_area_management(GTK_WIDGET(prvt->fixed_size_area), 
                                                       GTK_WIDGET(prvt->fixed_size_area)); 
}

void AW_window::recalc_pos_atShow(AW_PosRecalc /*pr*/){
    GTK_NOT_IMPLEMENTED;
}

AW_PosRecalc AW_window::get_recalc_pos_atShow() const {
    GTK_NOT_IMPLEMENTED;
    return AW_KEEP_POS; // 0
}

void AW_window::recalc_size_atShow(enum AW_SizeRecalc sr) {
    if (sr == AW_RESIZE_ANY) {
        sr = (recalc_size_at_show == AW_RESIZE_USER) ? AW_RESIZE_USER : AW_RESIZE_DEFAULT;
    }
    recalc_size_at_show = sr;
}

void AW_window::on_hide(aw_hide_cb /*call_on_hide*/){
    GTK_NOT_IMPLEMENTED;
}


AW_color_idx AW_window::alloc_named_data_color(int colnum, char *colorname) {
    if (!color_table_size) {
        color_table_size = AW_STD_COLOR_IDX_MAX + colnum;
        color_table      = (AW_rgb*)malloc(sizeof(AW_rgb) *color_table_size);
        for (int i = 0; i<color_table_size; ++i) color_table[i] = AW_NO_COLOR;
    }
    else {
        if (colnum>=color_table_size) {
            long new_size = colnum+8;
            color_table   = (AW_rgb*)realloc(color_table, new_size*sizeof(AW_rgb)); // valgrinders : never freed because AW_window never is freed
            for (int i = color_table_size; i<new_size; ++i) color_table[i] = AW_NO_COLOR;
            color_table_size = new_size;
        }
    }

    color_table[colnum] = get_root()->alloc_named_data_color(colorname);
    
    if (colnum == AW_DATA_BG) {
        AW_area_management* pMiddleArea = prvt->areas[AW_MIDDLE_AREA];
        if(pMiddleArea) {
            GdkColor color = get_root()->getColor(color_table[colnum]);
            gtk_widget_modify_bg(pMiddleArea->get_area(),GTK_STATE_NORMAL, &color);
            FIXME("no idea if this works");
        }
    }
    return (AW_color_idx)colnum;
}

void AW_window::create_window_variables() {
    char buffer[200];

    sprintf(buffer, "window/%s/horizontal_page_increment", window_defaults_name);
    get_root()->awar_int(buffer, 50);
    get_root()->awar(buffer)->add_callback(horizontal_scrollbar_redefinition_cb, (AW_CL)this);

    sprintf(buffer, "window/%s/vertical_page_increment", window_defaults_name);
    get_root()->awar_int(buffer, 50);
    get_root()->awar(buffer)->add_callback(vertical_scrollbar_redefinition_cb, (AW_CL)this);

    sprintf(buffer, "window/%s/scroll_width_horizontal", window_defaults_name);
    get_root()->awar_int(buffer, 9);
    get_root()->awar(buffer)->add_callback(horizontal_scrollbar_redefinition_cb, (AW_CL)this);

    sprintf(buffer, "window/%s/scroll_width_vertical", window_defaults_name);
    get_root()->awar_int(buffer, 20);
    get_root()->awar(buffer)->add_callback(vertical_scrollbar_redefinition_cb, (AW_CL)this);
}

void AW_window::create_devices() {
    if (prvt->areas[AW_INFO_AREA]) {
        prvt->areas[AW_INFO_AREA]->create_devices(this, AW_INFO_AREA);
    }
    if (prvt->areas[AW_MIDDLE_AREA]) {
        prvt->areas[AW_MIDDLE_AREA]->create_devices(this, AW_MIDDLE_AREA);
    }
    if (prvt->areas[AW_BOTTOM_AREA]) {
        prvt->areas[AW_BOTTOM_AREA]->create_devices(this, AW_BOTTOM_AREA);
    }
}


void AW_window::create_font_button(const char* /*awar_name*/, const char */*label_*/) {
  GTK_NOT_IMPLEMENTED;
}

void AW_window::create_color_button(const char* /*awar_name*/, const char */*label_*/) {
  GTK_NOT_IMPLEMENTED;
}

AW_xfig* AW_window::get_xfig_data() {
    return xfig_data;
}
