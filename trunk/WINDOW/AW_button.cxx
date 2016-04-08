// =============================================================== //
//                                                                 //
//   File      : AW_button.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_at.hxx"
#include "aw_select.hxx"
#include "aw_awar.hxx"
#include "aw_window_Xm.hxx"
#include "aw_msg.hxx"
#include "aw_root.hxx"
#include "aw_xargs.hxx"
#include "aw_varupdate.hxx"

#include <arb_algo.h>
#include <ad_cb.h>

#include <Xm/Frame.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ScrolledW.h>
#include <Xm/Scale.h>

void AW_variable_update_callback(Widget /*wgt*/, XtPointer variable_update_struct, XtPointer call_data) {
    VarUpdateInfo *vui = (VarUpdateInfo *) variable_update_struct;
    aw_assert(vui);

    vui->change_from_widget(call_data);
}

struct TrackedAwarChange {
    AW_awar *awar;
    bool     changed;

    TrackedAwarChange(AW_awar *awar_) : awar(awar_), changed(false) {}
};

static void track_awar_change_cb(GBDATA *IF_ASSERTION_USED(gbd), TrackedAwarChange *tac, GB_CB_TYPE IF_ASSERTION_USED(cb_type)) {
    aw_assert(cb_type == GB_CB_CHANGED);
    aw_assert(tac->awar->gb_var == gbd);
    tac->changed = true;
}

#define SCALER_MIN_VALUE 0
#define SCALER_MAX_VALUE 1000

static int calculate_scaler_value(AW_awar *awar, AW_ScalerType scalerType) {
    float modAwarRel  = AW_ScalerTransformer(scalerType).awar2scaler(awar);
    int   scalerValue = SCALER_MIN_VALUE + modAwarRel * (SCALER_MAX_VALUE-SCALER_MIN_VALUE);

    return scalerValue;
}

static void write_scalervalue_to_awar(int scalerVal, AW_awar *awar, AW_ScalerType scalerType) {
    float scaleRel = scalerVal/double(SCALER_MAX_VALUE-SCALER_MIN_VALUE);
    aw_assert(scaleRel>=0.0 && scaleRel<=1.0);

    float aval = AW_ScalerTransformer(scalerType).scaler2awar(scaleRel, awar);

    switch (awar->variable_type) {
        case AW_FLOAT:
            awar->write_float(aval);
            break;

        case AW_INT:
            awar->write_int(aval);
            break;

        default:
            GBK_terminatef("Unsupported awar type %i in write_scalervalue_to_awar", int(awar->variable_type));
            break;
    }
}

void VarUpdateInfo::change_from_widget(XtPointer call_data) {
    AW_cb::useraction_init();
    
    GB_ERROR  error = NULL;
    AW_root  *root  = awar->root;

    if (root->value_changed) {
        root->changer_of_variable = widget;
    }

    TrackedAwarChange tac(awar);
    if (root->is_tracking()) {
        // add a callback which writes macro-code (BEFORE any other callback happens; last added, first calledback)
        GB_transaction ta(awar->gb_var);
        GB_add_callback(awar->gb_var, GB_CB_CHANGED, makeDatabaseCallback(track_awar_change_cb, &tac));
    }

    bool run_cb = true;
    switch (widget_type) {
        case AW_WIDGET_INPUT_FIELD:
        case AW_WIDGET_TEXT_FIELD:
            if (!root->value_changed) {
                run_cb = false;
            }
            else {
                char *new_text = XmTextGetString((widget));
                error          = awar->write_as_string(new_text);
                XtFree(new_text);
            }
            break;

        case AW_WIDGET_TOGGLE:
            root->changer_of_variable = 0;
            error = awar->toggle_toggle();
            break;

        case AW_WIDGET_TOGGLE_FIELD:
            if (XmToggleButtonGetState(widget) == False) break; // no toggle is selected (?)
            // fall-through
        case AW_WIDGET_CHOICE_MENU:
            error = value.write_to(awar);
            break;

        case AW_WIDGET_SELECTION_LIST: {
            char *selected; {
                XmListCallbackStruct *xml = (XmListCallbackStruct*)call_data;
                XmStringGetLtoR(xml->item, XmSTRING_DEFAULT_CHARSET, &selected);
            }

            AW_selection_list_entry *entry = ts.sellist->list_table;
            while (entry && strcmp(entry->get_displayed(), selected) != 0) {
                entry = entry->next;
            }

            if (!entry) {   
                entry = ts.sellist->default_select; // use default selection
                if (!entry) GBK_terminate("no default specified for selection list"); // or die
            }
            entry->value.write_to(awar);
            XtFree(selected);
            break;
        }

        case AW_WIDGET_LABEL_FIELD:
            break;

        case AW_WIDGET_SCALER: {
            XmScaleCallbackStruct *xms = (XmScaleCallbackStruct*)call_data;
            write_scalervalue_to_awar(xms->value, awar, ts.scalerType);
            break;
        }
        default:
            GBK_terminatef("Unknown widget type %i in AW_variable_update_callback", widget_type);
            break;
    }

    if (root->is_tracking()) {
        {
            GB_transaction ta(awar->gb_var);
            GB_remove_callback(awar->gb_var, GB_CB_CHANGED, makeDatabaseCallback(track_awar_change_cb, &tac));
        }
        if (tac.changed) {
            root->track_awar_change(awar);
        }
    }

    if (error) {
        root->changer_of_variable = 0;
        awar->update();
        aw_message(error);
    }
    else {
        if (cbs && run_cb) cbs->run_callbacks();
        root->value_changed = false;

        if (GB_have_error()) aw_message(GB_await_error()); // show error exported by awar-change-callback
    }

    AW_cb::useraction_done(aw_parent);
}


