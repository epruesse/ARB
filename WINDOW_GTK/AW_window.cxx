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
#define DUMP_BUTTON_CREATION

const int AW_NUMBER_OF_F_KEYS = 20;


void AW_window::at(int x, int y){ _at.at(x,y); }
void AW_window::at_x(int x) { _at.at_x(x); }
void AW_window::at_y(int y){ _at.at_y(y); }
void AW_window::at_shift(int x, int y){ _at.at_shift(x,y); }
void AW_window::at_newline(){ _at.at_newline(); }
void AW_window::unset_at_commands() { _at.unset_at_commands(); }
bool AW_window::at_ifdef(const  char *id){  return _at.at_ifdef(id); }
void AW_window::at_set_to(bool attach_x, bool attach_y, int xoff, int yoff) {
  _at.at_set_to(attach_x, attach_y, xoff, yoff);
}
void AW_window::at_unset_to() { _at.at_unset_to(); }
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

void AW_window::get_scrollarea_size(AW_screen_area *square) {
    _get_area_size(AW_MIDDLE_AREA, square);
    FIXME("left indent and right indent not implemented");
//    square->r -= left_indent_of_horizontal_scrollbar;
//    square->b -= top_indent_of_vertical_scrollbar;
}

void AW_window::calculate_scrollbars(){

    GTK_PARTLY_IMPLEMENTED;
    AW_screen_area scrollArea;
    get_scrollarea_size(&scrollArea);

    // HORIZONTAL
    {
        const int picture_width = (int)get_scrolled_picture_width();
        const int scrollArea_width = scrollArea.r - scrollArea.l;
        
        gtk_adjustment_set_lower(prvt->hAdjustment, 0);
        gtk_adjustment_set_step_increment(prvt->hAdjustment, 1);
        gtk_adjustment_set_page_increment(prvt->hAdjustment, 10);
        gtk_adjustment_set_upper(prvt->hAdjustment, picture_width);
        gtk_adjustment_set_page_size(prvt->hAdjustment, scrollArea_width);
        gtk_adjustment_changed(prvt->hAdjustment);
        //gtk_adjustment_set_value()
        
        
        
//        int slider_max = 
//        if (slider_max <1) {
//            slider_max = 1;
//        }
//   
//
//        bool use_horizontal_bar     = true;
//        int  slider_size_horizontal = scrollArea.r;
//
//        if (slider_size_horizontal < 1) slider_size_horizontal = 1; // ist der slider zu klein (<1) ?
//        if (slider_size_horizontal > slider_max) { // Schirm groesser als Bild
//            slider_size_horizontal = slider_max; // slider nimmt ganze laenge ein
//            gtk_adjustment_set_value(prvt->hAdjustment, 0);// slider ganz links setzen
//            use_horizontal_bar = false; // kein horizontaler slider mehr
//        }
//
//        // check wether XmNValue is to big
//        double position_of_slider = gtk_adjustment_get_value(prvt->hAdjustment);
//        
//        if (position_of_slider > (slider_max - slider_size_horizontal)) { // steht der slider fuer slidergroesse zu rechts ?
//            position_of_slider = slider_max - slider_size_horizontal; // -1 ? vielleicht !
//            if (position_of_slider < 0) position_of_slider = 0;
//            gtk_adjustment_set_value(prvt->hAdjustment, position_of_slider);
//        }
//        // Anpassung fuer resize, wenn unbeschriebener Bereich vergroessert wird
//        int max_slider_pos = (int)(get_scrolled_picture_width() - scrollArea.r);
//        if (slider_pos_horizontal>max_slider_pos) {
//            slider_pos_horizontal = use_horizontal_bar ? max_slider_pos : 0;
//        }
//
//        gtk_adjustment_set_upper(prvt->hAdjustment, slider_max);
//        XtVaSetValues(p_w->scroll_bar_horizontal, XmNmaximum, slider_max, NULL);
//        XtVaSetValues(p_w->scroll_bar_horizontal, XmNsliderSize, slider_size_horizontal, NULL);

        update_scrollbar_settings_from_awars(AW_HORIZONTAL);
    }

    // VERTICAL
    {
        
        const int picture_height = (int)get_scrolled_picture_height();
        const int scrollArea_height= scrollArea.b - scrollArea.t;
        
        gtk_adjustment_set_lower(prvt->vAdjustment, 0);
        gtk_adjustment_set_step_increment(prvt->vAdjustment, 1);
        gtk_adjustment_set_page_increment(prvt->vAdjustment, 10);
        gtk_adjustment_set_upper(prvt->vAdjustment, picture_height);
        gtk_adjustment_set_page_size(prvt->vAdjustment, scrollArea_height);
        gtk_adjustment_changed(prvt->vAdjustment);

//        
//        
//        
//        int slider_max = (int)get_scrolled_picture_height();
//        if (slider_max <1) {
//            slider_max = 1;
//            XtVaSetValues(p_w->scroll_bar_vertical, XmNsliderSize, 1, NULL);
//        }
//
//        bool use_vertical_bar     = true;
//        int  slider_size_vertical = scrollArea.b;
//
//        if (slider_size_vertical < 1) slider_size_vertical = 1;
//        if (slider_size_vertical > slider_max) {
//            slider_size_vertical = slider_max;
//            XtVaSetValues(p_w->scroll_bar_vertical, XmNvalue, 0, NULL);
//            use_vertical_bar = false;
//        }
//
//        // check wether XmNValue is to big
//        int position_of_slider;
//        XtVaGetValues(p_w->scroll_bar_vertical, XmNvalue, &position_of_slider, NULL);
//        if (position_of_slider > (slider_max-slider_size_vertical)) {
//            position_of_slider = slider_max-slider_size_vertical; // -1 ? vielleicht !
//            if (position_of_slider < 0) position_of_slider = 0;
//            XtVaSetValues(p_w->scroll_bar_vertical, XmNvalue, position_of_slider, NULL);
//        }
//        // Anpassung fuer resize, wenn unbeschriebener Bereich vergroessert wird
//        int max_slider_pos = (int)(get_scrolled_picture_height() - scrollArea.b);
//        if (slider_pos_vertical>max_slider_pos) {
//            slider_pos_vertical = use_vertical_bar ? max_slider_pos : 0;
//        }
//
//        XtVaSetValues(p_w->scroll_bar_vertical, XmNsliderSize, 1, NULL);
//        XtVaSetValues(p_w->scroll_bar_vertical, XmNmaximum, slider_max, NULL);
//        XtVaSetValues(p_w->scroll_bar_vertical, XmNsliderSize, slider_size_vertical, NULL);

        update_scrollbar_settings_from_awars(AW_VERTICAL);
    }
}

void AW_window::callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2){
    FIXME("Callback newed every time, possible memory leak");
    prvt->callback = new AW_cb_struct(this, (AW_CB)f, cd1, cd2);
}

void AW_window::callback(void (*f)(AW_window*)){
    FIXME("Callback newed every time, possible memory leak");
    prvt->callback = new AW_cb_struct(this, (AW_CB)f);
}


void AW_window::callback(void (*f)(AW_window*, AW_CL), AW_CL cd1){
    FIXME("Callback newed every time, possible memory leak");
    prvt->callback = new AW_cb_struct(this, (AW_CB)f, cd1);
}

