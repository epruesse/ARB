// ================================================================ //
//                                                                  //
//   File      : AW_preset.cxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <aw_color_groups.hxx>
#include "aw_preset.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_root.hxx"
#include "aw_awar.hxx"
#include "aw_device.hxx"
#include "aw_advice.hxx"
#include "aw_question.hxx"
#include "aw_msg.hxx"
#include "aw_assert.hxx"
#include <arbdbt.h>
#include <ad_colorset.h>

CONSTEXPR_RETURN inline bool valid_color_group(int color_group) {
    return color_group>0 && color_group<=AW_COLOR_GROUPS;
}

static const char* _gc_awar_name(const char* tpl, const char* window, const char* colname) {
    char*       key = GBS_string_2_key(colname);
    const char* res = GBS_global_string(tpl, window, key);
    free(key);
    return res;
}    
static const char* _color_awarname(const char* window, const char* colname) {
    return _gc_awar_name("GCS/%s/MANAGE_GCS/%s/colorname", window, colname);
}
static const char* _font_awarname(const char* window, const char* colname) {
    return _gc_awar_name("GCS/%s/MANAGE_GCS/%s/fontname", window, colname);
}
static const char* _colorgroupname_awarname(int color_group) {
    if (!valid_color_group(color_group)) return NULL;
    return GBS_global_string(AWAR_COLOR_GROUPS_PREFIX "/name%i", color_group);
}
static const char* _colorgroup_name(int color_group) {
    return GBS_global_string(AW_COLOR_GROUP_PREFIX "%i", color_group);
}
static const char* _aa_awar_name(const char* window) {
    return GBS_global_string("GCS/%s/AA_SETTING", window);
}

/**
 * Default color group colors for ARB_NTREE (also general default)
 */
static const char *ARB_NTREE_color_group[AW_COLOR_GROUPS+1] = {
    "+-" AW_COLOR_GROUP_PREFIX  "1$#D50000", "-" AW_COLOR_GROUP_PREFIX  "2$#00ffff",
    "+-" AW_COLOR_GROUP_PREFIX  "3$#00FF77", "-" AW_COLOR_GROUP_PREFIX  "4$#c700c7",
    "+-" AW_COLOR_GROUP_PREFIX  "5$#0000ff", "-" AW_COLOR_GROUP_PREFIX  "6$#FFCE5B",

    "+-" AW_COLOR_GROUP_PREFIX  "7$#AB2323", "-" AW_COLOR_GROUP_PREFIX  "8$#008888",
    "+-" AW_COLOR_GROUP_PREFIX  "9$#008800", "-" AW_COLOR_GROUP_PREFIX "10$#880088",
    "+-" AW_COLOR_GROUP_PREFIX "11$#000088", "-" AW_COLOR_GROUP_PREFIX "12$#888800",

    0
};

/**
 * Default color group colors for ARB_EDIT4
 */
static const char *ARB_EDIT4_color_group[AW_COLOR_GROUPS+1] = {
    "+-" AW_COLOR_GROUP_PREFIX  "1$#FFAFAF", "-" AW_COLOR_GROUP_PREFIX  "2$#A1FFFF",
    "+-" AW_COLOR_GROUP_PREFIX  "3$#AAFFAA", "-" AW_COLOR_GROUP_PREFIX  "4$#c700c7",
    "+-" AW_COLOR_GROUP_PREFIX  "5$#C5C5FF", "-" AW_COLOR_GROUP_PREFIX  "6$#FFE370",

    "+-" AW_COLOR_GROUP_PREFIX  "7$#F87070", "-" AW_COLOR_GROUP_PREFIX  "8$#DAFFFF",
    "+-" AW_COLOR_GROUP_PREFIX  "9$#8DE28D", "-" AW_COLOR_GROUP_PREFIX "10$#880088",
    "+-" AW_COLOR_GROUP_PREFIX "11$#000088", "-" AW_COLOR_GROUP_PREFIX "12$#F1F169",

    0
};

