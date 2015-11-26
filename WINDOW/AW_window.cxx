// =============================================================== //
//                                                                 //
//   File      : AW_window.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_at.hxx"
#include "aw_nawar.hxx"
#include "aw_xfig.hxx"
#include "aw_Xm.hxx"
#include "aw_window.hxx"
#include "aw_window_Xm.hxx"
#include "aw_xkey.hxx"
#include "aw_select.hxx"
#include "aw_awar.hxx"
#include "aw_msg.hxx"
#include "aw_root.hxx"
#include "aw_xargs.hxx"
#include "aw_device_click.hxx"

#include <arbdbt.h>
#include <arb_file.h>
#include <arb_str.h>
#include <arb_strarray.h>

#include <X11/Shell.h>
#include <Xm/AtomMgr.h>
#include <Xm/Frame.h>
#include <Xm/PushB.h>
#include <Xm/Protocols.h>
#include <Xm/RowColumn.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/MainW.h>
#include <Xm/CascadeB.h>
#include <Xm/MenuShell.h>
#include <Xm/ScrollBar.h>
#include <Xm/MwmUtil.h>

#include <cctype>
#include "aw_question.hxx"
#include <map>

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
void AW_window::destroyCreateWindowCallback(CreateWindowCallback *windowMaker) {
    delete windowMaker;
}


void AW_window::label(const char *_label) {
    freedup(_at->label_for_inputfield, _label);
}

// ----------------------------------------------------------------------
// force-diff-sync 1927391236 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::get_window_size(int &width, int &height){
    unsigned short hoffset = 0;
    if (p_w->menu_bar[0]) XtVaGetValues(p_w->menu_bar[0], XmNheight, &hoffset, NULL);
    width = _at->max_x_size;
    height = hoffset + _at->max_y_size;
}

void AW_window::help_text(const char *id) {
    //! set help text for next created button
    delete _at->helptext_for_next_button; // @@@ delete/free mismatch
    _at->helptext_for_next_button   = strdup(id);
}

void AW_window::highlight() {
    //! make next created button default button
    _at->highlight = true;
}

void AW_window::sens_mask(AW_active mask) {
    //! Set up sensitivity mask for next widget (action)
    aw_assert(legal_mask(mask));
    _at->widget_mask = mask;
}

void AW_window::callback(const WindowCallback& wcb) {
    /*!
     * Register callback for the next action implicitly created
     * when making a widget.
     */
    _callback = new AW_cb(this, wcb);
}

void AW_window::d_callback(const WindowCallback& wcb) {
    /*!
     * Register double click callback for the next action implicitly created
     * when making a widget.
     */
    _d_callback = new AW_cb(this, wcb);
}

// -------------------------------------------------------------
//      code used by scalers (common between motif and gtk)

static float apply_ScalerType(float val, AW_ScalerType scalerType, bool inverse) {
    float res = 0.0;

    aw_assert(val>=0.0 && val<=1.0);

    switch (scalerType) {
        case AW_SCALER_LINEAR:
            res = val;
            break;

        case AW_SCALER_EXP_LOWER:
            if (inverse) res = pow(val, 1/3.0);
            else         res = pow(val, 3.0);
            break;

        case AW_SCALER_EXP_UPPER:
            res = 1.0-apply_ScalerType(1.0-val, AW_SCALER_EXP_LOWER, inverse);
            break;

        case AW_SCALER_EXP_BORDER:
            inverse = !inverse;
            // fall-through
        case AW_SCALER_EXP_CENTER: {
            AW_ScalerType subType = inverse ? AW_SCALER_EXP_UPPER : AW_SCALER_EXP_LOWER;
            if      (val>0.5) res = (1+apply_ScalerType(2*val-1, subType, false))/2;
            else if (val<0.5) res = (1-apply_ScalerType(1-2*val, subType, false))/2;
            else              res = val;
            break;
        }
    }

    aw_assert(res>=0.0 && res<=1.0);

    return res;
}

float AW_ScalerTransformer::scaler2awar(float scaler, AW_awar *awar) {
    float amin        = awar->get_min();
    float amax        = awar->get_max();
    float modScaleRel = apply_ScalerType(scaler, type, false);
    float aval        = amin + modScaleRel*(amax-amin);

    return aval;
}

float AW_ScalerTransformer::awar2scaler(AW_awar *awar) {
    float amin = awar->get_min();
    float amax = awar->get_max();

    float awarRel = 0.0;
    switch (awar->variable_type) {
        case AW_FLOAT:
            awarRel = (awar->read_float()-amin) / (amax-amin);
            break;

        case AW_INT:
            awarRel = (awar->read_int()-amin) / (amax-amin);
            break;

        default:
            GBK_terminatef("Unsupported awar type %i in calculate_scaler_value", int(awar->variable_type));
            break;
    }

    aw_assert(awarRel>=0.0 && awarRel<=1.0);

    return apply_ScalerType(awarRel, type, true);
}


// ----------------------------------------------------------------------
// force-diff-sync 7284637824 (remove after merging back to trunk)
// ----------------------------------------------------------------------

static unsigned timed_window_title_cb(AW_root*, char *title, AW_window *aw) {
    aw->number_of_timed_title_changes--;
    if (!aw->number_of_timed_title_changes) {
        aw->set_window_title_intern(title);
    }

    delete title;
    return 0; // do not recall
}

void AW_window::message(char *title, int ms) {
    char *old_title = NULL;

    number_of_timed_title_changes++;

    old_title = strdup(window_name);

    XtVaSetValues(p_w->shell, XmNtitle, title, NULL);

    get_root()->add_timed_callback(ms, makeTimedCallback(timed_window_title_cb, old_title, this));
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

        if (AW_IS_IMAGEREF(topic_name)) {
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
        if (!AW_IS_IMAGEREF(topic_name)) {
            fputs("Warning: mnemonic is missing", stderr);
            print_at_location(stderr, topic_name);
            requestPossibilities(topic_name);
        }
    }
}

static void open_test_duplicate_mnemonics(const char *sub_menu_name, const char *mnemonic) {
    aw_assert(current_mscope);
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

void AW_window::create_menu(AW_label name, const char *mnemonic, AW_active mask) {
    aw_assert(legal_mask(mask));
    p_w->menu_deep = 0;

#ifdef CHECK_DUPLICATED_MNEMONICS
    init_duplicate_mnemonic(window_name);
#endif

#if defined(DUMP_MENU_LIST)
    dumpCloseAllSubMenus();
#endif // DUMP_MENU_LIST
    insert_sub_menu(name, mnemonic, mask);
}

void AW_window::close_sub_menu() {
    /**
     * Closes the currently open sub menu.
     * If no menu is open this method will crash.
     */
#ifdef CHECK_DUPLICATED_MNEMONICS
    close_test_duplicate_mnemonics();
#endif
#if defined(DUMP_MENU_LIST)
    dumpCloseSubMenu();
#endif // DUMP_MENU_LIST
    if (p_w->menu_deep>0)
        p_w->menu_deep--;
}

void AW_window::all_menus_created() const { // this is called by AW_window::show() (i.e. after all menus have been created)
#if defined(DEBUG)
    if (p_w->menu_deep>0) { // window had menu
        aw_assert(p_w->menu_deep == 1);
#ifdef CHECK_DUPLICATED_MNEMONICS
        exit_duplicate_mnemonic(); // cleanup mnemonic check
#endif
    }
#endif // DEBUG
}

// ----------------------------------------------------------------------
// force-diff-sync 2939128467234 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::draw_line(int x1, int y1, int x2, int y2, int width, bool resize) {
    aw_assert(xfig_data); // forgot to call load_xfig ?

    xfig_data->add_line(x1, y1, x2, y2, width);

    _at->max_x_size = std::max(_at->max_x_size, xfig_data->maxx - xfig_data->minx);
    _at->max_y_size = std::max(_at->max_y_size, xfig_data->maxy - xfig_data->miny);

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
#if defined(ARB_MOTIF)
        set_window_size(WIDER_THAN_SCREEN, HIGHER_THAN_SCREEN);
#endif
    }
}

AW_device_click *AW_window::get_click_device(AW_area area, int mousex, int mousey, int max_distance) {
    AW_area_management *aram         = MAP_ARAM(area);
    AW_device_click    *click_device = NULL;

    if (aram) {
        click_device = aram->get_click_device();
        click_device->init_click(AW::Position(mousex, mousey), max_distance, AW_ALL_DEVICES);
    }
    return click_device;
}

AW_device *AW_window::get_device(AW_area area) { // @@@ rename to get_screen_device
    AW_area_management *aram = MAP_ARAM(area);
    arb_assert(aram);
    return aram->get_screen_device();
}

void AW_window::get_event(AW_event *eventi) const {
    *eventi = event;
}

AW_device_print *AW_window::get_print_device(AW_area area) {
    AW_area_management *aram = MAP_ARAM(area);
    return aram ? aram->get_print_device() : NULL;
}

AW_device_size *AW_window::get_size_device(AW_area area) {
    AW_area_management *aram = MAP_ARAM(area);
    if (!aram) return NULL;

    AW_device_size *size_device = aram->get_size_device();
    if (size_device) {
        size_device->restart_tracking();
        size_device->reset(); // @@@ hm
    }
    return size_device;
}


void AW_window::insert_help_topic(const char *labeli, 
                                  const char *mnemonic, const char *helpText,
                                  AW_active mask, const WindowCallback& cb) {
    aw_assert(legal_mask(mask));
    Widget button;

#ifdef CHECK_DUPLICATED_MNEMONICS
    if (!current_mscope) init_duplicate_mnemonic(window_name);
    MnemonicScope *tmp = current_mscope;
    current_mscope     = help_mscope;
    test_duplicate_mnemonics(labeli, mnemonic);
    current_mscope     = tmp;
#endif

    // insert one help-sub-menu-entry
    button = XtVaCreateManagedWidget("", xmPushButtonWidgetClass,
                                     p_w->help_pull_down,
                                     RES_CONVERT(XmNlabelString, labeli),
                                     RES_CONVERT(XmNmnemonic, mnemonic), NULL);
    XtAddCallback(button, XmNactivateCallback,
                  (XtCallbackProc) AW_server_callback,
                  (XtPointer) new AW_cb(this, cb, helpText));

    root->make_sensitive(button, mask);
}

void AW_window::insert_menu_topic(const char *topic_id, const char *labeltext,
                                  const char *mnemonic, const char *helpText,
                                  AW_active mask, const WindowCallback& wcb) {
    aw_assert(legal_mask(mask));
    Widget button;

    TuneBackground(p_w->menu_bar[p_w->menu_deep], TUNE_MENUTOPIC); // set background color for normal menu topics

#if defined(DUMP_MENU_LIST)
    dumpMenuEntry(name);
#endif // DUMP_MENU_LIST

#ifdef CHECK_DUPLICATED_MNEMONICS
    test_duplicate_mnemonics(labeltext, mnemonic);
#endif

    Label topiclabel(labeltext, this);
    if (mnemonic && *mnemonic && strchr(labeltext, mnemonic[0])) {
        // create one sub-menu-point
        button = XtVaCreateManagedWidget("", xmPushButtonWidgetClass,
                                         p_w->menu_bar[p_w->menu_deep],
                                         RES_LABEL_CONVERT(topiclabel),
                                         RES_CONVERT(XmNmnemonic, mnemonic),
                                         XmNbackground, _at->background_color, NULL);
    }
    else {
        button = XtVaCreateManagedWidget("",
                                         xmPushButtonWidgetClass,
                                         p_w->menu_bar[p_w->menu_deep],
                                         RES_LABEL_CONVERT(topiclabel),
                                         XmNbackground, _at->background_color,
                                         NULL);
    }

    AW_label_in_awar_list(this, button, labeltext);
    AW_cb *cbs = new AW_cb(this, wcb, helpText);
    XtAddCallback(button, XmNactivateCallback,
                  (XtCallbackProc) AW_server_callback,
                  (XtPointer) cbs);

#if defined(DEVEL_RALF) // wanted version
    aw_assert(topic_id);
#else // !defined(DEVEL_RALF)
    if (!topic_id) topic_id = labeltext;
#endif
    cbs->id = strdup(topic_id);
    root->define_remote_command(cbs);
    root->make_sensitive(button, mask);
}

// ----------------------------------------------------------------------
// force-diff-sync 824638723647 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::insert_sub_menu(const char *labeli, const char *mnemonic, AW_active mask){
    aw_assert(legal_mask(mask));
    Widget shell, Label;

    TuneBackground(p_w->menu_bar[p_w->menu_deep], TUNE_SUBMENU); // set background color for submenus
    // (Note: This must even be called if TUNE_SUBMENU is 0!
    //        Otherwise several submenus get the TUNE_MENUTOPIC color)

#if defined(DUMP_MENU_LIST)
    dumpOpenSubMenu(name);
#endif // DUMP_MENU_LIST

#ifdef CHECK_DUPLICATED_MNEMONICS
    open_test_duplicate_mnemonics(labeli, mnemonic);
#endif

    // create shell for Pull-Down
    shell = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
                                 p_w->menu_bar[p_w->menu_deep],
                                 XmNwidth, 1,
                                 XmNheight, 1,
                                 XmNallowShellResize, true,
                                 XmNoverrideRedirect, true,
                                 NULL);

    // create row column in Pull-Down shell

    p_w->menu_bar[p_w->menu_deep+1] = XtVaCreateWidget("menu_row_column",
                                                       xmRowColumnWidgetClass, shell,
                                                       XmNrowColumnType, XmMENU_PULLDOWN,
                                                       XmNtearOffModel, XmTEAR_OFF_ENABLED,
                                                       NULL);

    // create label in menu bar
    if (mnemonic && *mnemonic && strchr(labeli, mnemonic[0])) {
        // if mnemonic is "" -> Cannot convert string "" to type KeySym
        Label = XtVaCreateManagedWidget("menu1_top_b1",
                                        xmCascadeButtonWidgetClass, p_w->menu_bar[p_w->menu_deep],
                                        RES_CONVERT(XmNlabelString, labeli),
                                        RES_CONVERT(XmNmnemonic, mnemonic),
                                        XmNsubMenuId, p_w->menu_bar[p_w->menu_deep+1],
                                        XmNbackground, _at->background_color, NULL);
    }
    else {
        Label = XtVaCreateManagedWidget("menu1_top_b1",
                                        xmCascadeButtonWidgetClass,
                                        p_w->menu_bar[p_w->menu_deep],
                                        RES_CONVERT(XmNlabelString, labeli),
                                        XmNsubMenuId, p_w->menu_bar[p_w->menu_deep+1],
                                        XmNbackground, _at->background_color,
                                        NULL);
    }

    if (p_w->menu_deep < AW_MAX_MENU_DEEP-1) p_w->menu_deep++;

    root->make_sensitive(Label, mask);
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

