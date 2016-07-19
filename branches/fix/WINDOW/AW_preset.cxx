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
#include "aw_xfont.hxx"
#include "aw_rgb.hxx"

#include <arbdbt.h>
#include <arb_strarray.h>

#include <vector>
#include <map>
#include <string>

#if defined(DEBUG)
#include <ctime>
#endif


using std::string;

#define AWAR_RANGE_OVERLAY       "tmp/GCS/range/overlay"          // global toggle (used for all gc-managers!)
#define AWAR_COLOR_GROUPS_PREFIX "color_groups"
#define AWAR_COLOR_GROUPS_USE    AWAR_COLOR_GROUPS_PREFIX "/use"  // int : whether to use the colors in display or not

#define ATPL_GCMAN_LOCAL "GCS/%s"                          // awar prefix for awars local to gc-manager
#define ATPL_GC_LOCAL    ATPL_GCMAN_LOCAL "/MANAGE_GCS/%s" // awar prefix for awars local to a single gc

#define ALL_FONTS_ID "all_fonts"

// prototypes for motif-only section at bottom of this file:
static void aw_create_color_chooser_window(AW_window *aww, const char *awar_name, const char *color_description);
static void aw_create_font_chooser_window(AW_window *aww, const char *gc_base_name, const struct gc_desc *gcd);

CONSTEXPR_RETURN inline bool valid_color_group(int color_group) {
    return color_group>0 && color_group<=AW_COLOR_GROUPS;
}

inline const char* gcman_specific_awarname(const char *tpl, const char *gcman_id, const char *localpart) {
    aw_assert(GB_check_key(gcman_id)   == NULL); // has to be a key
    aw_assert(GB_check_hkey(localpart) == NULL); // has to be a key or hierarchical key

    static SmartCharPtr awar_name;
    awar_name = GBS_global_string_copy(tpl, gcman_id, localpart);
    return &*awar_name;
}
inline const char* gc_awarname(const char *tpl, const char *gcman_id, const string& colname) {
    return gcman_specific_awarname(tpl, gcman_id, colname.c_str());
}
inline const char* gcman_awarname(const char* gcman_id, const char *localpart) {
    return gcman_specific_awarname(ATPL_GCMAN_LOCAL "/%s", gcman_id, localpart);
}

inline const char* color_awarname   (const char* gcman_id, const string& colname) { return gc_awarname(ATPL_GC_LOCAL "/colorname", gcman_id, colname); }
inline const char* fontname_awarname(const char* gcman_id, const string& colname) { return gc_awarname(ATPL_GC_LOCAL "/font",      gcman_id, colname); }
inline const char* fontsize_awarname(const char* gcman_id, const string& colname) { return gc_awarname(ATPL_GC_LOCAL "/size",      gcman_id, colname); }
inline const char* fontinfo_awarname(const char* gcman_id, const string& colname) { return gc_awarname(ATPL_GC_LOCAL "/info",      gcman_id, colname); }

inline const char *colorgroupname_awarname(int color_group) {
    if (!valid_color_group(color_group)) return NULL;
    return GBS_global_string(AWAR_COLOR_GROUPS_PREFIX "/name%i", color_group);
}
inline const char* default_colorgroup_name(int color_group) {
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
    GC_TYPE_RANGE,
};

#define RANGE_INDEX_BITS 4
#define RANGE_INDEX_MASK ((1<<RANGE_INDEX_BITS)-1)

inline int build_range_gc_number(int range_idx, int color_idx) {
    aw_assert(range_idx == (range_idx & RANGE_INDEX_MASK));
    return (color_idx << RANGE_INDEX_BITS) | range_idx;
}

inline string name2ID(const char *name) { // does not test uniqueness!
    char   *keyCopy = GBS_string_2_key(name);
    string  id      = keyCopy;
    free(keyCopy);
    return id;
}
inline string name2ID(const string& name) { return name2ID(name.c_str()); }

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

    const char *parse_decl(const char *decl, const char *id_prefix) {
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

        key = string(id_prefix ? id_prefix : "") + name2ID(colorlabel);

        return default_color;
    }
};

// --------------------------------
//      types for color-ranges

enum gc_range_type {
    GC_RANGE_INVALID,
    GC_RANGE_LINEAR,
    GC_RANGE_CYCLIC,
    GC_RANGE_PLANAR,
    GC_RANGE_SPATIAL,
};

class gc_range {
    string        name; // name of range shown in config window
    string        id;
    gc_range_type type;

    int index;       // in range-array of AW_gc_manager
    int color_count; // number of support colors (ie. customizable colors)
    int gc_index;    // of first support color in GCs-array of AW_gc_manager

public:
    gc_range(const string& name_, gc_range_type type_, int index_, int gc_index_) :
        name(name_),
        id(name2ID(name)),
        type(type_),
        index(index_),
        color_count(0),
        gc_index(gc_index_)
    {}

    void add_color(const string& colordef, AW_gc_manager *gcman);
    void update_colors(const AW_gc_manager *gcman) const;

    AW_rgb_normalized get_color(int idx, const AW_gc_manager *gcman) const;

    const string& get_name() const { return name; }
    const string& get_id() const { return id; }
    gc_range_type get_type() const { return type; }
    int get_dimension() const {
        switch (type) {
            case GC_RANGE_LINEAR:
            case GC_RANGE_CYCLIC:  return 1;

            case GC_RANGE_PLANAR:  return 2;
            case GC_RANGE_SPATIAL: return 3;
            case GC_RANGE_INVALID: aw_assert(0); break;
        }
        return 0;
    }
};

