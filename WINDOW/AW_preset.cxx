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

#if defined(DEBUG)
#include <ctime>
#endif


using std::string;

#define AWAR_COLOR_GROUPS_PREFIX "color_groups"
#define AWAR_COLOR_GROUPS_USE    AWAR_COLOR_GROUPS_PREFIX "/use"  // int : whether to use the colors in display or not
#define GC_AWARNAME_TPL_PREFIX   "GCS/%s/MANAGE_GCS/%s"

#define ALL_FONTS_ID "all_fonts"

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
    GC_TYPE_RANGE,
};

#define RANGE_INDEX_BITS 4
#define RANGE_INDEX_MASK ((1<<RANGE_INDEX_BITS)-1)

inline int build_range_gc_number(int range_idx, int color_idx) {
    aw_assert(range_idx == (range_idx & RANGE_INDEX_MASK));
    return (color_idx << RANGE_INDEX_BITS) | range_idx;
}

struct gc_desc {
    // data for one GC
    // - used to populate color config windows and
    // - in change-callbacks

    int     gc;              // -1 = background;
                             // if type!=GC_TYPE_RANGE: [0..n-1] (where n=AW_gc_manager::drag_gc_offset if no gc_ranges defined)
                             // if type==GC_TYPE_RANGE: contains range- and color-index
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

    bool is_color_group()   const { return type == GC_TYPE_GROUP; }
    bool belongs_to_range() const { return type == GC_TYPE_RANGE; }

    int get_range_index() const { aw_assert(belongs_to_range()); return gc  & RANGE_INDEX_MASK; }
    int get_color_index() const { aw_assert(belongs_to_range()); return gc >> RANGE_INDEX_BITS; }

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

// --------------------------------
//      types for color-ranges

enum gc_range_type {
    GC_RANGE_INVALID,
    GC_RANGE_LINEAR,
};

class gc_range {
    gc_range_type type;

    int index;       // in range-array of AW_gc_manager
    int color_count; // number of support colors (ie. customizable colors)
    int gc_index;    // of first support color in GCs-array of AW_gc_manager

public:
    gc_range(gc_range_type type_, int index_, int gc_index_) :
        type(type_),
        index(index_),
        color_count(0),
        gc_index(gc_index_)
    {}

    void add_color(const string& colordef, AW_gc_manager *gcman);
    void update_colors(const AW_gc_manager *gcman, int changed_color) const;
};

// --------------------
//      GC manager

class AW_gc_manager : virtual Noncopyable {
    const char *gc_base_name;
    AW_device  *device;
    int         drag_gc_offset; // = drag_gc (as used by clients)

    int first_colorgroup_idx; // index into 'GCs' or NO_INDEX (if no color groups defined)

    typedef std::vector<gc_desc>  gc_container;
    typedef std::vector<gc_range> gc_range_container;

    gc_container       GCs;
    gc_range_container color_ranges;

    GcChangedCallback changed_cb;

    mutable bool          suppress_changed_cb;  // if true -> collect cb-trigger in 'did_suppress_change'
    mutable GcChange      did_suppress_change;  // "max" change suppressed so far (will be triggered by delay_changed_callbacks())
    static const GcChange GC_NOTHING_CHANGED = GcChange(0);

#if defined(ASSERTION_USED)
    bool valid_idx(int idx) const { return idx>=0 && idx<int(GCs.size()); }
    bool valid_gc(int gc) const {
        // does not test gc is really valid, just tests whether it is completely out-of-bounds
        return gc>=GC_BACKGROUND && gc < drag_gc_offset;
    }
#endif

    static void ignore_change_cb(GcChange) {}

    void allocate_gc(int gc) const;

    void update_gc_color_internal(int gc, const char *color) const;
    void update_range_colors(const gc_desc& gcd) const;
    void update_range_font(const char *font) const;

public:
    static const char **color_group_defaults;
    static bool         use_color_groups;

    static bool color_groups_initialized() { return color_group_defaults != 0; }

