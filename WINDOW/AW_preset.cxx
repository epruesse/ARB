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
#include "aw_assert.hxx"

#include "aw_root.hxx"
#include "aw_awar.hxx"
#include "aw_device.hxx"
#include "aw_advice.hxx"
#include "aw_question.hxx"
#include "aw_msg.hxx"

#include <arbdbt.h>
#include <ad_colorset.h>

using std::string;

#define AWAR_COLOR_GROUPS_PREFIX "color_groups"
#define AWAR_COLOR_GROUPS_USE    AWAR_COLOR_GROUPS_PREFIX "/use"  // int : whether to use the colors in display or not
#define GC_AWARNAME_TPL_PREFIX   "GCS/%s/MANAGE_GCS/%s"

CONSTEXPR_RETURN inline bool valid_color_group(int color_group) {
    return color_group>0 && color_group<=AW_COLOR_GROUPS;
}

static const char* gc_awarname(const char *tpl, const char *gcman_id, const string& colname) {
    aw_assert(GB_check_key(colname.c_str()) == NULL); // colname has to be a key

    static SmartCharPtr awar_name;
    awar_name = GBS_global_string_copy(tpl, gcman_id, colname.c_str());
    return &*awar_name;
}

static const char* color_awarname(const char *gcman_id, const string& colname) { return gc_awarname(GC_AWARNAME_TPL_PREFIX "/colorname", gcman_id, colname); }
static const char* font_awarname (const char *gcman_id, const string& colname) { return gc_awarname(GC_AWARNAME_TPL_PREFIX "/fontname",  gcman_id, colname); }

