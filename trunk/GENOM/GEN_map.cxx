/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <aw_question.hxx>
#include <awt_canvas.hxx>
#include <awt.hxx>
#include <AW_rename.hxx>
#include <awt_input_mask.hxx>

#include "GEN_local.hxx"
#include "GEN_gene.hxx"
#include "GEN_graphic.hxx"
#include "GEN_nds.hxx"
#include "GEN_interface.hxx"
#include "EXP.hxx"
#include "EXP_interface.hxx"
#include "EXP_local.hxx"
#include "../NTREE/ad_spec.hxx" // needed for species query window

using namespace std;

extern GBDATA *gb_main;

// -----------------------------
//      class GEN_map_window
// -----------------------------
class GEN_map_window: public AW_window_menu_modes {
    int          window_nr;
    GEN_graphic *gen_graphic;
    AWT_canvas  *gen_canvas;

    GEN_map_window(const GEN_map_window& other); // copying not allowed
    GEN_map_window& operator = (const GEN_map_window& other); // assignment not allowed
public:
    GEN_map_window(int window_nr_)
        : AW_window_menu_modes()
        , window_nr(window_nr_)
        , gen_graphic(0)
        , gen_canvas(0)
    { }

    void init(AW_root *root);

    GEN_graphic *get_graphic() const { gen_assert(gen_graphic); return gen_graphic; }
    AWT_canvas *get_canvas() const { gen_assert(gen_canvas); return gen_canvas; }
    int get_nr() const { return window_nr; }
};

// -------------------------------
//      class GEN_map_manager
// -------------------------------

class GEN_map_manager {
    static AW_root         *aw_root;
    static GEN_map_manager *the_manager; // there can be only one!

    int              window_counter;
    GEN_map_window **windows;   // array of managed windows
    int              windows_size; // size of 'windows' array

    GEN_map_manager(const GEN_map_manager& other); // copying not allowed
    GEN_map_manager& operator = (const GEN_map_manager& other); // assignment not allowed

public:

    GEN_map_manager();

    static bool initialized() { return aw_root != 0; }
    static void initialize(AW_root *aw_root_) { aw_root = aw_root_; }
    static GEN_map_manager *get_map_manager();

    GEN_map_window *get_map_window(int nr);

    int no_of_managed_windows() { return window_counter; }

    typedef void (*GMW_CB2)(GEN_map_window*, AW_CL, AW_CL);
    typedef void (*GMW_CB1)(GEN_map_window*, AW_CL);
    typedef void (*GMW_CB0)(GEN_map_window*);

    static void with_all_mapped_windows(GMW_CB2 callback, AW_CL cd1, AW_CL cd2);
    static void with_all_mapped_windows(GMW_CB1 callback, AW_CL cd) { with_all_mapped_windows((GMW_CB2)callback, cd, 0); }
    static void with_all_mapped_windows(GMW_CB0 callback) { with_all_mapped_windows((GMW_CB2)callback, 0, 0); }
};

// ____________________________________________________________
// start of implementation of class GEN_map_manager:

AW_root         *GEN_map_manager::aw_root     = 0;
GEN_map_manager *GEN_map_manager::the_manager = 0;

GEN_map_manager::GEN_map_manager()
    : window_counter(0)
    , windows(new GEN_map_window *[5])
    , windows_size(5)
{
    gen_assert(!the_manager);   // only one instance allowed
    the_manager = this;
}

GEN_map_manager *GEN_map_manager::get_map_manager() {
    if (!the_manager) {
        gen_assert(aw_root);    // call initialize() before!
        new GEN_map_manager();  // sets the manager
        gen_assert(the_manager);
    }
    return the_manager;
}

void GEN_map_manager::with_all_mapped_windows(GMW_CB2 callback, AW_CL cd1, AW_CL cd2) {
    GEN_map_manager *mm       = get_map_manager();
    int              winCount = mm->no_of_managed_windows();
    for (int nr = 0; nr<winCount; ++nr) {
        callback(mm->get_map_window(nr), cd1, cd2);
    }
}

GEN_map_window *GEN_map_manager::get_map_window(int nr)
{
    gen_assert(aw_root);
    gen_assert(nr >= 0);
    if (nr<window_counter) {
        return windows[nr]; // window has already been created before
    }

    gen_assert(nr == window_counter); // increase nr sequentially!

    window_counter++;
    if (window_counter>windows_size) {
        int             new_windows_size = windows_size+5;
        GEN_map_window **new_windows      = new GEN_map_window*[new_windows_size];

        for (int i = 0; i<windows_size; ++i) new_windows[i] = windows[i];

        delete [] windows;
        windows      = new_windows;
        windows_size = new_windows_size;
    }

    gen_assert(nr<windows_size);

    windows[nr] = new GEN_map_window(nr);
    windows[nr]->init(aw_root);
    return windows[nr];
}

// -end- of implementation of class GEN_map_manager.



const char *GEN_window_local_awar_name(const char *awar_name, int window_nr) {
    return GBS_global_string("%s_%i", awar_name, window_nr);
}

static void reinit_NDS_4_window(GEN_map_window *win) {
    win->get_graphic()->get_gen_root()->reinit_NDS();
}

static void GEN_NDS_changed(GBDATA *, int *, gb_call_back_type) {
    GEN_make_node_text_init(gb_main);
    GEN_map_manager::with_all_mapped_windows(reinit_NDS_4_window);

    // GEN_GRAPHIC->gen_root->reinit_NDS();
}

struct gene_container_changed_cb_data {
    AWT_canvas  *canvas;
    GEN_graphic *graphic;
    GBDATA      *gb_gene_data; // callback was installed for this gene_data

    gene_container_changed_cb_data() : canvas(0), graphic(0), gb_gene_data(0) {}
    gene_container_changed_cb_data(AWT_canvas *canvas_, GEN_graphic *graphic_, GBDATA *gb_gene_data_)
        : canvas(canvas_)
        , graphic(graphic_)
        , gb_gene_data(gb_gene_data_)
    {}
};

static void GEN_gene_container_changed_cb(GBDATA */*gb_gene_data*/, int *cl_cb_data, GB_CB_TYPE /*gb_type*/) {
    gene_container_changed_cb_data *cb_data = (gene_container_changed_cb_data*)cl_cb_data;
    cb_data->graphic->reinit_gen_root(cb_data->canvas, true);
    cb_data->canvas->refresh();
}

static void GEN_gene_container_cb_installer(bool install, AWT_canvas *gmw, GEN_graphic *gg) {
    typedef map<GEN_graphic*, gene_container_changed_cb_data> callback_dict;
    static callback_dict                                      installed_callbacks;

    callback_dict::iterator found = installed_callbacks.find(gg);
    if (found == installed_callbacks.end()) {
        gen_assert(install); // if !install then entry has to exist!
        installed_callbacks[gg] = gene_container_changed_cb_data(gmw, gg, 0);
        found                   = installed_callbacks.find(gg);
        gen_assert(found != installed_callbacks.end());
    }

    gene_container_changed_cb_data *curr_cb_data = &(found->second);

    if (install) {
        gen_assert(curr_cb_data->gb_gene_data == 0); // 0 means : no callback installed
        GBDATA         *gb_main    = gg->get_gb_main();
        GB_transaction  ta(gb_main);
        curr_cb_data->gb_gene_data = GEN_get_current_gene_data(gb_main, gg->get_aw_root());
        // @@@ FIXME: get data of local genome!

        if (curr_cb_data->gb_gene_data) {
            GB_add_callback(curr_cb_data->gb_gene_data, (GB_CB_TYPE)(GB_CB_DELETE|GB_CB_CHANGED), GEN_gene_container_changed_cb, (int*)curr_cb_data);
        }
    }
    else {
        if (curr_cb_data->gb_gene_data) { // if callback is installed
            GB_remove_callback(curr_cb_data->gb_gene_data, (GB_CB_TYPE)(GB_CB_DELETE|GB_CB_CHANGED), GEN_gene_container_changed_cb, (int*)curr_cb_data);
            curr_cb_data->gb_gene_data = 0;
        }
    }
}

