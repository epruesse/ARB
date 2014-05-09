// ============================================================= //
//                                                               //
//   File      : AW_window.cxx                                   //
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
#include "aw_msg.hxx"
#include "aw_awar.hxx"
#include "aw_common.hxx"
#include "aw_help.hxx"
#include "aw_type_checking.hxx"
#include "aw_select.hxx"
#include "aw_choice.hxx"
#include <gtk/gtk.h>

#include <string>
#include <sstream>
// #include <arbdb.h>
#include <arb_str.h>
#include <arb_strarray.h>

void AW_POPDOWN(AW_window *window){
    window->hide();
}

/**
 * CB wrapper for create_*_window calls to ensure that a window
 * is only created once.
 */

#if defined(DEBUG)
#define NOWINWARN() fputs("window factory did not create a window\n", stderr)
#else
#define NOWINWARN() 
#endif

static AW_window *find_or_createAndRegisterWindow(CreateWindowCallback *windowMaker) {
    typedef std::map<CreateWindowCallback, AW_window*> window_map;

    static window_map     window;  // previously popped up windows
    CreateWindowCallback& maker = *windowMaker;

    if (window.find(maker) == window.end()) {
        AW_window *made = maker(AW_root::SINGLETON);
        if (!made) { NOWINWARN(); return NULL; }

        window[maker] = made;
    }
    return window[maker];
}

void AW_window::popper(AW_window *, CreateWindowCallback *windowMaker) {
    AW_window *toPopup = find_or_createAndRegisterWindow(windowMaker);
    if (toPopup) toPopup->activate();
}
void AW_window::replacer(AW_window *caller, CreateWindowCallback *windowMaker) {
    AW_window *toPopup = find_or_createAndRegisterWindow(windowMaker);
    if (toPopup) {
        toPopup->activate();
        caller->hide();
    }
}

void AW_POPUP(AW_window */*window*/, AW_CL callback, AW_CL callback_data) { // @@@ obsolete (when #432 is done)
    typedef AW_window* (*popup_cb_t)(AW_root*, AW_CL);
    typedef std::map<std::pair<popup_cb_t,AW_CL>, AW_window*> window_map;

    static window_map windows; // previously popped up windows

    std::pair<popup_cb_t, AW_CL> popup((popup_cb_t)callback, callback_data);

    if (windows.find(popup) == windows.end()) {
        AW_window *made = popup.first(AW_root::SINGLETON, popup.second);
        if (!made) { NOWINWARN(); return; }

        windows[popup] = made;
    }

    windows[popup]->activate();
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
void AW_window::auto_increment(int dx, int dy) { _at.auto_increment(dx, dy); }
void AW_window::restore_at_size_and_attach(const AW_at_size *at_size){
    at_size->restore(_at);
}
void AW_window::store_at_size_and_attach(AW_at_size *at_size) {
    at_size->store(_at);
}

// ----------------------------------------------------------------------
// force-diff-sync 1927391236 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::get_window_size(int& width, int& height){
    width  = _at.max_x_size;
    height = _at.max_y_size;
}

/**
 * set help text for next created button
 */
void AW_window::help_text(const char *id){
    prvt->action_template.set_help(id);
}

/**
 * make next created button default button
 */
void AW_window::highlight(){
    _at.highlight = true;
}

/** 
 * Set up sensitivity mask for next widget (action)
 */
void AW_window::sens_mask(AW_active mask){
    prvt->action_template.set_active_mask(mask);
}

/**
 * Register callback for the next action implicitly created 
 * when making a widget.
 */
void AW_window::callback(const WindowCallback& wcb){
    prvt->action_template.clicked.connect(wcb, this);
}

/**
 * Register double click callback for the next action implicitly created 
 * when making a widget.
 */