    AW_gc_manager(const char* name, AW_device* device_, int drag_gc_offset_)
        : gc_base_name(name),
          device(device_),
          drag_gc_offset(drag_gc_offset_),
          first_colorgroup_idx(NO_INDEX),
          changed_cb(makeGcChangedCallback(ignore_change_cb)),
          suppress_changed_cb(false),
          did_suppress_change(GC_NOTHING_CHANGED)
    {}

    void init_all_fonts() const;
    void update_all_fonts() const;

    void init_color_ranges(int& gc) const;
    bool has_color_range() const { return !color_ranges.empty(); }
    int first_range_gc() const { return drag_gc_offset - AW_RANGE_COLORS; }

    bool has_color_groups() const { return first_colorgroup_idx != NO_INDEX; }

    int size() const { return GCs.size(); }
    const gc_desc& get_gc_desc(int idx) const {
        aw_assert(valid_idx(idx));
        return GCs[idx];
    }
    const char *get_base_name() const { return gc_base_name; }
    int get_drag_gc() const { return drag_gc_offset; }

    void add_gc      (const char *gc_description, int& gc, gc_type type);
    void add_gc_range(const char *gc_description);
    void reserve_gcs (const char *gc_description, int& gc);
    void add_color_groups(int& gc);

    void update_gc_color(int idx) const;
    void update_gc_font(int idx) const;
    void update_aa_setting();

    void update_range_gc_color(int idx, const char *color) const {
        update_gc_color_internal(first_range_gc()+idx, color);
    }

    void create_gc_buttons(AW_window *aww, bool for_colorgroups);

    void set_changed_cb(const GcChangedCallback& ccb) { changed_cb = ccb; }
    void trigger_changed_cb(GcChange whatChanged) const {
        if (suppress_changed_cb) {
            did_suppress_change = GcChange(std::max(whatChanged, did_suppress_change));
        }
        else {
#if defined(DEBUG)
            fprintf(stderr, "[changed_cb] @ %zu\n", clock());
#endif
            changed_cb(whatChanged);
            device->queue_draw();
        }
    }

    void delay_changed_callbacks(bool suppress) const {
        aw_assert(suppress != suppress_changed_cb);

        suppress_changed_cb = suppress;
        if (suppress) {
            did_suppress_change = GC_NOTHING_CHANGED;
        }
        else if (did_suppress_change>GC_NOTHING_CHANGED) {
            trigger_changed_cb(did_suppress_change);
        }
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
        if (gcd.belongs_to_range()) {
            update_range_font(font);
            break; // all leftover GCs belong to ranges = > stop here
        }

        device->set_font(gcd.gc,                font);
        device->set_font(gcd.gc+drag_gc_offset, font);
    }