void GEN_jump_cb(AW_window *aww, AW_CL cl_force_center_if_fits) {
    GEN_map_window *win                  = dynamic_cast<GEN_map_window*>(aww);
    bool            force_center_if_fits = (bool)cl_force_center_if_fits; // center gene if gene fits into display
    gen_assert(win);

    AW_rectangle  screen;       // screen coordinates
    AW_device    *device = win->get_graphic()->get_device();
    device->get_area_size(&screen);
#if defined(DEBUG)
    printf("Window %i: screen is: %i/%i -> %i/%i\n", win->get_nr(), screen.l, screen.t, screen.r, screen.b);
#endif // DEBUG

    const GEN_root *gen_root = win->get_graphic()->get_gen_root();
    if (gen_root) {
        const AW_world& wrange = gen_root->get_selected_range();
#if defined(DEBUG)
        printf("Window %i: Draw world range of selected gene is: %f/%f -> %f/%f\n", win->get_nr(), wrange.l, wrange.t, wrange.r, wrange.b);
#endif // DEBUG

        AW_rectangle srange;
        device->transform(int(wrange.l), int(wrange.t), srange.l, srange.t);
        device->transform(int(wrange.r), int(wrange.b), srange.r, srange.b);
#if defined(DEBUG)
        printf("Window %i: Draw screen range of selected gene is: %i/%i -> %i/%i\n", win->get_nr(), srange.l, srange.t, srange.r, srange.b);
#endif // DEBUG

        // add padding :
        // srange.t -= 2; srange.b += 2;
        // srange.l -= 2; srange.r += 2;

        AWT_canvas *canvas  = win->get_canvas();
        int         scrollx = 0;
        int         scrolly = 0;

        if (srange.t < 0) {
            scrolly = srange.t-2;
        }
        else if (srange.b > screen.b) {
            scrolly = (srange.b-screen.b)+2;
            // if (srange.t < scrolly) scrolly = srange.t; // avoid scrolling out top side of gene
        }
        if (srange.l < 0) {
            scrollx = srange.l-2;
        }
        else if (srange.r > screen.r) {
            scrollx = srange.r-screen.r+2;
            // if (srange.l < scrollx) scrollx = srange.l; // avoid scrolling out left side of gene
        }

        if (force_center_if_fits) {
            if (!scrollx) { // no scrolling needed, i.e. gene fits onto display horizontally
                int gene_center_x   = (srange.l+srange.r)/2;
                int screen_center_x = (screen.l+screen.r)/2;
                scrollx             = gene_center_x-screen_center_x;
#if defined(DEBUG)
                printf("center x\n");
#endif // DEBUG
            }
            if (!scrolly) { // no scrolling needed, i.e. gene fits onto display vertically
                int gene_center_y   = (srange.t+srange.b)/2;
                int screen_center_y = (screen.t+screen.b)/2;
                scrolly             = gene_center_y-screen_center_y;
#if defined(DEBUG)
                printf("center y\n");
#endif // DEBUG
            }
        }

#if defined(DEBUG)
        printf("scroll %i/%i\n", scrollx, scrolly);
#endif // DEBUG

        canvas->scroll(aww, scrollx, scrolly);
    }
    win->get_canvas()->refresh();
}

void GEN_jump_cb_auto(AW_root *root, GEN_map_window *win, bool force_refresh) {
    int jump = root->awar(AWAR_GENMAP_AUTO_JUMP)->read_int();
    if (jump) {
        win->get_canvas()->refresh(); // needed to recalculate position
        GEN_jump_cb(win, (AW_CL)false);
    }
    else if (force_refresh) win->get_canvas()->refresh();
}

void GEN_local_organism_or_gene_name_changed_cb(AW_root *awr, AW_CL cl_win) {
    GEN_map_window *win = (GEN_map_window*)cl_win;
    win->get_graphic()->reinit_gen_root(win->get_canvas(), false);
    GEN_jump_cb_auto(awr, win, true);
}

// void GEN_gene_name_changed_cb(AW_root *awr, AWT_canvas *gmw) {
//     GEN_GRAPHIC->reinit_gen_root(gmw);
//     GEN_jump_cb_auto(awr, gmw);
//     gmw->refresh();
// }

static void GEN_map_window_zoom_reset_and_refresh(GEN_map_window *gmw) {
    AWT_canvas *canvas = gmw->get_canvas();
    canvas->zoom_reset();
    canvas->refresh();
}
// static void GEN_map_window_refresh(GEN_map_window *gmw) {
    // AWT_canvas *canvas = gmw->get_canvas();
    // canvas->refresh();
// }

#define DISPLAY_TYPE_BIT(disp_type) (1<<(disp_type))
#define ALL_DISPLAY_TYPES           (DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLES)-1)

static void GEN_map_window_refresh_if_display_type(GEN_map_window *win, AW_CL cl_display_type_mask) {
    int display_type_mask = int(cl_display_type_mask);
    int my_display_type   = win->get_graphic()->get_display_style();

    if (display_type_mask & DISPLAY_TYPE_BIT(my_display_type)) {
        AWT_canvas *canvas = win->get_canvas();
        canvas->refresh();
    }
}

static void GEN_update_unlocked_organism_and_gene_awars(GEN_map_window *win, AW_CL cl_organism, AW_CL cl_gene) {
    AW_root *aw_root   = win->get_graphic()->get_aw_root();
    int      window_nr = win->get_nr();
    if (!aw_root->awar(AWAR_LOCAL_ORGANISM_LOCK(window_nr))->read_int()) {
        aw_root->awar(AWAR_LOCAL_ORGANISM_NAME(window_nr))->write_string((const char*)cl_organism);
    }
    if (!aw_root->awar(AWAR_LOCAL_GENE_LOCK(window_nr))->read_int()) {
        aw_root->awar(AWAR_LOCAL_GENE_NAME(window_nr))->write_string((const char*)cl_gene);
    }
}

static void GEN_organism_or_gene_changed_cb(AW_root *awr) {
    char *organism = awr->awar(AWAR_ORGANISM_NAME)->read_string();
    char *gene     = awr->awar(AWAR_GENE_NAME)->read_string();

    GEN_map_manager::with_all_mapped_windows(GEN_update_unlocked_organism_and_gene_awars, (AW_CL)organism, (AW_CL)gene);

    free(gene);
    free(organism);
}


static void GEN_local_lock_changed_cb(AW_root *awr, AW_CL cl_win, AW_CL cl_what_lock) {
    int             what_lock = (int)cl_what_lock;
    GEN_map_window *win       = (GEN_map_window*)cl_win;
    int             window_nr = win->get_nr();

    const char *local_awar_name      = 0;
    const char *local_lock_awar_name = 0;
    const char *global_awar_name     = 0;

    if (what_lock == 0) { // organism
        local_awar_name      = AWAR_LOCAL_ORGANISM_NAME(window_nr);
        local_lock_awar_name = AWAR_LOCAL_ORGANISM_LOCK(window_nr);
        global_awar_name     = AWAR_ORGANISM_NAME;
    }
    else { // gene
        local_awar_name      = AWAR_LOCAL_GENE_NAME(window_nr);
        local_lock_awar_name = AWAR_LOCAL_GENE_LOCK(window_nr);
        global_awar_name     = AWAR_GENE_NAME;
    }

    AW_awar *local_awar  = awr->awar(local_awar_name);
    AW_awar *global_awar = awr->awar(global_awar_name);

    int lock_value = awr->awar(local_lock_awar_name)->read_int();
    if (lock_value == 0) { // lock has been removed -> map to global awar
        local_awar->map(global_awar);
    }
    else { // lock has been installed -> unmap from global awar
        local_awar->unmap();
        char *content = global_awar->read_string();
        local_awar->write_string(content);
        free(content);
    }
}

// ------------------------------------
//      display parameter change cb
// ------------------------------------

static void GEN_display_param_changed_cb(AW_root */*awr*/, AW_CL cl_display_type_mask) {
    GEN_map_manager::with_all_mapped_windows(GEN_map_window_refresh_if_display_type, cl_display_type_mask);
}
inline void set_display_update_callback(AW_root *awr, const char *awar_name, int display_type_mask) {
    awr->awar(awar_name)->add_callback(GEN_display_param_changed_cb, AW_CL(display_type_mask));
}

// -------------------------
//      View-local AWARS
// -------------------------

static void GEN_create_genemap_local_awars(AW_root *aw_root,AW_default /*def*/, int window_nr) {
    // awars local to each view

    aw_root->awar_int(AWAR_GENMAP_DISPLAY_TYPE(window_nr), GEN_DISPLAY_STYLE_RADIAL); // @@@ FIXME: make local

    aw_root->awar_string(AWAR_LOCAL_ORGANISM_NAME(window_nr), "");
    aw_root->awar_string(AWAR_LOCAL_GENE_NAME(window_nr), "");

    aw_root->awar_int(AWAR_LOCAL_ORGANISM_LOCK(window_nr), 0);
    aw_root->awar_int(AWAR_LOCAL_GENE_LOCK(window_nr), 0);
}

static void GEN_add_local_awar_callbacks(AW_root *awr,AW_default /*def*/, GEN_map_window *win) {
    int window_nr = win->get_nr();

    awr->awar(AWAR_LOCAL_ORGANISM_NAME(window_nr))->add_callback(GEN_local_organism_or_gene_name_changed_cb, (AW_CL)win);
    awr->awar(AWAR_LOCAL_GENE_NAME(window_nr))->add_callback(GEN_local_organism_or_gene_name_changed_cb, (AW_CL)win);

    // awr->awar(AWAR_LOCAL_ORGANISM_NAME(window_nr))->map(AWAR_ORGANISM_NAME);
    // awr->awar(AWAR_LOCAL_GENE_NAME(window_nr))->map(AWAR_GENE_NAME);

    AW_awar *awar_lock_organism = awr->awar(AWAR_LOCAL_ORGANISM_LOCK(window_nr));
    AW_awar *awar_lock_gene     = awr->awar(AWAR_LOCAL_GENE_LOCK(window_nr));

    awar_lock_organism->add_callback(GEN_local_lock_changed_cb, (AW_CL)win, (AW_CL)0);
    awar_lock_gene->add_callback(GEN_local_lock_changed_cb, (AW_CL)win, (AW_CL)1);

    awar_lock_organism->touch();
    awar_lock_gene->touch();
}

// ---------------------
//      global AWARS
// ---------------------

