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
#include "aw_xfig.hxx"
#include "aw_root.hxx"
#include "aw_device.hxx"
#include "aw_at.hxx"
#include <arbdb.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkfixed.h>
#include <gtk/gtkbutton.h>
#include "aw_msg.hxx"

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

void AW_window::at_shift(int /*x*/, int /*y*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::at_newline(){
    GTK_NOT_IMPLEMENTED;
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

void AW_window::auto_space(int /*xspace*/, int /*yspace*/){
    GTK_NOT_IMPLEMENTED;
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
    GTK_NOT_IMPLEMENTED;
}



void AW_window::_get_area_size(AW_area /*area*/, AW_screen_area */*square*/) {
    GTK_NOT_IMPLEMENTED;
}


void AW_window::clear_option_menu(AW_option_menu_struct */*oms*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::TuneOrSetBackground(Widget w, const char *color, int modStrength) {
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

void AW_window::TuneBackground(Widget w, int modStrength) {
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

static void aw_attach_widget(GtkWidget* w, AW_at& _at, int default_width = -1) {
    GTK_NOT_IMPLEMENTED;
}


void AW_label_in_awar_list(AW_window *aww, GtkWidget* widget, const char *str) {
    GTK_NOT_IMPLEMENTED;
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

void AW_window::set_background(const char *colorname, Widget parentWidget) {
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


void AW_window::_set_activate_callback(void *widget) {
    GTK_NOT_IMPLEMENTED;
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
//    TuneOrSetBackground(_at.attach_any ? prvt.areas[AW_INFO_AREA]->get_form() : prvt.areas[AW_INFO_AREA]->get_area(), // set background for buttons / text displays
//                        color,
//                        _callback ? TUNE_BUTTON : 0);


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

    //GtkWidget* parent_widget = (_at.attach_any) ? prvt.areas[AW_INFO_AREA]->get_form() : prvt.areas[AW_INFO_AREA]->get_area();
    //FIXME quick hack
    GtkWidget* parent_widget = GTK_WIDGET(prvt.fixed_size_area);

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


        //old motif code for reference
        //        tmp_label = XtVaCreateManagedWidget("label",
        //                                            xmLabelWidgetClass,
        //                                            parent_widget,
        //                                            XmNwidth, (int)(width_of_label + 2),
        //                                            RES_LABEL_CONVERT(_at.label_for_inputfield),
        //                                            XmNrecomputeSize, false,
        //                                            XmNalignment, XmALIGNMENT_BEGINNING,
        //                                            XmNfontList, p_global->fontlist,
        //                                            XmNx, (int)(x_label),
        //                                            XmNy, (int)(y_label),
        //                                            NULL);
        //these macros are used to determine wether a pixmap or text should be used on the label
        //#define RES_LABEL_CONVERT_AWW(str,aww)                             \
        //    XmNlabelType, (str[0]=='#') ? XmPIXMAP : XmSTRING,             \
        //    XtVaTypedArg, (str[0]=='#') ? XmNlabelPixmap : XmNlabelString, \
        //    XmRString,                                                     \
        //    aw_str_2_label(str, aww),                                      \
        //    strlen(aw_str_2_label(str, aww))+1
        //
        //#define RES_LABEL_CONVERT(str) RES_LABEL_CONVERT_AWW(str, this)


        if (_at.attach_any) {
            aw_attach_widget(tmp_label, _at);
        }

        AW_label_in_awar_list(this, tmp_label, _at.label_for_inputfield);
    }

    _at.x_for_next_button = x_button;
    _at.y_for_next_button = y_button;

    GtkWidget* fatherwidget = parent_widget; // used as father for button below
    if (_at.highlight) {
        if (_at.attach_any) {
            #if defined(DEBUG)
                        printf("Attaching highlighted buttons does not work - "
                               "highlight ignored for button '%s'!\n", buttonlabel);
            #endif // DEBUG
                        _at.highlight = false;
        }
        else {
            //FIXME not implemented
//            int shadow_offset = _at.shadow_thickness;
//            int x_shadow      = x_button - shadow_offset;
//            int y_shadow      = y_button - shadow_offset;
//
//            fatherwidget = XtVaCreateManagedWidget("draw_area",
//                                                   xmFrameWidgetClass,
//                                                   INFO_WIDGET,
//                                                   XmNx, (int)(x_shadow),
//                                                   XmNy, (int)(y_shadow),
//                                                   XmNshadowType, XmSHADOW_IN,
//                                                   XmNshadowThickness, _at.shadow_thickness,
//                                                   NULL);
        }
    }

    GtkWidget* buttonOrLabel  = 0;

    {
        if (_callback) {//button

            buttonOrLabel = gtk_button_new();

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

    //FIXME button block not implemented. what is it"s prpose?
//    {
//        Widget  toRecenter   = 0;
//        int     recenterSize = 0;
//
//        if (!height || !width) {
//            // ask motif for real button size
//            Widget ButOrHigh = _at.highlight ? fatherwidget : button;
//            XtVaGetValues(ButOrHigh, XmNheight, &height, XmNwidth, &width, NULL);
//
//            if (let_motif_choose_size) {
//                if (_at.correct_for_at_center) {
//                    toRecenter   = ButOrHigh;
//                    recenterSize = width;
//                }
//                width = 0;          // ignore the used size (because it may use more than the window size)
//            }
//        }
//
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
//    }

    _at.correct_for_at_center = org_correct_for_at_center; // restore original justification
    _at.y_for_next_button     = org_y_for_next_button;
    //FIXME prvt.toggle_field not set to button. What is its purpose anyway?
   // p_w->toggle_field = button;
    this->_set_activate_callback((void *)buttonOrLabel);
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





void AW_window::create_autosize_button(const char */*macro_name*/, AW_label /*label*/, const char */*mnemonic*/ /* = 0*/, unsigned /*xtraSpace*/ /* = 1 */){

}
Widget AW_window::get_last_widget() const{
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_window::create_toggle(const char */*awar_name*/){
    GTK_NOT_IMPLEMENTED;
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



void AW_window::create_menu(AW_label /*name*/, const char */*mnemonic*/, AW_active /*mask*/ /*= AWM_ALL*/){
    GTK_NOT_IMPLEMENTED;
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
}

int AW_window::get_at_yposition() const {
    GTK_NOT_IMPLEMENTED;
}

AW_device_click *AW_window::get_click_device(AW_area /*area*/, int /*mousex*/, int /*mousey*/, AW_pos /*max_distance_linei*/,
                                  AW_pos /*max_distance_texti*/, AW_pos /*radi*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}
AW_device *AW_window::get_device(AW_area area){
    AW_area_management *aram   = prvt.areas[area];
    arb_assert(NULL != aram);
    return (AW_device *)aram->get_screen_device();
}

void AW_window::get_event(AW_event */*eventi*/) const{

}

AW_device_print *AW_window::get_print_device(AW_area /*area*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_device_size *AW_window::get_size_device(AW_area /*area*/){
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_window::get_window_size(int& /*width*/, int& /*height*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::help_text(const char */*id*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::hide(){
    GTK_NOT_IMPLEMENTED;
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

void AW_window::insert_menu_topic(const char *id, AW_label /*name*/, const char */*mnemonic*/, const char */*help_text_*/, AW_active /*mask*/, AW_CB /*cb*/, AW_CL /*cd1*/, AW_CL /*cd2*/){
    GTK_NOT_IMPLEMENTED;
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

void AW_window::insert_sub_menu(AW_label /*name*/, const char */*mnemonic*/, AW_active mask /*= AWM_ALL*/){
    GTK_NOT_IMPLEMENTED;
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

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
        set_window_size(_at.max_x_size, _at.max_y_size);
    }


    AW_device *device = get_device(AW_INFO_AREA);
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
//    AW_area_management *aram = prvt.areas[area];
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
    AW_area_management *aram = prvt.areas[area];
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
    gtk_window_set_default_size(prvt.window, width, height);
}

void AW_window::set_window_title(const char */*title*/){
    GTK_NOT_IMPLEMENTED;
}

void AW_window::shadow_width (int /*shadow_thickness*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::show() {
    arb_assert(NULL != prvt.window);
    gtk_widget_show(GTK_WIDGET(prvt.window));

}

void AW_window::store_at_size_and_attach(AW_at_size */*at_size*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::tell_scrolled_picture_size(AW_world /*rectangle*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_window::update_label(Widget /*widget*/, const char */*var_value*/) {
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

    GTK_NOT_IMPLEMENTED;

}

AW_window::~AW_window() {
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

void AW_window_menu_modes::init(AW_root */*root*/, const char */*wid*/, const char */*windowname*/, int /*width*/, int /*height*/) {
    GTK_NOT_IMPLEMENTED;
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
    prvt.window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
    prvt.fixed_size_area = GTK_FIXED(gtk_fixed_new());
    gtk_container_add(GTK_CONTAINER(prvt.window), GTK_WIDGET(prvt.fixed_size_area));

    //Creates the GDK (windowing system) resources associated with a widget.
    //This is done as early as possible because xfig drawing relies on the gdk stuff.
    gtk_widget_realize(GTK_WIDGET(prvt.window));
    gtk_widget_realize(GTK_WIDGET(prvt.fixed_size_area));
    gtk_widget_show(GTK_WIDGET(prvt.fixed_size_area));


    int width  = 100;                               // this is only the minimum size!
    int height = 100;
    int posx   = 50;
    int posy   = 50;

    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    gtk_window_set_resizable(prvt.window, true);
    gtk_window_set_title(prvt.window, window_name);

    GTK_PARTLY_IMPLEMENTED;



   // p_w->shell = aw_create_shell(this, true, true, width, height, posx, posy);

    // add this to disable resize or maximize in simple dialogs (avoids broken layouts)
    // XtVaSetValues(p_w->shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE, NULL);

//    Widget form1 = XtVaCreateManagedWidget("forms", xmFormWidgetClass,
//            p_w->shell,
//            NULL);

    //FIXME what exactly is the difference between area and form?
    prvt.areas[AW_INFO_AREA] = new AW_area_management(root, GTK_WIDGET(prvt.window), GTK_WIDGET(prvt.window));


  //  aw_realize_widget(this);
    create_devices();
}

void AW_window::create_devices() {

    GTK_PARTLY_IMPLEMENTED;

    unsigned long background_color;
    if (prvt.areas[AW_INFO_AREA]) {
        prvt.areas[AW_INFO_AREA]->create_devices(this, AW_INFO_AREA);
//        XtVaGetValues(p_w->areas[AW_INFO_AREA]->get_area(), XmNbackground, &background_color, NULL);
//        p_global->color_table[AW_WINDOW_DRAG] = background_color ^ p_global->color_table[AW_WINDOW_FG];
    }
    if (prvt.areas[AW_MIDDLE_AREA]) {
        prvt.areas[AW_MIDDLE_AREA]->create_devices(this, AW_MIDDLE_AREA);
    }
    if (prvt.areas[AW_BOTTOM_AREA]) {
        prvt.areas[AW_BOTTOM_AREA]->create_devices(this, AW_BOTTOM_AREA);
    }
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


