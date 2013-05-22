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
#include "aw_help.hxx"
#include "aw_varUpdateInfo.hxx" 
#include "aw_type_checking.hxx"
#include "aw_select.hxx"
#include <gtk/gtk.h>

#include <string>
#include <sstream>
#ifndef ARBDB_H
#include <arbdb.h>
#endif

void AW_POPDOWN(AW_window *window){
    window->hide();
}

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
void AW_window::label(const char *_label){ freedup(_at.label_for_inputfield, _label); }
void AW_window::button_length(int length){ _at.button_length(length); }
int AW_window::get_button_length() const { return _at.get_button_length(); }
void AW_window::get_at_position(int *x, int *y) const { _at.get_at_position(x,y); }
int AW_window::get_at_xposition() const { return _at.get_at_xposition(); }
int AW_window::get_at_yposition() const { return _at.get_at_yposition(); }
void AW_window::at(const char *at_id) { _at.at(at_id); }
void AW_window::sens_mask(AW_active mask){  _at.set_mask(mask); }
void AW_window::auto_increment(int dx, int dy) { _at.auto_increment(dx, dy); }
void AW_window::restore_at_size_and_attach(const AW_at_size *at_size){
    at_size->restore(_at);
}
void AW_window::store_at_size_and_attach(AW_at_size *at_size) {
    at_size->store(_at);
}
void AW_window::get_window_size(int& width, int& height){
    width  = _at.max_x_size;
    height = _at.max_y_size;
}

/**
 * set help text for next created button
 */
void AW_window::help_text(const char *id){
    if(NULL != _at.helptext_for_next_button)
    {
        delete _at.helptext_for_next_button;
    }
    _at.helptext_for_next_button   = strdup(id);
}

/**
 * make next created button default button
 */