static void GEN_create_genemap_global_awars(AW_root *aw_root,AW_default def) {

    // layout options:

    aw_root->awar_int(AWAR_GENMAP_ARROW_SIZE, 150);
    aw_root->awar_int(AWAR_GENMAP_SHOW_HIDDEN, 0);
    aw_root->awar_int(AWAR_GENMAP_SHOW_ALL_NDS, 0);

    aw_root->awar_int(AWAR_GENMAP_BOOK_BASES_PER_LINE, 15000);
    aw_root->awar_float(AWAR_GENMAP_BOOK_WIDTH_FACTOR, 0.1);
    aw_root->awar_int(AWAR_GENMAP_BOOK_LINE_HEIGHT, 20);
    aw_root->awar_int(AWAR_GENMAP_BOOK_LINE_SPACE, 5);

    aw_root->awar_float(AWAR_GENMAP_VERTICAL_FACTOR_X, 1.0);
    aw_root->awar_float(AWAR_GENMAP_VERTICAL_FACTOR_Y, 0.3);

    aw_root->awar_float(AWAR_GENMAP_RADIAL_INSIDE, 50);
    aw_root->awar_float(AWAR_GENMAP_RADIAL_OUTSIDE, 4);

    // other options:

    aw_root->awar_int(AWAR_GENMAP_AUTO_JUMP, 1);

    GEN_create_nds_vars(aw_root, def, gb_main, GEN_NDS_changed);
}

static void GEN_add_global_awar_callbacks(AW_root *awr) {
    set_display_update_callback(awr, AWAR_GENMAP_ARROW_SIZE,          ALL_DISPLAY_TYPES^DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLE_BOOK));
    set_display_update_callback(awr, AWAR_GENMAP_SHOW_HIDDEN,         ALL_DISPLAY_TYPES);
    set_display_update_callback(awr, AWAR_GENMAP_SHOW_ALL_NDS,        ALL_DISPLAY_TYPES);

    set_display_update_callback(awr, AWAR_GENMAP_BOOK_BASES_PER_LINE, DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLE_BOOK));
    set_display_update_callback(awr, AWAR_GENMAP_BOOK_WIDTH_FACTOR,   DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLE_BOOK));
    set_display_update_callback(awr, AWAR_GENMAP_BOOK_LINE_HEIGHT,    DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLE_BOOK));
    set_display_update_callback(awr, AWAR_GENMAP_BOOK_LINE_SPACE,     DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLE_BOOK));

    set_display_update_callback(awr, AWAR_GENMAP_VERTICAL_FACTOR_X,   DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLE_VERTICAL));
    set_display_update_callback(awr, AWAR_GENMAP_VERTICAL_FACTOR_Y,   DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLE_VERTICAL));

    set_display_update_callback(awr, AWAR_GENMAP_RADIAL_INSIDE,       DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLE_RADIAL));
    set_display_update_callback(awr, AWAR_GENMAP_RADIAL_OUTSIDE,      DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLE_RADIAL));

    awr->awar(AWAR_ORGANISM_NAME)->add_callback(GEN_organism_or_gene_changed_cb);
    awr->awar(AWAR_GENE_NAME)->add_callback(GEN_organism_or_gene_changed_cb);
}


// --------------------------------------------------------------------------------

void GEN_mode_event( AW_window *aws, AW_CL cl_win, AW_CL cl_mode) {
    GEN_map_window   *win  = (GEN_map_window*)cl_win;
    AWT_COMMAND_MODE  mode = (AWT_COMMAND_MODE)cl_mode;
    const char       *text = 0;
    switch (mode) {
        case AWT_MODE_SELECT: {
            text="SELECT MODE  LEFT: click to select";
            break;
        }
        case AWT_MODE_ZOOM: {
            text="ZOOM MODE    LEFT: drag to zoom   RIGHT: zoom out";
            break;
        }
        case AWT_MODE_MOD: {
            text="INFO MODE    LEFT: click for info";
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    gen_assert(strlen(text) < AWAR_FOOTER_MAX_LEN); // text too long!

    aws->get_root()->awar(AWAR_FOOTER)->write_string( text);
    AWT_canvas *canvas = win->get_canvas();
    canvas->set_mode(mode);
    canvas->refresh();
}

void GEN_undo_cb(AW_window *, AW_CL undo_type) {
    GB_ERROR error = GB_undo(gb_main,(GB_UNDO_TYPE)undo_type);
    if (error) {
        aw_message(error);
    }
    else{
        GB_begin_transaction(gb_main);
        GB_commit_transaction(gb_main);
        // ntw->refresh();
    }
}

AW_window *GEN_create_options_window(AW_root *awr) {
    static AW_window_simple *aws = 0;
    if (!aws) {

        aws = new AW_window_simple;
        aws->init( awr, "GEN_OPTIONS", "GENE MAP OPTIONS");
        aws->load_xfig("gene_options.fig");

        aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE","CLOSE","C");

        aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"gene_options.hlp");
        aws->create_button("HELP","HELP","H");

        aws->at("button");
        aws->auto_space(10,10);
        aws->label_length(30);

        aws->label("Auto jump to selected gene");
        aws->create_toggle(AWAR_GENMAP_AUTO_JUMP);
        aws->at_newline();
    }
    return aws;
}

AW_window *GEN_create_layout_window(AW_root *awr) {
    static AW_window_simple *aws = 0;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(awr, "GENE_LAYOUT", "Gene Map Layout");
        aws->load_xfig("gene_layout.fig");

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback( AW_POPUP_HELP,(AW_CL)"gen_layout.hlp");
        aws->at("help");
        aws->create_button("HELP","HELP","H");

        aws->at("base_pos");        aws->create_input_field(AWAR_GENMAP_BOOK_BASES_PER_LINE, 15);
        aws->at("width_factor");    aws->create_input_field(AWAR_GENMAP_BOOK_WIDTH_FACTOR, 7);
        aws->at("line_height");     aws->create_input_field(AWAR_GENMAP_BOOK_LINE_HEIGHT, 5);
        aws->at("line_space");      aws->create_input_field(AWAR_GENMAP_BOOK_LINE_SPACE, 5);

        aws->at("factor_x");        aws->create_input_field(AWAR_GENMAP_VERTICAL_FACTOR_X, 5);
        aws->at("factor_y");        aws->create_input_field(AWAR_GENMAP_VERTICAL_FACTOR_Y, 5);

        aws->at("inside");          aws->create_input_field(AWAR_GENMAP_RADIAL_INSIDE, 5);
        aws->at("outside");         aws->create_input_field(AWAR_GENMAP_RADIAL_OUTSIDE, 5);

        aws->at("arrow_size");      aws->create_input_field(AWAR_GENMAP_ARROW_SIZE, 5);

        aws->at("show_hidden");
        aws->label("Show hidden genes");
        aws->create_toggle(AWAR_GENMAP_SHOW_HIDDEN);

        aws->at("show_all");
        aws->label("Show NDS for all genes");
        aws->create_toggle(AWAR_GENMAP_SHOW_ALL_NDS);
    }
    return aws;
}

typedef enum  {
    GEN_PERFORM_ALL_ORGANISMS,
    GEN_PERFORM_CURRENT_ORGANISM,
    GEN_PERFORM_ALL_BUT_CURRENT_ORGANISM,
    GEN_PERFORM_MARKED_ORGANISMS
} GEN_PERFORM_MODE;

typedef enum  {
    GEN_MARK,
    GEN_UNMARK,
    GEN_INVERT_MARKED,
    GEN_COUNT_MARKED,

    //     GEN_MARK_COLORED,              done by awt_colorize_marked now
    //     GEN_UNMARK_COLORED,
    //     GEN_INVERT_COLORED,
    //     GEN_COLORIZE_MARKED,

    GEN_EXTRACT_MARKED,

    GEN_MARK_HIDDEN,
    GEN_MARK_VISIBLE,
    GEN_UNMARK_HIDDEN,
    GEN_UNMARK_VISIBLE
} GEN_MARK_MODE;

typedef enum  {
    GEN_HIDE_ALL,
    GEN_UNHIDE_ALL,
    GEN_INVERT_HIDE_ALL,

    GEN_HIDE_MARKED,
    GEN_UNHIDE_MARKED,
    GEN_INVERT_HIDE_MARKED
} GEN_HIDE_MODE;

// --------------------------------------------------------------------------------

inline bool nameIsUnique(const char *short_name, GBDATA *gb_species_data) {
    return GBT_find_species_rel_species_data(gb_species_data, short_name)==0;
}

static GB_ERROR GEN_species_add_entry(GBDATA *gb_pseudo, const char *key, const char *value) {
    GB_ERROR  error = 0;
    GB_clear_error();
    GBDATA   *gbd   = GB_find(gb_pseudo, key, 0, down_level);

    if (!gbd) { // key does not exist yet -> create
        gbd   = GB_create(gb_pseudo, key, GB_STRING);
        error = GB_get_error();
    }
    else { // key exists
        if (GB_read_type(gbd) != GB_STRING) { // test correct key type
            error = GB_export_error("field '%s' exists and has wrong type", key);
        }
    }

    if (!error) error = GB_write_string(gbd, value);

    return error;
}

static AW_repeated_question *ask_about_existing_gene_species = 0;
static AW_repeated_question *ask_to_overwrite_alignment      = 0;

