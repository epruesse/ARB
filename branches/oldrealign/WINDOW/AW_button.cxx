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
#include "aw_Xm.hxx"
#include "aw_select.hxx"
#include "aw_nawar.hxx"
#include "aw_awar.hxx"
#include "aw_window_Xm.hxx"
#include "aw_msg.hxx"
#include "aw_root.hxx"
#include "aw_question.hxx"
#include "aw_xargs.hxx"

#include <arb_algo.h>
#include <ad_cb.h>

#include <Xm/Frame.h>
#include <Xm/MenuShell.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ScrolledW.h>

#include <iostream>

#if defined(DEBUG)
// #define DUMP_BUTTON_CREATION
#endif // DEBUG

class VarUpdateInfo : virtual Noncopyable { // used to refresh single items on change
    AW_window         *aw_parent;
    Widget             widget;
    AW_widget_type     widget_type;
    AW_awar           *awar;
    AW_scalar          value;
    AW_cb             *cbs;
    AW_selection_list *sellist;

public:
    VarUpdateInfo(AW_window *aw, Widget w, AW_widget_type wtype, AW_awar *a, AW_cb *cbs_)
        : aw_parent(aw), widget(w), widget_type(wtype), awar(a),
          value(a),
          cbs(cbs_), sellist(NULL)
    {
    }
    template<typename T>
    VarUpdateInfo(AW_window *aw, Widget w, AW_widget_type wtype, AW_awar *a, T t, AW_cb *cbs_)
        : aw_parent(aw), widget(w), widget_type(wtype), awar(a),
          value(t),
          cbs(cbs_), sellist(NULL)
    {
    }

    void change_from_widget(XtPointer call_data);

    void set_widget(Widget w) { widget = w; }
    void set_sellist(AW_selection_list *asl) { sellist = asl; }
};

static void AW_variable_update_callback(Widget /*wgt*/, XtPointer variable_update_struct, XtPointer call_data) {
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

            AW_selection_list_entry *entry = sellist->list_table;
            while (entry && strcmp(entry->get_displayed(), selected) != 0) {
                entry = entry->next;
            }

            if (!entry) {   
                entry = sellist->default_select; // use default selection
                if (!entry) GBK_terminate("no default specified for selection list"); // or die
            }
            entry->value.write_to(awar);
            XtFree(selected);
            break;
        }

        case AW_WIDGET_LABEL_FIELD:
            break;

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