static void AW_value_changed_callback(Widget /*wgt*/, XtPointer rooti, XtPointer /*call_data*/) {
    AW_root *root = (AW_root *)rooti;
    root->value_changed = true;
}

void aw_attach_widget(Widget w, AW_at *_at, int default_width) {
    short height = 0;
    short width = 0;
    if (!_at->to_position_exists) {
        XtVaGetValues(w, XmNheight, &height, XmNwidth, &width, NULL);
        if (default_width >0) width = default_width;

        switch (_at->correct_for_at_center) {
            case 0:             // left justified
                _at->to_position_x      = _at->x_for_next_button + width;
                break;
            case 1:             // centered
                _at->to_position_x      = _at->x_for_next_button + width/2;
                _at->x_for_next_button -= width/2;
                break;
            case 2:             // right justified
                _at->to_position_x      = _at->x_for_next_button;
                _at->x_for_next_button -= width;
                break;
        }
        _at->to_position_y = _at->y_for_next_button + height;
        _at->attach_x      = _at->attach_lx;
        _at->attach_y      = _at->attach_ly;
    }

#define MIN_RIGHT_OFFSET  10
#define MIN_BOTTOM_OFFSET 10

    aw_xargs args(4*2);

    if (_at->attach_x) {
        int right_offset = _at->max_x_size - _at->to_position_x;
        if (right_offset<MIN_RIGHT_OFFSET) {
            right_offset    = MIN_RIGHT_OFFSET;
            _at->max_x_size = _at->to_position_x+right_offset;
        }

        args.add(XmNrightAttachment, XmATTACH_FORM);
        args.add(XmNrightOffset,     right_offset);
    }
    else {
        args.add(XmNrightAttachment, XmATTACH_OPPOSITE_FORM);
        args.add(XmNrightOffset,     -_at->to_position_x);
    }

    if (_at->attach_lx) {
        args.add(XmNleftAttachment, XmATTACH_NONE);
        args.add(XmNwidth,          _at->to_position_x - _at->x_for_next_button);
    }
    else {
        args.add(XmNleftAttachment, XmATTACH_FORM);
        args.add(XmNleftOffset,     _at->x_for_next_button);
    }

    if (_at->attach_y) {
        int bottom_offset = _at->max_y_size - _at->to_position_y;
        if (bottom_offset<MIN_BOTTOM_OFFSET) {
            bottom_offset   = MIN_BOTTOM_OFFSET;
            _at->max_y_size = _at->to_position_y+bottom_offset;
        }

        args.add(XmNbottomAttachment, XmATTACH_FORM);
        args.add(XmNbottomOffset,     bottom_offset);
    }
    else {
        args.add(XmNbottomAttachment, XmATTACH_OPPOSITE_FORM);
        args.add(XmNbottomOffset,     - _at->to_position_y);
    }
    if (_at->attach_ly) {
        args.add(XmNtopAttachment, XmATTACH_NONE);
        args.add(XmNheight,        _at->to_position_y - _at->y_for_next_button);
    }
    else {
        args.add(XmNtopAttachment, XmATTACH_FORM);
        args.add(XmNtopOffset,     _at->y_for_next_button);
    }

    args.assign_to_widget(w);
}

const char *AW_get_pixmapPath(const char *pixmapName) {
    // const char *pixmapsDir = "pixmaps"; // normal pixmaps (as used in gtk branch)
    const char *pixmapsDir = "motifHack/pixmaps"; // see ../lib/motifHack/README

    return GB_path_in_ARBLIB(pixmapsDir, pixmapName);
}

static char *pixmapPath(const char *pixmapName) {
    return nulldup(AW_get_pixmapPath(pixmapName));
}


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

void AW_window::calculate_label_size(int *width, int *height, bool in_pixel, const char *non_at_label) {
    // in_pixel == true -> calculate size in pixels
    // in_pixel == false -> calculate size in characters

    const char *label_ = non_at_label ? non_at_label : _at->label_for_inputfield;
    if (label_) {
        calculate_textsize(label_, width, height);
        if (_at->length_of_label_for_inputfield) {
            *width = _at->length_of_label_for_inputfield;
        }
        if (in_pixel) {
            *width  = calculate_string_width(*width);
            *height = calculate_string_height(*height, 0);
        }
    }
    else {
        *width  = 0;
        *height = 0;
    }
}