void GEN_extract_gene_2_pseudoSpecies(GBDATA *gb_species, GBDATA *gb_gene, const char *ali) {
    GBDATA *gb_sp_name        = GB_find(gb_species,"name",0,down_level);
    GBDATA *gb_sp_fullname    = GB_find(gb_species,"full_name",0,down_level);
    char   *species_name      = gb_sp_name ? GB_read_string(gb_sp_name) : 0;
    char   *full_species_name = gb_sp_fullname ? GB_read_string(gb_sp_fullname) : species_name;
    GBDATA *gb_species_data   = GB_search(gb_main, "species_data",  GB_CREATE_CONTAINER);

    if (!species_name) {
        aw_message("Skipped species w/o name");
        return;
    }

    GBDATA *gb_ge_name = GB_find(gb_gene,"name",0,down_level);
    char   *gene_name  = gb_sp_name ? GB_read_string(gb_ge_name) : 0;

    if (!gene_name) {
        aw_message("Skipped gene w/o name");
        free(species_name);
        return;
    }

    char *full_name = GBS_strdup(GBS_global_string("%s [%s]", full_species_name, gene_name));

    char *sequence = GBT_read_gene_sequence(gb_gene, true);
    if (!sequence) {
        aw_message(GB_get_error());
    }
    else  {
        long id = GBS_checksum(sequence, 1, ".-");
        char acc[100];
        sprintf(acc, "ARB_GENE_%lX", id);

        // test if this gene has been already extracted to a gene-species

        GBDATA   *gb_exist_geneSpec       = GEN_find_pseudo_species(gb_main, species_name, gene_name);
        bool      create_new_gene_species = true;
        char     *short_name              = 0;
        GB_ERROR  error                   = 0;

        if (gb_exist_geneSpec) {
            GBDATA *gb_name       = GB_find(gb_exist_geneSpec, "name", 0, down_level);
            char   *existing_name = GB_read_string(gb_name);

            gen_assert(ask_about_existing_gene_species);
            gen_assert(ask_to_overwrite_alignment);

            char *question = GBS_global_string_copy("Already have a gene-species for %s/%s ('%s')", species_name, gene_name, existing_name);
            int   answer   = ask_about_existing_gene_species->get_answer(question, "Overwrite species,Insert new alignment,Skip,Create new", "all", true);

            create_new_gene_species = false;

            switch (answer) {
                case 0: {   // Overwrite species
                    // @@@ FIXME:  delete species needed here
                    create_new_gene_species = true;
                    short_name              = strdup(existing_name);
                    break;
                }
                case 1: {     // Insert new alignment or overwrite alignment
                    GBDATA *gb_ali = GB_find(gb_exist_geneSpec, ali, 0, down_level);
                    if (gb_ali) { // the alignment already exists
                        char *question2        = GBS_global_string_copy("Gene-species '%s' already has data in '%s'", existing_name, ali);
                        int   overwrite_answer = ask_to_overwrite_alignment->get_answer(question2, "Overwrite data,Skip", "all", true);

                        if (overwrite_answer == 1) error      = GBS_global_string("Skipped gene-species '%s' (already had data in alignment)", existing_name); // Skip
                        else if (overwrite_answer == 2) error = "Aborted."; // Abort
                        // @@@ FIXME: overwrite data is missing

                        free(question2);
                    }
                    break;
                }
                case 2: {       // Skip existing ones
                    error = GBS_global_string("Skipped gene-species '%s'", existing_name);
                    break;
                }
                case 3: {   // Create with new name
                    create_new_gene_species = true;
                    break;
                }
                case 4: {   // Abort
                    error = "Aborted.";
                    break;
                }
                default : gen_assert(0);
            }
            free(question);
            free(existing_name);
        }

        if (!error) {
            if (create_new_gene_species) {
                if (!short_name) { // create a new name
                    error = AWTC_generate_one_name(gb_main, full_name, acc, short_name, false);
                    if (!error) { // name was created
                        if (!nameIsUnique(short_name, gb_species_data)) {
                            char *uniqueName = AWTC_makeUniqueShortName(short_name, gb_species_data);
                            free(short_name);
                            short_name       = uniqueName;
                            if (!short_name) error = "No short name created.";
                        }
                    }
                    if (error) {            // try to make a random name
                        const char *msg = GBS_global_string("%s\nGenerating a random name instead.", error);
                        aw_message(msg);
                        error      = 0;

                        short_name = AWTC_generate_random_name(gb_species_data);
                        if (!short_name) error = GBS_global_string("Failed to create a new name for pseudo gene-species '%s'", full_name);
                    }
                }

                if (!error) { // create the species
                    gen_assert(short_name);
                    gb_exist_geneSpec = GBT_create_species(gb_main, short_name);
                    if (!gb_exist_geneSpec) error = GB_export_error("Failed to create pseudo-species '%s'", short_name);
                }
            }
            else {
                gen_assert(gb_exist_geneSpec); // do not generate new or skip -> should only occur when gene-species already existed
            }

            if (!error) { // write sequence data
                GBDATA *gb_data = GBT_add_data(gb_exist_geneSpec, ali, "data", GB_STRING);
                if (gb_data) {
                    size_t sequence_length = strlen(sequence);
                    error                  = GBT_write_sequence(gb_data, ali, sequence_length, sequence);
                }
                else {
                    error = GB_get_error();
                }

                //                 GBDATA *gb_ali = GB_search(gb_exist_geneSpec, ali, GB_DB);
                //                 if (gb_ali) {
                //                     GBDATA *gb_data = GB_search(gb_ali, "data", GB_STRING);
                //                     error           = GB_write_string(gb_data, sequence);
                //                     GBT_write_sequence(...);
                //                 }
                //                 else {
                //                     error = GB_export_error("Can't create alignment '%s' for '%s'", ali, short_name);
                //                 }
            }



            // write other entries:
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "full_name", full_name);
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "ARB_origin_species", species_name);
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "ARB_origin_gene", gene_name);
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "acc", acc);

            // copy codon_start and transl_table :
            const char *codon_start  = 0;
            const char *transl_table = 0;

            {
                GBDATA *gb_codon_start  = GB_find(gb_gene, "codon_start", 0, down_level);
                GBDATA *gb_transl_table = GB_find(gb_gene, "transl_table", 0, down_level);

                if (gb_codon_start)  codon_start  = GB_read_char_pntr(gb_codon_start);
                if (gb_transl_table) transl_table = GB_read_char_pntr(gb_transl_table);
            }

            if (!error && codon_start)  error = GEN_species_add_entry(gb_exist_geneSpec, "codon_start", codon_start);
            if (!error && transl_table) error = GEN_species_add_entry(gb_exist_geneSpec, "transl_table", transl_table);
        }
        if (error) aw_message(error);

        free(short_name);
        free(sequence);
    }

    free(full_name);
    free(gene_name);
    free(full_species_name);
    free(species_name);
}

static long gen_count_marked_genes = 0; // used to count marked genes

static void do_mark_command_for_one_species(int imode, GBDATA *gb_species, AW_CL cl_user) {
    GEN_MARK_MODE mode  = (GEN_MARK_MODE)imode;
    GB_ERROR      error = 0;

    for (GBDATA *gb_gene = GEN_first_gene(gb_species);
         !error && gb_gene;
         gb_gene = GEN_next_gene(gb_gene))
    {
        bool mark_flag     = GB_read_flag(gb_gene) != 0;
        bool org_mark_flag = mark_flag;
        //         int wantedColor;

        switch (mode) {
            case GEN_MARK:              mark_flag = 1; break;
            case GEN_UNMARK:            mark_flag = 0; break;
            case GEN_INVERT_MARKED:     mark_flag = !mark_flag; break;

            case GEN_COUNT_MARKED:     {
                if (mark_flag) ++gen_count_marked_genes;
                break;
            }
            case GEN_EXTRACT_MARKED: {
                if (mark_flag) GEN_extract_gene_2_pseudoSpecies(gb_species, gb_gene, (const char *)cl_user);
                break;
            }
                //             case GEN_COLORIZE_MARKED:  {
                //                 if (mark_flag) error = AW_set_color_group(gb_gene, wantedColor);
                //                 break;
                //             }
            default: {
                GBDATA *gb_hidden = GB_find(gb_gene, ARB_HIDDEN, 0, down_level);
                bool    hidden    = gb_hidden ? GB_read_byte(gb_hidden) != 0 : false;
                //                 long    myColor   = AW_find_color_group(gb_gene, true);

                switch (mode) {
                    //                     case GEN_MARK_COLORED:      if (myColor == wantedColor) mark_flag = 1; break;
                    //                     case GEN_UNMARK_COLORED:    if (myColor == wantedColor) mark_flag = 0; break;
                    //                     case GEN_INVERT_COLORED:    if (myColor == wantedColor) mark_flag = !mark_flag; break;

                    case GEN_MARK_HIDDEN:       if (hidden) mark_flag = 1; break;
                    case GEN_UNMARK_HIDDEN:     if (hidden) mark_flag = 0; break;
                    case GEN_MARK_VISIBLE:      if (!hidden) mark_flag = 1; break;
                    case GEN_UNMARK_VISIBLE:    if (!hidden) mark_flag = 0; break;
                    default: gen_assert(0); break;
                }
            }
        }

        if (mark_flag != org_mark_flag) {
            error = GB_write_flag(gb_gene, mark_flag?1:0);
        }
    }

    if (error) aw_message(error);
}

