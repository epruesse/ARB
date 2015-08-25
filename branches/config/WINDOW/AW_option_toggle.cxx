// ============================================================== //
//                                                                //
//   File      : AW_option_toggle.cxx                             //
//   Purpose   : option-menu- and toggle-code                     //
//                                                                //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include "aw_at.hxx"
#include "aw_window.hxx"
#include "aw_awar.hxx"
#include "aw_window_Xm.hxx"
#include "aw_root.hxx"
#include "aw_xargs.hxx"
#include "aw_varupdate.hxx"

#include <Xm/MenuShell.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>

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
            calculate_label_size(&width_help_label, &height_help_label, false, tmp_label);
            // @@@ FIXME: use height_help_label for Y-alignment
#if defined(DUMP_BUTTON_CREATION)
            printf("width_help_label=%i label='%s'\n", width_help_label, tmp_label);
#endif // DUMP_BUTTON_CREATION

            {
                aw_assert(!AW_IS_IMAGEREF(tmp_label)); // using images as labels for option menus will work in gtk (wont fix in motif)

                char *help_label = this->align_string(tmp_label, width_help_label);
                optionMenu1      = XtVaCreateManagedWidget("optionMenu1",
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
                                    RES_CONVERT(XmNlabelString, name), // force text (as gtk version does)
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

// ----------------------------------------------------------------------
// force-diff-sync 917381378912 (remove after merging back to trunk)
// ----------------------------------------------------------------------

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

// ----------------------------------------------------------------------
// force-diff-sync 917234982173 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::insert_option        (const char *on, const char *mn, const char *vv, const char *noc) { insert_option_internal(on, mn, vv, noc, false); }
void AW_window::insert_default_option(const char *on, const char *mn, const char *vv, const char *noc) { insert_option_internal(on, mn, vv, noc, true); }
void AW_window::insert_option        (const char *on, const char *mn, int vv,         const char *noc) { insert_option_internal(on, mn, vv, noc, false); }
void AW_window::insert_default_option(const char *on, const char *mn, int vv,         const char *noc) { insert_option_internal(on, mn, vv, noc, true); }
void AW_window::insert_option        (const char *on, const char *mn, float vv,       const char *noc) { insert_option_internal(on, mn, vv, noc, false); }
void AW_window::insert_default_option(const char *on, const char *mn, float vv,       const char *noc) { insert_option_internal(on, mn, vv, noc, true); }
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
    /*!
     * Begins a radio button group with a label
     */
    if (labeli) {
        this->label(labeli);
    }
    create_toggle_field(var_name);
}

void AW_window::create_toggle_field(const char *var_name, int orientation /*= 0*/) {
    /*!
     * Begins a radio button group
     * @param var_name    name of awar
     * @param orientation 0 -> vertical, != 0 horizontal layout
     */

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
        calculate_label_size(&width_of_label, &height_of_label, true, tmp_label);
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
                                      const char *labeltext, const char *mnemonic,
                                      VarUpdateInfo *awus,
                                      AW_widget_value_pair *toggle, bool default_toggle) {
    AW_root *root = aww->get_root();

    Label label(labeltext, aww);
    Widget toggleButton = XtVaCreateManagedWidget("toggleButton",
                                                  xmToggleButtonWidgetClass,
                                                  toggle_field,
                                                  RES_LABEL_CONVERT(label),
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
    root->make_sensitive(toggleButton, aww->get_at().widget_mask);

    aww->unset_at_commands();
    return  toggleButton;
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// force-diff-sync 189273814273 (remove after merging back to trunk)
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------


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


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// force-diff-sync 381892714273 (remove after merging back to trunk)
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------


void AW_window::insert_toggle        (const char *toggle_label, const char *mnemonic, const char *var_value) { insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(const char *toggle_label, const char *mnemonic, const char *var_value) { insert_toggle_internal(toggle_label, mnemonic, var_value, true); }
void AW_window::insert_toggle        (const char *toggle_label, const char *mnemonic, int var_value)         { insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(const char *toggle_label, const char *mnemonic, int var_value)         { insert_toggle_internal(toggle_label, mnemonic, var_value, true); }
void AW_window::insert_toggle        (const char *toggle_label, const char *mnemonic, float var_value)       { insert_toggle_internal(toggle_label, mnemonic, var_value, false); }
void AW_window::insert_default_toggle(const char *toggle_label, const char *mnemonic, float var_value)       { insert_toggle_internal(toggle_label, mnemonic, var_value, true); }

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