void AW_window::callback(AW_cb_struct * /* owner */ awcbs) {
    prvt->callback = awcbs;
}


void AW_window::close_sub_menu(){
    arb_assert(NULL != prvt);
    arb_assert(prvt->menus.size() > 1);
    
    prvt->menus.pop();
    
}



void AW_window::_get_area_size(AW_area area, AW_screen_area *square) {
    AW_area_management *aram = prvt->areas[area];
    *square = aram->get_common()->get_screen();
}








inline void calculate_textsize(const char *str, int *width, int *height) {
    int textwidth  = 0;
    int textheight = 1;
    int linewidth  = 0;

    for (int p = 0; str[p]; ++p) {
        if (str[p] == '\n') {
            if (linewidth>textwidth) textwidth = linewidth;

            linewidth = 0;
            textheight++;
        }
        else {
            linewidth++;
        }
    }

    if (linewidth>textwidth) textwidth = linewidth;

    *width  = textwidth;
    *height = textheight;
}

//TODO right now it seems that this method can be transformed into a private member of aw_window. However I am not sure yet. Check later
static void calculate_label_size(AW_window *aww, int *width, int *height, bool in_pixel, const char *non_at_label) {
    // in_pixel == true -> calculate size in pixels
    // in_pixel == false -> calculate size in characters

    const char *label = non_at_label ? non_at_label : aww->_at.label_for_inputfield;
    if (label) {
        calculate_textsize(label, width, height);
        if (aww->_at.length_of_label_for_inputfield) {
            *width = aww->_at.length_of_label_for_inputfield;
        }
        if (in_pixel) {
            *width  = aww->calculate_string_width(*width);
            *height = aww->calculate_string_height(*height, 0);
        }
    }
    else {
        *width  = 0;
        *height = 0;
    }
}


//TODO I think this method should be transformed into a private member
static char *pixmapPath(const char *pixmapName) {
    return nulldup(GB_path_in_ARBLIB("pixmaps", pixmapName));
}


//TODO I think this should be a private member
const char *aw_str_2_label(const char *str, AW_window *aww) {
    aw_assert(str);

    static const char *last_label = 0;
    static const char *last_str   = 0;
    static AW_window  *last_aww   = 0;

    const char *label;
    if (str == last_str && aww == last_aww) { // reuse result ?
        label = last_label;
    }
    else {
        if (str[0] == '#') {
            label = GB_path_in_ARBLIB("pixmaps", str+1);
        }
        else {
            AW_awar *is_awar = aww->get_root()->label_is_awar(str);

            if (is_awar) { // for labels displaying awar values, insert dummy text here
                int wanted_len = aww->_at.length_of_buttons - 2;
                if (wanted_len < 1) wanted_len = 1;

                char *labelbuf       = GB_give_buffer(wanted_len+1);
                memset(labelbuf, 'y', wanted_len);
                labelbuf[wanted_len] = 0;

                label = labelbuf;
            }
            else {
                label = str;
            }
        }

        // store results locally, cause aw_str_2_label is nearly always called twice with same arguments
        // (see RES_LABEL_CONVERT)
        last_label = label;
        last_str   = str;
        last_aww   = aww;
    }
    return label;
}

static void aw_attach_widget(GtkWidget* /*w*/, AW_at& /*_at*/, int /*default_width*/ = -1) {
    GTK_NOT_IMPLEMENTED;
}


static void AW_label_in_awar_list(AW_window *aww, GtkWidget* widget, const char *str) {
    AW_awar *is_awar = aww->get_root()->label_is_awar(str);
    if (is_awar) {
        char *var_value = is_awar->read_as_string();
        if (var_value) {
            aww->update_label(widget, var_value);
        }
        else {
            FIXME("Code only reachable if asserts are disabled");
            aw_assert(0); // awar not found
            aww->update_label(widget, str); 
        }
        free(var_value);
        is_awar->tie_widget(0, widget, AW_WIDGET_LABEL_FIELD, aww);
    }
}

void AW_window::set_background(const char */*colorname*/, GtkWidget* /*parentWidget*/) {
    GTK_NOT_IMPLEMENTED;
}

int AW_window::calculate_string_height(int rows, int offset) const {
    if (xfig_data) {
        AW_xfig *xfig = (AW_xfig *)xfig_data;
        return (int)((rows * XFIG_DEFAULT_FONT_HEIGHT + offset) * xfig->font_scale); // stdfont 8x13
    }
    else {
        return (rows * XFIG_DEFAULT_FONT_HEIGHT + offset);    // stdfont 8x13
    }
}


static void AW_server_callback(GtkWidget* /*wgt*/, gpointer aw_cb_struct) {
    GTK_PARTLY_IMPLEMENTED;

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
   FIXME("recording not implemented");
   // if (get_root()->prvt->recording) get_root()->prvt->recording->record_action(cbs->id);

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
        
        FIXME("this assumes that widget is a button");
        FIXME("investigate why this code works but the commented one does not");
        g_signal_connect((gpointer)widget, "clicked", G_CALLBACK(AW_server_callback), (gpointer)prvt->callback);
        //g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(AW_server_callback), G_OBJECT(_callback));
    }
    prvt->callback = NULL;
}

int AW_window::calculate_string_width(int columns) const {

    if (xfig_data) {
        AW_xfig *xfig = (AW_xfig *)xfig_data;
        return (int)(columns * xfig->font_scale * XFIG_DEFAULT_FONT_WIDTH);   // stdfont 8x13
    }
    else {
        return columns * XFIG_DEFAULT_FONT_WIDTH; // stdfont 8x13
    }
}

AW_area_management* AW_window::get_area(int index) {
    
    if(index < AW_MAX_AREA) {
        return prvt->areas[index];
    }
    
    return NULL;
}