void AW_window::get_font_size(int& width, int& height) {
    width  = get_root()->font_width;
    height = get_root()->font_height;
}

// ----------------------------------------------------------------------
// force-diff-sync 264782364273 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::load_xfig(const char *file, bool resize /*= true*/) {
    int width, height;
    get_font_size(width, height);

    if (file)   xfig_data = new AW_xfig(file, width, height);
    else        xfig_data = new AW_xfig(width, height); // create an empty xfig

    xfig_data->create_gcs(get_device(AW_INFO_AREA), get_root()->color_mode ? 8 : 1);

    // @@@ move into AW_at::set_xfig
    int xsize = xfig_data->maxx - xfig_data->minx;
    int ysize = xfig_data->maxy - xfig_data->miny;
    if (xsize>_at->max_x_size) _at->max_x_size = xsize;
    if (ysize>_at->max_y_size) _at->max_y_size = ysize;

    if (resize) {
        recalc_size_atShow(AW_RESIZE_ANY);
#if defined(ARB_MOTIF)
        set_window_size(WIDER_THAN_SCREEN, HIGHER_THAN_SCREEN);
#else
        // @@@ should not be needed in gtk, as soon as recalc_size_atShow has proper effect (see #377)
        // @@@ This is the last remaining use in gtk! Remove if removed here.
        set_window_size(_at->max_x_size, _at->max_y_size);
#endif
    }

    set_expose_callback(AW_INFO_AREA, makeWindowCallback(AW_xfigCB_info_area, xfig_data));
}

/**
 * Construct window-local id.
 * Prefixes @param id with AW_window::window_defaults_name + "/"
 */
const char *AW_window::local_id(const char *id) const {
    static char *last_local_id = 0;
    freeset(last_local_id, GBS_global_string_copy("%s/%s", get_window_id(), id));
    return last_local_id;
}

void AW_window::sep______________() {
    //! insert a separator into the currently open menu
    XtVaCreateManagedWidget("", xmSeparatorWidgetClass,
                            p_w->menu_bar[p_w->menu_deep],
                            NULL);
}

/**
 * Registers @param wcb as expose callback.
 * Called whenever the @param area receives an expose event.
 * This is where any drawing should be handled.
 */
void AW_window::set_expose_callback(AW_area area, const WindowCallback& wcb) {
    AW_area_management *aram = MAP_ARAM(area);
    if (aram) aram->set_expose_callback(this, wcb);
}

static void AW_focusCB(Widget /*wgt*/, XtPointer cl_aww, XEvent*, Boolean*) {
    AW_window *aww = (AW_window*)cl_aww;
    aww->run_focus_callback();
}

/**
 * Registers a focus callback.
 */
void AW_window::set_focus_callback(const WindowCallback& wcb) {
    if (!focus_cb) {
        XtAddEventHandler(MIDDLE_WIDGET, EnterWindowMask, FALSE, AW_focusCB, (XtPointer) this);
    }
    if (!focus_cb || !focus_cb->contains((AnyWinCB)wcb.callee())) {
        focus_cb = new AW_cb(this, wcb, 0, focus_cb);
    }
}

/**
 * Registers callback to be executed after the window is shown.
 * Called multiple times if a show() follows a hide().
 */
void AW_window::set_popup_callback(const WindowCallback& wcb) {
    p_w->popup_cb = new AW_cb(this, wcb, 0, p_w->popup_cb);
}

void AW_window::set_info_area_height(int h) {
    XtVaSetValues(INFO_WIDGET, XmNheight, h, NULL);
    XtVaSetValues(p_w->frame, XmNtopOffset, h, NULL);
}

void AW_window::set_bottom_area_height(int h) {
    XtVaSetValues(BOTTOM_WIDGET, XmNheight, h, NULL);
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNbottomOffset, (int)h, NULL);
}

void AW_window::set_input_callback(AW_area area, const WindowCallback& wcb) {
    AW_area_management *aram = MAP_ARAM(area);
    if (aram) aram->set_input_callback(this, wcb);
}

void AW_window::set_motion_callback(AW_area area, const WindowCallback& wcb) {
    AW_area_management *aram = MAP_ARAM(area);
    if (aram) aram->set_motion_callback(this, wcb);
}

void AW_window::set_resize_callback(AW_area area, const WindowCallback& wcb) {
    AW_area_management *aram = MAP_ARAM(area);
    if (aram) aram->set_resize_callback(this, wcb);
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

void AW_window::set_vertical_scrollbar_top_indent(int indent) {
    XtVaSetValues(p_w->scroll_bar_vertical, XmNtopOffset, (int)indent, NULL);
    top_indent_of_vertical_scrollbar = indent;
}

void AW_window::set_horizontal_scrollbar_left_indent(int indent) {
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNleftOffset, (int)indent, NULL);
    left_indent_of_horizontal_scrollbar = indent;
}

void AW_window::_get_area_size(AW_area area, AW_screen_area *square) {
    AW_area_management *aram = MAP_ARAM(area);
    *square = aram->get_common()->get_screen();
}

void AW_window::get_scrollarea_size(AW_screen_area *square) {
    _get_area_size(AW_MIDDLE_AREA, square);
    square->r -= left_indent_of_horizontal_scrollbar;
    square->b -= top_indent_of_vertical_scrollbar;
}

void AW_window::calculate_scrollbars() {
    AW_screen_area scrollArea;
    get_scrollarea_size(&scrollArea);

    // HORIZONTAL
    {
        int slider_max = (int)get_scrolled_picture_width();
        if (slider_max <1) {
            slider_max = 1;
            XtVaSetValues(p_w->scroll_bar_horizontal, XmNsliderSize, 1, NULL);
        }

        bool use_horizontal_bar     = true;
        int  slider_size_horizontal = scrollArea.r;

        if (slider_size_horizontal < 1) slider_size_horizontal = 1; // ist der slider zu klein (<1) ?
        if (slider_size_horizontal > slider_max) { // Schirm groesser als Bild
            slider_size_horizontal = slider_max; // slider nimmt ganze laenge ein
            XtVaSetValues(p_w->scroll_bar_horizontal, XmNvalue, 0, NULL); // slider ganz links setzen
            use_horizontal_bar = false; // kein horizontaler slider mehr
        }

        // check wether XmNValue is to big
        int position_of_slider;
        XtVaGetValues(p_w->scroll_bar_horizontal, XmNvalue, &position_of_slider, NULL);
        if (position_of_slider > (slider_max-slider_size_horizontal)) { // steht der slider fuer slidergroesse zu rechts ?
            position_of_slider = slider_max-slider_size_horizontal; // -1 ? vielleicht !
            if (position_of_slider < 0) position_of_slider = 0;
            XtVaSetValues(p_w->scroll_bar_horizontal, XmNvalue, position_of_slider, NULL);
        }
        // Anpassung fuer resize, wenn unbeschriebener Bereich vergroessert wird
        int max_slider_pos = (int)(get_scrolled_picture_width() - scrollArea.r);
        if (slider_pos_horizontal>max_slider_pos) {
            slider_pos_horizontal = use_horizontal_bar ? max_slider_pos : 0;
        }

        XtVaSetValues(p_w->scroll_bar_horizontal, XmNsliderSize, 1, NULL);
        XtVaSetValues(p_w->scroll_bar_horizontal, XmNmaximum, slider_max, NULL);
        XtVaSetValues(p_w->scroll_bar_horizontal, XmNsliderSize, slider_size_horizontal, NULL);

        update_scrollbar_settings_from_awars(AW_HORIZONTAL);
    }

    // VERTICAL
    {
        int slider_max = (int)get_scrolled_picture_height();
        if (slider_max <1) {
            slider_max = 1;
            XtVaSetValues(p_w->scroll_bar_vertical, XmNsliderSize, 1, NULL);
        }

        bool use_vertical_bar     = true;
        int  slider_size_vertical = scrollArea.b;

        if (slider_size_vertical < 1) slider_size_vertical = 1;
        if (slider_size_vertical > slider_max) {
            slider_size_vertical = slider_max;
            XtVaSetValues(p_w->scroll_bar_vertical, XmNvalue, 0, NULL);
            use_vertical_bar = false;
        }

        // check wether XmNValue is to big
        int position_of_slider;
        XtVaGetValues(p_w->scroll_bar_vertical, XmNvalue, &position_of_slider, NULL);
        if (position_of_slider > (slider_max-slider_size_vertical)) {
            position_of_slider = slider_max-slider_size_vertical; // -1 ? vielleicht !
            if (position_of_slider < 0) position_of_slider = 0;
            XtVaSetValues(p_w->scroll_bar_vertical, XmNvalue, position_of_slider, NULL);
        }
        // Anpassung fuer resize, wenn unbeschriebener Bereich vergroessert wird
        int max_slider_pos = (int)(get_scrolled_picture_height() - scrollArea.b);
        if (slider_pos_vertical>max_slider_pos) {
            slider_pos_vertical = use_vertical_bar ? max_slider_pos : 0;
        }

        XtVaSetValues(p_w->scroll_bar_vertical, XmNsliderSize, 1, NULL);
        XtVaSetValues(p_w->scroll_bar_vertical, XmNmaximum, slider_max, NULL);
        XtVaSetValues(p_w->scroll_bar_vertical, XmNsliderSize, slider_size_vertical, NULL);

        update_scrollbar_settings_from_awars(AW_VERTICAL);
    }
}

// ----------------------------------------------------------------------
// force-diff-sync 82346723846 (remove after merging back to trunk)
// ----------------------------------------------------------------------

static void horizontal_scrollbar_redefinition_cb(AW_root*, AW_window *aw) {
    aw->update_scrollbar_settings_from_awars(AW_HORIZONTAL);
}

static void vertical_scrollbar_redefinition_cb(AW_root*, AW_window *aw) {
    aw->update_scrollbar_settings_from_awars(AW_VERTICAL);
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
    RootCallback hor_src = makeRootCallback(horizontal_scrollbar_redefinition_cb, this);
    RootCallback ver_src = makeRootCallback(vertical_scrollbar_redefinition_cb, this);

    get_root()->awar_int(window_local_awarname("horizontal_page_increment"), 50)->add_callback(hor_src);
    get_root()->awar_int(window_local_awarname("vertical_page_increment"),   50)->add_callback(ver_src);
    get_root()->awar_int(window_local_awarname("scroll_width_horizontal"),    9)->add_callback(hor_src);
    get_root()->awar_int(window_local_awarname("scroll_width_vertical"),     20)->add_callback(ver_src);
    get_root()->awar_int(window_local_awarname("scroll_delay_horizontal"),   20)->add_callback(hor_src);
    get_root()->awar_int(window_local_awarname("scroll_delay_vertical"),     20)->add_callback(ver_src);
}

void AW_window::set_vertical_scrollbar_position(int position){
#if defined(DEBUG) && 0
    fprintf(stderr, "set_vertical_scrollbar_position to %i\n", position);
#endif
    // @@@ test and constrain against limits
    slider_pos_vertical = position;
    XtVaSetValues(p_w->scroll_bar_vertical, XmNvalue, position, NULL);
}

void AW_window::set_horizontal_scrollbar_position(int position) {
#if defined(DEBUG) && 0
    fprintf(stderr, "set_horizontal_scrollbar_position to %i\n", position);
#endif
    // @@@ test and constrain against limits
    slider_pos_horizontal = position;
    XtVaSetValues(p_w->scroll_bar_horizontal, XmNvalue, position, NULL);
}

static void value_changed_scroll_bar_vertical(Widget /*wgt*/, XtPointer aw_cb_struct, XtPointer call_data) {
    XmScrollBarCallbackStruct *sbcbs = (XmScrollBarCallbackStruct *)call_data;
    AW_cb *cbs = (AW_cb *) aw_cb_struct;
    cbs->aw->slider_pos_vertical = sbcbs->value; // setzt Scrollwerte im AW_window
    cbs->run_callbacks();
}
static void value_changed_scroll_bar_horizontal(Widget /*wgt*/, XtPointer aw_cb_struct, XtPointer call_data) {
    XmScrollBarCallbackStruct *sbcbs = (XmScrollBarCallbackStruct *)call_data;
    AW_cb *cbs = (AW_cb *) aw_cb_struct;
    (cbs->aw)->slider_pos_horizontal = sbcbs->value; // setzt Scrollwerte im AW_window
    cbs->run_callbacks();
}

static void drag_scroll_bar_vertical(Widget /*wgt*/, XtPointer aw_cb_struct, XtPointer call_data) {
    XmScrollBarCallbackStruct *sbcbs = (XmScrollBarCallbackStruct *)call_data;
    AW_cb *cbs = (AW_cb *) aw_cb_struct;
    cbs->aw->slider_pos_vertical = sbcbs->value; // setzt Scrollwerte im AW_window
    cbs->run_callbacks();
}
static void drag_scroll_bar_horizontal(Widget /*wgt*/, XtPointer aw_cb_struct, XtPointer call_data) {
    XmScrollBarCallbackStruct *sbcbs = (XmScrollBarCallbackStruct *)call_data;
    AW_cb *cbs = (AW_cb *) aw_cb_struct;
    (cbs->aw)->slider_pos_horizontal = sbcbs->value; // setzt Scrollwerte im AW_window
    cbs->run_callbacks();
}

