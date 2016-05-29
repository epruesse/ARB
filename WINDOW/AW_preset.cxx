// ================================================================ //
//                                                                  //
//   File      : AW_preset.cxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef IN_ARB_WINDOW
#error MODULE_... is not known
#endif

#include <aw_color_groups.hxx>
#include "aw_preset.hxx"
#include "aw_root.hxx"
#include "aw_awar.hxx"
#include "aw_device.hxx"
#include "aw_advice.hxx"
#include "aw_question.hxx"
#include "aw_msg.hxx"

#include "aw_def.hxx"
#include "aw_nawar.hxx"

#include <arbdbt.h>
#include <ad_colorset.h>

#include <vector>
#include <map>
#include <string>

using std::string;

#define AWAR_COLOR_GROUPS_PREFIX  "color_groups"
#define AWAR_COLOR_GROUPS_USE     AWAR_COLOR_GROUPS_PREFIX "/use" // int : whether to use the colors in display or not
#define GC_AWARNAME_TPL_PREFIX    "GCS/%s/MANAGE_GCS/%s"

// prototypes for motif-only section at bottom of this file:
static void aw_create_color_chooser_window(AW_window *aww, const char *awar_name, const char *color_description);
static void aw_create_font_chooser_window(AW_window *aww, AW_gc_manager *gcman, int gc_idx);

CONSTEXPR_RETURN inline bool valid_color_group(int color_group) {
    return color_group>0 && color_group<=AW_COLOR_GROUPS;
}

static const char* gc_awarname(const char *tpl, const char *gcman_id, const string& colname) {
    aw_assert(GB_check_key(colname.c_str()) == NULL); // colname has to be a key

    static SmartCharPtr awar_name;
    awar_name = GBS_global_string_copy(tpl, gcman_id, colname.c_str());
    return &*awar_name;
}

static const char* color_awarname   (const char* gcman_id, const string& colname) { return gc_awarname(GC_AWARNAME_TPL_PREFIX "/colorname", gcman_id, colname); }
static const char* fontname_awarname(const char* gcman_id, const string& colname) { return gc_awarname(GC_AWARNAME_TPL_PREFIX "/font",      gcman_id, colname); }
static const char* fontsize_awarname(const char* gcman_id, const string& colname) { return gc_awarname(GC_AWARNAME_TPL_PREFIX "/size",      gcman_id, colname); }
static const char* fontinfo_awarname(const char* gcman_id, const string& colname) { return gc_awarname(GC_AWARNAME_TPL_PREFIX "/info",      gcman_id, colname); }

static const char *colorgroupname_awarname(int color_group) {
    if (!valid_color_group(color_group)) return NULL;
    return GBS_global_string(AWAR_COLOR_GROUPS_PREFIX "/name%i", color_group);
}
static const char* default_colorgroup_name(int color_group) {
    return GBS_global_string(AW_COLOR_GROUP_PREFIX "%i", color_group);
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

    AW_window *aww;             // motif only (colors get allocated in window)
    int        colorindex_base; // motif-only (colorindex of background-color)

    std::vector<gc_desc> GCs;

    GcChangedCallback *changed_cb; // @@@ use a null-cb

#if defined(ASSERTION_USED)
    bool valid_idx(int idx) const { return idx>=0 && idx<int(GCs.size()); }
    bool valid_gc(int gc) const {
        // does not test gc is really valid, just tests whether it is completely out-of-bounds
        return gc>=GC_BACKGROUND && gc <= GCs.back().gc;
    }
#endif

    AW_color_idx colorindex(int gc) const {
        aw_assert(valid_gc(gc));
        return AW_color_idx(colorindex_base+gc+1);
    }
public:
    static const char **color_group_defaults;
    static bool         use_color_groups;

    static bool color_groups_initialized() { return color_group_defaults != 0; }

    AW_gc_manager(const char*  name, AW_device* device_, int drag_gc_offset_,
                  AW_window   *aww_, int colorindex_base_)
        : gc_base_name(name),
          device(device_),
          drag_gc_offset(drag_gc_offset_),
          first_colorgroup_idx(NO_INDEX),
          aww(aww_),
          colorindex_base(colorindex_base_),
          changed_cb(NULL)
    {}

    ~AW_gc_manager() {
        delete changed_cb;
    }

    void init_all_fonts() const;

#if 0 // @@@ unused atm
    const char *get_color_awarname(int idx) { // @@@ use where applicable // @@@ add similar functions for font
        aw_assert(valid_idx(idx));
        return color_awarname(gc_base_name, GCs[idx].key);
    }
#endif

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

    void create_gc_buttons(AW_window *aww, bool for_colorgroups);

    void set_changed_cb(const GcChangedCallback& ccb) {
        aw_assert(!changed_cb);
        changed_cb = new GcChangedCallback(ccb);
    }
    void trigger_changed_cb(GcChange whatChanged) const {
        if (changed_cb) (*changed_cb)(whatChanged);
    }
};