void AW_window::create_button(const char *macro_name, AW_label button_text, const char */*mnemonic*/, const char */*color*/) {
    // Create a button or text display.
    //
    // If a callback is bound via at->callback(), a button is created.
    // Otherwise a text display is created.
    //
    // if button_text starts with '#' the rest of button_text is used as name of bitmap file used for button
    // if button_text contains a '/' it's interpreted as AWAR name and the button displays the content of the awar
    // otherwise button_text is interpreted as pure text (may contain '\n').
    //
    // If a label has been set, the button/text will be prefixed with a label. 

#if defined(DUMP_BUTTON_CREATION)
    printf("------------------------------ Button '%s'\n", button_text);
    printf("x_for_next_button=%i y_for_next_button=%i\n", _at.x_for_next_button, _at.y_for_next_button);
#endif // DUMP_BUTTON_CREATION

    // register macro
    if (prvt->callback && ((long)prvt->callback != 1)) {
        if (macro_name) {
            prvt->callback->id = GBS_global_string_copy("%s/%s", this->window_defaults_name, macro_name);
            get_root()->define_remote_command(prvt->callback);
        }
        else {
            prvt->callback->id = 0;
        }
    }

    #define SPACE_BEHIND_LABEL  10
    #define SPACE_BEHIND_BUTTON 3

    #define BUTTON_TEXT_X_PADDING 4
    #define BUTTON_TEXT_Y_PADDING 10

    #define BUTTON_GRAPHIC_PADDING 12
    #define FLAT_GRAPHIC_PADDING   4 // for buttons w/o callback

    GtkWidget *buttonOrLabel;
    if (prvt->callback) { // this is a real button
        buttonOrLabel = gtk_button_new();
        this->_set_activate_callback(buttonOrLabel);
        
        if (_at.highlight) {
            gtk_widget_grab_default(buttonOrLabel);
        }

        if (button_text[0] == '#') {
            GtkWidget *button_image = gtk_image_new_from_file(aw_str_2_label(button_text, this));
            gtk_button_set_image(GTK_BUTTON(buttonOrLabel), button_image);
        } else {
            gtk_button_set_label(GTK_BUTTON(buttonOrLabel), aw_str_2_label(button_text, this));
        }
        // _at.shadow_thickness
    } 
    else { // this 'button' can't be clicked 
        if (button_text[0] == '#') {
            buttonOrLabel = gtk_image_new_from_file(aw_str_2_label(button_text, this));
        } else {
            buttonOrLabel = gtk_label_new(aw_str_2_label(button_text, this));
        }
         //args.add(XmNalignment, (org_correct_for_at_center == 1) ? XmALIGNMENT_CENTER : XmALIGNMENT_BEGINNING);
    }
    // args.add(XmNfontList,   (XtArgVal)p_global->fontlist);
    // _at.background_color);
     
    GtkWidget *labelWidget = NULL;
    if (_at.label_for_inputfield) {
        if (_at.label_for_inputfield[0] == '#') {
            labelWidget = gtk_image_new_from_file(aw_str_2_label(_at.label_for_inputfield, this));
        } else {
            labelWidget = gtk_label_new(aw_str_2_label(_at.label_for_inputfield, this));
        }
    }
        
    // determine natural sizes
    GtkRequisition requisition;
    gtk_widget_size_request(GTK_WIDGET(buttonOrLabel), &requisition);
    int width_of_button = requisition.width;
    int height_of_button = requisition.height;
    int width_of_label = 0, height_of_label = 0, width_of_label_and_spacer = 0;
    if (labelWidget) {
        gtk_widget_size_request(GTK_WIDGET(labelWidget), &requisition);
        width_of_label = requisition.width;
        height_of_label = requisition.height;
        width_of_label_and_spacer = width_of_label + SPACE_BEHIND_LABEL;
    }

    // if size specified in xfig, scale button width and height accordingly
    if (_at.to_position_exists) { // size has explicitly been specified in xfig -> calculate
        width_of_button  = _at.to_position_x - _at.x_for_next_button - width_of_label_and_spacer;
        height_of_button = _at.to_position_y - _at.y_for_next_button;
    }
    else if (_at.length_of_buttons) { // if button width specified via at(), calculate
        width_of_button  = BUTTON_TEXT_X_PADDING + calculate_string_width(_at.length_of_buttons+1);
       
        if (button_text[0] != '#') {
            // if not image button, get calculate height from configured rows
            if (_at.height_of_buttons) { // button height specified from client code
                height_of_button = BUTTON_TEXT_Y_PADDING + calculate_string_height(_at.height_of_buttons, 0);
            }
            else { // or actual rows (\n based)
                int textwidth, textheight;
                calculate_textsize(button_text, &textwidth, &textheight);
                height_of_button = BUTTON_TEXT_Y_PADDING + calculate_string_height(textheight, 0);
            }
        }
        else { // all images have one text line height if there is 'length-of-buttons' ?!
            height_of_button = BUTTON_TEXT_Y_PADDING + calculate_string_height(1, 0);
        }
    }

    // button can't be less tall than label
    if (height_of_label && height_of_button<height_of_label) {
        height_of_button = height_of_label;
    }

    gtk_widget_set_size_request(buttonOrLabel, width_of_button, height_of_button);

    // determine x/y positions 
    int x_label  = _at.x_for_next_button;
    int y_label  = _at.y_for_next_button;
    int x_button = x_label + width_of_label_and_spacer;
    int y_button = y_label;
    
    // do center/right align
    if (_at.correct_for_at_center) {
        int total_width = width_of_label_and_spacer + width_of_button;
        if (_at.correct_for_at_center == 1) total_width /= 2; // center
        x_label -= total_width;
        x_button -= total_width;
    }

    // finish up the label
    if (labelWidget) {
        gtk_fixed_put(prvt->fixed_size_area, labelWidget, x_label, y_label);
        gtk_widget_show(labelWidget);

        if (_at.attach_any) {
            // we might need to set x/y for this
            aw_attach_widget(labelWidget, _at);
        }

        AW_label_in_awar_list(this, labelWidget, _at.label_for_inputfield);
    }

    // finish up the button
    gtk_fixed_put(prvt->fixed_size_area, buttonOrLabel, x_button, y_button);
    gtk_widget_realize(buttonOrLabel);
    gtk_widget_show(buttonOrLabel);

    //if (!_at.attach_any || !prvt->_callback) args.add(XmNrecomputeSize, false);
    if (_at.attach_any) aw_attach_widget(buttonOrLabel, _at);

    if (prvt->callback) {
        get_root()->register_widget(buttonOrLabel, _at.widget_mask);
        // should we also enable/disable the label?
    }

    AW_label_in_awar_list(this, buttonOrLabel, button_text);
  
    FIXME("prvt->toggle_field not set. What is it's purpose anyway?");
   // p_w->toggle_field = button;

    this->unset_at_commands();
    _at.increment_at_commands(width_of_label_and_spacer + width_of_button + SPACE_BEHIND_BUTTON, 
                              height_of_button);
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

void AW_window::create_autosize_button(const char *macro_name, AW_label buttonlabel, 
                                       const char *mnemonic /* = 0*/, unsigned xtraSpace /* = 1 */){
    aw_assert(buttonlabel[0] != '#');    // use create_button() for graphical buttons!
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
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_window::create_toggle(const char */*var_name*/){

    GtkWidget* checkButton = gtk_check_button_new();

    GtkWidget* parent_widget = GTK_WIDGET(prvt->fixed_size_area);
    gtk_fixed_put(GTK_FIXED(parent_widget), checkButton, _at.x_for_next_button, _at.y_for_next_button);
    gtk_widget_show(checkButton);


    short height = 0;
    short width  = 0;

    if (_at.to_position_exists) {
        // size has explicitly been specified in xfig -> calculate
        height = _at.to_position_y - _at.y_for_next_button;
        width  = _at.to_position_x - _at.x_for_next_button;
    }


    if (!height || !width) {
        // ask gtk for real button size
        width = checkButton->allocation.width;
        height = checkButton->allocation.height;

    }
    _at.increment_at_commands(width + SPACE_BEHIND_BUTTON, height);
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
    GtkWidget     *textField      = 0;
    GtkWidget     *tmp_label      = 0;
    AW_cb_struct  *cbs            = 0;
    AW_varUpdateInfo *vui         = 0;
    char          *str            = 0;
    int            xoff_for_label = 0;

    if (!columns) columns = _at.length_of_buttons;

    AW_awar *vs = get_root()->awar(var_name);
    str         = get_root()->awar(var_name)->read_as_string();

    aw_assert(NULL != vs);
    
    int width_of_input_label, height_of_input_label;
    calculate_label_size(this, &width_of_input_label, &height_of_input_label, true, 0);
    // @@@ FIXME: use height_of_input_label for propper Y-adjusting of label
    // width_of_input_label = this->calculate_string_width( calculate_label_length() );

    int width_of_input = this->calculate_string_width(columns+1) + 9;
    // calculate width for 1 additional character (input field is not completely used)
    // + 4 pixel for shadow + 4 unknown missing pixels + 1 add. pixel needed for visible text area

    //Widget parentWidget = _at.attach_any ? INFO_FORM : INFO_WIDGET;

    if (_at.label_for_inputfield) {
        tmp_label = gtk_label_new(_at.label_for_inputfield);
        gtk_fixed_put(prvt->fixed_size_area, tmp_label, _at.x_for_next_button, (int)(_at.y_for_next_button) + get_root()->y_correction_for_input_labels -1);
        gtk_widget_show(tmp_label);
//        tmp_label = XtVaCreateManagedWidget("label",
//                                            xmLabelWidgetClass,
//                                            parentWidget,
//                                            XmNwidth, (int)(width_of_input_label + 2),
//                                            XmNhighlightThickness, 0,
//                                            RES_CONVERT(XmNlabelString, _at.label_for_inputfield),
//                                            XmNrecomputeSize, false,
//                                            XmNalignment, XmALIGNMENT_BEGINNING,
//                                            XmNfontList, p_global->fontlist,
//                                            (_at.attach_any) ? NULL : XmNx, (int)_at.x_for_next_button,
//                                            XmNy, (int)(_at.y_for_next_button) + get_root()->y_correction_for_input_labels -1,
//                                            NULL);
        if (_at.attach_any)
        {
            FIXME("input field label attaching not implemented");
            //aw_attach_widget(tmp_label, _at);
        }
        xoff_for_label = width_of_input_label + 10;
    }


    int width_of_last_widget = xoff_for_label + width_of_input + 2;

    if (_at.to_position_exists) {
        width_of_input = _at.to_position_x - _at.x_for_next_button - xoff_for_label + 2;
        width_of_last_widget = _at.to_position_x - _at.x_for_next_button;
    }

    {
        //TuneBackground(parentWidget, TUNE_INPUT);
        textField = gtk_entry_new();
        gtk_fixed_put(prvt->fixed_size_area, textField, (int)(_at.x_for_next_button + xoff_for_label), (int)(_at.y_for_next_button + 5) - 8);
        gtk_widget_show(textField);
//        textField = XtVaCreateManagedWidget("textField",
//                                            xmTextFieldWidgetClass,
//                                            parentWidget,
//                                            XmNwidth, (int)width_of_input,
//                                            XmNrows, 1,
//                                            XmNvalue, str,
//                                            XmNfontList, p_global->fontlist,
//                                            XmNbackground, _at.background_color,
//                                            (_at.attach_any) ? NULL : XmNx, (int)(_at.x_for_next_button + xoff_for_label),
//                                            XmNy, (int)(_at.y_for_next_button + 5) - 8,
//                                            NULL);
        if (_at.attach_any) {
            FIXME("attaching input field not implemented");
//            _at.x_for_next_button += xoff_for_label;
//            aw_attach_widget(textField, _at);
//            _at.x_for_next_button -= xoff_for_label;
        }
    }

    free(str);

    // user-own callback
    cbs = prvt->callback;

    // callback for enter
    vui = new AW_varUpdateInfo(this, textField, AW_WIDGET_INPUT_FIELD, vs, cbs);
    g_signal_connect(G_OBJECT(textField), "activate",
                     G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback),
                     (gpointer) vui);
    
    if (_d_callback) {
        g_signal_connect(G_OBJECT(textField), "activate",
                         G_CALLBACK(AW_server_callback),
                         (gpointer) _d_callback);
        _d_callback->id = GBS_global_string_copy("INPUT:%s", var_name);
        get_root()->define_remote_command(_d_callback);
    }
   
    // callback for losing focus
    g_signal_connect(G_OBJECT(textField), "focus-out-event",
                     G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback_event),
                     (gpointer) vui);  
    // callback for value changed
    g_signal_connect(G_OBJECT(textField), "changed",
                     G_CALLBACK(AW_value_changed_callback),
                     (gpointer) get_root());
    vs->tie_widget(0, textField, AW_WIDGET_INPUT_FIELD, this);

    get_root()->register_widget(textField, _at.widget_mask);
    
    int height_of_last_widget = textField->allocation.height;

    if (_at.correct_for_at_center == 1) {   // middle centered
        gtk_fixed_move(prvt->fixed_size_area, textField, (int)(_at.x_for_next_button + xoff_for_label) - (int)(width_of_last_widget/2) + 1, textField->allocation.y);
        
        if (tmp_label) {
            gtk_fixed_move(prvt->fixed_size_area, tmp_label,  ((int)(_at.x_for_next_button) - (int)(width_of_last_widget/2) + 1), tmp_label->allocation.y);
        }
        width_of_last_widget = width_of_last_widget / 2;
    }
    if (_at.correct_for_at_center == 2) {   // right centered
        gtk_fixed_move(prvt->fixed_size_area, textField, (int)(_at.x_for_next_button + xoff_for_label - width_of_last_widget + 3), textField->allocation.y);
        if (tmp_label) {
            gtk_fixed_move(prvt->fixed_size_area, tmp_label,  (int)(_at.x_for_next_button - width_of_last_widget + 3), tmp_label->allocation.y);
        }
        width_of_last_widget = 0;
    }
    width_of_last_widget -= 2;

    this->unset_at_commands();
    _at.increment_at_commands(width_of_last_widget, height_of_last_widget);

}