void AW_window::set_vertical_change_callback(const WindowCallback& wcb) {
    XtAddCallback(p_w->scroll_bar_vertical, XmNvalueChangedCallback,
            (XtCallbackProc) value_changed_scroll_bar_vertical,
            (XtPointer) new AW_cb(this, wcb, ""));
    XtAddCallback(p_w->scroll_bar_vertical, XmNdragCallback,
            (XtCallbackProc) drag_scroll_bar_vertical,
            (XtPointer) new AW_cb(this, wcb, ""));

    XtAddCallback(p_w->scroll_bar_vertical, XmNpageIncrementCallback,
            (XtCallbackProc) drag_scroll_bar_vertical,
            (XtPointer) new AW_cb(this, wcb, ""));
    XtAddCallback(p_w->scroll_bar_vertical, XmNpageDecrementCallback,
            (XtCallbackProc) drag_scroll_bar_vertical,
            (XtPointer) new AW_cb(this, wcb, ""));
}

void AW_window::set_horizontal_change_callback(const WindowCallback& wcb) {
    XtAddCallback(p_w->scroll_bar_horizontal, XmNvalueChangedCallback,
                  (XtCallbackProc) value_changed_scroll_bar_horizontal,
                  (XtPointer) new AW_cb(this, wcb, ""));
    XtAddCallback(p_w->scroll_bar_horizontal, XmNdragCallback,
            (XtCallbackProc) drag_scroll_bar_horizontal,
            (XtPointer) new AW_cb(this, wcb, ""));
}

// END SCROLLBAR STUFF


void AW_window::set_window_size(int width, int height) {
    XtVaSetValues(p_w->shell, XmNwidth, (int)width, XmNheight, (int)height, NULL);
}

void AW_window::set_window_title_intern(char *title) {
    XtVaSetValues(p_w->shell, XmNtitle, title, NULL);
}

void AW_window::set_window_title(const char *title){
    XtVaSetValues(p_w->shell, XmNtitle, title, NULL);
    freedup(window_name, title);
}

const char* AW_window::get_window_title() const {
    return window_name;
}

void AW_window::shadow_width(int shadow_thickness) {
    _at->shadow_thickness = shadow_thickness;
}

#if defined(ARB_MOTIF)
void AW_window::button_height(int height) {
    _at->height_of_buttons = height>1 ? height : 0;
}
#endif

// ----------------------------------------------------------------------
// force-diff-sync 874687234237 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::window_fit() {
    int width, height;
    get_window_size(width, height);
    set_window_size(width, height);
}

AW_window::AW_window()
    : recalc_size_at_show(AW_KEEP_SIZE),
      recalc_pos_at_show(AW_KEEP_POS),
      hide_cb(NULL),
      expose_callback_added(false),
      focus_cb(NULL),
      xfig_data(NULL),
      _at(new AW_at),
      left_indent_of_horizontal_scrollbar(0),
      top_indent_of_vertical_scrollbar(0),
      root(NULL),
      click_time(0),
      color_table_size(0),
      color_table(NULL),
      number_of_timed_title_changes(0),
      p_w(new AW_window_Motif),
      _callback(NULL),
      _d_callback(NULL),
      window_name(NULL),
      window_defaults_name(NULL),
      window_is_shown(false),
      slider_pos_vertical(0),
      slider_pos_horizontal(0),
      main_drag_gc(0),
      picture(new AW_screen_area)
{
    reset_scrolled_picture_size();
}

AW_window::~AW_window() {
    delete picture;
    delete p_w;
}

#if defined(DEBUG)
// #define DUMP_MENU_LIST          // this should NOT be defined normally (if defined, every window writes all menu-entries to stdout)
#endif // DEBUG
#if defined(DUMP_MENU_LIST)

static char *window_name = 0;
static char *sub_menu = 0;

static void initMenuListing(const char *win_name) {
    aw_assert(win_name);

    freedup(window_name, win_name);
    freenull(sub_menu);

    printf("---------------------------------------- list of menus for '%s'\n", window_name);
}

static void dumpMenuEntry(const char *entry) {
    aw_assert(window_name);
    if (sub_menu) {
        printf("'%s/%s/%s'\n", window_name, sub_menu, entry);
    }
    else {
        printf("'%s/%s'\n", window_name, entry);
    }
}

static void dumpOpenSubMenu(const char *sub_name) {
    aw_assert(sub_name);

    dumpMenuEntry(sub_name); // dump the menu itself

    if (sub_menu) freeset(sub_menu, GBS_global_string_copy("%s/%s", sub_menu, sub_name));
    else sub_menu = strdup(sub_name);
}

static void dumpCloseSubMenu() {
    aw_assert(sub_menu);
    char *lslash = strrchr(sub_menu, '/');
    if (lslash) {
        lslash[0] = 0;
    }
    else freenull(sub_menu);
}

static void dumpCloseAllSubMenus() {
    freenull(sub_menu);
}

#endif // DUMP_MENU_LIST

AW_window_menu_modes::AW_window_menu_modes() : AW_window_menu_modes_private(NULL) {}
AW_window_menu_modes::~AW_window_menu_modes() {}

AW_window_menu::AW_window_menu() {}
AW_window_menu::~AW_window_menu() {}

AW_window_simple::AW_window_simple() {}
AW_window_simple::~AW_window_simple() {}

AW_window_simple_menu::AW_window_simple_menu() {}
AW_window_simple_menu::~AW_window_simple_menu() {}

AW_window_message::AW_window_message() {}
AW_window_message::~AW_window_message() {}

#define LAYOUT_AWAR_ROOT "window/windows"
#define BUFSIZE 256
static char aw_size_awar_name_buffer[BUFSIZE];
static const char *aw_size_awar_name(AW_window *aww, const char *sub_entry) {
#if defined(DEBUG)
    int size =
#endif // DEBUG
            sprintf(aw_size_awar_name_buffer, LAYOUT_AWAR_ROOT "/%s/%s",
                    aww->window_defaults_name, sub_entry);
#if defined(DEBUG)
    aw_assert(size < BUFSIZE);
#endif // DEBUG
    return aw_size_awar_name_buffer;
    // (see also AW_window::window_local_awarname)
}
#undef BUFSIZE

#define aw_awar_name_posx(aww)   aw_size_awar_name((aww), "posx")
#define aw_awar_name_posy(aww)   aw_size_awar_name((aww), "posy")
#define aw_awar_name_width(aww)  aw_size_awar_name((aww), "width")
#define aw_awar_name_height(aww) aw_size_awar_name((aww), "height")

void AW_window::create_user_geometry_awars(int posx, int posy, int width, int height) {
    get_root()->awar_int(aw_awar_name_posx(this), posx);
    get_root()->awar_int(aw_awar_name_posy(this), posy);
    get_root()->awar_int(aw_awar_name_width(this), width);
    get_root()->awar_int(aw_awar_name_height(this), height);
}


void AW_window::store_size_in_awars(int width, int height) {
    get_root()->awar(aw_awar_name_width(this))->write_int(width);
    get_root()->awar(aw_awar_name_height(this))->write_int(height);
}

void AW_window::get_size_from_awars(int& width, int& height) {
    width  = get_root()->awar(aw_awar_name_width(this))->read_int();
    height = get_root()->awar(aw_awar_name_height(this))->read_int();
}

void AW_window::store_pos_in_awars(int posx, int posy) {
    get_root()->awar(aw_awar_name_posx(this))->write_int(posx);
    get_root()->awar(aw_awar_name_posy(this))->write_int(posy);
}

void AW_window::get_pos_from_awars(int& posx, int& posy) {
    posx = get_root()->awar(aw_awar_name_posx(this))->read_int();
    posy = get_root()->awar(aw_awar_name_posy(this))->read_int();
}

void AW_window::reset_geometry_awars() {
    AW_root *awr = get_root();

    ASSERT_NO_ERROR(awr->awar(aw_awar_name_posx(this))  ->reset_to_default());
    ASSERT_NO_ERROR(awr->awar(aw_awar_name_posy(this))  ->reset_to_default());
    ASSERT_NO_ERROR(awr->awar(aw_awar_name_width(this)) ->reset_to_default());
    ASSERT_NO_ERROR(awr->awar(aw_awar_name_height(this))->reset_to_default());

    recalc_pos_atShow(AW_REPOS_TO_MOUSE_ONCE);
    recalc_size_atShow(AW_RESIZE_ANY);
}

#undef aw_awar_name_posx
#undef aw_awar_name_posy
#undef aw_awar_name_width
#undef aw_awar_name_height

static void aw_onExpose_calc_WM_offsets(AW_window *aww);
static unsigned aw_calc_WM_offsets_delayed(AW_root *, AW_window*aww) { aw_onExpose_calc_WM_offsets(aww); return 0; }

static void aw_onExpose_calc_WM_offsets(AW_window *aww) {
    AW_window_Motif *motif = p_aww(aww);

    int posx,  posy;  aww->get_window_content_pos(posx, posy);

    bool knows_window_position = posx != 0 || posy != 0;

    if (!knows_window_position) { // oops - motif has no idea where the window has been placed
        // assume positions stored in awars are correct and use them.
        // This works around problems with unclickable GUI-elements when running on 'Unity'

        int oposx, oposy; aww->get_pos_from_awars(oposx, oposy);
        aww->set_window_frame_pos(oposx, oposy);

        if (!motif->knows_WM_offset()) {
            aww->get_root()->add_timed_callback(100, makeTimedCallback(aw_calc_WM_offsets_delayed, aww));
        }
    }
    else if (!motif->knows_WM_offset()) {
        int oposx, oposy; aww->get_pos_from_awars(oposx, oposy);

        // calculate offset
        int left = posx-oposx;
        int top  = posy-oposy;

        if (top == 0 || left == 0)  { // invalid frame size
            if (motif->WM_left_offset == 0) {
                motif->WM_left_offset = 1; // hack: unused yet, use as flag to avoid endless repeats
#if defined(DEBUG)
                fprintf(stderr, "aw_onExpose_calc_WM_offsets: failed to detect framesize (shift window 1 pixel and retry)\n");
#endif
                oposx = oposx>10 ? oposx-1 : oposx+1;
                oposy = oposy>10 ? oposy-1 : oposy+1;

                aww->set_window_frame_pos(oposx, oposy);
                aww->store_pos_in_awars(oposx, oposy);

                aww->get_root()->add_timed_callback(500, makeTimedCallback(aw_calc_WM_offsets_delayed, aww));
                return;
            }
            else {
                // use maximum seen frame size
                top  = AW_window_Motif::WM_max_top_offset;
                left = AW_window_Motif::WM_max_left_offset;

                if (top == 0 || left == 0) { // still invalid -> retry later
                    aww->get_root()->add_timed_callback(10000, makeTimedCallback(aw_calc_WM_offsets_delayed, aww));
                    return;
                }
            }
        }
        motif->WM_top_offset  = top;
        motif->WM_left_offset = left;

        // remember maximum seen offsets
        AW_window_Motif::WM_max_top_offset  = std::max(AW_window_Motif::WM_max_top_offset,  motif->WM_top_offset);
        AW_window_Motif::WM_max_left_offset = std::max(AW_window_Motif::WM_max_left_offset, motif->WM_left_offset);

#if defined(DEBUG)
            fprintf(stderr, "WM_top_offset=%i WM_left_offset=%i (pos from awar: %i/%i, content-pos: %i/%i)\n",
                    motif->WM_top_offset, motif->WM_left_offset,
                    oposx, oposy, posx, posy);
#endif
    }
}

AW_root_Motif::AW_root_Motif()
    : last_widget(0),
      display(NULL),
      context(0),
      toplevel_widget(0),
      main_widget(0),
      main_aww(NULL),
      message_shell(0),
      foreground(0),
      background(0),
      fontlist(0),
      option_menu_list(NULL),
      last_option_menu(NULL),
      current_option_menu(NULL),
      toggle_field_list(NULL),
      last_toggle_field(NULL),
      selection_list(NULL),
      last_selection_list(NULL),
      screen_depth(0),
      color_table(NULL),
      colormap(0),
      help_active(0),
      clock_cursor(0),
      question_cursor(0),
      old_cursor_display(NULL),
      old_cursor_window(0),
      no_exit(false),
      action_hash(NULL)
{}

AW_root_Motif::~AW_root_Motif() {
    GBS_free_hash(action_hash);
    XmFontListFree(fontlist);
    free(color_table);
}

void AW_root_Motif::set_cursor(Display *d, Window w, Cursor c) {
    XSetWindowAttributes attrs;
    old_cursor_display = d;
    old_cursor_window = w;

    if (c)
        attrs.cursor = c;
    else
        attrs.cursor = None;

    if (d && w) {
        XChangeWindowAttributes(d, w, CWCursor, &attrs);
    }
    XChangeWindowAttributes(XtDisplay(main_widget), XtWindow(main_widget),
            CWCursor, &attrs);
    XFlush(XtDisplay(main_widget));
}

void AW_root_Motif::normal_cursor() {
    set_cursor(old_cursor_display, old_cursor_window, 0);
}

void AW_server_callback(Widget /*wgt*/, XtPointer aw_cb_struct, XtPointer /*call_data*/) {
    AW_cb *cbs = (AW_cb *) aw_cb_struct;

    AW_root *root = cbs->aw->get_root();
    if (p_global->help_active) {
        p_global->help_active = 0;
        p_global->normal_cursor();

        if (cbs->help_text && ((GBS_string_matches(cbs->help_text, "*.ps", GB_IGNORE_CASE)) ||
                               (GBS_string_matches(cbs->help_text, "*.hlp", GB_IGNORE_CASE)) ||
                               (GBS_string_matches(cbs->help_text, "*.help", GB_IGNORE_CASE))))
        {
            AW_help_popup(cbs->aw, cbs->help_text);
        }
        else {
            aw_message("Sorry no help available");
        }
        return;
    }

    if (root->is_tracking()) root->track_action(cbs->id);

    p_global->set_cursor(XtDisplay(p_global->toplevel_widget),
                         XtWindow(p_aww(cbs->aw)->shell),
                         p_global->clock_cursor);
    cbs->run_callbacks();

    XEvent event; // destroy all old events !!!
    while (XCheckMaskEvent(XtDisplay(p_global->toplevel_widget),
                           ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
                           KeyPressMask|KeyReleaseMask|PointerMotionMask, &event)) {
    }

    if (p_global->help_active) {
        p_global->set_cursor(XtDisplay(p_global->toplevel_widget),
                             XtWindow(p_aww(cbs->aw)->shell),
                             p_global->question_cursor);
    }
    else {
        p_global->set_cursor(XtDisplay(p_global->toplevel_widget),
                             XtWindow(p_aww(cbs->aw)->shell),
                             0);
    }
}