const char **AW_gc_manager::color_group_defaults = NULL;
bool         AW_gc_manager::use_color_groups     = false;

// ---------------------------
//      GC awar callbacks

void AW_gc_manager::update_gc_font(int idx) const {
    aw_assert(valid_idx(idx));

    static bool avoid_recursion = false;
    if (avoid_recursion) return;
    LocallyModify<bool> flag(avoid_recursion, true);

    const gc_desc& gcd0 = GCs[idx];
    aw_assert(gcd0.gc != GC_BACKGROUND); // background has no font

    AW_awar *awar_fontname = AW_root::SINGLETON->awar(fontname_awarname(gc_base_name, gcd0.key));
    AW_awar *awar_fontsize = AW_root::SINGLETON->awar(fontsize_awarname(gc_base_name, gcd0.key));
    AW_awar *awar_fontinfo = AW_root::SINGLETON->awar(fontinfo_awarname(gc_base_name, gcd0.key));

    int fname = awar_fontname->read_int();
    int fsize = awar_fontsize->read_int();

    int found_font_size;
    device->set_font(gcd0.gc,                fname, fsize, &found_font_size);
    device->set_font(gcd0.gc+drag_gc_offset, fname, fsize, 0);

    bool autocorrect_fontsize = (found_font_size != fsize) && (found_font_size != -1);
    if (autocorrect_fontsize) {
        awar_fontsize->write_int(found_font_size);
        fsize = found_font_size;
    }

    // set font of all following GCs which do NOT define a font themselves
    for (int i = idx+1; i<int(GCs.size()); ++i) {
        const gc_desc& gcd = GCs[i];
        if (gcd.has_font) break; // abort if GC defines its own font

        device->set_font(gcd.gc,                fname, fsize, 0);
        device->set_font(gcd.gc+drag_gc_offset, fname, fsize, 0);
    }

    awar_fontinfo->write_string(GBS_global_string("font %i | %i", fname, fsize)); // @@@ use meaningful font-abbrev

    trigger_changed_cb(GC_FONT_CHANGED);
}
static void gc_fontOrSize_changed_cb(AW_root*, AW_gc_manager *mgr, int idx) {
    mgr->update_gc_font(idx);
}

void AW_gc_manager::update_gc_color(int idx) const {
    aw_assert(valid_idx(idx));

    const gc_desc&  gcd   = GCs[idx];
    const char     *color = AW_root::SINGLETON->awar(color_awarname(gc_base_name, gcd.key))->read_char_pntr();

    AW_color_idx colorIdx = colorindex(gcd.gc);
    aww->alloc_named_data_color(colorIdx, color);

    if (gcd.gc == GC_BACKGROUND && colorIdx == AW_DATA_BG) {
        // if background color changes, all drag-gc colors need to be updated
        // (did not understand why, just refactored existing code --ralf)

        for (int i = 1; i<size(); ++i) {
            int g    = GCs[i].gc;
            colorIdx = colorindex(g);
            device->set_foreground_color(g + drag_gc_offset, colorIdx);
        }
    }
    else {
        int gc = gcd.gc;

        if (gc == GC_BACKGROUND) gc = 0; // special case: background color of bottom-area (only used by arb_phylo)

        device->set_foreground_color(gc,                  colorIdx);
        device->set_foreground_color(gc + drag_gc_offset, colorIdx);
    }
    trigger_changed_cb(GC_COLOR_CHANGED);
}
static void gc_color_changed_cb(AW_root*, AW_gc_manager *mgr, int idx) {
    mgr->update_gc_color(idx);
}