void AW_window::create_text_field(const char */*awar_name*/, int /*columns*/ /* = 20 */, int /*rows*/ /*= 4*/){
    GTK_NOT_IMPLEMENTED;
}


void AW_window::create_menu(AW_label name, const char *mnemonic, AW_active mask /*= AWM_ALL*/){
    aw_assert(legal_mask(mask));
        
    GTK_PARTLY_IMPLEMENTED;
    FIXME("debug code for duplicate mnemonics missing");


//    #ifdef DEBUG
//        init_duplicate_mnemonic();
//    #endif
//    #if defined(DUMP_MENU_LIST)
//        dumpCloseAllSubMenus();
//    #endif // DUMP_MENU_LIST
        
    // The user might leave sub menus open. Close them before creating a new top level menu.
    while(prvt->menus.size() > 1) {
        close_sub_menu();
    }
    
    insert_sub_menu(name, mnemonic, mask);
}

static void aw_mode_callback(AW_window *aww, long mode, AW_cb_struct *cbs) {
    aww->select_mode((int)mode);
    cbs->run_callback();
}

int AW_window::create_mode(const char *pixmap, const char *helpText, AW_active mask, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2){
    
    aw_assert(legal_mask(mask));
    aw_assert(NULL != prvt->mode_menu);
    


    const char *path = GB_path_in_ARBLIB("pixmaps", pixmap);
    GtkWidget *icon = gtk_image_new_from_file(path);
    GtkToolItem *item = GTK_TOOL_ITEM(gtk_tool_button_new(icon, NULL)); //use icon, not label
    
   
    gtk_toolbar_insert(prvt->mode_menu, item, -1); //-1 = append
    
    AW_cb_struct *cbs = new AW_cb_struct(this, f, cd1, cd2, 0);
    AW_cb_struct *cb2 = new AW_cb_struct(this, (AW_CB)aw_mode_callback, (AW_CL)prvt->number_of_modes, (AW_CL)cbs, helpText, cbs);
    
    g_signal_connect((gpointer)item, "clicked", G_CALLBACK(AW_server_callback), (gpointer)cb2);
    
    if (!prvt->modes_f_callbacks) {
        prvt->modes_f_callbacks = (AW_cb_struct **)GB_calloc(sizeof(AW_cb_struct*), AW_NUMBER_OF_F_KEYS); // valgrinders : never freed because AW_window never is freed
    }
    
    FIXME("prvt->modes_widgets not implemented. Not sure if really needed");
    
//    if (!p_w->modes_widgets) {
//        p_w->modes_widgets = (Widget *)GB_calloc(sizeof(Widget), AW_NUMBER_OF_F_KEYS);
//    }
    if (prvt->number_of_modes<AW_NUMBER_OF_F_KEYS) {
        prvt->modes_f_callbacks[prvt->number_of_modes] = cb2;
        //p_w->modes_widgets[p_w->number_of_modes] = button;
    }

    get_root()->register_widget(GTK_WIDGET(item), mask);
    prvt->number_of_modes++;

    return prvt->number_of_modes;
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
                                                     AW_label tmp_label, 
                                                     const char *){
    int x_for_position_of_menu = 10;
    FIXME("mnemonic not implemented");
    if (_at.label_for_inputfield) {
        tmp_label = _at.label_for_inputfield;
    }
    if (tmp_label && !tmp_label[0]) {
        aw_assert(0); // do not specify empty labels (causes misalignment)
        tmp_label = NULL;
    }
    _at.saved_x = _at.x_for_next_button;
    if (!tmp_label) {
      _at.saved_x -= 10;
    }

    GtkWidget *labelWidget = NULL; //contains the label, or NULL
    prvt->combo_box = gtk_hbox_new(false, 1); //This box is used to align label and combobox
    GtkWidget *cbox = gtk_combo_box_new_text();

    gtk_combo_box_set_button_sensitivity(GTK_COMBO_BOX(cbox), GTK_SENSITIVITY_ON);

    int hbox_x = 0;//where to put the hbox
    int hbox_y = 0;
    
    if (!_at.attach_x && !_at.attach_lx)
    {
        hbox_x= x_for_position_of_menu;
    }
    else
    {
        FIXME("option menu attaching not implemented");
    }
    
    if (!_at.attach_y && !_at.attach_ly)
    {
        hbox_y = _at.y_for_next_button-5;
    }
    else
    {
        FIXME("option menu attaching not implemented");
    }
    

    if (tmp_label) {
        labelWidget = gtk_label_new(tmp_label);
        if (_at.length_of_label_for_inputfield) {
          gtk_label_set_width_chars(GTK_LABEL(labelWidget), _at.length_of_label_for_inputfield);
          gtk_misc_set_alignment(GTK_MISC(labelWidget), 0.0, 0.5);
        }
        gtk_box_pack_start(GTK_BOX(prvt->combo_box), labelWidget, false, false, 0);
    }

    gtk_box_pack_start(GTK_BOX(prvt->combo_box), cbox, false, false, 0);    
    gtk_fixed_put(prvt->fixed_size_area, prvt->combo_box, 
                  _at.x_for_next_button, _at.y_for_next_button);
    
    get_root()->number_of_option_menus++;

    AW_awar *vs = get_root()->awar(awar_name);

    AW_option_menu_struct *next =
        new AW_option_menu_struct(get_root()->number_of_option_menus,
                                  awar_name,
                                  vs->variable_type,
                                  labelWidget,
                                  cbox,
                                  _at.x_for_next_button - 7,
                                  _at.y_for_next_button,
                                  _at.correct_for_at_center);

    if (get_root()->option_menu_list) {
        get_root()->last_option_menu->next = next;
        get_root()->last_option_menu = get_root()->last_option_menu->next;
    }
    else {
        get_root()->last_option_menu = get_root()->option_menu_list = next;
    }

    get_root()->current_option_menu = get_root()->last_option_menu;

    vs->tie_widget((AW_CL)get_root()->current_option_menu, cbox, AW_WIDGET_CHOICE_MENU, this);
    
    //connect changed signal
    g_signal_connect(G_OBJECT(cbox), "changed", G_CALLBACK(AW_combo_box_changed), (gpointer) next);
   
    get_root()->register_widget(prvt->combo_box, _at.widget_mask);
    GtkRequisition requisition;
    gtk_widget_size_request(GTK_WIDGET(prvt->combo_box), &requisition);
    unset_at_commands();
    _at.increment_at_commands(requisition.width, requisition.height);
    
    return get_root()->current_option_menu;
}