// ----------------------------------------------------------------------
// force-diff-sync 2964732647236 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::recalc_pos_atShow(AW_PosRecalc pr) {
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



// --------------
//      focus


void AW_window::run_focus_callback() {
    if (focus_cb) focus_cb->run_callbacks();
}

bool AW_window::is_focus_callback(AnyWinCB f) {
    return focus_cb && focus_cb->contains(f);
}

// ---------------
//      expose

inline void AW_area_management::run_expose_callback() {
    if (expose_cb) expose_cb->run_callbacks();
}

static void AW_exposeCB(Widget /*wgt*/, XtPointer aw_cb_struct, XmDrawingAreaCallbackStruct *call_data) {
    XEvent *ev = call_data->event;
    AW_area_management *aram = (AW_area_management *) aw_cb_struct;
    if (ev->xexpose.count == 0) { // last expose cb
        aram->run_expose_callback();
    }
}

void AW_area_management::set_expose_callback(AW_window *aww, const WindowCallback& cb) {
    // insert expose callback for draw_area
    if (!expose_cb) {
        XtAddCallback(area, XmNexposeCallback, (XtCallbackProc) AW_exposeCB,
                (XtPointer) this);
    }
    expose_cb = new AW_cb(aww, cb, 0, expose_cb);
}

bool AW_area_management::is_expose_callback(AnyWinCB f) {
    return expose_cb && expose_cb->contains(f);
}

bool AW_window::is_expose_callback(AW_area area, AnyWinCB f) {
    AW_area_management *aram = MAP_ARAM(area);
    return aram && aram->is_expose_callback(f);
}

void AW_window::force_expose() {
    XmDrawingAreaCallbackStruct da_struct;

    da_struct.reason = XmCR_EXPOSE;
    da_struct.event = (XEvent *) NULL;
    da_struct.window = XtWindow(p_w->shell);

    XtCallCallbacks(p_w->shell, XmNexposeCallback, (XtPointer) &da_struct);
}

// ---------------
//      resize


bool AW_area_management::is_resize_callback(AnyWinCB f) {
    return resize_cb && resize_cb->contains(f);
}

bool AW_window::is_resize_callback(AW_area area, AnyWinCB f) {
    AW_area_management *aram = MAP_ARAM(area);
    return aram && aram->is_resize_callback(f);
}

void AW_window::set_window_frame_pos(int x, int y) {
    // this will set the position of the frame around the client-area (see also WM_top_offset ..)
    XtVaSetValues(p_w->shell, XmNx, (int)x, XmNy, (int)y, NULL);
}
void AW_window::get_window_content_pos(int& xpos, int& ypos) {
    // this will report the position of the client-area (see also WM_top_offset ..)
    unsigned short x, y;
    XtVaGetValues(p_w->shell, XmNx, &x, XmNy, &y, NULL);
    xpos = x;
    ypos = y;
}

void AW_window::get_screen_size(int &width, int &height) {
    Screen *screen = XtScreen(p_w->shell);

    width  = WidthOfScreen(screen);
    height = HeightOfScreen(screen);
}

bool AW_window::get_mouse_pos(int& x, int& y) {
    Display      *d  = XtDisplay(p_w->shell);
    Window        w1 = XtWindow(p_w->shell);
    Window        w2;
    Window        w3;
    int           rx, ry;
    int           wx, wy;
    unsigned int  mask;

    Bool ok = XQueryPointer(d, w1, &w2, &w3, &rx, &ry, &wx, &wy, &mask);

    if (ok) {
#if defined(DEBUG) && 0
        printf("get_mouse_pos: rx/ry=%i/%i wx/wy=%i/%i\n", rx, ry, wx, wy);
#endif // DEBUG
        x = rx;
        y = ry;
    }
    return ok;
}

static int is_resize_event(Display *display, XEvent *event, XPointer) {
    // Predicate function: checks, if the given event is a ResizeEvent
    if (event && (event->type == ResizeRequest || event->type
            == ConfigureNotify) && event->xany.display == display) {
        return 1;
    }
    return 0;
}

static void cleanupResizeEvents(Display *display) {
    // Removes redundant resize events from the x-event queue
    if (display) {
        XLockDisplay(display);
        XEvent event;
        if (XCheckIfEvent(display, &event, is_resize_event, 0)) {
            // Some magic happens here... ;-) (removing redundant events from queue)
            while (XCheckIfEvent(display, &event, is_resize_event, 0))
                ;
            // Keep last Event in queue
            XPutBackEvent(display, &event);
        }
        XUnlockDisplay(display);
    }
}

static void aw_window_avoid_destroy_cb(Widget, AW_window *, XmAnyCallbackStruct *) {
    aw_message("If YOU do not know what to answer, how should ARB know?\nPlease think again and answer the prompt!");
}
static void aw_window_noexit_destroy_cb(Widget,  AW_window *aww, XmAnyCallbackStruct *) {
    aww->hide();
    // don't exit, when using destroy callback
}
static void aw_window_destroy_cb(Widget,  AW_window *aww, XmAnyCallbackStruct *) {
    AW_root *root = aww->get_root();
    if ((p_global->main_aww == aww) || !p_global->main_aww->is_shown()) {
#ifdef NDEBUG
        if (!aw_ask_sure("quit_by_X", "Are you sure to quit?")) return;
#endif
        exit(EXIT_SUCCESS);
    }
    aww->hide();
}

static void aw_set_delete_window_cb(AW_window *aww, Widget shell, bool allow_close) {
    Atom WM_DELETE_WINDOW = XmInternAtom(XtDisplay(shell), (char*)"WM_DELETE_WINDOW", False);

    // remove any previous callbacks
    XmRemoveWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_avoid_destroy_cb,  (caddr_t)aww);
    XmRemoveWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_noexit_destroy_cb, (caddr_t)aww);
    XmRemoveWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_destroy_cb,        (caddr_t)aww);

    if (!allow_close) {
        XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_avoid_destroy_cb, (caddr_t)aww);
    }
    else {
        AW_root *root = aww->get_root();
        if (p_global->no_exit) {
            XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_noexit_destroy_cb, (caddr_t)aww);
        }
        else {
            XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW, (XtCallbackProc)aw_window_destroy_cb, (caddr_t)aww);
        }
    }
}

// ----------------------------------------------------------------------
// force-diff-sync 8294723642364 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::allow_delete_window(bool allow_close) {
    aw_set_delete_window_cb(this, p_w->shell, allow_close);
}

// ----------------------------------------------------------------------
// force-diff-sync 9268347253894 (remove after merging back to trunk)
// ----------------------------------------------------------------------

bool AW_window::is_shown() const {
    // return true if window is shown ( = not invisible and already created)
    // Note: does return TRUE!, if window is only minimized by WM
    return window_is_shown;
}

static void aw_update_window_geometry_awars(AW_window *aww) {
    AW_window_Motif *motif = p_aww(aww);

    short          posx, posy;
    unsigned short width, height, borderwidth;
    XtVaGetValues(motif->shell, // bad hack
                  XmNborderWidth, &borderwidth,
                  XmNwidth, &width,
                  XmNheight, &height,
                  XmNx, &posx,
                  XmNy, &posy,
                  NULL);

    if (motif->knows_WM_offset()) {
        posx -= motif->WM_left_offset;
        posy -= motif->WM_top_offset;

        if (posx<0) posx = 0;
        if (posy<0) posy = 0;

        aww->store_pos_in_awars(posx, posy);
    }
#if defined(DEBUG)
    else {
        fprintf(stderr, "Warning: WM_offsets unknown => did not update awars for window '%s'\n", aww->get_window_title());
    }
#endif
    aww->store_size_in_awars(width, height);
}

void AW_window::show() {
    bool was_shown = true;
    if (!window_is_shown) {
        all_menus_created();
        get_root()->window_show();
        window_is_shown = true;
        was_shown       = false;
    }

    int  width, height; // w/o window frame
    if (recalc_size_at_show != AW_KEEP_SIZE) {
        if (recalc_size_at_show == AW_RESIZE_DEFAULT) {
            // window ignores user-size (even if changed inside current session).
            // used for question boxes, user masks, ...
            window_fit();
            get_window_size(width, height);
        }
        else {
            aw_assert(recalc_size_at_show == AW_RESIZE_USER);
            // check whether user size is too small and increase to minimum window size

            int min_width, min_height;   get_window_size(min_width, min_height);
            int user_width, user_height; get_size_from_awars(user_width, user_height);

            if (user_width <min_width)  user_width  = min_width;
            if (user_height<min_height) user_height = min_height;

            set_window_size(user_width, user_height); // restores user size

            width  = user_width;
            height = user_height;

#if defined(DEBUG)
            if (!was_shown) { // first popup
                // warn about too big default size
#define LOWEST_SUPPORTED_SCREEN_X 1280
#define LOWEST_SUPPORTED_SCREEN_Y 800

                if (min_width>LOWEST_SUPPORTED_SCREEN_X || min_height>LOWEST_SUPPORTED_SCREEN_Y) {
                    fprintf(stderr,
                            "Warning: Window '%s' won't fit on %ix%i (win=%ix%i)\n",
                            get_window_title(),
                            LOWEST_SUPPORTED_SCREEN_X, LOWEST_SUPPORTED_SCREEN_Y,
                            min_width, min_height);
#if defined(DEVEL_RALF)
                    aw_assert(0);
#endif
                }
            }
#endif
        }
        store_size_in_awars(width, height);
        recalc_size_at_show = AW_KEEP_SIZE;
    }
    else {
        get_window_size(width, height);
    }

    {
        int  posx, posy;
        int  swidth, sheight; get_screen_size(swidth, sheight);
        bool posIsFrame = false;

        switch (recalc_pos_at_show) {
            case AW_REPOS_TO_MOUSE_ONCE:
                recalc_pos_at_show = AW_KEEP_POS;
                // fallthrough
            case AW_REPOS_TO_MOUSE: {
                int mx, my;
                if (!get_mouse_pos(mx, my)) goto FALLBACK_CENTER;

                posx   = mx-width/2;
                posy   = my-height/2;
                break;
            }
            case AW_REPOS_TO_CENTER:
            FALLBACK_CENTER:
                posx   = (swidth-width)/2;
                posy   = (sheight-height)/4;
                break;

            case AW_KEEP_POS:
                if (was_shown) {
                    // user might have moved the window -> store (new) positions
                    aw_update_window_geometry_awars(this);
                }
                get_pos_from_awars(posx, posy);
                get_size_from_awars(width, height);
                posIsFrame = true;
                break;
        }

        int frameWidth, frameHeight;
        if (p_w->knows_WM_offset()) {
            frameWidth  = p_w->WM_left_offset;
            frameHeight = p_w->WM_top_offset;
        }
        else {
            // estimate
            frameWidth  = AW_window_Motif::WM_max_left_offset + 1;
            frameHeight = AW_window_Motif::WM_max_top_offset  + 1;
        }

        if (!posIsFrame) {
            posx       -= frameWidth;
            posy       -= frameHeight;
            posIsFrame  = true;
        }

        // avoid windows outside of screen (or too near to screen-edges)
        {
            int maxx = swidth  - 2*frameWidth           - width;
            int maxy = sheight - frameWidth-frameHeight - height; // assumes lower border-width == frameWidth

            if (posx>maxx) posx = maxx;
            if (posy>maxy) posy = maxy;

            if (p_w->knows_WM_offset()) {
                if (posx<0) posx = 0;
                if (posy<0) posy = 0;
            }
            else {
                // workaround a bug leading to wrong WM-offsets (occurs when window-content is placed at screen-border)
                // shift window away from border
                if (posx<frameWidth)  posx = frameWidth;
                if (posy<frameHeight) posy = frameHeight;
            }
        }

        aw_assert(implicated(p_w->knows_WM_offset(), posIsFrame));

        // store position and position window
        store_pos_in_awars(posx, posy);
        set_window_frame_pos(posx, posy);
    }

    XtPopup(p_w->shell, XtGrabNone);
    if (!expose_callback_added) {
        set_expose_callback(AW_INFO_AREA, aw_onExpose_calc_WM_offsets); // @@@ should be removed after it was called once
        expose_callback_added = true;
    }
}

void AW_window::show_modal() {
    recalc_pos_atShow(AW_REPOS_TO_MOUSE);
    get_root()->current_modal_window = this;
    activate();
}

void AW_window::hide() {
    if (window_is_shown) {
        aw_update_window_geometry_awars(this);
        if (hide_cb) hide_cb(this);
        get_root()->window_hide(this);
        window_is_shown = false;
    }
    XtPopdown(p_w->shell);
}

void AW_window::hide_or_notify(const char *error) {
    if (error) aw_message(error);
    else hide();
}

// ----------------------------------------------------------------------
// force-diff-sync 9287423467632 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_window::on_hide(WindowCallbackSimple call_on_hide) {
    hide_cb = call_on_hide;
}

inline void AW_area_management::run_resize_callback() {
    if (resize_cb) resize_cb->run_callbacks();
}

static void AW_resizeCB_draw_area(Widget /*wgt*/, XtPointer aw_cb_struct, XtPointer /*call_data*/) {
    AW_area_management *aram = (AW_area_management *) aw_cb_struct;
    cleanupResizeEvents(aram->get_common()->get_display());
    aram->run_resize_callback();
}