void AW_window::d_callback(const WindowCallback& wcb){
    prvt->action_template.dclicked.connect(wcb, this);
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

    bool text_contains_mnemonic = (pos != strlen(text));
#if defined(DEVEL_RALF)
    aw_warn_if_fail(text_contains_mnemonic);
#endif
    if (text_contains_mnemonic) {
        rval[pos]='_';
        strcpy(rval + pos + 1, text + pos);
    } else {
        rval[pos++] = ' ';
        rval[pos++] = '(';
        rval[pos++] = '_';
        rval[pos++] = mnemonic[0];
        rval[pos++] = ')';
        rval[pos++] = '\0';
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
        AW_awar *awar = get_root()->label_is_awar(label_text);
        if (awar) { 
            widget = gtk_label_new("");
            awar->bind_value(G_OBJECT(widget), "label");
        }
        else if (mnemonic) {
            char *label_w_mnemonic = aw_convert_mnemonic(label_text, mnemonic);        
            widget = gtk_label_new_with_mnemonic(label_w_mnemonic);
            free(label_w_mnemonic);
        } else {
            widget = gtk_label_new_with_mnemonic(label_text);
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


void AW_window::put_with_label(GtkWidget* widget) {
    //label will not absorb any free space and will be centered
    GtkAlignment *align = GTK_ALIGNMENT(gtk_alignment_new(0.5f, 0.5f, 0.0f, 0.0f));
    put_with_label(widget, align);
}

void AW_window::put_with_label(GtkWidget *widget, GtkAlignment* label_alignment) {
    aw_return_if_fail(widget);
    aw_return_if_fail(label_alignment);
    
    #define SPACE_BEHIND 5
    #define SPACE_BELOW 5
    GtkWidget *hbox = 0, *wlabel = 0;
    // create label from label text
    if (_at.label_for_inputfield) {
        wlabel = make_label(_at.label_for_inputfield, _at.length_of_label_for_inputfield);
    }
    
    // (having/not having the hbox changes appearance!)
    hbox = gtk_hbox_new(false,0);
    if (_at.at_id) gtk_widget_set_name(hbox, _at.at_id);

    if (wlabel) {
        // pack label into alignment and alignment into hbox
        gtk_container_add(GTK_CONTAINER(label_alignment), wlabel);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(label_alignment), true, true, 0);
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


    aw_drawing_area_put(prvt->fixed_size_area, hbox, &_at);

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

bool AW_window::close_window_handler(GtkWidget* wgt, GdkEvent*, gpointer data) {
    aw_return_val_if_fail(data, false);
    AW_window *w = (AW_window*)data;

    if (w->prvt->close_action) {
        w->prvt->close_action->user_clicked(wgt);
    }
 
    /* If you return FALSE in the "delete-event" signal handler,
     * GTK will emit the "destroy" signal. Returning TRUE means
     * you don't want the window to be destroyed.
     * This is useful for popping up 'are you sure you want to quit?'
     * type dialogs. */
    
    if(w->prvt->hide_on_close) {
        w->hide();
    }
    return true;
}

/**
 * Create a label
 */
void AW_window::create_label(const char* button_text, bool highlight_) {
    aw_return_if_fail(button_text != NULL);

    GtkWidget *labelw = make_label(button_text, _at.length_of_buttons);

    if (highlight_) {
        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
        gtk_container_add(GTK_CONTAINER(frame), labelw);
        labelw = frame;
    }

    put_with_label(labelw);
}

/**
 * Create a button or text display.
 * If a callback was given via at->callback(), creates a button;
 * otherwise creates a label.
 */ 
void AW_window::create_button(const char *macro_name, const char *button_text, 
                              const char *mnemonic, const char *color) {
    if (prvt->action_template.clicked.size() == 0) {
        // @@@MERGE: Refactor to use create_label directly
        create_label(button_text, color && color[0]=='+');
        return;
    }

    GtkWidget *button_label, *button;
    button_label = make_label(button_text, _at.length_of_buttons, mnemonic);

    AW_action *act = action_register(macro_name, true);

    if (button_text) act->set_label(button_text); // FIXME Mnemonic

    button = gtk_button_new();
    gtk_container_add(GTK_CONTAINER(button), button_label);
       
    act->bind(button, "clicked");

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

struct toggle_button_data {
    AW_awar* awar;
    GtkWidget *yes, *no, *toggle;
};

static void _aw_update_toggle_icon(AW_root*, toggle_button_data *tbd) {
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(tbd->toggle));
    if (tbd->awar->read_int()) {
        if (child == tbd->yes) return; // nothing to do
        g_object_ref(tbd->no);
        gtk_container_remove(GTK_CONTAINER(tbd->toggle), tbd->no);
        gtk_container_add(GTK_CONTAINER(tbd->toggle), tbd->yes);
    } 
    else {
        if (child == tbd->no) return; // nothing to do
        g_object_ref(tbd->yes);
        gtk_container_remove(GTK_CONTAINER(tbd->toggle), tbd->yes);
        gtk_container_add(GTK_CONTAINER(tbd->toggle), tbd->no);
    }
    gtk_widget_show_all(tbd->toggle);    
}

/**
 * Creates a toggle button.
 * 
 * @param var_name  must be integer awar. 0=no, 1=yes
 * @param yes       Text/Icon for active toggle button
 * @param no        Text/Icon for inactive toggle button
 * @param width
 */
void AW_window::create_toggle(const char *awar_name, const char *no, const char *yes, int width) {
    AW_awar* awar = get_root()->awar_no_error(awar_name);
    aw_return_if_fail(awar != NULL);
    aw_return_if_fail(yes != NULL);
    aw_return_if_fail(no != NULL);

    GtkWidget* checkButton = gtk_toggle_button_new();
    GtkWidget* no_label    = make_label(no, width);
    GtkWidget* yes_label   = make_label(yes, width);
    gtk_container_add(GTK_CONTAINER(checkButton), no_label);

    toggle_button_data *data = new toggle_button_data; //@@@LEAK 
    data->awar = awar;
    data->yes  = yes_label;
    data->no   = no_label;
    data->toggle = checkButton;
    awar->add_callback(makeRootCallback(_aw_update_toggle_icon, data));
    awar->bind_value(G_OBJECT(checkButton), "active");
    _aw_update_toggle_icon(NULL, data);
   
    awar->changed_by_user += prvt->action_template.clicked;
    prvt->action_template = AW_action();
    
    put_with_label(checkButton);   
}

/**
 * Creates a checkbox
 * @param awar_name Name of AWAR to be connected to button
 * @param inverse   Invert AWAR (checked box -> false)
 */
void AW_window::create_checkbox(const char* awar_name, bool inverse) {
    AW_awar* awar = get_root()->awar_no_error(awar_name);
    aw_return_if_fail(awar != NULL);

    GtkWidget *checkBox = gtk_check_button_new();
    awar->bind_value(G_OBJECT(checkBox), "active",
                     inverse ? new _awar_inverse_bool_mapper() : NULL);

    awar->changed_by_user += prvt->action_template.clicked;
    prvt->action_template = AW_action();

    put_with_label(checkBox);
}

/**
 * Creates checkbox with inverted truth value
 * @param awar_name Name of AWAR to be connected to button
 */
void AW_window::create_checkbox_inverse(const char* var_name) {
    create_checkbox(var_name, true); 
}

static gboolean noop_signal_handler(GtkWidget* /*wgt*/, gpointer /*user_data*/) {
    return true; // stop signal
}

/**
 * Creates a data entry field. 
 * If the AWAR is numeric, creates a spinner, if text creates a 
 * single line text field.
 */
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

    awar->changed_by_user += prvt->action_template.clicked;
    prvt->action_template = AW_action();

    if (width > 0) {
        gtk_entry_set_width_chars(GTK_ENTRY(entry), width);
    }

    gtk_entry_set_activates_default(GTK_ENTRY(entry), true);
    put_with_label(entry);
}


void AW_window::create_text_field(const char *var_name, int columns /* = 20 */, int rows /*= 4*/){
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_if_fail(awar != NULL);

    GtkWidget *entry = gtk_text_view_new();
    GtkTextBuffer *textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(entry));
    awar->bind_value(G_OBJECT(textbuffer), "text");
    // text property doesn't seem to notify on its own
    g_signal_connect(G_OBJECT(textbuffer), "changed", G_CALLBACK(g_object_notify), (gpointer)"text");

    GtkWidget *scrolled_entry = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_entry), 
                                   GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_entry), entry);

    int char_width, char_height;
    prvt->get_font_size(char_width, char_height);
    gtk_widget_set_size_request(scrolled_entry, char_width * columns, char_height * rows);

    awar->changed_by_user += prvt->action_template.clicked;
    prvt->action_template = AW_action();

    put_with_label(scrolled_entry);
}