void AW_window::highlight(){
    _at.highlight = true;
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
    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;
    
    AW_root *root = AW_root::SINGLETON;
     
    if (root->is_help_active()) {
        root->set_help_active(false);
        root->set_cursor(NORMAL_CURSOR);

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
        root->set_cursor(WAIT_CURSOR);
        
        cbs->run_callback();
        
        if (root->is_help_active()) {
            root->set_cursor(HELP_CURSOR);
        }
        else {
            root->set_cursor(NORMAL_CURSOR);
        }
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

/** 
 * checks if a label string is an image reference.
 * image references have "#" as their first character
 *
 * @param ref the label string
 * @return true if ref is an image reference
 */
static bool AW_IS_IMAGEREF(const char* ref) {
    return ref[0] == '#';
}

/** 
 * Converts ARB type mnemonics into GTK style mnemoics.
 * @param  text     the label text
 * @param  mnemonic the mnemonic character
 * @return copy of @param text with a _ inserted before the first
 *         occurance of @param mnemonic. MUST BE FREED.
 */
static char* aw_convert_mnemonic(const char* text, const char* mnemonic) {
    char *rval = (char*) malloc(strlen(text)
                                + 1 // \0 terminator
                                + 1 // _ character
                                + 4 // " (x)"
                                );
    aw_return_val_if_fail(rval, NULL);
    strcpy(rval, text);

    if (AW_IS_IMAGEREF(text) || !mnemonic || mnemonic[0]=='\0') {
        return rval;
    }

    size_t pos = strcspn(text, mnemonic);
    aw_return_val_if_fail(pos <= strlen(text), rval); // paranoia check

    if (pos == strlen(text)) { // mnemonic does not occur in text
        rval[pos++] = ' ';
        rval[pos++] = '(';
        rval[pos++] = '_';
        rval[pos++] = mnemonic[0];
        rval[pos++] = ')';
        rval[pos++] = '\0';
    } else {
        rval[pos]='_';
        strcpy(rval + pos + 1, text + pos);
    }

    return rval;
}

/**
 * creates a label widget from ARB label descriptions
 * @param  label_text The text of the label. Image labels begin with a #.
 * @param  label_len  A fixed label width in characters or 0.
 * @param  mnemonic   Mnemononic char to be used
 * @return An GtkImage or a GtkLabel, depending. 
 */
GtkWidget* AW_window::make_label(const char* label_text, short label_len, const char* mnemonic) {
    aw_return_val_if_fail(label_text, NULL);
    GtkWidget *widget;

    if (AW_IS_IMAGEREF(label_text)) {
        widget = gtk_image_new_from_file(GB_path_in_ARBLIB("pixmaps", label_text+1));
    }
    else {
        if (mnemonic) {
            char *label_w_mnemonic = aw_convert_mnemonic(label_text, mnemonic);        
            widget = gtk_label_new_with_mnemonic(label_w_mnemonic);
            free(label_w_mnemonic);
        } else {
            widget = gtk_label_new_with_mnemonic(label_text);
        }
    
        AW_awar *awar = get_root()->label_is_awar(label_text);
        if (awar) { 
            awar->bind_value(G_OBJECT(widget), "label");
        }

        if (label_len) {
            gtk_label_set_width_chars(GTK_LABEL(widget), label_len);
        }
    }
    
    return widget;
}

void AW_window::update_label(GtkWidget* widget, const char* newlabel) {
    g_object_set(G_OBJECT(widget), "label", newlabel, NULL);
}


/**
 * Places @param *widget according to _at in prvt->fixedArea
 * - creates label if defined in _at (using hbox)
 **/
void AW_window::put_with_label(GtkWidget *widget) {
    #define SPACE_BEHIND 5
    #define SPACE_BELOW 5
    GtkWidget *hbox = 0, *wlabel = 0;

    // create label from label text
    if (_at.label_for_inputfield) {
        wlabel = make_label(_at.label_for_inputfield, _at.length_of_label_for_inputfield);
    }
    
    // pack widget and label into hbox
    // (having/not having the hbox changes appearance!)
    hbox = gtk_hbox_new(false,0);
    if (wlabel) {
        // label does not expand (hence "false,false")
        gtk_box_pack_start(GTK_BOX(hbox), wlabel, false, false, 0);
        gtk_misc_set_alignment(GTK_MISC(wlabel),1.0f, 1.0f); //center label in hbox
    }
    gtk_box_pack_start(GTK_BOX(hbox), widget, true, true, 0);

    GtkRequisition hbox_req;
    gtk_widget_show_all(hbox);
    gtk_widget_size_request(GTK_WIDGET(hbox), &hbox_req);

    // if size given by xfig, scale hbox
    if (_at.to_position_exists) { 
        aw_warn_if_fail(_at.to_position_x >_at.x_for_next_button);               
        aw_warn_if_fail(_at.to_position_y >_at.y_for_next_button);         
        hbox_req.width = std::max(hbox_req.width, _at.to_position_x - _at.x_for_next_button);
        hbox_req.height = std::max(hbox_req.height, _at.to_position_y - _at.y_for_next_button);
        
        gtk_widget_set_size_request(hbox, hbox_req.width, hbox_req.height);
    }


    aw_at_layout_put(prvt->fixed_size_area, hbox, &_at);

    _at.increment_at_commands(hbox_req.width * (2-_at.correct_for_at_center) / 2. + SPACE_BEHIND, 
                                  hbox_req.height + SPACE_BELOW);
    unset_at_commands();
    prvt->last_widget = hbox;
}


/** Creates a progress bar that is bound to the specified awar.*/
void AW_window::create_progressBar(const char *var_name) {
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_if_fail(awar != NULL);

    GtkWidget *bar = gtk_progress_bar_new();
    gtk_widget_show(bar);
    put_with_label(bar);
    
    awar->bind_value(G_OBJECT(bar), "fraction");
}

/**
 * Create a button or text display.
 * If a callback was given via at->callback(), creates a button;
 * otherwise creates a label.
 */ 
void AW_window::create_button(const char *macro_name, const char *button_text, 
                              const char *mnemonic, const char */*color*/) {
#if defined(DUMP_BUTTON_CREATION)
    printf("------------------------------ Button '%s'\n", button_text);
    printf("x_for_next_button=%i y_for_next_button=%i\n", _at.x_for_next_button, _at.y_for_next_button);
#endif // DUMP_BUTTON_CREATION

    GtkWidget *button_label, *button;
    button_label = make_label(button_text, _at.length_of_buttons, mnemonic);

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
        
        get_root()->register_widget(button, _at.widget_mask);
    } 
    else {
        button = button_label;
#if defined(DEVEL_RALF) && 1
        aw_assert(!macro_name); // please pass NULL for buttons w/o callback
#endif
    }

    bool do_highlight = _at.highlight;
    put_with_label(button);

    if (do_highlight) {
        gtk_widget_set_can_default(button, true);
        gtk_widget_grab_default(button);
        gtk_widget_grab_focus(button);
    }
}

void AW_window::create_autosize_button(const char *macro_name, const char *buttonlabel, 
                                       const char *mnemonic /* = 0*/, unsigned xtraSpace /* = 1 */){
    aw_return_if_fail(buttonlabel != NULL);
    aw_return_if_fail(!AW_IS_IMAGEREF(buttonlabel)); 
    aw_return_if_fail(!_at.to_position_exists); // wont work if to-position exists

    const char* content = buttonlabel;

    if (get_root()->label_is_awar(buttonlabel)) {
        content = get_root()->awar(buttonlabel)->read_char_pntr();

        // awar must be filled before calling create_autosize_button,
        // otherwise size cannot be detected 
        aw_return_if_fail(strlen(content)>0);
    }

    size_t width = 0, height = 0;
    std::stringstream labelLines;
    labelLines << buttonlabel;
    std::string line;
    while(std::getline(labelLines, line))
    {
        if (line.size() > width) width = line.size();
        height++;
    }

    short length_of_buttons = _at.length_of_buttons;
    short height_of_buttons = _at.height_of_buttons;

    _at.length_of_buttons = width+(xtraSpace*2) + 1 ;
    _at.height_of_buttons = height;
    create_button(macro_name, buttonlabel, mnemonic);

    _at.length_of_buttons = length_of_buttons;
    _at.height_of_buttons = height_of_buttons;
}

GtkWidget* AW_window::get_last_widget() const{
    return prvt->last_widget;
}

extern "C" gboolean AW_switch_widget_child(GtkWidget *bin, gpointer other_bin) {
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(bin));
    GtkWidget *other_child = gtk_bin_get_child(GTK_BIN(other_bin));
    g_object_ref(child);
    g_object_ref(other_child);
    gtk_container_remove(GTK_CONTAINER(bin), child);
    gtk_container_remove(GTK_CONTAINER(other_bin), other_child);
    gtk_container_add(GTK_CONTAINER(bin), GTK_WIDGET(other_child));
    gtk_container_add(GTK_CONTAINER(other_bin), GTK_WIDGET(child));

    g_object_unref(child);
    g_object_unref(other_child);
    return false; // event not consumed
}