void AW_area_management::set_resize_callback(AW_window *aww, const WindowCallback& cb) {
    // insert resize callback for draw_area
    if (!resize_cb) {
        XtAddCallback(area, XmNresizeCallback,
                (XtCallbackProc) AW_resizeCB_draw_area, (XtPointer) this);
    }
    resize_cb = new AW_cb(aww, cb, 0, resize_cb);
}

// -------------------
//      user input


static void AW_inputCB_draw_area(Widget wgt, XtPointer aw_cb_struct, XmDrawingAreaCallbackStruct *call_data) {
    XEvent             *ev                        = call_data->event;
    AW_cb       *cbs                       = (AW_cb *) aw_cb_struct;
    AW_window          *aww                       = cbs->aw;
    bool                run_callback              = false;
    bool                run_double_click_callback = false;
    AW_area_management *area                      = 0;
    {
        int i;
        for (i=0; i<AW_MAX_AREA; i++) {
            if (p_aww(aww)->areas[i]->get_area() == wgt) {
                area = p_aww(aww)->areas[i];
                break;
            }
        }
    }

    if (ev->xbutton.type == ButtonPress || ev->xbutton.type == ButtonRelease) {
        aww->event.button      = AW_MouseButton(ev->xbutton.button);
        aww->event.x           = ev->xbutton.x;
        aww->event.y           = ev->xbutton.y;
        aww->event.keymodifier = (AW_key_mod)(ev->xbutton.state & (AW_KEYMODE_SHIFT|AW_KEYMODE_CONTROL|AW_KEYMODE_ALT));
        aww->event.keycode     = AW_KEY_NONE;
        aww->event.character   = '\0';

        if (ev->xbutton.type == ButtonPress) {
            aww->event.type = AW_Mouse_Press;
            if (area && area->get_double_click_cb()) {
                if ((ev->xbutton.time - area->get_click_time()) < 200) {
                    run_double_click_callback = true;
                }
                else {
                    run_callback = true;
                }
                area->set_click_time(ev->xbutton.time);
            }
            else {
                run_callback = true;
            }
            aww->event.time = ev->xbutton.time;
        }
        else if (ev->xbutton.type == ButtonRelease) {
            aww->event.type = AW_Mouse_Release;
            run_callback = true;
            // keep event.time set in ButtonPress-branch
        }
    }
    else if (ev->xkey.type == KeyPress || ev->xkey.type == KeyRelease) {
        aww->event.time = ev->xbutton.time;

        const awXKeymap *mykey = aw_xkey_2_awkey(&(ev->xkey));

        aww->event.keycode = mykey->awkey;
        aww->event.keymodifier = mykey->awmod;

        if (mykey->awstr) {
            aww->event.character = mykey->awstr[0];
        }
        else {
            aww->event.character = 0;
        }

        if (ev->xkey.type == KeyPress) {
            aww->event.type = AW_Keyboard_Press;
        }
        else {
            aww->event.type = AW_Keyboard_Release;
        }
        aww->event.button = AW_BUTTON_NONE;
        aww->event.x = ev->xbutton.x;
        aww->event.y = ev->xbutton.y;

        if (!mykey->awmod && mykey->awkey >= AW_KEY_F1 && mykey->awkey
                <= AW_KEY_F12 && p_aww(aww)->modes_f_callbacks && p_aww(aww)->modes_f_callbacks[mykey->awkey-AW_KEY_F1]
                && aww->event.type == AW_Keyboard_Press) {
            p_aww(aww)->modes_f_callbacks[mykey->awkey-AW_KEY_F1]->run_callbacks();
        }
        else {
            run_callback = true;
        }
    }

    if (run_double_click_callback) {
        if (cbs->help_text == (char*)1) {
            cbs->run_callbacks();
        }
        else {
            if (area)
                area->get_double_click_cb()->run_callbacks();
        }
    }

    if (run_callback && (cbs->help_text == (char*)0)) {
        cbs->run_callbacks();
    }
}

void AW_area_management::set_input_callback(AW_window *aww, const WindowCallback& wcb) {
    XtAddCallback(area, XmNinputCallback,
            (XtCallbackProc)AW_inputCB_draw_area,
            (XtPointer)new AW_cb(aww, wcb));
}

void AW_area_management::set_double_click_callback(AW_window *aww, const WindowCallback& wcb) {
    double_click_cb = new AW_cb(aww, wcb, (char*)0, double_click_cb);
}

void AW_window::set_double_click_callback(AW_area area, const WindowCallback& wcb) {
    AW_area_management *aram = MAP_ARAM(area);
    if (!aram)
        return;
    aram->set_double_click_callback(this, wcb);
}

// ---------------
//      motion

static void AW_motionCB(Widget /*w*/, XtPointer aw_cb_struct, XEvent *ev, Boolean*) {
    AW_cb *cbs = (AW_cb *) aw_cb_struct;

    cbs->aw->event.type    = AW_Mouse_Drag;
    cbs->aw->event.x       = ev->xmotion.x;
    cbs->aw->event.y       = ev->xmotion.y;
    cbs->aw->event.keycode = AW_KEY_NONE;

    cbs->run_callbacks();
}
void AW_area_management::set_motion_callback(AW_window *aww, const WindowCallback& wcb) {
    XtAddEventHandler(area, ButtonMotionMask, False,
                      AW_motionCB, (XtPointer) new AW_cb(aww, wcb, ""));
}
static long destroy_awar(const char *, long val, void *) {
    AW_awar *awar = (AW_awar*)val;
    delete   awar;
    return 0; // remove from hash
}

void AW_root::exit_variables() {
    if (hash_table_for_variables) {
        GBS_hash_do_loop(hash_table_for_variables, destroy_awar, NULL);
        GBS_free_hash(hash_table_for_variables);
        hash_table_for_variables = NULL;
    }

    if (hash_for_windows) {
        GBS_free_hash(hash_for_windows);
        hash_for_windows = NULL;
    }

    if (application_database) {
        GBDATA *prop_main    = application_database;
        application_database = NULL;
        GB_close(prop_main);
    }
}

void AW_root::exit_root() {
    aw_uninstall_xkeys();
}

void AW_window::update_scrollbar_settings_from_awars(AW_orientation orientation) {
    AW_screen_area scrolled;
    get_scrollarea_size(&scrolled);

    if (orientation == AW_HORIZONTAL) {
        XtVaSetValues(p_w->scroll_bar_horizontal, XmNpageIncrement, (int)(scrolled.r*(window_local_awar("horizontal_page_increment")->read_int()*0.01)), NULL);
        XtVaSetValues(p_w->scroll_bar_horizontal, XmNincrement,     (int)(window_local_awar("scroll_width_horizontal")->read_int()), NULL);
        XtVaSetValues(p_w->scroll_bar_horizontal, XmNrepeatDelay,   (int)(window_local_awar("scroll_delay_horizontal")->read_int()), NULL);
    }
    else {
        XtVaSetValues(p_w->scroll_bar_vertical, XmNpageIncrement, (int)(scrolled.b*(window_local_awar("vertical_page_increment")->read_int()*0.01)), NULL);
        XtVaSetValues(p_w->scroll_bar_vertical, XmNincrement,     (int)(window_local_awar("scroll_width_vertical")->read_int()), NULL);
        XtVaSetValues(p_w->scroll_bar_vertical, XmNrepeatDelay,   (int)(window_local_awar("scroll_delay_vertical")->read_int()), NULL);
    }
}

void AW_area_management::create_devices(AW_window *aww, AW_area ar) {
    AW_root *root = aww->get_root();
    common = new AW_common_Xm(XtDisplay(area), XtWindow(area), p_global->color_table, aww->color_table, aww->color_table_size, aww, ar);
}

AW_color_idx AW_window::alloc_named_data_color(int colnum, const char *colorname) {
    if (!color_table_size) {
        color_table_size = AW_STD_COLOR_IDX_MAX + colnum;
        color_table      = (AW_rgb*)malloc(sizeof(AW_rgb) *color_table_size);
        for (int i = 0; i<color_table_size; ++i) color_table[i] = AW_NO_COLOR;
    }
    else {
        if (colnum>=color_table_size) {
            long new_size  = colnum+8;
            realloc_unleaked(color_table, new_size*sizeof(AW_rgb)); // valgrinders : never freed because AW_window never is freed
            if (!color_table) GBK_terminate("out of memory");
            for (int i = color_table_size; i<new_size; ++i) color_table[i] = AW_NO_COLOR;
            color_table_size                                               = new_size;
        }
    }
    XColor xcolor_returned, xcolor_exakt;

    if (p_global->screen_depth == 1) { // Black and White Monitor
        static int col = 1;
        if (colnum == AW_DATA_BG) {
            col = 1;
            if (strcmp(colorname, "white"))
                col *= -1;
        }
        if (col==1) {
            color_table[colnum] = WhitePixelOfScreen(XtScreen(p_global->toplevel_widget));
        }
        else {
            color_table[colnum] = BlackPixelOfScreen(XtScreen(p_global->toplevel_widget));
        }
        if (colnum == AW_DATA_BG)
            col *= -1;
    }
    else { // Color monitor
        if (color_table[colnum] != AW_NO_COLOR) {
            unsigned long color = color_table[colnum];
            XFreeColors(p_global->display, p_global->colormap, &color, 1, 0);
        }
        if (XAllocNamedColor(p_global->display, p_global->colormap, colorname, &xcolor_returned, &xcolor_exakt) == 0) {
            aw_message(GBS_global_string("XAllocColor failed: %s\n", colorname));
            color_table[colnum] = AW_NO_COLOR;
        }
        else {
            color_table[colnum] = xcolor_returned.pixel;
        }
    }
    if (colnum == AW_DATA_BG) {
        XtVaSetValues(p_w->areas[AW_MIDDLE_AREA]->get_area(), XmNbackground, color_table[colnum], NULL);
    }
    return (AW_color_idx)colnum;
}

void AW_window::create_devices() {
    unsigned long background_color;
    if (p_w->areas[AW_INFO_AREA]) {
        p_w->areas[AW_INFO_AREA]->create_devices(this, AW_INFO_AREA);
        XtVaGetValues(p_w->areas[AW_INFO_AREA]->get_area(), XmNbackground, &background_color, NULL);
        p_global->color_table[AW_WINDOW_DRAG] = background_color ^ p_global->color_table[AW_WINDOW_FG];
    }
    if (p_w->areas[AW_MIDDLE_AREA]) {
        p_w->areas[AW_MIDDLE_AREA]->create_devices(this, AW_MIDDLE_AREA);
    }
    if (p_w->areas[AW_BOTTOM_AREA]) {
        p_w->areas[AW_BOTTOM_AREA]->create_devices(this, AW_BOTTOM_AREA);
    }
}

void aw_insert_default_help_entries(AW_window *aww) {
    aww->insert_help_topic("Click here and then on the questionable button/menu/...", "q", 0, AWM_ALL, AW_help_entry_pressed);

    aww->insert_help_topic("How to use help", "H", "help.hlp", AWM_ALL, makeHelpCallback("help.hlp"));
    aww->insert_help_topic("ARB help",        "A", "arb.hlp",  AWM_ALL, makeHelpCallback("arb.hlp"));
}

inline char *strdup_getlen(const char *str, int& len) {
    len = strlen(str);
    return GB_strduplen(str, len);
}
Label::Label(const char *labeltext, AW_window *aww) {
    imageref = AW_IS_IMAGEREF(labeltext);
    if (imageref) {
        label = strdup_getlen(AW_get_pixmapPath(labeltext+1), len);
    }
    else {
        AW_awar *is_awar = aww->get_root()->label_is_awar(labeltext);

        if (is_awar) { // for labels displaying awar values, insert dummy text here
            len = aww->get_at().length_of_buttons - 2;
            if (len < 1) len = 1;

            label = (char*)malloc(len+1);
            memset(label, 'y', len);
            label[len] = 0;
        }
        else {
            label = strdup_getlen(labeltext, len);
        }
    }
}

void AW_label_in_awar_list(AW_window *aww, Widget widget, const char *str) {
    AW_awar *is_awar = aww->get_root()->label_is_awar(str);
    if (is_awar) {
        char *display = is_awar->read_as_string();

        if (!display) {
            aw_assert(0); // awar not found
            freeset(display, GBS_global_string_copy("<undef AWAR: %s>", str));
        }
        if (!display[0]) freeset(display, strdup(" ")); // empty string causes wrong-sized buttons

        aww->update_label(widget, display);
        free(display);

        is_awar->tie_widget(0, widget, AW_WIDGET_LABEL_FIELD, aww);
    }
}

static long aw_loop_get_window_geometry(const char *, long val, void *) {
    aw_update_window_geometry_awars((AW_window *)val);
    return val;
}
void aw_update_all_window_geometry_awars(AW_root *awr) {
    GBS_hash_do_loop(awr->hash_for_windows, aw_loop_get_window_geometry, NULL);
}

static long aw_loop_forget_window_geometry(const char *, long val, void *) {
    ((AW_window*)val)->reset_geometry_awars();
    return val;
}
void AW_forget_all_window_geometry(AW_window *aww) {
    AW_root *awr = aww->get_root();
    GBS_hash_do_loop(awr->hash_for_windows, aw_loop_forget_window_geometry, NULL); // reset geometry for used windows

    // reset geometry in stored, unused windows
    GBDATA         *gb_props = awr->check_properties(NULL);
    GB_transaction  ta(gb_props);
    GBDATA         *gb_win   = GB_search(gb_props, LAYOUT_AWAR_ROOT, GB_FIND);
    if (gb_win) {
        for (GBDATA *gbw = GB_child(gb_win); gbw; ) {
            const char *key     = GB_read_key_pntr(gbw);
            long        usedWin = GBS_read_hash(awr->hash_for_windows, key);
            GBDATA     *gbnext  = GB_nextChild(gbw);

            if (!usedWin) {
                GB_ERROR error = GB_delete(gbw);
                aw_message_if(error);
            }
            gbw = gbnext;
        }
    }
}

static const char *existingPixmap(const char *icon_relpath, const char *name) {
    const char *icon_relname  = GBS_global_string("%s/%s.xpm", icon_relpath, name);
    const char *icon_fullname = AW_get_pixmapPath(icon_relname);

    if (!GB_is_regularfile(icon_fullname)) icon_fullname = NULL;

    return icon_fullname;
}