static const char *colorgroupname_awarname(int color_group) {
    if (!valid_color_group(color_group)) return NULL;
    return GBS_global_string(AWAR_COLOR_GROUPS_PREFIX "/name%i", color_group);
}
static const char* default_colorgroup_name(int color_group) {
    return GBS_global_string(AW_COLOR_GROUP_PREFIX "%i", color_group);
}
static const char* aa_awarname(const char *gcman_id) {
    return GBS_global_string("GCS/%s/AA_SETTING", gcman_id);
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

const int GC_BACKGROUND = -1;
const int GC_INVALID    = -2;

const int NO_INDEX = -1;

enum gc_type {
    GC_TYPE_NORMAL,
    GC_TYPE_GROUP,
};

struct gc_desc {
    // data for one GC
    // - used to populate color config windows and
    // - in change-callbacks

    int     gc;              // -1 = background; [0..n-1] for normal GCs (where n=AW_gc_manager::drag_gc_offset)
    string  colorlabel;      // label to appear next to chooser
    string  key;             // key (normally build from colorlabel)
    bool    has_font;        // show font selector
    bool    fixed_width_font; // only allow fixed width fonts
    bool    same_line;       // no line break after this
    gc_type type;

    gc_desc(int gc_, gc_type type_) :
        gc(gc_),
        has_font(true),
        fixed_width_font(false),
        same_line(false),
        type(type_)
    {}

    bool is_color_group() const { return type == GC_TYPE_GROUP; }

private:
    bool parse_char(char c) {
        switch (c) {
            case '#': fixed_width_font = true; break;
            case '+': same_line        = true; break;
            case '-': has_font         = false; break;
            default : return false;
        }
        return true;
    }

    void correct() {
        if (same_line && has_font)   same_line = false; // GC needs full line if defining both color and font
    }
public:

    const char *parse_decl(const char *decl) {
        // returns default color
        int offset = 0;
        while (decl[offset]) {
            if (!parse_char(decl[offset])) break;
            offset++;
        }
        correct();

        decl += offset;

        const char *split         = strchr(decl, '$');
        const char *default_color = NULL;
        if (split) { // defines a default color
            colorlabel    = string(decl, split-decl);
            default_color = split+1;
        }
        else {
            colorlabel    = decl;
            default_color = "black";
        }

        char *keyCopy = GBS_string_2_key(colorlabel.c_str());
        key           = keyCopy;
        free(keyCopy);

        return default_color;
    }
};

class AW_gc_manager : virtual Noncopyable {
    const char *gc_base_name;
    AW_device  *device;
    int         drag_gc_offset; // = drag_gc (as used by clients)

    int first_colorgroup_idx; // index into 'GCs' or NO_INDEX (if no color groups defined)

    std::vector<gc_desc> GCs;

    GcChangedCallback changed_cb;

#if defined(ASSERTION_USED)
    bool valid_idx(int idx) const { return idx>=0 && idx<int(GCs.size()); }
    bool valid_gc(int gc) const {
        // does not test gc is really valid, just tests whether it is completely out-of-bounds
        return gc>=GC_BACKGROUND && gc <= GCs.back().gc;
    }
#endif

    static void ignore_change_cb(GcChange) {}
public:
    static const char **color_group_defaults;
    static bool         use_color_groups;

    static bool color_groups_initialized() { return color_group_defaults != 0; }

    AW_gc_manager(const char* name, AW_device* device_, int drag_gc_offset_)
        : gc_base_name(name),
          device(device_),
          drag_gc_offset(drag_gc_offset_),
          first_colorgroup_idx(NO_INDEX),
          changed_cb(makeGcChangedCallback(ignore_change_cb))
    {}

    void init_all_fonts() const;

    const char *get_base_name() const { return gc_base_name; }
    bool has_color_groups() const { return first_colorgroup_idx != NO_INDEX; }
    int size() const { return GCs.size(); }

    const gc_desc& get_gc_desc(int idx) const {
        aw_assert(valid_idx(idx));
        return GCs[idx];
    }
    int get_drag_gc() const { return drag_gc_offset; }

    void add_gc(const char* gc_desc, int& gc, bool is_color_group);
    void update_gc_color(int idx) const;
    void update_gc_font(int idx) const;
    void update_aa_setting();

    void create_gc_buttons(AW_window *aww, bool for_colorgroups);

    void set_changed_cb(const GcChangedCallback& ccb) { changed_cb = ccb; }
    void trigger_changed_cb(GcChange whatChanged) const {
        changed_cb(whatChanged);
        device->queue_draw();
    }
};

const char **AW_gc_manager::color_group_defaults = NULL;
bool         AW_gc_manager::use_color_groups     = false;

// ---------------------------
//      GC awar callbacks

void AW_gc_manager::update_gc_font(int idx) const {
    aw_assert(valid_idx(idx));

    const gc_desc& gcd0 = GCs[idx];
    aw_assert(gcd0.gc != GC_BACKGROUND); // background has no font

    AW_awar *awar_font = AW_root::SINGLETON->awar(font_awarname(gc_base_name, gcd0.key));

    const char *font = awar_font->read_char_pntr();

    device->set_font(gcd0.gc,                  font);
    device->set_font(gcd0.gc + drag_gc_offset, font);

    // set font of all following GCs which do NOT define a font themselves
    for (int i = idx+1; i<int(GCs.size()); ++i) {
        const gc_desc& gcd = GCs[i];
        if (gcd.has_font) break; // abort if GC defines its own font

        device->set_font(gcd.gc,                font);
        device->set_font(gcd.gc+drag_gc_offset, font);
    }

    trigger_changed_cb(GC_FONT_CHANGED);
}
static void gc_fontOrSize_changed_cb(AW_root*, AW_gc_manager *mgr, int idx) {
    mgr->update_gc_font(idx);
}

void AW_gc_manager::update_gc_color(int idx) const {
    aw_assert(valid_idx(idx));

    const gc_desc&  gcd   = GCs[idx];
    const char     *color = AW_root::SINGLETON->awar(color_awarname(gc_base_name, gcd.key))->read_char_pntr();

    device->set_foreground_color(gcd.gc, color);
    if (gcd.gc != GC_BACKGROUND) {
        device->set_foreground_color(gcd.gc + drag_gc_offset, color);
    }
    trigger_changed_cb(GC_COLOR_CHANGED);
}
static void gc_color_changed_cb(AW_root*, AW_gc_manager *mgr, int idx) {
    mgr->update_gc_color(idx);
}

static void color_group_use_changed_cb(AW_root *awr, AW_gc_manager *gcmgr) {
    AW_gc_manager::use_color_groups = awr->awar(AWAR_COLOR_GROUPS_USE)->read_int();
    gcmgr->trigger_changed_cb(GC_COLOR_GROUP_USE_CHANGED);
}

void AW_gc_manager::update_aa_setting() {
    AW_antialias aa = (AW_antialias) AW_root::SINGLETON->awar(aa_awarname(gc_base_name))->read_int();
    device->get_common()->set_default_aa(aa);
    trigger_changed_cb(GC_FONT_CHANGED); // @@@ no idea whether a resize is really necessary here
}
static void aw_update_aa_setting(AW_root*, AW_gc_manager *mgr) {
    mgr->update_aa_setting();
}

// -----------------
//      add GCs

void AW_gc_manager::add_gc(const char* gc_description, int& gc, bool is_color_group) {
    if (gc_description[0] == '!') { // just reserve one or several GCs (eg. done in arb_pars)
        int amount = atoi(gc_description+1);
        aw_assert(amount>=1);

        gc += amount;
        return;
    }

    int idx = int(GCs.size()); // index position where new GC will be added
    if (is_color_group && first_colorgroup_idx == NO_INDEX) {
        first_colorgroup_idx = idx;
    }

    GCs.push_back(gc_desc(gc, is_color_group ? GC_TYPE_GROUP : GC_TYPE_NORMAL));
    gc_desc &gcd = GCs.back();

    bool is_background = gc == GC_BACKGROUND;
    {
        device->new_gc(gc);
        device->set_line_attributes(gc, 1, AW_SOLID);
        device->set_function(gc, AW_COPY);

        int gc_drag = gc + drag_gc_offset;
        device->new_gc(gc_drag);
        device->set_line_attributes(gc_drag, 1, AW_SOLID);
        device->set_function(gc_drag, AW_XOR);
        device->establish_default(gc_drag);
    }

    const char *default_color = gcd.parse_decl(gc_description);

    aw_assert(implicated(gc == 0, gcd.has_font)); // first GC always has to define a font!

    if (default_color[0] == '{') {
        // use current value of an already defined color as default for current color:
        // (e.g. used in SECEDIT)
        const char *close_brace = strchr(default_color+1, '}');
        aw_assert(close_brace); // missing '}' in reference!
        char *referenced_colorlabel = GB_strpartdup(default_color+1, close_brace-1);
        bool  found                 = false;

        for (std::vector<gc_desc>::iterator g = GCs.begin(); g != GCs.end(); ++g) {
            if (strcmp(g->colorlabel.c_str(), referenced_colorlabel) == 0) {
                default_color = AW_root::SINGLETON->awar(color_awarname(gc_base_name, g->key))->read_char_pntr();
                found         = true;
                break;
            }
        }

        aw_assert(found); // unknown default color!
        free(referenced_colorlabel);
    }

    AW_root::SINGLETON->awar_string(color_awarname(gc_base_name, gcd.key), default_color)
        ->add_callback(makeRootCallback(gc_color_changed_cb, this, idx));
    gc_color_changed_cb(NULL, this, idx);

    if (!is_background) { // no font for background
        if (gcd.has_font) {
            const char *default_font = gcd.fixed_width_font ? "monospace 10" : "sans 10";

            AW_root::SINGLETON->awar_string(font_awarname(gc_base_name, gcd.key), default_font)->add_callback(makeRootCallback(gc_fontOrSize_changed_cb, this, idx));
        }
        // Note: fonts are not initialized here. This is done in init_all_fonts() after all GCs have been defined.
    }

    gc++;
}
void AW_gc_manager::init_all_fonts() const {
    // initialize fonts of all defined GCs:
    for (int idx = 0; idx<int(GCs.size()); ++idx) {
        if (GCs[idx].has_font) {
            update_gc_font(idx);
        }
    }
}

AW_gc_manager *AW_manage_GC(AW_window                *aww,
                            const char               *gc_base_name,
                            AW_device                *device,
                            int                       base_gc,
                            int                       base_drag,
                            AW_GCM_AREA               /*area*/, // remove AFTERMERGE
                            const GcChangedCallback&  changecb,
                            bool                      define_color_groups,
                            const char               *default_background_color,
                            ...)
{
    /*!
     * creates some GC pairs:
     * - one for normal operation,
     * - the other for drag mode
     * eg.
     *     AW_manage_GC(aww, "ARB_NT", device, 0, 1, AW_GCM_DATA_AREA, my_expose_cb, false, "white","#sequence", NULL);
     *     (see implementation for more details on parameter strings)
     *     will create 2 GCs:
     *      GC 0 (normal)
     *      GC 1 (drag)
     *  Don't forget the NULL sentinel at the end of the GC definition list.
     *
     *  When the GCs are modified the 'changecb' is called
     *
     * @param aww          base window
     * @param gc_base_name (usually the window_id, prefixed to awars)
     * @param device       screen device
     * @param base_gc      first gc number @@@REFACTOR: always 0 so far...
     * @param base_drag    one after last gc
     * @param area         AW_GCM_DATA_AREA or AW_GCM_WINDOW_AREA (motif only)
     * @param changecb     cb if changed
     * @param define_color_groups  true -> add colors for color groups
     * @param ...                  NULL terminated list of \0 terminated strings:
     *                             first GC is fixed: '-background'
     *                             optionsSTRING   name of GC and AWAR
     *                             options:        #   fixed fonts only
     *                                             -   no fonts
     *                                             !   unused GC (only reserves a GC number; used eg. in arb_pars to sync GCs with arb_ntree)
     *                                             +   append next in same line
     *
     *                                             $color at end of string    => define default color value
     *                                             ${GCname} at end of string => use default of previously defined color
     */

    aw_assert(default_background_color[0]);
    aw_assert(base_gc == 0);
    // @@@ assert that aww->window_defaults_name is equal to gc_base_name?

#if defined(ASSERTION_USED)
    int base_drag_given = base_drag;
#endif

    AW_root *aw_root = AW_root::SINGLETON;

    AW_gc_manager *gcmgr = new AW_gc_manager(gc_base_name, device, base_drag);

    int  gc = GC_BACKGROUND; // gets incremented by add_gc
    char background[50];
    sprintf(background, "-background$%s", default_background_color);
    gcmgr->add_gc(background, gc, false);

    va_list parg;
    va_start(parg, default_background_color);

    const char *id;
    while ( (id = va_arg(parg, char*)) ) {
        gcmgr->add_gc(id, gc, false);
    }
    va_end(parg);

    {
        AW_awar *awar_useGroups = aw_root->awar_int(AWAR_COLOR_GROUPS_USE, 1);

        awar_useGroups->add_callback(makeRootCallback(color_group_use_changed_cb, gcmgr));
        AW_gc_manager::use_color_groups = awar_useGroups->read_int();
    }

    if (define_color_groups) {
        for (int i = 1; i <= AW_COLOR_GROUPS; ++i) {
            aw_root->awar_string(colorgroupname_awarname(i), default_colorgroup_name(i));
        }

        const char **color_group_gc_default = AW_gc_manager::color_group_defaults;
        while (*color_group_gc_default) {
            gcmgr->add_gc(*color_group_gc_default++, gc, true);
        }
    }

    gcmgr->init_all_fonts();

    // anti-aliasing settings:
    aw_root->awar_int(aa_awarname(gc_base_name), AW_AA_NONE)->add_callback(makeRootCallback(aw_update_aa_setting, gcmgr));
    aw_update_aa_setting(aw_root, gcmgr);

    // installing changed callback here avoids that it gets triggered by initializing GCs
    gcmgr->set_changed_cb(changecb);
    aw_assert(gc == base_drag_given); // parameter 'base_drag' has wrong value!

    return gcmgr;
}

void AW_copy_GC_colors(AW_root *aw_root, const char *source_gcman, const char *dest_gcman, const char *id0, ...) {
    // read the color values of the specified GCs from 'source_gcman'
    // and write the values into same-named GCs of 'dest_gcman'.
    //
    // 'id0' is the first of a list of color ids.
    // Notes:
    // - a NULL sentinel has to be passed after the last color
    // - the ids (may) differ from the descriptions passed to AW_manage_GC (ids are keys!)

    va_list parg;
    va_start(parg, id0);

    const char *id = id0;
    while (id) {
        const char *value = aw_root->awar(color_awarname(source_gcman, id))->read_char_pntr();
        aw_root->awar(color_awarname(dest_gcman, id))->write_string(value);

        id = va_arg(parg, const char*); // another argument?
    }

    va_end(parg);
}

void AW_init_color_group_defaults(const char *for_program) {
    // if for_program == NULL defaults of arb_ntree are silently used
    // if for_program is unknown a warning is shown

    aw_assert(!AW_gc_manager::color_group_defaults);

    AW_gc_manager::color_group_defaults = ARB_NTREE_color_group;
    if (for_program) {
        if (strcmp(for_program, "arb_edit4") == 0) {
            AW_gc_manager::color_group_defaults = ARB_EDIT4_color_group;
        }
#if defined(ASSERTION_USED)
        else {
            aw_assert(strcmp(for_program, "arb_ntree") == 0); // you might want to add specific defaults for_program
                                                              // alternatively pass NULL to use ARB_NTREE_color_group
        }
#endif
    }
}

long AW_find_active_color_group(GBDATA *gb_item) {
    /*! same as GBT_get_color_group() if color groups are active
     * @param gb_item the item
     * @return always 0 if color groups are inactive
     */
    aw_assert(AW_gc_manager::color_groups_initialized());
    return AW_gc_manager::use_color_groups ? GBT_get_color_group(gb_item) : 0;
}

char *AW_get_color_group_name(AW_root *awr, int color_group) {
    aw_assert(AW_gc_manager::color_groups_initialized());
    aw_assert(valid_color_group(color_group));
    return awr->awar(colorgroupname_awarname(color_group))->read_string();
}

void AW_gc_manager::create_gc_buttons(AW_window *aws, bool for_colorgroups) {
    int idx = 0;

    std::vector<gc_desc>::iterator gcd = GCs.begin();
    if (for_colorgroups) {
        advance(gcd, first_colorgroup_idx);
        idx += first_colorgroup_idx;
    }

    const int STD_LABEL_LEN = 15;

    for (; gcd != GCs.end(); ++gcd, ++idx) { // @@@ loop over idx
        if (gcd->is_color_group() != for_colorgroups) continue;

        if (for_colorgroups) {
            int color_group_no = idx-first_colorgroup_idx+1;
            char buf[5];
            sprintf(buf, "%2i:", color_group_no); // @@@ shall this short label be stored in gc_desc?
            aws->label_length(3);
            aws->label(buf);
            aws->create_input_field(colorgroupname_awarname(color_group_no), 20);
        }
        else {
            aws->label_length(STD_LABEL_LEN);
            aws->label(gcd->colorlabel.c_str());
        }

        // length of color button cannot be modified (diff to motif version)
        aws->create_color_button(color_awarname(gc_base_name, gcd->key), NULL);
        if (gcd->has_font)   {
            // length of font button cannot be modified (diff to motif version)
            aws->create_font_button(font_awarname(gc_base_name, gcd->key), NULL);
        }
        if (!gcd->same_line) aws->at_newline();
    }
}

static void AW_create_gc_color_groups_window(AW_window *, AW_root *aw_root, AW_gc_manager *gcmgr) {
    aw_assert(AW_gc_manager::color_groups_initialized());

    typedef std::map<AW_gc_manager*, AW_window_simple*> GroupWindowRegistry;

    static GroupWindowRegistry     existing_window;
    GroupWindowRegistry::iterator  found = existing_window.find(gcmgr);
    AW_window_simple              *aws   = found == existing_window.end() ? NULL : found->second;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aw_root, "COLOR_GROUP_DEF", "Define color groups");

        aws->at(10, 10);
        aws->auto_space(5, 5);

        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");
        aws->callback(makeHelpCallback("color_props_groups.hlp"));
        aws->create_button("HELP", "HELP", "H");
        aws->at_newline();

        aws->label("Enable color groups:");
        aws->create_checkbox(AWAR_COLOR_GROUPS_USE);
        aws->at_newline();

        gcmgr->create_gc_buttons(aws, true);

        aws->window_fit();
        existing_window[gcmgr] = aws;
    }

    aws->activate();
}