struct _awar_inverse_bool_mapper : public AW_awar_gvalue_mapper {
    bool operator()(GValue* gval, AW_awar* awar) OVERRIDE {
        awar->write_as_bool(!g_value_get_boolean(gval), true);
        return true;
    }
    bool operator()(AW_awar* awar, GValue* gval) OVERRIDE {
        g_value_set_boolean(gval, !awar->read_as_bool());
        return true;
    }
};

/**should be used if the awar is of type int but gtk needs float to work properly e.g. for spin boxes or sliders*/
struct _awar_float_to_int_mapper : public AW_awar_gvalue_mapper {
    bool operator()(GValue* gval, AW_awar* awar) OVERRIDE {
        int value = -1;
        
        if(G_VALUE_HOLDS_FLOAT(gval)){
            value = (int)g_value_get_float(gval);
        }
        else if(G_VALUE_HOLDS_DOUBLE(gval)) {
            value = (int)g_value_get_double(gval);
        }
        else {
            aw_return_val_if_reached(false);
        }
        awar->write_int(value, true);
        return true;
    }
    bool operator()(AW_awar* awar, GValue* gval) OVERRIDE {
        if(G_VALUE_HOLDS_FLOAT(gval)) {
            g_value_set_float(gval, awar->read_as_float());
        }
        else if(G_VALUE_HOLDS_DOUBLE(gval)) {
            g_value_set_double(gval, awar->read_as_float());
        }
        else {
            aw_return_val_if_reached(false);
        }
        return true;
    }
};


void AW_window::create_toggle(const char *var_name, bool inverse, 
                              const char *yes, const char *no, int width) {
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_if_fail(awar != NULL);
    aw_return_if_fail((yes==NULL) == (no==NULL)); // either both or none

    GtkWidget* checkButton;
    if (yes) {
        // create our own switching labels toggle
        
        checkButton = gtk_toggle_button_new();
        GtkWidget* no_label = make_label(no, width);
        gtk_container_add(GTK_CONTAINER(checkButton), no_label);
        
        GtkWidget* bin = gtk_toggle_button_new();
        GtkWidget* other_label = make_label(yes, width);
        gtk_container_add(GTK_CONTAINER(bin), other_label);

        gtk_widget_show_all(bin);

        g_signal_connect(G_OBJECT(checkButton), "released", 
                         G_CALLBACK(AW_switch_widget_child),
                         (gpointer) bin);
    }
    else {
        // just use a normal check button
        checkButton = gtk_check_button_new();
    }
    
    awar->bind_value(G_OBJECT(checkButton), "active",
                     inverse ? new _awar_inverse_bool_mapper() : NULL);

    if (prvt->callback) {
      _set_activate_callback(checkButton);
    }
    
    put_with_label(checkButton);   
}

static gboolean noop_signal_handler(GtkWidget* /*wgt*/, gpointer /*user_data*/) {
    return true; // stop signal
}