class _AW_gc_manager : virtual Noncopyable {
private:
    const char         *gc_base_name;
    AW_device          *device;
    int                 drag_gc_offset;

    struct gc_desc {
        char    *label;       // label to appear next to chooser
        AW_awar *awar_color;  // awar to hold color description
        AW_awar *awar_font;   // awar to hold font description
        
        bool    hidden;       // do not show in color config
        bool    has_font;     // show font selector
        bool    has_color;    // show color selector
        bool    fixed_width_font; // only allow fixed width fonts
        bool    same_line;    // no line break after this
        bool    is_color_group; 
        gc_desc() :
            label(NULL), 
            awar_color(NULL),
            awar_font(NULL),
            hidden(false), 
            has_font(true), has_color(true),
            fixed_width_font(false),
            same_line(false), is_color_group(false)
        {}
    };
    
    std::vector<gc_desc>  GCs;

public:
    static const char **color_group_defaults;
    
    _AW_gc_manager(const char* name, AW_device* device, 
                   int drag_gc_offset);
    void add_gc(const char* gc_desc, bool is_color_group);
    void update_gc_color(int gc);
    void update_gc_font(int gc);
    void update_aa_setting();

    void create_gc_buttons(AW_window *aww);
    AW_signal changed;

    
};

const char **_AW_gc_manager::color_group_defaults = ARB_NTREE_color_group;


static void aw_update_gc_color(AW_root*, _AW_gc_manager* mgr, int gc) {
    mgr->update_gc_color(gc);
}
static void aw_update_gc_font(AW_root*, _AW_gc_manager* mgr, int gc) {
    mgr->update_gc_font(gc);
}

static void aw_update_aa_setting(AW_root*, _AW_gc_manager* mgr) {
    mgr->update_aa_setting();
}

_AW_gc_manager::_AW_gc_manager(const char* name, AW_device* device_, 
                               int drag_gc_offset_) 
    : gc_base_name(name),
      device(device_),
      drag_gc_offset(drag_gc_offset_)
{
    AW_root::SINGLETON->awar_int(_aa_awar_name(name), AW_AA_NONE)
        ->add_callback(makeRootCallback(aw_update_aa_setting, this));
}


void _AW_gc_manager::add_gc(const char* gc_description, bool is_color_group=false) {
    int gc      = GCs.size() - 1; // -1 is background
    int gc_drag = GCs.size() + drag_gc_offset - 1;

    GCs.push_back(gc_desc());
    gc_desc &gcd = GCs.back();
    gcd.is_color_group = is_color_group;

    device->new_gc(gc);
    device->set_line_attributes(gc, 1, AW_SOLID);
    device->set_function(gc, AW_COPY);
    
    device->new_gc(gc_drag);
    device->set_line_attributes(gc_drag, 1, AW_SOLID);
    device->set_function(gc_drag, AW_XOR);
    device->establish_default(gc_drag);

    bool done_parsing = false;
    while (*gc_description && !done_parsing) {
        switch (*gc_description++) {
        case '#': 
            gcd.fixed_width_font = true;
            break;
        case '+': 
            gcd.same_line = true; 
            break;
        case '=': 
            gcd.has_color = false; 
            break;
        case '-': 
            if (*(gc_description+1) == '-') {
                gcd.hidden = true;
            }
            else {
                gcd.has_font = false;
            }
            break;
        default:
            gc_description--;
            done_parsing = true;
        }
    }

    // hide GC if it defines neither font nor color 
    if (!gcd.has_font && !gcd.has_color) {
        gcd.hidden = true;
    }

    // GC needs full line if definint both color and font
    if (gcd.has_font && gcd.same_line) {
        gcd.same_line = false;
    }

    // first GC always has a font
    if (gc == 0) {
        gcd.has_font = true;
    }


    char *default_color;
    const char *split = strchr(gc_description, '$');
    if (split) { // have color spec
        gcd.label = strndup(gc_description, split-gc_description);
        default_color = strdup(split+1);
    }
    else {
        gcd.label = strdup(gc_description);
        default_color = strdup("black");
    }

    if (default_color[0] == '{') {
        default_color[strlen(default_color)-1]='\0';
        for (std::vector<gc_desc>::iterator it = GCs.begin(); it != GCs.end(); ++it) {
            if (strcmp(it->label, default_color) == 0) {
                free(default_color);
                default_color = it->awar_color->read_string();
                break;
            }
        }
    }
    gcd.awar_color = AW_root::SINGLETON->awar_string(_color_awarname(gc_base_name, gcd.label), default_color);
    gcd.awar_color->add_callback(makeRootCallback(aw_update_gc_color, this, gc));
    aw_update_gc_color(NULL, this, gc);

    if (gc != -1) { // no font for background
        if (gcd.has_font) {
            const char *default_font = gcd.fixed_width_font ? "monospace 10" : "sans 10";
            gcd.awar_font = AW_root::SINGLETON->awar_string(_font_awarname(gc_base_name, gcd.label), default_font);
        }
        else {
            // font is same as previous font -> copy awar reference
            gcd.awar_font = GCs[GCs.size() - 2].awar_font;
        }
        gcd.awar_font->add_callback(makeRootCallback(aw_update_gc_font, this, gc));
        aw_update_gc_font(NULL, this, gc);
    }
    
    free(default_color);
}

