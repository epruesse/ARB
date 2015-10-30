// =============================================================== //
//                                                                 //
//   File      : GEN_map.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 2001           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "GEN_local.hxx"
#include "GEN_gene.hxx"
#include "GEN_graphic.hxx"
#include "GEN_nds.hxx"
#include "EXP_local.hxx"

#include <dbui.h>
#include <aw_preset.hxx>
#include <aw_awars.hxx>
#include <aw_question.hxx>
#include <AW_rename.hxx>
#include <awt.hxx>
#include <awt_input_mask.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <adGene.h>
#include <db_query.h>
#include <rootAsWin.h>
#include <mode_text.h>
#include <ad_cb.h>

#include <map>

using namespace std;

extern GBDATA *GLOBAL_gb_main;

// -----------------------
//      GEN_map_window

class GEN_map_window : public AW_window_menu_modes {
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
    {}

    void init(AW_root *root, GBDATA *gb_main);

    GEN_graphic *get_graphic() const { gen_assert(gen_graphic); return gen_graphic; }
    AWT_canvas *get_canvas() const { gen_assert(gen_canvas); return gen_canvas; }
    int get_nr() const { return window_nr; }

    GBDATA *get_gb_main() const { return get_canvas()->gb_main; }
};

// ------------------------
//      GEN_map_manager

DECLARE_CBTYPE_FVV_AND_BUILDERS(GenmapWindowCallback, void, GEN_map_window*); // generates makeGenmapWindowCallback

class GEN_map_manager {
    static AW_root         *aw_root;
    static GBDATA          *gb_main;
    static GEN_map_manager *the_manager;            // there can be only one!

    int              window_counter;
    GEN_map_window **windows;   // array of managed windows
    int              windows_size; // size of 'windows' array

    GEN_map_manager(const GEN_map_manager& other); // copying not allowed
    GEN_map_manager& operator = (const GEN_map_manager& other); // assignment not allowed

public:

    GEN_map_manager();

    static bool initialized() { return aw_root != 0; }
    static void initialize(AW_root *aw_root_, GBDATA *gb_main_) { aw_root = aw_root_; gb_main = gb_main_; }

    static GEN_map_manager *get_map_manager();

    GEN_map_window *get_map_window(int nr);

    int no_of_managed_windows() { return window_counter; }

    static void with_all_mapped_windows(const GenmapWindowCallback& gwcb);
    static void with_all_mapped_windows(void (*cb)(GEN_map_window*)) { with_all_mapped_windows(makeGenmapWindowCallback(cb)); }
};

// ____________________________________________________________
// start of implementation of class GEN_map_manager:

AW_root         *GEN_map_manager::aw_root     = 0;
GBDATA          *GEN_map_manager::gb_main     = 0;
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
        new GEN_map_manager;  // sets the manager
        gen_assert(the_manager);
    }
    return the_manager;
}

void GEN_map_manager::with_all_mapped_windows(const GenmapWindowCallback& gwcb) {
    if (aw_root) { // no genemap opened yet
        GEN_map_manager *mm       = get_map_manager();
        int              winCount = mm->no_of_managed_windows();
        for (int nr = 0; nr<winCount; ++nr) {
            gwcb(mm->get_map_window(nr));
        }
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
    windows[nr]->init(aw_root, gb_main);

    return windows[nr];
}

// -end- of implementation of class GEN_map_manager.



const char *GEN_window_local_awar_name(const char *awar_name, int window_nr) {
    return GBS_global_string("%s_%i", awar_name, window_nr);
}

static void reinit_NDS_4_window(GEN_map_window *win) {
    win->get_graphic()->get_gen_root()->reinit_NDS();
}

static void GEN_NDS_changed(GBDATA *gb_viewkey) {
    GBDATA *gb_main = GB_get_root(gb_viewkey);
    GEN_make_node_text_init(gb_main);
    GEN_map_manager::with_all_mapped_windows(reinit_NDS_4_window);
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

static void GEN_gene_container_changed_cb(GBDATA*, gene_container_changed_cb_data *cb_data) {
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
            GB_add_callback(curr_cb_data->gb_gene_data, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(GEN_gene_container_changed_cb, curr_cb_data));
        }
    }
    else {
        if (curr_cb_data->gb_gene_data) { // if callback is installed
            GB_remove_callback(curr_cb_data->gb_gene_data, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(GEN_gene_container_changed_cb, curr_cb_data));
            curr_cb_data->gb_gene_data = 0;
        }
    }
}