void AW_window::create_input_field(const char *var_name,   int columns) {
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_if_fail(awar != NULL);
    
    GtkWidget *entry;

    int width = columns ? columns : _at.length_of_buttons;

    if (awar->get_type() == GB_STRING) {
        entry = gtk_entry_new();
        awar->bind_value(G_OBJECT(entry), "text");
    }
    else {
        GtkAdjustment *adj;

        int climb_rate = 1;
        int precision;
        double step;
        if        (awar->get_type() == GB_FLOAT) {
            precision = 2;
            step      = 0.1;
        } else if (awar->get_type() == GB_INT) {
            precision = 0;
            step      = 1;
        } else {
            aw_return_if_reached();
        }

        adj = GTK_ADJUSTMENT(gtk_adjustment_new(awar->read_as_float(), 
                                                awar->get_min(), awar->get_max(), 
                                                step, 50 *step, 0));
        entry = gtk_spin_button_new(adj, climb_rate, precision);
        if(awar->get_type() == GB_INT)
        {//spin button value is always float, if we want to connect it to an int we need to use a wrapper
            awar->bind_value(G_OBJECT(entry), "value", new _awar_float_to_int_mapper());
        }
        else
        {
            awar->bind_value(G_OBJECT(entry), "value");
        }
     
        width--; // make room for the spinner
    }

    if (width > 0) {
        gtk_entry_set_width_chars(GTK_ENTRY(entry), width);
    }

    gtk_entry_set_activates_default(GTK_ENTRY(entry), true);
    put_with_label(entry);
    get_root()->register_widget(entry, _at.widget_mask);
}


void AW_window::create_text_field(const char *var_name, int columns /* = 20 */, int rows /*= 4*/){
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_if_fail(awar != NULL);

    GtkWidget *entry = gtk_text_view_new();
    GtkTextBuffer *textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(entry));
    awar->bind_value(G_OBJECT(textbuffer), "text");

    GtkWidget *scrolled_entry = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_entry), 
                                   GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_entry), entry);

    int char_width, char_height;
    prvt->get_font_size(char_width, char_height);
    gtk_widget_set_size_request(scrolled_entry, char_width * columns, char_height * rows);

    
    // callback for enter
    //g_signal_connect(G_OBJECT(entry), "activate",
    //                 G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback),
    //                 (gpointer) vui);
    
    if (prvt->d_callback) {
        g_signal_connect(G_OBJECT(entry), "activate",
                         G_CALLBACK(AW_window::click_handler),
                         (gpointer) prvt->d_callback);
        prvt->d_callback->id = GBS_global_string_copy("INPUT:%s", var_name);
        get_root()->define_remote_command(prvt->d_callback);
    }
    put_with_label(scrolled_entry);
    get_root()->register_widget(entry, _at.widget_mask);
}

void AW_window::create_menu(const char *name, const char *mnemonic, AW_active mask /*= AWM_ALL*/){
    aw_return_if_fail(legal_mask(mask));

    // The user might leave sub menus open. Close them before creating a new top level menu.
    while(prvt->menus.size() > 1) {
        close_sub_menu();
    }
    
    insert_sub_menu(name, mnemonic, mask);
}

void AW_window::close_sub_menu(){
    aw_return_if_fail(prvt->menus.size() > 1);
    
    prvt->menus.pop();
}

AW_selection_list *AW_window::create_option_menu(const char *var_name, 
                                                     const char *tmp_label, 
                                                     const char *){
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_val_if_fail(awar, NULL);

    if (tmp_label) this->label(tmp_label);

    GtkWidget *combo_box = gtk_combo_box_new();
    AW_selection_list *slist = new AW_selection_list(awar);
    slist->bind_widget(combo_box);

    //prvt->callback

    get_root()->register_widget(combo_box, _at.widget_mask);

    prvt->combo_box = combo_box;
    prvt->selection_list = slist;
    return slist;
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
void AW_window::insert_option_internal(const char *option_name, const char */*mnemonic*/, 
                                       T var_value, const char */*name_of_color*/, 
                                       bool /*default_option*/) {
    aw_return_if_fail(prvt->selection_list != NULL); //current option menu has to be set
    //aw_return_if_fail(prvt->selection_list->variable_type == AW_TypeCheck::getVarType(var_value)); //type missmatch
    
    FIXME("background color not implemented");
    FIXME("check for distinct per-option callbacks");
    FIXME("setting sensitivity of option menu entries not implemented");

    prvt->selection_list->insert(option_name, var_value);
}

void AW_window::update_option_menu() {
    prvt->selection_list->update();
    put_with_label(prvt->combo_box);
}

void AW_window::clear_option_menu(AW_selection_list *sel) {
    sel->clear(true);
}


static void row_activated_cb(GtkTreeView       * /*tree_view*/,
                             GtkTreePath       * /*path*/,
                             GtkTreeViewColumn * /*column*/,
                             gpointer           user_data) {
    AW_cb_struct *cbs = (AW_cb_struct *) user_data;
    cbs->run_callback();
}

AW_selection_list* AW_window::create_selection_list(const char *var_name, int columns, int rows) {
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_val_if_fail(awar, NULL);
    aw_warn_if_fail(!_at.label_for_inputfield); // labels have no effect for selection lists

   
    GtkWidget *tree = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
   
    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_win), tree);

    // set size
    int char_width, char_height;
    prvt->get_font_size(char_width, char_height);
    gtk_widget_set_size_request(scrolled_win, char_width * columns, char_height * rows);

    AW_selection_list *slist = new AW_selection_list(awar);
    slist->bind_widget(tree);

    if (prvt->d_callback) {
        g_signal_connect(G_OBJECT(tree), "row-activated", 
                         G_CALLBACK(row_activated_cb),
                         (gpointer) prvt->d_callback);
    }

    get_root()->register_widget(tree, _at.widget_mask);
    put_with_label(scrolled_win);
    return slist;
}