static void do_hide_command_for_one_species(int imode, GBDATA *gb_species, AW_CL /*cl_user*/) {
    GEN_HIDE_MODE mode = (GEN_HIDE_MODE)imode;

    for (GBDATA *gb_gene = GEN_first_gene(gb_species);
         gb_gene;
         gb_gene = GEN_next_gene(gb_gene))
    {
        bool marked = GB_read_flag(gb_gene) != 0;

        GBDATA *gb_hidden  = GB_find(gb_gene, ARB_HIDDEN, 0, down_level);
        bool    hidden     = gb_hidden ? (GB_read_byte(gb_hidden) != 0) : false;
        bool    org_hidden = hidden;

        switch (mode) {
            case GEN_HIDE_ALL:              hidden = true; break;
            case GEN_UNHIDE_ALL:            hidden = false; break;
            case GEN_INVERT_HIDE_ALL:       hidden = !hidden; break;
            case GEN_HIDE_MARKED:           if (marked) hidden = true; break;
            case GEN_UNHIDE_MARKED:         if (marked) hidden = false; break;
            case GEN_INVERT_HIDE_MARKED:    if (marked) hidden = !hidden; break;
            default: gen_assert(0); break;
        }

        if (hidden != org_hidden) {
            if (!gb_hidden) gb_hidden = GB_create(gb_gene, ARB_HIDDEN, GB_BYTE);
            GB_write_byte(gb_hidden, hidden ? 1 : 0); // change gene visibility
        }
    }
}

static void  GEN_perform_command(AW_window *aww, GEN_PERFORM_MODE pmode,
                                 void (*do_command)(int cmode, GBDATA *gb_species, AW_CL cl_user), int mode, AW_CL cl_user) {
    GB_ERROR error = 0;

    GB_begin_transaction(gb_main);

    switch (pmode) {
        case GEN_PERFORM_ALL_ORGANISMS: {
            for (GBDATA *gb_organism = GEN_first_organism(gb_main);
                 gb_organism;
                 gb_organism = GEN_next_organism(gb_organism))
            {
                do_command(mode, gb_organism, cl_user);
            }
            break;
        }
        case GEN_PERFORM_MARKED_ORGANISMS: {
            for (GBDATA *gb_organism = GEN_first_marked_organism(gb_main);
                 gb_organism;
                 gb_organism = GEN_next_marked_organism(gb_organism))
            {
                do_command(mode, gb_organism, cl_user);
            }
            break;
        }
        case GEN_PERFORM_ALL_BUT_CURRENT_ORGANISM: {
            AW_root *aw_root          = aww->get_root();
            GBDATA  *gb_curr_organism = GEN_get_current_organism(gb_main, aw_root);

            for (GBDATA *gb_organism = GEN_first_organism(gb_main);
                 gb_organism;
                 gb_organism = GEN_next_organism(gb_organism))
            {
                if (gb_organism != gb_curr_organism) do_command(mode, gb_organism, cl_user);
            }
            break;
        }
        case GEN_PERFORM_CURRENT_ORGANISM: {
            AW_root *aw_root     = aww->get_root();
            GBDATA  *gb_organism = GEN_get_current_organism(gb_main, aw_root);

            if (!gb_organism) {
                error = "First you have to select a species.";
            }
            else {
                do_command(mode, gb_organism, cl_user);
            }
            break;
        }
        default: {
            gen_assert(0);
            break;
        }
    }

    if (!error) GB_commit_transaction(gb_main);
    else GB_abort_transaction(gb_main);

    if (error) aw_message(error);

}
static void GEN_hide_command(AW_window *aww, AW_CL cl_pmode, AW_CL cl_hmode) {
    GEN_perform_command(aww, (GEN_PERFORM_MODE)cl_pmode, do_hide_command_for_one_species, cl_hmode, 0);
}
static void GEN_mark_command(AW_window *aww, AW_CL cl_pmode, AW_CL cl_mmode) {
    gen_count_marked_genes = 0;
    GEN_perform_command(aww, (GEN_PERFORM_MODE)cl_pmode, do_mark_command_for_one_species, cl_mmode, 0);

    if ((GEN_MARK_MODE)cl_mmode == GEN_COUNT_MARKED) {
        const char *where = 0;

        switch ((GEN_PERFORM_MODE)cl_pmode) {
            case GEN_PERFORM_CURRENT_ORGANISM:         where = "the current species"; break;
            case GEN_PERFORM_MARKED_ORGANISMS:         where = "all marked species"; break;
            case GEN_PERFORM_ALL_ORGANISMS:            where = "all species"; break;
            case GEN_PERFORM_ALL_BUT_CURRENT_ORGANISM: where = "all but the current species"; break;
            default: gen_assert(0); break;
        }
        aw_message(GBS_global_string("There are %li marked genes in %s", gen_count_marked_genes, where));
    }
}

void gene_extract_cb(AW_window *aww, AW_CL cl_pmode){
    char     *ali   = aww->get_root()->awar(AWAR_GENE_EXTRACT_ALI)->read_string();
    GB_ERROR  error = GBT_check_alignment_name(ali);

    if (!error) {
        GB_transaction  dummy(gb_main);
        GBDATA         *gb_ali = GBT_get_alignment(gb_main, ali);

        if (!gb_ali && !GBT_create_alignment(gb_main,ali,0,0,0,"dna")) {
            error = GB_get_error();
        }
    }

    if (error) {
        aw_message(error);
    }
    else {
        ask_about_existing_gene_species = new AW_repeated_question();
        ask_to_overwrite_alignment      = new AW_repeated_question();

        aw_openstatus("Extracting pseudo-species");
        {
            PersistantNameServerConnection stayAlive; 
            GEN_perform_command(aww, (GEN_PERFORM_MODE)cl_pmode, do_mark_command_for_one_species, GEN_EXTRACT_MARKED, (AW_CL)ali);
        }
        aw_closestatus();

        delete ask_to_overwrite_alignment;
        delete ask_about_existing_gene_species;
        ask_to_overwrite_alignment      = 0;
        ask_about_existing_gene_species = 0;
    }
    free(ali);
}

GBDATA *GEN_find_pseudo(GBDATA *gb_organism, GBDATA *gb_gene) {
    GBDATA *gb_species_data = GB_get_father(gb_organism);
    GBDATA *gb_name         = GB_find(gb_organism, "name", 0, down_level);
    char   *organism_name   = GB_read_string(gb_name);
    gb_name                 = GB_find(gb_gene, "name", 0, down_level);
    char   *gene_name       = GB_read_string(gb_name);
    GBDATA *gb_pseudo       = 0;

    for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        const char *this_organism_name = GEN_origin_organism(gb_species);

        if (this_organism_name && strcmp(this_organism_name, organism_name) == 0)
        {
            if (strcmp(GEN_origin_gene(gb_species), gene_name) == 0)
            {
                gb_pseudo = gb_species;
                break;
            }
        }
    }

    return gb_pseudo;
}

static void mark_organisms(AW_window */*aww*/, AW_CL cl_mark, AW_CL cl_canvas) {
    // cl_mark == 0 -> unmark
    // cl_mark == 1 -> mark
    // cl_mark == 2 -> invert mark
    // cl_mark == 3 -> mark organisms, unmark rest
    GB_transaction dummy(gb_main);
    int            mark = (int)cl_mark;

    if (mark == 3) {
        GBT_mark_all(gb_main, 0); // unmark all species
        mark = 1;
    }

    for (GBDATA *gb_org = GEN_first_organism(gb_main);
         gb_org;
         gb_org = GEN_next_organism(gb_org))
    {
        if (mark == 2) {
            GB_write_flag(gb_org, !GB_read_flag(gb_org)); // invert mark of organism
        }
        else {
            GB_write_flag(gb_org, mark); // mark/unmark organism
        }
    }

    AWT_canvas *canvas = (AWT_canvas*)cl_canvas;
    if (canvas) canvas->refresh();
}