// ----------------------------------------------------------------------
// force-diff-sync 1284672342939 (remove after merging back to trunk)
// ----------------------------------------------------------------------

// -------------------------
//      Hotkey Checking

#if defined(DEBUG)
#define CHECK_DUPLICATED_MNEMONICS
#endif

#ifdef CHECK_DUPLICATED_MNEMONICS

inline char oppositeCase(char c) {
    return isupper(c) ? tolower(c) : toupper(c);
}
static void strcpy_overlapping(char *dest, char *src) {
    int src_len = strlen(src);
    memmove(dest, src, src_len+1);
}
static const char *possible_mnemonics(const char *used_mnemonics, bool top_level, const char *topic_name) {
    static char *unused;
    for (int fromTopic = 1; fromTopic>=0; --fromTopic) {
        freedup(unused, fromTopic ? topic_name : "abcdefghijklmnopqrstuvwxyz");

        for (int t = 0; unused[t]; ++t) {
            bool remove = false;
            if ((top_level && !isalpha(unused[t])) || !isalnum(unused[t])) { // remove useless chars
                remove = true;
            }
            else {
                char *dup = strchr(unused, unused[t]);
                if (dup && (dup-unused)<t) { // remove duplicated chars
                    remove = true;
                }
                else {
                    dup = strchr(unused, oppositeCase(unused[t]));
                    if (dup && (dup-unused)<t) { // char is duplicated with opposite case
                        dup[0] = toupper(dup[0]); // prefer upper case
                        remove = true;
                    }
                }
            }
            if (remove) {
                strcpy_overlapping(unused+t, unused+t+1);
                --t;
            }
        }

        for (int t = 0; used_mnemonics[t]; ++t) {
            char c = used_mnemonics[t]; // upper case!
            char *u = strchr(unused, c);
            if (u) strcpy_overlapping(u, u+1); // remove char
            u = strchr(unused, tolower(c));
            if (u) strcpy_overlapping(u, u+1); // remove char
        }

        if (unused[0]) {
            if (!fromTopic) {
                freeset(unused, GBS_global_string_copy("unused but not in topic: \"%s\"", unused));
            }
            break;
        }
    }
    return unused;
}

class MnemonicScope : virtual Noncopyable {
    char          *name; // of menu or window
    char          *used; // mnemonics (all upper case)
    MnemonicScope *parent;
    StrArray       requested; // topics lacking sufficient mnemonic

    void print_at_menu(FILE *out) {
        if (parent) {
            parent->print_at_menu(out);
            fputc('|', out);
        }
        fputs(name, out);
    }
    void print_at_location(FILE *out, const char *topic) {
        fputs(" (", out);
        print_at_menu(out);
        fprintf(out, "|%s)\n", topic);
    }

    void requestPossibilities(const char *topic_name) {
        // will be shows delayed (when menu closes)
        requested.put(strdup(topic_name));
    }

    void showRequestedPossibilities() {
        for (int i = 0; requested[i]; ++i) {
            const char *possible = possible_mnemonics(used, !parent, requested[i]);
            fprintf(stderr, "Warning: Possible mnemonics for '%s': '%s'\n", requested[i], possible);
        }
    }

    void warn_mnemonic(const char *topic_name, const char *mnemonic, const char *warning) {
        fprintf(stderr, "Warning: mnemonic '%s' %s", mnemonic, warning);
        print_at_location(stderr, topic_name);
    }

public:

    MnemonicScope(const char *name_, MnemonicScope *parent_)
        : name(strdup(name_)),
          used(strdup("")),
          parent(parent_)
    {}

    ~MnemonicScope() {
        showRequestedPossibilities();
        free(name);
        free(used);
    }

    void add(const char *topic_name, const char *mnemonic);

    MnemonicScope *get_parent() { return parent; }
};

static MnemonicScope *current_mscope = NULL;
static MnemonicScope *help_mscope    = NULL;

void MnemonicScope::add(const char *topic_name, const char *mnemonic) {
    if (mnemonic && mnemonic[0] != 0) {
        if (mnemonic[1]) { // longer than 1 char -> wrong
            warn_mnemonic(topic_name, mnemonic, "is too long; only 1 character allowed");
        }

        if (topic_name[0] == '#') { // graphical menu
            if (mnemonic[0]) {
                warn_mnemonic(topic_name, mnemonic, "is useless for graphical menu entry");
            }
        }
        else if (!isalpha(mnemonic[0]) && !get_parent()) { // do not allow top-level numeric mnemonics
            warn_mnemonic(topic_name, mnemonic, "is invalid (allowed: a-z)");
            requestPossibilities(topic_name);
        }
        else if (!isalnum(mnemonic[0])) {
            warn_mnemonic(topic_name, mnemonic, "is invalid (allowed: a-z and 0-9)");
            requestPossibilities(topic_name);
        }
        else {
            char *TOPIC_NAME = ARB_strupper(strdup(topic_name));
            char  HOTKEY     = toupper(mnemonic[0]); // store hotkeys case-less (case does not matter when pressing the hotkey)

            if (strchr(TOPIC_NAME, HOTKEY)) {  // occurs in menu text
                if (strchr(used, HOTKEY)) {
                    warn_mnemonic(topic_name, mnemonic, "is used multiple times");
                    requestPossibilities(topic_name);
                }
                else {
                    freeset(used, GBS_global_string_copy("%s%c", used, HOTKEY));

                    if (!strchr(topic_name, mnemonic[0])) {
                        char *warning = GBS_global_string_copy("has wrong case, use '%c'", HOTKEY == mnemonic[0] ? tolower(HOTKEY) : HOTKEY);
                        warn_mnemonic(topic_name, mnemonic, warning);
                        free(warning);
                    }
                }
            }
            else {
                warn_mnemonic(topic_name, mnemonic, "is useless; does not occur in text");
                requestPossibilities(topic_name);
            }
            free(TOPIC_NAME);
        }
    }
    else {
        if (topic_name[0] != '#') { // not a graphical menu
            fputs("Warning: mnemonic is missing", stderr);
            print_at_location(stderr, topic_name);
            requestPossibilities(topic_name);
        }
    }
}

static void open_test_duplicate_mnemonics(const char *sub_menu_name, const char *mnemonic) {
    aw_warn_if_fail_BREAKPOINT(current_mscope);
    current_mscope->add(sub_menu_name, mnemonic);

    MnemonicScope *prev = current_mscope;
    current_mscope             = new MnemonicScope(sub_menu_name, prev);
}

static void close_test_duplicate_mnemonics() {
    MnemonicScope *prev = current_mscope->get_parent();
    delete current_mscope;
    current_mscope = prev;
}

static void init_duplicate_mnemonic(const char *window_name) {
    if (!current_mscope) {
        current_mscope = new MnemonicScope(window_name, NULL);
        help_mscope    = new MnemonicScope("<HELP>", current_mscope);
    }
    else {
        while (current_mscope->get_parent()) {
            close_test_duplicate_mnemonics();
        }
    }
}

static void test_duplicate_mnemonics(const char *topic_name, const char *mnemonic) {
    aw_assert(current_mscope);
    current_mscope->add(topic_name, mnemonic);
}

static void exit_duplicate_mnemonic() {
    delete help_mscope; help_mscope = NULL;
    while (current_mscope) close_test_duplicate_mnemonics();
}


#endif

// ----------------------------------------------------------------------
// force-diff-sync 9273192739 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::create_menu(const char *name, const char *mnemonic, AW_active mask /*= AWM_ALL*/){
    aw_return_if_fail(legal_mask(mask));

    // The user might leave sub menus open. Close them before creating a new top level menu.
    while(prvt->menus.size() > 1) {
        close_sub_menu();
    }

#ifdef CHECK_DUPLICATED_MNEMONICS
    init_duplicate_mnemonic(window_name);
#endif

    insert_sub_menu(name, mnemonic, mask);
}

void AW_window::close_sub_menu(){
#ifdef CHECK_DUPLICATED_MNEMONICS
    close_test_duplicate_mnemonics();
#endif
    aw_return_if_fail(prvt->menus.size() > 1);
    
    prvt->menus.pop();
}

void AW_window::all_menus_created() const { // this is called by AW_window::show() (i.e. after all menus have been created)
#if defined(DEBUG)
    if (prvt->menus.size()>0) { // window had menu
        aw_warn_if_fail_BREAKPOINT(prvt->menus.size() == 1);
#ifdef CHECK_DUPLICATED_MNEMONICS
        exit_duplicate_mnemonic(); // cleanup mnemonic check
#endif
    }
#endif // DEBUG
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

    prvt->combo_box = combo_box;
    prvt->selection_list = slist;
    return slist;
}

