// ============================================================= //
//                                                               //
//   File      : AW_preset.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 2, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include <aw_color_groups.hxx>
#include "aw_preset.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_def.hxx"
#include "aw_root.hxx"
#include "aw_awar.hxx"
#include "aw_device.hxx"
#include "aw_advice.hxx"
#include "aw_assert.hxx"
#include <arbdbt.h>


// values optimized for ARB_NTREE :
static const char *ARB_NTREE_color_group[AW_COLOR_GROUPS+1] = {
    "+-" AW_COLOR_GROUP_PREFIX  "1$#D50000", "-" AW_COLOR_GROUP_PREFIX  "2$#00ffff",
    "+-" AW_COLOR_GROUP_PREFIX  "3$#00FF77", "-" AW_COLOR_GROUP_PREFIX  "4$#c700c7",
    "+-" AW_COLOR_GROUP_PREFIX  "5$#0000ff", "-" AW_COLOR_GROUP_PREFIX  "6$#FFCE5B",

    "+-" AW_COLOR_GROUP_PREFIX  "7$#AB2323", "-" AW_COLOR_GROUP_PREFIX  "8$#008888",
    "+-" AW_COLOR_GROUP_PREFIX  "9$#008800", "-" AW_COLOR_GROUP_PREFIX "10$#880088",
    "+-" AW_COLOR_GROUP_PREFIX "11$#000088", "-" AW_COLOR_GROUP_PREFIX "12$#888800",

    0
};

// values optimized for ARB_EDIT4 :
static const char *ARB_EDIT4_color_group[AW_COLOR_GROUPS+1] = {
    "+-" AW_COLOR_GROUP_PREFIX  "1$#FFAFAF", "-" AW_COLOR_GROUP_PREFIX  "2$#A1FFFF",
    "+-" AW_COLOR_GROUP_PREFIX  "3$#AAFFAA", "-" AW_COLOR_GROUP_PREFIX  "4$#c700c7",
    "+-" AW_COLOR_GROUP_PREFIX  "5$#C5C5FF", "-" AW_COLOR_GROUP_PREFIX  "6$#FFE370",

    "+-" AW_COLOR_GROUP_PREFIX  "7$#F87070", "-" AW_COLOR_GROUP_PREFIX  "8$#DAFFFF",
    "+-" AW_COLOR_GROUP_PREFIX  "9$#8DE28D", "-" AW_COLOR_GROUP_PREFIX "10$#880088",
    "+-" AW_COLOR_GROUP_PREFIX "11$#000088", "-" AW_COLOR_GROUP_PREFIX "12$#F1F169",

    0
};

struct AW_MGC_cb_struct {   // one for each window
    AW_MGC_cb_struct(AW_window *awi, void (*g)(AW_window*, AW_CL, AW_CL), AW_CL cd1i, AW_CL cd2i);

    AW_window *aw;
    void (*f)(AW_window*, AW_CL, AW_CL);
    AW_CL      cd1;
    AW_CL      cd2;
    char      *window_awar_name;
    AW_device *device;

    struct AW_MGC_awar_cb_struct *next_drag;
};

AW_MGC_cb_struct::AW_MGC_cb_struct(AW_window *awi, void (*g)(AW_window*, AW_CL, AW_CL), AW_CL cd1i, AW_CL cd2i) {
    memset((char*)this, 0, sizeof(AW_MGC_cb_struct));
    aw  = awi;
    f   = g;
    cd1 = cd1i;
    cd2 = cd2i;

    window_awar_name = strdup(awi->get_window_id());
}

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

void AW_save_properties(AW_window */*aw*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_copy_GCs(AW_root */*aw_root*/, const char */*source_window*/, const char */*dest_window*/, bool /*has_font_info*/, const char */*id0*/, ...) {
    GTK_NOT_IMPLEMENTED;
}


void AW_save_specific_properties(AW_window */*aw*/, const char */*filename*/) {
    GTK_NOT_IMPLEMENTED;
}

void AW_insert_common_property_menu_entries(AW_window_menu_modes */*awmm*/) {
    GTK_NOT_IMPLEMENTED;

}

void AW_insert_common_property_menu_entries(AW_window_simple_menu */*awsm*/) {
    GTK_NOT_IMPLEMENTED;
}

static bool color_groups_initialized = false;
static bool use_color_groups = false;
static const char **color_group_gc_defaults = 0;


static void aw_gc_changed_cb(AW_root *awr, AW_MGC_awar_cb_struct *cbs, long mode)
{
    static int dont_recurse = 0;

    if (dont_recurse == 0) {
        ++dont_recurse;
        // mode == -1 -> no callback
        char awar_name[256];
        int  font;
        int  size;

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
            cbs->cbs->f(cbs->cbs->aw, cbs->cbs->cd1, cbs->cbs->cd2);
        }
        --dont_recurse;
    }
}

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