void AW_window::insert_option(AW_label on, const char *mn, const char *vv, const char *noc) {
        insert_option_internal(on, mn, vv, noc, false); 
}

void AW_window::insert_default_option(AW_label on, const char *mn, const char *vv, const char *noc) { 
    insert_option_internal(on, mn, vv, noc, true); 

}
void AW_window::insert_option(AW_label on, const char *mn, int vv, const char *noc) { 
    insert_option_internal(on, mn, vv, noc, false);
}

void AW_window::insert_default_option(AW_label on, const char *mn, int vv, const char *noc) { 
    insert_option_internal(on, mn, vv, noc, true); 

}
void AW_window::insert_option(AW_label on, const char *mn, float vv, const char *noc) { 
    insert_option_internal(on, mn, vv, noc, false); 
}

void AW_window::insert_default_option(AW_label on, const char *mn, float vv, const char *noc) { 
    insert_option_internal(on, mn, vv, noc, true); 
}


template <class T>
void AW_window::insert_option_internal(AW_label option_name, const char *mnemonic, T var_value, const char *name_of_color, bool default_option) {
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
    GtkRequisition requisition;
    gtk_widget_show_all(prvt->combo_box);
    gtk_widget_size_request(GTK_WIDGET(prvt->combo_box), &requisition);
    unset_at_commands();
    // no clue where the +10 comes from
    _at.increment_at_commands(requisition.width + 10, requisition.height);
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


AW_selection_list* AW_window::create_selection_list(const char *var_name, int columns, int rows) {

    GTK_PARTLY_IMPLEMENTED;
    GtkListStore *store;
    GtkWidget *tree;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    AW_awar *vs = 0;
    
    aw_assert(var_name); // @@@ case where var_name == NULL is relict from multi-selection-list (not used; removed)
    vs = get_root()->awar(var_name);
    
    tree = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes("List Items",
             renderer, "text", 0, NULL); //column headers are disabled, text does not matter
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    store = gtk_list_store_new(1, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));

    GtkWidget *scrolled_win = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_win),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_win), tree);

    AW_varUpdateInfo *vui = 0;
    AW_cb_struct  *cbs = 0;
    
    int width_of_list;
    int height_of_list;
    int width_of_last_widget  = 0;
    int height_of_last_widget = 0;

    aw_assert(!_at.label_for_inputfield); // labels have no effect for selection lists

    width_of_list  = this->calculate_string_width(columns) + 9;
    height_of_list = this->calculate_string_height(rows, 4*rows) + 9;
    gtk_widget_set_usize(scrolled_win, width_of_list, height_of_list);
    
    if (_at.to_position_exists) {
        width_of_list = _at.to_position_x - _at.x_for_next_button - 18;
        if (_at.y_for_next_button  < _at.to_position_y - 18) {
            height_of_list = _at.to_position_y - _at.y_for_next_button - 18;
        }
        gtk_widget_set_usize(scrolled_win, width_of_list, height_of_list);
        FIXME("Attaching list widget not implemented");
        gtk_fixed_put(prvt->fixed_size_area, scrolled_win, 
                      _at.x_for_next_button, _at.y_for_next_button);

        width_of_last_widget = _at.to_position_x - _at.x_for_next_button;
        height_of_last_widget = _at.to_position_y - _at.y_for_next_button;
    }
    else {
        gtk_fixed_put(prvt->fixed_size_area, scrolled_win, 10, _at.y_for_next_button);
    }

    {
        
        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
        if(NULL == vs) {
            gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);
        }
        else
        {
            gtk_tree_selection_set_mode(selection, GTK_SELECTION_BROWSE);
        }
    }

    if (!_at.to_position_exists) {
        FIXME("selection list alignment not implemented");
//        short height;
//        XtVaGetValues(scrolledList, XmNheight, &height, NULL);
//        height_of_last_widget = height + 20;
//        width_of_last_widget  = width_of_list + 20;
//
//        switch (_at.correct_for_at_center) {
//            case 3: break;
//            case 0: // left aligned
//                XtVaSetValues(scrolledWindowList, XmNx, (int)(_at.x_for_next_button), NULL);
//                break;
//
//            case 1: // center aligned
//                XtVaSetValues(scrolledWindowList, XmNx, (int)(_at.x_for_next_button - (width_of_last_widget/2)), NULL);
//                width_of_last_widget = width_of_last_widget / 2;
//                break;
//
//            case 2: // right aligned
//                XtVaSetValues(scrolledWindowList, XmNx, (int)(_at.x_for_next_button - width_of_list - 18), NULL);
//                width_of_last_widget = 0;
//                break;
        }

    {
        int type = GB_STRING;
        if (vs)  type = vs->variable_type;

        get_root()->append_selection_list(new AW_selection_list(var_name, type, GTK_TREE_VIEW(tree)));
    }


    // user-own callback
    cbs = prvt->callback;

    
    // callback for enter
    if (vs) {
        vui = new AW_varUpdateInfo(this, tree, AW_WIDGET_SELECTION_LIST, vs, cbs);
        vui->set_sellist(get_root()->get_last_selection_list());

        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
        g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback), (gpointer) vui);

        FIXME("double click callback not implemented");
