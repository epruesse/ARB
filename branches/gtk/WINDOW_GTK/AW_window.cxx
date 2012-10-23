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
#include "aw_device.hxx"
#include "aw_at.hxx"
#include "aw_msg.hxx"
#include "aw_awar.hxx"
#include "aw_common.hxx"
#include <arbdb.h>

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


/**
 * This class hides all gtk dependent attributes. 
 * This is done to avoid gtk includes in the header file.
 */
class AW_window::AW_window_gtk {
public:
    
    GtkWindow *window; /**< The gtk window instance managed by this aw_window */
    
    /**
     *  A fixed size widget spanning the whole window. Everything is positioned on this widget using absolut coordinates.
     * @note This area is only present in aw_window_simple
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
     * A window consists of several areas.
     * Some of those areas are named, some are unnamed.
     * The unnamed areas are instantiated once and never changed, therefore no references to unnamed areas exist.
     * Named areas are instantiated depending on the window type.
     *
     * This vector contains references to the named areas.
     * The AW_Area enum is used to index this vector.
     */
    std::vector<AW_area_management *> areas;
    
    AW_window_gtk() : window(NULL), fixed_size_area(NULL), menu_bar(NULL) {}
    
};



void AW_clock_cursor(AW_root *) {
    GTK_NOT_IMPLEMENTED;
}

void AW_normal_cursor(AW_root *) {
    GTK_NOT_IMPLEMENTED;
}

void AW_help_entry_pressed(AW_window *) {
    GTK_NOT_IMPLEMENTED;
}

void AW_POPDOWN(AW_window *){
    GTK_NOT_IMPLEMENTED;
}