static Pixmap getIcon(Screen *screen, const char *iconName, Pixel foreground, Pixel background) {
    static SmartCustomPtr(GB_HASH, GBS_free_hash) icon_hash;
    if (icon_hash.isNull()) {
        icon_hash = GBS_create_hash(100, GB_MIND_CASE);
    }

    Pixmap pixmap = GBS_read_hash(&*icon_hash, iconName);

    if (!pixmap && iconName) {
        const char *iconFile = existingPixmap("icons", iconName);

        if (iconFile) {
            char *ico = strdup(iconFile);
            pixmap    = XmGetPixmap(screen, ico, foreground, background);
            GBS_write_hash(&*icon_hash, iconName, pixmap);
            free(ico);
        }
    }

    return pixmap;
}

Widget aw_create_shell(AW_window *aww, bool allow_resize, bool allow_close, int width, int height, int posx, int posy) { // @@@ move into AW_window
    AW_root *root = aww->get_root();
    Widget shell;

    // set minimum window size to size provided by init
    if (width >aww->get_at().max_x_size) aww->get_at().max_x_size = width;
    if (height>aww->get_at().max_y_size) aww->get_at().max_y_size = height;

    bool has_user_geometry = false;
    if (!GBS_read_hash(root->hash_for_windows, aww->get_window_id())) {
        GBS_write_hash(root->hash_for_windows, aww->get_window_id(), (long)aww);

        aww->create_user_geometry_awars(posx, posy, width, height);
    }
    {
        int user_width, user_height; aww->get_size_from_awars(user_width, user_height);
        int user_posx,  user_posy;   aww->get_pos_from_awars(user_posx,  user_posy);

        if (allow_resize) {
            if (width != user_width) { width = user_width; has_user_geometry = true; }
            if (height != user_height) { height = user_height; has_user_geometry = true; }
        }

        // @@@ FIXME:  maximum should be set to current screen size minus some offset
        // to ensure that windows do not appear outside screen
        if (posx != user_posx) { posx = user_posx; has_user_geometry = true; }
        if (posy != user_posy) { posy = user_posy; has_user_geometry = true; }

        if (has_user_geometry) {
            aww->recalc_size_atShow(AW_RESIZE_USER); // keep user geometry (only if user size is smaller than default size, the latter is used)
        }
        else { // no geometry yet
            aww->recalc_pos_atShow(AW_REPOS_TO_MOUSE_ONCE); // popup the window at current mouse position
        }
    }
    
    if (allow_resize) {
        // create the window big enough to ensure that all widgets
        // are created in visible area (otherwise widget are crippled).
        // window will be resized later (on show)

        width  = WIDER_THAN_SCREEN;
        height = HIGHER_THAN_SCREEN;

        aww->recalc_size_atShow(AW_RESIZE_ANY);
    }

    Widget  father      = p_global->toplevel_widget;
    Screen *screen      = XtScreen(father);
    Pixmap  icon_pixmap = getIcon(screen, aww->window_defaults_name, p_global->foreground, p_global->background);

    if (!icon_pixmap) {
        icon_pixmap = getIcon(screen, root->program_name, p_global->foreground, p_global->background);
    }

    if (!icon_pixmap) {
        GBK_terminatef("Missing icon pixmap for window '%s'\n", aww->window_defaults_name);
    }
    else if (icon_pixmap == XmUNSPECIFIED_PIXMAP) {
        GBK_terminatef("Failed to load icon pixmap for window '%s'\n", aww->window_defaults_name);
    }

    int focusPolicy = root->focus_follows_mouse ? XmPOINTER : XmEXPLICIT;

    {
        aw_xargs args(9);

        args.add(XmNwidth, width);
        args.add(XmNheight, height);
        args.add(XmNx, posx);
        args.add(XmNy, posy);
        args.add(XmNtitle, (XtArgVal)aww->window_name);
        args.add(XmNiconName, (XtArgVal)aww->window_name);
        args.add(XmNkeyboardFocusPolicy, focusPolicy);
        args.add(XmNdeleteResponse, XmDO_NOTHING);
        args.add(XtNiconPixmap, icon_pixmap);

        if (!p_global->main_widget || !p_global->main_aww->is_shown()) {
            shell = XtCreatePopupShell("editor", applicationShellWidgetClass, father, args.list(), args.size());
        }
        else {
            shell = XtCreatePopupShell("transient", transientShellWidgetClass, father, args.list(), args.size());
        }
    }

    if (!p_global->main_widget) {
        p_global->main_widget = shell;
        p_global->main_aww    = aww;
    }
    else {
        if (!p_global->main_aww->is_shown()) {  // now i am the root window
            p_global->main_widget = shell;
            p_global->main_aww    = aww;
        }
    }

    aw_set_delete_window_cb(aww, shell, allow_close);

    // set icon window (for window managers where iconified applications are dropped onto desktop or similar)
    {
        Window icon_window;
        XtVaGetValues(shell, XmNiconWindow, &icon_window, NULL);

        Display *dpy = XtDisplay(shell);
        if (!icon_window) {
            XSetWindowAttributes attr;
            attr.background_pixmap = icon_pixmap;

            int          xpos, ypos;
            unsigned int xsize, ysize, borderwidth, depth;
            Window       wroot;

            if (XGetGeometry(dpy, icon_pixmap, &wroot, &xpos, &ypos, &xsize, &ysize, &borderwidth, &depth)) {
                icon_window = XCreateWindow(dpy, wroot, 0, 0, xsize, ysize, 0, depth, CopyFromParent, CopyFromParent, CWBackPixmap, &attr);
            }
        }
        if (!icon_window) {
            XtVaSetValues(shell, XmNiconPixmap, icon_pixmap, NULL);
        }
        else {
            XtVaSetValues(shell, XmNiconWindow, icon_window, NULL);
            XSetWindowBackgroundPixmap(dpy, icon_window, icon_pixmap);
            XClearWindow(dpy, icon_window);
        }
    }

    return shell;
}


void aw_realize_widget(AW_window *aww) {
    if (p_aww(aww)->areas[AW_INFO_AREA] && p_aww(aww)->areas[AW_INFO_AREA]->get_form()) {
        XtManageChild(p_aww(aww)->areas[AW_INFO_AREA]->get_form());
    }
    if (p_aww(aww)->areas[AW_MIDDLE_AREA] && p_aww(aww)->areas[AW_MIDDLE_AREA]->get_form()) {
        XtManageChild(p_aww(aww)->areas[AW_MIDDLE_AREA]->get_form());
    }
    if (p_aww(aww)->areas[AW_BOTTOM_AREA] && p_aww(aww)->areas[AW_BOTTOM_AREA]->get_form()) {
        XtManageChild(p_aww(aww)->areas[AW_BOTTOM_AREA]->get_form());
    }
    XtRealizeWidget(p_aww(aww)->shell);
    p_aww(aww)->WM_top_offset = AW_CALC_OFFSET_ON_EXPOSE;
}

void AW_window_menu_modes::init(AW_root *root_in, const char *wid, const char *windowname, int  width, int height) { 
    Widget      main_window;
    Widget      help_popup;
    Widget      help_label;
    Widget      separator;
    Widget      form1;
    Widget      form2;
    const char *help_button   = "HELP";
    const char *help_mnemonic = "H";

#if defined(DUMP_MENU_LIST)
    initMenuListing(windowname);
#endif // DUMP_MENU_LIST
    root = root_in; // for macro
    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    int posx = 50;
    int posy = 50;

    p_w->shell = aw_create_shell(this, true, true, width, height, posx, posy);

    main_window = XtVaCreateManagedWidget("mainWindow1",
                                          xmMainWindowWidgetClass, p_w->shell,
                                          NULL);

    p_w->menu_bar[0] = XtVaCreateManagedWidget("menu1", xmRowColumnWidgetClass,
                                               main_window,
                                               XmNrowColumnType, XmMENU_BAR,
                                               NULL);

    // create shell for help-cascade
    help_popup = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
                                      p_w->menu_bar[0],
                                      XmNwidth, 1,
                                      XmNheight, 1,
                                      XmNallowShellResize, true,
                                      XmNoverrideRedirect, true,
                                      NULL);

    // create row column in Pull-Down shell
    p_w->help_pull_down = XtVaCreateWidget("menu_row_column",
                                           xmRowColumnWidgetClass, help_popup,
                                           XmNrowColumnType, XmMENU_PULLDOWN,
                                           NULL);

    // create HELP-label in menu bar
    help_label = XtVaCreateManagedWidget("menu1_top_b1",
                                         xmCascadeButtonWidgetClass, p_w->menu_bar[0],
                                         RES_CONVERT(XmNlabelString, help_button),
                                         RES_CONVERT(XmNmnemonic, help_mnemonic),
                                         XmNsubMenuId, p_w->help_pull_down, NULL);
    XtVaSetValues(p_w->menu_bar[0], XmNmenuHelpWidget, help_label, NULL);
    root->make_sensitive(help_label, AWM_ALL);

    form1 = XtVaCreateManagedWidget("form1",
                                    xmFormWidgetClass,
                                    main_window,
                                    // XmNwidth, width,
                                    // XmNheight, height,
                                    XmNresizePolicy, XmRESIZE_NONE,
                                    // XmNx, 0,
                                    // XmNy, 0,
                                    NULL);

    p_w->mode_area = XtVaCreateManagedWidget("mode area",
                                             xmDrawingAreaWidgetClass,
                                             form1,
                                             XmNresizePolicy, XmRESIZE_NONE,
                                             XmNwidth, 38,
                                             XmNheight, height,
                                             XmNx, 0,
                                             XmNy, 0,
                                             XmNleftOffset, 0,
                                             XmNtopOffset, 0,
                                             XmNbottomAttachment, XmATTACH_FORM,
                                             XmNleftAttachment, XmATTACH_POSITION,
                                             XmNtopAttachment, XmATTACH_POSITION,
                                             XmNmarginHeight, 2,
                                             XmNmarginWidth, 1,
                                             NULL);

    separator = XtVaCreateManagedWidget("separator",
                                        xmSeparatorWidgetClass,
                                        form1,
                                        XmNx, 37,
                                        XmNshadowThickness, 4,
                                        XmNorientation, XmVERTICAL,
                                        XmNbottomAttachment, XmATTACH_FORM,
                                        XmNtopAttachment, XmATTACH_FORM,
                                        XmNleftAttachment, XmATTACH_NONE,
                                        XmNleftWidget, NULL,
                                        XmNrightAttachment, XmATTACH_NONE,
                                        XmNleftOffset, 70,
                                        XmNleftPosition, 0,
                                        NULL);

    form2 = XtVaCreateManagedWidget("form2",
                                    xmFormWidgetClass,
                                    form1,
                                    XmNwidth, width,
                                    XmNheight, height,
                                    XmNtopOffset, 0,
                                    XmNbottomOffset, 0,
                                    XmNleftOffset, 0,
                                    XmNrightOffset, 0,
                                    XmNrightAttachment, XmATTACH_FORM,
                                    XmNbottomAttachment, XmATTACH_FORM,
                                    XmNleftAttachment, XmATTACH_WIDGET,
                                    XmNleftWidget, separator,
                                    XmNtopAttachment, XmATTACH_POSITION,
                                    XmNresizePolicy, XmRESIZE_NONE,
                                    XmNx, 0,
                                    XmNy, 0,
                                    NULL);
    p_w->areas[AW_INFO_AREA] =
        new AW_area_management(form2, XtVaCreateManagedWidget("info_area",
                                                              xmDrawingAreaWidgetClass,
                                                              form2,
                                                              XmNheight, 0,
                                                              XmNbottomAttachment, XmATTACH_NONE,
                                                              XmNtopAttachment, XmATTACH_FORM,
                                                              XmNleftAttachment, XmATTACH_FORM,
                                                              XmNrightAttachment, XmATTACH_FORM,
                                                              XmNmarginHeight, 2,
                                                              XmNmarginWidth, 2,
                                                              NULL));

    p_w->areas[AW_BOTTOM_AREA] =
        new AW_area_management(form2, XtVaCreateManagedWidget("bottom_area",
                                                              xmDrawingAreaWidgetClass,
                                                              form2,
                                                              XmNheight, 0,
                                                              XmNbottomAttachment, XmATTACH_FORM,
                                                              XmNtopAttachment, XmATTACH_NONE,
                                                              XmNleftAttachment, XmATTACH_FORM,
                                                              XmNrightAttachment, XmATTACH_FORM,
                                                              NULL));

    p_w->scroll_bar_horizontal = XtVaCreateManagedWidget("scroll_bar_horizontal",
                                                         xmScrollBarWidgetClass,
                                                         form2,
                                                         XmNheight, 15,
                                                         XmNminimum, 0,
                                                         XmNmaximum, AW_SCROLL_MAX,
                                                         XmNincrement, 10,
                                                         XmNsliderSize, AW_SCROLL_MAX,
                                                         XmNrightAttachment, XmATTACH_FORM,
                                                         XmNbottomAttachment, XmATTACH_FORM,
                                                         XmNbottomOffset, 0,
                                                         XmNleftAttachment, XmATTACH_FORM,
                                                         XmNtopAttachment, XmATTACH_NONE,
                                                         XmNorientation, XmHORIZONTAL,
                                                         XmNrightOffset, 18,
                                                         NULL);

    p_w->scroll_bar_vertical = XtVaCreateManagedWidget("scroll_bar_vertical",
                                                       xmScrollBarWidgetClass,
                                                       form2,
                                                       XmNwidth, 15,
                                                       XmNminimum, 0,
                                                       XmNmaximum, AW_SCROLL_MAX,
                                                       XmNincrement, 10,
                                                       XmNsliderSize, AW_SCROLL_MAX,
                                                       XmNrightAttachment, XmATTACH_FORM,
                                                       XmNbottomAttachment, XmATTACH_WIDGET,
                                                       XmNbottomWidget, p_w->scroll_bar_horizontal,
                                                       XmNbottomOffset, 3,
                                                       XmNleftOffset, 3,
                                                       XmNrightOffset, 3,
                                                       XmNleftAttachment, XmATTACH_NONE,
                                                       XmNtopAttachment, XmATTACH_WIDGET,
                                                       XmNtopWidget, INFO_WIDGET,
                                                       NULL);

    p_w->frame = XtVaCreateManagedWidget("draw_area",
                                         xmFrameWidgetClass,
                                         form2,
                                         XmNshadowType, XmSHADOW_IN,
                                         XmNshadowThickness, 2,
                                         XmNleftOffset, 3,
                                         XmNtopOffset, 3,
                                         XmNbottomOffset, 3,
                                         XmNrightOffset, 3,
                                         XmNbottomAttachment, XmATTACH_WIDGET,
                                         XmNbottomWidget, p_w->scroll_bar_horizontal,
                                         XmNtopAttachment, XmATTACH_FORM,
                                         XmNtopOffset, 0,
                                         XmNleftAttachment, XmATTACH_FORM,
                                         XmNrightAttachment, XmATTACH_WIDGET,
                                         XmNrightWidget, p_w->scroll_bar_vertical,
                                         NULL);

    p_w->areas[AW_MIDDLE_AREA] =
        new AW_area_management(p_w->frame, XtVaCreateManagedWidget("draw area",
                                                                   xmDrawingAreaWidgetClass,
                                                                   p_w->frame,
                                                                   XmNmarginHeight, 0,
                                                                   XmNmarginWidth, 0,
                                                                   NULL));

    XmMainWindowSetAreas(main_window, p_w->menu_bar[0], (Widget) NULL, (Widget) NULL, (Widget) NULL, form1);

    aw_realize_widget(this);

    create_devices();
    aw_insert_default_help_entries(this);
    create_window_variables();
}