// BEGIN TOGGLE FIELD STUFF

void AW_window::create_toggle_field(const char *var_name, int orientation /*= 0*/){
    // orientation = 0 -> vertical else horizontal layout

    if (orientation == 0) {
      prvt->toggle_field = gtk_vbox_new(true, 2);
    } 
    else {
      prvt->toggle_field = gtk_hbox_new(true, 2);
    }
    
    prvt->toggle_field_awar_name = var_name;

    FIXME("bind awar  to widget");
    
    get_root()->register_widget(prvt->toggle_field, _at.widget_mask);
}

void AW_window::create_toggle_field(const char *var_name, const char *labeli, const char *mnemonic) {
    char *lab = aw_convert_mnemonic(labeli, mnemonic);
    if (labeli) this->label(lab);
    free(lab);
    create_toggle_field(var_name);
}

template <class T>
void AW_window::insert_toggle_internal(const char *toggle_label, const char *mnemonic, 
                                       T var_value, bool default_toggle) {
    AW_awar* awar = get_root()->awar_no_error(prvt->toggle_field_awar_name);
    aw_return_if_fail(awar != NULL);

    prvt->radio_last = GTK_RADIO_BUTTON(gtk_radio_button_new_from_widget(prvt->radio_last));

    gtk_container_add(GTK_CONTAINER(prvt->radio_last),
                      make_label(toggle_label, 0, mnemonic));
    
    gtk_box_pack_start(GTK_BOX(prvt->toggle_field), 
                       GTK_WIDGET(prvt->radio_last), true, true, 2);
    

    AW_varUpdateInfo *vui = new AW_varUpdateInfo(this, NULL, AW_WIDGET_TOGGLE_FIELD,
                                                 awar, var_value, prvt->callback);
    g_signal_connect((gpointer)prvt->radio_last, "clicked", 
                     G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback),
                     (gpointer)vui);
    
    if(default_toggle){
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prvt->radio_last), true);
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
// END TOGGLE FIELD STUFF

void AW_window::draw_line(int x1, int y1, int x2, int y2, int width, bool resize){
    aw_return_if_fail(xfig_data != NULL);  // forgot to call load_xfig ?
    
    xfig_data->add_line(x1, y1, x2, y2, width);
    _at.set_xfig(xfig_data);

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
        set_window_size(_at.max_x_size+1000, _at.max_y_size+1000);
    }
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

AW_device_print *AW_window::get_print_device(AW_area area){
    AW_area_management *aram = prvt->areas[area];
    aw_return_val_if_fail(aram, NULL);
    return aram->get_print_device();
}

AW_device_size *AW_window::get_size_device(AW_area area){
    AW_area_management *aram        = prvt->areas[area];
    aw_return_val_if_fail(aram, NULL);

    AW_device_size *size_device = aram->get_size_device();
    aw_return_val_if_fail(size_device, NULL); //paranoia

    size_device->forget_tracked();
    size_device->reset(); // @@@ hm

    return size_device;
}


void AW_window::insert_help_topic(const char *labeli, 
                                  const char *mnemonic, const char *helpText,
                                  AW_active mask, AW_CB f, AW_CL cd1, AW_CL cd2) {
    aw_return_if_fail(prvt->help_menu != NULL);
    
    prvt->menus.push(prvt->help_menu);
    insert_menu_topic("", labeli, mnemonic, helpText, mask, f, cd1, cd2);
    prvt->menus.pop();
}