void _AW_gc_manager::update_gc_color(int gc) {
    aw_assert(gc >= -1 && gc < (int)GCs.size() - 1);
    const char* color = GCs[gc+1].awar_color->read_char_pntr();
    device->set_foreground_color(gc, color);
    if (gc != -1) 
        device->set_foreground_color(drag_gc_offset + gc, color);
    changed.emit();
    device->queue_draw();
}

void _AW_gc_manager::update_gc_font(int gc) {
    aw_assert(gc >= -1 && gc < (int)GCs.size() - 1);
    device->set_font(gc, GCs[gc+1].awar_font->read_char_pntr());
    if (gc != -1) 
        device->set_font(drag_gc_offset + gc, GCs[gc+1].awar_font->read_char_pntr());
    changed.emit();
    device->queue_draw();
}

void _AW_gc_manager::update_aa_setting() {
    AW_antialias aa = (AW_antialias) AW_root::SINGLETON->awar(_aa_awar_name(gc_base_name))->read_int();
    device->get_common()->set_default_aa(aa);
    changed.emit();
    device->queue_draw();
}

// ----------------------------------------------------------------------
// force-diff-sync 931274892174 (remove after merging back to trunk)
// ----------------------------------------------------------------------

void AW_copy_GCs(AW_root *aw_root, const char *source_window, const char *dest_window, bool has_font_info, const char *id0, ...) {
    // read the values of the specified GCs from 'source_window'
    // and write the values into same-named GCs of 'dest_window'
    //
    // 'id0' is the first of a list of color ids
    // a NULL pointer has to be given behind the last color!

    va_list parg;
    va_start(parg, id0);

    const char *id = id0;
    while (id) {
        char *value = aw_root->awar(_color_awarname(source_window, id))->read_string();
        aw_root->awar(_color_awarname(dest_window, id))->write_string(value);
        free(value);

        if (has_font_info) {
            int ivalue = aw_root->awar(_font_awarname(source_window, id))->read_int();
            aw_root->awar(_font_awarname(dest_window, id))->write_int(ivalue);
        }

        id = va_arg(parg, const char*); // another argument ?
    }

    va_end(parg);
}

// ----------------------------------------------------------------------
// force-diff-sync 265873246583745 (remove after merging back to trunk)
// ----------------------------------------------------------------------