static void mark_gene_species(AW_window */*aww*/, AW_CL cl_mark, AW_CL cl_canvas) {
    //  cl_mark == 0 -> unmark
    //  cl_mark == 1 -> mark
    //  cl_mark == 2 -> invert mark
    //  cl_mark == 3 -> mark gene-species, unmark rest
    GB_transaction dummy(gb_main);
    int            mark = (int)cl_mark;

    if (mark == 3) {
        GBT_mark_all(gb_main, 0); // unmark all species
        mark = 1;
    }

    for (GBDATA *gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        if (mark == 2) {
            GB_write_flag(gb_pseudo, !GB_read_flag(gb_pseudo)); // invert mark of pseudo-species
        }
        else {
            GB_write_flag(gb_pseudo, mark); // mark/unmark gene-species
        }
    }

    AWT_canvas *canvas = (AWT_canvas*)cl_canvas;
    if (canvas) canvas->refresh();
}
static void mark_gene_species_of_marked_genes(AW_window */*aww*/, AW_CL cl_canvas, AW_CL) {
    GB_transaction dummy(gb_main);

    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        for (GBDATA *gb_gene = GEN_first_gene(gb_species);
             gb_gene;
             gb_gene = GEN_next_gene(gb_gene))
        {
            if (GB_read_flag(gb_gene)) {
                GBDATA *gb_pseudo = GEN_find_pseudo(gb_species, gb_gene);
                if (gb_pseudo) GB_write_flag(gb_pseudo, 1);
            }
        }
    }
    AWT_canvas *canvas = (AWT_canvas*)cl_canvas;
    if (canvas) canvas->refresh();
}
static void mark_organisms_with_marked_genes(AW_window */*aww*/, AW_CL /*cl_canvas*/, AW_CL) {
    GB_transaction dummy(gb_main);

    for (GBDATA *gb_species = GBT_first_species(gb_main);
         gb_species;
         gb_species = GBT_next_species(gb_species))
    {
        for (GBDATA *gb_gene = GEN_first_gene(gb_species);
             gb_gene;
             gb_gene = GEN_next_gene(gb_gene))
        {
            if (GB_read_flag(gb_gene)) {
                GB_write_flag(gb_species, 1);
                break; // continue with next organism
            }
        }
    }
}
static void mark_gene_species_using_current_alignment(AW_window */*aww*/, AW_CL /*cl_canvas*/, AW_CL) {
    GB_transaction  dummy(gb_main);
    char           *ali = GBT_get_default_alignment(gb_main);

    for (GBDATA *gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        GBDATA *gb_ali = GB_find(gb_pseudo, ali, 0, down_level);
        if (gb_ali) {
            GBDATA *gb_data = GB_find(gb_ali, "data", 0, down_level);
            if (gb_data) {
                GB_write_flag(gb_pseudo, 1);
            }
        }
    }
}
static void mark_genes_of_marked_gene_species(AW_window */*aww*/, AW_CL, AW_CL) {
    GB_transaction dummy(gb_main);

    for (GBDATA *gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        if (GB_read_flag(gb_pseudo)) {
            GBDATA *gb_gene = GEN_find_origin_gene(gb_pseudo);
            GB_write_flag(gb_gene, 1); // mark gene
        }
    }
}

AW_window *create_gene_extract_window(AW_root *root, AW_CL cl_pmode)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "EXTRACT_GENE", "Extract genes to alignment");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nof the alignment to extract to");

    aws->at("input");
    aws->create_input_field(AWAR_GENE_EXTRACT_ALI,15);

    aws->at("ok");
    aws->callback(gene_extract_cb, cl_pmode);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

#define AWMIMT awm->insert_menu_topic

void GEN_insert_extract_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                                const char *help_file) {
    //     GEN_insert_multi_submenu(awm, macro_prefix, submenu_name, hot_key, help_file, GEN_extract_marked_command, (AW_CL)mark_mode);
    awm->insert_sub_menu(0, submenu_name, hot_key);

    char macro_name_buffer[50];

    sprintf(macro_name_buffer, "%s_of_current", macro_prefix);
    AWMIMT(macro_name_buffer, "of current species...", "c", help_file, AWM_ALL, AW_POPUP, (AW_CL)create_gene_extract_window, (AW_CL)GEN_PERFORM_CURRENT_ORGANISM);

    sprintf(macro_name_buffer, "%s_of_marked", macro_prefix);
    AWMIMT(macro_name_buffer, "of marked species...", "m", help_file, AWM_ALL, AW_POPUP, (AW_CL)create_gene_extract_window, (AW_CL)GEN_PERFORM_MARKED_ORGANISMS);

    sprintf(macro_name_buffer, "%s_of_all", macro_prefix);
    AWMIMT(macro_name_buffer, "of all species...", "a", help_file, AWM_ALL, AW_POPUP, (AW_CL)create_gene_extract_window, (AW_CL)GEN_PERFORM_ALL_ORGANISMS);

    awm->close_sub_menu();
}

void GEN_insert_multi_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                              const char *help_file, void (*command)(AW_window*, AW_CL, AW_CL), AW_CL command_mode)
{
    awm->insert_sub_menu(0, submenu_name, hot_key);

    char macro_name_buffer[50];

    sprintf(macro_name_buffer, "%s_of_current", macro_prefix);
    AWMIMT(macro_name_buffer, "of current species", "c", help_file, AWM_ALL, command, GEN_PERFORM_CURRENT_ORGANISM, command_mode);

    sprintf(macro_name_buffer, "%s_of_all_but_current", macro_prefix);
    AWMIMT(macro_name_buffer, "of all but current species", "b", help_file, AWM_ALL, command, GEN_PERFORM_ALL_BUT_CURRENT_ORGANISM, command_mode);

    sprintf(macro_name_buffer, "%s_of_marked", macro_prefix);
    AWMIMT(macro_name_buffer, "of marked species", "m", help_file, AWM_ALL, command, GEN_PERFORM_MARKED_ORGANISMS, command_mode);

    sprintf(macro_name_buffer, "%s_of_all", macro_prefix);
    AWMIMT(macro_name_buffer, "of all species", "a", help_file, AWM_ALL, command, GEN_PERFORM_ALL_ORGANISMS, command_mode);

    awm->close_sub_menu();
}
void GEN_insert_mark_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                             const char *help_file, GEN_MARK_MODE mark_mode)
{
    GEN_insert_multi_submenu(awm, macro_prefix, submenu_name, hot_key, help_file, GEN_mark_command, (AW_CL)mark_mode);
}
void GEN_insert_hide_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                             const char *help_file, GEN_HIDE_MODE hide_mode) {
    GEN_insert_multi_submenu(awm, macro_prefix, submenu_name, hot_key, help_file, GEN_hide_command, (AW_CL)hide_mode);
}

#if defined(DEBUG)
AW_window *GEN_create_awar_debug_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aw_root, "DEBUG_AWARS", "DEBUG AWARS");
        aws->at(10, 10);
        aws->auto_space(10,10);

        const int width = 50;

        ;                  aws->label("AWAR_SPECIES_NAME            "); aws->create_input_field(AWAR_SPECIES_NAME, width);
        aws->at_newline(); aws->label("AWAR_ORGANISM_NAME           "); aws->create_input_field(AWAR_ORGANISM_NAME, width);
        aws->at_newline(); aws->label("AWAR_GENE_NAME               "); aws->create_input_field(AWAR_GENE_NAME, width);
        aws->at_newline(); aws->label("AWAR_COMBINED_GENE_NAME      "); aws->create_input_field(AWAR_COMBINED_GENE_NAME, width);
        aws->at_newline(); aws->label("AWAR_EXPERIMENT_NAME         "); aws->create_input_field(AWAR_EXPERIMENT_NAME, width);
        aws->at_newline(); aws->label("AWAR_COMBINED_EXPERIMENT_NAME"); aws->create_input_field(AWAR_COMBINED_EXPERIMENT_NAME, width);
        aws->at_newline(); aws->label("AWAR_PROTEOM_NAME            "); aws->create_input_field(AWAR_PROTEOM_NAME, width);
        aws->at_newline(); aws->label("AWAR_PROTEIN_NAME            "); aws->create_input_field(AWAR_PROTEIN_NAME, width);

        aws->window_fit();
    }
    return aws;
}
#endif // DEBUG

// --------------------------
//      user mask section
// --------------------------

class GEN_item_type_species_selector : public awt_item_type_selector {
public:
    GEN_item_type_species_selector() : awt_item_type_selector(AWT_IT_GENE) {}
    virtual ~GEN_item_type_species_selector() {}

    virtual const char *get_self_awar() const {
        return AWAR_COMBINED_GENE_NAME;
    }
    virtual size_t get_self_awar_content_length() const {
        return 12 + 1 + 40; // species-name+'/'+gene_name
    }
    virtual void add_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const { // add callbacks to awars
        root->awar(get_self_awar())->add_callback(f, cl_mask);
    }
    virtual void remove_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const  {
        root->awar(get_self_awar())->remove_callback(f, cl_mask);
    }
    virtual GBDATA *current(AW_root *root) const { // give the current item
        char   *species_name = root->awar(AWAR_ORGANISM_NAME)->read_string();
        char   *gene_name   = root->awar(AWAR_GENE_NAME)->read_string();
        GBDATA *gb_gene      = 0;

        if (species_name[0] && gene_name[0]) {
            GB_transaction dummy(gb_main);
            GBDATA *gb_species = GBT_find_species(gb_main,species_name);
            if (gb_species) {
                gb_gene = GEN_find_gene(gb_species, gene_name);
            }
        }

        free(gene_name);
        free(species_name);

        return gb_gene;
    }
    virtual const char *getKeyPath() const { // give the keypath for items
        return CHANGE_KEY_PATH_GENES;
    }
};

static GEN_item_type_species_selector item_type_gene;

static void GEN_open_mask_window(AW_window *aww, AW_CL cl_id, AW_CL) {
    int                              id         = int(cl_id);
    const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
    gen_assert(descriptor);
    if (descriptor) AWT_initialize_input_mask(aww->get_root(), gb_main, &item_type_gene, descriptor->get_internal_maskname(), descriptor->is_local_mask());
}

static void GEN_create_mask_submenu(AW_window_menu_modes *awm) {
    AWT_create_mask_submenu(awm, AWT_IT_GENE, GEN_open_mask_window);
}

static AW_window *GEN_create_gene_colorize_window(AW_root *aw_root) {
    return awt_create_item_colorizer(aw_root, gb_main, &GEN_item_selector);
}

static AW_window *GEN_create_organism_colorize_window(AW_root *aw_root) {
    return awt_create_item_colorizer(aw_root, gb_main, &AWT_organism_selector);
}