//        if (_d_callback) {
//            XtAddCallback(scrolledList, XmNdefaultActionCallback,
//                          (XtCallbackProc) AW_server_callback,
//                          (XtPointer) _d_callback);
//        }
        vs->tie_widget((AW_CL)get_root()->get_last_selection_list(), tree, AW_WIDGET_SELECTION_LIST, this);
        get_root()->register_widget(tree, _at.widget_mask);
    }

    this->unset_at_commands();
    _at.increment_at_commands(width_of_last_widget, height_of_last_widget);
    
    gtk_widget_show(scrolled_win);
    
    return get_root()->get_last_selection_list();
}

// BEGIN TOGGLE FIELD STUFF

void AW_window::create_toggle_field(const char *awar_name, int orientation /*= 0*/){
    // orientation = 0 -> vertical else horizontal layout

    GtkWidget *parent_widget = GTK_WIDGET(prvt->fixed_size_area);
    GtkWidget *label_for_toggle;

    int xoff_for_label           = 0;
    int width_of_label           = 0;
    int x_for_position_of_option = 0;

    const char *tmp_label = "";

    if (_at.label_for_inputfield) {
        tmp_label = _at.label_for_inputfield;
    }

    if (_at.correct_for_at_center) {
        _at.saved_x = _at.x_for_next_button;
        x_for_position_of_option = 10;
    }
    else {
        x_for_position_of_option = _at.x_for_next_button;
    }


    if (tmp_label) {
        int height_of_label;
        calculate_label_size(this, &width_of_label, &height_of_label, true, tmp_label);
        // @@@ FIXME: use height_of_label for Y-alignment
        // width_of_label = this->calculate_string_width( this->calculate_label_length() );
        label_for_toggle = gtk_label_new(tmp_label);
        gtk_fixed_put(GTK_FIXED(parent_widget), label_for_toggle, 
                      _at.x_for_next_button, 
                      _at.y_for_next_button + get_root()->y_correction_for_input_labels);
        
        _at.saved_xoff_for_label = xoff_for_label = width_of_label + 10;
        // prvt->toggle_label = label_for_toggle;
    }
    else {
        _at.saved_xoff_for_label = 0;
        // prvt->toggle_label = NULL;
    }

    if (orientation == 0) {
      prvt->toggle_field = gtk_vbox_new(true, 2);
    } 
    else {
      prvt->toggle_field = gtk_hbox_new(true, 2);
    }
    
    if (_at.attach_any) {
        FIXME("Attaching toggle fields not implemented");
       // aw_attach_widget(toggle_field, _at, 300);
    }
    
    prvt->toggle_field_awar_name = awar_name;
    
    get_root()->register_widget(prvt->toggle_field, _at.widget_mask);
}

void AW_window::create_toggle_field(const char *var_name, AW_label labeli, const char */*mnemonic*/) {
    if (labeli) this->label(labeli);
    create_toggle_field(var_name);
}

template <class T>
void AW_window::insert_toggle_internal(AW_label toggle_label, const char *mnemonic, T var_value, bool default_toggle) {
    GtkWidget *radio;  
    std::string mnemonicLabel = AW_motif_gtk_conversion::convert_mnemonic(toggle_label, mnemonic);
    // create and chain radio button
    radio = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(prvt->radio_last), mnemonicLabel.c_str());
    prvt->radio_last = radio;
    
    gtk_box_pack_start(GTK_BOX(prvt->toggle_field), radio, true, true, 2);
    
    AW_varUpdateInfo *vui = new AW_varUpdateInfo(this, NULL, AW_WIDGET_TOGGLE_FIELD, get_root()->awar(prvt->toggle_field_awar_name), var_value, prvt->callback);
    g_signal_connect((gpointer)radio, "clicked", G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback), (gpointer)vui);
    
    if(default_toggle){
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio), true);
    }
}