// --------------------
//      GC manager

class AW_gc_manager : virtual Noncopyable {
    const char *gc_base_name;
    AW_device  *device;
    int         drag_gc_offset; // = drag_gc (as used by clients)

    int first_colorgroup_idx; // index into 'GCs' or NO_INDEX (if no color groups defined)

    AW_window *aww;             // motif only (colors get allocated in window)
    int        colorindex_base; // motif-only (colorindex of background-color)

    typedef std::vector<gc_desc>  gc_container;
    typedef std::vector<gc_range> gc_range_container;

    gc_container       GCs;
    gc_range_container color_ranges;
    unsigned           active_range_number; // offset into 'color_ranges'

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

    AW_color_idx colorindex(int gc) const {
        aw_assert(valid_gc(gc));
        return AW_color_idx(colorindex_base+gc+1);
    }
    static void ignore_change_cb(GcChange) {}

    void allocate_gc(int gc) const;

    void update_gc_color_internal(int gc, const char *color) const;
    void update_range_colors(const gc_desc& gcd) const;
    void update_range_font(int fname, int fsize) const;

public:
    static const char **color_group_defaults;

    static bool use_color_groups;
    static bool show_range_overlay;

    static bool color_groups_initialized() { return color_group_defaults != 0; }

    AW_gc_manager(const char*  name, AW_device* device_, int drag_gc_offset_,
                  AW_window   *aww_, int colorindex_base_)
        : gc_base_name(name),
          device(device_),
          drag_gc_offset(drag_gc_offset_),
          first_colorgroup_idx(NO_INDEX),
          aww(aww_),
          colorindex_base(colorindex_base_),
          active_range_number(-1), // => will be initialized from init_color_ranges
          changed_cb(makeGcChangedCallback(ignore_change_cb)),
          suppress_changed_cb(false),
          did_suppress_change(GC_NOTHING_CHANGED)
    {}

    void init_all_fonts() const;
    void update_all_fonts(bool sizeChanged) const;

    void init_color_ranges(int& gc);
    bool has_color_range() const { return !color_ranges.empty(); }
    int first_range_gc() const { return drag_gc_offset - AW_RANGE_COLORS; }

    bool has_color_groups() const { return first_colorgroup_idx != NO_INDEX; }
    bool has_variable_width_font() const;

    int size() const { return GCs.size(); }
    const gc_desc& get_gc_desc(int idx) const {
        aw_assert(valid_idx(idx));
        return GCs[idx];
    }
    const char *get_base_name() const { return gc_base_name; }
    int get_drag_gc() const { return drag_gc_offset; }

    void add_gc      (const char *gc_description, int& gc, gc_type type, const char *id_prefix = NULL);
    void add_gc_range(const char *gc_description);
    void reserve_gcs (const char *gc_description, int& gc);
    void add_color_groups(int& gc);

    void update_gc_color(int idx) const;
    void update_gc_font(int idx) const;

    void update_range_gc_color(int idx, const char *color) const {
        update_gc_color_internal(first_range_gc()+idx, color);
    }

    void create_gc_buttons(AW_window *aww, gc_type for_gc_type);

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

    const char *get_current_color(int idx) const {
        return AW_root::SINGLETON->awar(color_awarname(get_base_name(), get_gc_desc(idx).key))->read_char_pntr();
    }

    void getColorRangeNames(int dimension, ConstStrArray& ids, ConstStrArray& names) const;
    void activateColorRange(const char *id);
    const char *getActiveColorRangeID(int *dimension) const;

    const char *awarname_active_range() const { return gcman_awarname(get_base_name(), "range/active"); }
    void active_range_changed_cb(AW_root *awr);
};

const char **AW_gc_manager::color_group_defaults = NULL;

bool AW_gc_manager::use_color_groups   = false;
bool AW_gc_manager::show_range_overlay = false;

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
        if (gcd.belongs_to_range()) {
            update_range_font(fname, fsize);
            break; // all leftover GCs belong to ranges = > stop here
        }

        device->set_font(gcd.gc,                fname, fsize, 0);
        device->set_font(gcd.gc+drag_gc_offset, fname, fsize, 0);
    }

    awar_fontinfo->write_string(GBS_global_string("%s | %i", AW_get_font_shortname(fname), fsize));

    trigger_changed_cb(GC_FONT_CHANGED);
}
static void gc_fontOrSize_changed_cb(AW_root*, AW_gc_manager *mgr, int idx) {
    mgr->update_gc_font(idx);
}
inline bool font_has_fixed_width(AW_font aw_font_nr) {
    return AW_font_2_xfig(aw_font_nr) < 0;
}
void AW_gc_manager::update_all_fonts(bool sizeChanged) const {
    AW_root *awr = AW_root::SINGLETON;

    int fname = awr->awar(fontname_awarname(gc_base_name, ALL_FONTS_ID))->read_int();
    int fsize = awr->awar(fontsize_awarname(gc_base_name, ALL_FONTS_ID))->read_int();

    delay_changed_callbacks(true); // temp. disable callbacks
    for (gc_container::const_iterator g = GCs.begin(); g != GCs.end(); ++g) {
        if (g->has_font) {
            if (sizeChanged) {
                awr->awar(fontsize_awarname(gc_base_name, g->key))->write_int(fsize);
            }
            else {
                bool update = !g->fixed_width_font || font_has_fixed_width(fname);
                if (update) awr->awar(fontname_awarname(gc_base_name, g->key))->write_int(fname);
            }
        }
    }
    delay_changed_callbacks(false);
}
static void all_fontsOrSizes_changed_cb(AW_root*, const AW_gc_manager *mgr, bool sizeChanged) {
    mgr->update_all_fonts(sizeChanged);
}