// used to avoid that the organisms info window is stored in a menu (or with a button)
void GEN_popup_organism_window(AW_window *aww, AW_CL, AW_CL) {
    AW_window *aws = NT_create_organism_window(aww->get_root());
    aws->show();
}
void GEN_create_organism_submenu(AW_window_menu_modes *awm, bool submenu/*, AWT_canvas *ntree_canvas*/) {
    const char *title  = "Organisms";
    const char *hotkey = "O";

    if (submenu) awm->insert_sub_menu(0, title, hotkey);
    else awm->create_menu(0, title, hotkey, "no.hlp", AWM_ALL);

    {
        AWMIMT( "organism_info", "Organism information", "i", "organism_info.hlp", AWM_ALL,GEN_popup_organism_window,  0, 0);
//         AWMIMT( "organism_info", "Organism information", "i", "organism_info.hlp", AWM_ALL,AW_POPUP,   (AW_CL)NT_create_organism_window,  0 );

        awm->insert_separator();

        AWMIMT("mark_organisms", "Mark All organisms", "A", "organism_mark.hlp", AWM_ALL, mark_organisms, 1, 0/*(AW_CL)ntree_canvas*/);
        AWMIMT("mark_organisms_unmark_rest", "Mark all organisms, unmark Rest", "R", "organism_mark.hlp", AWM_ALL, mark_organisms, 3, 0/*(AW_CL)ntree_canvas*/);
        AWMIMT("unmark_organisms", "Unmark all organisms", "U", "organism_mark.hlp", AWM_ALL, mark_organisms, 0, 0/*(AW_CL)ntree_canvas*/);
        AWMIMT("invmark_organisms", "Invert marks of all organisms", "v", "organism_mark.hlp", AWM_ALL, mark_organisms, 2, 0/*(AW_CL)ntree_canvas*/);
        awm->insert_separator();
        AWMIMT("mark_organisms_with_marked_genes", "Mark organisms with marked Genes", "G", "organism_mark.hlp", AWM_ALL, mark_organisms_with_marked_genes, 0/*(AW_CL)ntree_canvas*/, 0);
        awm->insert_separator();
        AWMIMT( "organism_colors",  "Colors ...",           "C",    "mark_colors.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_organism_colorize_window, 0);
    }
    if (submenu) awm->close_sub_menu();
}

void GEN_create_gene_species_submenu(AW_window_menu_modes *awm, bool submenu/*, AWT_canvas *ntree_canvas*/) {
    const char *title  = "Gene-Species";
    const char *hotkey = "S";

    if (submenu) awm->insert_sub_menu(0, title, hotkey);
    else awm->create_menu(0, title, hotkey, "no.hlp", AWM_ALL);

    {
        AWMIMT("mark_gene_species", "Mark All gene-species", "A", "gene_species_mark.hlp", AWM_ALL, mark_gene_species, 1, 0/*(AW_CL)ntree_canvas*/);
        AWMIMT("mark_gene_species_unmark_rest", "Mark all gene-species, unmark Rest", "R", "gene_species_mark.hlp", AWM_ALL, mark_gene_species, 3, 0/*(AW_CL)ntree_canvas*/);
        AWMIMT("unmark_gene_species", "Unmark all gene-species", "U", "gene_species_mark.hlp", AWM_ALL, mark_gene_species, 0, 0/*(AW_CL)ntree_canvas*/);
        AWMIMT("invmark_gene_species", "Invert marks of all gene-species", "I", "gene_species_mark.hlp", AWM_ALL, mark_gene_species, 2, 0/*(AW_CL)ntree_canvas*/);
        awm->insert_separator();
        AWMIMT("mark_gene_species_of_marked_genes", "Mark gene-species of marked genes", "M", "gene_species_mark.hlp", AWM_ALL, mark_gene_species_of_marked_genes, 0/*(AW_CL)ntree_canvas*/, 0);
        AWMIMT("mark_gene_species", "Mark all gene-species using Current alignment", "C", "gene_species_mark.hlp", AWM_ALL, mark_gene_species_using_current_alignment, 0/*(AW_CL)ntree_canvas*/, 0);
    }

    if (submenu) awm->close_sub_menu();
}

struct GEN_update_info {
    AWT_canvas *canvas1;        // just canvasses of different windows (needed for updates)
    AWT_canvas *canvas2;
};

void GEN_create_genes_submenu(AW_window_menu_modes *awm, bool for_ARB_NTREE/*, AWT_canvas *ntree_canvas*/) {
    // gen_assert(ntree_canvas != 0);

    awm->create_menu(0,"Genome", "G", "no.hlp",    AWM_ALL);
    {
#if defined(DEBUG)
        AWMIMT("debug_awars", "[DEBUG] Show main AWARs", "", "no.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_awar_debug_window, 0);
        awm->insert_separator();
#endif // DEBUG

        if (for_ARB_NTREE) {
            AWMIMT( "gene_map", "Gene Map", "", "gene_map.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_map_first, 0 /*(AW_CL)ntree_canvas*/); // initial gene map
            awm->insert_separator();

            GEN_create_gene_species_submenu(awm, true/*, ntree_canvas*/); // Gene-species
            GEN_create_organism_submenu(awm, true/*, ntree_canvas*/); // Organisms
            EXP_create_experiments_submenu(awm, true); // Experiments
            awm->insert_separator();
        }

        AWMIMT( "gene_info",    "Gene information", "", "gene_info.hlp", AWM_ALL,GEN_popup_gene_window, (AW_CL)awm, 0);
//         AWMIMT( "gene_info",    "Gene information", "", "gene_info.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_gene_window, 0 );
        AWMIMT( "gene_search",  "Search and Query", "", "gene_search.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_gene_query_window, 0 );

        GEN_create_mask_submenu(awm);

        awm->insert_separator();

        GEN_insert_mark_submenu(awm, "gene_mark_all", "Mark all genes", "M", "gene_mark.hlp",  GEN_MARK);
        GEN_insert_mark_submenu(awm, "gene_unmark_all", "Unmark all genes", "U", "gene_mark.hlp", GEN_UNMARK);
        GEN_insert_mark_submenu(awm, "gene_invert_marked", "Invert marked genes", "v", "gene_mark.hlp", GEN_INVERT_MARKED);
        GEN_insert_mark_submenu(awm, "gene_count_marked", "Count marked genes", "C", "gene_mark.hlp", GEN_COUNT_MARKED);

        AWMIMT( "gene_colors",  "Colors ...", "l",    "mark_colors.hlp", AWM_ALL,AW_POPUP,   (AW_CL)GEN_create_gene_colorize_window, 0);

        awm->insert_separator();

        AWMIMT("mark_genes_of_marked_gene_species", "Mark genes of marked gene-species", "g", "gene_mark.hlp", AWM_ALL, mark_genes_of_marked_gene_species, 0, 0);

        awm->insert_separator();

        GEN_insert_extract_submenu(awm, "gene_extract_marked", "Extract marked genes", "E", "gene_extract.hlp");

        if (!for_ARB_NTREE) {   // only in ARB_GENE_MAP:
            awm->insert_separator();
            GEN_insert_mark_submenu(awm, "gene_mark_hidden", "Mark hidden genes", "", "gene_hide.hlp", GEN_MARK_HIDDEN);
            GEN_insert_mark_submenu(awm, "gene_mark_visible", "Mark visible genes", "", "gene_hide.hlp", GEN_MARK_VISIBLE);

            awm->insert_separator();
            GEN_insert_mark_submenu(awm, "gene_unmark_hidden", "Unmark hidden genes", "", "gene_hide.hlp", GEN_UNMARK_HIDDEN);
            GEN_insert_mark_submenu(awm, "gene_unmark_visible", "Unmark visible genes", "", "gene_hide.hlp", GEN_UNMARK_VISIBLE);
        }
    }
}

#undef AWMIMT

void GEN_create_hide_submenu(AW_window_menu_modes *awm) {
    awm->create_menu(0,"Hide","H","no.hlp", AWM_ALL);
    {
        GEN_insert_hide_submenu(awm, "gene_hide_marked", "Hide marked genes", "H", "gene_hide.hlp", GEN_HIDE_MARKED);
        GEN_insert_hide_submenu(awm, "gene_unhide_marked", "Unhide marked genes", "U", "gene_hide.hlp", GEN_UNHIDE_MARKED);
        GEN_insert_hide_submenu(awm, "gene_invhide_marked", "Invert-hide marked genes", "v", "gene_hide.hlp", GEN_INVERT_HIDE_MARKED);

        awm->insert_separator();
        GEN_insert_hide_submenu(awm, "gene_hide_all", "Hide all genes", "", "gene_hide.hlp", GEN_HIDE_ALL);
        GEN_insert_hide_submenu(awm, "gene_unhide_all", "Unhide all genes", "", "gene_hide.hlp", GEN_UNHIDE_ALL);
        GEN_insert_hide_submenu(awm, "gene_invhide_all", "Invert-hide all genes", "", "gene_hide.hlp", GEN_INVERT_HIDE_ALL);
    }
}

void GEN_set_display_style(AW_window *aww, AW_CL cl_style) {
    GEN_DisplayStyle  style = (GEN_DisplayStyle)cl_style;
    GEN_map_window   *win   = dynamic_cast<GEN_map_window*>(aww);
    gen_assert(win);

    win->get_root()->awar(AWAR_GENMAP_DISPLAY_TYPE(win->get_nr()))->write_int(style);
    win->get_graphic()->set_display_style(style);
    GEN_map_window_zoom_reset_and_refresh(win);
}

// ____________________________________________________________
// start of implementation of class GEN_map_window::

void GEN_map_window::init(AW_root *awr) {
    {
        char *window_name = (window_nr == 0) ? strdup("ARB Gene Map") : GBS_global_string_copy("ARB Gene Map %i", window_nr);
        AW_window_menu_modes::init(awr, "ARB_GENE_MAP", window_name, 200, 200);
        free(window_name);
    }

    GEN_create_genemap_local_awars(awr, AW_ROOT_DEFAULT, window_nr);

    gen_graphic = new GEN_graphic(awr, gb_main, GEN_gene_container_cb_installer, window_nr);

    AW_gc_manager aw_gc_manager;
    gen_canvas = new AWT_canvas(gb_main, this, gen_graphic, aw_gc_manager, AWAR_SPECIES_NAME);

    GEN_add_local_awar_callbacks(awr, AW_ROOT_DEFAULT, this);

    {
        GB_transaction ta(gb_main);
        gen_graphic->reinit_gen_root(gen_canvas, false);
    }

    gen_canvas->recalc_size();
    gen_canvas->refresh();
    gen_canvas->set_mode(AWT_MODE_SELECT); // Default-Mode

    // --------------
    //      menus
    // --------------

    // File Menu
    create_menu( 0, "File", "F", "no.hlp",  AWM_ALL );
    insert_menu_topic( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);
    insert_menu_topic( "new_view", "New view", "v","new_view.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_map,(AW_CL)window_nr+1);

    GEN_create_genes_submenu(this, false/*, ntree_canvas*/); // Genes
    GEN_create_gene_species_submenu(this, false/*, ntree_canvas*/); // Gene-species
    GEN_create_organism_submenu(this, false/*, ntree_canvas*/); // Organisms

    EXP_create_experiments_submenu(this, false); // Experiments

    GEN_create_hide_submenu(this); // Hide Menu

    // Properties Menu
    create_menu("props","Properties","r","no.hlp", AWM_ALL);
    insert_menu_topic("gene_props_menu",   "Menu: Colors and Fonts ...",   "M","props_frame.hlp",  AWM_ALL, AW_POPUP, (AW_CL)AW_preset_window, 0 );
    // @@@ FIXME: replace AW_preset_window by local function returning same window for all mapped views
    insert_menu_topic("gene_props",        "GENEMAP: Colors and Fonts ...","C","gene_props_data.hlp",AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager );
    // @@@ FIXME: replace AW_create_gc_window by local function returning same window for all mapped views
    insert_menu_topic("gene_layout",       "Layout",   "L", "gene_layout.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_layout_window, 0);
    insert_menu_topic("gene_options",      "Options",  "O", "gene_options.hlp", AWM_ALL, AW_POPUP, (AW_CL)GEN_create_options_window, 0);
    insert_menu_topic("gene_nds",          "NDS ( Select Gene Information ) ...",      "N","props_nds.hlp",    AWM_ALL, AW_POPUP, (AW_CL)GEN_open_nds_window, (AW_CL)gb_main );
    insert_menu_topic("gene_save_props",   "Save Defaults (in ~/.arb_prop/ntree.arb)", "D","savedef.hlp",  AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );

    // ---------------------
    //      mode buttons
    // ---------------------

    create_mode(0, "select.bitmap", "gen_mode.hlp", AWM_ALL, GEN_mode_event, (AW_CL)this, (AW_CL)AWT_MODE_SELECT);
    create_mode(0, "pzoom.bitmap",  "gen_mode.hlp", AWM_ALL, GEN_mode_event, (AW_CL)this, (AW_CL)AWT_MODE_ZOOM);
    create_mode(0, "info.bitmap",   "gen_mode.hlp", AWM_ALL, GEN_mode_event, (AW_CL)this, (AW_CL)AWT_MODE_MOD);

    // ------------------
    //      info area
    // ------------------

    set_info_area_height( 250 );
    at(11,2);
    auto_space(2,-2);
    shadow_width(1);

    // close + undo button, info area, define line y-positions:

    int cur_x, cur_y, start_x, first_line_y, second_line_y, third_line_y;
    get_at_position( &start_x,&first_line_y);
    button_length(6);

    at(start_x, first_line_y);
    help_text("quit.hlp");
    callback((AW_CB0)AW_POPDOWN);
    create_button("Close", "Close");

    get_at_position( &cur_x,&cur_y );

    int gene_x = cur_x;
    at_newline();
    get_at_position( &cur_x,&second_line_y);

    at(start_x, second_line_y);
    help_text("undo.hlp");
    callback(GEN_undo_cb,(AW_CL)GB_UNDO_UNDO);
    create_button("Undo", "Undo");

    at_newline();
    get_at_position( &cur_x,&third_line_y);

    button_length(AWAR_FOOTER_MAX_LEN);
    create_button(0,AWAR_FOOTER);

    at_newline();
    get_at_position( &cur_x,&cur_y );
    set_info_area_height( cur_y+6 );

    // gene+species buttons:
    button_length(20);

    at(gene_x, first_line_y);
    help_text("organism_search.hlp");
    callback( AW_POPUP, (AW_CL)ad_create_query_window, 0); // @@@ hier sollte eine Art "Organism-Search" verwendet werden (AWT_organism_selector anpassen)
    create_button("SEARCH_ORGANISM", AWAR_LOCAL_ORGANISM_NAME(window_nr));

    at(gene_x, second_line_y);
    help_text("gene_search.hlp");
    callback( AW_POPUP, (AW_CL)GEN_create_gene_query_window, 0);
    create_button("SEARCH_GENE",AWAR_LOCAL_GENE_NAME(window_nr));

    get_at_position( &cur_x,&cur_y );
    int lock_x = cur_x;

    at(lock_x, first_line_y);
    create_toggle(AWAR_LOCAL_ORGANISM_LOCK(window_nr));

    at(lock_x, second_line_y);
    create_toggle(AWAR_LOCAL_GENE_LOCK(window_nr));

    get_at_position( &cur_x,&cur_y );
    int dtype_x1 = cur_x;

    // display type buttons:

    button_length(4);

    at(dtype_x1, first_line_y);
    help_text("gen_disp_radial.hlp");
    callback(GEN_set_display_style,(AW_CL)GEN_DISPLAY_STYLE_RADIAL);
    create_button("RADIAL_DISPLAY_TYPE", "#gen_radial.bitmap",0);

    help_text("gen_disp_book.hlp");
    callback(GEN_set_display_style,(AW_CL)GEN_DISPLAY_STYLE_BOOK);
    create_button("RADIAL_DISPLAY_TYPE", "#gen_book.bitmap",0);

    get_at_position( &cur_x,&cur_y );
    int jump_x = cur_x;

    at(dtype_x1, second_line_y);
    help_text("gen_disp_vertical.hlp");
    callback(GEN_set_display_style,(AW_CL)GEN_DISPLAY_STYLE_VERTICAL);
    create_button("RADIAL_DISPLAY_TYPE", "#gen_vertical.bitmap",0);

    // jump button:

    button_length(0);

    at(jump_x, first_line_y);
    help_text("gen_jump.hlp");
    callback(GEN_jump_cb, (AW_CL)true);
    create_button("Jump", "Jump");

    // help buttons:

    get_at_position( &cur_x,&cur_y );
    int help_x = cur_x;

    at(help_x, first_line_y);
    help_text("help.hlp");
    callback(AW_POPUP_HELP,(AW_CL)"gene_map.hlp");
    create_button("HELP", "HELP","H");

    at(help_x, second_line_y);
    callback( AW_help_entry_pressed );
    create_button(0,"?");

}

// -end- of implementation of class GEN_map_window:.


AW_window *GEN_map(AW_root *aw_root, int window_number) {
    // window_number shall be 0 for first window (called from ARB_NTREE)
    // additional views are always created by the previous window created with GEN_map

    GEN_map_manager *manager = 0;
    if (!GEN_map_manager::initialized()) { // first call
        gen_assert(window_number == 0); // has to be 0 for first call

        GEN_map_manager::initialize(aw_root);
        manager = GEN_map_manager::get_map_manager(); // creates the manager

        // global initialization (AWARS etc.) done here :

        GEN_create_genemap_global_awars(aw_root, AW_ROOT_DEFAULT);
        GEN_add_global_awar_callbacks(aw_root);
        {
            GB_transaction dummy(gb_main);
            GEN_make_node_text_init(gb_main);
        }
    }
    else {
        manager = GEN_map_manager::get_map_manager();
    }

    GEN_map_window *gmw = manager->get_map_window(window_number);
    return gmw;

#if 0
    static AW_window *aw_gen = 0;

    if (!aw_gen) {              // do not open window twice
        {
            GB_transaction dummy(gb_main);
            GEN_make_node_text_init(gb_main);
        }

        aw_gen = GEN_map_create_main_window(aw_root, nt_canvas);
        if (!aw_gen) {
            aw_message("Couldn't start Gene-Map");
            return 0;
        }

    }

    aw_gen->show();
    return aw_gen;
#endif
}

AW_window *GEN_map_first(AW_root *aw_root) {
    AW_window *aw_map1 = GEN_map(aw_root, 0);

#if defined(DEVEL_RALF)
    AW_window *aw_map2 = GEN_map(aw_root, 1);
    aw_map2->show();
#endif // DEVEL_RALF

    return aw_map1;
}