static void GEN_jump_cb(AW_window *aww, bool force_center_if_fits) {
    // force_center_if_fits==true => center gene if it fits into display

    GEN_map_window *win = dynamic_cast<GEN_map_window*>(aww); // @@@ use same callback-type as for with_all_mapped_windows
    gen_assert(win);

    AW_device             *device = win->get_graphic()->get_device();
    const AW_screen_area&  screen = device->get_area_size();
#if defined(DEBUG)
    printf("Window %i: screen is: %i/%i -> %i/%i\n", win->get_nr(), screen.l, screen.t, screen.r, screen.b);
#endif // DEBUG

    const GEN_root *gen_root = win->get_graphic()->get_gen_root();
    if (gen_root) {
        const AW::Rectangle& wrange = gen_root->get_selected_range();

        if (wrange.valid()) {
#if defined(DEBUG)
            printf("Window %i: Draw world range of selected gene is: %f/%f -> %f/%f\n",
                   win->get_nr(), wrange.left(), wrange.top(), wrange.right(), wrange.bottom());
#endif // DEBUG

            AW::Rectangle srange = device->transform(wrange);
#if defined(DEBUG)
            printf("Window %i: Draw screen range of selected gene is: %f/%f -> %f/%f\n",
                   win->get_nr(), srange.left(), srange.top(), srange.right(), srange.bottom());
#endif // DEBUG

            AWT_canvas *canvas  = win->get_canvas();
            int         scrollx = 0;
            int         scrolly = 0;

            if (srange.top() < 0) {
                scrolly = AW_INT(srange.top())-2;
            }
            else if (srange.bottom() > screen.b) {
                scrolly = AW_INT(srange.bottom())-screen.b+2;
            }
            if (srange.left() < 0) {
                scrollx = AW_INT(srange.left())-2;
            }
            else if (srange.right() > screen.r) {
                scrollx = AW_INT(srange.right())-screen.r+2;
            }

            if (force_center_if_fits) {
                if (!scrollx) { // no scrolling needed, i.e. gene fits onto display horizontally
                    int gene_center_x   = AW_INT((srange.left()+srange.right())/2);
                    int screen_center_x = (screen.l+screen.r)/2;
                    scrollx             = gene_center_x-screen_center_x;
#if defined(DEBUG)
                    printf("center x\n");
#endif // DEBUG
                }
                if (!scrolly) { // no scrolling needed, i.e. gene fits onto display vertically
                    int gene_center_y   = AW_INT((srange.top()+srange.bottom())/2);
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

            canvas->scroll(scrollx, scrolly);
        }
    }
    win->get_canvas()->refresh();
}

static void GEN_jump_cb_auto(AW_root *root, GEN_map_window *win, bool force_refresh) {
    int jump = root->awar(AWAR_GENMAP_AUTO_JUMP)->read_int();
    if (jump) {
        win->get_canvas()->refresh(); // needed to recalculate position
        GEN_jump_cb(win, false);
    }
    else if (force_refresh) win->get_canvas()->refresh();
}

static void GEN_local_organism_or_gene_name_changed_cb(AW_root *awr, GEN_map_window *win) {
    win->get_graphic()->reinit_gen_root(win->get_canvas(), false);
    GEN_jump_cb_auto(awr, win, true);
}

static void GEN_map_window_zoom_reset_and_refresh(GEN_map_window *gmw) {
    gmw->get_canvas()->zoom_reset_and_refresh();
}

#define DISPLAY_TYPE_BIT(disp_type) (1<<(disp_type))
#define ALL_DISPLAY_TYPES           (DISPLAY_TYPE_BIT(GEN_DISPLAY_STYLES)-1)

static void GEN_map_window_refresh(GEN_map_window *win) {
    win->get_canvas()->refresh();
}
void GEN_refresh_all_windows() {
    GEN_map_manager::with_all_mapped_windows(GEN_map_window_refresh);
}

static void GEN_map_window_refresh_if_display_type(GEN_map_window *win, int display_type_mask) {
    int my_display_type = win->get_graphic()->get_display_style();
    if (display_type_mask & DISPLAY_TYPE_BIT(my_display_type)) {
        AWT_canvas *canvas = win->get_canvas();
        canvas->refresh();
    }
}

static void GEN_update_unlocked_organism_and_gene_awars(GEN_map_window *win, const char *organismName, const char *geneName) {
    AW_root *aw_root   = win->get_graphic()->get_aw_root();
    int      window_nr = win->get_nr();
    if (!aw_root->awar(AWAR_LOCAL_ORGANISM_LOCK(window_nr))->read_int()) {
        aw_root->awar(AWAR_LOCAL_ORGANISM_NAME(window_nr))->write_string(organismName);
    }
    if (!aw_root->awar(AWAR_LOCAL_GENE_LOCK(window_nr))->read_int()) {
        aw_root->awar(AWAR_LOCAL_GENE_NAME(window_nr))->write_string(geneName);
    }
}

static void GEN_organism_or_gene_changed_cb(AW_root *awr) {
    char *organism = awr->awar(AWAR_ORGANISM_NAME)->read_string();
    char *gene     = awr->awar(AWAR_GENE_NAME)->read_string();

    GEN_map_manager::with_all_mapped_windows(makeGenmapWindowCallback(GEN_update_unlocked_organism_and_gene_awars, organism, gene));

    free(gene);
    free(organism);
}


static void GEN_local_lock_changed_cb(AW_root *awr, GEN_map_window *win, bool gene_lock) {
    int window_nr = win->get_nr();

    const char *local_awar_name      = 0;
    const char *local_lock_awar_name = 0;
    const char *global_awar_name     = 0;

    if (gene_lock) {
        local_awar_name      = AWAR_LOCAL_GENE_NAME(window_nr);
        local_lock_awar_name = AWAR_LOCAL_GENE_LOCK(window_nr);
        global_awar_name     = AWAR_GENE_NAME;
    }
    else { // otherwise organism
        local_awar_name      = AWAR_LOCAL_ORGANISM_NAME(window_nr);
        local_lock_awar_name = AWAR_LOCAL_ORGANISM_LOCK(window_nr);
        global_awar_name     = AWAR_ORGANISM_NAME;
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

// -------------------------------------
//      display parameter change cb

static void GEN_display_param_changed_cb(AW_root*, int display_type_mask) {
    GEN_map_manager::with_all_mapped_windows(makeGenmapWindowCallback(GEN_map_window_refresh_if_display_type, display_type_mask));
}
inline void set_display_update_callback(AW_root *awr, const char *awar_name, int display_type_mask) {
    awr->awar(awar_name)->add_callback(makeRootCallback(GEN_display_param_changed_cb, display_type_mask));
}

// --------------------------
//      View-local AWARS

static void GEN_create_genemap_local_awars(AW_root *aw_root, AW_default /* def */, int window_nr) {
    // awars local to each view

    aw_root->awar_int(AWAR_GENMAP_DISPLAY_TYPE(window_nr), GEN_DISPLAY_STYLE_RADIAL); // @@@ FIXME: make local

    aw_root->awar_string(AWAR_LOCAL_ORGANISM_NAME(window_nr), "");
    aw_root->awar_string(AWAR_LOCAL_GENE_NAME(window_nr), "");

    aw_root->awar_int(AWAR_LOCAL_ORGANISM_LOCK(window_nr), 0);
    aw_root->awar_int(AWAR_LOCAL_GENE_LOCK(window_nr), 0);
}

static void GEN_add_local_awar_callbacks(AW_root *awr, AW_default /* def */, GEN_map_window *win) {
    int window_nr = win->get_nr();

    RootCallback nameChangedCb = makeRootCallback(GEN_local_organism_or_gene_name_changed_cb, win);
    awr->awar(AWAR_LOCAL_ORGANISM_NAME(window_nr))->add_callback(nameChangedCb);
    awr->awar(AWAR_LOCAL_GENE_NAME(window_nr))    ->add_callback(nameChangedCb);

    AW_awar *awar_lock_orga = awr->awar(AWAR_LOCAL_ORGANISM_LOCK(window_nr));
    AW_awar *awar_lock_gene = awr->awar(AWAR_LOCAL_GENE_LOCK(window_nr));

    awar_lock_orga->add_callback(makeRootCallback(GEN_local_lock_changed_cb, win, false));
    awar_lock_gene->add_callback(makeRootCallback(GEN_local_lock_changed_cb, win, true));

    awar_lock_orga->touch();
    awar_lock_gene->touch();
}

// ----------------------
//      global AWARS

static void GEN_create_genemap_global_awars(AW_root *aw_root, AW_default def, GBDATA *gb_main) {
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

    GEN_create_nds_vars(aw_root, def, gb_main, makeDatabaseCallback(GEN_NDS_changed));
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

void GEN_disconnect_from_DB() {
    // called before ntree disconnects awars from DB
    // (deactivates callbacks which otherwise would crash)

    if (GEN_map_manager::initialized()) {
        int window_count = GEN_map_manager::get_map_manager()->no_of_managed_windows();

        AW_root *aw_root = AW_root::SINGLETON;
        aw_root->awar(AWAR_ORGANISM_NAME)->write_string("");

        for (int window_nr = 0; window_nr<window_count; ++window_nr) {
            aw_root->awar(AWAR_LOCAL_ORGANISM_LOCK(window_nr))->write_int(0);
            aw_root->awar(AWAR_LOCAL_ORGANISM_NAME(window_nr))->write_string("");
        }
    }
}

// --------------------------------------------------------------------------------

static void GEN_mode_event(AW_window *aws, GEN_map_window *win, AWT_COMMAND_MODE mode) {
    const char *text = 0;
    switch (mode) {
        case AWT_MODE_SELECT: text = MODE_TEXT_1BUTTON("SELECT", "click to select a gene"); break;
        case AWT_MODE_INFO:   text = MODE_TEXT_1BUTTON("INFO",   "click for info");         break;

        case AWT_MODE_ZOOM: text = MODE_TEXT_STANDARD_ZOOMMODE(); break;

        default: text = no_mode_text_defined(); break;
    }

    gen_assert(strlen(text) < AWAR_FOOTER_MAX_LEN); // text too long!

    aws->get_root()->awar(AWAR_FOOTER)->write_string(text);
    AWT_canvas *canvas = win->get_canvas();
    canvas->set_mode(mode);
    canvas->refresh();
}

static void GEN_undo_cb(AW_window*, GB_UNDO_TYPE undo_type, GBDATA *gb_main) {
    GB_ERROR error = GB_undo(gb_main, undo_type);
    if (error) {
        aw_message(error);
    }
    else {
        GB_begin_transaction(gb_main);
        GB_commit_transaction(gb_main);
    }
}

static AW_window *GEN_create_options_window(AW_root *awr) {
    static AW_window_simple *aws = 0;

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(awr, "GEN_OPTS", "Genemap options");
        aws->load_xfig("gen_options.fig");

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("gen_options.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        // all displays:
        aws->at("arrow_size");      aws->create_input_field(AWAR_GENMAP_ARROW_SIZE, 5);

        aws->label_length(26);

        aws->at("show_hidden");
        aws->label("Show hidden genes");
        aws->create_toggle(AWAR_GENMAP_SHOW_HIDDEN);

        aws->at("show_all");
        aws->label("Show NDS for all genes");
        aws->create_toggle(AWAR_GENMAP_SHOW_ALL_NDS);

        aws->at("autojump");
        aws->label("Auto jump to selected gene");
        aws->create_toggle(AWAR_GENMAP_AUTO_JUMP);

        // book-style:
        aws->at("base_pos");        aws->create_input_field(AWAR_GENMAP_BOOK_BASES_PER_LINE, 15);
        aws->at("width_factor");    aws->create_input_field(AWAR_GENMAP_BOOK_WIDTH_FACTOR, 7);
        aws->at("line_height");     aws->create_input_field(AWAR_GENMAP_BOOK_LINE_HEIGHT, 5);
        aws->at("line_space");      aws->create_input_field(AWAR_GENMAP_BOOK_LINE_SPACE, 5);

        // vertical-style:
        aws->at("factor_x");        aws->create_input_field(AWAR_GENMAP_VERTICAL_FACTOR_X, 5);
        aws->at("factor_y");        aws->create_input_field(AWAR_GENMAP_VERTICAL_FACTOR_Y, 5);

        // radial style:
        aws->at("inside");          aws->create_input_field(AWAR_GENMAP_RADIAL_INSIDE, 5);
        aws->at("outside");         aws->create_input_field(AWAR_GENMAP_RADIAL_OUTSIDE, 5);
    }
    return aws;
}

static AW_window *GEN_create_gc_window(AW_root *awr, AW_gc_manager gcman) {
    // only create one gc window for all genemap views
    static AW_window *awgc = NULL;
    if (!awgc) awgc        = AW_create_gc_window_named(awr, gcman, "GENMAP_PROPS_GC", "Genemap colors and fonts"); // use named gc window (otherwise clashes with ARB_NTREE gc window)
    return awgc;
}

enum GEN_PERFORM_MODE {
    GEN_PERFORM_ALL_ORGANISMS,
    GEN_PERFORM_CURRENT_ORGANISM,
    GEN_PERFORM_ALL_BUT_CURRENT_ORGANISM,
    GEN_PERFORM_MARKED_ORGANISMS,

    GEN_PERFORM_MODES, // counter
};

static const char *GEN_PERFORM_MODE_id[GEN_PERFORM_MODES] = {
    "org_all",
    "org_current",
    "org_butcur",
    "org_marked",
};

inline string performmode_relative_id(const char *id, GEN_PERFORM_MODE pmode) {
    return GBS_global_string("%s_%s", GEN_PERFORM_MODE_id[pmode], id);
}

enum GEN_MARK_MODE {
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
};

enum GEN_HIDE_MODE {
    GEN_HIDE_ALL,
    GEN_UNHIDE_ALL,
    GEN_INVERT_HIDE_ALL,

    GEN_HIDE_MARKED,
    GEN_UNHIDE_MARKED,
    GEN_INVERT_HIDE_MARKED
};

// --------------------------------------------------------------------------------

inline bool nameIsUnique(const char *short_name, GBDATA *gb_species_data) {
    return GBT_find_species_rel_species_data(gb_species_data, short_name)==0;
}

static GB_ERROR GEN_species_add_entry(GBDATA *gb_pseudo, const char *key, const char *value) {
    GB_ERROR  error = 0;
    GB_clear_error();
    GBDATA   *gbd   = GB_entry(gb_pseudo, key);

    if (!gbd) { // key does not exist yet -> create
        gbd             = GB_create(gb_pseudo, key, GB_STRING);
        if (!gbd) error = GB_await_error();
    }
    else { // key exists
        if (GB_read_type(gbd) != GB_STRING) { // test correct key type
            error = GB_export_errorf("field '%s' exists and has wrong type", key);
        }
    }

    if (!error) error = GB_write_string(gbd, value);

    return error;
}

static AW_repeated_question *ask_about_existing_gene_species = 0;
static AW_repeated_question *ask_to_overwrite_alignment      = 0;

struct EG2PS_data : virtual Noncopyable {
    arb_progress        progress;
    GBDATA             *gb_species_data;
    char               *ali;
    UniqueNameDetector  existing;
    GB_HASH            *pseudo_hash;
    int                 duplicateSpecies; // counts created gene-species with identical ACC
    bool                nameProblem; // nameserver and DB disagree about names

    EG2PS_data(const char *ali_, GBDATA *gb_species_data_, int marked_genes_)
        : progress(marked_genes_), 
          gb_species_data(gb_species_data_), 
          ali(strdup(ali_)), 
          existing(gb_species_data, marked_genes_), 
          duplicateSpecies(0), 
          nameProblem(false)
    {
        pseudo_hash = GEN_create_pseudo_species_hash(GB_get_root(gb_species_data), marked_genes_);
    }

    ~EG2PS_data() {
        if (duplicateSpecies>0) {
            aw_message(GBS_global_string("There are %i duplicated gene-species (with identical sequence and accession number)\n"
                                         "Duplicated gene-species got names with numerical postfixes ('.1', '.2', ...)"
                                         , duplicateSpecies));
        }
        if (nameProblem) {
            aw_message("Naming problems occurred.\nYou have to call 'Species/Synchronize IDs'!");
        }
        GBS_free_hash(pseudo_hash);
        free(ali);
    }
};

static const char* readACC(GBDATA *gb_species_data, const char *name) {
    const char *other_acc    = 0;
    GBDATA *gb_other_species = GBT_find_species_rel_species_data(gb_species_data, name);
    if (gb_other_species) {
        GBDATA *gb_other_acc = GB_entry(gb_other_species, "acc");
        if (gb_other_acc) other_acc = GB_read_char_pntr(gb_other_acc);
    }
    return other_acc;
}

static void gen_extract_gene_2_pseudoSpecies(GBDATA *gb_species, GBDATA *gb_gene, EG2PS_data *eg2ps) {
    const char *gene_name         = GBT_read_name(gb_gene);
    const char *species_name      = GBT_read_name(gb_species);
    const char *full_species_name = GBT_read_char_pntr(gb_species, "full_name");

    if (!full_species_name) full_species_name = species_name;

    char     *full_name = GBS_global_string_copy("%s [%s]", full_species_name, gene_name);
    char     *sequence  = GBT_read_gene_sequence(gb_gene, true, 0);
    GB_ERROR  error     = 0;

    if (!sequence) error = GB_await_error();
    else {
        const char *ali = eg2ps->ali;
        long        id  = GBS_checksum(sequence, 1, ".-");
        char        acc[100];

        sprintf(acc, "ARB_GENE_%lX", id);

        // test if this gene has been already extracted to a gene-species

        GBDATA *gb_main                 = GB_get_root(gb_species);
        GBDATA *gb_exist_geneSpec       = GEN_find_pseudo_species(gb_main, species_name, gene_name, eg2ps->pseudo_hash);
        bool    create_new_gene_species = true;
        char   *short_name              = 0;

        if (gb_exist_geneSpec) {
            const char *existing_name = GBT_read_name(gb_exist_geneSpec);

            gen_assert(ask_about_existing_gene_species);
            gen_assert(ask_to_overwrite_alignment);

            char *question = GBS_global_string_copy("Already have a gene-species for %s/%s ('%s')", species_name, gene_name, existing_name);
            int   answer   = ask_about_existing_gene_species->get_answer("existing_pseudo_species", question, "Overwrite species,Insert new alignment,Skip,Create new", "all", true);

            create_new_gene_species = false;

            switch (answer) {
                case 0: {   // Overwrite species
                    // @@@ FIXME:  delete species needed here
                    create_new_gene_species = true;
                    short_name              = strdup(existing_name);
                    break;
                }
                case 1: {     // Insert new alignment or overwrite alignment
                    GBDATA     *gb_ali = GB_entry(gb_exist_geneSpec, ali);
                    if (gb_ali) { // the alignment already exists
                        char *question2        = GBS_global_string_copy("Gene-species '%s' already has data in '%s'", existing_name, ali);
                        int   overwrite_answer = ask_to_overwrite_alignment->get_answer("overwrite_gene_data", question2, "Overwrite data,Skip", "all", true);

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
                default: gen_assert(0);
            }
            free(question);
        }

        if (!error) {
            if (create_new_gene_species) {
                if (!short_name) { // create a new name
                    error = AWTC_generate_one_name(gb_main, full_name, acc, 0, short_name);
                    if (!error) { // name has been created
                        if (eg2ps->existing.name_known(short_name)) { // nameserver-generated name is already in use
                            const char *other_acc    = readACC(eg2ps->gb_species_data, short_name);
                            if (other_acc) {
                                if (strcmp(acc, other_acc) == 0) { // duplicate (gene-)species -> generate postfixed name
                                    char *newName = 0;
                                    for (int postfix = 1; ; ++postfix) {
                                        newName = GBS_global_string_copy("%s.%i", short_name, postfix);
                                        if (!eg2ps->existing.name_known(newName)) {
                                            eg2ps->duplicateSpecies++;
                                            break;
                                        }

                                        other_acc = readACC(eg2ps->gb_species_data, newName);
                                        if (!other_acc || strcmp(acc, other_acc) != 0) {
                                            eg2ps->nameProblem = true;
                                            error              = GBS_global_string("Unexpected acc-mismatch for '%s'", newName);
                                            break;
                                        }
                                    }

                                    if (!error) freeset(short_name, newName);
                                    else free(newName);
                                }
                                else { // different acc, but uses name generated by nameserver
                                    eg2ps->nameProblem = true;
                                    error              = GBS_global_string("acc of '%s' differs from acc stored in nameserver", short_name);
                                }
                            }
                            else { // can't detect acc of existing species
                                eg2ps->nameProblem = true;
                                error       = GBS_global_string("can't detect acc of species '%s'", short_name);
                            }
                        }
                    }
                    if (error) {            // try to make a random name
                        const char *msg = GBS_global_string("%s\nGenerating a random name instead.", error);
                        aw_message(msg);
                        error      = 0;

                        short_name = AWTC_generate_random_name(eg2ps->existing);
                        if (!short_name) error = GBS_global_string("Failed to create a new name for pseudo gene-species '%s'", full_name);
                    }
                }

                if (!error) { // create the species
                    gen_assert(short_name);
                    gb_exist_geneSpec = GBT_find_or_create_species(gb_main, short_name);
                    if (!gb_exist_geneSpec) error = GB_export_errorf("Failed to create pseudo-species '%s'", short_name);
                    else eg2ps->existing.add_name(short_name);
                }
            }
            else {
                gen_assert(gb_exist_geneSpec); // do not generate new or skip -> should only occur when gene-species already existed
            }

            if (!error) { // write sequence data
                GBDATA *gb_data     = GBT_add_data(gb_exist_geneSpec, ali, "data", GB_STRING);
                if (!gb_data) error = GB_await_error();
                else    error       = GB_write_string(gb_data, sequence);
            }

            // write other entries:
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "full_name", full_name);
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "ARB_origin_species", species_name);
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "ARB_origin_gene", gene_name);
            if (!error) GEN_add_pseudo_species_to_hash(gb_exist_geneSpec, eg2ps->pseudo_hash);
            if (!error) error = GEN_species_add_entry(gb_exist_geneSpec, "acc", acc);

            // copy codon_start and transl_table :
            const char *codon_start  = 0;
            const char *transl_table = 0;

            {
                GBDATA *gb_codon_start  = GB_entry(gb_gene, "codon_start");
                GBDATA *gb_transl_table = GB_entry(gb_gene, "transl_table");

                if (gb_codon_start)  codon_start  = GB_read_char_pntr(gb_codon_start);
                if (gb_transl_table) transl_table = GB_read_char_pntr(gb_transl_table);
            }

            if (!error && codon_start)  error = GEN_species_add_entry(gb_exist_geneSpec, "codon_start", codon_start);
            if (!error && transl_table) error = GEN_species_add_entry(gb_exist_geneSpec, "transl_table", transl_table);
        }

        free(short_name);
        free(sequence);
    }
    if (error) aw_message(error);

    free(full_name);
}

static long gen_count_marked_genes = 0; // used to count marked genes

static void do_mark_command_for_one_species(int imode, GBDATA *gb_species, AW_CL cl_user) {
    GEN_MARK_MODE mode = (GEN_MARK_MODE)imode;

    for (GBDATA *gb_gene = GEN_first_gene(gb_species);
         gb_gene;
         gb_gene = GEN_next_gene(gb_gene))
    {
        bool mark_flag     = GB_read_flag(gb_gene) != 0;
        bool org_mark_flag = mark_flag;

        switch (mode) {
            case GEN_MARK:              mark_flag = 1; break;
            case GEN_UNMARK:            mark_flag = 0; break;
            case GEN_INVERT_MARKED:     mark_flag = !mark_flag; break;

            case GEN_COUNT_MARKED: {
                if (mark_flag) ++gen_count_marked_genes;
                break;
            }
            case GEN_EXTRACT_MARKED: {
                if (mark_flag) {
                    EG2PS_data *eg2ps = (EG2PS_data*)cl_user;
                    gen_extract_gene_2_pseudoSpecies(gb_species, gb_gene, eg2ps);
                    eg2ps->progress.inc();
                }
                break;
            }
            default: {
                GBDATA *gb_hidden = GB_entry(gb_gene, ARB_HIDDEN);
                bool    hidden    = gb_hidden ? GB_read_byte(gb_hidden) != 0 : false;

                switch (mode) {
                    case GEN_MARK_HIDDEN:       if (hidden) mark_flag = 1; break;
                    case GEN_UNMARK_HIDDEN:     if (hidden) mark_flag = 0; break;
                    case GEN_MARK_VISIBLE:      if (!hidden) mark_flag = 1; break;
                    case GEN_UNMARK_VISIBLE:    if (!hidden) mark_flag = 0; break;
                    default: gen_assert(0); break;
                }
            }
        }

        if (mark_flag != org_mark_flag) GB_write_flag(gb_gene, mark_flag ? 1 : 0);
    }
}

static void do_hide_command_for_one_species(int imode, GBDATA *gb_species, AW_CL /* cl_user */) {
    GEN_HIDE_MODE mode = (GEN_HIDE_MODE)imode;

    for (GBDATA *gb_gene = GEN_first_gene(gb_species);
         gb_gene;
         gb_gene = GEN_next_gene(gb_gene))
    {
        bool marked = GB_read_flag(gb_gene) != 0;

        GBDATA *gb_hidden  = GB_entry(gb_gene, ARB_HIDDEN);
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

static void GEN_perform_command(AW_window *aww, GBDATA *gb_main, GEN_PERFORM_MODE pmode,
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

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

class GEN_unique_param {
    GEN_unique_param(GBDATA *gb_main_, AW_CL unique_)
        : unique(unique_)
        , gb_main(gb_main_)
    {}

    typedef map<AW_CL, GEN_unique_param> ExistingParams;
    static ExistingParams existing_params; // to ensure exactly one instance per 'command_mode'

    AW_CL   unique;
public:
    GBDATA *gb_main;

    AW_CL get_unique() { return unique; }

    static GEN_unique_param *get(GBDATA *gb_main_, AW_CL unique_) { // get unique instance for each 'unique_'
        pair<ExistingParams::iterator, bool> created = existing_params.insert(ExistingParams::value_type(unique_, GEN_unique_param(gb_main_, unique_)));
        ExistingParams::iterator             wanted  = created.first;
        GEN_unique_param&                    param   = wanted->second;
        return &param;
    }
};
GEN_unique_param::ExistingParams GEN_unique_param::existing_params;


struct GEN_command_mode_param : public GEN_unique_param {
    static GEN_command_mode_param *get(GBDATA *gb_main_, AW_CL command_mode_) { return (GEN_command_mode_param*)GEN_unique_param::get(gb_main_, command_mode_); }
    AW_CL get_command_mode() { return get_unique(); }
};


static void GEN_hide_command(AW_window *aww, GEN_PERFORM_MODE perform_mode, GEN_command_mode_param *param) {
    GEN_perform_command(aww, param->gb_main, perform_mode, do_hide_command_for_one_species, param->get_command_mode(), 0);
}

static void GEN_mark_command(AW_window *aww, GEN_PERFORM_MODE perform_mode, GEN_command_mode_param *param) {
    gen_count_marked_genes = 0;
    GEN_perform_command(aww, param->gb_main, perform_mode, do_mark_command_for_one_species, param->get_command_mode(), 0);

    if ((GEN_MARK_MODE)param->get_command_mode() == GEN_COUNT_MARKED) {
        const char *where = 0;

        switch (perform_mode) {
            case GEN_PERFORM_CURRENT_ORGANISM:         where = "the current species"; break;
            case GEN_PERFORM_MARKED_ORGANISMS:         where = "all marked species"; break;
            case GEN_PERFORM_ALL_ORGANISMS:            where = "all species"; break;
            case GEN_PERFORM_ALL_BUT_CURRENT_ORGANISM: where = "all but the current species"; break;
            default: gen_assert(0); break;
        }
        aw_message(GBS_global_string("There are %li marked genes in %s", gen_count_marked_genes, where));
    }
}

struct GEN_extract_mode_param : public GEN_unique_param {
    static GEN_extract_mode_param *get(GBDATA *gb_main_, GEN_PERFORM_MODE perform_mode) { return (GEN_extract_mode_param*)GEN_unique_param::get(gb_main_, perform_mode); }
    GEN_PERFORM_MODE get_perform_mode() { return (GEN_PERFORM_MODE)get_unique(); }
};

static void gene_extract_cb(AW_window *aww, GEN_extract_mode_param *param) {
    GBDATA   *gb_main = param->gb_main;
    char     *ali     = aww->get_root()->awar(AWAR_GENE_EXTRACT_ALI)->read_string();
    GB_ERROR  error   = GBT_check_alignment_name(ali);

    if (!error) {
        GB_transaction  ta(gb_main);
        GBDATA         *gb_ali = GBT_get_alignment(gb_main, ali);

        if (!gb_ali && !GBT_create_alignment(gb_main, ali, 0, 0, 0, "dna")) {
            error = GB_await_error();
        }
    }

    if (error) {
        aw_message(error);
    }
    else {
        ask_about_existing_gene_species = new AW_repeated_question;
        ask_to_overwrite_alignment      = new AW_repeated_question;

        arb_progress progress("Extracting pseudo-species");
        {
            EG2PS_data *eg2ps = 0;
            {
                gen_count_marked_genes = 0;
                GEN_perform_command(aww, gb_main, param->get_perform_mode(), do_mark_command_for_one_species, GEN_COUNT_MARKED, 0);

                GB_transaction  ta(gb_main);
                GBDATA         *gb_species_data = GBT_get_species_data(gb_main);
                eg2ps                           = new EG2PS_data(ali, gb_species_data, gen_count_marked_genes);
            }

            PersistentNameServerConnection stayAlive;

            GEN_perform_command(aww, gb_main, param->get_perform_mode(), do_mark_command_for_one_species, GEN_EXTRACT_MARKED, (AW_CL)eg2ps);
            delete eg2ps;
        }

        delete ask_to_overwrite_alignment;
        delete ask_about_existing_gene_species;
        ask_to_overwrite_alignment      = 0;
        ask_about_existing_gene_species = 0;
    }
    free(ali);
}

enum MarkCommand {
    UNMARK, // UNMARK and MARK have to be 0 and 1!
    MARK,
    INVERT,
    MARK_UNMARK_REST,
};

static void mark_organisms(AW_window*, MarkCommand mark, GBDATA *gb_main) {
    GB_transaction ta(gb_main);

    if (mark == MARK_UNMARK_REST) {
        GBT_mark_all(gb_main, 0); // unmark all species
        mark = MARK;
    }

    for (GBDATA *gb_org = GEN_first_organism(gb_main);
         gb_org;
         gb_org = GEN_next_organism(gb_org))
    {
        if (mark == INVERT) {
            GB_write_flag(gb_org, !GB_read_flag(gb_org)); // invert mark of organism
        }
        else {
            GB_write_flag(gb_org, mark); // mark/unmark organism
        }
    }
}


static void mark_gene_species(AW_window*, MarkCommand mark, GBDATA *gb_main) {
    GB_transaction ta(gb_main);

    if (mark == MARK_UNMARK_REST) {
        GBT_mark_all(gb_main, 0); // unmark all species
        mark = MARK;
    }

    for (GBDATA *gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        if (mark == INVERT) {
            GB_write_flag(gb_pseudo, !GB_read_flag(gb_pseudo)); // invert mark of pseudo-species
        }
        else {
            GB_write_flag(gb_pseudo, mark); // mark/unmark gene-species
        }
    }
}

static void mark_gene_species_of_marked_genes(AW_window*, GBDATA *gb_main) {
    GB_transaction  ta(gb_main);
    GB_HASH        *organism_hash = GBT_create_organism_hash(gb_main);

    for (GBDATA *gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        GBDATA *gb_gene = GEN_find_origin_gene(gb_pseudo, organism_hash);
        if (GB_read_flag(gb_gene)) {
            GB_write_flag(gb_pseudo, 1); // mark pseudo
        }
    }

    GBS_free_hash(organism_hash);
}



static void mark_organisms_with_marked_genes(AW_window*, GBDATA *gb_main) {
    GB_transaction ta(gb_main);

    for (GBDATA *gb_species = GEN_first_organism(gb_main);
         gb_species;
         gb_species = GEN_next_organism(gb_species))
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
static void mark_gene_species_using_current_alignment(AW_window*, GBDATA *gb_main) {
    GB_transaction  ta(gb_main);
    char           *ali = GBT_get_default_alignment(gb_main);

    for (GBDATA *gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        GBDATA *gb_ali = GB_entry(gb_pseudo, ali);
        if (gb_ali) {
            GBDATA *gb_data = GB_entry(gb_ali, "data");
            if (gb_data) {
                GB_write_flag(gb_pseudo, 1);
            }
        }
    }
}
static void mark_genes_of_marked_gene_species(AW_window*, GBDATA *gb_main) {
    GB_transaction  ta(gb_main);
    GB_HASH        *organism_hash = GBT_create_organism_hash(gb_main);

    for (GBDATA *gb_pseudo = GEN_first_pseudo_species(gb_main);
         gb_pseudo;
         gb_pseudo = GEN_next_pseudo_species(gb_pseudo))
    {
        if (GB_read_flag(gb_pseudo)) {
            GBDATA *gb_gene = GEN_find_origin_gene(gb_pseudo, organism_hash);
            GB_write_flag(gb_gene, 1); // mark gene
        }
    }

    GBS_free_hash(organism_hash);
}

static AW_window *create_gene_extract_window(AW_root *root, GEN_extract_mode_param *param) {
    AW_window_simple *aws       = new AW_window_simple;
    string            window_id = performmode_relative_id("EXTRACT_GENE", param->get_perform_mode());

    aws->init(root, window_id.c_str(), "Extract genes to alignment");
    aws->load_xfig("ad_al_si.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("label");
    aws->create_autosize_button(0, "Please enter the name\nof the alignment to extract to");

    aws->at("input");
    aws->create_input_field(AWAR_GENE_EXTRACT_ALI, 15);

    aws->at("ok");
    aws->callback(makeWindowCallback(gene_extract_cb, param));

    aws->create_button("GO", "GO", "G");

    return aws;
}

static void GEN_insert_extract_submenu(AW_window_menu_modes *awm, GBDATA *gb_main, const char *macro_prefix, const char *submenu_name, const char *hot_key, const char *help_file) {
    awm->insert_sub_menu(submenu_name, hot_key);

    char macro_name_buffer[50];

    sprintf(macro_name_buffer, "%s_of_current", macro_prefix);
    awm->insert_menu_topic(awm->local_id(macro_name_buffer), "of current species...", "c", help_file, AWM_ALL, makeCreateWindowCallback(create_gene_extract_window, GEN_extract_mode_param::get(gb_main, GEN_PERFORM_CURRENT_ORGANISM)));

    sprintf(macro_name_buffer, "%s_of_marked", macro_prefix);
    awm->insert_menu_topic(awm->local_id(macro_name_buffer), "of marked species...", "m", help_file, AWM_ALL, makeCreateWindowCallback(create_gene_extract_window, GEN_extract_mode_param::get(gb_main, GEN_PERFORM_MARKED_ORGANISMS)));

    sprintf(macro_name_buffer, "%s_of_all", macro_prefix);
    awm->insert_menu_topic(awm->local_id(macro_name_buffer), "of all species...", "a", help_file, AWM_ALL, makeCreateWindowCallback(create_gene_extract_window, GEN_extract_mode_param::get(gb_main, GEN_PERFORM_ALL_ORGANISMS)));

    awm->close_sub_menu();
}

static void GEN_insert_multi_submenu(AW_window_menu_modes *awm, const char *macro_prefix, const char *submenu_name, const char *hot_key,
                              const char *help_file, void (*command)(AW_window*, GEN_PERFORM_MODE, GEN_command_mode_param*), GEN_command_mode_param *command_mode)
{
    awm->insert_sub_menu(submenu_name, hot_key);

    char macro_name_buffer[50];

    sprintf(macro_name_buffer, "%s_of_current", macro_prefix);
    awm->insert_menu_topic(macro_name_buffer, "of current species", "c", help_file, AWM_ALL, makeWindowCallback(command, GEN_PERFORM_CURRENT_ORGANISM, command_mode));

    sprintf(macro_name_buffer, "%s_of_all_but_current", macro_prefix);
    awm->insert_menu_topic(macro_name_buffer, "of all but current species", "b", help_file, AWM_ALL, makeWindowCallback(command, GEN_PERFORM_ALL_BUT_CURRENT_ORGANISM, command_mode));

    sprintf(macro_name_buffer, "%s_of_marked", macro_prefix);
    awm->insert_menu_topic(macro_name_buffer, "of marked species", "m", help_file, AWM_ALL, makeWindowCallback(command, GEN_PERFORM_MARKED_ORGANISMS, command_mode));

    sprintf(macro_name_buffer, "%s_of_all", macro_prefix);
    awm->insert_menu_topic(macro_name_buffer, "of all species", "a", help_file, AWM_ALL, makeWindowCallback(command, GEN_PERFORM_ALL_ORGANISMS, command_mode));

    awm->close_sub_menu();
}

static void GEN_insert_mark_submenu(AW_window_menu_modes *awm, GBDATA *gb_main, const char *macro_prefix, const char *submenu_name, const char *hot_key, const char *help_file, GEN_MARK_MODE mark_mode) {
    GEN_insert_multi_submenu(awm, macro_prefix, submenu_name, hot_key, help_file, GEN_mark_command, GEN_command_mode_param::get(gb_main, mark_mode));
}
static void GEN_insert_hide_submenu(AW_window_menu_modes *awm, GBDATA *gb_main, const char *macro_prefix, const char *submenu_name, const char *hot_key, const char *help_file, GEN_HIDE_MODE hide_mode) {
    GEN_insert_multi_submenu(awm, macro_prefix, submenu_name, hot_key, help_file, GEN_hide_command, GEN_command_mode_param::get(gb_main, hide_mode));
}

#if defined(DEBUG)
static AW_window *GEN_create_awar_debug_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aw_root, "DEBUG_AWARS", "DEBUG AWARS");
        aws->at(10, 10);
        aws->auto_space(10, 10);

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

// ---------------------------
//      user mask section

struct GEN_item_type_species_selector : public awt_item_type_selector {
    GEN_item_type_species_selector() : awt_item_type_selector(AWT_IT_GENE) {}
    virtual ~GEN_item_type_species_selector() OVERRIDE {}

    virtual const char *get_self_awar() const OVERRIDE {
        return AWAR_COMBINED_GENE_NAME;
    }
    virtual size_t get_self_awar_content_length() const OVERRIDE {
        return 12 + 1 + 40; // species-name+'/'+gene_name
    }
    virtual GBDATA *current(AW_root *root, GBDATA *gb_main) const OVERRIDE { // give the current item
        char   *species_name = root->awar(AWAR_ORGANISM_NAME)->read_string();
        char   *gene_name   = root->awar(AWAR_GENE_NAME)->read_string();
        GBDATA *gb_gene      = 0;

        if (species_name[0] && gene_name[0]) {
            GB_transaction ta(gb_main);
            GBDATA *gb_species = GBT_find_species(gb_main, species_name);
            if (gb_species) {
                gb_gene = GEN_find_gene(gb_species, gene_name);
            }
        }

        free(gene_name);
        free(species_name);

        return gb_gene;
    }
    virtual const char *getKeyPath() const OVERRIDE { // give the keypath for items
        return CHANGE_KEY_PATH_GENES;
    }
};

static GEN_item_type_species_selector item_type_gene;

static void GEN_open_mask_window(AW_window *aww, int id, GBDATA *gb_main) {
    const awt_input_mask_descriptor *descriptor = AWT_look_input_mask(id);
    gen_assert(descriptor);
    if (descriptor) {
        AWT_initialize_input_mask(aww->get_root(), gb_main, &item_type_gene, descriptor->get_internal_maskname(), descriptor->is_local_mask());
    }
}

static void GEN_create_mask_submenu(AW_window_menu_modes *awm, GBDATA *gb_main) {
    AWT_create_mask_submenu(awm, AWT_IT_GENE, GEN_open_mask_window, gb_main);
}

static AW_window *create_colorize_genes_window(AW_root *aw_root, GBDATA *gb_main) {
    return QUERY::create_colorize_items_window(aw_root, gb_main, GEN_get_selector());
}

static AW_window *create_colorize_organisms_window(AW_root *aw_root, GBDATA *gb_main) {
    return QUERY::create_colorize_items_window(aw_root, gb_main, ORGANISM_get_selector());
}

static void GEN_create_organism_submenu(AW_window_menu_modes *awm, GBDATA *gb_main, bool submenu) {
    const char *title  = "Organisms";
    const char *hotkey = "O";

    if (submenu) awm->insert_sub_menu(title, hotkey);
    else awm->create_menu(title, hotkey, AWM_ALL);

    {
        awm->insert_menu_topic("organism_info", "Organism information", "i", "sp_info.hlp", AWM_ALL, RootAsWindowCallback::simple(DBUI::popup_organism_info_window, gb_main));

        awm->sep______________();

        awm->insert_menu_topic("mark_organisms",             "Mark All organisms",              "A", "organism_mark.hlp", AWM_ALL, makeWindowCallback(mark_organisms, MARK,             gb_main));
        awm->insert_menu_topic("mark_organisms_unmark_rest", "Mark all organisms, unmark Rest", "R", "organism_mark.hlp", AWM_ALL, makeWindowCallback(mark_organisms, MARK_UNMARK_REST, gb_main));
        awm->insert_menu_topic("unmark_organisms",           "Unmark all organisms",            "U", "organism_mark.hlp", AWM_ALL, makeWindowCallback(mark_organisms, UNMARK,           gb_main));
        awm->insert_menu_topic("invmark_organisms",          "Invert marks of all organisms",   "v", "organism_mark.hlp", AWM_ALL, makeWindowCallback(mark_organisms, INVERT,           gb_main));
        awm->sep______________();
        awm->insert_menu_topic("mark_organisms_with_marked_genes", "Mark organisms with marked Genes", "G", "organism_mark.hlp", AWM_ALL, makeWindowCallback(mark_organisms_with_marked_genes, gb_main));
        awm->sep______________();
        awm->insert_menu_topic(awm->local_id("organism_colors"),   "Colors ...",           "C",    "colorize.hlp", AWM_ALL, makeCreateWindowCallback(create_colorize_organisms_window, gb_main));
    }
    if (submenu) awm->close_sub_menu();
}

static void GEN_create_gene_species_submenu(AW_window_menu_modes *awm, GBDATA *gb_main, bool submenu) {
    const char *title  = "Gene-Species";
    const char *hotkey = "S";

    if (submenu) awm->insert_sub_menu(title, hotkey);
    else awm->create_menu(title, hotkey, AWM_ALL);

    {
        awm->insert_menu_topic("mark_gene_species",             "Mark All gene-species",              "A", "gene_species_mark.hlp", AWM_ALL, makeWindowCallback(mark_gene_species, MARK,             gb_main));
        awm->insert_menu_topic("mark_gene_species_unmark_rest", "Mark all gene-species, unmark Rest", "R", "gene_species_mark.hlp", AWM_ALL, makeWindowCallback(mark_gene_species, MARK_UNMARK_REST, gb_main));
        awm->insert_menu_topic("unmark_gene_species",           "Unmark all gene-species",            "U", "gene_species_mark.hlp", AWM_ALL, makeWindowCallback(mark_gene_species, UNMARK,           gb_main));
        awm->insert_menu_topic("invmark_gene_species",          "Invert marks of all gene-species",   "I", "gene_species_mark.hlp", AWM_ALL, makeWindowCallback(mark_gene_species, INVERT,           gb_main));
        awm->sep______________();
        awm->insert_menu_topic("mark_gene_species_of_marked_genes", "Mark gene-species of marked genes",             "M", "gene_species_mark.hlp", AWM_ALL, makeWindowCallback(mark_gene_species_of_marked_genes, gb_main));
        awm->insert_menu_topic("mark_gene_species_curr_ali",        "Mark all gene-species using Current alignment", "C", "gene_species_mark.hlp", AWM_ALL, makeWindowCallback(mark_gene_species_using_current_alignment, gb_main));
    }

    if (submenu) awm->close_sub_menu();
}

struct GEN_update_info {
    AWT_canvas *canvas1;        // just canvasses of different windows (needed for updates)
    AWT_canvas *canvas2;
};

void GEN_create_genes_submenu(AW_window_menu_modes *awm, GBDATA *gb_main, bool for_ARB_NTREE) {
    awm->create_menu("Genome", "G", AWM_ALL);
    {
#if defined(DEBUG)
        awm->insert_menu_topic(awm->local_id("debug_awars"), "[DEBUG] Show main AWARs", "A", NULL, AWM_ALL, GEN_create_awar_debug_window);
        awm->sep______________();
#endif // DEBUG

        if (for_ARB_NTREE) {
            awm->insert_menu_topic("gene_map", "Gene Map", "p", "gene_map.hlp", AWM_ALL, makeCreateWindowCallback(GEN_create_first_map, gb_main)); // initial gene map
            awm->sep______________();

            GEN_create_gene_species_submenu(awm, gb_main, true); // Gene-species
            GEN_create_organism_submenu    (awm, gb_main, true); // Organisms
            EXP_create_experiments_submenu (awm, gb_main, true); // Experiments
            awm->sep______________();
        }

        awm->insert_menu_topic("gene_info",                  "Gene information", "i", "gene_info.hlp",   AWM_ALL, RootAsWindowCallback::simple(GEN_popup_gene_infowindow,    gb_main));
        awm->insert_menu_topic(awm->local_id("gene_search"), "Search and Query", "Q", "gene_search.hlp", AWM_ALL, makeCreateWindowCallback    (GEN_create_gene_query_window, gb_main));

        GEN_create_mask_submenu(awm, gb_main);

        awm->sep______________();

        GEN_insert_mark_submenu(awm, gb_main, "gene_mark_all",      "Mark all genes",      "M", "gene_mark.hlp", GEN_MARK);
        GEN_insert_mark_submenu(awm, gb_main, "gene_unmark_all",    "Unmark all genes",    "U", "gene_mark.hlp", GEN_UNMARK);
        GEN_insert_mark_submenu(awm, gb_main, "gene_invert_marked", "Invert marked genes", "v", "gene_mark.hlp", GEN_INVERT_MARKED);
        GEN_insert_mark_submenu(awm, gb_main, "gene_count_marked",  "Count marked genes",  "C", "gene_mark.hlp", GEN_COUNT_MARKED);

        awm->insert_menu_topic(awm->local_id("gene_colors"), "Colors ...", "l", "colorize.hlp", AWM_ALL, makeCreateWindowCallback(create_colorize_genes_window, gb_main));

        awm->sep______________();

        awm->insert_menu_topic("mark_genes_of_marked_gene_species", "Mark genes of marked gene-species", "g", "gene_mark.hlp", AWM_ALL, makeWindowCallback(mark_genes_of_marked_gene_species, gb_main));

        awm->sep______________();

        GEN_insert_extract_submenu(awm, gb_main, "gene_extract_marked", "Extract marked genes", "E", "gene_extract.hlp");

        if (!for_ARB_NTREE) {   // only in ARB_GENE_MAP:
            awm->sep______________();
            GEN_insert_mark_submenu(awm, gb_main, "gene_mark_hidden",  "Mark hidden genes",  "h", "gene_hide.hlp", GEN_MARK_HIDDEN);
            GEN_insert_mark_submenu(awm, gb_main, "gene_mark_visible", "Mark visible genes", "s", "gene_hide.hlp", GEN_MARK_VISIBLE);

            awm->sep______________();
            GEN_insert_mark_submenu(awm, gb_main, "gene_unmark_hidden",  "Unmark hidden genes",  "d", "gene_hide.hlp", GEN_UNMARK_HIDDEN);
            GEN_insert_mark_submenu(awm, gb_main, "gene_unmark_visible", "Unmark visible genes", "r", "gene_hide.hlp", GEN_UNMARK_VISIBLE);
        }
    }
}

static void GEN_create_hide_submenu(AW_window_menu_modes *awm, GBDATA *gb_main) {
    awm->create_menu("Hide", "H", AWM_ALL);
    {
        GEN_insert_hide_submenu(awm, gb_main, "gene_hide_marked",    "Hide marked genes",        "H", "gene_hide.hlp", GEN_HIDE_MARKED);
        GEN_insert_hide_submenu(awm, gb_main, "gene_unhide_marked",  "Unhide marked genes",      "U", "gene_hide.hlp", GEN_UNHIDE_MARKED);
        GEN_insert_hide_submenu(awm, gb_main, "gene_invhide_marked", "Invert-hide marked genes", "v", "gene_hide.hlp", GEN_INVERT_HIDE_MARKED);

        awm->sep______________();
        GEN_insert_hide_submenu(awm, gb_main, "gene_hide_all",    "Hide all genes",        "a", "gene_hide.hlp", GEN_HIDE_ALL);
        GEN_insert_hide_submenu(awm, gb_main, "gene_unhide_all",  "Unhide all genes",      "l", "gene_hide.hlp", GEN_UNHIDE_ALL);
        GEN_insert_hide_submenu(awm, gb_main, "gene_invhide_all", "Invert-hide all genes", "I", "gene_hide.hlp", GEN_INVERT_HIDE_ALL);
    }
}

static void GEN_set_display_style(AW_window *aww, GEN_DisplayStyle style) {
    GEN_map_window *win = dynamic_cast<GEN_map_window*>(aww); // @@@ use same callback-type as for with_all_mapped_windows
    gen_assert(win);

    win->get_root()->awar(AWAR_GENMAP_DISPLAY_TYPE(win->get_nr()))->write_int(style);
    win->get_graphic()->set_display_style(style);
    GEN_map_window_zoom_reset_and_refresh(win);
}

static AW_window *GEN_create_map(AW_root *aw_root, GEN_create_map_param *param) {
    // param->window_nr shall be 0 for first window (called from ARB_NTREE)
    // additional views are always created by the previous window created with GEN_map
    GEN_map_manager *manager = 0;

    if (!GEN_map_manager::initialized()) {          // first call
        gen_assert(param->window_nr == 0); // has to be 0 for first call

        GEN_map_manager::initialize(aw_root, param->gb_main);
        manager = GEN_map_manager::get_map_manager(); // creates the manager

        // global initialization (AWARS etc.) done here :

        GEN_create_genemap_global_awars(aw_root, AW_ROOT_DEFAULT, param->gb_main);
        GEN_add_global_awar_callbacks(aw_root);
        {
            GB_transaction ta(param->gb_main);
            GEN_make_node_text_init(param->gb_main);
        }
    }
    else {
        manager = GEN_map_manager::get_map_manager();
    }

    GEN_map_window *gmw = manager->get_map_window(param->window_nr);
    return gmw;
}

AW_window *GEN_create_first_map(AW_root *aw_root, GBDATA *gb_main) {
    return GEN_create_map(aw_root, new GEN_create_map_param(gb_main, 0));
}

// ____________________________________________________________
// start of implementation of class GEN_map_window::

void GEN_map_window::init(AW_root *awr, GBDATA *gb_main) {
    {
        char *windowName = (window_nr == 0) ? strdup("ARB Gene Map") : GBS_global_string_copy("ARB Gene Map %i", window_nr);
        char *windowID   = (window_nr == 0) ? strdup("ARB_GENE_MAP") : GBS_global_string_copy("ARB_GENE_MAP_%i", window_nr);

        AW_window_menu_modes::init(awr, windowID, windowName, 200, 200);

        free(windowID);
        free(windowName);
    }

    GEN_create_genemap_local_awars(awr, AW_ROOT_DEFAULT, window_nr);

    gen_graphic = new GEN_graphic(awr, gb_main, GEN_gene_container_cb_installer, window_nr);
    gen_canvas  = new AWT_canvas(gb_main, this, get_window_id(), gen_graphic, AWAR_SPECIES_NAME);

    GEN_add_local_awar_callbacks(awr, AW_ROOT_DEFAULT, this);

    {
        GB_transaction ta(gb_main);
        gen_graphic->reinit_gen_root(gen_canvas, false);
    }

    gen_canvas->recalc_size_and_refresh();
    gen_canvas->set_mode(AWT_MODE_SELECT); // Default-Mode

    // ---------------
    //      menus

    // File Menu
    create_menu("File", "F", AWM_ALL);
    insert_menu_topic("close",              "Close",    "C", NULL,               AWM_ALL, AW_POPDOWN);
    insert_menu_topic(local_id("new_view"), "New view", "v", "gen_new_view.hlp", AWM_ALL, makeCreateWindowCallback(GEN_create_map, new GEN_create_map_param(gb_main, window_nr+1)));

    GEN_create_genes_submenu       (this, gb_main, false); // Genes
    GEN_create_gene_species_submenu(this, gb_main, false); // Gene-species
    GEN_create_organism_submenu    (this, gb_main, false); // Organisms
    EXP_create_experiments_submenu (this, gb_main, false); // Experiments
    GEN_create_hide_submenu(this, gb_main);         // Hide Menu

    // Properties Menu
    create_menu("Properties", "r", AWM_ALL);
#if defined(ARB_MOTIF)
    insert_menu_topic(local_id("gene_props_menu"), "Frame settings ...",                  "M", "props_frame.hlp", AWM_ALL, AW_preset_window);
#endif
    insert_menu_topic(local_id("gene_props"),      "GENEMAP: Colors and Fonts ...",       "C", "color_props.hlp", AWM_ALL, makeCreateWindowCallback(GEN_create_gc_window, gen_canvas->gc_manager));
    insert_menu_topic(local_id("gene_options"),    "Options",                             "O", "gen_options.hlp", AWM_ALL, GEN_create_options_window);
    insert_menu_topic(local_id("gene_nds"),        "NDS ( Select Gene Information ) ...", "N", "props_nds.hlp",   AWM_ALL, makeCreateWindowCallback(GEN_open_nds_window, gb_main));
    sep______________();
    AW_insert_common_property_menu_entries(this);
    sep______________();
    insert_menu_topic("gene_save_props",   "Save Defaults (ntree.arb)", "D", "savedef.hlp", AWM_ALL, AW_save_properties);

    // ----------------------
    //      mode buttons

    create_mode("mode_select.xpm", "gen_mode.hlp", AWM_ALL, makeWindowCallback(GEN_mode_event, this, AWT_MODE_SELECT));
    create_mode("mode_zoom.xpm",   "gen_mode.hlp", AWM_ALL, makeWindowCallback(GEN_mode_event, this, AWT_MODE_ZOOM));
    create_mode("mode_info.xpm",   "gen_mode.hlp", AWM_ALL, makeWindowCallback(GEN_mode_event, this, AWT_MODE_INFO));

    // -------------------
    //      info area

    set_info_area_height(250);
    at(11, 2);
    auto_space(2, -2);
    shadow_width(1);

    // close + undo button, info area, define line y-positions:

    int cur_x, cur_y, start_x, first_line_y, second_line_y, third_line_y;
    get_at_position(&start_x, &first_line_y);
    button_length(6);

    at(start_x, first_line_y);
    help_text("quit.hlp");
    callback((AW_CB0)AW_POPDOWN);
    create_button("Close", "Close");

    get_at_position(&cur_x, &cur_y);

    int gene_x = cur_x;
    at_newline();
    get_at_position(&cur_x, &second_line_y);

    at(start_x, second_line_y);
    help_text("undo.hlp");
    callback(makeWindowCallback(GEN_undo_cb, GB_UNDO_UNDO, gb_main));
    create_button("Undo", "Undo");

    at_newline();
    get_at_position(&cur_x, &third_line_y);

    button_length(AWAR_FOOTER_MAX_LEN);
    create_button(0, AWAR_FOOTER);

    at_newline();
    get_at_position(&cur_x, &cur_y);
    set_info_area_height(cur_y+6);

    // gene+species buttons:
    button_length(20);

    at(gene_x, first_line_y);
    help_text("sp_search.hlp");
    callback(makeCreateWindowCallback(DBUI::create_species_query_window, gb_main)); // @@@ should use an organism search (using ITEM_organism) 
    create_button("SEARCH_ORGANISM", AWAR_LOCAL_ORGANISM_NAME(window_nr));

    at(gene_x, second_line_y);
    help_text("gene_search.hlp");
    callback(makeCreateWindowCallback(GEN_create_gene_query_window, gb_main));
    create_button("SEARCH_GENE", AWAR_LOCAL_GENE_NAME(window_nr));

    get_at_position(&cur_x, &cur_y);
    int lock_x = cur_x;

    at(lock_x, first_line_y);
    create_toggle(AWAR_LOCAL_ORGANISM_LOCK(window_nr));

    at(lock_x, second_line_y);
    create_toggle(AWAR_LOCAL_GENE_LOCK(window_nr));

    get_at_position(&cur_x, &cur_y);
    int dtype_x1 = cur_x;

    // display type buttons:

    button_length(4);

    at(dtype_x1, first_line_y);
    help_text("gen_disp_style.hlp");
    callback(makeWindowCallback(GEN_set_display_style, GEN_DISPLAY_STYLE_RADIAL));
    create_button("RADIAL_DISPLAY_TYPE", "#gen_radial.xpm", 0);

    help_text("gen_disp_style.hlp");
    callback(makeWindowCallback(GEN_set_display_style, GEN_DISPLAY_STYLE_BOOK));
    create_button("BOOK_DISPLAY_TYPE", "#gen_book.xpm", 0);

    get_at_position(&cur_x, &cur_y);
    int jump_x = cur_x;

    at(dtype_x1, second_line_y);
    help_text("gen_disp_style.hlp");
    callback(makeWindowCallback(GEN_set_display_style, GEN_DISPLAY_STYLE_VERTICAL));
    create_button("VERTICAL_DISPLAY_TYPE", "#gen_vertical.xpm", 0);

    // jump button:

    button_length(0);

    at(jump_x, first_line_y);
    help_text("gen_jump.hlp");
    callback(makeWindowCallback(GEN_jump_cb, true));
    create_button("Jump", "Jump");

    // help buttons:

    get_at_position(&cur_x, &cur_y);
    int help_x = cur_x;

    at(help_x, first_line_y);
    help_text("help.hlp");
    callback(makeHelpCallback("gene_map.hlp"));
    create_button("HELP", "HELP", "H");

    at(help_x, second_line_y);
    callback(AW_help_entry_pressed);
    create_button(0, "?");

}

// -end- of implementation of class GEN_map_window:.