void AW_window::insert_option(const char *on, const char *mn, const char *vv) {
    insert_option_internal(on, mn, vv, false); 
}
void AW_window::insert_default_option(const char *on, const char *mn, const char *vv) { 
    insert_option_internal(on, mn, vv, true); 
}
void AW_window::insert_option(const char *on, const char *mn, int vv) { 
    insert_option_internal(on, mn, vv, false);
}
void AW_window::insert_default_option(const char *on, const char *mn, int vv) { 
    insert_option_internal(on, mn, vv, true); 
}
void AW_window::insert_option(const char *on, const char *mn, float vv) { 
    insert_option_internal(on, mn, vv, false); 
}
void AW_window::insert_default_option(const char *on, const char *mn, float vv) { 
    insert_option_internal(on, mn, vv, true); 
}


template <class T>
void AW_window::insert_option_internal(const char *option_name, const char */*mnemonic*/, 
                                       T var_value, 
                                       bool default_option) {
    aw_return_if_fail(prvt->selection_list != NULL); //current option menu has to be set
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

void AW_window::update_option_menu() {
    aw_return_if_fail(prvt->selection_list);
    prvt->selection_list->update();
    put_with_label(prvt->combo_box);
    prvt->selection_list = NULL;
}

void AW_window::clear_option_menu(AW_selection_list *sel) {
    sel->clear();
}

AW_selection_list* AW_window::create_selection_list(const char *awar_name, int columns, int rows, bool fallback2default) {
    AW_awar* awar = get_root()->awar_no_error(awar_name);
    aw_return_val_if_fail(awar, NULL);
    aw_warn_if_fail(!_at.label_for_inputfield); // labels have no effect for selection lists

   
    GtkWidget *tree = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
   
    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scrolled_win), tree);

    if (!_at.to_position_exists) {
        // set size
        int char_width, char_height;
        prvt->get_font_size(char_width, char_height);
        gtk_widget_set_size_request(scrolled_win, char_width * columns, char_height * rows);
    }

    AW_selection_list *slist = new AW_selection_list(awar);
    slist->bind_widget(tree);

    awar->dclicked += prvt->action_template.dclicked;
    prvt->action_template = AW_action();

    put_with_label(scrolled_win);
    return slist;
}

// BEGIN TOGGLE FIELD STUFF

/**
 * Begins a radio button group
 * @param var_name    name of awar
 * @param orientation 0 -> vertical, != 0 horizontal layout
 */
void AW_window::create_toggle_field(const char *var_name, int orientation /*= 0*/){
    AW_awar* awar = get_root()->awar_no_error(var_name);
    aw_return_if_fail(awar != NULL);
    
    if (orientation == 0) {
        prvt->toggle_field = gtk_vbox_new(true, 2);
    } 
    else {
        prvt->toggle_field = gtk_hbox_new(true, 2);
    }

    prvt->toggle_field_awar_name = var_name;
}

/**
 * Begins a radio button group with a label
 */
void AW_window::create_toggle_field(const char *var_name, const char *labeli, const char *mnemonic) {
    if(labeli){
        char *lab = aw_convert_mnemonic(labeli, mnemonic);
        this->label(lab);
        free(lab);
    }
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

    prvt->action_template.set_label(toggle_label); // fixme mnemonic

    AW_choice *choice = awar->add_choice(prvt->action_template, var_value, default_toggle);
    choice->bind(GTK_WIDGET(prvt->radio_last), "clicked");
    choice->set_label(toggle_label);
    /*
    AW_varUpdateInfo *vui = new AW_varUpdateInfo(this, NULL, AW_WIDGET_TOGGLE_FIELD,
                                                 awar, var_value, prvt->callback);
    g_signal_connect((gpointer)prvt->radio_last, "clicked", 
                     G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback),
                     (gpointer)vui);
    
    if(default_toggle){
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prvt->radio_last), true);
    }
    */
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
    GtkAlignment* align = GTK_ALIGNMENT(gtk_alignment_new(0.0f, 0.0f, 0.0f, 0.0f));
    put_with_label(prvt->toggle_field, align);
    prvt->radio_last = NULL; // end of radio group
    prvt->toggle_field = NULL;
}
// END TOGGLE FIELD STUFF

// ----------------------------------------------------------------------
// force-diff-sync 2939128467234 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::draw_line(int x1, int y1, int x2, int y2, int width, bool resize){
    aw_return_if_fail(xfig_data != NULL);  // forgot to call load_xfig ?
    
    xfig_data->add_line(x1, y1, x2, y2, width);
    _at.set_xfig(xfig_data);

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
        set_window_size(_at.max_x_size+1000, _at.max_y_size+1000);
    }
}