static void aw_font_changed_cb(AW_root *awr, AW_CL cl_cbs) {
    AW_MGC_awar_cb_struct *cbs = (AW_MGC_awar_cb_struct*)cl_cbs;
    aw_init_font_sizes(awr, cbs, false);
}

static void aw_gc_color_changed_cb(AW_root *root, AW_MGC_awar_cb_struct *cbs, long mode)
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
        cbs->cbs->f(cbs->cbs->aw, cbs->cbs->cd1, cbs->cbs->cd2);
    }
    free(colorname);
}

static const char *AW_get_color_group_name_awarname(int color_group) {
    if (color_group>0 && color_group <= AW_COLOR_GROUPS) {
        static char buf[sizeof(AWAR_COLOR_GROUPS_PREFIX)+1+4+2+1];
        sprintf(buf, AWAR_COLOR_GROUPS_PREFIX "/name%i", color_group);
        return buf;
    }
    return 0;
}



long AW_find_color_group(GBDATA *gbd, bool ignore_usage_flag) {
    /* species/genes etc. may have a color group entry ('ARB_color')
     * call with ignore_usage_flag == true to read color group regardless of global usage flag (AWAR_COLOR_GROUPS_USE)
     */
    aw_assert(color_groups_initialized);
    if (!use_color_groups && !ignore_usage_flag) return 0;

    GBDATA *gb_group = GB_entry(gbd, AW_COLOR_GROUP_ENTRY);
    if (gb_group) return GB_read_int(gb_group);
    return 0;                   // no color group
}

static void AW_color_group_usage_changed_cb(AW_root *awr, AW_CL /* cl_ntw */) {
    use_color_groups       = awr->awar(AWAR_COLOR_GROUPS_USE)->read_int();
    //     AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    //     ntw->refresh();
    // @@@ TODO: a working refresh is missing
}

static void AW_color_group_name_changed_cb(AW_root *) {
    AW_advice("To activate the new names for color groups you have to\n"
              "save properties and restart the program.",
              AW_ADVICE_TOGGLE, "Color group name has been changed", 0);
}


