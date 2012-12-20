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
#include "AW_area_management.hxx"
#include "aw_xfig.hxx" 
#include "aw_root.hxx"
#include "devices/aw_device_click.hxx"  
#include "devices/aw_device_size.hxx"
#include "aw_at.hxx"
#include "aw_msg.hxx"
#include "aw_awar.hxx"
#include "aw_common.hxx"
#include "AW_motif_gtk_conversion.hxx"
#include "AW_modal.hxx"
#include "aw_help.hxx"
#include "aw_varUpdateInfo.hxx" 

#ifndef ARBDB_H
#include <arbdb.h>
#endif

#include <gtk/gtklabel.h>
#include <gtk/gtkfixed.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkhbox.h>

#include <vector>
#include <stack>
#include <gtk-2.0/gtk/gtkmenuitem.h>
#include <gtk-2.0/gtk/gtkseparatormenuitem.h>
#include <gtk-2.0/gtk/gtktoolbar.h>



const int AW_NUMBER_OF_F_KEYS = 20;

/**
 * This class hides all private or gtk dependent attributes.
 * This is done to avoid gtk includes in the header file.
 */
class AW_window::AW_window_gtk {
public:
    
    GtkWindow *window; /**< The gtk window instance managed by this aw_window */
    
    /**
     *  A fixed size widget spanning the whole window or a part of the window. Everything is positioned on this widget using absolut coordinates.
     * @note This area might not be present in every window.
     * @note This area is the same as the INFO_AREA
     */
    GtkFixed *fixed_size_area; 
    
    /**
     * A menu bar at the top of the window.
     * This may not be present in all windows. Check for NULL before use
     */
    GtkMenuBar *menu_bar;
    
    /**
     * Used to keep track of the menu structure while creating menus.
     * Empty if menu_bar is NULL.
     */
    std::stack<GtkMenuShell*> menus;
    
    /**
     * The help menu. Might not exist in some windows. Check for NULL before use.
     */
    GtkMenuShell *help_menu;
    
    /**
     * The mode menu. Might not exist in some windows. Check for NULL before use.
     */
    GtkToolbar *mode_menu;
  

    /**
     * A window consists of several areas.
     * Some of those areas are named, some are unnamed.
     * The unnamed areas are instantiated once and never changed, therefore no references to unnamed areas exist.
     * Named areas are instantiated depending on the window type.
     *
     * This vector contains references to the named areas.
     * The AW_Area enum is used to index this vector.
     */
    std::vector<AW_area_management *> areas;
    
    /**
     * This is a counter for the number of items in the mode_menu.
     * It only exists to satisfy the old interface of create_mode()
     */
    int number_of_modes;
    
    /**
     * Callback struct for the currently open popup
     */
    AW_cb_struct  *popup_cb;
    
    /**
     TODO comment
     */
    AW_cb_struct *focus_cb;
    
    /*
     * List of callbacks for the different mode buttons
     */
    AW_cb_struct **modes_f_callbacks;
    
    AW_window_gtk() : window(NULL), fixed_size_area(NULL), menu_bar(NULL), help_menu(NULL),
                      mode_menu(NULL), number_of_modes(0), popup_cb(NULL), focus_cb(NULL),
                      modes_f_callbacks(NULL){}

    
};


AW_buttons_struct::AW_buttons_struct(AW_active maski, GtkWidget* w, AW_buttons_struct *prev_button) {
    aw_assert(w);
    aw_assert(legal_mask(maski));

    mask     = maski;
    button   = w;
    next     = prev_button;
}

AW_buttons_struct::~AW_buttons_struct() {
    delete next;
}

void AW_clock_cursor(AW_root *) {
    GTK_NOT_IMPLEMENTED;
}

void AW_normal_cursor(AW_root *) {
    GTK_NOT_IMPLEMENTED;
}

void AW_help_entry_pressed(AW_window *window) {
    aw_assert(NULL != window);
    //Help mode is global. It is managed by aw_root.
    //This method should be part of aw_root, not aw_window.
    FIXME("This method should be moved to aw_root.");
    AW_root* pRoot = window->get_root();
    aw_assert(NULL != pRoot);
    
    pRoot->set_help_active(true);
    
    pRoot->help_cursor();
}