static void aw_attach_widget(Widget w, AW_at *_at, int default_width = -1) {
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

static void calculate_label_size(AW_window *aww, int *width, int *height, bool in_pixel, const char *non_at_label) {
    // in_pixel == true -> calculate size in pixels
    // in_pixel == false -> calculate size in characters

    const char *label = non_at_label ? non_at_label : aww->_at->label_for_inputfield;
    if (label) {
        calculate_textsize(label, width, height);
        if (aww->_at->length_of_label_for_inputfield) {
            *width = aww->_at->length_of_label_for_inputfield;
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
    aw_assert(buttonlabel[0] != '#');    // use create_button for graphical buttons!
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
            _callback->id = GBS_global_string_copy("%s/%s", this->window_defaults_name, macro_name);
            get_root()->define_remote_command(_callback);
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

        tmp_label = XtVaCreateManagedWidget("label",
                                            xmLabelWidgetClass,
                                            parent_widget,
                                            XmNwidth, (int)(width_of_label + 2),
                                            RES_LABEL_CONVERT(_at->label_for_inputfield),
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

        if (_callback) {
            args.add(XmNshadowThickness, _at->shadow_thickness);
            args.add(XmNalignment,       XmALIGNMENT_CENTER);

            button = XtVaCreateManagedWidget("button", xmPushButtonWidgetClass, fatherwidget, RES_LABEL_CONVERT(buttonlabel), NULL);
        }
        else { // button w/o callback; (flat, not clickable)
            button = XtVaCreateManagedWidget("label", xmLabelWidgetClass, parent_widget, RES_LABEL_CONVERT(buttonlabel), NULL);
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

    aw_assert(no[0] == '#');
    aw_assert(yes[0] == '#');

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
    calculate_label_size(this, &width_of_input_label, &height_of_input_label, true, 0);
    // @@@ FIXME: use height_of_input_label for propper Y-adjusting of label
    // width_of_input_label = this->calculate_string_width( calculate_label_length() );

    int width_of_input = this->calculate_string_width(columns+1) + 9;
    // calculate width for 1 additional character (input field is not completely used)
    // + 4 pixel for shadow + 4 unknown missing pixels + 1 add. pixel needed for visible text area

    Widget parentWidget = _at->attach_any ? INFO_FORM : INFO_WIDGET;

    if (_at->label_for_inputfield) {
        tmp_label = XtVaCreateManagedWidget("label",
                                            xmLabelWidgetClass,
                                            parentWidget,
                                            XmNwidth, (int)(width_of_input_label + 2),
                                            XmNhighlightThickness, 0,
                                            RES_CONVERT(XmNlabelString, _at->label_for_inputfield),
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
    calculate_label_size(this, &width_of_text_label, &height_of_text_label, true, 0);
    // @@@ FIXME: use height_of_text_label for propper Y-adjusting of label

    // width_of_text_label = this->calculate_string_width( calculate_label_length() );
    width_of_text = this->calculate_string_width(columns) + 18;
    height_of_text = this->calculate_string_height(rows, rows*4) + 9;


    if (_at->label_for_inputfield) {
        tmp_label = XtVaCreateManagedWidget("label",
                                            xmLabelWidgetClass,
                                            INFO_WIDGET,
                                            XmNx, (int)_at->x_for_next_button,
                                            XmNy, (int)(_at->y_for_next_button) + this->get_root()->y_correction_for_input_labels + 5 - 6,
                                            XmNwidth, (int)(width_of_text_label + 2),
                                            RES_CONVERT(XmNlabelString, _at->label_for_inputfield),
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

__ATTR__NORETURN inline void option_type_mismatch(const char *triedType) { type_mismatch(triedType, "option-menu"); }
__ATTR__NORETURN inline void toggle_type_mismatch(const char *triedType) { type_mismatch(triedType, "toggle"); }

// ----------------------
//      Options-Menu

AW_option_menu_struct *AW_window::create_option_menu(const char *awar_name, bool /*fallback2default*/) {
    // Note: fallback2default has no meaning in motif-version (always acts like 'false', i.e. never does fallback)
    // see also .@create_selection_list

    Widget optionMenu_shell;
    Widget optionMenu;
    Widget optionMenu1;
    int    x_for_position_of_menu;

    const char *tmp_label = _at->label_for_inputfield;
    if (tmp_label && !tmp_label[0]) {
        aw_assert(0); // do not specify empty labels (causes misalignment)
        tmp_label = NULL;
    }

    _at->saved_x           = _at->x_for_next_button - (tmp_label ? 0 : 10);
    x_for_position_of_menu = 10;

    optionMenu_shell = XtVaCreatePopupShell ("optionMenu shell",
                                             xmMenuShellWidgetClass,
                                             INFO_WIDGET,
                                             XmNwidth, 1,
                                             XmNheight, 1,
                                             XmNallowShellResize, true,
                                             XmNoverrideRedirect, true,
                                             XmNfontList, p_global->fontlist,
                                             NULL);

    optionMenu = XtVaCreateWidget("optionMenu_p1",
                                  xmRowColumnWidgetClass,
                                  optionMenu_shell,
                                  XmNrowColumnType, XmMENU_PULLDOWN,
                                  XmNfontList, p_global->fontlist,
                                  NULL);
    {
        aw_xargs args(3);
        args.add(XmNfontList, (XtArgVal)p_global->fontlist);
        if (!_at->attach_x && !_at->attach_lx) args.add(XmNx, x_for_position_of_menu);
        if (!_at->attach_y && !_at->attach_ly) args.add(XmNy, _at->y_for_next_button-5);

        if (tmp_label) {
            int   width_help_label, height_help_label;
            calculate_label_size(this, &width_help_label, &height_help_label, false, tmp_label);
            // @@@ FIXME: use height_help_label for Y-alignment
#if defined(DUMP_BUTTON_CREATION)
            printf("width_help_label=%i label='%s'\n", width_help_label, tmp_label);
#endif // DUMP_BUTTON_CREATION

            {
                char *help_label = this->align_string(tmp_label, width_help_label);
                optionMenu1 = XtVaCreateManagedWidget("optionMenu1",
                                                      xmRowColumnWidgetClass,
                                                      (_at->attach_any) ? INFO_FORM : INFO_WIDGET,
                                                      XmNrowColumnType, XmMENU_OPTION,
                                                      XmNsubMenuId, optionMenu,
                                                      RES_CONVERT(XmNlabelString, help_label),
                                                      NULL);
                free(help_label);
            }
        }
        else {
            _at->x_for_next_button = _at->saved_x;

            optionMenu1 = XtVaCreateManagedWidget("optionMenu1",
                                                  xmRowColumnWidgetClass,
                                                  (_at->attach_any) ? INFO_FORM : INFO_WIDGET,
                                                  XmNrowColumnType, XmMENU_OPTION,
                                                  XmNsubMenuId, optionMenu,
                                                  NULL);
        }
        args.assign_to_widget(optionMenu1);
    }

#if 0
    // setting background color for radio button only does not work.
    // works only for label and button together, but that's not what we want.
    TuneBackground(optionMenu_shell, TUNE_BUTTON); // set background color for radio button
    XtVaSetValues(optionMenu1, // colorizes background and label
                  XmNbackground, _at->background_color,
                  NULL);
#endif

    get_root()->number_of_option_menus++;

    AW_awar *vs = root->awar(awar_name);
    {
        AW_option_menu_struct *next =
            new AW_option_menu_struct(get_root()->number_of_option_menus,
                                      awar_name,
                                      vs->variable_type,
                                      optionMenu1,
                                      optionMenu,
                                      _at->x_for_next_button - 7,
                                      _at->y_for_next_button,
                                      _at->correct_for_at_center);

        if (p_global->option_menu_list) {
            p_global->last_option_menu->next = next;
            p_global->last_option_menu = p_global->last_option_menu->next;
        }
        else {
            p_global->last_option_menu = p_global->option_menu_list = next;
        }
    }

    p_global->current_option_menu = p_global->last_option_menu;

    vs->tie_widget((AW_CL)p_global->current_option_menu, optionMenu, AW_WIDGET_CHOICE_MENU, this);
    root->make_sensitive(optionMenu1, _at->widget_mask);

    return p_global->current_option_menu;
}

static void remove_option_from_option_menu(AW_root *aw_root, AW_widget_value_pair *os) {
    aw_root->remove_button_from_sens_list(os->widget);
    XtDestroyWidget(os->widget);
}

void AW_window::clear_option_menu(AW_option_menu_struct *oms) {
    p_global->current_option_menu = oms; // define as current (for subsequent inserts)

    AW_widget_value_pair *next_os;
    for (AW_widget_value_pair *os = oms->first_choice; os; os = next_os) {
        next_os  = os->next;
        os->next = 0;
        remove_option_from_option_menu(root, os);
        delete os;
    }

    if (oms->default_choice) {
        remove_option_from_option_menu(root, oms->default_choice);
        oms->default_choice = 0;
    }

    oms->first_choice   = 0;
    oms->last_choice    = 0;
}

void *AW_window::_create_option_entry(AW_VARIABLE_TYPE IF_ASSERTION_USED(type), const char *name, const char */*mnemonic*/, const char *name_of_color) {
    Widget                 entry;
    AW_option_menu_struct *oms = p_global->current_option_menu;

    aw_assert(oms->variable_type == type); // adding wrong entry type

    TuneOrSetBackground(oms->menu_widget, name_of_color, TUNE_BUTTON); // set background color for radio button entries
    entry = XtVaCreateManagedWidget("optionMenu_entry",
                                    xmPushButtonWidgetClass,
                                    oms->menu_widget,
                                    RES_LABEL_CONVERT(((char *)name)),
                                    XmNfontList, p_global->fontlist,
                                    XmNbackground, _at->background_color,
                                    NULL);
    AW_label_in_awar_list(this, entry, name);
    return (void *)entry;
}

inline void option_menu_add_option(AW_option_menu_struct *oms, AW_widget_value_pair *os, bool default_option) {
    if (default_option) {
        oms->default_choice = os;
    }
    else {
        if (oms->first_choice) {
            oms->last_choice->next = os;
            oms->last_choice       = oms->last_choice->next;
        }
        else {
            oms->last_choice = oms->first_choice = os;
        }
    }
}

// for string :

void AW_window::insert_option_internal(AW_label option_name, const char *mnemonic, const char *var_value, const char *name_of_color, bool default_option) {
    AW_option_menu_struct *oms = p_global->current_option_menu;
    if (oms->variable_type != AW_STRING) {
        option_type_mismatch("string");
    }
    else {
        Widget        entry = (Widget)_create_option_entry(AW_STRING, option_name, mnemonic, name_of_color);
        AW_cb *cbs   = _callback; // user-own callback

        // callback for new choice
        XtAddCallback(entry, XmNactivateCallback,
                      (XtCallbackProc) AW_variable_update_callback,
                      (XtPointer) new VarUpdateInfo(this, NULL, AW_WIDGET_CHOICE_MENU, root->awar(oms->variable_name), var_value, cbs));

        option_menu_add_option(p_global->current_option_menu, new AW_widget_value_pair(var_value, entry), default_option);
        root->make_sensitive(entry, _at->widget_mask);
        this->unset_at_commands();
    }
}
void AW_window::insert_option_internal(AW_label option_name, const char *mnemonic, int var_value, const char *name_of_color, bool default_option) {
    AW_option_menu_struct *oms = p_global->current_option_menu;

    if (oms->variable_type != AW_INT) {
        option_type_mismatch("int");
    }
    else {
        Widget        entry = (Widget)_create_option_entry(AW_INT, option_name, mnemonic, name_of_color);
        AW_cb *cbs   = _callback; // user-own callback

        // callback for new choice
        XtAddCallback(entry, XmNactivateCallback,
                      (XtCallbackProc) AW_variable_update_callback,
                      (XtPointer) new VarUpdateInfo(this, NULL, AW_WIDGET_CHOICE_MENU, root->awar(oms->variable_name), var_value, cbs));

        option_menu_add_option(p_global->current_option_menu, new AW_widget_value_pair(var_value, entry), default_option);
        root->make_sensitive(entry, _at->widget_mask);
        this->unset_at_commands();
    }
}
void AW_window::insert_option_internal(AW_label option_name, const char *mnemonic, float var_value, const char *name_of_color, bool default_option) {
    AW_option_menu_struct *oms = p_global->current_option_menu;

    if (oms->variable_type != AW_FLOAT) {
        option_type_mismatch("float");
    }
    else {
        Widget        entry = (Widget)_create_option_entry(AW_FLOAT, option_name, mnemonic, name_of_color);
        AW_cb *cbs   = _callback; // user-own callback

        // callback for new choice
        XtAddCallback(entry, XmNactivateCallback,
                      (XtCallbackProc) AW_variable_update_callback,
                      (XtPointer) new VarUpdateInfo(this, NULL, AW_WIDGET_CHOICE_MENU, root->awar(oms->variable_name), var_value, cbs));

        option_menu_add_option(p_global->current_option_menu, new AW_widget_value_pair(var_value, entry), default_option);
        root->make_sensitive(entry, _at->widget_mask);
        this->unset_at_commands();
    }
}

void AW_window::insert_option        (AW_label on, const char *mn, const char *vv, const char *noc) { insert_option_internal(on, mn, vv, noc, false); }
void AW_window::insert_default_option(AW_label on, const char *mn, const char *vv, const char *noc) { insert_option_internal(on, mn, vv, noc, true); }
void AW_window::insert_option        (AW_label on, const char *mn, int vv,         const char *noc) { insert_option_internal(on, mn, vv, noc, false); }
void AW_window::insert_default_option(AW_label on, const char *mn, int vv,         const char *noc) { insert_option_internal(on, mn, vv, noc, true); }
void AW_window::insert_option        (AW_label on, const char *mn, float vv,       const char *noc) { insert_option_internal(on, mn, vv, noc, false); }
void AW_window::insert_default_option(AW_label on, const char *mn, float vv,       const char *noc) { insert_option_internal(on, mn, vv, noc, true); }
// (see insert_option_internal for longer parameter names)

void AW_window::update_option_menu() {
    AW_option_menu_struct *oms = p_global->current_option_menu;
    refresh_option_menu(oms);

    if (_at->attach_any) aw_attach_widget(oms->label_widget, _at);

    short width;
    short height;
    XtVaGetValues(oms->label_widget, XmNwidth, &width, XmNheight, &height, NULL);
    int   width_of_last_widget  = width;
    int   height_of_last_widget = height;

    if (!_at->to_position_exists) {
        if (oms->correct_for_at_center_intern == 0) {   // left aligned
            XtVaSetValues(oms->label_widget, XmNx, short(_at->saved_x), NULL);
        }
        if (oms->correct_for_at_center_intern == 1) {   // middle centered
            XtVaSetValues(oms->label_widget, XmNx, short(_at->saved_x - width/2), NULL);
            width_of_last_widget = width_of_last_widget / 2;
        }
        if (oms->correct_for_at_center_intern == 2) {   // right aligned
            XtVaSetValues(oms->label_widget, XmNx, short(_at->saved_x - width), NULL);
            width_of_last_widget = 0;
        }
    }
            
    width_of_last_widget += SPACE_BEHIND_BUTTON;

    this->unset_at_commands();
    this->increment_at_commands(width_of_last_widget, height_of_last_widget);
}

void AW_window::refresh_option_menu(AW_option_menu_struct *oms) {
    if (get_root()->changer_of_variable != oms->label_widget) {
        AW_widget_value_pair *active_choice = oms->first_choice;
        {
            AW_scalar global_var_value(root->awar(oms->variable_name));
            while (active_choice && global_var_value != active_choice->value) {
                active_choice = active_choice->next;
            }
        }

        if (!active_choice) active_choice = oms->default_choice;
        if (active_choice) XtVaSetValues(oms->label_widget, XmNmenuHistory, active_choice->widget, NULL);

    }
}

// -------------------------------------------------------
//      toggle field (actually this are radio buttons)

void AW_window::create_toggle_field(const char *var_name, AW_label labeli, const char */*mnemonic*/) {
    if (labeli) this->label(labeli);
    this->create_toggle_field(var_name);
}


void AW_window::create_toggle_field(const char *var_name, int orientation) {
    // orientation = 0 -> vertical else horizontal layout

    Widget label_for_toggle;
    Widget toggle_field;

    int xoff_for_label           = 0;
    int width_of_label           = 0;
    int x_for_position_of_option = 0;

    const char *tmp_label = "";

    if (_at->label_for_inputfield) {
        tmp_label = _at->label_for_inputfield;
    }

    if (_at->correct_for_at_center) {
        _at->saved_x = _at->x_for_next_button;
        x_for_position_of_option = 10;
    }
    else {
        x_for_position_of_option = _at->x_for_next_button;
    }


    if (tmp_label) {
        int height_of_label;
        calculate_label_size(this, &width_of_label, &height_of_label, true, tmp_label);
        // @@@ FIXME: use height_of_label for Y-alignment
        // width_of_label = this->calculate_string_width( this->calculate_label_length() );
        label_for_toggle = XtVaCreateManagedWidget("label",
                                                   xmLabelWidgetClass,
                                                   INFO_WIDGET,
                                                   XmNx, (int)_at->x_for_next_button,
                                                   XmNy, (int)(_at->y_for_next_button) + this->get_root()->y_correction_for_input_labels,
                                                   XmNwidth, (int)(width_of_label + 2),
                                                   RES_CONVERT(XmNlabelString, tmp_label),
                                                   XmNrecomputeSize, false,
                                                   XmNalignment, XmALIGNMENT_BEGINNING,
                                                   XmNfontList, p_global->fontlist,
                                                   NULL);

        _at->saved_xoff_for_label = xoff_for_label = width_of_label + 10;

        p_w->toggle_label = label_for_toggle;
    }
    else {
        p_w->toggle_label = NULL;
        _at->saved_xoff_for_label = 0;
    }

    {
        aw_xargs args(6);
        args.add(XmNx,              x_for_position_of_option + xoff_for_label);
        args.add(XmNy,              _at->y_for_next_button - 2);
        args.add(XmNradioBehavior,  True);
        args.add(XmNradioAlwaysOne, True);
        args.add(XmNfontList,       (XtArgVal)p_global->fontlist);
        args.add(XmNorientation,    orientation ? XmHORIZONTAL : XmVERTICAL);
        
        toggle_field = XtVaCreateManagedWidget("rowColumn for toggle field", xmRowColumnWidgetClass, (_at->attach_any) ? INFO_FORM : INFO_WIDGET, NULL);

        args.assign_to_widget(toggle_field);
    }
    if (_at->attach_any) {
        aw_attach_widget(toggle_field, _at, 300);
    }

    AW_awar *vs = root->awar(var_name);

    p_w->toggle_field = toggle_field;
    free((p_w->toggle_field_var_name));
    p_w->toggle_field_var_name = strdup(var_name);
    p_w->toggle_field_var_type = vs->variable_type;

    get_root()->number_of_toggle_fields++;

    if (p_global->toggle_field_list) {
        p_global->last_toggle_field->next = new AW_toggle_field_struct(get_root()->number_of_toggle_fields, var_name, vs->variable_type, toggle_field, _at->correct_for_at_center);
        p_global->last_toggle_field = p_global->last_toggle_field->next;
    }
    else {
        p_global->last_toggle_field = p_global->toggle_field_list = new AW_toggle_field_struct(get_root()->number_of_toggle_fields, var_name, vs->variable_type, toggle_field, _at->correct_for_at_center);
    }

    vs->tie_widget(get_root()->number_of_toggle_fields, toggle_field, AW_WIDGET_TOGGLE_FIELD, this);
    root->make_sensitive(toggle_field, _at->widget_mask);
}

static Widget _aw_create_toggle_entry(AW_window *aww, Widget toggle_field,
                                      const char *label, const char *mnemonic,
                                      VarUpdateInfo *awus,
                                      AW_widget_value_pair *toggle, bool default_toggle) {
    AW_root *root = aww->get_root();

    Widget          toggleButton;

    toggleButton = XtVaCreateManagedWidget("toggleButton",
                                           xmToggleButtonWidgetClass,
                                           toggle_field,
                                           RES_LABEL_CONVERT_AWW(label, aww),
                                           RES_CONVERT(XmNmnemonic, mnemonic),
                                           XmNindicatorSize, 16,
                                           XmNfontList, p_global->fontlist,

                                           NULL);
    toggle->widget = toggleButton;
    awus->set_widget(toggleButton);
    XtAddCallback(toggleButton, XmNvalueChangedCallback,
                  (XtCallbackProc) AW_variable_update_callback,
                  (XtPointer) awus);
    if (default_toggle) {
        delete p_global->last_toggle_field->default_toggle;
        p_global->last_toggle_field->default_toggle = toggle;
    }
    else {
        if (p_global->last_toggle_field->first_toggle) {
            p_global->last_toggle_field->last_toggle->next = toggle;
            p_global->last_toggle_field->last_toggle = toggle;
        }
        else {
            p_global->last_toggle_field->last_toggle = toggle;
            p_global->last_toggle_field->first_toggle = toggle;
        }
    }
    root->make_sensitive(toggleButton, aww->_at->widget_mask);

    aww->unset_at_commands();
    return  toggleButton;
}


void AW_window::insert_toggle_internal(AW_label toggle_label, const char *mnemonic, const char *var_value, bool default_toggle) {
    if (p_w->toggle_field_var_type != AW_STRING) {
        toggle_type_mismatch("string");
    }
    else {
        _aw_create_toggle_entry(this, p_w->toggle_field, toggle_label, mnemonic,
                                new VarUpdateInfo(this, NULL, AW_WIDGET_TOGGLE_FIELD, root->awar(p_w->toggle_field_var_name), var_value, _callback),
                                new AW_widget_value_pair(var_value, 0),
                                default_toggle);
    }
}
void AW_window::insert_toggle_internal(AW_label toggle_label, const char *mnemonic, int var_value, bool default_toggle) {
    if (p_w->toggle_field_var_type != AW_INT) {
        toggle_type_mismatch("int");
    }
    else {
        _aw_create_toggle_entry(this, p_w->toggle_field, toggle_label, mnemonic,
                                new VarUpdateInfo(this, NULL, AW_WIDGET_TOGGLE_FIELD, root->awar(p_w->toggle_field_var_name), var_value, _callback),
                                new AW_widget_value_pair(var_value, 0),
                                default_toggle);
    }
}
void AW_window::insert_toggle_internal(AW_label toggle_label, const char *mnemonic, float var_value, bool default_toggle) {
    if (p_w->toggle_field_var_type != AW_FLOAT) {
        toggle_type_mismatch("float");
    }
    else {
        _aw_create_toggle_entry(this, p_w->toggle_field, toggle_label, mnemonic,
                                new VarUpdateInfo(this, NULL, AW_WIDGET_TOGGLE_FIELD, root->awar(p_w->toggle_field_var_name), var_value, _callback),
                                new AW_widget_value_pair(var_value, 0),
                                default_toggle);
    }
}

void AW_window::insert_toggle        (AW_label toggle_label, const char *mnemonic, const char *var_value) { insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(AW_label toggle_label, const char *mnemonic, const char *var_value) { insert_toggle_internal(toggle_label, mnemonic, var_value, true); }
void AW_window::insert_toggle        (AW_label toggle_label, const char *mnemonic, int var_value)           { insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(AW_label toggle_label, const char *mnemonic, int var_value)           { insert_toggle_internal(toggle_label, mnemonic, var_value, true); }
void AW_window::insert_toggle        (AW_label toggle_label, const char *mnemonic, float var_value)         { insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(AW_label toggle_label, const char *mnemonic, float var_value)         { insert_toggle_internal(toggle_label, mnemonic, var_value, true); }

void AW_window::update_toggle_field() {
    this->refresh_toggle_field(get_root()->number_of_toggle_fields);
}


void AW_window::refresh_toggle_field(int toggle_field_number) {
#if defined(DEBUG)
    static int inside_here = 0;
    aw_assert(!inside_here);
    inside_here++;
#endif // DEBUG

    AW_toggle_field_struct *toggle_field_list = p_global->toggle_field_list;
    {
        while (toggle_field_list) {
            if (toggle_field_number == toggle_field_list->toggle_field_number) {
                break;
            }
            toggle_field_list = toggle_field_list->next;
        }
    }

    if (toggle_field_list) {
        AW_widget_value_pair *active_toggle = toggle_field_list->first_toggle;
        {
            AW_scalar global_value(root->awar(toggle_field_list->variable_name));
            while (active_toggle && active_toggle->value != global_value) {
                active_toggle = active_toggle->next;
            }
            if (!active_toggle) active_toggle = toggle_field_list->default_toggle;
        }

        // iterate over all toggles including default_toggle and set their state
        for (AW_widget_value_pair *toggle = toggle_field_list->first_toggle; toggle;) {
            XmToggleButtonSetState(toggle->widget, toggle == active_toggle, False);

            if (toggle->next)                                     toggle = toggle->next;
            else if (toggle != toggle_field_list->default_toggle) toggle = toggle_field_list->default_toggle;
            else                                                  toggle = 0;
        }

        // @@@ code below should go to update_toggle_field
        {
            short length;
            short height;
            XtVaGetValues(p_w->toggle_field, XmNwidth, &length, XmNheight, &height, NULL);
            length                += (short)_at->saved_xoff_for_label;

            int width_of_last_widget  = length;
            int height_of_last_widget = height;

            if (toggle_field_list->correct_for_at_center_intern) {
                if (toggle_field_list->correct_for_at_center_intern == 1) {   // middle centered
                    XtVaSetValues(p_w->toggle_field, XmNx, (short)((short)_at->saved_x - (short)(length/2) + (short)_at->saved_xoff_for_label), NULL);
                    if (p_w->toggle_label) {
                        XtVaSetValues(p_w->toggle_label, XmNx, (short)((short)_at->saved_x - (short)(length/2)), NULL);
                    }
                    width_of_last_widget = width_of_last_widget / 2;
                }
                if (toggle_field_list->correct_for_at_center_intern == 2) {   // right centered
                    XtVaSetValues(p_w->toggle_field, XmNx, (short)((short)_at->saved_x - length + (short)_at->saved_xoff_for_label), NULL);
                    if (p_w->toggle_label) {
                        XtVaSetValues(p_w->toggle_label, XmNx, (short)((short)_at->saved_x - length),    NULL);
                    }
                    width_of_last_widget = 0;
                }
            }

            this->unset_at_commands();
            this->increment_at_commands(width_of_last_widget, height_of_last_widget);
        }
    }
    else {
        GBK_terminatef("update_toggle_field: toggle field %i does not exist", toggle_field_number);
    }

#if defined(DEBUG)
    inside_here--;
#endif // DEBUG
}