static void AW_init_color_groups(AW_root *awr, AW_default def) {
    if (!color_groups_initialized) {
        AW_awar *useAwar = awr->awar_int(AWAR_COLOR_GROUPS_USE, 1, def);
        use_color_groups = useAwar->read_int();
        useAwar->add_callback(AW_color_group_usage_changed_cb, (AW_CL)0);

        char name_buf[AW_COLOR_GROUP_NAME_LEN+1];
        for (int i = 1; i <= AW_COLOR_GROUPS; ++i) {
            sprintf(name_buf, "color_group_%i", i);
            awr->awar_string(AW_get_color_group_name_awarname(i), name_buf, def)->add_callback(AW_color_group_name_changed_cb);
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


AW_gc_manager AW_manage_GC(AW_window *aww,
                           AW_device *device, int base_gc, int base_drag, AW_GCM_AREA area,
                           void (*changecb)(AW_window*, AW_CL, AW_CL), AW_CL  cd1, AW_CL cd2,
                           bool define_color_groups,
                           const char *default_background_color,
                           ...) {
   /*  Parameter:
     *          aww:                    base window
     *          device:                 screen device
     *          base_gc:                first gc number
     *          base_drag:              one after last gc
     *          area:                   middle,top ...
     *          changecb:               cb if changed
     *          cd1,cd2:                free Parameters to changecb
     *          define_color_groups:    true -> add colors for color groups
     *
     *          ...:        NULL terminated list of \0 terminated strings:
     *
     *          first GC is fixed: '-background'
     *
     *          optionsSTRING   name of GC and AWAR
     *          options:        #   fixed fonts only
     *                          -   no fonts
     *                          --  completely hide GC
     *                          =   no color selector
     *                          +   append next in same line
     *
     *                          $color at end of string = > define default color value
     *                          ${GCname} at end of string = > use default of previously defined color
     */

    AW_root    *aw_root = aww->get_root();
    AW_default  aw_def  = AW_ROOT_DEFAULT;

    AW_init_color_groups(aw_root, aw_def);

    const char           *id;
    va_list               parg;
    va_start(parg, default_background_color);
    AW_font               def_font;
    struct aw_gc_manager *gcmgrlast = 0, *gcmgr2=0, *gcmgrfirst=0;

    AW_MGC_cb_struct *mcbs = new AW_MGC_cb_struct(aww, changecb, cd1, cd2);
    mcbs->device = device;

    int col = AW_WINDOW_BG;
    if (area == AW_GCM_DATA_AREA) {
        col = AW_DATA_BG;
    }
    bool first = true;

    aww->main_drag_gc = base_drag;
    gcmgrfirst = gcmgrlast = new aw_gc_manager(mcbs->window_awar_name, 0);

    const char *last_font_base_name = "default";

    char background[50];
    aw_assert(default_background_color[0]);

    sprintf(background, "-background$%s", default_background_color);

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

            def_font = gcp.fixed_fonts_only ? AW_DEFAULT_FIXED_FONT : AW_DEFAULT_NORMAL_FONT;

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
            aw_root->awar(awar_name)->add_callback((AW_RCB)aw_gc_color_changed_cb, (AW_CL)acbs, (AW_CL)0);

            aw_gc_color_changed_cb(aw_root, acbs, -1);

            acbs->fontbasename  = gcp.select_font ? acbs->colorbasename : last_font_base_name;
            last_font_base_name = acbs->fontbasename;

            {
                sprintf(awar_name, AWP_FONTNAME_TEMPLATE, mcbs->window_awar_name, acbs->fontbasename);
                AW_awar *font_awar = aw_root->awar_int(awar_name, def_font, aw_def);
                sprintf(awar_name, AWP_FONTSIZE_TEMPLATE,  mcbs->window_awar_name, acbs->fontbasename);
                AW_awar *font_size_awar = aw_root->awar_int(awar_name, DEF_FONTSIZE, aw_def);

                if (gcp.select_font) {
                    font_awar->add_callback(aw_font_changed_cb, (AW_CL)acbs);
                    gcmgr2->set_font_change_parameter(acbs);
                }

                font_awar->add_callback((AW_RCB)aw_gc_changed_cb, (AW_CL)acbs, (AW_CL)0);
                font_size_awar->add_callback((AW_RCB)aw_gc_changed_cb, (AW_CL)acbs, (AW_CL)0);
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


AW_window *AW_preset_window(AW_root *root) {
    GTK_PARTLY_IMPLEMENTED;
    AW_window_simple *aws = new AW_window_simple;
    const int   tabstop = 400;
    aws->init(root, "PROPS_FRAME", "WINDOW_PROPERTIES");

    aws->label_length(25);
    aws->button_length(20);

    aws->at           (10, 10);
    aws->auto_space(10, 10);

    aws->callback     (AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"props_frame.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at_newline();

    aws->create_font_button("window/font", "Main Menu Font");
    aws->at_x(tabstop);
    aws->create_input_field("window/font", 12);

    aws->at_newline();

    aws->button_length(10);
    aws->create_color_button("window/background", "Application Background");
    aws->at_x(tabstop);
    aws->create_input_field("window/background", 12);

    aws->at_newline();

    aws->create_color_button("window/foreground", "Application Foreground");
    aws->at_x(tabstop);
    aws->create_input_field("window/foreground", 12);

    aws->at_newline();

    aws->create_color_button("window/color_1", "Color 1");
    aws->at_x(tabstop);
    aws->create_input_field("window/color_1", 12);

    aws->at_newline();

    aws->create_color_button("window/color_2", "Color 2");
    aws->at_x(tabstop);
    aws->create_input_field("window/color_2", 12);

    aws->at_newline();

    aws->create_color_button("window/color_3", "Color 3");
    //AW_preset_create_color_chooser(aws, "window/color_3", "Color 3", true, true);
    aws->at_x(tabstop);
    aws->create_input_field("window/color_3", 12);
    
    aws->at_newline();

    aws->window_fit();
    return (AW_window *)aws;
}

AW_window *AW_create_gc_window(AW_root *aw_root, AW_gc_manager id_par) {
    return AW_create_gc_window_named(aw_root, id_par, "PROPS_GC", "Colors and Fonts");
}


AW_window *AW_create_gc_window_named(AW_root *aw_root, AW_gc_manager id_par, const char *wid, 
                                     const char *windowname){
    AW_window_simple * aws = new AW_window_simple;

    aws->init(aw_root, wid, windowname);

    aw_gc_manager *gcmgr = (aw_gc_manager *)id_par;

    aws->at(10, 10);
    aws->auto_space(5, 5);

    aws->callback((AW_CB0) AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"color_props.hlp");
    aws->create_button("HELP", "HELP", "H");

    aws->at_newline();

    bool has_color_groups = false;//aw_insert_gcs(aw_root, aws, gcmgr, false);

    if (has_color_groups) {
        // aws->callback((AW_CB)AW_create_gc_color_groups_window, (AW_CL)aw_root, (AW_CL)id_par);
        aws->create_autosize_button("EDIT_COLOR_GROUP", "Edit color groups", "E");
        aws->at_newline();
    }

    aws->window_fit();
    return (AW_window *) aws;
}