void AW_window::insert_menu_topic(const char *cmd, const char *labeli, 
                                  const char *mnemonic, const char *helpText, 
                                  AW_active mask, AW_CB f, AW_CL cd1, AW_CL cd2){
    aw_return_if_fail(legal_mask(mask));
    aw_return_if_fail(prvt->menus.size() > 0); //closed too many menus
    aw_return_if_fail(cmd != NULL);
  
    GtkWidget *wlabel    = make_label(labeli, 0, mnemonic);
    GtkWidget *alignment = gtk_alignment_new(0.f, 0.5f, 0.f, 0.f);
    GtkWidget *item      = gtk_menu_item_new();
    gtk_container_add(GTK_CONTAINER(alignment), wlabel);
    gtk_container_add(GTK_CONTAINER(item),      alignment);

    gtk_menu_shell_append(prvt->menus.top(), item);  
   
    AW_cb_struct *cbs = new AW_cb_struct(this, f, cd1, cd2, helpText);
    
    g_signal_connect((gpointer)item, "activate", G_CALLBACK(AW_window::click_handler), (gpointer)cbs);

    cbs->id = strdup(cmd);
    get_root()->define_remote_command(cbs);
    get_root()->register_widget(item, mask);
}

void AW_window::insert_sub_menu(const char *labeli, const char *mnemonic, AW_active mask /*= AWM_ALL*/){
    aw_return_if_fail(legal_mask(mask));
    aw_return_if_fail(prvt->menus.top());
  
  
    //create new menu item with attached submenu
    GtkWidget   *wlabel    = make_label(labeli, 0, mnemonic);
    GtkWidget   *submenu   = gtk_menu_new();
    GtkMenuItem *item      = GTK_MENU_ITEM(gtk_menu_item_new());
    GtkWidget   *alignment = gtk_alignment_new(0.f, 0.5f, 0.f, 0.f);

    gtk_container_add(GTK_CONTAINER(alignment), wlabel);
    gtk_container_add(GTK_CONTAINER(item), alignment);
    gtk_menu_item_set_submenu(item, submenu);

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

    // 0.8 is _exactly_ right, +/- 0.01 already looks off
    width *= 0.8; height *=0.8;

    if (file)   xfig_data = new AW_xfig(file, width, height);
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

    set_expose_callback(AW_INFO_AREA, (AW_CB)AW_xfigCB_info_area, (AW_CL)xfig_data);
}

/**
 * Construct window-local id. 
 * Prefixes @param id with @member window_defaults_name + "/"
 */
const char *AW_window::local_id(const char *id) const{
    return GBS_global_string("%s/%s", window_defaults_name, id);
}

void AW_window::sep______________() {
    aw_return_if_fail(prvt->menus.size() > 0);
    
    GtkWidget *item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(prvt->menus.top(), item);
}

/**
 * Registers an expose callback.
 * Called whenever the @param area receives an expose event.
 * This is where any drawing should be handled.
 */
void AW_window::set_expose_callback(AW_area area, AW_CB f, AW_CL cd1 /*= 0*/, AW_CL cd2 /*= 0*/) {
    AW_area_management *aram = prvt->areas[area];
    if (aram) aram->set_expose_callback(this, f, cd1, cd2);
}

/**
 * Registers a focus callback.
 * They used to be called as soon as the mouse entered the main drawing area. 
 * For now, they are not called at all.
 */
void AW_window::set_focus_callback(AW_CB f, AW_CL cd1, AW_CL cd2) {
    // this is only ever called by AWT_canvas and triggers a re-render of the
    // middle area. the API was designed for "focus-follows-mouse" mode,
    // and makes little sense for GTK
    // (we might need other means to trigger an update of the tree at the right time)
    prvt->focus_cb = new AW_cb_struct(this, f, cd1, cd2, 0, prvt->focus_cb);
}

/**
 * Registers callback to be executed after the window is shown.
 * Called multiple times if a show() follows a hide().
 */
void AW_window::set_popup_callback(AW_CB f, AW_CL cd1, AW_CL cd2) {
    prvt->popup_cb = new AW_cb_struct(this, f, cd1, cd2, 0, prvt->popup_cb);
}

void AW_window::set_info_area_height(int h) {
    aw_return_if_fail(prvt->fixed_size_area != NULL);
    gtk_widget_set_size_request(GTK_WIDGET(prvt->fixed_size_area), -1, h);
}

void AW_window::set_bottom_area_height(int h) {
    aw_return_if_fail(prvt->bottom_area != NULL);
    
    // FIXME: workaround for @@@PH_main.cxx
    // Every client besides PHYLO calls with h=0;
    // PHYLO however sets a fixed height. So we have to scale according
    // to font height (which PHYLO assumes to be 13)
    int char_width, char_height;
    prvt->get_font_size(char_width, char_height);
    h *= (double)char_height/13;

    gtk_widget_set_size_request(GTK_WIDGET(prvt->bottom_area), -1, h);
}

void AW_window::set_input_callback(AW_area area, AW_CB f, AW_CL cd1, AW_CL cd2) {
    AW_area_management *aram = prvt->areas[area];
    aw_return_if_fail(aram != NULL);
    aram->set_input_callback(this, f, cd1, cd2);
}