void AW_POPDOWN(AW_window *window){
    window->hide();
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

void AW_window::at(int x, int y){
    at_x(x);
    at_y(y);
}

void AW_window::at_x(int x){
    if (_at.x_for_next_button > _at.max_x_size) _at.max_x_size = _at.x_for_next_button;
    _at.x_for_next_button = x;
    if (_at.x_for_next_button > _at.max_x_size) _at.max_x_size = _at.x_for_next_button;
}

void AW_window::at_y(int y){
    if (_at.y_for_next_button + _at.biggest_height_of_buttons > _at.max_y_size)
        _at.max_y_size = _at.y_for_next_button + _at.biggest_height_of_buttons;
    _at.biggest_height_of_buttons = _at.biggest_height_of_buttons + _at.y_for_next_button - y;
    if (_at.biggest_height_of_buttons<0) {
        _at.biggest_height_of_buttons = 0;
        if (_at.max_y_size < y)    _at.max_y_size = y;
    }
    _at.y_for_next_button = y;
}

void AW_window::at_shift(int x, int y){
    at(x + _at.x_for_next_button, y + _at.y_for_next_button);
}

void AW_window::at_newline(){
    if (_at.do_auto_increment) {
        at_y(_at.auto_increment_y + _at.y_for_next_button);
    }
    else {
        if (_at.do_auto_space) {
            at_y(_at.y_for_next_button + _at.auto_space_y + _at.biggest_height_of_buttons);
        }
        else {
            GBK_terminate("neither auto_space nor auto_increment activated while using at_newline");
        }
    }
    at_x(_at.x_for_newline);
}


void AW_window::at(const char *at_id) {
    char to_position[100];
    memset(to_position, 0, sizeof(to_position));

    _at.attach_y   = _at.attach_x = false;
    _at.attach_ly  = _at.attach_lx = false;
    _at.attach_any = false;

    if (!xfig_data) GBK_terminatef("no xfig-data loaded, can't position at(\"%s\")", at_id);

    AW_xfig     *xfig = xfig_data;
    AW_xfig_pos *pos;

    pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, at_id);

    if (!pos) {
        sprintf(to_position, "X:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at.attach_any = _at.attach_lx = true;
    }
    if (!pos) {
        sprintf(to_position, "Y:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at.attach_any = _at.attach_ly = true;
    }
    if (!pos) {
        sprintf(to_position, "XY:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at.attach_any = _at.attach_lx = _at.attach_ly = true;
    }

    if (!pos) GBK_terminatef("ID '%s' does not exist in xfig file", at_id);

    at((pos->x - xfig->minx), (pos->y - xfig->miny - this->get_root()->font_height - 9));
    _at.correct_for_at_center = pos->center;

    sprintf(to_position, "to:%s", at_id);
    pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);

    if (!pos) {
        sprintf(to_position, "to:X:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at.attach_any = _at.attach_x = true;
    }
    if (!pos) {
        sprintf(to_position, "to:Y:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at.attach_any = _at.attach_y = true;
    }
    if (!pos) {
        sprintf(to_position, "to:XY:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at.attach_any = _at.attach_x = _at.attach_y = true;
    }

    if (pos) {
        _at.to_position_exists = true;
        _at.to_position_x = (pos->x - xfig->minx);
        _at.to_position_y = (pos->y - xfig->miny);
        _at.correct_for_at_center = 0; // always justify left when a to-position exists
    }
    else {
        _at.to_position_exists = false;
    }
}



bool AW_window::at_ifdef(const  char * /*id*/){
    GTK_NOT_IMPLEMENTED;
    return false;
}


void AW_window::at_set_to(bool attach_x, bool attach_y, int xoff, int yoff) {
    _at.attach_any = attach_x || attach_y;
    _at.attach_x   = attach_x;
    _at.attach_y   = attach_y;

    _at.to_position_exists = true;
    _at.to_position_x      = xoff >= 0 ? _at.x_for_next_button + xoff : _at.max_x_size+xoff;
    _at.to_position_y      = yoff >= 0 ? _at.y_for_next_button + yoff : _at.max_y_size+yoff;

    if (_at.to_position_x > _at.max_x_size) _at.max_x_size = _at.to_position_x;
    if (_at.to_position_y > _at.max_y_size) _at.max_y_size = _at.to_position_y;

    _at.correct_for_at_center = 0;
}

void AW_window::at_unset_to(){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::at_set_min_size(int /*xmin*/, int /*ymin*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::auto_space(int x, int y){
    _at.do_auto_space = true;
    _at.auto_space_x  = x;
    _at.auto_space_y  = y;
    
    _at.do_auto_increment = false;

    _at.x_for_newline = _at.x_for_next_button;
    _at.biggest_height_of_buttons = 0;
}


void AW_window::label_length(int /*length*/){
    GTK_NOT_IMPLEMENTED;
}
void AW_window::button_length(int length) {
    _at.length_of_buttons = length; 
}

int  AW_window::get_button_length() const {
    return _at.length_of_buttons; 
}

void AW_window::calculate_scrollbars(){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::callback(void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1, AW_CL cd2){
    _callback = new AW_cb_struct(this, (AW_CB)f, cd1, cd2);
}

void AW_window::callback(void (*f)(AW_window*)){
    _callback = new AW_cb_struct(this, (AW_CB)f);
}


void AW_window::callback(void (*f)(AW_window*, AW_CL), AW_CL cd1){
    FIXME("Callback newed every time, possible memory leak");
    _callback = new AW_cb_struct(this, (AW_CB)f, cd1);
}

void AW_window::callback(AW_cb_struct * /* owner */ awcbs) {
    _callback = awcbs;
}


void AW_window::close_sub_menu(){
    arb_assert(NULL != prvt);
    arb_assert(prvt->menus.size() > 1);
    
    prvt->menus.pop();
    
}



void AW_window::_get_area_size(AW_area /*area*/, AW_screen_area */*square*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::clear_option_menu(AW_option_menu_struct */*oms*/) {
    GTK_NOT_IMPLEMENTED;
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

//TODO this method was originally defined in AW_button.cxx.
//TODO I think this method should be transformed into a private member
#define MAX_LINE_LENGTH 200
__ATTR__USERESULT static GB_ERROR detect_bitmap_size(const char *pixmapname, int *width, int *height) {
    GB_ERROR err = 0;

    *width  = 0;
    *height = 0;

    char *path = pixmapPath(pixmapname);
    FILE *in   = fopen(path, "rt");
    if (in) {
        const char *subdir = strrchr(pixmapname, '/');
        char       *name   = strdup(subdir ? subdir+1 : pixmapname);
        {
            char *dot       = strrchr(name, '.');
            if (dot) dot[0] = 0;
            else  err       = "'.' expected";
        }
        int  namelen = strlen(name);
        char buffer[MAX_LINE_LENGTH];
        bool done    = false;

        while (!done && !err) {
            if (!fgets(buffer, MAX_LINE_LENGTH, in)) {
                err = GB_IO_error("reading", pixmapname);
            }
            else if (strchr(buffer, 0)[-1] != '\n') {
                err = GBS_global_string("Line too long ('%s')", buffer); // increase MAX_LINE_LENGTH above
            }
            else if (strncmp(buffer, "#define", 7) != 0) {
                done = true;
            }
            else {
                char *name_pos = strstr(buffer+7, name);
                if (name_pos) {
                    char *behind = name_pos+namelen;
                    if (strncmp(behind, "_width ", 7) == 0) *width = atoi(behind+7);
                    else if (strncmp(behind, "_height ", 8) == 0) *height = atoi(behind+8);
                }
            }
        }

        if (done && ((*width == 0) || (*height == 0))) {
            if (strstr(buffer, "XPM") != NULL) {
                if (!fgets(buffer, MAX_LINE_LENGTH, in) || !fgets(buffer, MAX_LINE_LENGTH, in)) {
                    err = GB_IO_error("reading", pixmapname);
                }
                else {
                    char *temp = strtok(buffer+1, " ");
                    *width     = atoi(temp);
                    temp       = strtok(NULL,  " ");
                    *height    = atoi(temp);
                }
            }
            else {
                err = "can't detect size";
            }
        }

        free(name);
        fclose(in);
    }
    else {
        err = "no such file";
    }

    if (err) {
        err = GBS_global_string("%s: %s", pixmapname, err);
    }

#if defined(DUMP_BUTTON_CREATION)
    printf("Bitmap '%s' has size %i/%i\n", pixmapname, *width, *height);
#endif // DUMP_BUTTON_CREATION

    free(path);
    return err;
}
#undef MAX_LINE_LENGTH


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


void AW_label_in_awar_list(AW_window *aww, GtkWidget* widget, const char *str) {
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

void AW_window::increment_at_commands(int width, int height) {

    at_shift(width, 0);
    at_shift(-width, 0);        // set bounding box

    if (_at.do_auto_increment) {
        at_shift(_at.auto_increment_x, 0);
    }
    if (_at.do_auto_space) {
        at_shift(_at.auto_space_x + width, 0);
    }

    if (_at.biggest_height_of_buttons < height) {
        _at.biggest_height_of_buttons = height;
    }

    if (_at.max_y_size < (_at.y_for_next_button + _at.biggest_height_of_buttons + 3.0)) {
        _at.max_y_size = _at.y_for_next_button + _at.biggest_height_of_buttons + 3;
    }

    if (_at.max_x_size < (_at.x_for_next_button + this->get_root()->font_width)) {
        _at.max_x_size = _at.x_for_next_button + this->get_root()->font_width;
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


void AW_server_callback(GtkWidget* /*wgt*/, gpointer aw_cb_struct) {
    GTK_PARTLY_IMPLEMENTED;

    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;
    
    AW_root *root = cbs->aw->get_root();
     
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
   // if (root->prvt->recording) root->prvt->recording->record_action(cbs->id);

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
    if (_callback && (long)_callback != 1) {
        if (!_callback->help_text && _at.helptext_for_next_button) {
            _callback->help_text = _at.helptext_for_next_button;
            _at.helptext_for_next_button = 0;
        }
        
        FIXME("this assumes that widget is a button");
        FIXME("investigate why this code works but the commented one does not");
        g_signal_connect((gpointer)widget, "clicked", G_CALLBACK(AW_server_callback), (gpointer)_callback);
        //g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(AW_server_callback), G_OBJECT(_callback));
    }
    _callback = NULL;
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

void AW_window::create_button(const char *macro_name, AW_label buttonlabel, const char */*mnemonic*/, const char */*color*/) {
    // Create a button or text display.
    //
    // If a callback is bound via at->callback(), a button is created.
    // Otherwise a text display is created.
    //
    // if buttonlabel starts with '#' the rest of buttonlabel is used as name of bitmap file used for button
    // if buttonlabel contains a '/' it's interpreted as AWAR name and the button displays the content of the awar
    // otherwise buttonlabel is interpreted as button label (may contain '\n').
    //
    // Note 1: Button width 0 does not work together with labels!

    GTK_PARTLY_IMPLEMENTED;


#if defined(DUMP_BUTTON_CREATION)
    printf("------------------------------ Button '%s'\n", buttonlabel);
    printf("x_for_next_button=%i y_for_next_button=%i\n", _at.x_for_next_button, _at.y_for_next_button);
#endif // DUMP_BUTTON_CREATION

    if (_callback && ((long)_callback != 1))
    {
        if (macro_name) {
            _callback->id = GBS_global_string_copy("%s/%s", this->window_defaults_name, macro_name);
            get_root()->define_remote_command(_callback);
        }
        else {
            _callback->id = 0;
        }
    }

    #define SPACE_BEHIND_LABEL  10
    #define SPACE_BEHIND_BUTTON 3

    #define BUTTON_TEXT_X_PADDING 4
    #define BUTTON_TEXT_Y_PADDING 10

    #define BUTTON_GRAPHIC_PADDING 12
    #define FLAT_GRAPHIC_PADDING   4 // for buttons w/o callback

    bool is_graphical_button = buttonlabel[0] == '#';

    int width_of_button = -1, height_of_button = -1;

    int width_of_label, height_of_label;
    calculate_label_size(this, &width_of_label, &height_of_label, true, 0);
    int width_of_label_and_spacer = _at.label_for_inputfield ? width_of_label+SPACE_BEHIND_LABEL : 0;

    bool let_motif_choose_size  = false;

    if (_at.to_position_exists) { // size has explicitly been specified in xfig -> calculate
        width_of_button  = _at.to_position_x - _at.x_for_next_button - width_of_label_and_spacer;
        height_of_button = _at.to_position_y - _at.y_for_next_button;
    }
    else if (_at.length_of_buttons) { // button width specified from client code
        width_of_button  = BUTTON_TEXT_X_PADDING + calculate_string_width(_at.length_of_buttons+1);

        if (!is_graphical_button) {
            if (_at.height_of_buttons) { // button height specified from client code
                height_of_button = BUTTON_TEXT_Y_PADDING + calculate_string_height(_at.height_of_buttons, 0);
            }
            else {
                int textwidth, textheight;
                calculate_textsize(buttonlabel, &textwidth, &textheight);
                height_of_button = BUTTON_TEXT_Y_PADDING + calculate_string_height(textheight, 0);
            }
        }
        else {
            height_of_button = BUTTON_TEXT_Y_PADDING + calculate_string_height(1, 0);
        }
    }
    else { // no button_length() specified
   

        if (is_graphical_button) {
            int      width, height;
            GB_ERROR err = detect_bitmap_size(buttonlabel+1, &width, &height);

            if (!err) {
                int gpadding = _callback ? BUTTON_GRAPHIC_PADDING : FLAT_GRAPHIC_PADDING;

                width_of_button  = width+gpadding;
                height_of_button = height+gpadding;
            }
            else {
                err = GBS_global_string("button gfx error: %s", err);
                aw_message(err);
                let_motif_choose_size = true;
            }
        }
        else {
            int textwidth, textheight;
            calculate_textsize(buttonlabel, &textwidth, &textheight);

            width_of_button  = BUTTON_TEXT_X_PADDING + calculate_string_width(textwidth+1);
            height_of_button = BUTTON_TEXT_Y_PADDING + calculate_string_height(textheight, 0);
        }
    }

    if (!let_motif_choose_size) {
        if (height_of_button<height_of_label) height_of_button = height_of_label;
        aw_assert(width_of_button && height_of_button);
    }

    int x_label  = _at.x_for_next_button;
    int y_label  = _at.y_for_next_button;
    int x_button = x_label + width_of_label_and_spacer;
    int y_button = y_label;

    int org_correct_for_at_center = _at.correct_for_at_center; // store original justification
    int org_y_for_next_button     = _at.y_for_next_button; // store original y pos (modified while creating label)

    if (!let_motif_choose_size) { // don't correct position of button w/o known size
        // calculate justification manually

        int width_of_button_and_highlight = width_of_button + (_at.highlight ? 2*(_at.shadow_thickness+1)+1 : 0);
        int width_of_label_and_button     = width_of_label_and_spacer+width_of_button_and_highlight;

        if (_at.correct_for_at_center) { // not if left justified
            int shiftback = width_of_label_and_button; // shiftback for right justification
            if (_at.correct_for_at_center == 1) { // center justification
                shiftback /= 2;
            }
            x_label  -= shiftback;
            x_button -= shiftback;
        }

        // we already did the justification by calculating all positions manually, so..
        _at.correct_for_at_center = 0; // ..from now on act like "left justified"!
    }

    // correct label Y position
    if (_callback) {            // only if button is a real 3D-button
        y_label += (height_of_button-height_of_label)/2;
    }

    //GtkWidget* parent_widget = (_at.attach_any) ? prvt->areas[AW_INFO_AREA]->get_form() : prvt->areas[AW_INFO_AREA]->get_area();

    GtkWidget* parent_widget = GTK_WIDGET(prvt->fixed_size_area);

    GtkWidget* tmp_label = 0;

    if (_at.label_for_inputfield) {
        _at.x_for_next_button = x_label;
        _at.y_for_next_button = y_label;

        tmp_label = gtk_label_new(NULL); //NULL means: create label without text
        gtk_fixed_put(GTK_FIXED(parent_widget), tmp_label, x_label, y_label); 

        FIXME("width_of_label is no longer used and can be deleted (just not yet, am not sure if i will need it in the future?");


        if(_at.label_for_inputfield[0]=='#') {
            //is pixmap label
            gtk_label_set_text(GTK_LABEL(tmp_label), "pixmap not implemented");
        }
        else {
            //is string label
            gtk_label_set_text(GTK_LABEL(tmp_label), aw_str_2_label(_at.label_for_inputfield, this));

        }
        gtk_widget_show(tmp_label);


        if (_at.attach_any) {
            aw_attach_widget(tmp_label, _at);
        }

        AW_label_in_awar_list(this, tmp_label, _at.label_for_inputfield);
    }

    _at.x_for_next_button = x_button;
    _at.y_for_next_button = y_button;

    GtkWidget* fatherwidget = parent_widget; // used as father for button below

    GtkWidget* buttonOrLabel  = 0;

    {
        if (_callback) {//button

            buttonOrLabel = gtk_button_new();
            this->_set_activate_callback(buttonOrLabel);

            //highlight selected button
            if (_at.highlight) {
                if (_at.attach_any) {
                    #if defined(DEBUG)
                                printf("Attaching highlighted buttons does not work - "
                                       "highlight ignored for button '%s'!\n", buttonlabel);
                    #endif // DEBUG
                                _at.highlight = false;
                }
                else {
                    FIXME("Highlight button not implemented");
                }
            }

            if(buttonlabel[0]=='#') {
                //pixmap button
                
                GtkWidget* image = gtk_image_new_from_file(aw_str_2_label(buttonlabel, this));//note: aw_str_2_label only returns a path if buttonlabel[0]=='#'
                gtk_button_set_image(GTK_BUTTON(buttonOrLabel), image);
            }
            else {
                //label button
                gtk_button_set_label(GTK_BUTTON(buttonOrLabel), aw_str_2_label(buttonlabel, this));
            }

            FIXME("Button shadow thickness not implemented");
            //gtkbutton does not have shadow thickness. Maybe gtk_button_set_relief
            //args.add(XmNshadowThickness, _at.shadow_thickness);
            FIXME("label alignment inside gtk button not implemented");
            //args.add(XmNalignment,       XmALIGNMENT_CENTER);

        }
        else { // button w/o callback (a label)


            if(buttonlabel[0]=='#') {
                //in motif this was a label with pixmap (XmNlabelPixmap)
                //In gtk it is replaced by a pixmap.
                buttonOrLabel = gtk_image_new_from_file(aw_str_2_label(buttonlabel, this));//note: aw_str_2_label only returns a path if buttonlabel[0]=='#'
            }
            else {
                //just a lable
                buttonOrLabel = gtk_label_new(NULL);
                gtk_label_set_label(GTK_LABEL(buttonOrLabel), aw_str_2_label(buttonlabel, this));
            }

//            button = XtVaCreateManagedWidget("label", xmLabelWidgetClass, parent_widget, RES_LABEL_CONVERT(buttonlabel), NULL);
//            args.add(XmNalignment, (org_correct_for_at_center == 1) ? XmALIGNMENT_CENTER : XmALIGNMENT_BEGINNING);
        }

        FIXME("Button is not using fontlist");
        //args.add(XmNfontList,   (XtArgVal)p_global->fontlist);

        FIXME("Background color for buttons is missing");
        //args.add(XmNbackground, _at.background_color);

        //TODO this is probably useless
//        if (!let_motif_choose_size) {
//            args.add(XmNwidth,  width_of_button);
//            args.add(XmNheight, height_of_button);
//        }

        gtk_fixed_put(GTK_FIXED(fatherwidget), buttonOrLabel, x_button, y_button);
        gtk_widget_realize(buttonOrLabel);
        gtk_widget_show(buttonOrLabel);


        //if (!_at.attach_any || !_callback) args.add(XmNrecomputeSize, false);


        if (_at.attach_any) aw_attach_widget(buttonOrLabel, _at);

        if (_callback) {
            root->make_sensitive(buttonOrLabel, _at.widget_mask);
        }
        else {
            aw_assert(_at.correct_for_at_center == 0);
            FIXME("button justify label, what is this?");
           // AW_JUSTIFY_LABEL(button, _at.correct_for_at_center); // @@@ strange, again sets XmNalignment (already done above). maybe some relict. does nothing if (_at.correct_for_at_center == 0)
        }

        AW_label_in_awar_list(this, buttonOrLabel, buttonlabel);
    }

    short height = 0;
    short width  = 0;

    if (_at.to_position_exists) {
        // size has explicitly been specified in xfig -> calculate
        height = _at.to_position_y - _at.y_for_next_button;
        width  = _at.to_position_x - _at.x_for_next_button;
    }


    {
        //Widget  toRecenter   = 0;
//        int     recenterSize = 0;

        if (!height || !width) {
            // ask gtk for real button size. (note that we use the requested size not the real one.
            //because the real one might be smaller depending on the current window size)
            GtkRequisition requisition;
            gtk_widget_size_request(GTK_WIDGET(buttonOrLabel), &requisition);
            
            height = requisition.height;
            width = requisition.width;

            FIXME("let motif choose size not implemented");
//            if (let_motif_choose_size) {
//                if (_at.correct_for_at_center) {
//                    toRecenter   = ButOrHigh;
//                    recenterSize = width;
//                }
//                width = 0;          // ignore the used size (because it may use more than the window size)
//            }
        }
        FIXME("button shiftback not implemented");
//        if (toRecenter) {
//            int shiftback = 0;
//            switch (_at.correct_for_at_center) {
//                case 1: shiftback = recenterSize/2; break;
//                case 2: shiftback = recenterSize; break;
//            }
//            if (shiftback) {
//                XtVaSetValues(toRecenter, XmNx, x_button-shiftback, NULL);
//            }
//        }
    }

    _at.correct_for_at_center = org_correct_for_at_center; // restore original justification
    _at.y_for_next_button     = org_y_for_next_button;
    FIXME("prvt->toggle_field not set. What is it's purpose anyway?");
   // p_w->toggle_field = button;

    this->unset_at_commands();
    this->increment_at_commands(width+SPACE_BEHIND_BUTTON, height);
}

void AW_window::unset_at_commands() {
    _callback   = NULL;
    _d_callback = NULL;

    _at.correct_for_at_center = 0;
    _at.to_position_exists    = false;
    _at.highlight             = false;

    freenull(_at.helptext_for_next_button);
    freenull(_at.label_for_inputfield);

    _at.background_color = 0;
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

void AW_window::create_autosize_button(const char *macro_name, AW_label buttonlabel, const char *mnemonic /* = 0*/, unsigned xtraSpace /* = 1 */){
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
    this->increment_at_commands(width + SPACE_BEHIND_BUTTON, height);

    //old code:
    //create_toggle(var_name, "#no.bitmap", "#yes.bitmap");
}

void AW_window::create_toggle(const char */*awar_name*/, const char */*nobitmap*/, const char */*yesbitmap*/, int /*buttonWidth*/ /* = 0 */){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_text_toggle(const char */*var_name*/, const char */*noText*/, const char */*yesText*/, int /*buttonWidth*/ /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
}
void AW_window::allow_delete_window(bool /*allow_close*/) {
    GTK_NOT_IMPLEMENTED;
    //aw_set_delete_window_cb(this, p_w->shell, allow_close);
}

void AW_window::create_input_field(const char */*awar_name*/, int /*columns*/ /* = 0 */){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_text_field(const char */*awar_name*/, int /*columns*/ /* = 20 */, int /*rows*/ /*= 4*/){
    GTK_NOT_IMPLEMENTED;
}


// -------------------------
//      Hotkey Checking

#ifdef DEBUG

#define MAX_DEEP_TO_TEST       10
#define MAX_MENU_ITEMS_TO_TEST 50

static char *TD_menu_name = 0;
static char  TD_mnemonics[MAX_DEEP_TO_TEST][MAX_MENU_ITEMS_TO_TEST];
static int   TD_topics[MAX_DEEP_TO_TEST];

struct SearchPossibilities : virtual Noncopyable {
    char *menu_topic;
    SearchPossibilities *next;

    SearchPossibilities(const char* menu_topic_, SearchPossibilities *next_) {
        menu_topic = strdup(menu_topic_);
        next = next_;
    }

    ~SearchPossibilities() {
        free(menu_topic);
        delete next;
    }
};

typedef SearchPossibilities *SearchPossibilitiesPtr;
static SearchPossibilitiesPtr TD_poss[MAX_DEEP_TO_TEST] = { 0 };

inline void addToPoss(int menu_deep, const char *topic_name) {
    TD_poss[menu_deep] = new SearchPossibilities(topic_name, TD_poss[menu_deep]);
}

inline char oppositeCase(char c) {
    return isupper(c) ? tolower(c) : toupper(c);
}

static void strcpy_overlapping(char *dest, char *src) {
    int src_len = strlen(src);
    memmove(dest, src, src_len+1);
}

static const char *possible_mnemonics(int menu_deep, const char *topic_name) {
    int t;
    static char *unused;

    freedup(unused, topic_name);

    for (t = 0; unused[t]; ++t) {
        bool remove = false;
        if (!isalnum(unused[t])) { // remove useless chars
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

    int topics = TD_topics[menu_deep];
    for (t = 0; t<topics; ++t) {
        char c = TD_mnemonics[menu_deep][t]; // upper case!
        char *u = strchr(unused, c);
        if (u)
            strcpy_overlapping(u, u+1); // remove char
        u = strchr(unused, tolower(c));
        if (u)
            strcpy_overlapping(u, u+1); // remove char
    }

    return unused;
}

static void printPossibilities(int menu_deep) {
    SearchPossibilities *sp = TD_poss[menu_deep];
    while (sp) {
        const char *poss = possible_mnemonics(menu_deep, sp->menu_topic);
        fprintf(stderr, "          - Possibilities for '%s': '%s'\n", sp->menu_topic, poss);

        sp = sp->next;
    }

    delete TD_poss[menu_deep];
    TD_poss[menu_deep] = 0;
}

static int menu_deep_check = 0;

static void test_duplicate_mnemonics(int menu_deep, const char *topic_name, const char *mnemonic) {
    if (mnemonic && mnemonic[0] != 0) {
        if (mnemonic[1]) { // longer than 1 char -> wrong
            fprintf(stderr, "Warning: Hotkey '%s' is too long; only 1 character allowed (%s|%s)\n", mnemonic, TD_menu_name, topic_name);
        }
        if (topic_name[0] == '#') { // graphical menu
            if (mnemonic[0]) {
                fprintf(stderr, "Warning: Hotkey '%s' is useless for graphical menu entry (%s|%s)\n", mnemonic, TD_menu_name, topic_name);
            }
        }
        else {
            if (strchr(topic_name, mnemonic[0])) {  // occurs in menu text
                int topics = TD_topics[menu_deep];
                int t;
                char hotkey = toupper(mnemonic[0]); // store hotkeys case-less (case does not matter when pressing the hotkey)

                TD_mnemonics[menu_deep][topics] = hotkey;

                for (t=0; t<topics; t++) {
                    if (TD_mnemonics[menu_deep][t]==hotkey) {
                        fprintf(stderr, "Warning: Hotkey '%c' used twice (%s|%s)\n", hotkey, TD_menu_name, topic_name);
                        addToPoss(menu_deep, topic_name);
                        break;
                    }
                }

                TD_topics[menu_deep] = topics+1;
            }
            else {
                fprintf(stderr, "Warning: Hotkey '%c' is useless; does not occur in text (%s|%s)\n", mnemonic[0], TD_menu_name, topic_name);
                addToPoss(menu_deep, topic_name);
            }
        }
    }
#if defined(DEVEL_RALF)
    else {
        if (topic_name[0] != '#') { // not a graphical menu
            fprintf(stderr, "Warning: Missing hotkey for (%s|%s)\n", TD_menu_name, topic_name);
            addToPoss(menu_deep, topic_name);
        }
    }
#endif // DEVEL_RALF
}

static void open_test_duplicate_mnemonics(int menu_deep, const char *sub_menu_name, const char *mnemonic) {
    aw_assert(menu_deep == menu_deep_check+1);
    menu_deep_check = menu_deep;

    int len = strlen(TD_menu_name)+1+strlen(sub_menu_name)+1;
    char *buf = (char*)malloc(len);

    memset(buf, 0, len);
    sprintf(buf, "%s|%s", TD_menu_name, sub_menu_name);

    test_duplicate_mnemonics(menu_deep-1, sub_menu_name, mnemonic);

    freeset(TD_menu_name, buf);
    TD_poss[menu_deep] = 0;
}

static void close_test_duplicate_mnemonics(int menu_deep) {
    aw_assert(menu_deep == menu_deep_check);
    menu_deep_check = menu_deep-1;

    printPossibilities(menu_deep);
    TD_topics[menu_deep] = 0;

    aw_assert(TD_menu_name);
    // otherwise no menu was opened

    char *slash = strrchr(TD_menu_name, '|');
    if (slash) {
        slash[0] = 0;
    }
    else {
        TD_menu_name[0] = 0;
    }
}

static void init_duplicate_mnemonic() {
    int i;

    if (TD_menu_name) close_test_duplicate_mnemonics(1); // close last menu
    freedup(TD_menu_name, "");

    for (i=0; i<MAX_DEEP_TO_TEST; i++) {
        TD_topics[i] = 0;
    }
    aw_assert(menu_deep_check == 0);
}
static void exit_duplicate_mnemonic() {
    close_test_duplicate_mnemonics(1); // close last menu
    aw_assert(TD_menu_name);
    freenull(TD_menu_name);
    aw_assert(menu_deep_check == 0);
}
#endif

// --------------------------------------------------------------------------------



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
        
        //The user might leave sub menus open. Close them before creating a new top level menu.
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

    root->make_sensitive(GTK_WIDGET(item), mask);
    prvt->number_of_modes++;

    return prvt->number_of_modes;
}


AW_option_menu_struct *AW_window::create_option_menu(const char */*awar_name*/, AW_label /*label*/ /*= 0*/, const char */*mnemonic*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
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
    vs = root->awar(var_name);
    //GType type; // gtk type for the list column
    
//    if(NULL == vs) 
//    {
//        type = G_TYPE_STRING;
//    }
//    else
//    {
//        type = convert_aw_type_to_gtk_type(vs->variable_type);
//    }
    
//    aw_assert(type != G_TYPE_NONE);
    
    tree = gtk_tree_view_new();
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes("List Items",
             renderer, "text", 0, NULL); //column headers are disabled, text does not matter
    gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    store = gtk_list_store_new(1, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));

    
    //    Widget         scrolledWindowList; // @@@ fix locals
//    Widget         scrolledList;
    AW_varUpdateInfo *vui;
    AW_cb_struct  *cbs;
    
    int width_of_list;
    int height_of_list;
    int width_of_last_widget  = 0;
    int height_of_last_widget = 0;

    aw_assert(!_at.label_for_inputfield); // labels have no effect for selection lists

    width_of_list  = this->calculate_string_width(columns) + 9;
    height_of_list = this->calculate_string_height(rows, 4*rows) + 9;
    gtk_widget_set_size_request(GTK_WIDGET(tree), width_of_list, height_of_list);
    
    {
//        aw_xargs args(7);
//        args.add(XmNvisualPolicy,           XmVARIABLE);
//        args.add(XmNscrollBarDisplayPolicy, XmSTATIC);
//        args.add(XmNshadowThickness,        0);
//        args.add(XmNfontList,               (XtArgVal)p_global->fontlist);

        if (_at.to_position_exists) {
            width_of_list = _at.to_position_x - _at.x_for_next_button - 18;
            if (_at.y_for_next_button  < _at.to_position_y - 18) {
                height_of_list = _at.to_position_y - _at.y_for_next_button - 18;
            }
            gtk_widget_set_size_request(GTK_WIDGET(tree), width_of_list, height_of_list);
            FIXME("Attaching list widget not implemented");
            gtk_fixed_put(prvt->fixed_size_area, tree, 10, _at.y_for_next_button);
            //scrolledWindowList = XtVaCreateManagedWidget("scrolledWindowList1", xmScrolledWindowWidgetClass, p_w->areas[AW_INFO_AREA]->get_form(), NULL);

           // args.assign_to_widget(scrolledWindowList);
            //aw_attach_widget(scrolledWindowList, _at);

            width_of_last_widget = _at.to_position_x - _at.x_for_next_button;
            height_of_last_widget = _at.to_position_y - _at.y_for_next_button;
        }
        else {
//            scrolledWindowList = XtVaCreateManagedWidget("scrolledWindowList1", xmScrolledWindowWidgetClass, p_w->areas[AW_INFO_AREA]->get_area(), NULL);
            gtk_fixed_put(prvt->fixed_size_area, tree, 10, _at.y_for_next_button);
        }
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

        root->append_selection_list(new AW_selection_list(var_name, type, GTK_TREE_VIEW(tree)));
    }


    // user-own callback
    cbs = _callback;

    FIXME("enter callback not implemented");
    // callback for enter
    if (vs) {
        vui = new AW_varUpdateInfo(this, tree, AW_WIDGET_SELECTION_LIST, vs, cbs);
        vui->set_sellist(root->get_last_selection_list());

        GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
        g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(AW_varUpdateInfo::AW_variable_update_callback), (gpointer) vui);

        FIXME("double click callback not implemented");
//        if (_d_callback) {
//            XtAddCallback(scrolledList, XmNdefaultActionCallback,
//                          (XtCallbackProc) AW_server_callback,
//                          (XtPointer) _d_callback);
//        }
        vs->tie_widget((AW_CL)root->get_last_selection_list(), tree, AW_WIDGET_SELECTION_LIST, this);
        root->make_sensitive(tree, _at.widget_mask);
    }

    this->unset_at_commands();
    this->increment_at_commands(width_of_last_widget, height_of_last_widget);
    
    gtk_widget_show(tree);
    
    return root->get_last_selection_list();
}



void AW_window::create_toggle_field(const char */*awar_name*/, int /*orientation*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::create_toggle_field(const char */*awar_name*/, AW_label /*label*/, const char */*mnemonic*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::d_callback(void (*/*f*/)(AW_window*, AW_CL), AW_CL /*cd1*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::d_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/){
    GTK_NOT_IMPLEMENTED;
}


AW_pos AW_window::get_scrolled_picture_width() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}
AW_pos AW_window::get_scrolled_picture_height() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
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

void AW_window::get_at_position(int *x, int *y) const {
    *x = _at.x_for_next_button; *y = _at.y_for_next_button; 
}

int AW_window::get_at_xposition() const {
    return _at.x_for_next_button; 
}

int AW_window::get_at_yposition() const {
    return _at.y_for_next_button; 
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
        size_device->clear();
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

void AW_window::insert_default_option (AW_label /*choice_label*/, const char */*mnemonic*/, const char */*var_value*/, const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_default_option (AW_label /*choice_label*/, const char */*mnemonic*/, int /*var_value*/,          const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_default_option (AW_label /*choice_label*/, const char */*mnemonic*/, float /*var_value*/,        const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_default_toggle(AW_label /*toggle_label*/, const char */*mnemonic*/, const char */*var_value*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_default_toggle(AW_label /*toggle_label*/, const char */*mnemonic*/, int /*var_value*/){
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
    root->define_remote_command(cbs);
    root->make_sensitive(item, mask);
}

void AW_window::insert_option(AW_label /*choice_label*/, const char */*mnemonic*/, const char */*var_value*/, const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_option(AW_label /*choice_label*/, const char */*mnemonic*/, int /*var_value*/, const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_option(AW_label /*choice_label*/, const char */*mnemonic*/, float /*var_value*/, const char */*name_of_color*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
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
    
    //append the new submenu to the current menu shell
    gtk_menu_shell_append(prvt->menus.top(), GTK_WIDGET(item));
    
    //use the new submenu as current menu shell.
    prvt->menus.push(GTK_MENU_SHELL(submenu));
    
    
    FIXME("duplicate mnemonic test not implemented");
    #if defined(DUMP_MENU_LIST)
        dumpOpenSubMenu(name);
    #endif // DUMP_MENU_LIST
    #ifdef DEBUG
       // open_test_duplicate_mnemonics(prvt->menu_deep+1, name, mnemonic);
    #endif

    root->make_sensitive(GTK_WIDGET(item), mask);
}


void AW_window::insert_toggle(AW_label /*toggle_label*/, const char */*mnemonic*/, const char */*var_value*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_toggle(AW_label /*toggle_label*/, const char */*mnemonic*/, int /*var_value*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::insert_toggle(AW_label /*toggle_label*/, const char */*mnemonic*/, float /*var_value*/){
    GTK_NOT_IMPLEMENTED;
}

bool AW_window::is_shown() const{
    GTK_NOT_IMPLEMENTED;
    return false;
}

void AW_window::label(const char */*label*/){
    GTK_NOT_IMPLEMENTED;

}

void AW_window::update_text_field(GtkWidget */*widget*/, const char */*var_value*/) {
   // XtVaSetValues(widget, XmNvalue, var_value, NULL);
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

void AW_window::update_input_field(GtkWidget */*widget*/, const char */*var_value*/) {
    //XtVaSetValues(widget, XmNvalue, var_value, NULL);
    GTK_NOT_IMPLEMENTED;
}

void AW_window::refresh_option_menu(AW_option_menu_struct */*oms*/) {
//    if (get_root()->changer_of_variable != oms->label_widget) {
//        AW_widget_value_pair *active_choice = oms->first_choice;
//        {
//            AW_scalar global_var_value(root->awar(oms->variable_name));
//            while (active_choice && global_var_value != active_choice->value) {
//                active_choice = active_choice->next;
//            }
//        }
//
//        if (!active_choice) active_choice = oms->default_choice;
//        if (active_choice) XtVaSetValues(oms->label_widget, XmNmenuHistory, active_choice->widget, NULL);
//    }
    GTK_NOT_IMPLEMENTED;
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
//            AW_scalar global_value(root->awar(toggle_field_list->variable_name));
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
//            length                += (short)_at.saved_xoff_for_label;
//
//            int width_of_last_widget  = length;
//            int height_of_last_widget = height;
//
//            if (toggle_field_list->correct_for_at_center_intern) {
//                if (toggle_field_list->correct_for_at_center_intern == 1) {   // middle centered
//                    XtVaSetValues(p_w->toggle_field, XmNx, (short)((short)_at.saved_x - (short)(length/2) + (short)_at.saved_xoff_for_label), NULL);
//                    if (p_w->toggle_label) {
//                        XtVaSetValues(p_w->toggle_label, XmNx, (short)((short)_at.saved_x - (short)(length/2)), NULL);
//                    }
//                    width_of_last_widget = width_of_last_widget / 2;
//                }
//                if (toggle_field_list->correct_for_at_center_intern == 2) {   // right centered
//                    XtVaSetValues(p_w->toggle_field, XmNx, (short)((short)_at.saved_x - length + (short)_at.saved_xoff_for_label), NULL);
//                    if (p_w->toggle_label) {
//                        XtVaSetValues(p_w->toggle_label, XmNx, (short)((short)_at.saved_x - length),    NULL);
//                    }
//                    width_of_last_widget = 0;
//                }
//            }
//
//            this->unset_at_commands();
//            this->increment_at_commands(width_of_last_widget, height_of_last_widget);
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


static void AW_xfigCB_info_area(AW_window *aww, AW_xfig *xfig) {

    AW_device *device = aww->get_device(AW_INFO_AREA);
    device->reset();

    device->set_offset(AW::Vector(-xfig->minx, -xfig->miny));
    xfig->print(device);
}




void AW_window::load_xfig(const char *file, bool resize /*= true*/){

    AW_xfig *xfig;

    if (file)   xfig = new AW_xfig(file, get_root()->font_width, get_root()->font_height);
    else        xfig = new AW_xfig(get_root()->font_width, get_root()->font_height); // create an empty xfig

    xfig_data = xfig;


    set_expose_callback(AW_INFO_AREA, (AW_CB)AW_xfigCB_info_area, (AW_CL)xfig_data, 0);
    //TODO remove color mode
    xfig->create_gcs(get_device(AW_INFO_AREA), get_root()->color_mode ? 8 : 1); 

    int xsize = xfig->maxx - xfig->minx;
    int ysize = xfig->maxy - xfig->miny;

    if (xsize > _at.max_x_size) _at.max_x_size = xsize;
    if (ysize > _at.max_y_size) _at.max_y_size = ysize;

    AW_device *device = get_device(AW_INFO_AREA);

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
        set_window_size(_at.max_x_size, _at.max_y_size);

        FIXME("this call should not be necessary. The screen size should have been set by the resize_callback...");
        device->get_common()->set_screen_size(_at.max_x_size, _at.max_y_size);

    }
}

const char *AW_window::local_id(const char */*id*/) const{
    GTK_NOT_IMPLEMENTED;
    return 0;
}



void AW_window::restore_at_size_and_attach(const AW_at_size */*at_size*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::sens_mask(AW_active /*mask*/){
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

void AW_window::set_horizontal_change_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_horizontal_scrollbar_position(int /*position*/) {
    GTK_NOT_IMPLEMENTED;
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

void AW_window::set_vertical_change_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_vertical_scrollbar_position(int /*position*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_window_size(int width, int height) {
    gtk_window_set_default_size(prvt->window, width, height);
}

void AW_window::set_window_title(const char */*title*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::shadow_width (int /*shadow_thickness*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::show() {
    arb_assert(NULL != prvt->window);
    gtk_widget_show_all(GTK_WIDGET(prvt->window));

}

void AW_window::button_height(int height) {
    _at.height_of_buttons = height>1 ? height : 0; 
}

void AW_window::store_at_size_and_attach(AW_at_size */*at_size*/) {
    GTK_NOT_IMPLEMENTED;
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


void AW_window_message::init(AW_root */*root_in*/, const char */*windowname*/, bool /*allow_close*/) {
    GTK_NOT_IMPLEMENTED;
//    root = root_in; // for macro
//
//    int width  = 100;
//    int height = 100;
//    int posx   = 50;
//    int posy   = 50;
//
//    window_name = strdup(windowname);
//    window_defaults_name = GBS_string_2_key(window_name);
//
//    // create shell for message box
//    p_w->shell = aw_create_shell(this, true, allow_close, width, height, posx, posy);
//
//    // disable resize or maximize in simple dialogs (avoids broken layouts)
//    XtVaSetValues(p_w->shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
//            NULL);
//
//    p_w->areas[AW_INFO_AREA] = new AW_area_management(root, p_w->shell, XtVaCreateManagedWidget("info_area",
//                    xmDrawingAreaWidgetClass,
//                    p_w->shell,
//                    XmNheight, 0,
//                    XmNbottomAttachment, XmATTACH_NONE,
//                    XmNtopAttachment, XmATTACH_FORM,
//                    XmNleftAttachment, XmATTACH_FORM,
//                    XmNrightAttachment, XmATTACH_FORM,
//                    NULL));
//
//    aw_realize_widget(this);
}


AW_window_message::AW_window_message() {
    GTK_NOT_IMPLEMENTED;
}

AW_window_message::~AW_window_message() {
    GTK_NOT_IMPLEMENTED;
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

void AW_window::update_option_menu() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::update_toggle_field() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::window_fit() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::wm_activate() {
    GTK_NOT_IMPLEMENTED;
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

AW_window::AW_window() {
    color_table_size = 0;
    color_table = NULL;
    
    picture = new AW_screen_area;
    reset_scrolled_picture_size();
    
    prvt = new AW_window::AW_window_gtk();
    for(int i = 0; i < AW_MAX_AREA; i++ ) {
        prvt->areas.push_back(NULL);
    }
}

void AW_window::reset_scrolled_picture_size() {
    picture->l = 0;
    picture->r = 0;
    picture->t = 0;
    picture->b = 0;
}

AW_window::~AW_window() {
    delete picture;
    delete prvt;
}


//aw_window_menu
//TODO move to own file
AW_window_menu::AW_window_menu(){
    GTK_NOT_IMPLEMENTED;
}

AW_window_menu::~AW_window_menu(){
    GTK_NOT_IMPLEMENTED;
}

void AW_window_menu::init(AW_root */*root*/, const char */*wid*/, const char */*windowname*/, int /*width*/, int /*height*/) {
    GTK_NOT_IMPLEMENTED;
}

AW_window_menu_modes::AW_window_menu_modes() {
    GTK_NOT_IMPLEMENTED;
}

void aw_create_help_entry(AW_window *aww) {
    aww->insert_help_topic("Click here and then on the questionable button/menu/...", "P", 0,
                           AWM_ALL, (AW_CB)AW_help_entry_pressed, 0, 0);
}

void AW_window_menu_modes::init(AW_root *root_in, const char *wid, const char *windowname, int width, int height) {

    GtkWidget *vbox;
    GtkWidget *hbox;

    const char *help_button   = "_HELP"; //underscore + mnemonic 


#if defined(DUMP_MENU_LIST)
    initMenuListing(windowname);
#endif // DUMP_MENU_LIST
    root = root_in; // for macro
    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);
    
    prvt->window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_window_set_title(prvt->window, window_name);
    gtk_window_set_default_size(prvt->window, width, height);
    vbox = gtk_vbox_new(false, 1); FIXME("pixel constants in gui init code");
    gtk_container_add(GTK_CONTAINER(prvt->window), vbox);

    GTK_PARTLY_IMPLEMENTED;
    
    prvt->menu_bar = (GtkMenuBar*) gtk_menu_bar_new();
    prvt->menus.push(GTK_MENU_SHELL(prvt->menu_bar));//The menu bar is the top level menu shell.

    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(prvt->menu_bar), false,
                       false, //has no effect if the third parameter is false
                       2); FIXME("pixel constants in gui init code");

    //create help menu
    FIXME("HELP button is not the rightmost button");
    GtkMenuItem *help_item = GTK_MENU_ITEM(gtk_menu_item_new_with_mnemonic(help_button));
    prvt->help_menu = GTK_MENU_SHELL(gtk_menu_new());
    gtk_menu_item_set_submenu(help_item, GTK_WIDGET(prvt->help_menu));
    gtk_menu_item_set_right_justified(help_item, true);
    
    gtk_menu_shell_append(GTK_MENU_SHELL(prvt->menu_bar), GTK_WIDGET(help_item));
    
    
    hbox = gtk_hbox_new(false, 1);FIXME("pixel constants in gui init code");
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hbox), true, true, 0);
    
    prvt->mode_menu = GTK_TOOLBAR(gtk_toolbar_new());
    gtk_toolbar_set_orientation(prvt->mode_menu, GTK_ORIENTATION_VERTICAL);
    
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(prvt->mode_menu), false, false, 0);

    GtkWidget *vbox2 = gtk_vbox_new(false, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox2, true, true, 0);
    
//    GtkWidget *hbox2 = gtk_hbox_new(false, 0);
//    gtk_box_pack_start(GTK_BOX(vbox2), hbox2, true, true, 0);
    
    
    //The buttons are above the drawing are
    prvt->fixed_size_area = GTK_FIXED(gtk_fixed_new());
    FIXME("form should be a frame around area?!");
    prvt->areas[AW_INFO_AREA] = new AW_area_management(root, GTK_WIDGET(prvt->fixed_size_area), GTK_WIDGET(prvt->fixed_size_area)); 
    gtk_box_pack_start(GTK_BOX(vbox2), GTK_WIDGET(prvt->fixed_size_area), false, false, 0);
    
    
    GtkWidget* drawing_area = gtk_drawing_area_new();
    gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), 3000, 3000); FIXME("pixel constants in gui init code");
    GtkWidget *scrollArea = gtk_scrolled_window_new(NULL, NULL); //NULL causes the scrolledWindow to create its own scroll adjustments
    gtk_box_pack_start(GTK_BOX(vbox2), scrollArea, true, true, 0);   
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollArea), drawing_area);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollArea), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS);
    
    //gtk_box_pack_end(GTK_BOX(vbox2), scrollArea, true, true, 0);
    gtk_widget_realize(GTK_WIDGET(drawing_area));

    prvt->areas[AW_MIDDLE_AREA] = new AW_area_management(root, drawing_area, drawing_area); //FIXME form should be a frame around the area.
    

    gtk_widget_realize(GTK_WIDGET(prvt->window)); //realizes everything
    create_devices();
    aw_create_help_entry(this);
    create_window_variables();
    
}


AW_color_idx AW_window::alloc_named_data_color(int colnum, char *colorname) {
    if (!color_table_size) {
        color_table_size = AW_STD_COLOR_IDX_MAX + colnum;
        color_table      = (AW_rgb*)malloc(sizeof(AW_rgb) *color_table_size);
        FIXME("warning: large integer implicitly truncated to unsigned type");
        for (int i = 0; i<color_table_size; ++i) color_table[i] = AW_NO_COLOR;
    }
    else {
        if (colnum>=color_table_size) {
            long new_size = colnum+8;
            color_table   = (AW_rgb*)realloc(color_table, new_size*sizeof(AW_rgb)); // valgrinders : never freed because AW_window never is freed
            FIXME("warning: large integer implicitly truncated to unsigned type");
            for (int i = color_table_size; i<new_size; ++i) color_table[i] = AW_NO_COLOR;
            color_table_size = new_size;
        }
    }

    color_table[colnum] = root->alloc_named_data_color(colorname);
    
    if (colnum == AW_DATA_BG) {
        AW_area_management* pMiddleArea = prvt->areas[AW_MIDDLE_AREA];
        if(pMiddleArea) {
            FIXME("warning: taking address of temporary");
            gtk_widget_modify_bg(pMiddleArea->get_area(),GTK_STATE_NORMAL, &root->getColor(color_table[colnum]));
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

    sprintf(buffer, "window/%s/scroll_delay_horizontal", window_defaults_name);
    get_root()->awar_int(buffer, 20);
    get_root()->awar(buffer)->add_callback(horizontal_scrollbar_redefinition_cb, (AW_CL)this);

    sprintf(buffer, "window/%s/scroll_delay_vertical", window_defaults_name);
    get_root()->awar_int(buffer, 20);
    get_root()->awar(buffer)->add_callback(vertical_scrollbar_redefinition_cb, (AW_CL)this);

    sprintf(buffer, "window/%s/scroll_width_horizontal", window_defaults_name);
    get_root()->awar_int(buffer, 9);
    get_root()->awar(buffer)->add_callback(horizontal_scrollbar_redefinition_cb, (AW_CL)this);

    sprintf(buffer, "window/%s/scroll_width_vertical", window_defaults_name);
    get_root()->awar_int(buffer, 20);
    get_root()->awar(buffer)->add_callback(vertical_scrollbar_redefinition_cb, (AW_CL)this);
}


AW_window_menu_modes::~AW_window_menu_modes() {
    GTK_NOT_IMPLEMENTED;
}

AW_window_simple_menu::AW_window_simple_menu() {
    GTK_NOT_IMPLEMENTED;
}

AW_window_simple_menu::~AW_window_simple_menu() {
    GTK_NOT_IMPLEMENTED;
}

void AW_window_simple_menu::init(AW_root */*root*/, const char */*wid*/, const char */*windowname*/) {
    GTK_NOT_IMPLEMENTED;
}

AW_window_simple::AW_window_simple() : AW_window() {
    GTK_NOT_IMPLEMENTED;
}
void AW_window_simple::init(AW_root *root_in, const char *wid, const char *windowname) {
    root = root_in; // for macro
    prvt->window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    prvt->fixed_size_area = GTK_FIXED(gtk_fixed_new());
    gtk_container_add(GTK_CONTAINER(prvt->window), GTK_WIDGET(prvt->fixed_size_area));

    //Creates the GDK (windowing system) resources associated with a widget.
    //This is done as early as possible because xfig drawing relies on the gdk stuff.
    gtk_widget_realize(GTK_WIDGET(prvt->window));
    gtk_widget_realize(GTK_WIDGET(prvt->fixed_size_area));
    gtk_widget_show(GTK_WIDGET(prvt->fixed_size_area));


//    int width  = 100;                               // this is only the minimum size!
//    int height = 100;
//    int posx   = 50;
//    int posy   = 50;

    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    gtk_window_set_resizable(prvt->window, true);
    gtk_window_set_title(prvt->window, window_name);

    GTK_PARTLY_IMPLEMENTED;



   // p_w->shell = aw_create_shell(this, true, true, width, height, posx, posy);

    // add this to disable resize or maximize in simple dialogs (avoids broken layouts)
    // XtVaSetValues(p_w->shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE, NULL);

//    Widget form1 = XtVaCreateManagedWidget("forms", xmFormWidgetClass,
//            p_w->shell,
//            NULL);
    prvt->areas.reserve(AW_MAX_AREA);
    prvt->areas[AW_INFO_AREA] = new AW_area_management(root, GTK_WIDGET(prvt->window), GTK_WIDGET(prvt->window));


  //  aw_realize_widget(this);
    create_devices();
}

void AW_window::create_devices() {

    GTK_PARTLY_IMPLEMENTED;

 //   unsigned long background_color;
    if (prvt->areas[AW_INFO_AREA]) {
        prvt->areas[AW_INFO_AREA]->create_devices(this, AW_INFO_AREA);
//        XtVaGetValues(p_w->areas[AW_INFO_AREA]->get_area(), XmNbackground, &background_color, NULL);
//        p_global->color_table[AW_WINDOW_DRAG] = background_color ^ p_global->color_table[AW_WINDOW_FG];
    }
    if (prvt->areas[AW_MIDDLE_AREA]) {
        prvt->areas[AW_MIDDLE_AREA]->create_devices(this, AW_MIDDLE_AREA);
    }
    if (prvt->areas[AW_BOTTOM_AREA]) {
        prvt->areas[AW_BOTTOM_AREA]->create_devices(this, AW_BOTTOM_AREA);
    }
}

void AW_window::update_scrollbar_settings_from_awars(AW_orientation /*orientation*/) {
//    AW_screen_area scrolled;
//    get_scrollarea_size(&scrolled);
//
//    // @@@ DRY awar code
//
//    char buffer[200];
//    if (orientation == AW_HORIZONTAL) {
//        sprintf(buffer, "window/%s/horizontal_page_increment", window_defaults_name); 
//        XtVaSetValues(p_w->scroll_bar_horizontal, XmNpageIncrement, (int)(scrolled.r*(get_root()->awar(buffer)->read_int()*0.01)), NULL);
//
//        sprintf(buffer, "window/%s/scroll_width_horizontal", window_defaults_name);
//        XtVaSetValues(p_w->scroll_bar_horizontal, XmNincrement, (int)(get_root()->awar(buffer)->read_int()), NULL);
//
//        sprintf(buffer, "window/%s/scroll_delay_horizontal", window_defaults_name);
//        XtVaSetValues(p_w->scroll_bar_horizontal, XmNrepeatDelay, (int)(get_root()->awar(buffer)->read_int()), NULL);
//    }
//    else {
//        sprintf(buffer, "window/%s/vertical_page_increment", window_defaults_name);
//        XtVaSetValues(p_w->scroll_bar_vertical, XmNpageIncrement, (int)(scrolled.b*(get_root()->awar(buffer)->read_int()*0.01)), NULL);
//
//        sprintf(buffer, "window/%s/scroll_width_vertical", window_defaults_name);
//        XtVaSetValues(p_w->scroll_bar_vertical, XmNincrement, (int)(get_root()->awar(buffer)->read_int()), NULL);
//
//        sprintf(buffer, "window/%s/scroll_delay_vertical", window_defaults_name);
//        XtVaSetValues(p_w->scroll_bar_vertical, XmNrepeatDelay, (int)(get_root()->awar(buffer)->read_int()), NULL);
//    }
    GTK_NOT_IMPLEMENTED;
}


AW_window_simple::~AW_window_simple() {
    GTK_NOT_IMPLEMENTED;
}


void AW_at_auto::restore(AW_at &/*at*/) const {
    GTK_NOT_IMPLEMENTED;
}

void AW_at_auto::store(AW_at const &/*at*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_at_maxsize::store(const AW_at &/*at*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_at_maxsize::restore(AW_at& /*at*/) const {
    GTK_NOT_IMPLEMENTED;
}


//TODO comment
AW_cb_struct_guard AW_cb_struct::guard_before = NULL;
AW_cb_struct_guard AW_cb_struct::guard_after  = NULL;
AW_postcb_cb       AW_cb_struct::postcb       = NULL;


AW_cb_struct::AW_cb_struct(AW_window *awi, void (*g)(AW_window*, AW_CL, AW_CL), AW_CL cd1i, AW_CL cd2i, const char *help_texti, class AW_cb_struct *nexti) {
    aw            = awi;
    f             = g;
    cd1           = cd1i;
    cd2           = cd2i;
    help_text     = help_texti;
    pop_up_window = NULL;
    this->next = nexti;

    id = NULL;
}


bool AW_cb_struct::contains(void (*g)(AW_window*, AW_CL, AW_CL)) {
    return (f == g) || (next && next->contains(g));
}

bool AW_cb_struct::is_equal(const AW_cb_struct& other) const {
    bool equal = false;
    if (f == other.f) {                             // same callback function
        equal = (cd1 == other.cd1) && (cd2 == other.cd2);
        if (equal) {
            if (f == AW_POPUP) {
                equal = aw->get_root() == other.aw->get_root();
            }
            else {
                equal = aw == other.aw;
                if (!equal) {
                    equal = aw->get_root() == other.aw->get_root();
#if defined(DEBUG) && 0
                    if (equal) {
                        fprintf(stderr,
                                "callback '%s' instantiated twice with different windows (w1='%s' w2='%s') -- assuming the callbacks are equal\n",
                                id, aw->get_window_id(), other.aw->get_window_id());
                    }
#endif // DEBUG
                }
            }
        }
    }
    return equal;
}




void AW_cb_struct::run_callback() {
    if (next) next->run_callback();                 // callback the whole list
    if (!f) return;                                 // run no callback

    AW_root *root = aw->get_root();
    if (root->disable_callbacks) {
        // some functions (namely aw_message, aw_input, aw_string_selection and aw_file_selection)
        // have to disable most callbacks, because they are often called from inside these callbacks
        // (e.g. because some exceptional condition occurred which needs user interaction) and if
        // callbacks weren't disabled, a recursive deadlock occurs.

        // the following callbacks are allowed even if disable_callbacks is true

        bool isModalCallback = (f == AW_CB(message_cb) ||
                                f == AW_CB(input_history_cb) ||
                                f == AW_CB(input_cb) ||
                                f == AW_CB(file_selection_cb));

        bool isPopdown = (f == AW_CB(AW_POPDOWN));
        bool isHelp    = (f == AW_CB(AW_POPUP_HELP));
        bool allow     = isModalCallback || isHelp || isPopdown;

        bool isInfoResizeExpose = false;

        if (!allow) {
            isInfoResizeExpose = aw->is_expose_callback(AW_INFO_AREA, f) || aw->is_resize_callback(AW_INFO_AREA, f);
            allow              = isInfoResizeExpose;
        }

        if (!allow) {
            // do not change position of modal dialog, when one of the following callbacks happens - just raise it
            // (other callbacks like pressing a button, will position the modal dialog under mouse)
            bool onlyRaise =
                aw->is_expose_callback(AW_MIDDLE_AREA, f) ||
                aw->is_focus_callback(f) ||
                root->is_focus_callback((AW_RCB)f) ||
                aw->is_resize_callback(AW_MIDDLE_AREA, f);

            if (root->current_modal_window) { 
                AW_window *awmodal = root->current_modal_window;

                AW_PosRecalc prev = awmodal->get_recalc_pos_atShow();
                if (onlyRaise) awmodal->recalc_pos_atShow(AW_KEEP_POS);
                awmodal->activate();
                awmodal->recalc_pos_atShow(prev);
            }
            else {
                aw_message("Internal error (callback suppressed when no modal dialog active)");
                aw_assert(0);
            }
#if defined(TRACE_CALLBACKS)
            printf("suppressing callback %p\n", f);
#endif // TRACE_CALLBACKS
            return; // suppress the callback!
        }
#if defined(TRACE_CALLBACKS)
        else {
            if (isModalCallback) printf("allowed modal callback %p\n", f);
            else if (isPopdown) printf("allowed AW_POPDOWN\n");
            else if (isHelp) printf("allowed AW_POPUP_HELP\n");
            else if (isInfoResizeExpose) printf("allowed expose/resize infoarea\n");
            else printf("allowed other (unknown) callback %p\n", f);
        }
#endif // TRACE_CALLBACKS
    }
    else {
#if defined(TRACE_CALLBACKS)
        printf("Callbacks are allowed (executing %p)\n", f);
#endif // TRACE_CALLBACKS
    }

    useraction_init();
    
    if (f == AW_POPUP) {
        if (pop_up_window) { // already exists
            pop_up_window->activate();
        }
        else {
            AW_PPP g = (AW_PPP)cd1;
            if (g) {
                pop_up_window = g(aw->get_root(), cd2, 0);
                pop_up_window->activate();
            }
            else {
                aw_message("not implemented -- please report to devel@arb-home.de");
            }
        }
        if (pop_up_window && pop_up_window->prvt->popup_cb)
            pop_up_window->prvt->popup_cb->run_callback();
    }
    else {
        f(aw, cd1, cd2);
    }

    useraction_done(aw);
}