void AW_gc_manager::update_gc_color_internal(int gc, const char *color) const {
    AW_color_idx colorIdx = colorindex(gc);
    aww->alloc_named_data_color(colorIdx, color);

    if (gc == GC_BACKGROUND && colorIdx == AW_DATA_BG) {
        // if background color changes, all drag-gc colors need to be updated
        // (did not understand why, just refactored existing code --ralf)

        for (int i = 1; i<size(); ++i) {
            const gc_desc& gcd = GCs[i];
            if (gcd.belongs_to_range()) break; // do not update range-GCs here

            int g    = gcd.gc;
            colorIdx = colorindex(g);
            device->set_foreground_color(g + drag_gc_offset, colorIdx);
        }
        if (has_color_range()) { // update drag GCs of all range GCs
            for (int i = 0; i<AW_RANGE_COLORS; ++i) {
                int g = first_range_gc()+i;
                colorIdx = colorindex(g);
                device->set_foreground_color(g + drag_gc_offset, colorIdx);
            }
        }
    }
    else {
        if (gc == GC_BACKGROUND) gc = 0; // special case: background color of bottom-area (only used by arb_phylo)

        device->set_foreground_color(gc,                  colorIdx);
        device->set_foreground_color(gc + drag_gc_offset, colorIdx);
    }
}

AW_rgb_normalized gc_range::get_color(int idx, const AW_gc_manager *gcman) const {
    aw_assert(idx>=0 && idx<color_count);
    return AW_rgb_normalized(gcman->get_current_color(gc_index+idx));
}

STATIC_ASSERT(AW_PLANAR_COLORS*AW_PLANAR_COLORS == AW_RANGE_COLORS); // Note: very strong assertion (could also use less than AW_RANGE_COLORS)
STATIC_ASSERT(AW_SPATIAL_COLORS*AW_SPATIAL_COLORS*AW_SPATIAL_COLORS == AW_RANGE_COLORS);