void AW_window_menu::init(AW_root *root_in, const char *wid, const char *windowname, int width, int height) { 
    Widget      main_window;
    Widget      help_popup;
    Widget      help_label;
    Widget      separator;
    Widget      form1;
    Widget      form2;
    const char *help_button   = "HELP";
    const char *help_mnemonic = "H";

#if defined(DUMP_MENU_LIST)
    initMenuListing(windowname);
#endif // DUMP_MENU_LIST
    root = root_in; // for macro
    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    int posx = 50;
    int posy = 50;

    p_w->shell = aw_create_shell(this, true, true, width, height, posx, posy);

    main_window = XtVaCreateManagedWidget("mainWindow1",
                                          xmMainWindowWidgetClass, p_w->shell,
                                          NULL);

    p_w->menu_bar[0] = XtVaCreateManagedWidget("menu1", xmRowColumnWidgetClass,
                                               main_window,
                                               XmNrowColumnType, XmMENU_BAR,
                                               NULL);

    // create shell for help-cascade
    help_popup = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
                                      p_w->menu_bar[0],
                                      XmNwidth, 1,
                                      XmNheight, 1,
                                      XmNallowShellResize, true,
                                      XmNoverrideRedirect, true,
                                      NULL);

    // create row column in Pull-Down shell
    p_w->help_pull_down = XtVaCreateWidget("menu_row_column",
                                           xmRowColumnWidgetClass, help_popup,
                                           XmNrowColumnType, XmMENU_PULLDOWN,
                                           NULL);

    // create HELP-label in menu bar
    help_label = XtVaCreateManagedWidget("menu1_top_b1",
                                         xmCascadeButtonWidgetClass, p_w->menu_bar[0],
                                         RES_CONVERT(XmNlabelString, help_button),
                                         RES_CONVERT(XmNmnemonic, help_mnemonic),
                                         XmNsubMenuId, p_w->help_pull_down, NULL);
    XtVaSetValues(p_w->menu_bar[0], XmNmenuHelpWidget, help_label, NULL);
    root->make_sensitive(help_label, AWM_ALL);

    form1 = XtVaCreateManagedWidget("form1",
                                    xmFormWidgetClass,
                                    main_window,
                                    // XmNwidth, width,
                                    // XmNheight, height,
                                    XmNresizePolicy, XmRESIZE_NONE,
                                    // XmNx, 0,
                                    // XmNy, 0,
                                    NULL);

    p_w->mode_area = XtVaCreateManagedWidget("mode area",
                                             xmDrawingAreaWidgetClass,
                                             form1,
                                             XmNresizePolicy, XmRESIZE_NONE,
                                             XmNwidth, 17,
                                             XmNheight, height,
                                             XmNx, 0,
                                             XmNy, 0,
                                             XmNleftOffset, 0,
                                             XmNtopOffset, 0,
                                             XmNbottomAttachment, XmATTACH_FORM,
                                             XmNleftAttachment, XmATTACH_POSITION,
                                             XmNtopAttachment, XmATTACH_POSITION,
                                             XmNmarginHeight, 2,
                                             XmNmarginWidth, 1,
                                             NULL);

    separator = p_w->mode_area;

    form2 = XtVaCreateManagedWidget("form2",
                                    xmFormWidgetClass,
                                    form1,
                                    XmNwidth, width,
                                    XmNheight, height,
                                    XmNtopOffset, 0,
                                    XmNbottomOffset, 0,
                                    XmNleftOffset, 0,
                                    XmNrightOffset, 0,
                                    XmNrightAttachment, XmATTACH_FORM,
                                    XmNbottomAttachment, XmATTACH_FORM,
                                    XmNleftAttachment, XmATTACH_WIDGET,
                                    XmNleftWidget, separator,
                                    XmNtopAttachment, XmATTACH_POSITION,
                                    XmNresizePolicy, XmRESIZE_NONE,
                                    XmNx, 0,
                                    XmNy, 0,
                                    NULL);
    p_w->areas[AW_INFO_AREA] =
        new AW_area_management(form2, XtVaCreateManagedWidget("info_area",
                                                              xmDrawingAreaWidgetClass,
                                                              form2,
                                                              XmNheight, 0,
                                                              XmNbottomAttachment, XmATTACH_NONE,
                                                              XmNtopAttachment, XmATTACH_FORM,
                                                              XmNleftAttachment, XmATTACH_FORM,
                                                              XmNrightAttachment, XmATTACH_FORM,
                                                              XmNmarginHeight, 2,
                                                              XmNmarginWidth, 2,
                                                              NULL));

    p_w->areas[AW_BOTTOM_AREA] =
        new AW_area_management(form2, XtVaCreateManagedWidget("bottom_area",
                                                              xmDrawingAreaWidgetClass,
                                                              form2,
                                                              XmNheight, 0,
                                                              XmNbottomAttachment, XmATTACH_FORM,
                                                              XmNtopAttachment, XmATTACH_NONE,
                                                              XmNleftAttachment, XmATTACH_FORM,
                                                              XmNrightAttachment, XmATTACH_FORM,
                                                              NULL));

    p_w->scroll_bar_horizontal = XtVaCreateManagedWidget("scroll_bar_horizontal",
                                                         xmScrollBarWidgetClass,
                                                         form2,
                                                         XmNheight, 15,
                                                         XmNminimum, 0,
                                                         XmNmaximum, AW_SCROLL_MAX,
                                                         XmNincrement, 10,
                                                         XmNsliderSize, AW_SCROLL_MAX,
                                                         XmNrightAttachment, XmATTACH_FORM,
                                                         XmNbottomAttachment, XmATTACH_FORM,
                                                         XmNbottomOffset, 0,
                                                         XmNleftAttachment, XmATTACH_FORM,
                                                         XmNtopAttachment, XmATTACH_NONE,
                                                         XmNorientation, XmHORIZONTAL,
                                                         XmNrightOffset, 18,
                                                         NULL);

    p_w->scroll_bar_vertical = XtVaCreateManagedWidget("scroll_bar_vertical",
                                                       xmScrollBarWidgetClass,
                                                       form2,
                                                       XmNwidth, 15,
                                                       XmNminimum, 0,
                                                       XmNmaximum, AW_SCROLL_MAX,
                                                       XmNincrement, 10,
                                                       XmNsliderSize, AW_SCROLL_MAX,
                                                       XmNrightAttachment, XmATTACH_FORM,
                                                       XmNbottomAttachment, XmATTACH_WIDGET,
                                                       XmNbottomWidget, p_w->scroll_bar_horizontal,
                                                       XmNbottomOffset, 3,
                                                       XmNleftOffset, 3,
                                                       XmNrightOffset, 3,
                                                       XmNleftAttachment, XmATTACH_NONE,
                                                       XmNtopAttachment, XmATTACH_WIDGET,
                                                       XmNtopWidget, INFO_WIDGET,
                                                       NULL);

    p_w->frame = XtVaCreateManagedWidget("draw_area",
                                         xmFrameWidgetClass,
                                         form2,
                                         XmNshadowType, XmSHADOW_IN,
                                         XmNshadowThickness, 2,
                                         XmNleftOffset, 3,
                                         XmNtopOffset, 3,
                                         XmNbottomOffset, 3,
                                         XmNrightOffset, 3,
                                         XmNbottomAttachment, XmATTACH_WIDGET,
                                         XmNbottomWidget, p_w->scroll_bar_horizontal,
                                         XmNtopAttachment, XmATTACH_FORM,
                                         XmNtopOffset, 0,
                                         XmNleftAttachment, XmATTACH_FORM,
                                         XmNrightAttachment, XmATTACH_WIDGET,
                                         XmNrightWidget, p_w->scroll_bar_vertical,
                                         NULL);

    p_w->areas[AW_MIDDLE_AREA] =
        new AW_area_management(p_w->frame, XtVaCreateManagedWidget("draw area",
                                                                   xmDrawingAreaWidgetClass,
                                                                   p_w->frame,
                                                                   XmNmarginHeight, 0,
                                                                   XmNmarginWidth, 0,
                                                                   NULL));

    XmMainWindowSetAreas(main_window, p_w->menu_bar[0], (Widget) NULL,
                         (Widget) NULL, (Widget) NULL, form1);

    aw_realize_widget(this);

    create_devices();
    aw_insert_default_help_entries(this);
    create_window_variables();
}

void AW_window_simple::init(AW_root *root_in, const char *wid, const char *windowname) {
    root = root_in; // for macro

    int width  = 100; // this is only the minimum size!
    int height = 100;
    int posx   = 50;
    int posy   = 50;

    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    p_w->shell = aw_create_shell(this, true, true, width, height, posx, posy);

    // add this to disable resize or maximize in simple dialogs (avoids broken layouts)
    // XtVaSetValues(p_w->shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE, NULL);

    Widget form1 = XtVaCreateManagedWidget("forms", xmFormWidgetClass,
            p_w->shell,
            NULL);

    p_w->areas[AW_INFO_AREA] =
        new AW_area_management(form1, XtVaCreateManagedWidget("info_area",
                                                              xmDrawingAreaWidgetClass,
                                                              form1,
                                                              XmNbottomAttachment, XmATTACH_FORM,
                                                              XmNtopAttachment, XmATTACH_FORM,
                                                              XmNleftAttachment, XmATTACH_FORM,
                                                              XmNrightAttachment, XmATTACH_FORM,
                                                              XmNmarginHeight, 2,
                                                              XmNmarginWidth, 2,
                                                              NULL));

    aw_realize_widget(this);
    create_devices();
}

void AW_window_simple_menu::init(AW_root *root_in, const char *wid, const char *windowname) { 
    root = root_in; // for macro

    const char *help_button = "HELP";
    const char *help_mnemonic = "H";
    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    int width = 100;
    int height = 100;
    int posx = 50;
    int posy = 50;

    p_w->shell = aw_create_shell(this, true, true, width, height, posx, posy);

    Widget main_window;
    Widget help_popup;
    Widget help_label;
    Widget form1;

    main_window = XtVaCreateManagedWidget("mainWindow1",
                                          xmMainWindowWidgetClass, p_w->shell,
                                          NULL);

    p_w->menu_bar[0] = XtVaCreateManagedWidget("menu1", xmRowColumnWidgetClass,
                                               main_window,
                                               XmNrowColumnType, XmMENU_BAR,
                                               NULL);

    // create shell for help-cascade
    help_popup = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
                                      p_w->menu_bar[0],
                                      XmNwidth, 1,
                                      XmNheight, 1,
                                      XmNallowShellResize, true,
                                      XmNoverrideRedirect, true,
                                      NULL);

    // create row column in Pull-Down shell
    p_w->help_pull_down = XtVaCreateWidget("menu_row_column",
                                           xmRowColumnWidgetClass, help_popup,
                                           XmNrowColumnType, XmMENU_PULLDOWN,
                                           NULL);

    // create HELP-label in menu bar
    help_label = XtVaCreateManagedWidget("menu1_top_b1",
                                         xmCascadeButtonWidgetClass, p_w->menu_bar[0],
                                         RES_CONVERT(XmNlabelString, help_button),
                                         RES_CONVERT(XmNmnemonic, help_mnemonic),
                                         XmNsubMenuId, p_w->help_pull_down, NULL);
    XtVaSetValues(p_w->menu_bar[0], XmNmenuHelpWidget, help_label, NULL);
    root->make_sensitive(help_label, AWM_ALL);

    form1 = XtVaCreateManagedWidget("form1",
                                    xmFormWidgetClass,
                                    main_window,
                                    XmNtopOffset, 10,
                                    XmNresizePolicy, XmRESIZE_NONE,
                                    NULL);

    p_w->areas[AW_INFO_AREA] =
        new AW_area_management(form1, XtVaCreateManagedWidget("info_area",
                                                              xmDrawingAreaWidgetClass,
                                                              form1,
                                                              XmNbottomAttachment, XmATTACH_FORM,
                                                              XmNtopAttachment, XmATTACH_FORM,
                                                              XmNleftAttachment, XmATTACH_FORM,
                                                              XmNrightAttachment, XmATTACH_FORM,
                                                              XmNmarginHeight, 2,
                                                              XmNmarginWidth, 2,
                                                              NULL));

    aw_realize_widget(this);

    aw_insert_default_help_entries(this);
    create_devices();
}

