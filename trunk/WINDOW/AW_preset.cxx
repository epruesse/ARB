
//  ----------------------------------------------------
//      This file is located in WINDOW/AW_preset.cxx
//      (AWT/AWT_preset.cxx is just a link)
//  ----------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_color_groups.hxx>
// PJ vectorfont stuff:
//#include <aw_xfig.hxx>
//#include <aw_xfigfont.hxx>

// #ifdef _ARB_AWT
#include "awt.hxx"
#include "awt_advice.hxx"
// #endif

#include <stdarg.h>
#include <stdlib.h>
#include <awt_canvas.hxx>
#include "aw_preset.hxx"
#include "aw_def.hxx" 

#ifdef _ARB_WINDOW

void AW_save_defaults( AW_window *aw ) {
    aw->get_root()->save_default(  "window/font" );
}

void AW_save_specific_defaults( AW_window *aw, const char *filename) { // special version for EDIT4
    aw->get_root()->save_default(  "window/font", filename);
}

void aw_message_reload(AW_root *){
    aw_message( "Sorry, to activate new colors:\n"
                "   save properties\n"
                "   and restart application");
}
char *aw_glob_font_awar_name = 0;

void aw_set_color(AW_window *aww,const char *color_name){
    aww->get_root()->awar(aw_glob_font_awar_name)->write_string(color_name);
}

// --------------------------------------------------------------------------------
//     static int hex2dez(char c)
// --------------------------------------------------------------------------------
static int hex2dez(char c) {
    if (c>='0' && c<='9') return c-'0';
    if (c>='A' && c<='F') return c-'A'+10;
    if (c>='a' && c<='f') return c-'a'+10;
    return -1;
}