AW_device_click *AW_window::get_click_device(AW_area area, int mousex, int mousey, int max_distance) {
    AW_area_management *aram         = prvt->areas[area];
    AW_device_click    *click_device = NULL;

    if (aram) {
        click_device = aram->get_click_device();
        click_device->init_click(mousex, mousey, max_distance, AW_ALL_DEVICES);
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

    size_device->restart_tracking();
    size_device->reset(); // @@@ hm

    return size_device;
}


void AW_window::insert_help_topic(const char *labeli, 
                                  const char *mnemonic, const char *helpText,
                                  AW_active mask, const WindowCallback& cb) {
    aw_return_if_fail(prvt->help_menu != NULL);
    
#ifdef CHECK_DUPLICATED_MNEMONICS
    if (!current_mscope) init_duplicate_mnemonic(window_name);
    MnemonicScope *tmp = current_mscope;
    current_mscope     = help_mscope;
    test_duplicate_mnemonics(labeli, mnemonic);
    current_mscope     = tmp;
#endif

    prvt->menus.push(prvt->help_menu);
    insert_menu_topic(helpText, labeli, mnemonic, helpText, mask, cb);
    prvt->menus.pop();
}

void AW_window::insert_menu_topic(const char *topic_id, const char *labeli,
                                  const char *mnemonic, const char *helpText,
                                  AW_active mask, const WindowCallback& wcb) {
    aw_return_if_fail(legal_mask(mask));
    aw_return_if_fail(prvt->menus.size() > 0); //closed too many menus
    aw_return_if_fail(topic_id != NULL);

#ifdef CHECK_DUPLICATED_MNEMONICS
    test_duplicate_mnemonics(labeli, mnemonic);
#endif
    prvt->action_template.set_label(labeli); // fixme mnemonic
    if (helpText) help_text(helpText);
    sens_mask(mask);
    callback(wcb);
    AW_action *act = action_register(topic_id, false);

    GtkWidget *wlabel    = make_label(labeli, 0, mnemonic);
    GtkWidget *alignment = gtk_alignment_new(0.f, 0.5f, 0.f, 0.f);
    GtkWidget *item      = gtk_menu_item_new();
    gtk_container_add(GTK_CONTAINER(alignment), wlabel);
    gtk_container_add(GTK_CONTAINER(item),      alignment);

    gtk_menu_shell_append(prvt->menus.top(), item);
    
    act->bind(item, "activate");
}

// ----------------------------------------------------------------------
// force-diff-sync 824638723647 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::insert_sub_menu(const char *labeli, const char *mnemonic, AW_active mask /*= AWM_ALL*/){
    aw_return_if_fail(legal_mask(mask));
    aw_return_if_fail(prvt->menus.top());
  
#ifdef CHECK_DUPLICATED_MNEMONICS
    open_test_duplicate_mnemonics(labeli, mnemonic);
#endif
  
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

    #if defined(DUMP_MENU_LIST)
        dumpOpenSubMenu(name);
    #endif // DUMP_MENU_LIST
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

// ----------------------------------------------------------------------
// force-diff-sync 264782364273 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::load_xfig(const char *file, bool resize /*= true*/){
    int width, height;
    prvt->get_font_size(width, height);

    // 0.8 is _exactly_ right, +/- 0.01 already looks off
    width *= 0.8; height *=0.8;

    if (file)   xfig_data = new AW_xfig(file, width, height);
    else        xfig_data = new AW_xfig(width, height); // create an empty xfig

    xfig_data->create_gcs(get_device(AW_INFO_AREA)); 

    _at.set_xfig(xfig_data);

    // AW_device *device = get_device(AW_INFO_AREA);

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
        set_window_size(_at.max_x_size, _at.max_y_size);
    }

    set_expose_callback(AW_INFO_AREA, makeWindowCallback(AW_xfigCB_info_area, xfig_data));
}

/**
 * Construct window-local id.
 * Prefixes @param id with AW_window::window_defaults_name + "/"
 */
const char *AW_window::local_id(const char *id) const{
    return GBS_global_string("%s/%s", window_defaults_name, id); // @@@ warning - very short lifetime, may cause undesired problems
                                                                 // (motif version was valid until next call of local_id())
}

void AW_window::sep______________() {
    aw_return_if_fail(prvt->menus.size() > 0);
    
    GtkWidget *item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(prvt->menus.top(), item);
}

/**
 * Registers @param wcb as expose callback.
 * Called whenever the @param area receives an expose event.
 * This is where any drawing should be handled.
 */
void AW_window::set_expose_callback(AW_area area, const WindowCallback& wcb) {
    AW_area_management *aram = prvt->areas[area];
    if (aram) aram->set_expose_callback(this, wcb);
}

/**
 * Registers a focus callback.
 * They used to be called as soon as the mouse entered the main drawing area. 
 * For now, they are not called at all.
 */
void AW_window::set_focus_callback(const WindowCallback& wcb) {
    // this is only ever called by AWT_canvas and triggers a re-render of the
    // middle area. the API was designed for "focus-follows-mouse" mode,
    // and makes little sense for GTK
    // (we might need other means to trigger an update of the tree at the right time)
    prvt->focus_cb.connect(wcb, this);
}

/**
 * Registers callback to be executed after the window is shown.
 * Called multiple times if a show() follows a hide().
 */