void AW_window::set_motion_callback(AW_area area, AW_CB f, AW_CL cd1, AW_CL cd2) {
    AW_area_management *aram = prvt->areas[area];
    aw_return_if_fail(aram != NULL);
    aram->set_motion_callback(this, f, cd1, cd2);
}

void AW_window::set_resize_callback(AW_area area, AW_CB f, AW_CL cd1 /*= 0*/, AW_CL cd2 /*= 0*/) {
    AW_area_management *aram = prvt->areas[area];
    aw_return_if_fail(aram != NULL);
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
    aw_return_if_fail(aram != NULL);
    *square = aram->get_common()->get_screen();
}

void AW_window::get_scrollarea_size(AW_screen_area *square) {
    aw_return_if_fail(square != NULL);
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
 
static void _aw_window_recalc_scrollbar_cb(AW_root*, AW_window* aww) {
    aww->calculate_scrollbars();
}

void AW_window::create_window_variables() {
    char buffer[200];

    sprintf(buffer, "window/%s/horizontal_page_increment", window_defaults_name);
    get_root()->awar_int(buffer, 50);
    get_root()->awar(buffer)->add_callback((AW_RCB1)_aw_window_recalc_scrollbar_cb, (AW_CL)this);

    sprintf(buffer, "window/%s/vertical_page_increment", window_defaults_name);
    get_root()->awar_int(buffer, 50);
    get_root()->awar(buffer)->add_callback((AW_RCB1)_aw_window_recalc_scrollbar_cb, (AW_CL)this);

    sprintf(buffer, "window/%s/scroll_width_horizontal", window_defaults_name);
    get_root()->awar_int(buffer, 9);
    get_root()->awar(buffer)->add_callback((AW_RCB1)_aw_window_recalc_scrollbar_cb, (AW_CL)this);

    sprintf(buffer, "window/%s/scroll_width_vertical", window_defaults_name);
    get_root()->awar_int(buffer, 20);
    get_root()->awar(buffer)->add_callback((AW_RCB1)_aw_window_recalc_scrollbar_cb, (AW_CL)this);
}

static void value_changed_scroll_bar_vertical(GtkAdjustment *adjustment, AW_cb_struct *cbs) {
    cbs->aw->slider_pos_vertical = gtk_adjustment_get_value(adjustment);
    cbs->run_callback();
}

static void value_changed_scroll_bar_horizontal(GtkAdjustment *adjustment, AW_cb_struct *cbs) {
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

void AW_window::set_vertical_change_callback(AW_CB f, AW_CL cd1, AW_CL cd2) {
    aw_return_if_fail(prvt->drawing_area != NULL);
    
    GtkAdjustment *hAdj = aw_drawing_area_get_vertical_adjustment(prvt->drawing_area);
    
    g_signal_connect((gpointer)hAdj, "value_changed",
                     G_CALLBACK(value_changed_scroll_bar_vertical),
                     (gpointer)new AW_cb_struct(this, f, cd1, cd2, ""));

}

void AW_window::set_horizontal_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    aw_return_if_fail(prvt->drawing_area != NULL);
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

const char* AW_window::get_window_title() const {
    return window_name;
}

void AW_window::shadow_width (int /*shadow_thickness*/) {
    // won't implement
}

void AW_window::button_height(int height) {
    _at.height_of_buttons = height>1 ? height : 0; 
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

void AW_window::window_fit() {
    // let gtk do the sizing based on content
    // (default size will be ignored if requisition of 
    //  children is greater)
    set_window_size(0,0);
}

AW_window::AW_window() 
  : recalc_size_at_show(AW_KEEP_SIZE),
    recalc_pos_at_show(AW_KEEP_POS),
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
    slider_pos_vertical(0),
    slider_pos_horizontal(0),
    main_drag_gc(0),
    picture(new AW_screen_area)
{
    aw_assert(AW_root::SINGLETON); // must have AW_root
  
    reset_scrolled_picture_size();
    
    prvt = new AW_window::AW_window_gtk();

    // AW Windows are never destroyed
    set_hide_on_close(true);
}

AW_window::~AW_window() {
    delete picture;
    delete prvt;
}

void AW_window::init_window(const char *window_name_, const char* window_title,
                            int width, int height, bool resizable) {
    // clean unwanted chars from window_name 
    window_defaults_name = GBS_string_2_key(window_name_);

    // get (and create if necessary) x,y,w,h awars for window
    std::string prop_root = std::string("window/windows/") + window_defaults_name;
    awar_posx   = get_root()->awar_int((prop_root + "/posx").c_str(), 0);
    awar_posy   = get_root()->awar_int((prop_root + "/posy").c_str(), 0);
    awar_width  = get_root()->awar_int((prop_root + "/height").c_str(), width);
    awar_height = get_root()->awar_int((prop_root + "/width").c_str(), height);
        
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
    prvt->areas[AW_INFO_AREA] = new AW_area_management(GTK_WIDGET(prvt->fixed_size_area), this);
}

void AW_window::recalc_pos_atShow(AW_PosRecalc pr){
    recalc_pos_at_show = pr;
}

AW_PosRecalc AW_window::get_recalc_pos_atShow() const {
    return recalc_pos_at_show;
}

void AW_window::recalc_size_atShow(enum AW_SizeRecalc sr) {
    if (sr == AW_RESIZE_ANY) {
        sr = (recalc_size_at_show == AW_RESIZE_USER) ? AW_RESIZE_USER : AW_RESIZE_DEFAULT;
    }
    recalc_size_at_show = sr;
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
            // GdkColor color = get_root()->getColor(color_table[colnum]);
            // gtk_widget_modify_bg(pMiddleArea->get_area(),GTK_STATE_NORMAL, &color);
            pMiddleArea->get_common()->set_bg_color(color_table[colnum]);
        }
    }
    return (AW_color_idx)colnum;
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


void AW_window::create_font_button(const char* var_name, const char *label_) {
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_if_fail(awar != NULL);
    
    GtkWidget *font_button = gtk_font_button_new();
    awar->bind_value(G_OBJECT(font_button), "font-name");

    if (label_) label(label_);
    put_with_label(font_button);
}

struct _awar_gdkcolor_mapper : public AW_awar_gvalue_mapper {
    bool operator()(GValue* gval, AW_awar* awar) OVERRIDE {
        GdkColor* col = (GdkColor*)g_value_get_boxed(gval);
        char* str = gdk_color_to_string(col);
        awar->write_string(str);
        free(str);
        return true;
    }
    bool operator()(AW_awar* awar, GValue* gval) OVERRIDE {
        GdkColor col;
        if (!gdk_color_parse(awar->read_char_pntr(), &col))
            return false;
        g_value_set_boxed(gval, &col);
        return true;
    }
};

void AW_window::create_color_button(const char* var_name, const char *label_) {
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_if_fail(awar != NULL);

    GtkWidget *color_button = gtk_color_button_new();
    awar->bind_value(G_OBJECT(color_button), 
                     "color", new _awar_gdkcolor_mapper());

    if (label_) label(label_);
    put_with_label(color_button);
}

AW_xfig* AW_window::get_xfig_data() {
    return xfig_data;
}


/********* window show/hide/delete ***********/

void AW_window::allow_delete_window(bool allow_close) {
    gtk_window_set_deletable(prvt->window, allow_close);
    // the window manager might still show the close button
    // => do nothing if clicked
    g_signal_connect(prvt->window, "delete-event", G_CALLBACK(noop_signal_handler), NULL);
}

bool AW_window::is_shown() const{
    return gtk_widget_get_visible(GTK_WIDGET(prvt->window));
}

void AW_window::show() {
    if (is_shown()) return; 

    switch (recalc_pos_at_show) {
    case AW_KEEP_POS:
        break;
    case AW_REPOS_TO_CENTER:
        gtk_window_set_position(prvt->window, GTK_WIN_POS_CENTER);
        break;
    case AW_REPOS_TO_MOUSE_ONCE:
        recalc_pos_at_show = AW_KEEP_POS;
        //fall though
    case AW_REPOS_TO_MOUSE:
        gtk_window_set_position(prvt->window, GTK_WIN_POS_MOUSE);
        break;
    default:
        aw_warn_if_reached();
    }

    prvt->show();
    get_root()->window_show(); // increment window counter

    if (prvt->popup_cb) prvt->popup_cb->run_callback();
}

void AW_window::hide(){
    if (!is_shown()) return;
    
    gtk_widget_hide(GTK_WIDGET(prvt->window));
    get_root()->window_hide(this);  // decrement window counter
}

void AW_window::hide_or_notify(const char *error){
    if (error) aw_message(error);
    else hide();
}

// make_visible, pop window to front and give it the focus
void AW_window::activate() {
    show();
    gtk_window_present(prvt->window);
}

void AW_window::on_hide(AW_CB0 f) {
    g_signal_connect_swapped(prvt->window, "hide", G_CALLBACK(f), this);
}

void AW_window::set_hide_on_close(bool value) {
    if(value) {
        if(-1 == prvt->delete_event_handler_id) { //if no signal handler is connected
            prvt->delete_event_handler_id = g_signal_connect (prvt->window, "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
        }
    }
    else
    {
        if(-1 != prvt->delete_event_handler_id) {
            g_signal_handler_disconnect(prvt->window, prvt->delete_event_handler_id);
            prvt->delete_event_handler_id = -1;
        }
    }
}