    trigger_changed_cb(GC_FONT_CHANGED);
}
static void gc_fontOrSize_changed_cb(AW_root*, AW_gc_manager *mgr, int idx) {
    mgr->update_gc_font(idx);
}
void AW_gc_manager::update_all_fonts() const {
    AW_root *awr = AW_root::SINGLETON;

    const char *font = awr->awar(font_awarname(gc_base_name, ALL_FONTS_ID))->read_char_pntr();

    delay_changed_callbacks(true); // temp. disable callbacks
    for (gc_container::const_iterator g = GCs.begin(); g != GCs.end(); ++g) {
        if (g->has_font) {
            // @@@ bug: will update fixed width fonts with variable fonts (fixed-width GCs are broken in gtk anyway)
            awr->awar(font_awarname(gc_base_name, g->key))->write_string(font);
        }
    }
    delay_changed_callbacks(false);
}
static void all_fontsOrSizes_changed_cb(AW_root*, const AW_gc_manager *mgr) {
    mgr->update_all_fonts();
}

void AW_gc_manager::update_gc_color_internal(int gc, const char *color) const {
    aw_assert(valid_gc(gc));

    device->set_foreground_color(gc, color);
    if (gc != GC_BACKGROUND) {
        device->set_foreground_color(gc + drag_gc_offset, color);
    }
}

void gc_range::update_colors(const AW_gc_manager *gcman, int changed_color) const {
    /*! recalculate colors of a range (called after one color changed)
     * @param gcman             the GC manager
     * @param changed_color     which color of a range has changed (0 = first, ...). -1 -> unknown => need complete update // @@@ not implemented, always acts like -1 is passed
     */
    aw_assert(type == GC_RANGE_LINEAR);
    aw_assert(color_count == 2);

    const gc_desc& low_gc  = gcman->get_gc_desc(gc_index);
    const gc_desc& high_gc = gcman->get_gc_desc(gc_index+1);

    const char *basename   = gcman->get_base_name();
    AW_root    *awr        = AW_root::SINGLETON;
    const char *low_color  = awr->awar(color_awarname(basename, low_gc.key))->read_char_pntr();
    const char *high_color = awr->awar(color_awarname(basename, high_gc.key))->read_char_pntr();

    AW_rgb_normalized low(low_color);
    AW_rgb_normalized high(high_color);

    float rdiff = high.r()-low.r();
    float gdiff = high.g()-low.g();
    float bdiff = high.b()-low.b();

    // @@@ try HSV color blending as alternative
    for (int i = 0; i<AW_RANGE_COLORS; ++i) { // blend colors
        float factor = i/float(AW_RANGE_COLORS-1);

        AW_rgb_normalized col(low.r()+factor*rdiff,
                              low.g()+factor*gdiff,
                              low.b()+factor*bdiff);

        gcman->update_range_gc_color(i, AW_rgb16(col).ascii());
    }
}
void AW_gc_manager::update_range_colors(const gc_desc& gcd) const {
    int defined_ranges = color_ranges.size();
    int range_idx      = gcd.get_range_index();

    if (range_idx<defined_ranges) {
        const gc_range& gcr = color_ranges[range_idx];
        gcr.update_colors(this, gcd.get_color_index());
    }
}
void AW_gc_manager::update_range_font(const char *font) const {
    // set font for all GCs belonging to color-range
    int first_gc = first_range_gc();
    for (int i = 0; i<AW_RANGE_COLORS; ++i) {
        int gc = first_gc+i;
        device->set_font(gc,                font, 0);
        device->set_font(gc+drag_gc_offset, font, 0);
    }
}

void AW_gc_manager::update_gc_color(int idx) const {
    aw_assert(valid_idx(idx));

    const gc_desc&  gcd   = GCs[idx];
    const char     *color = AW_root::SINGLETON->awar(color_awarname(gc_base_name, gcd.key))->read_char_pntr();

    if (gcd.belongs_to_range()) {
        update_range_colors(gcd); // @@@ should not happen during startup and only if affected range is the active range
    }
    else {
        update_gc_color_internal(gcd.gc, color);
    }

    trigger_changed_cb(GC_COLOR_CHANGED);
}
static void gc_color_changed_cb(AW_root*, AW_gc_manager *mgr, int idx) {
    mgr->update_gc_color(idx);
}

static void color_group_name_changed_cb(AW_root *) {
    static bool warned = false;
    if (!warned) {
        AW_advice("Color group names are used at various places of the interface.\n"
                  "To activate the changed names everywhere, you have to\n"
                  "save properties and restart the program.",
                  AW_ADVICE_TOGGLE, "Color group name has been changed", 0);
        warned = true;
    }
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

// ----------------------------
//      define color-ranges

void gc_range::add_color(const string& colordef, AW_gc_manager *gcman) {
    int gc = build_range_gc_number(index, color_count);
    gcman->add_gc(colordef.c_str(), gc, GC_TYPE_RANGE);
    color_count++;
}

void AW_gc_manager::add_gc_range(const char *gc_description) {
    GB_ERROR    error = NULL;
    const char *comma = strchr(gc_description+1, ',');
    if (!comma) error = "',' expected";
    else {
        string      range_name(gc_description+1, comma-gc_description-1);
        const char *colon = strchr(comma+1, ':');
        if (!colon) error = "':' expected";
        else {
            gc_range_type rtype = GC_RANGE_INVALID;
            string        range_type(comma+1, colon-comma-1);

            if (range_type == "linear") {
                rtype = GC_RANGE_LINEAR;
            }

            if (rtype == GC_RANGE_INVALID) {
                error = GBS_global_string("invalid range-type '%s'", range_type.c_str());
            }

            if (!error) {
                gc_range range(rtype, color_ranges.size(), GCs.size());

                const char *color_start = colon+1;
                comma                   = strchr(color_start, ',');
                while (comma) {
                    range.add_color(string(color_start, comma-color_start), this);
                    color_start = comma+1;
                    comma       = strchr(color_start, ',');
                }
                range.add_color(string(color_start), this);

                color_ranges.push_back(range); // add to manager
            }
        }
    }

    if (error) {
        GBK_terminatef("Failed to parse color range specification '%s'\n(Reason: %s)", gc_description, error);
    }
}

void AW_gc_manager::init_color_ranges(int& gc) const {
    if (has_color_range()) {
        aw_assert(gc == first_range_gc()); // 'drag_gc_offset' is wrong (is passed as 'base_drag' to AW_manage_GC)
        int last_gc = gc + AW_RANGE_COLORS-1;
        while (gc<=last_gc) allocate_gc(gc++);

        const gc_range& active_range = color_ranges[0]; // @@@ should use active color-range (atm there is only 1)
        active_range.update_colors(this, -1); // -1 means full update
    }
}


// -------------------------
//      reserve/add GCs

void AW_gc_manager::reserve_gcs(const char *gc_description, int& gc) {
    aw_assert(gc_description[0] == '!');

    // just reserve one or several GCs (eg. done in arb_pars)
    int amount = atoi(gc_description+1);
    aw_assert(amount>=1);

    gc += amount;
}

void AW_gc_manager::allocate_gc(int gc) const {
    device->new_gc(gc);
    device->set_line_attributes(gc, 1, AW_SOLID);
    device->set_function(gc, AW_COPY);

    int gc_drag = gc + drag_gc_offset;
    device->new_gc(gc_drag);
    device->set_line_attributes(gc_drag, 1, AW_SOLID);
    device->set_function(gc_drag, AW_XOR);
    device->establish_default(gc_drag);
}

void AW_gc_manager::add_gc(const char *gc_description, int& gc, gc_type type) {
    aw_assert(strchr("*!&", gc_description[0]) == NULL);
    aw_assert(implicated(type != GC_TYPE_RANGE, !has_color_range())); // color ranges have to be specified AFTER all other color definition strings (in call to AW_manage_GC)

    GCs.push_back(gc_desc(gc, type));
    gc_desc &gcd = GCs.back();
    int      idx = int(GCs.size()-1); // index position where new GC has been added

    if (gcd.is_color_group() && first_colorgroup_idx == NO_INDEX) {
        first_colorgroup_idx = idx; // remember index of first color-group
    }

    bool is_background = gc == GC_BACKGROUND;
    bool alloc_gc      = !gcd.belongs_to_range();
    if (alloc_gc)
    {
        allocate_gc(gc);
    }

    const char *default_color = gcd.parse_decl(gc_description);

    aw_assert(strcmp(gcd.key.c_str(), ALL_FONTS_ID) != 0); // id is reserved
    aw_assert(implicated(gc == 0 && type != GC_TYPE_RANGE, gcd.has_font)); // first GC always has to define a font!

    if (default_color[0] == '{') {
        // use current value of an already defined color as default for current color:
        // (e.g. used in SECEDIT)
        const char *close_brace = strchr(default_color+1, '}');
        aw_assert(close_brace); // missing '}' in reference!
        char *referenced_colorlabel = GB_strpartdup(default_color+1, close_brace-1);
        bool  found                 = false;

        for (gc_container::iterator g = GCs.begin(); g != GCs.end(); ++g) {
            if (strcmp(g->colorlabel.c_str(), referenced_colorlabel) == 0) {
                default_color = AW_root::SINGLETON->awar(color_awarname(gc_base_name, g->key))->read_char_pntr(); // @@@ should use default value (not current value)
                found         = true;
                break;
            }
        }

        aw_assert(found); // unknown default color!
        free(referenced_colorlabel);
    }

    // @@@ add assertion vs duplicate ids here?
    AW_root::SINGLETON->awar_string(color_awarname(gc_base_name, gcd.key), default_color)
        ->add_callback(makeRootCallback(gc_color_changed_cb, this, idx));
    gc_color_changed_cb(NULL, this, idx);

    if (!is_background) { // no font for background
        if (gcd.has_font) {
            aw_assert(!gcd.belongs_to_range()); // no fonts supported for ranges

            const char *default_font = gcd.fixed_width_font ? "monospace 10" : "sans 10";

            RootCallback fontChange_cb = makeRootCallback(gc_fontOrSize_changed_cb, this, idx);
            AW_root::SINGLETON->awar_string(font_awarname(gc_base_name, gcd.key), default_font)->add_callback(fontChange_cb);
        }
        // Note: fonts are not initialized here. This is done in init_all_fonts() after all GCs have been defined.
    }

    gc++;
}
void AW_gc_manager::init_all_fonts() const {
    char    *ad_font = NULL;
    AW_root *awr     = AW_root::SINGLETON;

    // initialize fonts of all defined GCs:
    for (int idx = 0; idx<int(GCs.size()); ++idx) {
        const gc_desc& gcd = GCs[idx];
        if (gcd.has_font) {
            update_gc_font(idx);

            if (ad_font == NULL) {
                ad_font = awr->awar(font_awarname(gc_base_name, gcd.key))->read_string();
            }
        }
    }

    // init global font awar (used to set ALL fonts)
    AW_root::SINGLETON->awar_string(font_awarname(gc_base_name, ALL_FONTS_ID), ad_font)->add_callback(makeRootCallback(all_fontsOrSizes_changed_cb, this));
    free(ad_font);
}

void AW_gc_manager::add_color_groups(int& gc) {
    AW_root *awr = AW_root::SINGLETON;
    for (int i = 1; i <= AW_COLOR_GROUPS; ++i) {
        awr->awar_string(colorgroupname_awarname(i), default_colorgroup_name(i))->add_callback(color_group_name_changed_cb);
    }

    const char **color_group_gc_default = AW_gc_manager::color_group_defaults;
    while (*color_group_gc_default) {
        add_gc(*color_group_gc_default++, gc, GC_TYPE_GROUP);
    }
}

AW_gc_manager *AW_manage_GC(AW_window                */*aww*/, // remove AFTERMERGE
                            const char               *gc_base_name,
                            AW_device                *device,
                            int                       base_gc,
                            int                       base_drag,
                            AW_GCM_AREA               /*area*/, // remove AFTERMERGE
                            const GcChangedCallback&  changecb,
                            const char               *default_background_color,
                            ...)
{
    /*!
     * creates some GC pairs:
     * - one for normal operation,
     * - the other for drag mode
     * eg.
     *     AW_manage_GC(aww, "ARB_NT", device, 0, 1, AW_GCM_DATA_AREA, my_expose_cb, "white","#sequence", NULL);
     *     (see implementation for more details on parameter strings)
     *     will create 2 GCs:
     *      GC 0 (normal)
     *      GC 1 (drag)
     *  Don't forget the NULL sentinel at the end of the GC definition list.
     *
     *  When the GCs are modified the 'changecb' is called
     *
     * @param aww          base window (motif only)
     * @param gc_base_name (usually the window_id, prefixed to awars)
     * @param device       screen device
     * @param base_gc      first gc number @@@REFACTOR: always 0 so far...
     * @param base_drag    one after last gc
     * @param area         AW_GCM_DATA_AREA or AW_GCM_WINDOW_AREA (motif only)
     * @param changecb     cb if changed
     * @param define_color_groups  true -> add colors for color groups
     * @param ...                  NULL terminated list of \0 terminated <definition>s:
     *
     *   <definition>::= <gcdef>|<reserve>|<rangedef>|<groupdef>
     *   <reserve>   ::= !<num>                                     "reserves <num> gc-numbers; used eg. in arb_pars to sync GCs with arb_ntree"
     *   <gcdef>     ::= [<option>+]<descript>['$'<default>]        "defines a GC"
     *   <option>    ::= '#'|'+'|'-'                                "'-'=no font; '#'=only fixed fonts; '+'=append next GC on same line"
     *   <descript>  ::=                                            "description of GC; shown as label; has to be unique when converted to key"
     *   <default>   ::= <xcolor>|<gcref>                           "default color of GC"
     *   <xcolor>    ::=                                            "accepts any valid X-colorname (e.g. 'white', '#c0ff40')"
     *   <gcref>     ::= '{'<descript>'}'                           "reference to another earlier defined gc"
     *   <rangedef>  ::= '*'<name>','<type>':'<gcdef>[','<gcdef>]+  "defines a GC-range (with one <gcdef> for each support color)"
     *   <name>      ::=                                            "description of range"
     *   <type>      ::= 'linear'|'cyclic'|'planar'|'spatial'       "rangetype; implies number of required support colors: linear=2 cyclic=3 planar=3 spatial=4" // @@@ only linear type with 2 support colors is accepted/implemented yet
     *   <groupdef>  ::= '&color_groups'                            "insert color-groups here"
     */

    aw_assert(default_background_color[0]);
    aw_assert(base_gc == 0);

#if defined(ASSERTION_USED)
    int base_drag_given = base_drag;
#endif

    AW_root *aw_root = AW_root::SINGLETON;

    AW_gc_manager *gcmgr = new AW_gc_manager(gc_base_name, device, base_drag);

    int  gc = GC_BACKGROUND; // gets incremented by add_gc
    char background[50];
    sprintf(background, "-background$%s", default_background_color);
    gcmgr->add_gc(background, gc, GC_TYPE_NORMAL);

    va_list parg;
    va_start(parg, default_background_color);

    bool defined_color_groups = false;

    const char *id;
    while ( (id = va_arg(parg, char*)) ) {
        switch (id[0]) {
            case '!': gcmgr->reserve_gcs(id, gc); break;
            case '*': gcmgr->add_gc_range(id); break;
            case '&':
                if (strcmp(id, "&color_groups") == 0) { // trigger use of color groups
                    if (!defined_color_groups) {
                        gcmgr->add_color_groups(gc);
                        defined_color_groups = true;
                    }
                    else aw_assert(0); // color-groups trigger specified twice!
                }
                else aw_assert(0); // unknown trigger
                break;
            default: gcmgr->add_gc(id, gc, GC_TYPE_NORMAL); break;
        }
    }
    va_end(parg);

    {
        AW_awar *awar_useGroups = aw_root->awar_int(AWAR_COLOR_GROUPS_USE, 1);

        awar_useGroups->add_callback(makeRootCallback(color_group_use_changed_cb, gcmgr));
        AW_gc_manager::use_color_groups = awar_useGroups->read_int();
    }

    gcmgr->init_color_ranges(gc);
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

bool AW_color_groups_active() {
    /*! returns true if color groups are active */
    aw_assert(AW_gc_manager::color_groups_initialized());
    return AW_gc_manager::use_color_groups;
}
const char *AW_get_color_groups_active_awarname() {
    aw_assert(AW_gc_manager::color_groups_initialized());
    return AWAR_COLOR_GROUPS_USE;
}

char *AW_get_color_group_name(AW_root *awr, int color_group) {
    aw_assert(AW_gc_manager::color_groups_initialized());
    aw_assert(valid_color_group(color_group));
    return awr->awar(colorgroupname_awarname(color_group))->read_string();
}

static const int STD_LABEL_LEN = 15;

void AW_gc_manager::create_gc_buttons(AW_window *aws, bool for_colorgroups) {
    for (int idx = for_colorgroups ? first_colorgroup_idx : 0; idx<int(GCs.size()); ++idx) {
        const gc_desc& gcd = GCs[idx];

        if (gcd.is_color_group() != for_colorgroups) continue;

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
            aws->label(gcd.colorlabel.c_str());
        }

        // length of color button cannot be modified (diff to motif version)
        aws->create_color_button(color_awarname(gc_base_name, gcd.key), NULL);
        if (gcd.has_font)   {
            // length of font button cannot be modified (diff to motif version)
            aws->create_font_button(font_awarname(gc_base_name, gcd.key), NULL);
        }
        if (!gcd.same_line) aws->at_newline();
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

    // select all fonts:
    aws->label_length(STD_LABEL_LEN);
    aws->label("All fonts:");
    aws->create_font_button(font_awarname(gcman->get_base_name(), ALL_FONTS_ID), NULL);
    aws->at_newline();

    // anti-aliasing:
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

// --------------------------------
//      RGB <-> HSV conversion

class AW_hsv {
    float H, S, V;
public:
    AW_hsv(float hue, float saturation, float value) : H(hue), S(saturation), V(value) {
        aw_assert(H>=0.0 && H<360.0);
        aw_assert(S>=0.0 && S<=1.0);
        aw_assert(V>=0.0 && V<=1.0);
    }
    AW_hsv(const AW_rgb_normalized& col) {
        float R = col.r();
        float G = col.g();
        float B = col.b();

        float min = std::min(std::min(R, G), B);
        float max = std::max(std::max(R, G), B);

        if (min == max) {
            H = 0;
        }
        else {
            H = 60;

            if      (max == R) { H *= 0 + (G-B)/(max-min); }
            else if (max == G) { H *= 2 + (B-R)/(max-min); }
            else               { H *= 4 + (R-G)/(max-min); }

            if (H<0) H += 360;
        }

        S = max ? (max-min)/max : 0;
        V = max;
    }
#if defined(Cxx11)
    AW_hsv(const AW_rgb16& col) : AW_hsv(AW_rgb_normalized(col)) {}
#else // !defined(Cxx11)
    AW_hsv(const AW_rgb16& col) { *this = AW_rgb_normalized(col); }
#endif

    AW_rgb_normalized rgb() const {
        int   hi = int(H/60);
        float f  = H/60-hi;

        float p = V*(1-S);
        float q = V*(1-S*f);
        float t = V*(1-S*(1-f));

        switch (hi) {
            case 0: return AW_rgb_normalized(V, t, p); //   0 <= H <  60 (deg)
            case 1: return AW_rgb_normalized(q, V, p); //  60 <= H < 120
            case 2: return AW_rgb_normalized(p, V, t); // 120 <= H < 180
            case 3: return AW_rgb_normalized(p, q, V); // 180 <= H < 240
            case 4: return AW_rgb_normalized(t, p, V); // 240 <= H < 300
            case 5: return AW_rgb_normalized(V, p, q); // 300 <= H < 360
        }
        aw_assert(0);
        return AW_rgb_normalized(0, 0, 0);
    }

    float h() const { return H; }
    float s() const { return S; }
    float v() const { return V; }
};


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_rgb_hsv_conversion() {
    // Note: more related tests in AW_rgb.cxx@RGB_TESTS

    const int tested[] = {
        // testing full rgb space takes too long
        // just test all combinations of these:

        0, 1, 2, 3, 4, 5,
        58, 59, 60, 61, 62,
        998, 999, 1000, 1001, 1002,
        32766, 32767, 32768, 32769, 32770,
        39998, 39999, 40000, 40001, 40002,
        65531, 65532, 65533, 65534, 65535
    };

    for (unsigned i = 0; i<ARRAY_ELEMS(tested); ++i) {
        int r = tested[i];
        for (unsigned j = 0; j<ARRAY_ELEMS(tested); ++j) {
            int g = tested[j];
            for (unsigned k = 0; k<ARRAY_ELEMS(tested); ++k) {
                int b = tested[k];

                TEST_ANNOTATE(GBS_global_string("rgb=%i/%i/%i", r, g, b));

                AW_hsv hsv(AW_rgb16(r, g, b));

                // check range overflow
                TEST_EXPECT(hsv.h()>=0.0 && hsv.h()<360.0);
                TEST_EXPECT(hsv.s()>=0.0 && hsv.s()<=1.0);
                TEST_EXPECT(hsv.v()>=0.0 && hsv.v()<=1.0);

                AW_rgb16 RGB(hsv.rgb());

                // fprintf(stderr, "rgb=%i/%i/%i hsv=%i/%i/%i RGB=%i/%i/%i\n", r, g, b, h, s, v, R, G, B);

                // check that rgb->hsv->RGB produces a similar color
                const int MAXDIFF    = 1; // less than .0015% difference per channel
                const int MAXDIFFSUM = 2; // less than .003% difference overall

                TEST_EXPECT(abs(r-RGB.r()) <= MAXDIFF);
                TEST_EXPECT(abs(g-RGB.g()) <= MAXDIFF);
                TEST_EXPECT(abs(b-RGB.b()) <= MAXDIFF);

                TEST_EXPECT((abs(r-RGB.r())+abs(g-RGB.g())+abs(b-RGB.b())) <= MAXDIFFSUM);
            }
        }
    }

    for (unsigned i = 0; i<ARRAY_ELEMS(tested); ++i) {
        int h = tested[i]*320/65535;
        for (unsigned j = 0; j<ARRAY_ELEMS(tested); ++j) {
            float s = tested[j]/65535.0;
            for (unsigned k = 0; k<ARRAY_ELEMS(tested); ++k) {
                float v = tested[k]/65535.0;

                TEST_ANNOTATE(GBS_global_string("hsv=%i/%.3f/%.3f", h, s, v));

                AW_rgb16 rgb(AW_hsv(h, s, v).rgb());
                AW_rgb16 RGB(AW_hsv(rgb).rgb());

                // fprintf(stderr, "hsv=%i/%i/%i rgb=%i/%i/%i HSV=%i/%i/%i RGB=%i/%i/%i\n", h, s, v, r, g, b, H, S, V, R, G, B);

                // check that hsv->rgb->HSV->RGB produces a similar color (comparing hsv makes no sense)
                const int MAXDIFF    = 1; // less than .0015% difference per channel
                const int MAXDIFFSUM = 2; // less than .003% difference overall

                TEST_EXPECT(abs(rgb.r()-RGB.r()) <= MAXDIFF);
                TEST_EXPECT(abs(rgb.g()-RGB.g()) <= MAXDIFF);
                TEST_EXPECT(abs(rgb.b()-RGB.b()) <= MAXDIFF);

                TEST_EXPECT((abs(rgb.r()-RGB.r())+abs(rgb.g()-RGB.g())+abs(rgb.b()-RGB.b())) <= MAXDIFFSUM);
            }
        }
    }

    // specific conversion (showed wrong 'hue' and 'saturation' until [14899])
    {
        AW_hsv hsv(AW_rgb16(0, 0, 14906));

        TEST_EXPECT_SIMILAR(hsv.h(), 240.0, 0.001); //= ~ 240 deg
        TEST_EXPECT_SIMILAR(hsv.s(), 1.0,   0.001); //= 100%
        TEST_EXPECT_SIMILAR(hsv.v(), 0.227, 0.001); //= ~ 22.7%

        AW_rgb16 rgb(hsv.rgb());

        TEST_EXPECT_EQUAL(rgb.r(), 0);
        TEST_EXPECT_EQUAL(rgb.g(), 0);
        TEST_EXPECT_EQUAL(rgb.b(), 14906);
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

