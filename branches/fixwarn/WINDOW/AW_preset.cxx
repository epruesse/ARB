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

#include "aw_def.hxx"
#include "aw_root.hxx"
#include "aw_awar.hxx"
#include "aw_device.hxx"
#include "aw_advice.hxx"
#include "aw_question.hxx"
#include "aw_msg.hxx"
#include "aw_nawar.hxx"

#include <arbdbt.h>
#include <ad_colorset.h>

#define AWAR_COLOR_GROUPS_PREFIX  "color_groups"
#define AWAR_COLOR_GROUPS_USE     AWAR_COLOR_GROUPS_PREFIX "/use" // int : whether to use the colors in display or not

CONSTEXPR_RETURN inline bool valid_color_group(int color_group) {
    return color_group>0 && color_group<=AW_COLOR_GROUPS;
}

static const char *_colorgroupname_awarname(int color_group) {
    if (!valid_color_group(color_group)) return NULL;
    return GBS_global_string(AWAR_COLOR_GROUPS_PREFIX "/name%i", color_group);
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

struct AW_MGC_cb_struct : virtual Noncopyable { // one for each canvas
    AW_window         *aw;
    GcChangedCallback  cb;

    void call(GcChange whatChanged) { cb(whatChanged); }

    char      *window_awar_name;
    AW_device *device;

    struct AW_MGC_awar_cb_struct *next_drag;

    AW_MGC_cb_struct(AW_window *aw_, const char *gc_base_name, const GcChangedCallback& wcb)
        : aw(aw_),
          cb(wcb),
          window_awar_name(strdup(gc_base_name)),
          device(NULL),
          next_drag(NULL)
    {}
};

class aw_gc_manager;
struct AW_MGC_awar_cb_struct {  // one for each awar
    struct AW_MGC_cb_struct      *cbs;
    const char                   *fontbasename;
    const char                   *colorbasename;
    short                         gc;
    short                         gc_drag;
    short                         colorindex;
    aw_gc_manager                *gcmgr;
    AW_window                    *gc_def_window;
    struct AW_MGC_awar_cb_struct *next;
};


class aw_gc_manager {
    const char            *field;
    const char            *default_value;
    AW_option_menu_struct *font_size_handle; // the option menu to define the font size of the GC
    AW_MGC_awar_cb_struct *font_change_cb_parameter;
    aw_gc_manager         *next;

public:
    aw_gc_manager(const char *field_, const char *default_value_)
        : field(field_)
        , default_value(default_value_)
        , font_size_handle(0)
        , next(0)
    {}

    void enqueue(aw_gc_manager *next_) {
        aw_assert(!next);
        next = next_;
    }

    const char *get_field() const { return field; }
    const char *get_default_value() const { return default_value; }
    const aw_gc_manager *get_next() const { return next; }
    aw_gc_manager *get_next() { return next; }

    void set_font_size_handle(AW_option_menu_struct *oms) { font_size_handle = oms; }
    AW_option_menu_struct *get_font_size_handle() const { return font_size_handle; }

    void set_font_change_parameter(AW_MGC_awar_cb_struct *cb_data) { font_change_cb_parameter = cb_data; }
    AW_MGC_awar_cb_struct *get_font_change_parameter() const { return font_change_cb_parameter; }
};

// --------------------
//      motif only

static void add_font_sizes_to_option_menu(AW_window *aww, int count, int *available_sizes) {
    char ssize[20];
    bool default_size_set = false;

    for (int idx = 0; idx < count; ++idx) {
        int size = available_sizes[idx];
        if (!default_size_set && size > DEF_FONTSIZE) { // insert default size if missing
            sprintf(ssize, "%i", DEF_FONTSIZE);
            aww->insert_default_option(ssize, 0, (int) DEF_FONTSIZE);
            default_size_set = true;
        }
        sprintf(ssize, "%i", size);
        if (size == DEF_FONTSIZE) {
            aww->insert_default_option(ssize, 0, (int) size);
            default_size_set = true;
        }
        else {
            aww->insert_option(ssize, 0, (int) size);
        }
    }

    if (!default_size_set) {
        sprintf(ssize, "%i", DEF_FONTSIZE);
        aww->insert_default_option(ssize, 0, (int) DEF_FONTSIZE);
    }

    aww->update_option_menu();
}
static void aw_init_font_sizes(AW_root *awr, AW_MGC_awar_cb_struct *cbs, bool firstCall) {
    AW_option_menu_struct *oms = cbs->gcmgr->get_font_size_handle();
    aw_assert(oms);

    if (oms) {                  // has font size definition
        int  available_sizes[MAX_FONTSIZE-MIN_FONTSIZE+1];
        char awar_name[256];
        sprintf(awar_name, AWP_FONTNAME_TEMPLATE, cbs->cbs->window_awar_name, cbs->fontbasename);

        int        font_nr     = awr->awar(awar_name)->read_int();
        int        found_sizes = cbs->cbs->device->get_available_fontsizes(cbs->gc, font_nr, available_sizes);
        AW_window *aww         = cbs->gc_def_window;
        aw_assert(aww);

        if (!firstCall) aww->clear_option_menu(oms);
        add_font_sizes_to_option_menu(aww, found_sizes, available_sizes);
    }
}
static void aw_font_changed_cb(AW_root *awr, AW_MGC_awar_cb_struct *cbs) {
    aw_init_font_sizes(awr, cbs, false);
}

static void aw_message_reload(AW_root *) {
    aw_message("Sorry, to activate new colors:\n"
                "   save properties\n"
                "   and restart application");
}
static void AW_preset_create_font_chooser(AW_window *aws, const char *awar, const char *label, bool message_reload) {
    if (message_reload) aws->get_root()->awar(awar)->add_callback(aw_message_reload);

    aws->label(label);
    aws->create_option_menu(awar, true);
    aws->insert_option        ("5x8",   "5", "5x8");
    aws->insert_option        ("6x10",   "6", "6x10");
    aws->insert_option        ("7x13",   "7", "7x13");
    aws->insert_option        ("7x13bold",   "7", "7x13bold");
    aws->insert_option        ("8x13",   "8", "8x13");
    aws->insert_option        ("8x13bold",   "8", "8x13bold");
    aws->insert_option        ("9x15",   "9", "9x15");
    aws->insert_option        ("9x15bold",   "9", "9x15bold");
    aws->insert_option        ("helvetica-12",   "9", "helvetica-12");
    aws->insert_option        ("helvetica-bold-12",   "9", "helvetica-bold-12");
    aws->insert_option        ("helvetica-13",   "9", "helvetica-13");
    aws->insert_option        ("helvetica-bold-13",   "9", "helvetica-bold-13");
    aws->insert_default_option("other", "o", "");
    aws->update_option_menu();
}

static char *aw_glob_font_awar_name = 0;

static int hex2dez(char c) {
    if (c>='0' && c<='9') return c-'0';
    if (c>='A' && c<='F') return c-'A'+10;
    if (c>='a' && c<='f') return c-'a'+10;
    return -1;
}
static void aw_incdec_color(AW_window *aww, const char *action) {
    // action is sth like "r+" "b-" "g++" "r--"
    AW_awar *awar  = aww->get_root()->awar(aw_glob_font_awar_name);
    char    *color = awar->read_string();
    bool     err   = true;

    fprintf(stderr, "current color is '%s'\n", color);

    if (color[0]=='#') {
        int len = strlen(color);
        if (len==4 || len==7) {
            len = (len-1)/3; // len of one color channel (1 or 2)
            aw_assert(len==1 || len==2);

            int diff = action[2]==action[1] ? 7 : 1;

            int channel[3];
            for (int c=0; c<3; ++c) {
                if (len==2) channel[c] = hex2dez(color[c*len+1])*16+hex2dez(color[c*len+2]);
                else        channel[c] = hex2dez(color[c*len+1])*16;
            }

            int rgb;
            for (rgb=0; rgb<3; ++rgb) {
                if (action[0]=="rgb"[rgb] || action[0]=='a') {
                    if (action[1]=='+') { channel[rgb] += diff; if (channel[rgb]>255) channel[rgb]=255; }
                    else                { channel[rgb] -= diff; if (channel[rgb]<0)   channel[rgb]=0; }
                }
            }

            sprintf(color, "#%2.2X%2.2X%2.2X", channel[0], channel[1], channel[2]);

            err = false;
            awar->write_string(color);
        }
    }

    if (err) {
        aw_message("Only color values in #rgb- or #rrggbb-style \n"
                   "can be modified by these buttons. \n"
                   "Choose a color below and try again.");
    }
}

#define AWAR_GLOBAL_COLOR_NAME "tmp/aw/color_label"

#define AWAR_CV_R "tmp/aw/color_r"
#define AWAR_CV_G "tmp/aw/color_g"
#define AWAR_CV_B "tmp/aw/color_b"
#define AWAR_CV_H "tmp/aw/color_h"
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

static bool ignore_color_value_change      = false;
static bool color_value_change_was_ignored = false;

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

static void aw_set_sliders_from_color(AW_root *awr) {
    const char *color = awr->awar(aw_glob_font_awar_name)->read_char_pntr();

    int r = 0;
    int g = 0;
    int b = 0;

    if (color[0] == '#') {
        int len = strlen(color+1);
        if (len == 6) {
            r = hex2dez(color[1])*16+hex2dez(color[2]);
            g = hex2dez(color[3])*16+hex2dez(color[4]);
            b = hex2dez(color[5])*16+hex2dez(color[6]);
        }
        else if (len == 3) {
            r = hex2dez(color[1]); r = r*16+r;
            g = hex2dez(color[2]); g = g*16+g;
            b = hex2dez(color[3]); b = b*16+b;
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

static void color_value_changed_cb(AW_root *awr, bool hsv_changed) {
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

static void aw_create_color_awars(AW_root *awr) {
    awr->awar_string(AWAR_GLOBAL_COLOR_NAME);
    static const char *colorValueAwars[] = {
        AWAR_CV_R, AWAR_CV_H,
        AWAR_CV_G, AWAR_CV_S,
        AWAR_CV_B, AWAR_CV_V,
    };

    for (int cv = 0; cv<6; ++cv) {
        awr->awar_int(colorValueAwars[cv])
            ->set_minmax(0, 255)
            ->add_callback(makeRootCallback(color_value_changed_cb, bool(cv%2)));
    }
}

static void aw_create_color_chooser_window(AW_window *aww, const char *awar_name, const char *label_name) {
    AW_root *awr = aww->get_root();
    static AW_window_simple *aws = 0;
    if (!aws) {
        aw_create_color_awars(awr);

        aws = new AW_window_simple;
        aws->init(awr, "COLORS", "COLORS");

        int x1 = 10;
        int y1 = 10;

        aws->at(x1, y1);
        aws->auto_space(3, 3);
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->button_length(20);
        aws->create_button(NULL, AWAR_GLOBAL_COLOR_NAME, "A");

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
    awr->awar(AWAR_GLOBAL_COLOR_NAME)->write_string(label_name);
    freedup(aw_glob_font_awar_name, awar_name);
    aw_set_sliders_from_color(awr);
    aws->activate();
}
static void AW_preset_create_color_chooser(AW_window *aws, const char *awar_name, const char *label, bool message_reload, bool show_label)
{
    if (message_reload) aws->get_root()->awar(awar_name)->add_callback(aw_message_reload);
    if (show_label) {
        aw_assert(label);
        aws->label(label);
    }
    aws->callback(makeWindowCallback(aw_create_color_chooser_window, strdup(awar_name), strdup(label)));
    char *color     = aws->get_root()->awar(awar_name)->read_string();
    char *button_id = GBS_global_string_copy("sel_color[%s]", awar_name);
    aws->create_button(button_id, " ", 0, color);
    free(button_id);
    free(color);
}

//      motif only (end)
// --------------------------

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
        char *value = aw_root->awar(GBS_global_string(AWP_COLORNAME_TEMPLATE, source_window, id))->read_string();
        aw_root->awar(GBS_global_string(AWP_COLORNAME_TEMPLATE, dest_window, id))->write_string(value);
        free(value);

        if (has_font_info) {
            int ivalue = aw_root->awar(GBS_global_string(AWP_FONTNAME_TEMPLATE, source_window, id))->read_int();
            aw_root->awar(GBS_global_string(AWP_FONTNAME_TEMPLATE, dest_window, id))->write_int(ivalue);
            ivalue     = aw_root->awar(GBS_global_string(AWP_FONTSIZE_TEMPLATE, source_window, id))->read_int();
            aw_root->awar(GBS_global_string(AWP_FONTSIZE_TEMPLATE, dest_window, id))->write_int(ivalue);
        }

        id = va_arg(parg, const char*); // another argument ?
    }

    va_end(parg);
}

static bool         color_groups_initialized = false;
static bool         use_color_groups         = false;
static const char **color_group_gc_defaults  = 0;


static void aw_gc_changed_cb(AW_root *awr, AW_MGC_awar_cb_struct *cbs, int mode) {
    static int dont_recurse = 0;

    if (dont_recurse == 0) {
        ++dont_recurse;
        // mode == -1 -> no callback
        char awar_name[256];
        int font;
        int size;

        sprintf(awar_name, AWP_FONTNAME_TEMPLATE, cbs->cbs->window_awar_name, cbs->fontbasename);
        font = awr->awar(awar_name)->read_int();

        sprintf(awar_name, AWP_FONTSIZE_TEMPLATE, cbs->cbs->window_awar_name, cbs->fontbasename);
        AW_awar *awar_font_size = awr->awar(awar_name);
        size                    = awar_font_size->read_int();

        int found_font_size;
        cbs->cbs->device->set_font(cbs->gc, font, size, &found_font_size);
        cbs->cbs->device->set_font(cbs->gc_drag, font, size, 0);
        if (found_font_size != -1 && found_font_size != size) {
            // correct awar value if exact fontsize wasn't found
            awar_font_size->write_int(found_font_size);
        }

        if (mode != -1) {
            cbs->cbs->call(GC_FONT_CHANGED);
        }
        --dont_recurse;
    }
}

static void aw_gc_color_changed_cb(AW_root *root, AW_MGC_awar_cb_struct *cbs, int mode)
{
    char awar_name[256];
    char *colorname;

    sprintf(awar_name, AWP_COLORNAME_TEMPLATE, cbs->cbs->window_awar_name, cbs->colorbasename);
    colorname = root->awar(awar_name)->read_string();
    AW_color_idx color = (AW_color_idx)cbs->colorindex;
    cbs->cbs->aw->alloc_named_data_color(color, colorname);
    if (color != AW_DATA_BG) {
        cbs->cbs->device->set_foreground_color(cbs->gc, color);
        cbs->cbs->device->set_foreground_color(cbs->gc_drag, color);
    }
    else {
        struct AW_MGC_awar_cb_struct *acbs;
        for (acbs = cbs->cbs->next_drag; acbs; acbs=acbs->next) {
            cbs->cbs->device->set_foreground_color(acbs->gc_drag, (AW_color_idx)acbs->colorindex);
        }
    }
    if (mode != -1) {
        cbs->cbs->call(GC_COLOR_CHANGED);
    }
    free(colorname);
}

static void AW_color_group_usage_changed_cb(AW_root *awr, const GcChangedCallback *gc_changed_cb) {
    use_color_groups = awr->awar(AWAR_COLOR_GROUPS_USE)->read_int();
    (*gc_changed_cb)(GC_COLOR_GROUP_USE_CHANGED);
}

static void AW_color_group_name_changed_cb(AW_root *) {
    AW_advice("To activate the new names for color groups you have to\n"
              "save properties and restart the program.",
              AW_ADVICE_TOGGLE, "Color group name has been changed", 0);
}

static void AW_init_color_groups(AW_root *awr, AW_default def, const GcChangedCallback& gccb) {
    if (!color_groups_initialized) {
        AW_awar                   *useAwar    = awr->awar_int(AWAR_COLOR_GROUPS_USE, 1, def);
        GcChangedCallback * const  changed_cb = new GcChangedCallback(gccb); // bound to cb

        use_color_groups = useAwar->read_int();
        useAwar->add_callback(makeRootCallback(AW_color_group_usage_changed_cb, changed_cb));

        char name_buf[AW_COLOR_GROUP_NAME_LEN+1];
        for (int i = 1; i <= AW_COLOR_GROUPS; ++i) {
            sprintf(name_buf, "color_group_%i", i);
            awr->awar_string(_colorgroupname_awarname(i), name_buf, def)->add_callback(AW_color_group_name_changed_cb);
        }
        color_groups_initialized = true;
    }
}

struct gc_props {
    bool hidden;
    bool select_font;
    bool select_color;

    bool fixed_fonts_only;
    bool append_same_line;

    gc_props()
        : hidden(false),
          select_font(true),
          select_color(true),
          fixed_fonts_only(false),
          append_same_line(false)
    {}

private:
    bool parse_char(char c) {
        switch (c) {
            case '#': fixed_fonts_only = true; break;
            case '+': append_same_line = true; break;

            case '=': select_color = false; break;
            case '-': {
                if (select_font) select_font = false;
                else hidden                  = true; // two '-' means 'hidden'
                break;
            }

            default : return false;
        }
        return true;
    }

    void correct() {
        if (!select_font && !select_color) hidden             = true;
        if (append_same_line && select_font) append_same_line = false;
    }
public:

    int parse_decl(const char *decl) {
        // returns number of (interpreted) prefix characters
        int offset = 0;
        while (decl[offset]) {
            if (!parse_char(decl[offset])) break;
            offset++;
        }
        correct();
        return offset;
    }
};

// ----------------------------------------------------------------------
// force-diff-sync 265873246583745 (remove after merging back to trunk)
// ----------------------------------------------------------------------

AW_gc_manager AW_manage_GC(AW_window                *aww,
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
     * creates some GCs
     * eg.
     *     AW_manage_GC(aww,"ARB_NT",device,10,20,AW_GCM_DATA_AREA, my_expose_cb, cd1 ,cd2, "name","#sequence",NULL);
     *     (see implementation for more details on parameter strings)
     *     will create 2 GCs:
     *      GC 10
     *      GC 11 (monospaced) (indicated by '#')
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

    AW_root *aw_root = aww->get_root();

    AW_MGC_cb_struct *mcbs = new AW_MGC_cb_struct(aww, gc_base_name, changecb);
    mcbs->device = device;

    AW_default aw_def = AW_ROOT_DEFAULT;
    AW_init_color_groups(aw_root, aw_def, changecb);

    char background[50];
    sprintf(background, "-background$%s", default_background_color);

    va_list parg;
    va_start(parg, default_background_color);
    const char *id;

    struct aw_gc_manager *gcmgrlast = 0, *gcmgr2=0, *gcmgrfirst=0;

    int col = AW_WINDOW_BG;
    if (area == AW_GCM_DATA_AREA) {
        col = AW_DATA_BG;
    }
    bool first = true;

    aww->main_drag_gc = base_drag;
    gcmgrfirst = gcmgrlast = new aw_gc_manager(mcbs->window_awar_name, 0);

    const char *last_font_base_name = "default";

    for (int loop = 1; loop <= 2; ++loop) {
        int color_group_counter = 0;

        if (loop == 1) { // set default colors
            id = background;
        }
        else { // set color_group colors
            id = 0;
            if (define_color_groups) {
                aw_assert(color_group_gc_defaults); // forgot to call AW_init_color_group_defaults ?
                id = color_group_gc_defaults[color_group_counter++];
            }
        }

        while (id) {
            gc_props gcp;
            AW_MGC_awar_cb_struct *acbs = 0;
            {
                char       *id_copy       = strdup(id);
                const char *default_color = 0;

                {
                    char *color = strchr(id_copy, '$'); // search color def

                    if (color) {
                        *color++ = 0;

                        if (color[0] == '{') {
                            char *close = strchr(color+1, '}');
                            aw_assert(close); // format is '${KNOWN_GC}'
                            aw_assert(close[1] == 0); // unexpected chars behind
                            if (close) {
                                close[0] = 0;
                                color++;

                                aw_gc_manager *known = gcmgrfirst;
                                aw_assert(known);
                                while (known) {
                                    if (strcmp(known->get_field(), color) == 0) { // found referred gc
                                        default_color = known->get_default_value(); // use it's default color
                                        break;
                                    }
                                    known = known->get_next();
                                }
                                aw_assert(known); // referred to unknown {GC}
                            }
                        }
                        else {
                            default_color = color;
                        }
                    }
                }

                if (!default_color) default_color = first ? "white" : "black";

                gcmgr2 = new aw_gc_manager(strdup(id_copy), strdup(default_color));

                gcmgrlast->enqueue(gcmgr2);
                gcmgrlast = gcmgr2;

                acbs                = new AW_MGC_awar_cb_struct;
                acbs->cbs           = mcbs;
                acbs->colorbasename = GBS_string_2_key(id_copy);
                acbs->gc            = base_gc;
                acbs->gc_drag       = base_drag;
                acbs->gcmgr         = gcmgr2;
                acbs->gc_def_window = 0;

                if (!first) {
                    acbs->next = mcbs->next_drag;
                    mcbs->next_drag = acbs;
                }

                gcp.parse_decl(id_copy);
                freenull(id_copy);
            }

            AW_font def_font = gcp.fixed_fonts_only ? AW_DEFAULT_FIXED_FONT : AW_DEFAULT_NORMAL_FONT;

            if ((area != AW_GCM_DATA_AREA) || !first) {
                device->new_gc(base_gc);
                device->set_line_attributes(base_gc, 1, AW_SOLID);
                device->set_function(base_gc, AW_COPY);

                device->new_gc(base_drag);
                device->set_line_attributes(base_drag, 1, AW_SOLID);
                device->set_function(base_drag, AW_XOR);
                device->establish_default(base_drag);
            }

            char awar_name[256]; memset(awar_name, 0, 256);
            sprintf(awar_name, AWP_COLORNAME_TEMPLATE, mcbs->window_awar_name, acbs->colorbasename);
            acbs->colorindex = col;

            aw_root->awar_string(awar_name, gcmgr2->get_default_value(), aw_def);
            aw_root->awar(awar_name)->add_callback(makeRootCallback(aw_gc_color_changed_cb, acbs, 0));

            aw_gc_color_changed_cb(aw_root, acbs, -1);

            acbs->fontbasename  = gcp.select_font ? acbs->colorbasename : last_font_base_name;
            last_font_base_name = acbs->fontbasename;

            {
                sprintf(awar_name, AWP_FONTNAME_TEMPLATE, mcbs->window_awar_name, acbs->fontbasename);
                AW_awar *font_awar = aw_root->awar_int(awar_name, def_font, aw_def);
                sprintf(awar_name, AWP_FONTSIZE_TEMPLATE,  mcbs->window_awar_name, acbs->fontbasename);
                AW_awar *font_size_awar = aw_root->awar_int(awar_name, DEF_FONTSIZE, aw_def);

                if (gcp.select_font) {
                    font_awar->add_callback(makeRootCallback(aw_font_changed_cb, acbs));
                    gcmgr2->set_font_change_parameter(acbs);
                }

                font_awar->add_callback(makeRootCallback(aw_gc_changed_cb, acbs, 0));
                font_size_awar->add_callback(makeRootCallback(aw_gc_changed_cb, acbs, 0));
            }

            if (!first) {
                aw_gc_changed_cb(aw_root, acbs, -1);
                base_gc++;
                base_drag++;
            }
            col++;
            first = false;

            // switch to next default:

            if (loop == 1) id = va_arg(parg, char*);
            else {
                aw_assert(color_group_gc_defaults); // forgot to call AW_init_color_group_defaults ?
                id = color_group_gc_defaults[color_group_counter++];
            }
        }
    }

    va_end(parg);

    return (AW_gc_manager)gcmgrfirst;
}

void AW_init_color_group_defaults(const char *for_program) {
    // if for_program == NULL defaults of arb_ntree are silently used
    // if for_program is unknown a warning is shown

    aw_assert(color_group_gc_defaults == 0); // oops - called twice

    if (for_program) {
        if (strcmp(for_program, "arb_ntree")      == 0) color_group_gc_defaults = ARB_NTREE_color_group;
        else if (strcmp(for_program, "arb_edit4") == 0) color_group_gc_defaults = ARB_EDIT4_color_group;
    }

    if (!color_group_gc_defaults) {
        if (for_program) { // unknown program ?
#if defined(DEBUG)
            fprintf(stderr, "No specific defaults for color groups defined (using those from ARB_NTREE)\n");
#endif // DEBUG
        }
        color_group_gc_defaults = ARB_NTREE_color_group;
    }

    aw_assert(color_group_gc_defaults);
}

long AW_find_active_color_group(GBDATA *gb_item) {
    /*! same as GBT_get_color_group() if color groups are active
     * @param gb_item the item
     * @return always 0 if color groups are inactive
     */
    aw_assert(color_groups_initialized);
    return use_color_groups ? GBT_get_color_group(gb_item) : 0;
}

char *AW_get_color_group_name(AW_root *awr, int color_group) {
    aw_assert(color_groups_initialized);
    aw_assert(valid_color_group(color_group));
    return awr->awar(_colorgroupname_awarname(color_group))->read_string();
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
    AW_preset_create_color_chooser(aws, "window/background", "Application Background", true, true);
    aws->at_x(tabstop);
    aws->create_input_field("window/background", 12);

    aws->at_newline();

    AW_preset_create_color_chooser(aws, "window/foreground", "Application Foreground", true, true);
    aws->at_x(tabstop);
    aws->create_input_field("window/foreground", 12);

    aws->at_newline();

    AW_preset_create_color_chooser(aws, "window/color_1", "Color 1", true, true);
    aws->at_x(tabstop);
    aws->create_input_field("window/color_1", 12);

    aws->at_newline();

    AW_preset_create_color_chooser(aws, "window/color_2", "Color 2", true, true);
    aws->at_x(tabstop);
    aws->create_input_field("window/color_2", 12);

    aws->at_newline();

    AW_preset_create_color_chooser(aws, "window/color_3", "Color 3", true, true);

    aws->at_x(tabstop);
    aws->create_input_field("window/color_3", 12);

    aws->at_newline();

    aws->window_fit();
    return (AW_window *)aws;
}



static bool aw_insert_gcs(AW_root *aw_root, AW_window_simple *aws, aw_gc_manager *gcmgr, bool insert_color_groups) {
    // returns true if GCs starting with COLOR_GROUP_PREFIX were found

    bool        has_color_groups = false;
    const char *window_awar_name = gcmgr->get_field();

    for (gcmgr = gcmgr->get_next(); gcmgr; gcmgr = gcmgr->get_next()) {
        const char *id = gcmgr->get_field();
        gc_props    gcp;

        id += gcp.parse_decl(id);

        char *fontbasename   = GBS_string_2_key(id);
        char  awar_name[256];
        bool  is_color_group = strncmp(id, AW_COLOR_GROUP_PREFIX, AW_COLOR_GROUP_PREFIX_LEN) == 0;
        int   color_group    = -1;

        if (is_color_group) {
            has_color_groups = true;
            color_group      = atoi(id+AW_COLOR_GROUP_PREFIX_LEN);
            if (!insert_color_groups) continue;
        }
        else { // is no color group
            if (insert_color_groups) continue;
        }

        if (!gcp.hidden) {
            sprintf(awar_name, AWP_COLORNAME_TEMPLATE, window_awar_name, fontbasename);
            aws->label_length(15);

            if (is_color_group) {
                aw_assert(color_group > 0);
                char *color_group_name = AW_get_color_group_name(aw_root, color_group);
                aws->label(color_group_name);
                free(color_group_name);
            }
            else {
                aws->label(id);
            }

            if (gcp.select_color) {
                aws->button_length(5);
                AW_preset_create_color_chooser(aws, awar_name, id, false, false);
            }
            aws->create_input_field(awar_name, 7);

            if (gcp.select_font) {
                sprintf(awar_name, AWP_FONTNAME_TEMPLATE, window_awar_name, fontbasename);

                aws->label_length(5);
                aws->label("Font");
                aws->create_option_menu(awar_name, true);
                {
                    int         font_nr;
                    const char *font_string;
                    int         fonts_inserted = 0;

                    for (font_nr = 0; ; font_nr++) {
                        font_string = AW_font_2_ascii((AW_font)font_nr);
                        if (!font_string) {
                            fprintf(stderr, "[Font detection: tried=%i, found=%i]\n", int(font_nr), fonts_inserted);
                            break;
                        }

                        // continue; // simulate "no font found"

                        if (gcp.fixed_fonts_only && AW_font_2_xfig((AW_font)font_nr) >= 0) continue;
                        aws->insert_option(font_string, 0, (int)font_nr);
                        ++fonts_inserted;
                    }
                    if (!fonts_inserted) aws->insert_option("No suitable fonts detected", 0, 0);
                    aws->update_option_menu();
                }

                sprintf(awar_name, AWP_FONTSIZE_TEMPLATE, window_awar_name, fontbasename);
                aws->label_length(5);

                aws->label("size");
                AW_option_menu_struct *oms = aws->create_option_menu(awar_name, true);
                gcmgr->set_font_size_handle(oms);

                AW_MGC_awar_cb_struct *acs = gcmgr->get_font_change_parameter();
                acs->gc_def_window         = aws;

                aw_init_font_sizes(aw_root, acs, true); // does update_option_menu
            }
            if (!gcp.append_same_line)  aws->at_newline();
        }
        free(fontbasename);
    }

    return has_color_groups;
}

struct attached_window {
    AW_window_simple *aws;
    AW_gc_manager     attached_to;
    attached_window  *next;
};

static void AW_create_gc_color_groups_name_window(AW_window *, AW_root *aw_root, aw_gc_manager *gcmgr) {
    static struct attached_window *head = 0;

    // search for attached window:

    struct attached_window *look = head;
    while (look) {
        if (look->attached_to == gcmgr) break;
        look = look->next;
    }

    AW_window_simple *aws = 0;

    if (look) {
        aws = look->aws;
    }
    else {
        look = new struct attached_window;

        look->aws         = new AW_window_simple;
        look->attached_to = gcmgr;
        look->next        = head;
        head              = look;

        aws = look->aws;

        aws->init(aw_root, "NAME_COLOR_GROUPS", "COLORS GROUP NAMES");

        aws->at(10, 10);
        aws->auto_space(5, 5);

        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        for (int i = 1; i <= AW_COLOR_GROUPS; ++i) {
            aws->at_newline();

            aws->label(GBS_global_string("Name for color group #%i%s", i, (i >= 10) ? "" : " "));
            aws->create_input_field(_colorgroupname_awarname(i), AW_COLOR_GROUP_NAME_LEN);
        }

        aws->window_fit();
    }

    aws->activate();
}

static void AW_create_gc_color_groups_window(AW_window *, AW_root *aw_root, aw_gc_manager *gcmgr) {
    aw_assert(color_groups_initialized);

    static struct attached_window *head = 0;

    // search for attached window:

    struct attached_window *look = head;
    while (look) {
        if (look->attached_to == gcmgr) break;
        look = look->next;
    }

    AW_window_simple *aws = 0;

    if (look) {
        aws = look->aws;
    }
    else {
        look = new struct attached_window;

        look->aws         = new AW_window_simple;
        look->attached_to = gcmgr;
        look->next        = head;
        head              = look;

        aws = look->aws;

        aws->init(aw_root, "PROPS_COLOR_GROUPS", "COLORS GROUPS");

        aws->at(10, 10);
        aws->auto_space(5, 5);

        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("color_props_groups.hlp"));
        aws->create_button("HELP", "HELP", "H");

        aws->at_newline();

        aw_insert_gcs(aw_root, aws, gcmgr, true);

        aws->at_newline();

        aws->label_length(16);
        aws->label("Use color groups");
        aws->create_toggle(AWAR_COLOR_GROUPS_USE);

        aws->callback(makeWindowCallback(AW_create_gc_color_groups_name_window, aw_root, gcmgr));
        aws->create_autosize_button("DEF_NAMES", "Define names", "D");

        aws->window_fit();
    }

    aws->activate();
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

    aw_gc_manager *gcmgr = (aw_gc_manager *)id_par;

    aws->at(10, 10);
    aws->auto_space(5, 5);

    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("color_props.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at_newline();

    bool has_color_groups = aw_insert_gcs(aw_root, aws, gcmgr, false);

    if (has_color_groups) {
        aws->callback(makeWindowCallback(AW_create_gc_color_groups_window, aw_root, id_par));
        aws->create_autosize_button("EDIT_COLOR_GROUP", "Edit color groups", "E");
        aws->at_newline();
    }

    aws->window_fit();
    return aws;
}

#if defined(UNIT_TESTS)
void fake_AW_init_color_groups() {
    if (!color_groups_initialized) {
        use_color_groups = true;
    }
    color_groups_initialized = true;
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