Widget AW_window::get_last_widget() const {
    return p_global->get_last_widget();
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

void AW_window::create_autosize_button(const char *macro_name, AW_label buttonlabel, const  char *mnemonic, unsigned xtraSpace) {
    aw_assert(!AW_IS_IMAGEREF(buttonlabel));    // use create_button for graphical buttons!
    aw_assert(!_at->to_position_exists); // wont work if to-position exists

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
    short length_of_buttons = _at->length_of_buttons;
    short height_of_buttons = _at->height_of_buttons;

    _at->length_of_buttons = len+1;
    _at->height_of_buttons = height;
    create_button(macro_name, buttonlabel, mnemonic);
    _at->length_of_buttons = length_of_buttons;
    _at->height_of_buttons = height_of_buttons;
}

void AW_window::create_button(const char *macro_name, AW_label buttonlabel, const char */*mnemonic*/, const char *color) {
    // Create a button or text display.
    //
    // If a callback is bound via at->callback(), a button is created.
    // Otherwise a text display is created.
    //
    // if buttonlabel starts with '#' the rest of buttonlabel is used as name of pixmap file used for button
    // if buttonlabel contains a '/' it's interpreted as AWAR name and the button displays the content of the awar
    // otherwise buttonlabel is interpreted as button label (may contain '\n').
    //
    // Note 1: Button width 0 does not work together with labels!

    // Note 2: "color" may be specified for the button background (see TuneOrSetBackground for details)

    TuneOrSetBackground(_at->attach_any ? INFO_FORM : INFO_WIDGET, // set background for buttons / text displays
                        color,
                        _callback ? TUNE_BUTTON : 0);

#if defined(DUMP_BUTTON_CREATION)
    printf("------------------------------ Button '%s'\n", buttonlabel);
    printf("x_for_next_button=%i y_for_next_button=%i\n", _at->x_for_next_button, _at->y_for_next_button);
#endif // DUMP_BUTTON_CREATION

    if (_callback && ((long)_callback != 1))
    {
        if (macro_name) {
            if (macro_name[0] == '@') { // will not be recorded and not be inserted into action_hash
                _callback->id = strdup(macro_name); // -> no need to localize
            }
            else {
                _callback->id = GBS_global_string_copy("%s/%s", this->window_defaults_name, macro_name);
                get_root()->define_remote_command(_callback);
            }
        }
        else {
            _callback->id = 0;
        }
    }
#if defined(DEVEL_RALF) && 1
    else {
        aw_assert(!macro_name); // please pass NULL for buttons w/o callback
    }
#endif

#define SPACE_BEHIND_LABEL  10

#define BUTTON_TEXT_X_PADDING 4
#define BUTTON_TEXT_Y_PADDING 10

#define BUTTON_GRAPHIC_PADDING 12
#define FLAT_GRAPHIC_PADDING   4 // for buttons w/o callback

    bool is_graphical_button = AW_IS_IMAGEREF(buttonlabel);

#if defined(ASSERTION_USED)
    AW_awar *is_awar = is_graphical_button ? NULL : get_root()->label_is_awar(buttonlabel);
#endif // ASSERTION_USED

    int width_of_button = -1, height_of_button = -1;

    int width_of_label, height_of_label;
    calculate_label_size(&width_of_label, &height_of_label, true, 0);
    int width_of_label_and_spacer = _at->label_for_inputfield ? width_of_label+SPACE_BEHIND_LABEL : 0;

    bool let_motif_choose_size  = false;

    if (_at->to_position_exists) { // size has explicitly been specified in xfig -> calculate
        width_of_button  = _at->to_position_x - _at->x_for_next_button - width_of_label_and_spacer;
        height_of_button = _at->to_position_y - _at->y_for_next_button;
    }
    else if (_at->length_of_buttons) { // button width specified from client code
        width_of_button  = BUTTON_TEXT_X_PADDING + calculate_string_width(_at->length_of_buttons+1);

        if (!is_graphical_button) {
            if (_at->height_of_buttons) { // button height specified from client code
                height_of_button = BUTTON_TEXT_Y_PADDING + calculate_string_height(_at->height_of_buttons, 0);
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

    int x_label  = _at->x_for_next_button;
    int y_label  = _at->y_for_next_button;
    int x_button = x_label + width_of_label_and_spacer;
    int y_button = y_label;

    int org_correct_for_at_center = _at->correct_for_at_center; // store original justification
    int org_y_for_next_button     = _at->y_for_next_button; // store original y pos (modified while creating label)

    if (!let_motif_choose_size) { // don't correct position of button w/o known size
        // calculate justification manually

        int width_of_button_and_highlight = width_of_button + (_at->highlight ? 2*(_at->shadow_thickness+1)+1 : 0);
        int width_of_label_and_button     = width_of_label_and_spacer+width_of_button_and_highlight;

        if (_at->correct_for_at_center) { // not if left justified
            int shiftback = width_of_label_and_button; // shiftback for right justification
            if (_at->correct_for_at_center == 1) { // center justification
                shiftback /= 2;
            }
            x_label  -= shiftback;
            x_button -= shiftback;
        }

        // we already did the justification by calculating all positions manually, so..
        _at->correct_for_at_center = 0; // ..from now on act like "left justified"!
    }

    // correct label Y position
    if (_callback) {            // only if button is a real 3D-button
        y_label += (height_of_button-height_of_label)/2;
    }

    Widget parent_widget = (_at->attach_any) ? INFO_FORM : INFO_WIDGET;
    Widget tmp_label         = 0;

    if (_at->label_for_inputfield) {
        _at->x_for_next_button = x_label;
        _at->y_for_next_button = y_label;

        Label clientlabel(_at->label_for_inputfield, this);
        tmp_label = XtVaCreateManagedWidget("label",
                                            xmLabelWidgetClass,
                                            parent_widget,
                                            XmNwidth, (int)(width_of_label + 2),
                                            RES_LABEL_CONVERT(clientlabel),
                                            XmNrecomputeSize, false,
                                            XmNalignment, XmALIGNMENT_BEGINNING,
                                            XmNfontList, p_global->fontlist,
                                            XmNx, (int)(x_label),
                                            XmNy, (int)(y_label),
                                            NULL);

        if (_at->attach_any) aw_attach_widget(tmp_label, _at);
        AW_label_in_awar_list(this, tmp_label, _at->label_for_inputfield);
    }

    _at->x_for_next_button = x_button;
    _at->y_for_next_button = y_button;

    Widget fatherwidget = parent_widget; // used as father for button below
    if (_at->highlight) {
        if (_at->attach_any) {
#if defined(DEBUG)
            printf("Attaching highlighted buttons does not work - "
                   "highlight ignored for button '%s'!\n", buttonlabel);
#endif // DEBUG
            _at->highlight = false;
        }
        else {
            int shadow_offset = _at->shadow_thickness;
            int x_shadow      = x_button - shadow_offset;
            int y_shadow      = y_button - shadow_offset;

            fatherwidget = XtVaCreateManagedWidget("draw_area",
                                                   xmFrameWidgetClass,
                                                   INFO_WIDGET,
                                                   XmNx, (int)(x_shadow),
                                                   XmNy, (int)(y_shadow),
                                                   XmNshadowType, XmSHADOW_IN,
                                                   XmNshadowThickness, _at->shadow_thickness,
                                                   NULL);
        }
    }

    Widget button = 0;

    {
        aw_xargs args(9);
        args.add(XmNx, x_button);
        args.add(XmNy, y_button);

        args.add(XmNfontList,   (XtArgVal)p_global->fontlist);
        args.add(XmNbackground, _at->background_color);

        if (!let_motif_choose_size) {
            args.add(XmNwidth,  width_of_button);
            args.add(XmNheight, height_of_button);
        }

        Label buttonLabel(buttonlabel, this);
        if (_callback) {
            args.add(XmNshadowThickness, _at->shadow_thickness);
            args.add(XmNalignment,       XmALIGNMENT_CENTER);

            button = XtVaCreateManagedWidget("button", xmPushButtonWidgetClass, fatherwidget, RES_LABEL_CONVERT(buttonLabel), NULL);
        }
        else { // button w/o callback; (flat, not clickable)
            button = XtVaCreateManagedWidget("label", xmLabelWidgetClass, parent_widget, RES_LABEL_CONVERT(buttonLabel), NULL);
            args.add(XmNalignment, (org_correct_for_at_center == 1) ? XmALIGNMENT_CENTER : XmALIGNMENT_BEGINNING);
        }

        if (!_at->attach_any || !_callback) args.add(XmNrecomputeSize, false);
        args.assign_to_widget(button);
        if (_at->attach_any) aw_attach_widget(button, _at);

        if (_callback) {
            root->make_sensitive(button, _at->widget_mask);
        }
        else {
            aw_assert(_at->correct_for_at_center == 0);
            AW_JUSTIFY_LABEL(button, _at->correct_for_at_center); // @@@ strange, again sets XmNalignment (already done above). maybe some relict. does nothing if (_at->correct_for_at_center == 0)
        }

        AW_label_in_awar_list(this, button, buttonlabel);
    }

    short height = 0;
    short width  = 0;

    if (_at->to_position_exists) {
        // size has explicitly been specified in xfig -> calculate
        height = _at->to_position_y - _at->y_for_next_button;
        width  = _at->to_position_x - _at->x_for_next_button;
    }

    {
        Widget  toRecenter   = 0;
        int     recenterSize = 0;

        if (!height || !width) {
            // ask motif for real button size
            Widget ButOrHigh = _at->highlight ? fatherwidget : button;
            XtVaGetValues(ButOrHigh, XmNheight, &height, XmNwidth, &width, NULL);

            if (let_motif_choose_size) {
                if (_at->correct_for_at_center) {
                    toRecenter   = ButOrHigh;
                    recenterSize = width;
                }
                width = 0;          // ignore the used size (because it may use more than the window size)
            }
        }

        if (toRecenter) {
            int shiftback = 0;
            switch (_at->correct_for_at_center) {
                case 1: shiftback = recenterSize/2; break;
                case 2: shiftback = recenterSize; break;
            }
            if (shiftback) {
                XtVaSetValues(toRecenter, XmNx, x_button-shiftback, NULL);
            }
        }
    }

    _at->correct_for_at_center = org_correct_for_at_center; // restore original justification
    _at->y_for_next_button     = org_y_for_next_button;

    p_w->toggle_field = button;
    this->_set_activate_callback((void *)button);
    this->unset_at_commands();
    this->increment_at_commands(width+SPACE_BEHIND_BUTTON, height);
}

void AW_window::dump_at_position(const char *tmp_label) const {
    printf("%s at x = %i / y = %i\n", tmp_label, _at->x_for_next_button, _at->y_for_next_button);
}

void AW_window::update_label(Widget widget, const char *var_value) {
    if (get_root()->changer_of_variable != widget) {
        XtVaSetValues(widget, RES_CONVERT(XmNlabelString, var_value), NULL);
    }
    else {
        get_root()->changer_of_variable = 0;
    }
}

// ----------------------
//      on/off toggle

struct aw_toggle_data {
    bool  isTextToggle;
    char *bitmapOrText[2];
    int   buttonWidth; // wanted width in characters
};

void AW_window::update_toggle(Widget widget, const char *var, AW_CL cd_toggle_data) {
    aw_toggle_data *tdata = (aw_toggle_data*)cd_toggle_data;
    const char     *text  = tdata->bitmapOrText[(var[0] == '0' || var[0] == 'n') ? 0 : 1];

    if (tdata->isTextToggle) {
        XtVaSetValues(widget, RES_CONVERT(XmNlabelString, text), NULL);
    }
    else {
        char *path = pixmapPath(text+1);
        XtVaSetValues(widget, RES_CONVERT(XmNlabelPixmap, path), NULL);
        free(path);
    }
}

void AW_window::create_toggle(const char *var_name, aw_toggle_data *tdata) {
    AW_cb *cbs = _callback;
    _callback         = (AW_cb *)1;

    {
        int old_length_of_buttons = _at->length_of_buttons;

        if (tdata->buttonWidth == 0) {
            if (tdata->isTextToggle) {
                int l1 = strlen(tdata->bitmapOrText[0]);
                int l2 = strlen(tdata->bitmapOrText[1]);

                _at->length_of_buttons = l1>l2 ? l1 : l2; // use longer text for button size
            }
            else {
                _at->length_of_buttons = 0;
            }
        }
        else {
            _at->length_of_buttons = tdata->buttonWidth;
        }

        create_button(0, tdata->bitmapOrText[0], 0);

        _at->length_of_buttons = old_length_of_buttons;
    }

    AW_awar *vs = this->get_root()->awar(var_name);
    {
        char *var_value = vs->read_as_string();

        this->update_toggle(p_w->toggle_field, var_value, (AW_CL)tdata);
        free(var_value);
    }

    VarUpdateInfo *vui;
    vui = new VarUpdateInfo(this, p_w->toggle_field, AW_WIDGET_TOGGLE, vs, cbs);

    XtAddCallback(p_w->toggle_field, XmNactivateCallback,
                  (XtCallbackProc) AW_variable_update_callback,
                  (XtPointer) vui);

    vs->tie_widget((AW_CL)tdata, p_w->toggle_field, AW_WIDGET_TOGGLE, this);
}


void AW_window::create_toggle(const char *var_name, const char *no, const char *yes, int buttonWidth) {
    aw_toggle_data *tdata  = new aw_toggle_data;
    tdata->isTextToggle    = false;

    aw_assert(AW_IS_IMAGEREF(no));
    aw_assert(AW_IS_IMAGEREF(yes));

    tdata->bitmapOrText[0] = strdup(no);
    tdata->bitmapOrText[1] = strdup(yes);

    tdata->buttonWidth = buttonWidth;

    create_toggle(var_name, tdata);
}

void AW_window::create_text_toggle(const char *var_name, const char *noText, const char *yesText, int buttonWidth) {
    aw_toggle_data *tdata  = new aw_toggle_data;
    tdata->isTextToggle    = true;
    tdata->bitmapOrText[0] = strdup(noText);
    tdata->bitmapOrText[1] = strdup(yesText);
    tdata->buttonWidth     = buttonWidth;

    create_toggle(var_name, tdata);
}


void AW_window::create_toggle(const char *var_name) {
    create_toggle(var_name, "#no.xpm", "#yes.xpm");
}

void AW_window::create_inverse_toggle(const char *var_name) {
    // like create_toggle, but displays inverse value
    // (i.e. it's checked if value is zero, and unchecked otherwise)
    create_toggle(var_name, "#yes.xpm", "#no.xpm");
}

// ---------------------
//      input fields

void AW_window::create_input_field(const char *var_name,   int columns) {
    Widget         textField      = 0;
    Widget         tmp_label      = 0;
    AW_cb  *cbs;
    VarUpdateInfo *vui;
    char          *str;
    int            xoff_for_label = 0;

    if (!columns) columns = _at->length_of_buttons;

    AW_awar *vs = root->awar(var_name);
    str         = root->awar(var_name)->read_as_string();

    int width_of_input_label, height_of_input_label;
    calculate_label_size(&width_of_input_label, &height_of_input_label, true, 0);
    // @@@ FIXME: use height_of_input_label for propper Y-adjusting of label
    // width_of_input_label = this->calculate_string_width( calculate_label_length() );

    int width_of_input = this->calculate_string_width(columns+1) + 9;
    // calculate width for 1 additional character (input field is not completely used)
    // + 4 pixel for shadow + 4 unknown missing pixels + 1 add. pixel needed for visible text area

    Widget parentWidget = _at->attach_any ? INFO_FORM : INFO_WIDGET;

    if (_at->label_for_inputfield) {
        Label clientlabel(_at->label_for_inputfield, this);
        tmp_label = XtVaCreateManagedWidget("label",
                                            xmLabelWidgetClass,
                                            parentWidget,
                                            XmNwidth, (int)(width_of_input_label + 2),
                                            XmNhighlightThickness, 0,
                                            RES_LABEL_CONVERT(clientlabel),
                                            XmNrecomputeSize, false,
                                            XmNalignment, XmALIGNMENT_BEGINNING,
                                            XmNfontList, p_global->fontlist,
                                            (_at->attach_any) ? NULL : XmNx, (int)_at->x_for_next_button,
                                            XmNy, (int)(_at->y_for_next_button) + root->y_correction_for_input_labels -1,
                                            NULL);
        if (_at->attach_any) aw_attach_widget(tmp_label, _at);
        xoff_for_label = width_of_input_label + 10;
    }


    int width_of_last_widget = xoff_for_label + width_of_input + 2;

    if (_at->to_position_exists) {
        width_of_input = _at->to_position_x - _at->x_for_next_button - xoff_for_label + 2;
        width_of_last_widget = _at->to_position_x - _at->x_for_next_button;
    }

    {
        TuneBackground(parentWidget, TUNE_INPUT);
        textField = XtVaCreateManagedWidget("textField",
                                            xmTextFieldWidgetClass,
                                            parentWidget,
                                            XmNwidth, (int)width_of_input,
                                            XmNrows, 1,
                                            XmNvalue, str,
                                            XmNfontList, p_global->fontlist,
                                            XmNbackground, _at->background_color,
                                            (_at->attach_any) ? NULL : XmNx, (int)(_at->x_for_next_button + xoff_for_label),
                                            XmNy, (int)(_at->y_for_next_button + 5) - 8,
                                            NULL);
        if (_at->attach_any) {
            _at->x_for_next_button += xoff_for_label;
            aw_attach_widget(textField, _at);
            _at->x_for_next_button -= xoff_for_label;
        }
    }

    free(str);

    // user-own callback
    cbs = _callback;

    // callback for enter
    vui = new VarUpdateInfo(this, textField, AW_WIDGET_INPUT_FIELD, vs, cbs);

    XtAddCallback(textField, XmNactivateCallback,
                  (XtCallbackProc) AW_variable_update_callback,
                  (XtPointer) vui);
    if (_d_callback) {
        XtAddCallback(textField, XmNactivateCallback,
                      (XtCallbackProc) AW_server_callback,
                      (XtPointer) _d_callback);
        _d_callback->id = GBS_global_string_copy("INPUT:%s", var_name);
        get_root()->define_remote_command(_d_callback);
    }

    // callback for losing focus
    XtAddCallback(textField, XmNlosingFocusCallback,
                  (XtCallbackProc) AW_variable_update_callback,
                  (XtPointer) vui);
    // callback for value changed
    XtAddCallback(textField, XmNvalueChangedCallback,
                  (XtCallbackProc) AW_value_changed_callback,
                  (XtPointer) root);

    vs->tie_widget(0, textField, AW_WIDGET_INPUT_FIELD, this);
    root->make_sensitive(textField, _at->widget_mask);

    short height;
    XtVaGetValues(textField, XmNheight, &height, NULL);
    int height_of_last_widget = height;

    if (_at->correct_for_at_center == 1) {   // middle centered
        XtVaSetValues(textField, XmNx, ((int)(_at->x_for_next_button + xoff_for_label) - (int)(width_of_last_widget/2) + 1), NULL);
        if (tmp_label) {
            XtVaSetValues(tmp_label, XmNx, ((int)(_at->x_for_next_button) - (int)(width_of_last_widget/2) + 1), NULL);
        }
        width_of_last_widget = width_of_last_widget / 2;
    }
    if (_at->correct_for_at_center == 2) {   // right centered
        XtVaSetValues(textField, XmNx, (int)(_at->x_for_next_button + xoff_for_label - width_of_last_widget + 3), NULL);
        if (tmp_label) {
            XtVaSetValues(tmp_label, XmNx, (int)(_at->x_for_next_button - width_of_last_widget + 3), NULL);
        }
        width_of_last_widget = 0;
    }
    width_of_last_widget -= 2;

    this->unset_at_commands();
    this->increment_at_commands(width_of_last_widget, height_of_last_widget);

}

void AW_window::update_input_field(Widget widget, const char *var_value) {
    XtVaSetValues(widget, XmNvalue, var_value, NULL);
}

void AW_window::create_text_field(const char *var_name, int columns, int rows) {
    Widget         scrolledWindowText;
    Widget         scrolledText;
    Widget         tmp_label             = 0;
    AW_cb         *cbs;
    VarUpdateInfo *vui;
    char          *str                   = NULL;
    short          width_of_last_widget  = 0;
    short          height_of_last_widget = 0;
    int            width_of_text         = 0;
    int            height_of_text        = 0;
    int            xoff_for_label        = 0;

    AW_awar *vs = root->awar(var_name);
    str         = root->awar(var_name)->read_string();

    int width_of_text_label, height_of_text_label;
    calculate_label_size(&width_of_text_label, &height_of_text_label, true, 0);
    // @@@ FIXME: use height_of_text_label for propper Y-adjusting of label

    // width_of_text_label = this->calculate_string_width( calculate_label_length() );
    width_of_text = this->calculate_string_width(columns) + 18;
    height_of_text = this->calculate_string_height(rows, rows*4) + 9;


    if (_at->label_for_inputfield) {
        Label clientlabel(_at->label_for_inputfield, this);
        tmp_label = XtVaCreateManagedWidget("label",
                                            xmLabelWidgetClass,
                                            INFO_WIDGET,
                                            XmNx, (int)_at->x_for_next_button,
                                            XmNy, (int)(_at->y_for_next_button) + this->get_root()->y_correction_for_input_labels + 5 - 6,
                                            XmNwidth, (int)(width_of_text_label + 2),
                                            RES_LABEL_CONVERT(clientlabel),
                                            XmNrecomputeSize, false,
                                            XmNalignment, XmALIGNMENT_BEGINNING,
                                            XmNfontList, p_global->fontlist,
                                            NULL);

        xoff_for_label = width_of_text_label + 10;

    }

    {
        aw_xargs args(6);
        args.add(XmNscrollingPolicy,        XmAPPLICATION_DEFINED);
        args.add(XmNvisualPolicy,           XmVARIABLE);
        args.add(XmNscrollBarDisplayPolicy, XmSTATIC);
        args.add(XmNfontList,               (XtArgVal)p_global->fontlist);

        if (_at->to_position_exists) {
            scrolledWindowText = XtVaCreateManagedWidget("scrolledWindowList1", xmScrolledWindowWidgetClass, INFO_FORM, NULL);
            args.assign_to_widget(scrolledWindowText);

            aw_attach_widget(scrolledWindowText, _at);
            width_of_text = _at->to_position_x - _at->x_for_next_button - xoff_for_label - 18;
            if (_at->y_for_next_button < _at->to_position_y - 18) {
                height_of_text = _at->to_position_y - _at->y_for_next_button - 18;
            }
        }
        else {
            scrolledWindowText = XtVaCreateManagedWidget("scrolledWindowText", xmScrolledWindowWidgetClass, INFO_WIDGET, NULL);
            args.add(XmNx, 10);
            args.add(XmNy, _at->y_for_next_button);
            args.assign_to_widget(scrolledWindowText);
        }
    }

    TuneBackground(scrolledWindowText, TUNE_INPUT);
    scrolledText = XtVaCreateManagedWidget("scrolledText1",
                                           xmTextWidgetClass,
                                           scrolledWindowText,
                                           XmNeditMode, XmMULTI_LINE_EDIT,
                                           XmNvalue, str,
                                           XmNscrollLeftSide, false,
                                           XmNwidth, (int)width_of_text,
                                           XmNheight, (int)height_of_text,
                                           XmNfontList, p_global->fontlist,
                                           XmNbackground, _at->background_color,
                                           NULL);
    free(str);

    if (!_at->to_position_exists) {
        XtVaGetValues(scrolledWindowText,   XmNheight, &height_of_last_widget,
                      XmNwidth, &width_of_last_widget, NULL);

        width_of_last_widget += (short)xoff_for_label;

        switch (_at->correct_for_at_center) {
            case 0: // left centered
                XtVaSetValues(scrolledWindowText, XmNx, (int)(_at->x_for_next_button + xoff_for_label), NULL);
                break;

            case 1: // middle centered
                XtVaSetValues(scrolledWindowText, XmNx, (int)(_at->x_for_next_button + xoff_for_label - (width_of_last_widget/2)), NULL);
                if (_at->label_for_inputfield) {
                    XtVaSetValues(tmp_label, XmNx, (int)(_at->x_for_next_button - (width_of_last_widget/2)), NULL);
                }
                width_of_last_widget = width_of_last_widget / 2;
                break;

            case 2: // right centered
                XtVaSetValues(scrolledWindowText, XmNx, (int)(_at->x_for_next_button + xoff_for_label - width_of_last_widget),     NULL);
                if (_at->label_for_inputfield) {
                    XtVaSetValues(tmp_label, XmNx, (int)(_at->x_for_next_button - width_of_last_widget), NULL);
                }
                width_of_last_widget = 0;
                break;
        }
    }

    // user-own callback
    cbs = _callback;

    // callback for enter
    vui = new VarUpdateInfo(this, scrolledText, AW_WIDGET_TEXT_FIELD, vs, cbs);
    XtAddCallback(scrolledText, XmNactivateCallback, (XtCallbackProc) AW_variable_update_callback, (XtPointer) vui);
    // callback for losing focus
    XtAddCallback(scrolledText, XmNlosingFocusCallback, (XtCallbackProc) AW_variable_update_callback, (XtPointer) vui);
    // callback for value changed
    XtAddCallback(scrolledText, XmNvalueChangedCallback, (XtCallbackProc) AW_value_changed_callback, (XtPointer) root);

    vs->tie_widget(0, scrolledText, AW_WIDGET_TEXT_FIELD, this);
    root->make_sensitive(scrolledText, _at->widget_mask);

    this->unset_at_commands();
    this->increment_at_commands(width_of_last_widget, height_of_last_widget);
}


void AW_window::update_text_field(Widget widget, const char *var_value) {
    XtVaSetValues(widget, XmNvalue, var_value, NULL);
}

static void scalerChanged_cb(Widget scale, XtPointer variable_update_struct, XtPointer call_data) {
    XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct*)call_data;
    VarUpdateInfo         *vui = (VarUpdateInfo*)variable_update_struct;

    bool do_update = true;
    if (cbs->reason == XmCR_DRAG) { // still dragging?
        double mean_callback_time = vui->get_awar()->mean_callback_time();

        const double MAX_DRAGGED_CALLBACK_TIME = 1.0;
        if (mean_callback_time>MAX_DRAGGED_CALLBACK_TIME) {
            do_update = false; // do not update while dragging, if update callback needs more than 1 second
                               // Note that a longer update will happen once!
        }
    }

    if (do_update) {
        AW_root::SINGLETON->value_changed = true;
        AW_variable_update_callback(scale, variable_update_struct, call_data);
    }
}

void AW_window::create_input_field_with_scaler(const char *awar_name, int textcolumns, int scaler_length, AW_ScalerType scalerType) {
    create_input_field(awar_name, textcolumns);

    Widget parentWidget = _at->attach_any ? INFO_FORM : INFO_WIDGET;

    AW_awar *vs = root->awar(awar_name);
    // Note: scaler also "works" if no min/max is defined for awar, but scaling is a bit weird
    int scalerValue = calculate_scaler_value(vs, scalerType);

    Widget scale = XtVaCreateManagedWidget("scale",
                                           xmScaleWidgetClass,
                                           parentWidget,
                                           XmNx, _at->x_for_next_button,
                                           XmNy, _at->y_for_next_button + 4,
                                           XmNorientation, XmHORIZONTAL,
                                           XmNscaleWidth, scaler_length,
                                           XmNshowValue, False,
                                           XmNminimum, SCALER_MIN_VALUE,
                                           XmNmaximum, SCALER_MAX_VALUE,
                                           XmNvalue, scalerValue,
                                           NULL);

    short width_of_last_widget  = 0;
    short height_of_last_widget = 0;

    XtVaGetValues(scale,
                  XmNheight, &height_of_last_widget,
                  XmNwidth, &width_of_last_widget,
                  NULL);

    AW_cb         *cbs = _callback;
    VarUpdateInfo *vui = new VarUpdateInfo(this, scale, AW_WIDGET_SCALER, vs, cbs);
    vui->set_scalerType(scalerType);

    XtAddCallback(scale, XmNvalueChangedCallback, scalerChanged_cb, (XtPointer)vui);
    XtAddCallback(scale, XmNdragCallback,         scalerChanged_cb, (XtPointer)vui);

    vs->tie_widget((AW_CL)scalerType, scale, AW_WIDGET_SCALER, this);
    root->make_sensitive(scale, _at->widget_mask);

    this->unset_at_commands();
    this->increment_at_commands(width_of_last_widget, height_of_last_widget);
}

void AW_window::update_scaler(Widget widget, AW_awar *awar, AW_ScalerType scalerType) {
    int scalerVal = calculate_scaler_value(awar, scalerType);
    XtVaSetValues(widget, XmNvalue, scalerVal, NULL);
}

// -----------------------
//      selection list


static void scroll_sellist(Widget scrolledList, bool upwards) {
    int oldPos, visible, items;
    XtVaGetValues(scrolledList,
                  XmNtopItemPosition, &oldPos,
                  XmNvisibleItemCount, &visible,
                  XmNitemCount,  &items, 
                  NULL);

    int amount = visible/5;
    if (amount<1) amount = 1;
    if (upwards) amount = -amount;

    int newPos = force_in_range(1, oldPos + amount, items-visible+2);
    if (newPos != oldPos) XmListSetPos(scrolledList, newPos);
}

static void scroll_sellist_up(Widget scrolledList, XEvent*, String*, Cardinal*) { scroll_sellist(scrolledList, true); }
static void scroll_sellist_dn(Widget scrolledList, XEvent*, String*, Cardinal*) { scroll_sellist(scrolledList, false); }

AW_selection_list* AW_window::create_selection_list(const char *var_name, int columns, int rows, bool /*fallback2default*/) {
    // Note: fallback2default has no meaning in motif-version (always acts like 'false', i.e. never does fallback)
    // see also .@create_option_menu

    Widget         scrolledWindowList; // @@@ fix locals
    Widget         scrolledList;
    VarUpdateInfo *vui;
    AW_cb         *cbs;

    int width_of_list;
    int height_of_list;
    int width_of_last_widget  = 0;
    int height_of_last_widget = 0;

    aw_assert(!_at->label_for_inputfield); // labels have no effect for selection lists

    AW_awar *vs = 0;

    aw_assert(var_name); // @@@ case where var_name == NULL is relict from multi-selection-list (not used; removed)

    if (var_name) vs = root->awar(var_name);

    width_of_list  = this->calculate_string_width(columns) + 9;
    height_of_list = this->calculate_string_height(rows, 4*rows) + 9;

    {
        aw_xargs args(7);
        args.add(XmNvisualPolicy,           XmVARIABLE);
        args.add(XmNscrollBarDisplayPolicy, XmSTATIC);
        args.add(XmNshadowThickness,        0);
        args.add(XmNfontList,               (XtArgVal)p_global->fontlist);

        if (_at->to_position_exists) {
            width_of_list = _at->to_position_x - _at->x_for_next_button - 18;
            if (_at->y_for_next_button  < _at->to_position_y - 18) {
                height_of_list = _at->to_position_y - _at->y_for_next_button - 18;
            }
            scrolledWindowList = XtVaCreateManagedWidget("scrolledWindowList1", xmScrolledWindowWidgetClass, INFO_FORM, NULL);

            args.assign_to_widget(scrolledWindowList);
            aw_attach_widget(scrolledWindowList, _at);

            width_of_last_widget = _at->to_position_x - _at->x_for_next_button;
            height_of_last_widget = _at->to_position_y - _at->y_for_next_button;
        }
        else {
            scrolledWindowList = XtVaCreateManagedWidget("scrolledWindowList1", xmScrolledWindowWidgetClass, INFO_WIDGET, NULL);

            args.add(XmNscrollingPolicy, XmAPPLICATION_DEFINED);
            args.add(XmNx, 10);
            args.add(XmNy, _at->y_for_next_button);
            args.assign_to_widget(scrolledWindowList);
        }
    }

    {
        int select_type = XmMULTIPLE_SELECT;
        if (vs) select_type = XmBROWSE_SELECT;

        TuneBackground(scrolledWindowList, TUNE_INPUT);
        scrolledList = XtVaCreateManagedWidget("scrolledList1",
                                               xmListWidgetClass,
                                               scrolledWindowList,
                                               XmNwidth, (int)width_of_list,
                                               XmNheight, (int) height_of_list,
                                               XmNscrollBarDisplayPolicy, XmSTATIC,
                                               XmNselectionPolicy, select_type,
                                               XmNlistSizePolicy, XmCONSTANT,
                                               XmNfontList, p_global->fontlist,
                                               XmNbackground, _at->background_color,
                                               NULL);

        static bool actionsAdded = false;
        if (!actionsAdded) {
            struct _XtActionsRec actions[2] = {
                {(char*)"scroll_sellist_up", scroll_sellist_up},
                {(char*)"scroll_sellist_dn", scroll_sellist_dn}
            };

            XtAppAddActions(p_global->context, actions, 2);
        }

        XtTranslations translations = XtParseTranslationTable(
            "<Btn4Down>:scroll_sellist_up()\n"
            "<Btn5Down>:scroll_sellist_dn()\n"
            );
        XtAugmentTranslations(scrolledList, translations);
    }

    if (!_at->to_position_exists) {
        short height;
        XtVaGetValues(scrolledList, XmNheight, &height, NULL);
        height_of_last_widget = height + 20;
        width_of_last_widget  = width_of_list + 20;

        switch (_at->correct_for_at_center) {
            case 3: break;
            case 0: // left aligned
                XtVaSetValues(scrolledWindowList, XmNx, (int)(_at->x_for_next_button), NULL);
                break;

            case 1: // center aligned
                XtVaSetValues(scrolledWindowList, XmNx, (int)(_at->x_for_next_button - (width_of_last_widget/2)), NULL);
                width_of_last_widget = width_of_last_widget / 2;
                break;

            case 2: // right aligned
                XtVaSetValues(scrolledWindowList, XmNx, (int)(_at->x_for_next_button - width_of_list - 18), NULL);
                width_of_last_widget = 0;
                break;
        }

    }

    {
        int type = GB_STRING;
        if (vs)  type = vs->variable_type;

        if (p_global->selection_list) {
            p_global->last_selection_list->next = new AW_selection_list(var_name, type, scrolledList);
            p_global->last_selection_list = p_global->last_selection_list->next;
        }
        else {
            p_global->last_selection_list = p_global->selection_list = new AW_selection_list(var_name, type, scrolledList);
        }
    }


    // user-own callback
    cbs = _callback;

    // callback for enter
    if (vs) {
        vui = new VarUpdateInfo(this, scrolledList, AW_WIDGET_SELECTION_LIST, vs, cbs);
        vui->set_sellist(p_global->last_selection_list);

        XtAddCallback(scrolledList, XmNbrowseSelectionCallback,
                      (XtCallbackProc) AW_variable_update_callback,
                      (XtPointer) vui);

        if (_d_callback) {
            XtAddCallback(scrolledList, XmNdefaultActionCallback,
                          (XtCallbackProc) AW_server_callback,
                          (XtPointer) _d_callback);
        }
        vs->tie_widget((AW_CL)p_global->last_selection_list, scrolledList, AW_WIDGET_SELECTION_LIST, this);
        root->make_sensitive(scrolledList, _at->widget_mask);
    }

    this->unset_at_commands();
    this->increment_at_commands(width_of_last_widget, height_of_last_widget);
    return p_global->last_selection_list;
}