AW_gc_manager AW_manage_GC(AW_window             *aww,
                           const char            *gc_base_name,
                           AW_device             *device,
                           int                    base_gc,
                           int                    base_drag,
                           AW_GCM_AREA            /*area*/, // remove AFTERMERGE
                           const WindowCallback&  changecb,
                           bool                   define_color_groups,
                           const char            *default_background_color,
                           ...)
{
    /*!
     * creates some GC pairs: one for normal operation,
     * the other for drag mode
     * eg.
     *     AW_manage_GC(aww,"ARB_NT",device,10,20,AW_GCM_DATA_AREA, my_expose_cb, cd1 ,cd2, "name","#sequence",NULL);
     *     (see implementation for more details on parameter strings)
     *     will create 4 GCs:
     *      GC 10 (normal) and 20 (drag)
     *      GC 11 (normal and monospaced (indicated by '#')
     *      GC 21 drag and monospaced
     *  Don't forget the 0 at the end of the fontname field
     *  When the GCs are modified the 'changecb' is called
     *
     * @param aww          base window
     * @param gc_base_name (usually the window_id, prefixed to awars)
     * @param device       screen device
     * @param base_gc      first gc number @@@REFACTOR: always 0 so far...
     * @param base_drag    one after last gc
     * @param area         AW_GCM_DATA_AREA or AW_GCM_WINDOW_AREA (motif only)
     * @param changecb     cb if changed
     * @param cd1,cd2      free Parameters to changecb
     * @param define_color_groups  true -> add colors for color groups
     * @param ...                  NULL terminated list of \0 terminated strings:
     *                             first GC is fixed: '-background'
     *                             optionsSTRING   name of GC and AWAR
     *                             options:        #   fixed fonts only
     *                                             -   no fonts
     *                                             --  completely hide GC
     *                                             =   no color selector
     *                                             +   append next in same line
     *
     *                                             $color at end of string    => define default color value
     *                                             ${GCname} at end of string => use default of previously defined color
     */

    aw_assert(default_background_color[0]);
    aw_assert(base_gc == 0);
    // @@@ assert that aww->window_defaults_name is equal to gc_base_name?

    AW_root *aw_root = AW_root::SINGLETON;

    AW_gc_manager gcmgr = new _AW_gc_manager(gc_base_name, device, base_drag);

    if (aww) aww->main_drag_gc = base_drag;

    char background[50];
    sprintf(background, "-background$%s", default_background_color);
    gcmgr->add_gc(background);

    va_list parg;
    va_start(parg, default_background_color);
    const char *id;
    while ( (id = va_arg(parg, char*)) ) {
        gcmgr->add_gc(id);
    }
    va_end(parg);
    
    aw_root->awar_int(AWAR_COLOR_GROUPS_USE, 1);
    if (define_color_groups) {
        for (int i = 1; i <= AW_COLOR_GROUPS; ++i) {
            aw_root->awar_string(_colorgroupname_awarname(i), _colorgroup_name(i));
        }

        const char **color_group_gc_default = _AW_gc_manager::color_group_defaults;
        while (*color_group_gc_default) {
            gcmgr->add_gc(*color_group_gc_default++, true);
        }
    }

    gcmgr->changed.connect(changecb, aww);

    return gcmgr;
}

void AW_init_color_group_defaults(const char *for_program) {
    if (!for_program) return;
    if (!strcmp(for_program, "arb_edit4")) {
        _AW_gc_manager::color_group_defaults = ARB_EDIT4_color_group;
    }
}

#if defined(UNIT_TESTS)
static bool always_ignore_usage_flag = false;
#endif

long AW_find_active_color_group(GBDATA *gb_item) {
    /*! same as GBT_get_color_group() if color groups are active
     * @param gb_item the item
     * @return always 0 if color groups are inactive
     */
    bool ignore_usage_flag = false;
#if defined(UNIT_TESTS)
    if (always_ignore_usage_flag) ignore_usage_flag = true;
#endif

    if (!ignore_usage_flag) {
        int use_color_groups = AW_root::SINGLETON->awar(AWAR_COLOR_GROUPS_USE)->read_int();
        if (!use_color_groups) return 0;
    }
    return GBT_get_color_group(gb_item);
}

char *AW_get_color_group_name(AW_root *awr, int color_group) {
    aw_assert(valid_color_group(color_group));
    return awr->awar(_colorgroupname_awarname(color_group))->read_string();
}