void AW_window::insert_toggle        (AW_label toggle_label, const char *mnemonic, const char *var_value) { insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(AW_label toggle_label, const char *mnemonic, const char *var_value) { insert_toggle_internal(toggle_label, mnemonic, var_value, true); }
void AW_window::insert_toggle        (AW_label toggle_label, const char *mnemonic, int var_value)           { insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(AW_label toggle_label, const char *mnemonic, int var_value)           { insert_toggle_internal(toggle_label, mnemonic, var_value, true); }
void AW_window::insert_toggle        (AW_label toggle_label, const char *mnemonic, float var_value)         { insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(AW_label toggle_label, const char *mnemonic, float var_value)         { insert_toggle_internal(toggle_label, mnemonic, var_value, true); }


void AW_window::update_toggle_field() { 
    GtkWidget *parent_widget = GTK_WIDGET(prvt->fixed_size_area);
    gtk_fixed_put(GTK_FIXED(parent_widget), prvt->toggle_field, 
                      _at.x_for_next_button + _at.saved_xoff_for_label, 
                      _at.y_for_next_button);
    gtk_widget_show_all(prvt->toggle_field);
    prvt->radio_last = NULL; // end of radio group
    prvt->toggle_field = NULL;
    unset_at_commands();
}
void AW_window::refresh_toggle_field(int /*toggle_field_number*/) {
    GTK_NOT_IMPLEMENTED;
//#if defined(DEBUG)
//    static int inside_here = 0;
//    aw_assert(!inside_here);
//    inside_here++;
//#endif // DEBUG
//
//    AW_toggle_field_struct *toggle_field_list = p_global->toggle_field_list;
//    {
//        while (toggle_field_list) {
//            if (toggle_field_number == toggle_field_list->toggle_field_number) {
//                break;
//            }
//            toggle_field_list = toggle_field_list->next;
//        }
//    }
//
//    if (toggle_field_list) {
//        AW_widget_value_pair *active_toggle = toggle_field_list->first_toggle;
//        {
//            AW_scalar global_value(get_root()->awar(toggle_field_list->variable_name));
//            while (active_toggle && active_toggle->value != global_value) {
//                active_toggle = active_toggle->next;
//            }
//            if (!active_toggle) active_toggle = toggle_field_list->default_toggle;
//        }
//
//        // iterate over all toggles including default_toggle and set their state
//        for (AW_widget_value_pair *toggle = toggle_field_list->first_toggle; toggle;) {
//            XmToggleButtonSetState(toggle->widget, toggle == active_toggle, False);
//
//            if (toggle->next)                                     toggle = toggle->next;
//            else if (toggle != toggle_field_list->default_toggle) toggle = toggle_field_list->default_toggle;
//            else                                                  toggle = 0;
//        }
//
//        // @@@ code below should go to update_toggle_field
//        {
//            short length;
//            short height;
//            XtVaGetValues(p_w->toggle_field, XmNwidth, &length, XmNheight, &height, NULL);
//            length                += (short)_at->saved_xoff_for_label;
//
//            int width_of_last_widget  = length;
//            int height_of_last_widget = height;
//
//            if (toggle_field_list->correct_for_at_center_intern) {
//                if (toggle_field_list->correct_for_at_center_intern == 1) {   // middle centered
//                    XtVaSetValues(p_w->toggle_field, XmNx, (short)((short)_at->saved_x - (short)(length/2) + (short)_at->saved_xoff_for_label), NULL);
//                    if (p_w->toggle_label) {
//                        XtVaSetValues(p_w->toggle_label, XmNx, (short)((short)_at->saved_x - (short)(length/2)), NULL);
//                    }
//                    width_of_last_widget = width_of_last_widget / 2;
//                }
//                if (toggle_field_list->correct_for_at_center_intern == 2) {   // right centered
//                    XtVaSetValues(p_w->toggle_field, XmNx, (short)((short)_at->saved_x - length + (short)_at->saved_xoff_for_label), NULL);
//                    if (p_w->toggle_label) {
//                        XtVaSetValues(p_w->toggle_label, XmNx, (short)((short)_at->saved_x - length),    NULL);
//                    }
//                    width_of_last_widget = 0;
//                }
//            }
//
//            this->unset_at_commands();
//            _at.increment_at_commands(width_of_last_widget, height_of_last_widget);
//        }
//    }
//    else {
//        GBK_terminatef("update_toggle_field: toggle field %i does not exist", toggle_field_number);
//    }
//
//#if defined(DEBUG)
//    inside_here--;
//#endif // DEBUG
}

// END TOGGLE FIELD STUFF


void AW_window::d_callback(void (*/*f*/)(AW_window*, AW_CL), AW_CL /*cd1*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::d_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/){
    GTK_NOT_IMPLEMENTED;
}


AW_pos AW_window::get_scrolled_picture_width() const {
    return (picture->r - picture->l);
}

AW_pos AW_window::get_scrolled_picture_height() const {
    return (picture->b - picture->t);
}

void AW_window::set_horizontal_scrollbar_left_indent(int /*indent*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_vertical_scrollbar_top_indent(int /*indent*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::select_mode(int /*mode*/) {
    GTK_NOT_IMPLEMENTED;
}

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

void AW_window::get_window_size(int& /*width*/, int& /*height*/){
    GTK_NOT_IMPLEMENTED;
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




void AW_window::insert_help_topic(AW_label name, const char */*mnemonic*/, const char */*help_text_*/, AW_active /*mask*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/){
    aw_assert(NULL != prvt->help_menu);
    
    FIXME("mnemonic not working");
    FIXME("help entry callback not working");
    FIXME("Help text not working");
    GtkWidget *item = gtk_menu_item_new_with_label(name);
    gtk_menu_shell_append(prvt->help_menu, item);
    
        
}


void AW_window::insert_menu_topic(const char *topic_id, AW_label name, const char *mnemonic, const char *helpText, AW_active mask, AW_CB f, AW_CL cd1, AW_CL cd2){
   aw_assert(legal_mask(mask));
  
   FIXME("duplicate mnemonic test not implemented");

   std::string topicName = AW_motif_gtk_conversion::convert_mnemonic(name, mnemonic);
   
   if (!topic_id) topic_id = name; // hmm, due to this we cannot insert_menu_topic w/o id. Change? @@@
   
   GtkWidget *item = gtk_menu_item_new_with_mnemonic(topicName.c_str());
   aw_assert(prvt->menus.size() > 0); //closed too many menus

   gtk_menu_shell_append(prvt->menus.top(), item);  
   
#if defined(DUMP_MENU_LIST)
 //   dumpMenuEntry(name);
#endif // DUMP_MENU_LIST
#ifdef DEBUG
//    test_duplicate_mnemonics(prvt.menu_deep, name, mnemonic);
#endif


    AW_label_in_awar_list(this, item, name);
    AW_cb_struct *cbs = new AW_cb_struct(this, f, cd1, cd2, helpText);
    
    
    g_signal_connect((gpointer)item, "activate", G_CALLBACK(AW_server_callback), (gpointer)cbs);

    cbs->id = strdup(topic_id);
    get_root()->define_remote_command(cbs);
    get_root()->register_widget(item, mask);
}



void AW_window::insert_sub_menu(AW_label name, const char *mnemonic, AW_active mask /*= AWM_ALL*/){
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
    
    //use the new submenu as current menu shell.
    prvt->menus.push(GTK_MENU_SHELL(submenu));
    
    
    FIXME("duplicate mnemonic test not implemented");
    #if defined(DUMP_MENU_LIST)
        dumpOpenSubMenu(name);
    #endif // DUMP_MENU_LIST
    #ifdef DEBUG
       // open_test_duplicate_mnemonics(prvt->menu_deep+1, name, mnemonic);
    #endif

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

    // The INFO_AREA must be gtk_fixed or the window itself for both
    // the buttons and the lines/text from xfig to show. For some
    // reason, button placement is relative to the widget origin,
    // while lines/text are placed relative to the window origin. 
    // Possibly, this is because the window hasn't been 'exposed' 
    // at that time yet, whereas we are now at expose time.
    // Whatever the cause, if the INFO_AREA is not placed exactly
    // at the origin of the window, lines and text need to be
    // rendered with offset to appear in the right places. So
    // that's what we do find out here:
    GtkAllocation allocation;
    GtkWidget *area = aww->get_area(AW_INFO_AREA)->get_area();
    gtk_widget_get_allocation(area, &allocation); 
    
    device->set_offset(AW::Vector(-xfig->minx + allocation.x, 
                                  -xfig->miny + allocation.y));
    xfig->print(device);
}

void AW_window::load_xfig(const char *file, bool resize /*= true*/){
    if (file)   xfig_data = new AW_xfig(file, get_root()->font_width, get_root()->font_height);
    else        xfig_data = new AW_xfig(get_root()->font_width, get_root()->font_height); // create an empty xfig

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



void AW_window::restore_at_size_and_attach(const AW_at_size */*at_size*/){
    GTK_NOT_IMPLEMENTED;
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

static void value_changed_scroll_bar_horizontal(GtkAdjustment *adjustment, gpointer user_data){
    AW_cb_struct *cbs = (AW_cb_struct *) user_data;
    (cbs->aw)->slider_pos_horizontal = gtk_adjustment_get_value(adjustment);
    cbs->run_callback();
}

void AW_window::set_horizontal_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {
    g_signal_connect((gpointer)prvt->hAdjustment, "value_changed",
                     G_CALLBACK(value_changed_scroll_bar_horizontal),
                     (gpointer)new AW_cb_struct(this, f, cd1, cd2, ""));
}

void AW_window::set_horizontal_scrollbar_position(int position) {
#if defined(DEBUG) && 0
    fprintf(stderr, "set_horizontal_scrollbar_position to %i\n", position);
#endif
    // @@@ test and constrain against limits
    slider_pos_horizontal = position;
    gtk_adjustment_set_value(prvt->hAdjustment, position);

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

static void value_changed_scroll_bar_vertical(GtkAdjustment *adjustment, gpointer user_data){
    AW_cb_struct *cbs = (AW_cb_struct *) user_data;
    cbs->aw->slider_pos_vertical = gtk_adjustment_get_value(adjustment);
    cbs->run_callback();
    
}

void AW_window::set_vertical_change_callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2) {

    g_signal_connect((gpointer)prvt->vAdjustment, "value_changed",
                     G_CALLBACK(value_changed_scroll_bar_vertical),
                     (gpointer)new AW_cb_struct(this, f, cd1, cd2, ""));

}

void AW_window::set_vertical_scrollbar_position(int position){
#if defined(DEBUG) && 0
    fprintf(stderr, "set_vertical_scrollbar_position to %i\n", position);
#endif
    // @@@ test and constrain against limits
    slider_pos_vertical = position;
    gtk_adjustment_set_value(prvt->vAdjustment, position);
}

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
    arb_assert(NULL != prvt->window);
    gtk_widget_show_all(GTK_WIDGET(prvt->window));
}

void AW_window::button_height(int height) {
    _at.height_of_buttons = height>1 ? height : 0; 
}

void AW_window::store_at_size_and_attach(AW_at_size *at_size) {
    at_size->store(_at);
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

void AW_window::tell_scrolled_picture_size(AW_world rectangle) {
    picture->l = (int)rectangle.l;
    picture->r = (int)rectangle.r;
    picture->t = (int)rectangle.t;
    picture->b = (int)rectangle.b;
}



bool AW_window::is_resize_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL)) {
    AW_area_management *aram = prvt->areas[area];
    return aram && aram->is_resize_callback(this, f);
}

void AW_window::update_label(GtkWidget* widget, const char *var_value) {
    
    if (get_root()->changer_of_variable != widget) {
        FIXME("this will break if the labels content should switch from text to image or vice versa");
        FIXME("only works for labels and buttons right now");
        
        if(GTK_IS_BUTTON(widget)) {
            gtk_button_set_label(GTK_BUTTON(widget), var_value);   
        }
        else if(GTK_IS_LABEL(widget)) {
            gtk_label_set_text(GTK_LABEL(widget), var_value);
        }
        else {
            aw_assert(false); //unable to set label of unknown type
        }
        
        //old implementation
        //XtVaSetValues(widget, RES_CONVERT(XmNlabelString, var_value), NULL);
        
    }
    else {
        get_root()->changer_of_variable = 0;
    }
}



void AW_window::window_fit() {
    GTK_NOT_IMPLEMENTED;
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

void AW_window::tell_scrolled_picture_size(AW_screen_area rectangle) {
    picture->l = rectangle.l;
    picture->r = rectangle.r;
    picture->t = rectangle.t;
    picture->b = rectangle.b;
}

void AW_window::d_callback(void (*/*f*/)(AW_window*)) {
    GTK_NOT_IMPLEMENTED;
}

AW_window::AW_window() 
  : recalc_size_at_show(AW_KEEP_SIZE),
    prvt(new AW_window::AW_window_gtk()),
    _at(this),
    _d_callback(NULL),
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
    for(int i = 0; i < AW_MAX_AREA; i++ ) {
        prvt->areas.push_back(NULL);
    }
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



void AW_window::reset_scrolled_picture_size() {
    picture->l = 0;
    picture->r = 0;
    picture->t = 0;
    picture->b = 0;
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

static void horizontal_scrollbar_redefinition_cb(AW_root*, AW_CL cd) {
    AW_window *aw = (AW_window *)cd;
    aw->update_scrollbar_settings_from_awars(AW_HORIZONTAL);
}

static void vertical_scrollbar_redefinition_cb(AW_root*, AW_CL cd) {
    AW_window *aw = (AW_window *)cd;
    aw->update_scrollbar_settings_from_awars(AW_VERTICAL);
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

void AW_window::update_scrollbar_settings_from_awars(AW_orientation orientation) {
    AW_screen_area scrolled;
    get_scrollarea_size(&scrolled);

   char buffer[200];
    if (orientation == AW_HORIZONTAL) {
        sprintf(buffer, "window/%s/horizontal_page_increment", window_defaults_name);   
        int page_increment = scrolled.r * get_root()->awar(buffer)->read_int() / 100;
        gtk_adjustment_set_page_increment(prvt->hAdjustment, page_increment);

        sprintf(buffer, "window/%s/scroll_width_horizontal", window_defaults_name);
        int step_increment = get_root()->awar(buffer)->read_int();
        gtk_adjustment_set_step_increment(prvt->hAdjustment, step_increment);
    }
    else {
        sprintf(buffer, "window/%s/vertical_page_increment", window_defaults_name);   
        int page_increment = scrolled.b * get_root()->awar(buffer)->read_int() / 100;
        gtk_adjustment_set_page_increment(prvt->vAdjustment, page_increment);

        sprintf(buffer, "window/%s/scroll_width_vertical", window_defaults_name);
        int step_increment = get_root()->awar(buffer)->read_int();
        gtk_adjustment_set_step_increment(prvt->vAdjustment, step_increment);
    }
}

void AW_window::create_font_button(const char* /*awar_name*/, AW_label /*label_*/) {
  GTK_NOT_IMPLEMENTED;
}

void AW_window::create_color_button(const char* /*awar_name*/, AW_label /*label_*/) {
  GTK_NOT_IMPLEMENTED;
}

AW_xfig* AW_window::get_xfig_data() {
    return xfig_data;
}