void AW_POPUP_HELP(AW_window *, AW_CL /* char */ /*helpfile*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_openURL(AW_root * /*aw_root*/, const char * /*url*/) {
    GTK_NOT_IMPLEMENTED;
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


void AW_window::at_set_to(bool /*attach_x*/, bool /*attach_y*/, int /*xoff*/, int /*yoff*/){
    GTK_NOT_IMPLEMENTED;
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
void AW_window::button_length(int /*length*/){
    GTK_NOT_IMPLEMENTED;
}

int  AW_window::get_button_length() const{
    GTK_NOT_IMPLEMENTED;
    return 0;
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
    _callback = new AW_cb_struct(this, (AW_CB)f, cd1);//FIXME is it really good to new the _callback each time?
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

void AW_window::TuneOrSetBackground(GtkWidget* w, const char *color, int modStrength) {
    // Sets the background for the next created widget.
    //
    // If 'color' is specified, it may contain one of the following values:
    //      "+"    means: slightly increase color of parent widget 'w'
    //      "-"    means: slightly decrease color of parent widget 'w'
    //      otherwise it contains a specific color ('name' or '#RGB')
    //
    // If color is not specified, the color of the parent widget 'w' is modified
    // by 'modStrength' (increased if positive,  decreased if negative)
    //
    // If it's not possible to modify the color (e.g. we cannot increase 'white'),
    // the color will be modified in the opposite direction. For details see TuneBackground()

    if (color) {
        switch (color[0]) {
        case '+':
            TuneBackground(w, TUNE_BRIGHT);
            break;
        case '-':
            TuneBackground(w, TUNE_DARK);
            break;
        default:
            set_background(color, w); // use explicit color
            break;
        }
    }
    else {
        TuneBackground(w, modStrength);
    }
}

void AW_window::TuneBackground(GtkWidget* /*w*/, int /*modStrength*/) {
    // Gets the Background Color, modifies the rgb values slightly and sets new background color
    // Intended to give buttons a nicer 3D-look.
    //
    // possible values for modStrength:
    //
    //    0        = do not modify (i.e. set to background color of parent widget)
    //    1 .. 127 = increase if background is bright, decrease if background is dark
    //   -1 ..-127 = opposite behavior than above
    //  256 .. 383 = always increase
    // -256 ..-383 = always decrease
    //
    // if it's impossible to decrease or increase -> opposite direction is used.
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

//FIXME right now it seems that this method can be transformed into a private member of aw_window. However I am not sure yet. Check later
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


//FIXME I think this method should be transformed into a private member
static char *pixmapPath(const char *pixmapName) {
    return nulldup(GB_path_in_ARBLIB("pixmaps", pixmapName));
}

//FIXME this method was originally defined in AW_button.cxx.
//FIXME I think this method should be transformed into a private member
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


//FIXME I think this should be a private member
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

static void aw_attach_widget(GtkWidget* /*w*/, AW_at& /*_at*/, int default_width = -1) {
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
            aw_assert(0); // awar not found
            aww->update_label(widget, str); //FIXME unreachable code
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

void AW_window::set_background(const char *colorname, GtkWidget* parentWidget) {
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

//FIXME why is this called server_callback??
void AW_server_callback(GtkWidget* /*wgt*/, gpointer aw_cb_struct) {
    GTK_PARTLY_IMPLEMENTED;

    AW_cb_struct *cbs = (AW_cb_struct *) aw_cb_struct;

    AW_root *root = cbs->aw->get_root();
    //FIXME AW_server_callback help not implemented
//    if (p_global->help_active) {
//        p_global->help_active = 0;
//        p_global->normal_cursor();
//
//        if (cbs->help_text && ((GBS_string_matches(cbs->help_text, "*.ps", GB_IGNORE_CASE)) ||
//                               (GBS_string_matches(cbs->help_text, "*.hlp", GB_IGNORE_CASE)) ||
//                               (GBS_string_matches(cbs->help_text, "*.help", GB_IGNORE_CASE))))
//        {
//            AW_POPUP_HELP(cbs->aw, (AW_CL)cbs->help_text);
//        }
//        else {
//            aw_message("Sorry no help available");
//        }
//        return;
//    }
    //FIXME AW_server_callback recording not implemented
   // if (root->prvt->recording) root->prvt->recording->record_action(cbs->id);

    if (cbs->f == AW_POPUP) {
        cbs->run_callback();
    }
    else {
        //FIXME AW_server_callback wait cursor not implemented
//        p_global->set_cursor(XtDisplay(p_global->toplevel_widget),
//                XtWindow(p_aww(cbs->aw)->shell),
//                p_global->clock_cursor);
        cbs->run_callback();
        //FIXME AW_server_callback destruction of old events not implemented
//        XEvent event; // destroy all old events !!!
//        while (XCheckMaskEvent(XtDisplay(p_global->toplevel_widget),
//        ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
//        KeyPressMask|KeyReleaseMask|PointerMotionMask, &event)) {
//        }
        //FIXME AW_server_callback help not implemented
//        if (p_global->help_active) {
//            p_global->set_cursor(XtDisplay(p_global->toplevel_widget),
//                    XtWindow(p_aww(cbs->aw)->shell),
//                    p_global->question_cursor);
//        }
//        else {
//            p_global->set_cursor(XtDisplay(p_global->toplevel_widget),
//                    XtWindow(p_aww(cbs->aw)->shell),
//                    0);
//        }
    }

}

void AW_window::_set_activate_callback(GtkWidget *widget) {
    if (_callback && (long)_callback != 1) {
        if (!_callback->help_text && _at.helptext_for_next_button) {
            _callback->help_text = _at.helptext_for_next_button;
            _at.helptext_for_next_button = 0;
        }

        //FIXME this assumes that widget is a button
        //FIXME investigate why this code works but the commented one does not
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

void AW_window::create_button(const char *macro_name, AW_label buttonlabel, const char */*mnemonic*/, const char *color) {
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

    // Note 2: "color" may be specified for the button background (see TuneOrSetBackground for details)

    GTK_PARTLY_IMPLEMENTED;
    TuneOrSetBackground(_at.attach_any ? prvt->areas[AW_INFO_AREA]->get_form() : prvt->areas[AW_INFO_AREA]->get_area(), // set background for buttons / text displays
                        color,
                        _callback ? TUNE_BUTTON : 0);



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

    #if defined(ASSERTION_USED)
        AW_awar *is_awar = is_graphical_button ? NULL : get_root()->label_is_awar(buttonlabel);
    #endif // ASSERTION_USED

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
        aw_assert(!is_awar); // please specify button_length() for AWAR button!

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
    //FIXME quick hack
    GtkWidget* parent_widget = GTK_WIDGET(prvt->fixed_size_area);

    GtkWidget* tmp_label = 0;

    if (_at.label_for_inputfield) {
        _at.x_for_next_button = x_label;
        _at.y_for_next_button = y_label;

        tmp_label = gtk_label_new(NULL); //NULL means: create label without text
        gtk_fixed_put(GTK_FIXED(parent_widget), tmp_label, x_label, y_label); //FIXME evil hack depends on areas beeing GtkFixed

        //FIXME width_of_label is no longer used and can be deleted (just not yet, am not sure if i will need it in the future?)


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
                   //FIXME highlight button here
                }
            }


            if(buttonlabel[0]=='#') {
                //pixmap button
                //FIXME pixmap button not implemented
                gtk_button_set_label(GTK_BUTTON(buttonOrLabel), "pixmap button not impl");
            }
            else {
                //label button
                gtk_button_set_label(GTK_BUTTON(buttonOrLabel), aw_str_2_label(buttonlabel, this));
            }


            //FIXME gtkbutton does not have shadow thickness. Maybe gtk_button_set_relief
            //args.add(XmNshadowThickness, _at.shadow_thickness);
            //FIXME label alignment inside gtk button not implemented
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

        //FIXME font stuff
        //args.add(XmNfontList,   (XtArgVal)p_global->fontlist);

        //FIXME background color for buttons is missing
        //args.add(XmNbackground, _at.background_color);

        //FIXME let motif choose size not implemented
//        if (!let_motif_choose_size) {
//            args.add(XmNwidth,  width_of_button);
//            args.add(XmNheight, height_of_button);
//        }

        gtk_fixed_put(GTK_FIXED(fatherwidget), buttonOrLabel, x_button, y_button);
        gtk_widget_show(buttonOrLabel);


        //if (!_at.attach_any || !_callback) args.add(XmNrecomputeSize, false);


        if (_at.attach_any) aw_attach_widget(buttonOrLabel, _at);

        if (_callback) {
            //FIXME sensitive button not implemented
            //root->make_sensitive(button, _at.widget_mask);
        }
        else {
            aw_assert(_at.correct_for_at_center == 0);
            //FIXME button justify label?
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
        int     recenterSize = 0;

        if (!height || !width) {
            // ask motif for real button size
            height = buttonOrLabel->allocation.height;
            width = buttonOrLabel->allocation.width;

            //FIXME let motif choose size not implemented
//            if (let_motif_choose_size) {
//                if (_at.correct_for_at_center) {
//                    toRecenter   = ButOrHigh;
//                    recenterSize = width;
//                }
//                width = 0;          // ignore the used size (because it may use more than the window size)
//            }
        }
        //FIXME shiftback not implemented
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
    //FIXME prvt->toggle_field not set to button. What is its purpose anyway?
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

void AW_window::create_toggle(const char *var_name){

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
        //FIXME debug code for duplicate mnemonics missing

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

int AW_window::create_mode(const char */*pixmap*/, const char */*help_text*/, AW_active /*mask*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}


AW_option_menu_struct *AW_window::create_option_menu(const char */*awar_name*/, AW_label /*label*/ /*= 0*/, const char */*mnemonic*/ /*= 0*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_selection_list *AW_window::create_selection_list(const char */*awar_name*/, int /*columns*/ /*= 4*/, int /*rows*/ /*= 4*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
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

}

void AW_window::get_at_position(int */*x*/, int */*y*/) const{

}

int AW_window::get_at_xposition() const{
    GTK_NOT_IMPLEMENTED;
    return 0;
}

int AW_window::get_at_yposition() const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_device_click *AW_window::get_click_device(AW_area /*area*/, int /*mousex*/, int /*mousey*/, AW_pos /*max_distance_linei*/,
                                  AW_pos /*max_distance_texti*/, AW_pos /*radi*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}
AW_device *AW_window::get_device(AW_area area){
    AW_area_management *aram   = prvt->areas[area];
    arb_assert(NULL != aram);
    return (AW_device *)aram->get_screen_device();
}

void AW_window::get_event(AW_event */*eventi*/) const{

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


void AW_window::insert_help_topic(AW_label /*name*/, const char */*mnemonic*/, const char */*help_text*/, AW_active /*mask*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/){
    GTK_NOT_IMPLEMENTED;
}


void AW_window::insert_menu_topic(const char *topic_id, AW_label name, const char *mnemonic, const char *help_text_, AW_active mask, AW_CB cb, AW_CL cd1, AW_CL cd2){
   aw_assert(legal_mask(mask));
  
   GTK_PARTLY_IMPLEMENTED;
   //FIXME mnemonics missing
   
   if (!topic_id) topic_id = name; // hmm, due to this we cannot insert_menu_topic w/o id. Change? @@@

   GtkWidget *item = gtk_menu_item_new_with_label(name);
   aw_assert(prvt->menus.size() > 0); //closed too many menus
   gtk_menu_shell_append(prvt->menus.top(), item);  
   
   // TuneBackground(p_w->menu_bar[p_w->menu_deep], TUNE_MENUTOPIC); // set background color for normal menu topics
   
   //FIXME duplicate mnemonic test not implemented
#if defined(DUMP_MENU_LIST)
 //   dumpMenuEntry(name);
#endif // DUMP_MENU_LIST
#ifdef DEBUG
//    test_duplicate_mnemonics(prvt.menu_deep, name, mnemonic);
#endif

//FIXME menu button callbacks not implemented
//    AW_label_in_awar_list(this, button, name);
//    AW_cb_struct *cbs = new AW_cb_struct(this, f, cd1, cd2, helpText);
//    XtAddCallback(button, XmNactivateCallback,
//                  (XtCallbackProc) AW_server_callback,
//                  (XtPointer) cbs);
//
//    cbs->id = strdup(topic_id);
//    root->define_remote_command(cbs);
//    root->make_sensitive(button, mask);
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
    
    //create new menu item with attached submenu
    GtkMenu *submenu  = GTK_MENU(gtk_menu_new());
    GtkMenuItem *item = GTK_MENU_ITEM(gtk_menu_item_new_with_label(name));
    //FIXME mnemonics missing
    gtk_menu_item_set_submenu(item, GTK_WIDGET(submenu));
    
    //append the new submenu to the current menu shell
    gtk_menu_shell_append(prvt->menus.top(), GTK_WIDGET(item));
    
    //use the new submenu as current menu shell.
    prvt->menus.push(GTK_MENU_SHELL(submenu));
    
    
    GTK_PARTLY_IMPLEMENTED;
    //TuneBackground(p_w->menu_bar[p_w->menu_deep], TUNE_SUBMENU); // set background color for submenus
    // (Note: This must even be called if TUNE_SUBMENU is 0!
    //        Otherwise several submenus get the TUNE_MENUTOPIC color)

//    #if defined(DUMP_MENU_LIST)
//        dumpOpenSubMenu(name);
//    #endif // DUMP_MENU_LIST
//    #ifdef DEBUG
//        open_test_duplicate_mnemonics(prvt->menu_deep+1, name, mnemonic);
//    #endif
//
//    // create shell for Pull-Down
//    shell = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
//            p_w->menu_bar[p_w->menu_deep],
//            XmNwidth, 1,
//            XmNheight, 1,
//            XmNallowShellResize, true,
//            XmNoverrideRedirect, true,
//            NULL);
//
//    // create row column in Pull-Down shell
//
//    p_w->menu_bar[p_w->menu_deep+1] = XtVaCreateWidget("menu_row_column",
//            xmRowColumnWidgetClass, shell,
//            XmNrowColumnType, XmMENU_PULLDOWN,
//            XmNtearOffModel, XmTEAR_OFF_ENABLED,
//            NULL);
//
//    // create label in menu bar
//    if (mnemonic && *mnemonic && strchr(name, mnemonic[0])) {
//        // if mnemonic is "" -> Cannot convert string "" to type KeySym
//        Label = XtVaCreateManagedWidget("menu1_top_b1",
//                xmCascadeButtonWidgetClass, p_w->menu_bar[p_w->menu_deep],
//                RES_CONVERT(XmNlabelString, name),
//                                         RES_CONVERT(XmNmnemonic, mnemonic),
//                                         XmNsubMenuId, p_w->menu_bar[p_w->menu_deep+1],
//                                         XmNbackground, _at->background_color, NULL);
//    }
//    else {
//        Label = XtVaCreateManagedWidget("menu1_top_b1",
//        xmCascadeButtonWidgetClass,
//        p_w->menu_bar[p_w->menu_deep],
//        RES_CONVERT(XmNlabelString, name),
//        XmNsubMenuId, p_w->menu_bar[p_w->menu_deep+1],
//        XmNbackground, _at->background_color,
//        NULL);
//    }
//
//    if (p_w->menu_deep < AW_MAX_MENU_DEEP-1) p_w->menu_deep++;
//
//    root->make_sensitive(Label, mask);
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

void AW_window::update_text_field(GtkWidget *widget, const char *var_value) {
   // XtVaSetValues(widget, XmNvalue, var_value, NULL);
    GTK_NOT_IMPLEMENTED;
}

void AW_window::update_toggle(GtkWidget *widget, const char *var, AW_CL cd_toggle_data) {
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

void AW_window::update_input_field(GtkWidget *widget, const char *var_value) {
    //XtVaSetValues(widget, XmNvalue, var_value, NULL);
    GTK_NOT_IMPLEMENTED;
}

void AW_window::refresh_option_menu(AW_option_menu_struct *oms) {
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

void AW_window::refresh_toggle_field(int toggle_field_number) {
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
    if (aww->get_root()->color_mode == 0) { // mono colr
        device->clear(-1);
    }
    device->set_offset(AW::Vector(-xfig->minx, -xfig->miny));
    xfig->print(device);
}




void AW_window::load_xfig(const char *file, bool resize /*= true*/){

    AW_xfig *xfig;

    if (file)   xfig = new AW_xfig(file, get_root()->font_width, get_root()->font_height);
    else        xfig = new AW_xfig(get_root()->font_width, get_root()->font_height); // create an empty xfig

    xfig_data = xfig;


    //expose callback is no longer needed. gtk takes care of that.
   // set_expose_callback(AW_INFO_AREA, (AW_CB)AW_xfigCB_info_area, (AW_CL)xfig_data, 0);

    xfig->create_gcs(get_device(AW_INFO_AREA), get_root()->color_mode ? 8 : 1); //FIXME remove color mode.

    int xsize = xfig->maxx - xfig->minx;
    int ysize = xfig->maxy - xfig->miny;

    if (xsize > _at.max_x_size) _at.max_x_size = xsize;
    if (ysize > _at.max_y_size) _at.max_y_size = ysize;

    AW_device *device = get_device(AW_INFO_AREA);

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
        set_window_size(_at.max_x_size, _at.max_y_size);

        //FIXME this call should not be necessary. The sceen size should have been set by the resize_callback...
        device->get_common()->set_screen_size(_at.max_x_size, _at.max_y_size);

    }

    device->reset();
    device->set_offset(AW::Vector(-xfig->minx, -xfig->miny));

    //print the whole xfig to the area once.
    //gtk takes care of refreshing etc.
    xfig_data->print(get_device(AW_INFO_AREA));


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
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_bottom_area_height(int /*height*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_expose_callback(AW_area area, void (*f)(AW_window*, AW_CL, AW_CL), AW_CL cd1 /*= 0*/, AW_CL cd2 /*= 0*/) {
//    AW_area_management *aram = prvt->areas[area];
//    if (aram) aram->set_expose_callback(this, f, cd1, cd2);
    GTK_NOT_IMPLEMENTED;
    //FIXME area management does no longer have an expose callback. Remove this if absolutely sure that it is not needed.
}

void AW_window::set_focus_callback(void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GTK_NOT_IMPLEMENTED;
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

void AW_window::set_input_callback(AW_area /*area*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/ /*= 0*/, AW_CL /*cd2*/ /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::set_motion_callback(AW_area /*area*/, void (*/*f*/)(AW_window*, AW_CL, AW_CL), AW_CL /*cd1*/ /*= 0*/, AW_CL /*cd2*/ /*= 0*/) {
    GTK_NOT_IMPLEMENTED;
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

void AW_window::store_at_size_and_attach(AW_at_size */*at_size*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::tell_scrolled_picture_size(AW_world /*rectangle*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::update_label(GtkWidget* /*widget*/, const char */*var_value*/) {
    GTK_NOT_IMPLEMENTED;
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

void AW_window::tell_scrolled_picture_size(AW_screen_area /*rectangle*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::d_callback(void (*/*f*/)(AW_window*)) {
    GTK_NOT_IMPLEMENTED;
}

AW_window::AW_window() {
    color_table_size = 0;
    color_table = NULL;
    
    prvt = new AW_window::AW_window_gtk();
    for(int i = 0; i < AW_MAX_AREA; i++ ) {
        prvt->areas.push_back(NULL);
    }
}

AW_window::~AW_window() {
    //FIXME delete prvt
}


//aw_window_menu
//FIXME move to own file
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
//    aww->insert_help_topic("Click here and then on the questionable button/menu/...", "P", 0,
//                           AWM_ALL, (AW_CB)AW_help_entry_pressed, 0, 0);
    GTK_NOT_IMPLEMENTED;
}

void AW_window_menu_modes::init(AW_root *root_in, const char *wid, const char *windowname, int width, int height) {

    GtkWidget *vbox;
    GtkWidget *hbox;

    //const char *help_button   = "HELP";
    //const char *help_mnemonic = "H";

#if defined(DUMP_MENU_LIST)
    initMenuListing(windowname);
#endif // DUMP_MENU_LIST
    root = root_in; // for macro
    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    //p_w->shell = aw_create_shell(this, true, true, width, height, posx, posy);

    prvt->window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    gtk_window_set_title(prvt->window, window_name);
    gtk_window_set_default_size(prvt->window, width, height);
    vbox = gtk_vbox_new(false, 1);//FIXME remove constant
    gtk_container_add(GTK_CONTAINER(prvt->window), vbox);
    
    
//    XtVaCreateManagedWidget("mainWindow1",
//                                          xmMainWindowWidgetClass, p_w->shell,
//                                          NULL);

    GTK_PARTLY_IMPLEMENTED;
    
    menu_bar = (GtkMenuBar*) gtk_menu_bar_new();
    prvt->menus.push(GTK_MENU_SHELL(menu_bar));//The menu bar is the top level menu shell.

//    p_w->menu_bar[0] = XtVaCreateManagedWidget("menu1", xmRowColumnWidgetClass,
//                                               main_window,
//                                               XmNrowColumnType, XmMENU_BAR,
//                                               NULL);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(menu_bar), false,
                       false, //has no effect if the third parameter is false
                       2); //FIXME constant value

    hbox = gtk_hbox_new(false, 1); //FIXME constant
    gtk_container_add(GTK_CONTAINER(vbox), hbox);
    
    
    
//
//    // create shell for help-cascade
//    help_popup = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
//                                      p_w->menu_bar[0],
//                                      XmNwidth, 1,
//                                      XmNheight, 1,
//                                      XmNallowShellResize, true,
//                                      XmNoverrideRedirect, true,
//                                      NULL);
//
//    // create row column in Pull-Down shell
//    p_w->help_pull_down = XtVaCreateWidget("menu_row_column",
//                                           xmRowColumnWidgetClass, help_popup,
//                                           XmNrowColumnType, XmMENU_PULLDOWN,
//                                           NULL);
//
//    // create HELP-label in menu bar
//    help_label = XtVaCreateManagedWidget("menu1_top_b1",
//                                         xmCascadeButtonWidgetClass, p_w->menu_bar[0],
//                                         RES_CONVERT(XmNlabelString, help_button),
//                                         RES_CONVERT(XmNmnemonic, help_mnemonic),
//                                         XmNsubMenuId, p_w->help_pull_down, NULL);
//    XtVaSetValues(p_w->menu_bar[0], XmNmenuHelpWidget, help_label, NULL);
//    root->make_sensitive(help_label, AWM_ALL);
//
//    form1 = XtVaCreateManagedWidget("form1",
//                                    xmFormWidgetClass,
//                                    main_window,
//                                    // XmNwidth, width,
//                                    // XmNheight, height,
//                                    XmNresizePolicy, XmRESIZE_NONE,
//                                    // XmNx, 0,
//                                    // XmNy, 0,
//                                    NULL);
//
//    p_w->mode_area = XtVaCreateManagedWidget("mode area",
//                                             xmDrawingAreaWidgetClass,
//                                             form1,
//                                             XmNresizePolicy, XmRESIZE_NONE,
//                                             XmNwidth, 38,
//                                             XmNheight, height,
//                                             XmNx, 0,
//                                             XmNy, 0,
//                                             XmNleftOffset, 0,
//                                             XmNtopOffset, 0,
//                                             XmNbottomAttachment, XmATTACH_FORM,
//                                             XmNleftAttachment, XmATTACH_POSITION,
//                                             XmNtopAttachment, XmATTACH_POSITION,
//                                             XmNmarginHeight, 2,
//                                             XmNmarginWidth, 1,
//                                             NULL);
//
//    separator = XtVaCreateManagedWidget("separator",
//                                        xmSeparatorWidgetClass,
//                                        form1,
//                                        XmNx, 37,
//                                        XmNshadowThickness, 4,
//                                        XmNorientation, XmVERTICAL,
//                                        XmNbottomAttachment, XmATTACH_FORM,
//                                        XmNtopAttachment, XmATTACH_FORM,
//                                        XmNleftAttachment, XmATTACH_NONE,
//                                        XmNleftWidget, NULL,
//                                        XmNrightAttachment, XmATTACH_NONE,
//                                        XmNleftOffset, 70,
//                                        XmNleftPosition, 0,
//                                        NULL);
//
//    form2 = XtVaCreateManagedWidget("form2",
//                                    xmFormWidgetClass,
//                                    form1,
//                                    XmNwidth, width,
//                                    XmNheight, height,
//                                    XmNtopOffset, 0,
//                                    XmNbottomOffset, 0,
//                                    XmNleftOffset, 0,
//                                    XmNrightOffset, 0,
//                                    XmNrightAttachment, XmATTACH_FORM,
//                                    XmNbottomAttachment, XmATTACH_FORM,
//                                    XmNleftAttachment, XmATTACH_WIDGET,
//                                    XmNleftWidget, separator,
//                                    XmNtopAttachment, XmATTACH_POSITION,
//                                    XmNresizePolicy, XmRESIZE_NONE,
//                                    XmNx, 0,
//                                    XmNy, 0,
//                                    NULL);

//
//    p_w->scroll_bar_horizontal = XtVaCreateManagedWidget("scroll_bar_horizontal",
//                                                         xmScrollBarWidgetClass,
//                                                         form2,
//                                                         XmNheight, 15,
//                                                         XmNminimum, 0,
//                                                         XmNmaximum, AW_SCROLL_MAX,
//                                                         XmNincrement, 10,
//                                                         XmNsliderSize, AW_SCROLL_MAX,
//                                                         XmNrightAttachment, XmATTACH_FORM,
//                                                         XmNbottomAttachment, XmATTACH_FORM,
//                                                         XmNbottomOffset, 0,
//                                                         XmNleftAttachment, XmATTACH_FORM,
//                                                         XmNtopAttachment, XmATTACH_NONE,
//                                                         XmNorientation, XmHORIZONTAL,
//                                                         XmNrightOffset, 18,
//                                                         NULL);
//
//    p_w->scroll_bar_vertical = XtVaCreateManagedWidget("scroll_bar_vertical",
//                                                       xmScrollBarWidgetClass,
//                                                       form2,
//                                                       XmNwidth, 15,
//                                                       XmNminimum, 0,
//                                                       XmNmaximum, AW_SCROLL_MAX,
//                                                       XmNincrement, 10,
//                                                       XmNsliderSize, AW_SCROLL_MAX,
//                                                       XmNrightAttachment, XmATTACH_FORM,
//                                                       XmNbottomAttachment, XmATTACH_WIDGET,
//                                                       XmNbottomWidget, p_w->scroll_bar_horizontal,
//                                                       XmNbottomOffset, 3,
//                                                       XmNleftOffset, 3,
//                                                       XmNrightOffset, 3,
//                                                       XmNleftAttachment, XmATTACH_NONE,
//                                                       XmNtopAttachment, XmATTACH_WIDGET,
//                                                       XmNtopWidget, INFO_WIDGET,
//                                                       NULL);
//
//    p_w->frame = XtVaCreateManagedWidget("draw_area",
//                                         xmFrameWidgetClass,
//                                         form2,
//                                         XmNshadowType, XmSHADOW_IN,
//                                         XmNshadowThickness, 2,
//                                         XmNleftOffset, 3,
//                                         XmNtopOffset, 3,
//                                         XmNbottomOffset, 3,
//                                         XmNrightOffset, 3,
//                                         XmNbottomAttachment, XmATTACH_WIDGET,
//                                         XmNbottomWidget, p_w->scroll_bar_horizontal,
//                                         XmNtopAttachment, XmATTACH_FORM,
//                                         XmNtopOffset, 0,
//                                         XmNleftAttachment, XmATTACH_FORM,
//                                         XmNrightAttachment, XmATTACH_WIDGET,
//                                         XmNrightWidget, p_w->scroll_bar_vertical,
//                                         NULL);
//
    
    GtkWidget* drawing_area = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(vbox), drawing_area);
    gtk_widget_realize(GTK_WIDGET(drawing_area));

    prvt->areas[AW_MIDDLE_AREA] = new AW_area_management(root, drawing_area, drawing_area); //FIXME form should be a frame around the area.
    
    
    GtkWidget* drawing_area_bottom = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(hbox), drawing_area_bottom);
    gtk_widget_realize(GTK_WIDGET(drawing_area_bottom));
  
    prvt->areas[AW_BOTTOM_AREA] = new AW_area_management(root, drawing_area_bottom, drawing_area_bottom); //FIXME form should be a frame around the area.
    
    GtkWidget* drawing_area_info = gtk_drawing_area_new();
    gtk_container_add(GTK_CONTAINER(hbox), drawing_area_info);
    gtk_widget_realize(GTK_WIDGET(drawing_area_info));
  
    prvt->areas[AW_INFO_AREA] = new AW_area_management(root, drawing_area_info, drawing_area_info); //FIXME form should be a frame around the area.


    
    
//
//    XmMainWindowSetAreas(main_window, p_w->menu_bar[0], (Widget) NULL, (Widget) NULL, (Widget) NULL, form1);

//    aw_realize_widget(this);

    gtk_widget_realize(GTK_WIDGET(prvt->window));
    create_devices();
    aw_create_help_entry(this);
    create_window_variables();
    
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

    color_table[colnum] = root->alloc_named_data_color(colorname);
    
    if (colnum == AW_DATA_BG) {
        AW_area_management* pMiddleArea = prvt->areas[AW_MIDDLE_AREA];
        if(pMiddleArea) {
            gtk_widget_modify_bg(pMiddleArea->get_area(),GTK_STATE_NORMAL, &root->getColor(color_table[colnum]));
            //FIXME no idea if this works :D
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

void AW_window::update_scrollbar_settings_from_awars(AW_orientation orientation) {
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


//FIXME comment
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



void AW_cb_struct::run_callback() {

    GTK_PARTLY_IMPLEMENTED;

    if (next) next->run_callback();                 // callback the whole list
    if (!f) return;                                 // run no callback

    //FIXME this is a very simplified version of the original run_callback. See original code below and fix it.
    useraction_init();
    f(aw, cd1, cd2);
    useraction_done(aw);

//    AW_root *root = aw->get_root();
//    if (root->disable_callbacks) {
//        // some functions (namely aw_message, aw_input, aw_string_selection and aw_file_selection)
//        // have to disable most callbacks, because they are often called from inside these callbacks
//        // (e.g. because some exceptional condition occurred which needs user interaction) and if
//        // callbacks weren't disabled, a recursive deadlock occurs.
//
//        // the following callbacks are allowed even if disable_callbacks is true
//
//        bool isModalCallback = (f == AW_CB(message_cb) ||
//                                f == AW_CB(input_history_cb) ||
//                                f == AW_CB(input_cb) ||
//                                f == AW_CB(file_selection_cb));
//
//        bool isPopdown = (f == AW_CB(AW_POPDOWN));
//        bool isHelp    = (f == AW_CB(AW_POPUP_HELP));
//        bool allow     = isModalCallback || isHelp || isPopdown;
//
//        bool isInfoResizeExpose = false;
//
//        if (!allow) {
//            isInfoResizeExpose = aw->is_expose_callback(AW_INFO_AREA, f) || aw->is_resize_callback(AW_INFO_AREA, f);
//            allow              = isInfoResizeExpose;
//        }
//
//        if (!allow) {
//            // do not change position of modal dialog, when one of the following callbacks happens - just raise it
//            // (other callbacks like pressing a button, will position the modal dialog under mouse)
//            bool onlyRaise =
//                aw->is_expose_callback(AW_MIDDLE_AREA, f) ||
//                aw->is_focus_callback(f) ||
//                root->is_focus_callback((AW_RCB)f) ||
//                aw->is_resize_callback(AW_MIDDLE_AREA, f);
//
//            if (root->current_modal_window) {
//                AW_window *awmodal = root->current_modal_window;
//
//                AW_PosRecalc prev = awmodal->get_recalc_pos_atShow();
//                if (onlyRaise) awmodal->recalc_pos_atShow(AW_KEEP_POS);
//                awmodal->activate();
//                awmodal->recalc_pos_atShow(prev);
//            }
//            else {
//                aw_message("Internal error (callback suppressed when no modal dialog active)");
//                aw_assert(0);
//            }
//#if defined(TRACE_CALLBACKS)
//            printf("suppressing callback %p\n", f);
//#endif // TRACE_CALLBACKS
//            return; // suppress the callback!
//        }
//#if defined(TRACE_CALLBACKS)
//        else {
//            if (isModalCallback) printf("allowed modal callback %p\n", f);
//            else if (isPopdown) printf("allowed AW_POPDOWN\n");
//            else if (isHelp) printf("allowed AW_POPUP_HELP\n");
//            else if (isInfoResizeExpose) printf("allowed expose/resize infoarea\n");
//            else printf("allowed other (unknown) callback %p\n", f);
//        }
//#endif // TRACE_CALLBACKS
//    }
//    else {
//#if defined(TRACE_CALLBACKS)
//        printf("Callbacks are allowed (executing %p)\n", f);
//#endif // TRACE_CALLBACKS
//    }
//
//    useraction_init();
//
//    if (f == AW_POPUP) {
//        if (pop_up_window) { // already exists
//            pop_up_window->activate();
//        }
//        else {
//            AW_PPP g = (AW_PPP)cd1;
//            if (g) {
//                pop_up_window = g(aw->get_root(), cd2, 0);
//                pop_up_window->activate();
//            }
//            else {
//                aw_message("not implemented -- please report to devel@arb-home.de");
//            }
//        }
//        if (pop_up_window && p_aww(pop_up_window)->popup_cb)
//            p_aww(pop_up_window)->popup_cb->run_callback();
//    }
//    else {
//        f(aw, cd1, cd2);
//    }
//
//    useraction_done(aw);
}