void AW_window::set_popup_callback(const WindowCallback& wcb) {
    prvt->popup_cb.connect(wcb, this);
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

void AW_window::set_input_callback(AW_area area, const WindowCallback& wcb) {
    AW_area_management *aram = prvt->areas[area];
    aw_return_if_fail(aram != NULL);
    aram->set_input_callback(this, wcb);
}

void AW_window::set_motion_callback(AW_area area, const WindowCallback& wcb) {
    AW_area_management *aram = prvt->areas[area];
    aw_return_if_fail(aram != NULL);
    aram->set_motion_callback(this, wcb);
}

void AW_window::set_resize_callback(AW_area area, const WindowCallback& wcb) {
    AW_area_management *aram = prvt->areas[area];
    aw_return_if_fail(aram != NULL);
    aram->set_resize_callback(this, wcb);
}

// SCROLL BAR STUFF

void AW_window::tell_scrolled_picture_size(AW_screen_area rectangle) {
    aw_drawing_area_set_size(prvt->drawing_area,
                             rectangle.r - rectangle.l, 
                             rectangle.b - rectangle.t);
}

void AW_window::tell_scrolled_picture_size(AW_world rectangle) {
    aw_drawing_area_set_size(prvt->drawing_area,
                             rectangle.r - rectangle.l, 
                             rectangle.b - rectangle.t);
}

AW_pos AW_window::get_scrolled_picture_width() const {
    return aw_drawing_area_get_width(prvt->drawing_area);
}

AW_pos AW_window::get_scrolled_picture_height() const {
    return aw_drawing_area_get_height(prvt->drawing_area);
}

void AW_window::reset_scrolled_picture_size() {
    aw_drawing_area_set_size(prvt->drawing_area, 0, 0);
}

void AW_window::set_vertical_scrollbar_top_indent(int indent) {
    aw_drawing_area_set_unscrolled_height(prvt->drawing_area, indent);
}

void AW_window::set_horizontal_scrollbar_left_indent(int indent) {
    aw_drawing_area_set_unscrolled_width(prvt->drawing_area, indent);
}

void AW_window::_get_area_size(AW_area area, AW_screen_area *square) {
    AW_area_management *aram = prvt->areas[area];
    aw_return_if_fail(aram != NULL);
    *square = aram->get_common()->get_screen();
}

void AW_window::get_scrollarea_size(AW_screen_area *square) {
    aw_return_if_fail(square != NULL);
    square->l = square->t = 0;
    aw_drawing_area_get_scrolled_size(prvt->drawing_area, &square->r, &square->b);
}

void AW_window::calculate_scrollbars() {
    AW_screen_area scrollArea;
    get_scrollarea_size(&scrollArea);

    const int hpage_increment = window_local_awar("horizontal_page_increment")->read_int() * scrollArea.r / 100;
    const int hstep_increment = window_local_awar("scroll_width_horizontal")  ->read_int();
    const int vpage_increment = window_local_awar("vertical_page_increment")  ->read_int() * scrollArea.b / 100;
    const int vstep_increment = window_local_awar("scroll_width_vertical")    ->read_int();

    aw_drawing_area_set_increments(prvt->drawing_area, hstep_increment, hpage_increment,
                                   vstep_increment, vpage_increment);
}

// ----------------------------------------------------------------------
// force-diff-sync 82346723846 (remove after merging back to trunk)
// ----------------------------------------------------------------------

static void _aw_window_recalc_scrollbar_cb(AW_root*, AW_window* aww) {
    aww->calculate_scrollbars();
}

const char *AW_window::window_local_awarname(const char *localname, bool tmp) {
    CONSTEXPR int MAXNAMELEN = 200;
    static char   buffer[MAXNAMELEN];
    return GBS_global_string_to_buffer(buffer, MAXNAMELEN,
                                       tmp ? "tmp/window/%s/%s" : "window/%s/%s",
                                       window_defaults_name, localname);
    // (see also aw_size_awar_name)
}

AW_awar *AW_window::window_local_awar(const char *localname, bool tmp) {
    return get_root()->awar(window_local_awarname(localname, tmp));
}

void AW_window::create_window_variables() {
    aw_return_if_fail(prvt->drawing_area != NULL);

    RootCallback scrollbar_recalc_cb = makeRootCallback(_aw_window_recalc_scrollbar_cb, this);

    get_root()->awar_int(window_local_awarname("horizontal_page_increment"), 50)->add_callback(scrollbar_recalc_cb);
    get_root()->awar_int(window_local_awarname("vertical_page_increment"),   50)->add_callback(scrollbar_recalc_cb);
    get_root()->awar_int(window_local_awarname("scroll_width_horizontal"),    9)->add_callback(scrollbar_recalc_cb);
    get_root()->awar_int(window_local_awarname("scroll_width_vertical"),     20)->add_callback(scrollbar_recalc_cb);

    GtkAdjustment *adj;
    adj = aw_drawing_area_get_vertical_adjustment(prvt->drawing_area);
    get_root()->awar_int(window_local_awarname("scroll_position_vertical"), 0)
        ->add_target_var(&this->slider_pos_vertical)
        ->bind_value(G_OBJECT(adj), "value", new _awar_float_to_int_mapper());

    adj = aw_drawing_area_get_horizontal_adjustment(prvt->drawing_area);
    get_root()->awar_int(window_local_awarname("scroll_position_horizontal"), 0)
        ->add_target_var(&this->slider_pos_horizontal)
        ->bind_value(G_OBJECT(adj), "value", new _awar_float_to_int_mapper());
}

void AW_window::set_vertical_scrollbar_position(int position){
#if defined(DEBUG) && 0
    fprintf(stderr, "set_vertical_scrollbar_position to %i\n", position);
#endif
    aw_drawing_area_set_vertical_slider(prvt->drawing_area, position);
}

void AW_window::set_horizontal_scrollbar_position(int position) {
#if defined(DEBUG) && 0
    fprintf(stderr, "set_horizontal_scrollbar_position to %i\n", position);
#endif
    aw_drawing_area_set_horizontal_slider(prvt->drawing_area, position);
}

void AW_window::set_vertical_change_callback(const WindowCallback& wcb) {
    window_local_awar("scroll_position_vertical")->changed.connect(wcb, this);
}

void AW_window::set_horizontal_change_callback(const WindowCallback& wcb) {
    window_local_awar("scroll_position_horizontal")->changed.connect(wcb, this);
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

// ----------------------------------------------------------------------
// force-diff-sync 874687234237 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::window_fit() {
    // let gtk do the sizing based on content
    // (default size will be ignored if requisition of 
    //  children is greater)
    prvt->set_size(0,0);
}

AW_window::AW_window() 
  : recalc_size_at_show(AW_KEEP_SIZE),
    recalc_pos_at_show(AW_KEEP_POS),
    prvt(new AW_window::AW_window_gtk()),
    _at(this),
    xfig_data(NULL),
    event(),
    window_name(NULL),
    window_defaults_name(NULL),
    slider_pos_vertical(0),
    slider_pos_horizontal(0),
    left_indent_of_horizontal_scrollbar(0),
    top_indent_of_vertical_scrollbar(0),
    picture(new AW_screen_area),
    main_drag_gc(0)
{
    aw_assert(AW_root::SINGLETON); // must have AW_root
  
    //    reset_scrolled_picture_size();
    
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
    
    prvt->set_size(awar_width->read_int(), awar_height->read_int());
    prvt->set_resizable(resizable);

    // set minimum window size to size provided by init
    if (width  > _at.max_x_size) _at.max_x_size = width;
    if (height > _at.max_y_size) _at.max_y_size = height;

    // manage transience:
    // the first created window is considered the main application
    // window. should it be closed, the next created window supersedes it.
    if (!get_root()->root_window || !get_root()->root_window->is_shown()) {
        // no root window yet, or root window not visible => I'm root
        get_root()->root_window = this;
    }

#if 0
    // transient windows are always in front of main window
    // we don't want this, at least not in all cases. 
    // disabled for now
    else {
        // all other windows are "transient", i.e. dialogs relative to it.
        // (relevant for window placement by window manager)
        gtk_window_set_transient_for(prvt->window, 
                                     get_root()->root_window->prvt->window);

    }
#endif

    // assign name to window (for styling and debugging)
    // scheme is "<main>/<window>", i.e. "ARB_NT/ARB_NT" or "ARB_NT/SEARCH_AND_QUERY"
    const char* name = GBS_global_string("%s/%s", get_root()->root_window->window_defaults_name, 
                      window_defaults_name);
    gtk_widget_set_name(GTK_WIDGET(prvt->window), name);

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
    prvt->fixed_size_area = AW_DRAWING_AREA(aw_drawing_area_new());
    prvt->areas[AW_INFO_AREA] = new AW_area_management(this, AW_INFO_AREA, GTK_WIDGET(prvt->fixed_size_area));
    g_signal_connect (prvt->window, "delete_event", G_CALLBACK (close_window_handler), this);
}

/** 
 * Connects an action to the close button of this window.
 * The action will be triggered /before/ the window is actually hidden.
 * Whether or not it will be hidden can be set with set_hide_on_close().
 */
void AW_window::set_close_action(AW_action *act) {
    prvt->close_action = act;
}

/**
 * Connects an action to the close button of this window.
 * See set_close_action(AW_action *act)
 */
void AW_window::set_close_action(const char* act_name) {
    AW_action *have_local  = action_try(act_name, true);
    AW_action *have_global = action_try(act_name, false);

    bool got_action = have_local || have_global;
    aw_warn_if_fail(got_action);
    if (got_action) {
        bool ambiguous_action = have_local && have_global;
#if defined(DEBUG)
        aw_warn_if_fail(!ambiguous_action); // possibly wrong behavior; resolve in client code
#endif
        set_close_action(have_local ? have_local : have_global);
    }
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

// ----------------------------------------------------------------------
// force-diff-sync 9268347253894 (remove after merging back to trunk)
// ----------------------------------------------------------------------

bool AW_window::is_shown() const{
    return gtk_widget_get_visible(GTK_WIDGET(prvt->window));
}

void AW_window::show() {
    if (is_shown()) return;

    // The user might leave sub menus open. Close them before checking mnemonics
    while(prvt->menus.size() > 1) {
        close_sub_menu();
    }
    all_menus_created();

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

    prvt->popup_cb.emit();
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
    prvt->hide_on_close = value;
}


/********* awar / action helpers ***********/

#define OPTIONALLY(loc,id) (loc ? local_id(id) : (id))

/** Calls AW_root::action_try with the ID of this window prefixed to action_id */
AW_action* AW_window::action_try(const char* action_id, bool localize) {
    return get_root()->action_try(OPTIONALLY(localize, action_id));
}

/** Calls AW_root::action with the ID of this window prefixed to action_id */
AW_action* AW_window::action(const char* action_id, bool localize) {
    return get_root()->action(OPTIONALLY(localize, action_id));
}

/** Calls AW_root::action_register with the ID of this window prefixed to action_id */
AW_action* AW_window::action_register(const char* action_id, bool localize) {
    // create action using template action from pimpl
    AW_action* act = get_root()->action_register(OPTIONALLY(localize, action_id), prvt->action_template);
    // clear action template
    prvt->action_template = AW_action(); 
    return act;
}

#undef OPTIONALLY