void AW_window_message::init(AW_root *root_in, const char *wid, const char *windowname, bool allow_close) {
    root = root_in; // for macro

    int width  = 100;
    int height = 100;
    int posx   = 50;
    int posy   = 50;

    window_name          = strdup(windowname);
    window_defaults_name = strdup(wid);

    // create shell for message box
    p_w->shell = aw_create_shell(this, true, allow_close, width, height, posx, posy);

    // disable resize or maximize in simple dialogs (avoids broken layouts)
    XtVaSetValues(p_w->shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE,
            NULL);

    p_w->areas[AW_INFO_AREA] =
        new AW_area_management(p_w->shell, XtVaCreateManagedWidget("info_area",
                                                                   xmDrawingAreaWidgetClass,
                                                                   p_w->shell,
                                                                   XmNheight, 0,
                                                                   XmNbottomAttachment, XmATTACH_NONE,
                                                                   XmNtopAttachment, XmATTACH_FORM,
                                                                   XmNleftAttachment, XmATTACH_FORM,
                                                                   XmNrightAttachment, XmATTACH_FORM,
                                                                   NULL));

    aw_realize_widget(this);
}
void AW_window_message::init(AW_root *root_in, const char *windowname, bool allow_close) {
    char *wid = GBS_string_2_key(windowname);
    init(root_in, wid, windowname, allow_close);
    free(wid);
}

void AW_window::set_focus_policy(bool follow_mouse) {
    int focusPolicy = follow_mouse ? XmPOINTER : XmEXPLICIT;
    XtVaSetValues(p_w->shell, XmNkeyboardFocusPolicy, focusPolicy, NULL);
}

void AW_window::select_mode(int mode) {
    if (mode >= p_w->number_of_modes)
        return;

    Widget oldwidget = p_w->modes_widgets[p_w->selected_mode];
    p_w->selected_mode = mode;
    Widget widget = p_w->modes_widgets[p_w->selected_mode];
    XtVaSetValues(oldwidget, XmNbackground, p_global->background, NULL);
    XtVaSetValues(widget, XmNbackground, p_global->foreground, NULL);
}

static void aw_mode_callback(AW_window *aww, short mode, AW_cb *cbs) {
    aww->select_mode(mode);
    cbs->run_callbacks();
}

#define MODE_BUTTON_OFFSET 34
inline int yoffset_for_mode_button(int button_number) {
    return button_number*MODE_BUTTON_OFFSET + (button_number/4)*8 + 2;
}

int AW_window::create_mode(const char *pixmap, const char *helpText, AW_active mask, const WindowCallback& cb) {
    aw_assert(legal_mask(mask));
    Widget button;

    TuneBackground(p_w->mode_area, TUNE_BUTTON); // set background color for mode-buttons

    const char *path = AW_get_pixmapPath(pixmap);

    int y = yoffset_for_mode_button(p_w->number_of_modes);
    button = XtVaCreateManagedWidget("", xmPushButtonWidgetClass, p_w->mode_area,
                                     XmNx,               0,
                                     XmNy,               y,
                                     XmNlabelType,       XmPIXMAP,
                                     XmNshadowThickness, 1,
                                     XmNbackground,      _at->background_color,
                                     NULL);
    XtVaSetValues(button, RES_CONVERT(XmNlabelPixmap, path), NULL);
    XtVaGetValues(button, XmNforeground, &p_global->foreground, NULL);

    AW_cb *cbs = new AW_cb(this, cb, 0);
    AW_cb *cb2 = new AW_cb(this, makeWindowCallback(aw_mode_callback, p_w->number_of_modes, cbs), helpText, cbs);
    XtAddCallback(button, XmNactivateCallback,
    (XtCallbackProc) AW_server_callback,
    (XtPointer) cb2);

    if (!p_w->modes_f_callbacks) {
        p_w->modes_f_callbacks = (AW_cb **)GB_calloc(sizeof(AW_cb*), AW_NUMBER_OF_F_KEYS); // valgrinders : never freed because AW_window never is freed
    }
    if (!p_w->modes_widgets) {
        p_w->modes_widgets = (Widget *)GB_calloc(sizeof(Widget), AW_NUMBER_OF_F_KEYS);
    }
    if (p_w->number_of_modes<AW_NUMBER_OF_F_KEYS) {
        p_w->modes_f_callbacks[p_w->number_of_modes] = cb2;
        p_w->modes_widgets[p_w->number_of_modes] = button;
    }

    root->make_sensitive(button, mask);
    p_w->number_of_modes++;

    int ynext = yoffset_for_mode_button(p_w->number_of_modes);
    if (ynext> _at->max_y_size) _at->max_y_size = ynext;

    return p_w->number_of_modes;
}

AW_area_management::AW_area_management(Widget formi, Widget widget) {
    memset((char *)this, 0, sizeof(AW_area_management));
    form = formi;
    area = widget;
}

AW_device_Xm *AW_area_management::get_screen_device() {
    if (!device) device = new AW_device_Xm(common);
    return device;
}

AW_device_size *AW_area_management::get_size_device() {
    if (!size_device) size_device = new AW_device_size(common);
    return size_device;
}

AW_device_print *AW_area_management::get_print_device() {
    if (!print_device) print_device = new AW_device_print(common);
    return print_device;
}

AW_device_click *AW_area_management::get_click_device() {
    if (!click_device) click_device = new AW_device_click(common);
    return click_device;
}

void AW_window::wm_activate() {
    {
        Boolean iconic = False;
        XtVaGetValues(p_w->shell, XmNiconic, &iconic, NULL);

        if (iconic == True) {
            XtVaSetValues(p_w->shell, XmNiconic, False, NULL);

            XtMapWidget(p_w->shell);
            XRaiseWindow(XtDisplay(p_w->shell), XtWindow(p_w->shell));
        }
    }

    {
        Display *xdpy            = XtDisplay(p_w->shell);
        Window   window          = XtWindow(p_w->shell);
        Atom     netactivewindow = XInternAtom(xdpy, "_NET_ACTIVE_WINDOW", False);

        if (netactivewindow) {

            XClientMessageEvent ce;
            ce.type         = ClientMessage;
            ce.display      = xdpy;
            ce.window       = window;
            ce.message_type = netactivewindow;
            ce.format       = 32;
            ce.data.l[0]    = 2;
            ce.data.l[1]    = None;
            ce.data.l[2]    = Above;
            ce.data.l[3]    = 0;
            ce.data.l[4]    = 0;

#if defined(DEBUG)
            Status ret =
#endif // DEBUG
                XSendEvent(xdpy, XDefaultRootWindow(xdpy),
                           False,
                           SubstructureRedirectMask | SubstructureNotifyMask,
                           (XEvent *) &ce);

#if defined(DEBUG)
            if (!ret) { fprintf(stderr, "Failed to send _NET_ACTIVE_WINDOW to WM (XSendEvent returns %i)\n", ret); }
#endif // DEBUG
            XSync(xdpy, False);
        }
#if defined(DEBUG)
        else {
            fputs("No such atom '_NET_ACTIVE_WINDOW'\n", stderr);
        }
#endif // DEBUG
    }
}

void AW_window::_set_activate_callback(void *widget) {
    if (_callback && (long)_callback != 1) {
        if (!_callback->help_text && _at->helptext_for_next_button) {
            _callback->help_text = _at->helptext_for_next_button;
            _at->helptext_for_next_button = 0;
        }

        XtAddCallback((Widget) widget, XmNactivateCallback,
                (XtCallbackProc) AW_server_callback, (XtPointer) _callback);
    }
    _callback = NULL;
}


void AW_window::set_background(const char *colorname, Widget parentWidget) {
    bool colorSet = false;

    if (colorname) {
        XColor unused, color;

        if (XAllocNamedColor(p_global->display, p_global->colormap, colorname, &color, &unused)
                == 0) {
            fprintf(stderr, "XAllocColor failed: %s\n", colorname);
        }
        else {
            _at->background_color = color.pixel;
            colorSet = true;
        }
    }

    if (!colorSet) {
        XtVaGetValues(parentWidget, XmNbackground, &(_at->background_color),
                NULL); // fallback to background color
    }
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

    int col[3];
    {
        Pixel bg;
        XtVaGetValues(w, XmNbackground, &bg, NULL);

        XColor xc;
        xc.pixel = bg;
        XQueryColor(XtDisplay(w), p_global->colormap, &xc);

        col[0] = xc.red >> 8; // take MSB
        col[1] = xc.green >> 8;
        col[2] = xc.blue >> 8;
    }

    int mod = modStrength;
    int preferredDir = 0;
    bool invertedMod = false;

    if (modStrength>0) {
        if (modStrength>255) {
            mod -= 256;
            preferredDir = 1; // increase preferred
        }
    }
    else {
        if (modStrength<-255) {
            mod = -modStrength-256;
            preferredDir = -1; // decrease preferred
        }
        else {
            invertedMod = true;
            mod = -mod;
        }
    }

    aw_assert(mod >= 0 && mod < 128);
    // illegal modification

    bool incPossible[3]; // increment possible for color
    bool decPossible[3]; // decrement possible for color
    int incs = 0; // count possible increments
    int decs = 0; // count possible decrements

    for (int i = 0; i<3; ++i) {
        if ((incPossible[i] = ((col[i]+mod) <= 255)))
            incs++;
        if ((decPossible[i] = ((col[i]-mod) >= 0)))
            decs++;
    }

    aw_assert(incs||decs);

    switch (preferredDir) {
    case 0: // no direction preferred yet, need to decide
        if (invertedMod)
            preferredDir = decs ? -1 : 1;
        else
            preferredDir = incs ? 1 : -1;
        break;
    case 1:
        if (!incs)
            preferredDir = -1;
        break;
    case -1:
        if (!decs)
            preferredDir = 1;
        break;
    }

    aw_assert(preferredDir == 1 || preferredDir == -1); // no direction chosen above

    if (preferredDir == 1) {
        for (int i=0; i<3; ++i) col[i] += (incPossible[i] ? mod : 0);
    }
    else if (preferredDir == -1) {
        for (int i=0; i<3; ++i) col[i] -= (decPossible[i] ? mod : 0);
    }


    char hex_color[50];
    sprintf(hex_color, "#%2.2X%2.2X%2.2X", col[0], col[1], col[2]);
    aw_assert(strlen(hex_color) == 7);
    // otherwise some value overflowed
    set_background(hex_color, w);
}

void AW_root::make_sensitive(Widget w, AW_active mask) {
    // Don't call make_sensitive directly!
    //
    // Simply set sens_mask(AWM_EXP) and after creating the expert-mode-only widgets,
    // set it back using sens_mask(AWM_ALL)

    aw_assert(w);
    aw_assert(legal_mask(mask));

    prvt->set_last_widget(w);

    if (mask != AWM_ALL) { // no need to make widget sensitive, if its shown unconditionally
        button_sens_list = new AW_buttons_struct(mask, w, button_sens_list);
        if (!(mask & global_mask)) XtSetSensitive(w, False); // disable widget if mask doesn't match
    }
}

AW_buttons_struct::AW_buttons_struct(AW_active maski, Widget w, AW_buttons_struct *prev_button) {
    aw_assert(w);
    aw_assert(legal_mask(maski));

    mask     = maski;
    button   = w;
    next     = prev_button;
}

AW_buttons_struct::~AW_buttons_struct() {
    delete next;
}

bool AW_root::remove_button_from_sens_list(Widget button) {
    bool removed = false;
    if (button_sens_list) {
        AW_buttons_struct *prev = 0;
        AW_buttons_struct *bl   = button_sens_list;

        while (bl) {
            if (bl->button == button) break; // found wanted widget
            prev = bl;
            bl = bl->next;
        }

        if (bl) {
            // remove from list
            if (prev) prev->next  = bl->next;
            else button_sens_list = bl->next;

            bl->next = 0;
            removed  = true;

            delete bl;
        }
    }
    return removed;
}

AW_option_menu_struct::AW_option_menu_struct(int numberi, const char *variable_namei,
                                             AW_VARIABLE_TYPE variable_typei, Widget label_widgeti,
                                             Widget menu_widgeti, AW_pos xi, AW_pos yi, int correct) {
    option_menu_number = numberi;
    variable_name      = strdup(variable_namei);
    variable_type      = variable_typei;
    label_widget       = label_widgeti;
    menu_widget        = menu_widgeti;
    first_choice       = NULL;
    last_choice        = NULL;
    default_choice     = NULL;
    next               = NULL;
    x                  = xi;
    y                  = yi;

    correct_for_at_center_intern = correct;
}

AW_toggle_field_struct::AW_toggle_field_struct(int toggle_field_numberi,
        const char *variable_namei, AW_VARIABLE_TYPE variable_typei,
        Widget label_widgeti, int correct) {

    toggle_field_number = toggle_field_numberi;
    variable_name = strdup(variable_namei);
    variable_type = variable_typei;
    label_widget = label_widgeti;
    first_toggle = NULL;
    last_toggle = NULL;
    default_toggle = NULL;
    next = NULL;
    correct_for_at_center_intern = correct;
}

AW_window_Motif::AW_window_Motif()
    : shell(0),
      scroll_bar_vertical(0),
      scroll_bar_horizontal(0),
      menu_deep(0),
      help_pull_down(0),
      mode_area(0),
      number_of_modes(0),
      modes_f_callbacks(NULL),
      modes_widgets(NULL),
      selected_mode(0),
      popup_cb(NULL),
      frame(0),
      toggle_field(0),
      toggle_label(0),
      toggle_field_var_name(NULL),
      toggle_field_var_type(AW_NONE),
      keymodifier(AW_KEYMODE_NONE),
      WM_top_offset(0),
      WM_left_offset(0)
{
    for (int i = 0; i<AW_MAX_MENU_DEEP; ++i) {
        menu_bar[i] = 0;
    }
    for (int i = 0; i<AW_MAX_AREA; ++i) {
        areas[i] = NULL;
    }
}

int AW_window_Motif::WM_max_top_offset  = 0;
int AW_window_Motif::WM_max_left_offset = 0;