static void AW_color_group_name_changed_cb(AW_root *) { // @@@ use again; merge to gtk
    // @@@ only warn once!
    AW_advice("To activate the new names for color groups you have to\n"
              "save properties and restart the program.",
              AW_ADVICE_TOGGLE, "Color group name has been changed", 0);
}

static void color_group_use_changed_cb(AW_root *awr, AW_gc_manager *gcmgr) {
    AW_gc_manager::use_color_groups = awr->awar(AWAR_COLOR_GROUPS_USE)->read_int();
    gcmgr->trigger_changed_cb(GC_COLOR_GROUP_USE_CHANGED);
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
    bool alloc_gc      = !is_background || colorindex_base != AW_DATA_BG;
    if (alloc_gc)
    {
        if (is_background) ++gc; // only happens for AW_BOTTOM_AREA defining GCs

        device->new_gc(gc);
        device->set_line_attributes(gc, 1, AW_SOLID);
        device->set_function(gc, AW_COPY);

        int gc_drag = gc + drag_gc_offset;
        device->new_gc(gc_drag);
        device->set_line_attributes(gc_drag, 1, AW_SOLID);
        device->set_function(gc_drag, AW_XOR);
        device->establish_default(gc_drag);

        if (is_background) --gc; // only happens for AW_BOTTOM_AREA defining GCs
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
            AW_font default_font = gcd.fixed_width_font ? AW_DEFAULT_FIXED_FONT : AW_DEFAULT_NORMAL_FONT;

            AW_root::SINGLETON->awar_int(fontname_awarname(gc_base_name, gcd.key), default_font)->add_callback(makeRootCallback(gc_fontOrSize_changed_cb, this, idx));
            AW_root::SINGLETON->awar_int(fontsize_awarname(gc_base_name, gcd.key), DEF_FONTSIZE)->add_callback(makeRootCallback(gc_fontOrSize_changed_cb, this, idx));
            AW_root::SINGLETON->awar_string(fontinfo_awarname(gc_base_name, gcd.key), "<select font>");
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
                            AW_GCM_AREA               area,
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

    int            colidx_base = area == AW_GCM_DATA_AREA ? AW_DATA_BG : AW_WINDOW_BG;
    AW_gc_manager *gcmgr       = new AW_gc_manager(gc_base_name, device, base_drag, aww, colidx_base);

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

static void create_color_button(AW_window *aws, const char *awar_name, const char *color_description) { // @@@ pass AW_gc_manager + gc
    aws->callback(makeWindowCallback(aw_create_color_chooser_window, strdup(awar_name), strdup(color_description))); // @@@ forward AW_gc_manager + gc

    char *color     = aws->get_root()->awar(awar_name)->read_string();
    char *button_id = GBS_global_string_copy("sel_color[%s]", awar_name); // @@@ change like create_font_button

    aws->create_button(button_id, " ", 0, color);

    free(button_id);
    free(color);
}

static void create_font_button(AW_window *aws, AW_gc_manager *gcman, int gc_idx) {
    const gc_desc& gcd = gcman->get_gc_desc(gc_idx);

    aws->callback(makeWindowCallback(aw_create_font_chooser_window, gcman, gc_idx));

    char *button_id = GBS_global_string_copy("sel_font_%s", gcd.key.c_str());

    aws->create_button(button_id, fontinfo_awarname(gcman->get_base_name(), gcd.key), 0);

    free(button_id);
}

void AW_gc_manager::create_gc_buttons(AW_window *aws, bool for_colorgroups) {
    int idx = 0;

    std::vector<gc_desc>::iterator gcd = GCs.begin();
    if (for_colorgroups) {
        advance(gcd, first_colorgroup_idx);
        idx += first_colorgroup_idx;
    }

    const int STD_LABEL_LEN    = 15;
    const int COLOR_BUTTON_LEN = 10;
    const int FONT_BUTTON_LEN  = COLOR_BUTTON_LEN+STD_LABEL_LEN+1;
    // => color+font has ~same length as 2 colors (does not work for color groups and does not work at all in gtk)

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

        aws->button_length(COLOR_BUTTON_LEN);
        create_color_button(aws, color_awarname(gc_base_name, gcd->key), gcd->colorlabel.c_str());
        if (gcd->has_font)   {
            aws->button_length(FONT_BUTTON_LEN);
            create_font_button(aws, this, idx);
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

        aws->label_length(20);
        aws->label("Enable color groups:");
        aws->create_toggle(AWAR_COLOR_GROUPS_USE);
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

// ------------------------------
//      motif color selector

#define AWAR_SELECTOR_COLOR_LABEL "tmp/aw/color_label"

#define AWAR_CV_R "tmp/aw/color_r" // rgb..
#define AWAR_CV_G "tmp/aw/color_g"
#define AWAR_CV_B "tmp/aw/color_b"
#define AWAR_CV_H "tmp/aw/color_h" // hsv..
#define AWAR_CV_S "tmp/aw/color_s"
#define AWAR_CV_V "tmp/aw/color_v"

static void rgb2hsv(int r, int b, int g, int& h, int& s, int& v) {
    float R = r/256.0;
    float G = g/256.0;
    float B = b/256.0;

    float min = std::min(std::min(R, B), G);
    float max = std::max(std::max(R, B), G);

    if (min == max) {
        h = 0;
    }
    else {
        float H = 60.0;

        if      (max == R) { H *= 0 + (G-B)/(max-min); }
        else if (max == G) { H *= 2 + (B-R)/(max-min); }
        else               { H *= 4 + (R-G)/(max-min); }

        if (H<0) H += 360;
        h = int(H/360.0*256);
    }

    s = max ? (max-min)/max*256 : 0;
    v = max*256;
}

static void hsv2rgb(int h, int s, int v, int& r, int& b, int& g) {
    float H = h/256.0*360;
    float S = s/256.0;
    float V = v/256.0;

    int   hi = int(H/60);
    float f  = H/60-hi;

    float p = 256*V*(1-S);
    float q = 256*V*(1-S*f);
    float t = 256*V*(1-S*(1-f));

    switch (hi) {
        case 0:
        case 6: r  = v; g = t; b = p; break;
        case 1: r  = q; g = v; b = p; break;
        case 2: r  = p; g = v; b = t; break;
        case 3: r  = p; g = q; b = v; break;
        case 4: r  = t; g = p; b = v; break;
        case 5: r  = v; g = p; b = q; break;
        default: r = g = b = 0; aw_assert(0); break;
    }
}

static char *aw_glob_font_awar_name         = 0; // @@@ weird name?
static bool  ignore_color_value_change      = false;
static bool  color_value_change_was_ignored = false;

static void aw_set_rgb_sliders(AW_root *awr, int r, int g, int b) {
    color_value_change_was_ignored = false;
    {
        LocallyModify<bool> delayUpdate(ignore_color_value_change, true);
        awr->awar(AWAR_CV_R)->write_int(r);
        awr->awar(AWAR_CV_G)->write_int(g);
        awr->awar(AWAR_CV_B)->write_int(b);
    }
    if (color_value_change_was_ignored) awr->awar(AWAR_CV_B)->touch();
}

static int hex2dec(char c) {
    if (c>='0' && c<='9') return c-'0';
    if (c>='A' && c<='F') return c-'A'+10;
    if (c>='a' && c<='f') return c-'a'+10;
    return -1;
}
static void aw_set_sliders_from_color(AW_root *awr) {
    const char *color = awr->awar(aw_glob_font_awar_name)->read_char_pntr();

    int r = 0;
    int g = 0;
    int b = 0;

    if (color[0] == '#') {
        int len = strlen(color+1);
        if (len == 6) {
            r = hex2dec(color[1])*16+hex2dec(color[2]);
            g = hex2dec(color[3])*16+hex2dec(color[4]);
            b = hex2dec(color[5])*16+hex2dec(color[6]);
        }
        else if (len == 3) {
            r = hex2dec(color[1]); r = r*16+r;
            g = hex2dec(color[2]); g = g*16+g;
            b = hex2dec(color[3]); b = b*16+b;
        }
    }
    aw_set_rgb_sliders(awr, r, g, b);
}

inline void aw_set_color(AW_root *awr, const char *color_name) {
    awr->awar(aw_glob_font_awar_name)->write_string(color_name);
    aw_set_sliders_from_color(awr);
}
static void aw_set_color(AW_window *aww, const char *color_name) {
    aw_set_color(aww->get_root(), color_name);
}

static void colorslider_changed_cb(AW_root *awr, bool hsv_changed) {
    if (ignore_color_value_change) {
        color_value_change_was_ignored = true;
    }
    else {
        LocallyModify<bool> noRecursion(ignore_color_value_change, true);

        if (hsv_changed) {
            int h = awr->awar(AWAR_CV_H)->read_int();
            int s = awr->awar(AWAR_CV_S)->read_int();
            int v = awr->awar(AWAR_CV_V)->read_int();

            int r, g, b;
            hsv2rgb(h, s, v, r, g, b);

            {
                char color_name[10];
                sprintf(color_name, "#%2.2X%2.2X%2.2X", r, g, b);
                aw_set_color(awr, color_name);
            }

            awr->awar(AWAR_CV_R)->write_int(r);
            awr->awar(AWAR_CV_G)->write_int(g);
            awr->awar(AWAR_CV_B)->write_int(b);
        }
        else {
            int r = awr->awar(AWAR_CV_R)->read_int();
            int g = awr->awar(AWAR_CV_G)->read_int();
            int b = awr->awar(AWAR_CV_B)->read_int();

            {
                char color_name[10];
                sprintf(color_name, "#%2.2X%2.2X%2.2X", r, g, b);
                aw_set_color(awr, color_name);
            }

            int h, s, v;
            rgb2hsv(r, g, b, h, s, v);

            awr->awar(AWAR_CV_H)->write_int(h);
            awr->awar(AWAR_CV_S)->write_int(s);
            awr->awar(AWAR_CV_V)->write_int(v);
        }
    }
}
static void aw_create_colorslider_awars(AW_root *awr) {
    awr->awar_string(AWAR_SELECTOR_COLOR_LABEL);
    static const char *colorValueAwars[] = {
        AWAR_CV_R, AWAR_CV_H,
        AWAR_CV_G, AWAR_CV_S,
        AWAR_CV_B, AWAR_CV_V,
    };

    for (int cv = 0; cv<6; ++cv) {
        awr->awar_int(colorValueAwars[cv])
            ->set_minmax(0, 255)
            ->add_callback(makeRootCallback(colorslider_changed_cb, bool(cv%2)));
    }
}
static void aw_create_color_chooser_window(AW_window *aww, const char *awar_name, const char *color_description) { // @@@ pass AW_gc_manager + gc
    AW_root *awr = aww->get_root();
    static AW_window_simple *aws = 0;
    if (!aws) {
        aw_create_colorslider_awars(awr);

        aws = new AW_window_simple;
        aws->init(awr, "COLORS", "Select color");

        int x1 = 10;
        int y1 = 10;

        aws->at(x1, y1);
        aws->auto_space(3, 3);
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->button_length(20);
        aws->create_button(NULL, AWAR_SELECTOR_COLOR_LABEL, 0);
        aws->at_newline();

        int x2, y2;
        aws->get_at_position(&x2, &y2);
        y2 += 3;

        struct ColorValue {
            const char *label;
            const char *awar;
        } colorValue[] = {
            { "R", AWAR_CV_R }, { "H", AWAR_CV_H },
            { "G", AWAR_CV_G }, { "S", AWAR_CV_S },
            { "B", AWAR_CV_B }, { "V", AWAR_CV_V },
        };

        for (int row = 0; row<3; ++row) {
            aws->at(x1, y1+(row+1)*(y2-y1));
            const ColorValue *vc = &colorValue[row*2];
            aws->label(vc->label);
            aws->create_input_field_with_scaler(vc->awar, 4, 256, AW_SCALER_LINEAR);
            ++vc;
            aws->label(vc->label);
            aws->create_input_field_with_scaler(vc->awar, 4, 256, AW_SCALER_LINEAR);
        }

        aws->button_length(2);
        aws->at_newline();

        for (int red = 0; red <= 255; red += 255/4) {
            for (int green = 0; green <= 255; green += 255/4) {
                for (int blue = 0; blue <= 255; blue += 255/4) {
                    if (red != green || green != blue) {
                        char color_name[10];
                        sprintf(color_name, "#%2.2X%2.2X%2.2X", red, green, blue);
                        aws->callback(makeWindowCallback(aw_set_color, strdup(color_name)));
                        aws->create_button(color_name, "=", 0, color_name);
                    }
                }
            }
            aws->at_newline();
        }
        const float STEP = 255.0/23;
        for (float greyF = 0.0; greyF < 256; greyF += STEP) { // grey buttons (including black+white)
            char color_name[10];
            int  grey = int(greyF+0.5);
            sprintf(color_name, "#%2.2X%2.2X%2.2X", grey, grey, grey);
            aws->callback(makeWindowCallback(aw_set_color, strdup(color_name)));
            aws->create_button(color_name, "=", 0, color_name);
        }
        aws->at_newline();

        aws->window_fit();
    }
    awr->awar(AWAR_SELECTOR_COLOR_LABEL)->write_string(color_description);
    freedup(aw_glob_font_awar_name, awar_name);
    aw_set_sliders_from_color(awr);
    aws->activate();
}

// ----------------------------
//      motif font chooser

#define AWAR_SELECTOR_FONT_LABEL "tmp/aw/font_label"
#define AWAR_SELECTOR_FONT_NAME  "tmp/aw/font_name"
#define AWAR_SELECTOR_FONT_SIZE  "tmp/aw/font_size"

#define NO_FONT -1
#define NO_SIZE -2

static void aw_create_font_chooser_awars(AW_root *awr) {
    awr->awar_string(AWAR_SELECTOR_FONT_LABEL, "<invalid>");
    awr->awar_int(AWAR_SELECTOR_FONT_NAME, NO_FONT);
    awr->awar_int(AWAR_SELECTOR_FONT_SIZE, NO_SIZE)->set_minmax(MIN_FONTSIZE, MAX_FONTSIZE);
}

static void aw_create_font_chooser_window(AW_window *aww, AW_gc_manager *gcman, int gc_idx) {
    AW_root        *awr = aww->get_root();
    const gc_desc&  gcd = gcman->get_gc_desc(gc_idx);

    static AW_window_simple *aw_fontChoose[2] = { NULL, NULL }; // one for fixed-width font; one general

    bool fixed_width_only = gcd.fixed_width_font;

    AW_window_simple*& aws = aw_fontChoose[fixed_width_only];
    AW_window_simple*& awo = aw_fontChoose[!fixed_width_only];

    if (!aws) {
        aw_create_font_chooser_awars(awr);

        aws = new AW_window_simple;
        aws->init(awr, "FONT", fixed_width_only ? "Select fixed width font" : "Select font");

        aws->auto_space(10, 10);

        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->button_length(20);
        aws->create_button(NULL, AWAR_SELECTOR_FONT_LABEL, 0);
        aws->at_newline();

        aws->label("Font:");
        aws->create_option_menu(AWAR_SELECTOR_FONT_NAME, true);
        {
            int fonts_inserted = 0;
            for (int font_nr = 0; ; font_nr++) {
                AW_font     aw_font_nr  = font_nr;
                const char *font_string = AW_font_2_ascii(aw_font_nr);
                if (!font_string) {
                    fprintf(stderr, "[Font detection: tried=%i, found=%i]\n", font_nr, fonts_inserted);
                    break;
                }

                if (fixed_width_only && AW_font_2_xfig(aw_font_nr) >= 0) continue;
                aws->insert_option(font_string, 0, font_nr);
                ++fonts_inserted;
            }
            if (!fonts_inserted) aws->insert_option("No suitable fonts detected", 0, 0);
            aws->insert_default_option("<no font selected>", 0, NO_FONT);
            aws->update_option_menu();
        }

        aws->at_newline();

        aws->label("Size:");
        aws->create_input_field_with_scaler(AWAR_SELECTOR_FONT_SIZE, 3, 330);
        aws->at_newline();

        aws->window_fit();
    }

    awr->awar(AWAR_SELECTOR_FONT_LABEL)->write_string(gcd.colorlabel.c_str());
    awr->awar(AWAR_SELECTOR_FONT_NAME)->map(awr->awar(fontname_awarname(gcman->get_base_name(), gcd.key)));
    awr->awar(AWAR_SELECTOR_FONT_SIZE)->map(awr->awar(fontsize_awarname(gcman->get_base_name(), gcd.key)));

    if (awo) awo->hide(); // both windows use same awars -> hide the other window to avoid chaos
    aws->activate();
}

// -----------------------------------
//      frame colors (motif only)

static void aw_message_reload(AW_root *) {
    aw_message("Sorry, to activate new colors:\n"
                "   save properties\n"
                "   and restart application");
}

static void AW_preset_create_font_chooser(AW_window *aws, const char *awar, const char *label, bool message_reload) {
    if (message_reload) aws->get_root()->awar(awar)->add_callback(aw_message_reload);

    aws->label(label);
    aws->create_option_menu(awar, true);

    aws->insert_option("5x8",               "5", "5x8");
    aws->insert_option("6x10",              "6", "6x10");
    aws->insert_option("7x13",              "7", "7x13");
    aws->insert_option("7x13bold",          "7", "7x13bold");
    aws->insert_option("8x13",              "8", "8x13");
    aws->insert_option("8x13bold",          "8", "8x13bold");
    aws->insert_option("9x15",              "9", "9x15");
    aws->insert_option("9x15bold",          "9", "9x15bold");
    aws->insert_option("helvetica-12",      "9", "helvetica-12");
    aws->insert_option("helvetica-bold-12", "9", "helvetica-bold-12");
    aws->insert_option("helvetica-13",      "9", "helvetica-13");
    aws->insert_option("helvetica-bold-13", "9", "helvetica-bold-13");

    aws->insert_default_option("other", "o", "");
    aws->update_option_menu();
}
static void AW_preset_create_color_button(AW_window *aws, const char *awar_name, const char *label) {
    aws->get_root()->awar(awar_name)->add_callback(aw_message_reload);
    aws->label(label);
    create_color_button(aws, awar_name, label);
}

AW_window *AW_preset_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    const int   tabstop = 400;
    aws->init(root, "PROPS_FRAME", "WINDOW_PROPERTIES");

    aws->label_length(25);
    aws->button_length(20);

    aws->at           (10, 10);
    aws->auto_space(10, 10);

    aws->callback     (AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("props_frame.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at_newline();

    AW_preset_create_font_chooser(aws, "window/font", "Main Menu Font", 1);
    aws->at_x(tabstop);
    aws->create_input_field("window/font", 12);

    aws->at_newline();

    aws->button_length(10);
    AW_preset_create_color_button(aws, "window/background", "Application Background");
    aws->at_x(tabstop);
    aws->create_input_field("window/background", 12);

    aws->at_newline();

    AW_preset_create_color_button(aws, "window/foreground", "Application Foreground");
    aws->at_x(tabstop);
    aws->create_input_field("window/foreground", 12);

    aws->at_newline();

    AW_preset_create_color_button(aws, "window/color_1", "Color 1");
    aws->at_x(tabstop);
    aws->create_input_field("window/color_1", 12);

    aws->at_newline();

    AW_preset_create_color_button(aws, "window/color_2", "Color 2");
    aws->at_x(tabstop);
    aws->create_input_field("window/color_2", 12);

    aws->at_newline();

    AW_preset_create_color_button(aws, "window/color_3", "Color 3");

    aws->at_x(tabstop);
    aws->create_input_field("window/color_3", 12);

    aws->at_newline();

    aws->window_fit();
    return (AW_window *)aws;
}