void gc_range::update_colors(const AW_gc_manager *gcman) const {
    /*! recalculate colors of a range (called after one color changed)
     * @param gcman    the GC manager
     */

    // @@@ try HSV color blending as alternative

    if (type == GC_RANGE_LINEAR) {
        aw_assert(color_count == 2); // currently exactly 2 support-colors are required for linear ranges

        AW_rgb_normalized low  = get_color(0, gcman);
        AW_rgb_normalized high = get_color(1, gcman);

        AW_rgb_diff low2high = high-low;
        for (int i = 0; i<AW_RANGE_COLORS; ++i) { // blend colors
            float factor = i/float(AW_RANGE_COLORS-1);
            gcman->update_range_gc_color(i, AW_rgb16(low + factor*low2high).ascii());
        }
    }
    else if (type == GC_RANGE_CYCLIC) {
        aw_assert(color_count >= 3); // less than 3 colors does not make sense for cyclic ranges!

        AW_rgb_normalized low = get_color(0, gcman);
        int               i1  = 0;
        for (int part = 0; part<color_count; ++part) {
            AW_rgb_normalized high     = get_color((part+1)%color_count, gcman);
            AW_rgb_diff       low2high = high-low;

            int i2 = AW_RANGE_COLORS * (float(part+1)/color_count);
            aw_assert(implicated((part+1) == color_count, i2 == AW_RANGE_COLORS));

            for (int i = i1; i<i2; ++i) { // blend colors
                int   o      = i-i1;
                float factor = o/float(i2-i1-1);
                gcman->update_range_gc_color(i, AW_rgb16(low + factor*low2high).ascii());
            }

            low = high;
            i1  = i2;
        }
    }
    else if (type == GC_RANGE_PLANAR) {
        aw_assert(color_count == 3); // currently exactly 3 support-colors are required for planar ranges

        AW_rgb_normalized low  = get_color(0, gcman);
        AW_rgb_normalized dim1 = get_color(1, gcman);
        AW_rgb_normalized dim2 = get_color(2, gcman);

        AW_rgb_diff low2dim1 = dim1-low;
        AW_rgb_diff low2dim2 = dim2-low;

        for (int i1 = 0; i1<AW_PLANAR_COLORS; ++i1) {
            float       fact1 = i1/float(AW_PLANAR_COLORS-1);
            AW_rgb_diff diff1 = fact1*low2dim1;

            for (int i2 = 0; i2<AW_PLANAR_COLORS; ++i2) {
                float       fact2 = i2/float(AW_PLANAR_COLORS-1);
                AW_rgb_diff diff2 = fact2*low2dim2;

                AW_rgb_normalized mixcol = low + (diff1+diff2);

                gcman->update_range_gc_color(i1*AW_PLANAR_COLORS+i2, AW_rgb16(mixcol).ascii());
            }
        }
    }
    else if (type == GC_RANGE_SPATIAL) {
        aw_assert(color_count == 4); // currently exactly 4 support-colors are required for planar ranges

        AW_rgb_normalized low  = get_color(0, gcman);
        AW_rgb_normalized dim1 = get_color(1, gcman);
        AW_rgb_normalized dim2 = get_color(2, gcman);
        AW_rgb_normalized dim3 = get_color(3, gcman);

        AW_rgb_diff low2dim1 = dim1-low;
        AW_rgb_diff low2dim2 = dim2-low;
        AW_rgb_diff low2dim3 = dim3-low;

        for (int i1 = 0; i1<AW_SPATIAL_COLORS; ++i1) {
            float       fact1 = i1/float(AW_SPATIAL_COLORS-1);
            AW_rgb_diff diff1 = fact1*low2dim1;

            for (int i2 = 0; i2<AW_SPATIAL_COLORS; ++i2) {
                float       fact2 = i2/float(AW_SPATIAL_COLORS-1);
                AW_rgb_diff diff2 = fact2*low2dim2;

                for (int i3 = 0; i3<AW_SPATIAL_COLORS; ++i3) {
                    float       fact3 = i3/float(AW_SPATIAL_COLORS-1);
                    AW_rgb_diff diff3 = fact3*low2dim3;

                    AW_rgb_normalized mixcol = low + (diff1+diff2+diff3);
                    gcman->update_range_gc_color((i1*AW_SPATIAL_COLORS+i2)*AW_SPATIAL_COLORS+i3, AW_rgb16(mixcol).ascii());
                }
            }
        }
    }
    else {
        aw_assert(0); // unsupported range-type
    }
}
void AW_gc_manager::update_range_colors(const gc_desc& gcd) const {
    int defined_ranges = color_ranges.size();
    int range_idx      = gcd.get_range_index();

    if (range_idx<defined_ranges) {
        const gc_range& gcr = color_ranges[range_idx];
        gcr.update_colors(this);
    }
}
void AW_gc_manager::update_range_font(int fname, int fsize) const {
    // set font for all GCs belonging to color-range
    int first_gc = first_range_gc();
    for (int i = 0; i<AW_RANGE_COLORS; ++i) {
        int gc = first_gc+i;
        device->set_font(gc,                fname, fsize, 0);
        device->set_font(gc+drag_gc_offset, fname, fsize, 0);
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
static void range_overlay_changed_cb(AW_root *awr, AW_gc_manager *gcmgr) {
    AW_gc_manager::show_range_overlay = awr->awar(AWAR_RANGE_OVERLAY)->read_int();
    gcmgr->trigger_changed_cb(GC_COLOR_GROUP_USE_CHANGED);
}

// ----------------------------
//      define color-ranges

void gc_range::add_color(const string& colordef, AW_gc_manager *gcman) {
    int gc = build_range_gc_number(index, color_count);
    gcman->add_gc(colordef.c_str(), gc, GC_TYPE_RANGE, get_id().c_str());
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

            if      (range_type == "linear")  rtype = GC_RANGE_LINEAR;
            else if (range_type == "planar")  rtype = GC_RANGE_PLANAR;
            else if (range_type == "spatial") rtype = GC_RANGE_SPATIAL;
            else if (range_type == "cyclic")  rtype = GC_RANGE_CYCLIC;

            if (rtype == GC_RANGE_INVALID) {
                error = GBS_global_string("invalid range-type '%s'", range_type.c_str());
            }

            if (!error) {
                gc_range range(range_name, rtype, color_ranges.size(), GCs.size());

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

void AW_gc_manager::active_range_changed_cb(AW_root *awr) {
    // read AWAR (awarname_active_range), compare with 'active_range_number', if differs = > update colors
    // (triggered by AWAR)

    unsigned wanted_range_number = awr->awar(awarname_active_range())->read_int();
    if (wanted_range_number != active_range_number) {
        aw_assert(wanted_range_number<color_ranges.size());

        active_range_number = wanted_range_number;
        const gc_range& active_range = color_ranges[active_range_number];
        active_range.update_colors(this);
    }
}
static void active_range_changed_cb(AW_root *awr, AW_gc_manager *gcman) { gcman->active_range_changed_cb(awr); }

void AW_gc_manager::init_color_ranges(int& gc) {
    if (has_color_range()) {
        aw_assert(gc == first_range_gc()); // 'drag_gc_offset' is wrong (is passed as 'base_drag' to AW_manage_GC)
        int last_gc = gc + AW_RANGE_COLORS-1;
        while (gc<=last_gc) allocate_gc(gc++);

        active_range_changed_cb(AW_root::SINGLETON); // either initializes default-range (=0) or the range-in-use when saving props
    }
}
void AW_gc_manager::activateColorRange(const char *id) {
    unsigned wanted_range_number = 0;
    for (gc_range_container::const_iterator r = color_ranges.begin(); r != color_ranges.end(); ++r) {
        const gc_range& range = *r;
        if (range.get_id() == id) {
            aw_assert(wanted_range_number<color_ranges.size());
            AW_root::SINGLETON->awar(awarname_active_range())->write_int(wanted_range_number); // => will update color range of all identical gc-managers
            break;
        }
        ++wanted_range_number;
    }
}
const char *AW_gc_manager::getActiveColorRangeID(int *dimension) const {
    const gc_range& range     = color_ranges[active_range_number];
    if (dimension) *dimension = range.get_dimension();
    return range.get_id().c_str();
}


void AW_gc_manager::getColorRangeNames(int dimension, ConstStrArray& ids, ConstStrArray& names) const {
    for (gc_range_container::const_iterator r = color_ranges.begin(); r != color_ranges.end(); ++r) {
        const gc_range& range = *r;
        if (range.get_dimension() == dimension) {
            ids.put(range.get_id().c_str());
            names.put(range.get_name().c_str());
        }
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

void AW_gc_manager::add_gc(const char *gc_description, int& gc, gc_type type, const char *id_prefix) {
    aw_assert(strchr("*!&", gc_description[0]) == NULL);
    aw_assert(implicated(type != GC_TYPE_RANGE, !has_color_range())); // color ranges have to be specified AFTER all other color definition strings (in call to AW_manage_GC)

    GCs.push_back(gc_desc(gc, type));
    gc_desc &gcd = GCs.back();
    int      idx = int(GCs.size()-1); // index position where new GC has been added

    if (gcd.is_color_group() && first_colorgroup_idx == NO_INDEX) {
        first_colorgroup_idx = idx; // remember index of first color-group
    }

    bool is_background = gc == GC_BACKGROUND;
    bool alloc_gc      = (!is_background || colorindex_base != AW_DATA_BG) && !gcd.belongs_to_range();
    if (alloc_gc)
    {
        allocate_gc(gc + is_background); // increment only happens for AW_BOTTOM_AREA defining GCs
    }

    const char *default_color = gcd.parse_decl(gc_description, id_prefix);

    aw_assert(strcmp(gcd.key.c_str(), ALL_FONTS_ID) != 0); // id is reserved
#if defined(ASSERTION_USED)
    {
        const string& lastId = gcd.key;
        for (int i = 0; i<idx; ++i) {
            const gc_desc& check = GCs[i];
            aw_assert(check.key != lastId); // duplicate GC-ID!
        }
    }
#endif
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

            AW_font default_font = gcd.fixed_width_font ? AW_DEFAULT_FIXED_FONT : AW_DEFAULT_NORMAL_FONT;

            RootCallback fontChange_cb = makeRootCallback(gc_fontOrSize_changed_cb, this, idx);
            AW_root::SINGLETON->awar_int(fontname_awarname(gc_base_name, gcd.key), default_font)->add_callback(fontChange_cb);
            AW_root::SINGLETON->awar_int(fontsize_awarname(gc_base_name, gcd.key), DEF_FONTSIZE)->add_callback(fontChange_cb);
            AW_root::SINGLETON->awar_string(fontinfo_awarname(gc_base_name, gcd.key), "<select font>");
        }
        // Note: fonts are not initialized here. This is done in init_all_fonts() after all GCs have been defined.
    }

    gc++;
}
void AW_gc_manager::init_all_fonts() const {
    int      ad_font = -1;
    int      ad_size = -1;
    AW_root *awr     = AW_root::SINGLETON;

    // initialize fonts of all defined GCs:
    for (int idx = 0; idx<int(GCs.size()); ++idx) {
        const gc_desc& gcd = GCs[idx];
        if (gcd.has_font) {
            update_gc_font(idx);

            if (ad_font == -1) {
                ad_font = awr->awar(fontname_awarname(gc_base_name, gcd.key))->read_int();
                ad_size = awr->awar(fontsize_awarname(gc_base_name, gcd.key))->read_int();
            }
        }
    }

    // init global font awars (used to set ALL fonts)
    AW_root::SINGLETON->awar_int(fontname_awarname(gc_base_name, ALL_FONTS_ID), ad_font)->add_callback(makeRootCallback(all_fontsOrSizes_changed_cb, this, false));
    AW_root::SINGLETON->awar_int(fontsize_awarname(gc_base_name, ALL_FONTS_ID), ad_size)->add_callback(makeRootCallback(all_fontsOrSizes_changed_cb, this, true));
}

bool AW_gc_manager::has_variable_width_font() const {
    for (gc_container::const_iterator g = GCs.begin(); g != GCs.end(); ++g) {
        if (g->has_font && !g->fixed_width_font) return true;
    }
    return false;
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

AW_gc_manager *AW_manage_GC(AW_window                *aww,
                            const char               *gc_base_name,
                            AW_device                *device,
                            int                       base_drag,
                            AW_GCM_AREA               area,
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
     * @param aww          base window
     * @param gc_base_name (usually the window_id, prefixed to awars)
     * @param device       screen device
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
     *   <type>      ::= 'linear'|'cyclic'|'planar'|'spatial'       "rangetype; implies number of required support colors: linear=2 cyclic=3-N planar=3 spatial=4"
     *   <groupdef>  ::= '&color_groups'                            "insert color-groups here"
     */

    aw_assert(default_background_color[0]);

#if defined(ASSERTION_USED)
    int base_drag_given = base_drag;
#endif

    AW_root *aw_root = AW_root::SINGLETON;

    int            colidx_base = area == AW_GCM_DATA_AREA ? AW_DATA_BG : AW_WINDOW_BG;
    AW_gc_manager *gcmgr       = new AW_gc_manager(gc_base_name, device, base_drag, aww, colidx_base);

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
                    aw_assert(!defined_color_groups); // color-groups trigger specified twice!
                    if (!defined_color_groups) {
                        gcmgr->add_color_groups(gc);
                        defined_color_groups = true;
                    }
                }
                else { aw_assert(0); } // unknown trigger
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

    aw_root->awar_int(AWAR_RANGE_OVERLAY, 0)->add_callback(makeRootCallback(range_overlay_changed_cb, gcmgr));
    aw_root->awar_int(gcmgr->awarname_active_range(), 0)->add_callback(makeRootCallback(active_range_changed_cb, gcmgr));

    gcmgr->init_color_ranges(gc);
    gcmgr->init_all_fonts();

    // installing changed callback here avoids that it gets triggered by initializing GCs
    gcmgr->set_changed_cb(changecb);

#if defined(ASSERTION_USED)
    if (strcmp(gc_base_name, "ARB_PARSIMONY") == 0) {
        // ARB_PARSIMONY does not define color-ranges, but uses same GCs as ARB_NTREE
        // => accept weird 'base_drag_given'
        aw_assert(gc == (base_drag_given-AW_RANGE_COLORS));
    }
    else {
        aw_assert(gc == base_drag_given); // parameter 'base_drag' has wrong value
                                          // (has to be next value after last GC or after last color-group-GC)
    }
#endif

    // @@@ add check: 1. all range IDs have to be unique 
    // @@@ add check: 2. all GC IDs have to be unique 

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

static void create_color_button(AW_window *aws, const char *awar_name, const char *color_description) {
    const char *color     = aws->get_root()->awar(awar_name)->read_char_pntr();
    char       *button_id = GBS_global_string_copy("sel_color[%s]", awar_name);

    aws->callback(makeWindowCallback(aw_create_color_chooser_window, strdup(awar_name), strdup(color_description)));
    aws->create_button(button_id, " ", 0, color);

    free(button_id);
}

static void create_font_button(AW_window *aws, const char *gc_base_name, const gc_desc& gcd) {
    char *button_id = GBS_global_string_copy("sel_font_%s", gcd.key.c_str());

    aws->callback(makeWindowCallback(aw_create_font_chooser_window, gc_base_name, &gcd));
    aws->create_button(button_id, fontinfo_awarname(gc_base_name, gcd.key), 0);

    free(button_id);
}

static const int STD_LABEL_LEN    = 15;
static const int COLOR_BUTTON_LEN = 10;
static const int FONT_BUTTON_LEN  = COLOR_BUTTON_LEN+STD_LABEL_LEN+1;
// => color+font has ~same length as 2 colors (does not work for color groups and does not work at all in gtk)

void AW_gc_manager::create_gc_buttons(AW_window *aws, gc_type for_gc_type) {
    for (int idx = 0; idx<int(GCs.size()); ++idx) {
        const gc_desc& gcd = GCs[idx];

        if (gcd.is_color_group())        { if (for_gc_type != GC_TYPE_GROUP) continue; }
        else if (gcd.belongs_to_range()) { if (for_gc_type != GC_TYPE_RANGE) continue; }
        else                             { if (for_gc_type != GC_TYPE_NORMAL) continue; }

        if (for_gc_type == GC_TYPE_RANGE) {
            if (gcd.get_color_index() == 0) { // first color of range
                const gc_range& range = color_ranges[gcd.get_range_index()];

                const char *type_info = NULL;
                switch (range.get_type()) {
                    case GC_RANGE_LINEAR:  type_info  = "linear 1D range"; break;
                    case GC_RANGE_CYCLIC:  type_info  = "cyclic 1D range"; break;
                    case GC_RANGE_PLANAR:  type_info  = "planar 2D range"; break;
                    case GC_RANGE_SPATIAL: type_info  = "spatial 3D range"; break;
                    case GC_RANGE_INVALID: type_info = "invalid range "; aw_assert(0); break;
                }

                const char *range_headline = GBS_global_string("%s (%s)", range.get_name().c_str(), type_info);
                aws->button_length(60);
                aws->create_button(NULL, range_headline, 0, "+");
                aws->at_newline();
            }
        }

        if (for_gc_type == GC_TYPE_GROUP) {
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

        aws->button_length(COLOR_BUTTON_LEN);
        create_color_button(aws, color_awarname(gc_base_name, gcd.key), gcd.colorlabel.c_str());
        if (gcd.has_font)   {
            aws->button_length(FONT_BUTTON_LEN);
            create_font_button(aws, gc_base_name, gcd);
        }
        if (!gcd.same_line) aws->at_newline();
    }
}

typedef std::map<AW_gc_manager*, AW_window_simple*> GroupWindowRegistry;

static void AW_popup_gc_color_groups_window(AW_window *aww, AW_gc_manager *gcmgr) {
    aw_assert(AW_gc_manager::color_groups_initialized());

    static GroupWindowRegistry     existing_window;
    GroupWindowRegistry::iterator  found = existing_window.find(gcmgr);
    AW_window_simple              *aws   = found == existing_window.end() ? NULL : found->second;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aww->get_root(), "COLOR_GROUP_DEF", "Define color groups");

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

        gcmgr->create_gc_buttons(aws, GC_TYPE_GROUP);

        aws->window_fit();
        existing_window[gcmgr] = aws;
    }

    aws->activate();
}

void AW_popup_gc_color_range_window(AW_window *aww, AW_gc_manager *gcmgr) {
    static GroupWindowRegistry     existing_window;
    GroupWindowRegistry::iterator  found = existing_window.find(gcmgr);
    AW_window_simple              *aws   = found == existing_window.end() ? NULL : found->second;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aww->get_root(), "COLOR_RANGE_EDIT", "Edit color ranges");

        aws->at(10, 10);
        aws->auto_space(5, 5);

        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");
        aws->callback(makeHelpCallback("color_ranges.hlp"));
        aws->create_button("HELP", "HELP", "H");
        aws->at_newline();

        aws->label("Overlay active range");
        aws->create_toggle(AWAR_RANGE_OVERLAY);
        aws->at_newline();

        gcmgr->create_gc_buttons(aws, GC_TYPE_RANGE);

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
    {
        static gc_desc allFont_fake_desc(-1, GC_TYPE_NORMAL);
        allFont_fake_desc.colorlabel       = "<all fonts>";
        allFont_fake_desc.key              = ALL_FONTS_ID;
        allFont_fake_desc.fixed_width_font = !gcman->has_variable_width_font();
        aws->callback(makeWindowCallback(aw_create_font_chooser_window, gcman->get_base_name(), &allFont_fake_desc));
    }
    aws->label_length(STD_LABEL_LEN);
    aws->label("All fonts:");
    aws->button_length(COLOR_BUTTON_LEN);
    aws->create_button("select_all_fonts", "Select", "s");
    aws->at_newline();

    // anti-aliasing:
    // (gtk-only)

    gcman->create_gc_buttons(aws, GC_TYPE_NORMAL);

    bool groups_or_range = false;
    if (gcman->has_color_groups()) {
        aws->callback(makeWindowCallback(AW_popup_gc_color_groups_window, gcman));
        aws->create_autosize_button("EDIT_COLOR_GROUP", "Edit color groups", "E");
        groups_or_range = true;
    }
    if (gcman->has_color_range()) {
        aws->callback(makeWindowCallback(AW_popup_gc_color_range_window, gcman));
        aws->create_autosize_button("EDIT_COLOR_RANGE", "Edit color ranges", "r");
        groups_or_range = true;
    }
    if (groups_or_range) aws->at_newline();

    aws->window_fit();
    return aws;
}
AW_window *AW_create_gc_window(AW_root *aw_root, AW_gc_manager *gcman) {
    return AW_create_gc_window_named(aw_root, gcman, "COLOR_DEF", "Colors and Fonts");
}

int AW_get_drag_gc(AW_gc_manager *gcman) {
    return gcman->get_drag_gc();
}

void AW_displayColorRange(AW_device *device, int first_range_gc, AW::Position start, AW_pos xsize, AW_pos ysize) {
    /*! display active color range
     * @param device          output device
     * @param first_range_gc  first color range gc
     * @param start           position of upper left corner
     * @param xsize           xsize used per color
     * @param ysize           xsize used per color
     *
     * displays AW_PLANAR_COLORS rows and AW_PLANAR_COLORS columns
     */
    using namespace AW;

    if (AW_gc_manager::show_range_overlay) {
        Vector size(xsize, ysize);
        for (int x = 0; x<AW_PLANAR_COLORS; ++x) {
            for (int y = 0; y<AW_PLANAR_COLORS; ++y) {
                int      gc     = first_range_gc + y*AW_PLANAR_COLORS+x;
                Vector   toCorner(x*xsize, y*ysize);
                Position corner = start+toCorner;
                device->box(gc, FillStyle::SOLID, Rectangle(corner, size));
            }
        }
    }
}

void AW_getColorRangeNames(const AW_gc_manager *gcman, int dimension, ConstStrArray& ids, ConstStrArray& names) {
    /*! retrieve selected color-range IDs
     * @param gcman      the gc-manager defining the color-ranges
     * @param dimension  the wanted dimension of the color-ranges
     * @param ids        array where range-IDs will be stored
     * @param names      array where corresponding range-names will be stored (same index as 'ids')
     */
    ids.clear();
    names.clear();
    gcman->getColorRangeNames(dimension, ids, names);
}

int AW_getFirstRangeGC(AW_gc_manager *gcman) { return gcman->first_range_gc(); }
void AW_activateColorRange(AW_gc_manager *gcman, const char *id) { gcman->activateColorRange(id); }

const char *AW_getActiveColorRangeID(AW_gc_manager *gcman, int *dimension) {
    /*! @return the ID of the active color range
     * @param gcman      of this gc-manager
     * @param dimension  if !NULL -> is set to dimension of range
     */
    return gcman->getActiveColorRangeID(dimension);
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


// ------------------------------
//      motif color selector

#define AWAR_SELECTOR_COLOR_LABEL "tmp/aw/color_label"

#define AWAR_CV_R "tmp/aw/color_r" // rgb..
#define AWAR_CV_G "tmp/aw/color_g"
#define AWAR_CV_B "tmp/aw/color_b"
#define AWAR_CV_H "tmp/aw/color_h" // hsv..
#define AWAR_CV_S "tmp/aw/color_s"
#define AWAR_CV_V "tmp/aw/color_v"

static char *current_color_awarname         = 0; // name of the currently modified color-awar
static bool  ignore_color_value_change      = false;
static bool  color_value_change_was_ignored = false;

static void aw_set_rgb_sliders(AW_root *awr, const AW_rgb_normalized& col) {
    color_value_change_was_ignored = false;
    {
        LocallyModify<bool> delayUpdate(ignore_color_value_change, true);
        awr->awar(AWAR_CV_R)->write_float(col.r());
        awr->awar(AWAR_CV_G)->write_float(col.g());
        awr->awar(AWAR_CV_B)->write_float(col.b());
    }
    if (color_value_change_was_ignored) awr->awar(AWAR_CV_B)->touch();
}

static void aw_set_sliders_from_color(AW_root *awr) {
    const char *color = awr->awar(current_color_awarname)->read_char_pntr();
    aw_set_rgb_sliders(awr, AW_rgb_normalized(color));
}

inline void aw_set_color(AW_root *awr, const char *color_name) {
    awr->awar(current_color_awarname)->write_string(color_name);
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
            float h = awr->awar(AWAR_CV_H)->read_float();
            float s = awr->awar(AWAR_CV_S)->read_float();
            float v = awr->awar(AWAR_CV_V)->read_float();

            if (h>=360.0) h -= 360;

            AW_rgb_normalized col(AW_hsv(h, s, v).rgb());
            aw_set_color(awr, AW_rgb16(col).ascii());

            awr->awar(AWAR_CV_R)->write_float(col.r());
            awr->awar(AWAR_CV_G)->write_float(col.g());
            awr->awar(AWAR_CV_B)->write_float(col.b());
        }
        else {
            AW_rgb_normalized col(awr->awar(AWAR_CV_R)->read_float(),
                                  awr->awar(AWAR_CV_G)->read_float(),
                                  awr->awar(AWAR_CV_B)->read_float());

            aw_set_color(awr, AW_rgb16(col).ascii());

            AW_hsv hsv(col);

            awr->awar(AWAR_CV_H)->write_float(hsv.h());
            awr->awar(AWAR_CV_S)->write_float(hsv.s());
            awr->awar(AWAR_CV_V)->write_float(hsv.v());
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
        awr->awar_float(colorValueAwars[cv])
            ->set_minmax(0.0, cv == 1 ? 360.0 : 1.0)
            ->add_callback(makeRootCallback(colorslider_changed_cb, bool(cv%2)));
    }
}
static void aw_create_color_chooser_window(AW_window *aww, const char *awar_name, const char *color_description) {
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

        const int INPUTFIELD_WIDTH = 12;
        const int SCALERLENGTH     = 320;

        for (int row = 0; row<3; ++row) {
            aws->at(x1, y1+(row+1)*(y2-y1));
            const ColorValue *vc = &colorValue[row*2];
            aws->label(vc->label);
            aws->create_input_field_with_scaler(vc->awar, INPUTFIELD_WIDTH, SCALERLENGTH, AW_SCALER_LINEAR);
            ++vc;
            aws->label(vc->label);
            aws->create_input_field_with_scaler(vc->awar, INPUTFIELD_WIDTH, SCALERLENGTH, AW_SCALER_LINEAR);
        }

        aws->button_length(1);
        aws->at_newline();

        const float SATVAL_INCREMENT = 0.2;
        const int   HUE_INCREMENT    = 10;
        const int   COLORS_PER_ROW   = 360/HUE_INCREMENT;

        for (int v = 5; v>=2; --v) {
            float val = v*SATVAL_INCREMENT;
            bool  rev = !(v%2);
            for (int s = rev ? 2 : 5; rev ? s<=5 : s>=2; s = rev ? s+1 : s-1) {
                float sat = s*SATVAL_INCREMENT;
                for (int hue = 0; hue<360;  hue += HUE_INCREMENT) {
                    const char *color_name = AW_rgb16(AW_hsv(hue, sat, val).rgb()).ascii();
                    aws->callback(makeWindowCallback(aw_set_color, strdup(color_name)));
                    aws->create_button(color_name, "", 0, color_name);
                }
                aws->at_newline();
            }
        }

        for (int p = 0; p<COLORS_PER_ROW; ++p) {
            float       grey       = (1.0 * p) / (COLORS_PER_ROW-1);
            const char *color_name = AW_rgb16(AW_rgb_normalized(grey, grey, grey)).ascii();

            aws->callback(makeWindowCallback(aw_set_color, strdup(color_name)));
            aws->create_button(color_name, "=", 0, color_name);
        }
        aws->at_newline();

        aws->window_fit();
    }
    awr->awar(AWAR_SELECTOR_COLOR_LABEL)->write_string(color_description);
    freedup(current_color_awarname, awar_name);
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

static void aw_create_font_chooser_window(AW_window *aww, const char *gc_base_name, const gc_desc *gcd) {
    AW_root *awr = aww->get_root();

    static AW_window_simple *aw_fontChoose[2] = { NULL, NULL }; // one for fixed-width font; one general

    bool fixed_width_only = gcd->fixed_width_font;

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
                const char *font_string = AW_get_font_specification(aw_font_nr);
                if (!font_string) {
                    fprintf(stderr, "[Font detection: tried=%i, found=%i]\n", font_nr, fonts_inserted);
                    break;
                }

                if (fixed_width_only && !font_has_fixed_width(aw_font_nr)) continue;
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

    awr->awar(AWAR_SELECTOR_FONT_LABEL)->write_string(gcd->colorlabel.c_str());
    awr->awar(AWAR_SELECTOR_FONT_NAME)->map(awr->awar(fontname_awarname(gc_base_name, gcd->key)));
    awr->awar(AWAR_SELECTOR_FONT_SIZE)->map(awr->awar(fontsize_awarname(gc_base_name, gcd->key)));

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
    return aws;
}