AW_window *AW_create_gc_window_named(AW_root *aw_root, AW_gc_manager *gcman, const char *wid, const char *windowname) {
    // same as AW_create_gc_window, but uses different window id and name
    // (use if if there are two or more color def windows in one application,
    // otherwise they save the same window properties)

    AW_window_simple *aws = new AW_window_simple;

    aws->init(aw_root, wid, windowname);

    aws->at(10, 10);
    aws->auto_space(5, 5);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");
    aws->callback(makeHelpCallback("color_props.hlp"));
    aws->create_button("HELP", "HELP", "H");
    aws->at_newline();

    aws->label_length(23);
    aws->label("Anti-Aliasing");
    aws->create_option_menu(aa_awarname(gcman->get_base_name()), true);
    aws->insert_option("System Default", "", AW_AA_DEFAULT);
    aws->insert_option("Disabled",       "", AW_AA_NONE);
    aws->insert_option("Greyscale",      "", AW_AA_GRAY);
    aws->insert_option("Subpixel",       "", AW_AA_SUBPIXEL);
    aws->insert_option("Fast",           "", AW_AA_FAST);
    aws->insert_option("Good",           "", AW_AA_GOOD);
    aws->insert_option("Best",           "", AW_AA_BEST);
    aws->update_option_menu();
    aws->at_newline();

    gcman->create_gc_buttons(aws, false);

    if (gcman->has_color_groups()) {
        aws->callback(makeWindowCallback(AW_create_gc_color_groups_window, aw_root, gcman));
        aws->create_autosize_button("EDIT_COLOR_GROUP", "Edit color groups", "E");
        aws->at_newline();
    }

    aws->window_fit();
    return aws;
}
AW_window *AW_create_gc_window(AW_root *aw_root, AW_gc_manager *gcman) {
    return AW_create_gc_window_named(aw_root, gcman, "COLOR_DEF", "Colors and Fonts");
}

int AW_get_drag_gc(AW_gc_manager *gcman) {
    return gcman->get_drag_gc();
}

#if defined(UNIT_TESTS)
void fake_AW_init_color_groups() {
    if (!AW_gc_manager::color_groups_initialized()) {
        AW_init_color_group_defaults(NULL);
    }
    AW_gc_manager::use_color_groups = true;
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