// --------------------------------------------------------------------------------
//     void aw_incdec_color(AW_window *aww,const char *action)
// --------------------------------------------------------------------------------
void aw_incdec_color(AW_window *aww,const char *action){
    // action is sth like "r+" "b-" "g++" "r--"
    AW_awar *awar = aww->get_root()->awar(aw_glob_font_awar_name);
    char *color = awar->read_string();
    AW_BOOL err = AW_TRUE;

    fprintf(stderr, "current color is '%s'\n", color);

    if (color[0]=='#') {
        int len = strlen(color);
        if (len==4 || len==7) {
            len = (len-1)/3; // len of one color channel (1 or 2)
            gb_assert(len==1 || len==2);

            int diff = action[2]==action[1] ? 7 : 1;

            int channel[3];
            for (int c=0; c<3; ++c) {
                if (len==2) channel[c] = hex2dez(color[c*len+1])*16+hex2dez(color[c*len+2]);
                else        channel[c] = hex2dez(color[c*len+1])*16;
            }

            int rgb;
            for (rgb=0; rgb<3;++rgb) {
                if (action[0]=="rgb"[rgb] || action[0]=='a') {
                    if (action[1]=='+') { channel[rgb] += diff; if (channel[rgb]>255) channel[rgb]=255; }
                    else                { channel[rgb] -= diff; if (channel[rgb]<0)   channel[rgb]=0; }
                }
            }

            sprintf(color, "#%2.2X%2.2X%2.2X", channel[0], channel[1], channel[2]);

            err = AW_FALSE;
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

void aw_create_color_chooser_window(AW_window *aww, const char *awar_name,const char *label_name){
    AW_root *awr = aww->get_root();
    static AW_window_simple *aws = 0;
    if (!aws){
        int x1, y1, x2, y2;

        awr->awar_string(AWAR_GLOBAL_COLOR_NAME);
        aws = new AW_window_simple;
        aws->init(awr, "COLORS", "COLORS");
        aws->at(10, 10);
        aws->auto_space(3, 3);
        aws->callback     ( AW_POPDOWN );
        aws->create_button( "CLOSE","CLOSE", "C" );
        aws->get_at_position(&x1, &y1);
        aws->at_newline();

        aws->button_length(20);
        aws->create_button( "LABEL",AWAR_GLOBAL_COLOR_NAME, "A" );
        aws->get_at_position(&x2, &y2);
        aws->at_newline();

        x1 = x1>x2 ? x1 : x2;

        int red,green,blue;

        for (int minus = 0; minus<=1; ++minus) {
            aws->at(x1, minus==0 ? y1 : y2);
            for (int rgb=0; rgb<4; ++rgb) {
                for (int big=0; big<=1; ++big) {
                    aws->button_length(2+big);

                    char action[4] = "xxx";
                    action[0] = "rgba"[rgb];
                    action[1] = "+-"[minus];
                    action[2] = big ? action[1] : 0;

                    char color_name[10];
                    sprintf(color_name, "#%2.2X%2.2X%2.2X", rgb==0 ? 0xff : 0x55, rgb==1 ? 0xff : 0x55, rgb==2 ? 0xff : 0x55);
                    aws->set_background(color_name);
                    aws->callback((AW_CB1)aw_incdec_color, (AW_CL)strdup(action));
                    aws->create_button(action, action+1);
                }
            }
        }

        aws->button_length(2);
        aws->at_newline();

        for (red = 0; red <= 255; red += 255/3){
            for (green = 0; green <= 255; green += 255/3){
                for (blue = 0; blue <= 255; blue += 255/3){
                    char color_name[256];
                    sprintf(color_name,"#%2.2X%2.2X%2.2X", red, green,blue);
                    aws->set_background(color_name);
                    aws->callback((AW_CB1)aw_set_color,(AW_CL)strdup(color_name));
                    aws->create_button(color_name,"=");
                }
            }
            aws->at_newline();
        }
        for (red = 0; red <= 256; red += 256/16){ // grey buttons
            char color_name[256];
            sprintf(color_name,"#%2.2X%2.2X%2.2X", (red==256)?255:red, (red>=256)?255:red,(red>=256)?255:red);
            aws->set_background(color_name);
            aws->callback((AW_CB1)aw_set_color,(AW_CL)strdup(color_name));
            aws->create_button(color_name,"=");
        }
        aws->at_newline();

        aws->window_fit();
    }
    awr->awar(AWAR_GLOBAL_COLOR_NAME)->write_string(label_name);
    free(aw_glob_font_awar_name);
    aw_glob_font_awar_name = strdup(awar_name);
    aws->show();
}

void AW_preset_create_color_chooser(AW_window *aws, const char *awar, const char *label,bool message_reload, bool show_label)
{
    if (message_reload) aws->get_root()->awar(awar)->add_callback(aw_message_reload);
    if (show_label) {
        aw_assert(label);
        aws->label(label);
    }
    aws->callback((AW_CB)aw_create_color_chooser_window,(AW_CL)strdup(awar),(AW_CL)strdup(label));
    aws->set_background(aws->get_root()->awar(awar)->read_string());
    aws->create_button("SELECT_A_COLOR"," ");
}

void AW_preset_create_font_chooser(AW_window *aws, const char *awar, const char *label,bool message_reload)
{
    if (message_reload){
        aws->get_root()->awar(awar)->add_callback(aw_message_reload);
    }

    aws->create_option_menu( awar, label );
    aws->insert_option        ( "5x8",  "5", "5x8" );
    aws->insert_option        ( "6x10",  "6", "6x10" );
    aws->insert_option        ( "7x13",  "7", "7x13" );
    aws->insert_option        ( "7x13bold",  "7", "7x13bold" );
    aws->insert_option        ( "8x13",  "8", "8x13" );
    aws->insert_option        ( "8x13bold",  "8", "8x13bold" );
    aws->insert_option        ( "9x15",  "9", "9x15" );
    aws->insert_option        ( "9x15bold",  "9", "9x15bold" );
    aws->insert_option        ( "helvetica-12",  "9", "helvetica-12" );
    aws->insert_option        ( "helvetica-bold-12",  "9", "helvetica-bold-12" );
    aws->insert_option        ( "helvetica-13",  "9", "helvetica-13" );
    aws->insert_option        ( "helvetica-bold-13",  "9", "helvetica-bold-13" );
    aws->insert_default_option( "other", "o", ""     );
    aws->update_option_menu();
}

//PJ - vectorfont stuff - user may set a factor for global scaling
void AW_preset_create_scale_chooser(AW_window *aws, char *awar, char *label)
{
    char buffer[2]; buffer[0] = label[0]; buffer[1] = 0;
    aws->create_option_menu( awar, label, buffer );
    aws->insert_option        ( "0.5",   " ", (float) 0.5);
    aws->insert_option        ( "0.8",   "0", (float) 0.8);
    aws->insert_option        ( "1.0",   "1", (float) 1.0);
    aws->insert_default_option( "other", "o", (float) 1.0 );
    aws->update_option_menu();
}

struct AW_MGC_awar_cb_struct;

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

struct AW_MGC_cb_struct {   // one for each window
    AW_MGC_cb_struct(AW_window *awi, void (*g)(AW_window*,AW_CL ,AW_CL), AW_CL cd1i, AW_CL cd2i);

    AW_window *aw;
    void (*f)(AW_window*,AW_CL ,AW_CL);
    AW_CL      cd1;
    AW_CL      cd2;
    char      *window_awar_name;
    AW_device *device;

    struct AW_MGC_awar_cb_struct *next_drag;
};

AW_MGC_cb_struct::AW_MGC_cb_struct( AW_window *awi, void (*g)(AW_window*,AW_CL,AW_CL), AW_CL cd1i, AW_CL cd2i ) {
    memset((char*)this,0,sizeof(AW_MGC_cb_struct));
    aw  = awi;
    f   = g;
    cd1 = cd1i;
    cd2 = cd2i;

    window_awar_name = strdup(awi->get_window_id());
}

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
        sprintf(awar_name,AWP_FONTNAME_TEMPLATE,cbs->cbs->window_awar_name,cbs->fontbasename);

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

static void aw_gc_changed_cb(AW_root *awr,AW_MGC_awar_cb_struct *cbs, long mode)
{
    static int dont_recurse = 0;

    if (dont_recurse == 0) {
        ++dont_recurse;
        // mode == -1 -> no callback
        char awar_name[256];
        int  font;
        int  size;

        sprintf(awar_name,AWP_FONTNAME_TEMPLATE,cbs->cbs->window_awar_name,cbs->fontbasename);
        font = awr->awar(awar_name)->read_int();

        sprintf(awar_name,AWP_FONTSIZE_TEMPLATE,cbs->cbs->window_awar_name,cbs->fontbasename);
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

void aw_gc_color_changed_cb(AW_root *root,AW_MGC_awar_cb_struct *cbs, long mode)
{
    char awar_name[256];
    char *colorname;

    sprintf(awar_name,AWP_COLORNAME_TEMPLATE,cbs->cbs->window_awar_name,cbs->colorbasename);
    colorname = root->awar(awar_name)->read_string();
    AW_color color = (AW_color)cbs->colorindex;
    cbs->cbs->aw->alloc_named_data_color(color,colorname);
    if (color != AW_DATA_BG) {
        cbs->cbs->device->set_foreground_color(cbs->gc,color);
        cbs->cbs->device->set_foreground_color(cbs->gc_drag,color);
    }else{
        struct AW_MGC_awar_cb_struct *acbs;
        for (acbs = cbs->cbs->next_drag; acbs; acbs=acbs->next){
            cbs->cbs->device->set_foreground_color(acbs->gc_drag,(AW_color)acbs->colorindex);
        }
    }
    if (mode != -1) {
        cbs->cbs->f(cbs->cbs->aw,cbs->cbs->cd1,cbs->cbs->cd2);
    }
    free(colorname);
}

static bool color_groups_initialized = false;
static bool use_color_groups = false;

const char *AW_get_color_group_name_awarname(int color_group) {
    if (color_group>0 && color_group <= AW_COLOR_GROUPS) {
        static char buf[sizeof(AWAR_COLOR_GROUPS_PREFIX)+1+4+2+1];
        sprintf(buf, AWAR_COLOR_GROUPS_PREFIX "/name%i", color_group);
        return buf;
    }
    return 0;
}

char *AW_get_color_group_name(AW_root *awr, int color_group) {
    aw_assert(color_groups_initialized);
    aw_assert(color_group>0 && color_group <= AW_COLOR_GROUPS);
    return awr->awar(AW_get_color_group_name_awarname(color_group))->read_string();
}

void AW_color_group_name_changed_cb(AW_root *) {
    AWT_advice("To activate the new names for color groups you have to\n"
               "save properties and restart the program.",
               AWT_ADVICE_TOGGLE, "Color group name has been changed", 0);
}
void AW_color_group_usage_changed_cb(AW_root *awr, AW_CL /*cl_ntw*/) {
    use_color_groups       = awr->awar(AWAR_COLOR_GROUPS_USE)->read_int();
    //     AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    //     ntw->refresh();
    // @@@ FIXME: a working refresh is missing
}

void AW_init_color_groups(AW_root *awr, AW_default def) {
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

static const char **color_group_gc_defaults = 0;

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


long AW_find_color_group(GBDATA *gbd, AW_BOOL ignore_usage_flag) {
    /* species/genes etc. may have a color group entry ('ARB_color')
     * call with ignore_usage_flag == AW_TRUE to read color group regardless of global usage flag (AWAR_COLOR_GROUPS_USE)
     */
    aw_assert(color_groups_initialized);
    if (!use_color_groups && !ignore_usage_flag) return 0;

    GBDATA *gb_group = GB_find(gbd, AW_COLOR_GROUP_ENTRY, 0, down_level);
    if (gb_group) return GB_read_int(gb_group);
    return 0;                   /* no color group */
}

GB_ERROR AW_set_color_group(GBDATA *gbd, long color_group) {
    GB_ERROR  error    = 0;
    GBDATA   *gb_group = GB_find(gbd, AW_COLOR_GROUP_ENTRY, 0, down_level);

    if (!gb_group) gb_group = GB_create(gbd, AW_COLOR_GROUP_ENTRY, GB_INT);

    if (gb_group)   error = GB_write_int(gb_group, color_group);
    else            error = GB_get_error();
    return error;
}

AW_gc_manager AW_manage_GC(AW_window   *aww,
                           AW_device   *device,
                           int          base_gc,
                           int          base_drag,
                           AW_GCM_AREA  area,
                           void (*changecb)(AW_window*,AW_CL,AW_CL),
                           AW_CL        cd1,
                           AW_CL        cd2,
                           bool         define_color_groups,
                           const char  *default_background_color,
                           ...)
{
    /*
     *  Parameter:  aww:    base window
     *          device: screen device
     *          base_gc:    first gc number
     *          base_drag:  one after last gc
     *          area:       middle,top ...
     *          changecb:   cb if changed
     *          cd1,cd2:    free Parameters to changecb
     *          define_color_groups: true -> add colors for color groups
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
     */

    AW_root    *aw_root = aww->get_root();
    AW_default  aw_def  = AW_ROOT_DEFAULT;

    AW_init_color_groups(aw_root, aw_def);

    const char           *id;
    va_list               parg;
    va_start(parg,default_background_color);
    AW_font               def_font;
    struct aw_gc_manager *gcmgrlast = 0,*gcmgr2=0,*gcmgrfirst=0;

    AW_MGC_cb_struct *mcbs = new AW_MGC_cb_struct(aww,changecb,cd1,cd2);
    mcbs->device = device;

    int col = AW_WINDOW_BG;
    if (area == AW_GCM_DATA_AREA) {
        col = AW_DATA_BG;
    }
    AW_BOOL first = AW_TRUE;

    aww->main_drag_gc = base_drag;
    gcmgrfirst = gcmgrlast = new aw_gc_manager(mcbs->window_awar_name, 0);

    const char *old_font_base_name = "default";

    char background[50];
    gb_assert(default_background_color[0]);

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
            AW_BOOL flag_fixed_fonts_only    = AW_FALSE;
            AW_BOOL flag_no_color_selector   = AW_FALSE;
            AW_BOOL flag_append_in_same_line = AW_FALSE;
            AW_BOOL flag_no_fonts            = AW_FALSE;

            AW_MGC_awar_cb_struct *acbs = 0;
            {
                char * id_copy = strdup(id);
                char *color = strchr(id_copy, '$'); // search color def
                if (color) {
                    *color++ = 0;
                }
                else {
                    if (first) color = (char*)"white";
                    else color = (char*)"navy";
                }

                gcmgr2 = new aw_gc_manager(strdup(id_copy), color ? strdup(color) : 0);

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

                int offset = 0;
                while (1){
                    switch( id_copy[offset] ){
                        case '#':   flag_fixed_fonts_only = AW_TRUE;        offset++; continue;
                        case '=':   flag_no_color_selector = AW_TRUE;       offset++; continue;
                        case '+':   flag_append_in_same_line = AW_TRUE;     offset++; continue;
                        case '-':   flag_no_fonts = AW_TRUE;                offset++; continue;
                        default:    break;
                    }
                    break;
                }

                free(id_copy); id_copy = 0;
            }

            if (flag_fixed_fonts_only) def_font = AW_DEFAULT_FIXED_FONT; // AW_LUCIDA_SANS_TYPEWRITER;
            else def_font                       = AW_DEFAULT_NORMAL_FONT; // AW_LUCIDA_SANS;

            if ((area != AW_GCM_DATA_AREA) || !first) {
                device->new_gc(base_gc);
                device->set_line_attributes(base_gc,0.0,AW_SOLID);
                device->set_function(base_gc,AW_COPY);

                device->new_gc(base_drag);
                device->set_line_attributes(base_drag,0.0,AW_SOLID);
                device->set_function(base_drag,AW_XOR);
            }

            char awar_name[256]; memset(awar_name,0,256);
            sprintf(awar_name,AWP_COLORNAME_TEMPLATE,mcbs->window_awar_name,acbs->colorbasename);
            acbs->colorindex = col;

            aw_root->awar_string(awar_name, gcmgr2->get_default_value(), aw_def);
            aw_root->awar(awar_name)->add_callback( (AW_RCB)aw_gc_color_changed_cb,(AW_CL)acbs,(AW_CL)0);

            aw_gc_color_changed_cb(aw_root,acbs,-1);

            if (flag_no_fonts) acbs->fontbasename = old_font_base_name;
            else old_font_base_name = acbs->fontbasename = acbs->colorbasename;

            {
                sprintf(awar_name,AWP_FONTNAME_TEMPLATE,mcbs->window_awar_name,acbs->fontbasename);
                AW_awar *font_awar = aw_root->awar_int(awar_name,def_font,aw_def);
                sprintf(awar_name,AWP_FONTSIZE_TEMPLATE,   mcbs->window_awar_name,acbs->fontbasename);
                AW_awar *font_size_awar = aw_root->awar_int(awar_name,DEF_FONTSIZE,aw_def);

                if (!flag_no_fonts) {
                    font_awar->add_callback(aw_font_changed_cb, (AW_CL)acbs);
                    gcmgr2->set_font_change_parameter(acbs);
                }

                font_awar->add_callback((AW_RCB)aw_gc_changed_cb,(AW_CL)acbs,(AW_CL)0);
                font_size_awar->add_callback((AW_RCB)aw_gc_changed_cb,(AW_CL)acbs,(AW_CL)0);
            }

            if (!first) {
                aw_gc_changed_cb(aw_root,acbs,-1);
                base_gc++;
                base_drag++;
            }
            col++;
            first = AW_FALSE;

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

void AW_copy_GCs(AW_root *aw_root, const char *source_window, const char *dest_window, AW_BOOL has_font_info, const char *id0, ...) {
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

static bool aw_insert_gcs(AW_root *aw_root, AW_window_simple *aws, aw_gc_manager *gcmgr, bool insert_color_groups) {
    // returns true if GCs starting with COLOR_GROUP_PREFIX were found

    bool        has_color_groups = false;
    const char *window_awar_name = gcmgr->get_field();
    AW_BOOL     first            = AW_TRUE;

    for (gcmgr = gcmgr->get_next(); gcmgr; gcmgr = gcmgr->get_next()) {
        const char *id = gcmgr->get_field();

        AW_BOOL flag_fixed_fonts_only    = AW_FALSE;
        AW_BOOL flag_no_color_selector   = AW_FALSE;
        AW_BOOL flag_append_in_same_line = AW_FALSE;
        AW_BOOL flag_no_fonts            = AW_FALSE;
        AW_BOOL flag_hide_this_gc        = AW_FALSE;

        while (1){
            switch( id[0] ){
                case '#':   flag_fixed_fonts_only = AW_TRUE;    id++; continue;
                case '=':   flag_no_color_selector = AW_TRUE;   id++; continue;
                case '+':   flag_append_in_same_line = AW_TRUE;     id++; continue;
                case '-':   {
                    if (flag_no_fonts) flag_hide_this_gc = AW_TRUE; // if gc definition contains -- the gc is completely hidden
                    else flag_no_fonts = AW_TRUE;
                    id++;
                    continue;
                }
                default:    break;
            }
            break;
        }

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

        if (!flag_hide_this_gc) {
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

            if (!flag_no_color_selector){
                aws->button_length(5);
                AW_preset_create_color_chooser(aws, awar_name, id);
            }
            aws->create_input_field(awar_name, 7);

            if (!flag_no_fonts) {
                sprintf(awar_name, AWP_FONTNAME_TEMPLATE, window_awar_name, fontbasename);

                aws->label_length(5);
                aws->create_option_menu(awar_name, "Font", 0);
                {
                    int font_nr;
                    const char *font_string;

                    for (font_nr = 0;; font_nr++) {
                        font_string = aw_root->font_2_ascii((AW_font) font_nr);
                        if (!font_string) break;
                        if (flag_fixed_fonts_only && aw_root->font_2_xfig((AW_font) font_nr) >= 0) continue;
                        aws->insert_option(font_string, 0, (int) font_nr);
                    }
                    aws->update_option_menu();
                }

                sprintf(awar_name, AWP_FONTSIZE_TEMPLATE,window_awar_name, fontbasename);
                aws->label_length(5);

                AW_option_menu_struct *oms = aws->create_option_menu(awar_name, "size", 0);
                gcmgr->set_font_size_handle(oms);

                AW_MGC_awar_cb_struct *acs = gcmgr->get_font_change_parameter();
                acs->gc_def_window         = aws; 

                aw_init_font_sizes(aw_root, acs, true); // does update_option_menu
            }
            if (!flag_append_in_same_line)  aws->at_newline();
        }
        first = AW_FALSE;
        free(fontbasename);
    }

    return has_color_groups;
}

struct attached_window {
    AW_window_simple       *aws;
    AW_CL                   attached_to;
    struct attached_window *next;
};

void AW_create_gc_color_groups_name_window(AW_window */*aww*/, AW_CL cl_aw_root, AW_CL cl_gcmgr) {
    AW_root       *aw_root = (AW_root*)cl_aw_root;
//     aw_gc_manager *gcmgr   = (aw_gc_manager*)cl_gcmgr;

    static struct attached_window *head = 0;

    // search for attached window:

    struct attached_window *look = head;
    while (look) {
        if (look->attached_to == cl_gcmgr) break;
        look = look->next;
    }

    AW_window_simple *aws = 0;

    if (look) {
        aws = look->aws;
    }
    else {
        look = new struct attached_window;

        look->aws         = new AW_window_simple;
        look->attached_to = cl_gcmgr;
        look->next        = head;
        head              = look;

        aws = look->aws;

        aws->init(aw_root, "NAME_COLOR_GROUPS", "COLORS GROUP NAMES");

        aws->at(10, 10);
        aws->auto_space(5, 5);

        aws->callback((AW_CB0) AW_POPDOWN);
        aws->create_button("CLOSE","CLOSE", "C");

        for (int i = 1; i <= AW_COLOR_GROUPS; ++i) {
            aws->at_newline();

            aws->label(GBS_global_string("Name for color group #%i%s", i, (i >= 10) ? "" : " "));
            aws->create_input_field(AW_get_color_group_name_awarname(i), AW_COLOR_GROUP_NAME_LEN);
        }

        aws->window_fit();
    }

    aws->show();
}

//  ------------------------------------------------------------------------------------------------
//      void AW_create_gc_color_groups_window(AW_window *aww, AW_CL cl_aw_root, AW_CL cl_gcmgr)
//  ------------------------------------------------------------------------------------------------
void AW_create_gc_color_groups_window(AW_window */*aww*/, AW_CL cl_aw_root, AW_CL cl_gcmgr) {
    aw_assert(color_groups_initialized);

    AW_root       *aw_root = (AW_root*)cl_aw_root;
    aw_gc_manager *gcmgr   = (aw_gc_manager*)cl_gcmgr;

    static struct attached_window *head = 0;

    // search for attached window:

    struct attached_window *look = head;
    while (look) {
        if (look->attached_to == cl_gcmgr) break;
        look = look->next;
    }

    AW_window_simple *aws = 0;

    if (look) {
        aws = look->aws;
    }
    else {
        look = new struct attached_window;

        look->aws         = new AW_window_simple;
        look->attached_to = cl_gcmgr;
        look->next        = head;
        head              = look;

        aws = look->aws;

        aws->init(aw_root, "PROPS_COLOR_GROUPS", "COLORS GROUPS");

        aws->at(10, 10);
        aws->auto_space(5, 5);

        aws->callback((AW_CB0) AW_POPDOWN);
        aws->create_button("CLOSE","CLOSE", "C");

        aws->callback(AW_POPUP_HELP,(AW_CL)"color_props_groups.hlp");
        aws->create_button("HELP","HELP", "H");

        aws->at_newline();

        aw_insert_gcs(aw_root, aws, gcmgr, true);

        aws->at_newline();

        aws->label_length(16);
        aws->label("Use color groups");
        aws->create_toggle(AWAR_COLOR_GROUPS_USE);

        aws->callback((AW_CB)AW_create_gc_color_groups_name_window, (AW_CL)aw_root, cl_gcmgr);
        aws->create_autosize_button("DEF_NAMES", "Define names", "D");

        aws->window_fit();
    }

    aws->show();
}

//  -------------------------------------------------------------------------------
//      AW_window *AW_create_gc_window(AW_root * aw_root, AW_gc_manager id_par)
//  -------------------------------------------------------------------------------
AW_window *AW_create_gc_window(AW_root * aw_root, AW_gc_manager id_par)
{
    //crazy simple function
    // The window has already been created
    AW_window_simple * aws = new AW_window_simple;
    aws->init(aw_root, "PROPS_GC", "COLORS AND FONTS");

    aw_gc_manager *gcmgr = (aw_gc_manager *)id_par;

    aws->at(10, 10);
    aws->auto_space(5, 5);

    aws->callback((AW_CB0) AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE", "C");

    aws->callback(AW_POPUP_HELP,(AW_CL)"color_props.hlp");
    aws->create_button("HELP","HELP", "H");

    aws->at_newline();

    bool has_color_groups = aw_insert_gcs(aw_root, aws, gcmgr, false);

    if (has_color_groups) {
        aws->callback((AW_CB)AW_create_gc_color_groups_window, (AW_CL)aw_root, (AW_CL)id_par);
        aws->create_autosize_button("EDIT_COLOR_GROUP","Edit color groups", "E");
        aws->at_newline();
    }

    aws->window_fit();
    return (AW_window *) aws;
}
#endif //ifdef _ARB_WINDOW


#ifdef _ARB_AWT
// callback to reset to default font
void awt_xfig_font_resetfont_cb(AW_window *aws){
    AW_root *aw_root = aws->get_root();
    aw_root->awar("vectorfont/file_name")->write_string("lib/pictures/fontgfx.vfont");
    //AW_POPDOWN(aws);
}

void awt_xfig_font_create_filerequest(AW_window *aw) {
    static AW_window_simple *aws = 0;
    AW_root *aw_root = aw->get_root();

    if (!aws) {
        // called *ONCE* to open the window
        aws = new AW_window_simple;
        aws->init( aw_root, "SELECT_VECTORFONT", "Select Xfig Vectorfont Resource");
        aws->load_xfig("fontgfx_request.fig");
        aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE","CLOSE","C");
        aws->at("reset");aws->callback((AW_CB0)awt_xfig_font_resetfont_cb);
        aws->create_button("RESET","RESET","R");
        // AWT_sel_boxes.cxx
        awt_create_selection_box((AW_window *)aws,"vectorfont","","ARBHOME");
    }
    aw_root->awar("vectorfont/file_name")->write_string(aw_root->vectorfont_name);
    aws->show();
}
AW_window *AWT_preset_window( AW_root *root )
#else
    AW_window *AW_preset_window( AW_root *root )
#endif
{
    AW_window_simple *aws = new AW_window_simple;
    const int   tabstop = 400;
    aws->init( root, "PROPS_FRAME", "WINDOW_PROPERTIES");

    aws->label_length( 25 );
    aws->button_length( 20 );

    aws->at           ( 10,10 );
    aws->auto_space(10,10);

    aws->callback     ( AW_POPDOWN );
    aws->create_button( "CLOSE","CLOSE", "C" );

    aws->callback(AW_POPUP_HELP,(AW_CL)"props_frame.hlp");
    aws->create_button("HELP","HELP", "H");

    aws->at_newline();

    //PJ vectorfont stuff
    aws->label("Vectorfont Resource");
#ifdef _ARB_AWT
    aws->callback( (AW_CB0) awt_xfig_font_create_filerequest);
#endif
    aws->create_button( "SELECT VECTORFONT", "Vectorfont Select", "V" );
    aws->at_x(tabstop);
    aws->create_input_field( "vectorfont/file_name",20);
    aws->at_newline();

    AW_preset_create_font_chooser(aws,"window/font", "Main Menu Font", 1);
    aws->at_x(tabstop);
    aws->create_input_field( "window/font", 12 );
    aws->at_newline();

    aws->button_length(10);
    AW_preset_create_color_chooser(aws,"window/background", "Application Background", true, true);
    aws->at_x(tabstop);
    aws->create_input_field( "window/background", 12 );
    aws->at_newline();

    AW_preset_create_color_chooser(aws,"window/foreground", "Application Foreground", true, true );
    aws->at_x(tabstop);
    aws->create_input_field( "window/foreground", 12 );
    aws->at_newline();


    AW_preset_create_color_chooser(aws,"window/color_1", "Color 1", true, true );
    aws->at_x(tabstop);
    aws->create_input_field( "window/color_1", 12 );
    aws->at_newline();

    AW_preset_create_color_chooser(aws,"window/color_2", "Color 2", true, true );
    aws->at_x(tabstop);
    aws->create_input_field( "window/color_2", 12 );
    aws->at_newline();

    AW_preset_create_color_chooser(aws,"window/color_3", "Color 3", true, true );
    aws->at_x(tabstop);
    aws->create_input_field( "window/color_3", 12 );
    aws->at_newline();


    aws->window_fit();
    return (AW_window *)aws;

}