void _AW_gc_manager::create_gc_buttons(AW_window *aww) {
    int color_group_no = 0;
    aww->label_length(23);

    aww->label("Anti-Aliasing");
    aww->create_option_menu(_aa_awar_name(gc_base_name), true);
    aww->insert_option("System Default", "", AW_AA_DEFAULT);
    aww->insert_option("Disabled", "", AW_AA_NONE);
    aww->insert_option("Greyscale", "", AW_AA_GRAY);
    aww->insert_option("Subpixel", "", AW_AA_SUBPIXEL);
    aww->insert_option("Fast", "", AW_AA_FAST);
    aww->insert_option("Good", "", AW_AA_GOOD);
    aww->insert_option("Best", "", AW_AA_BEST);
    aww->update_option_menu();
    aww->at_newline();

    for (std::vector<gc_desc>::iterator it = GCs.begin(); it != GCs.end(); ++it) {
        if (it->hidden) continue;
        
        if (it->is_color_group) {
            if (color_group_no == 0) {
                aww->label("Enable Color Groups:");
                aww->create_checkbox(AWAR_COLOR_GROUPS_USE);
                aww->label_length(3);
                aww->at_newline();
            }
            color_group_no++;
            char buf[5];
            aw_assert(color_group_no < 1000);
            sprintf(buf, "%i:", color_group_no);
            aww->label(buf);
            aww->create_input_field(_colorgroupname_awarname(color_group_no), 20);

        } else {
            aww->label(it->label);
        }

        if (it->has_color) {
            aww->create_color_button(it->awar_color->get_name(), NULL);
        }
        if (it->has_font) {
            aww->create_font_button(it->awar_font->get_name(), NULL);
        }
        if (!it->same_line) {
            aww->at_newline();
        }
    }
}


AW_window *AW_create_gc_window(AW_root *aw_root, AW_gc_manager id_par) {
    return AW_create_gc_window_named(aw_root, id_par, "PROPS_GC", "Colors and Fonts");
}

AW_window *AW_create_gc_window_named(AW_root *aw_root, AW_gc_manager id_par, const char *wid, const char *windowname) {
    // same as AW_create_gc_window, but uses different window id and name
    // (use if if there are two or more color def windows in one application,
    // otherwise they save the same window properties)

    AW_window_simple * aws = new AW_window_simple;

    aws->init(aw_root, wid, windowname);

    aws->at(10, 10);
    aws->auto_space(5, 5);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("color_props.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at_newline();

    id_par->create_gc_buttons(aws);

    aws->window_fit();
    return aws;
}

#if defined(UNIT_TESTS)
void fake_AW_init_color_groups() {
    always_ignore_usage_flag = true;
}
#endif

// @@@ move somewhere more sensible: (after!merge)

static void add_common_property_menu_entries(AW_window *aw) {
    aw->insert_menu_topic("enable_advices",   "Reactivate advices",   "R", "advice.hlp",    AWM_ALL, AW_reactivate_all_advices);
    aw->insert_menu_topic("enable_questions", "Reactivate questions", "q", "questions.hlp", AWM_ALL, AW_reactivate_all_questions);
#if defined(ARB_MOTIF)
    aw->insert_menu_topic("reset_win_layout", "Reset window layout",  "w", "reset_win_layout.hlp", AWM_ALL, AW_forget_all_window_geometry);
#endif
}
void AW_insert_common_property_menu_entries(AW_window_menu_modes *awmm) { add_common_property_menu_entries(awmm); }
void AW_insert_common_property_menu_entries(AW_window_simple_menu *awsm) { add_common_property_menu_entries(awsm); }

void AW_save_specific_properties(AW_window *aw, const char *filename) {  // special version for EDIT4
    GB_ERROR error = aw->get_root()->save_properties(filename);
    if (error) aw_message(error);
}
void AW_save_properties(AW_window *aw) {
    AW_save_specific_properties(aw, NULL);
}
