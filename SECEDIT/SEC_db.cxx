// =============================================================== //
//                                                                 //
//   File      : SEC_db.cxx                                        //
//   Purpose   : db interface                                      //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "SEC_graphic.hxx"
#include "SEC_root.hxx"
#include "SEC_bonddef.hxx"
#include "SEC_toggle.hxx"

#include <aw_awars.hxx>
#include <aw_file.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <ed4_extern.hxx>
#include <arbdbt.h>
#include <ad_cb.h>

#include <arb_defs.h>

using namespace std;

// -----------------------------------
//      member function callbacks

typedef void (SEC_db_interface::*interface_cb)(const SEC_dbcb *);

struct SEC_dbcb {
    SEC_db_interface *instance;
    interface_cb      member_fun;

    SEC_dbcb(SEC_db_interface *db, interface_cb cb) : instance(db), member_fun(cb) {}
    void call() const { (instance->*member_fun)(this); }
};

static void sec_dbcb(UNFIXED, const SEC_dbcb *cb) {
    cb->call();
}

// ----------------------
//      SEC_seq_data

SEC_seq_data::SEC_seq_data(GBDATA *gb_item, const char *aliname, const SEC_dbcb *cb) {
    gb_name   = GB_search(gb_item, "name", GB_FIND);
    gb_data   = GBT_find_sequence(gb_item, aliname);
    change_cb = cb;
    len       = GB_read_string_count(gb_data);
    Data      = GB_read_string(gb_data);

    if (gb_name) GB_add_callback(gb_name, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(sec_dbcb, change_cb));
    if (gb_data) GB_add_callback(gb_data, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(sec_dbcb, change_cb));
}

SEC_seq_data::~SEC_seq_data() {
    if (gb_name) GB_remove_callback(gb_name, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(sec_dbcb, change_cb));
    if (gb_data) GB_remove_callback(gb_data, GB_CB_CHANGED_OR_DELETED, makeDatabaseCallback(sec_dbcb, change_cb));
    free(Data);
}

// -------------------
//      pair defs

#define AWAR_PAIRS(type) AWAR_SECEDIT_##type##_PAIRS
#define AWAR_PCHAR(type) AWAR_SECEDIT_##type##_PAIR_CHAR

struct PairDefinition {
    const char *awar_pairs;
    const char *awar_pairchar;
    const char *default_pairs;
    const char *default_pairchar;
};

#define PAIR_TYPES 5
static PairDefinition pairdef[PAIR_TYPES] = {
    { AWAR_PAIRS(STRONG), AWAR_PCHAR(STRONG), "GC AU AT",                   "-" },
    { AWAR_PAIRS(NORMAL), AWAR_PCHAR(NORMAL), "GU GT",                      "." },
    { AWAR_PAIRS(WEAK),   AWAR_PCHAR(WEAK),   "GA",                         "o" },
    { AWAR_PAIRS(NO),     AWAR_PCHAR(NO),     "AA AC CC CU CT GG UU TT TU", ""  },
    { AWAR_PAIRS(USER),   AWAR_PCHAR(USER),   "",                           ""  },
};

GB_ERROR SEC_bond_def::update(AW_root *aw_root, const char *changed_awar_name)
{
    GB_ERROR error = 0;

    if (changed_awar_name) {
        int changed_pair_idx = -1;
        for (int i = 0; i<PAIR_TYPES; ++i) {
            if (strcmp(changed_awar_name, pairdef[i].awar_pairs) == 0) {
                changed_pair_idx = i;
                break;
            }
        }

        sec_assert(changed_pair_idx != -1);

        if (changed_pair_idx != -1) {
            char *content = aw_root->awar(changed_awar_name)->read_string();

            for (int i = 0; i<PAIR_TYPES; ++i) {
                if (i != changed_pair_idx) { // correct other pair strings
                    clear();
                    AW_awar *awar = aw_root->awar(pairdef[i].awar_pairs);

                    char *oldP = awar->read_string();

                    insert(oldP, '#');
                    insert(content, ' ');

                    char *newP = get_pair_string('#');

                    if (strcmp(oldP, newP) != 0) awar->write_string(newP);

                    free(newP);
                    free(oldP);
                }
            }
            free(content);
        }
    }

    clear();
    for (int i = 0; !error && i<PAIR_TYPES; ++i) {
        char *pairs    = aw_root->awar(pairdef[i].awar_pairs)->read_string();
        char *pairchar = aw_root->awar(pairdef[i].awar_pairchar)->read_string();

        error = insert(pairs, pairchar[0]);

        free(pairchar);
        free(pairs);
    }

    return error;
}

static void pair_def_changed_cb(AW_root *aw_root, SEC_db_interface *db, const char *awar_name) {
    GB_ERROR err = db->bonds()->update(aw_root, awar_name);
    if (err) aw_message(err);
    db->canvas()->refresh();
}

static void bind_bonddef_awars(SEC_db_interface *db) {
    AW_root *aw_root = db->awroot();

    for (int i = 0; i<PAIR_TYPES; ++i) {
        PairDefinition& pd = pairdef[i];
        aw_root->awar(pd.awar_pairs)   ->add_callback(makeRootCallback(pair_def_changed_cb, db, pd.awar_pairs));
        aw_root->awar(pd.awar_pairchar)->add_callback(makeRootCallback(pair_def_changed_cb, db, (const char *)NULL));
    }

    pair_def_changed_cb(aw_root, db, NULL);
}

// --------------------------------------------------------------------------------

void SEC_displayParams::reread(AW_root *aw_root, const ED4_plugin_host& host) {
    show_helixNrs            = aw_root->awar(AWAR_SECEDIT_SHOW_HELIX_NRS)->read_int();
    distance_between_strands = aw_root->awar(AWAR_SECEDIT_DIST_BETW_STRANDS)->read_float();
    show_bonds               = (ShowBonds)aw_root->awar(AWAR_SECEDIT_SHOW_BONDS)->read_int();
    show_curpos              = (ShowCursorPos)aw_root->awar(AWAR_SECEDIT_SHOW_CURPOS)->read_int();

    hide_bases     = aw_root->awar(AWAR_SECEDIT_HIDE_BASES)->read_int();
    show_ecoli_pos = aw_root->awar(AWAR_SECEDIT_SHOW_ECOLI_POS)->read_int();
    display_search = aw_root->awar(AWAR_SECEDIT_DISPLAY_SEARCH)->read_int();
    display_sai    = aw_root->awar(AWAR_SECEDIT_DISPLAY_SAI)->read_int() && host.SAIs_visualized();

    show_strSkeleton   = aw_root->awar(AWAR_SECEDIT_SHOW_STR_SKELETON)->read_int();
    skeleton_thickness = aw_root->awar(AWAR_SECEDIT_SKELETON_THICKNESS)->read_int();
    bond_thickness     = aw_root->awar(AWAR_SECEDIT_BOND_THICKNESS)->read_int();

    edit_rightward = aw_root->awar(AWAR_EDIT_RIGHTWARD)->read_int();

#if defined(DEBUG)
    show_debug = aw_root->awar(AWAR_SECEDIT_SHOW_DEBUG)->read_int();
#endif // DEBUG
}

// --------------------------
//      SEC_db_interface

void SEC_db_interface::reload_sequence(const SEC_dbcb *cb) {
    GB_transaction ta(gb_main);

    char   *species_name = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    bool    had_sequence = sequence;
    GBDATA *gb_species   = GBT_find_species(gb_main, species_name);

    delete sequence;
    sequence = gb_species ? new SEC_seq_data(gb_species, aliname, cb) : 0;
    Host.announce_current_species(species_name);

    if (bool(sequence) != had_sequence) gfx->request_update(SEC_UPDATE_ZOOM_RESET);
    if (sequence) gfx->request_update(SEC_UPDATE_SHOWN_POSITIONS);

    if (perform_refresh) scr->refresh();

    free(species_name);
}

void SEC_db_interface::reload_ecoli(const SEC_dbcb *cb) {
    GB_transaction ta(gb_main);

    delete ecoli_seq;
    delete Ecoli;

    GBDATA *gb_ecoli = GBT_find_SAI(gb_main, "ECOLI");
    if (gb_ecoli) {
        ecoli_seq = new SEC_seq_data(gb_ecoli, aliname, cb);
        Ecoli = new BI_ecoli_ref;

        Ecoli->init(ecoli_seq->sequence(), ecoli_seq->length());
    }
    else {
        ecoli_seq = 0;
        Ecoli     = 0;
    }

    gfx->request_update(SEC_UPDATE_SHOWN_POSITIONS);
    if (perform_refresh) scr->refresh();
}

void SEC_db_interface::reload_helix(const SEC_dbcb *cb) {
    GB_transaction ta(gb_main);

    delete helix_nr;    helix_nr  = 0;
    delete helix_pos;   helix_pos = 0;
    delete Helix;       Helix     = 0;

    char *helix_pos_name = GBT_get_default_helix(gb_main);
    GBDATA *gb_helix_pos = GBT_find_SAI(gb_main, helix_pos_name);
    if (gb_helix_pos) {
        char   *helix_nr_name = GBT_get_default_helix_nr(gb_main);
        GBDATA *gb_helix_nr   = GBT_find_SAI(gb_main, helix_nr_name);

        if (gb_helix_nr) {
            helix_pos = new SEC_seq_data(gb_helix_pos, aliname, cb);
            helix_nr  = new SEC_seq_data(gb_helix_nr, aliname, cb);
            Helix     = new BI_helix;

            GB_ERROR error = Helix->initFromData(helix_nr->sequence(), helix_pos->sequence(), helix_pos->length());
            if (error) {
                error = ta.close(error);
                aw_message(error);
            }
        }

        free(helix_nr_name);
    }
    free(helix_pos_name);

    gfx->request_update(static_cast<SEC_update_request>(SEC_UPDATE_SHOWN_POSITIONS|SEC_UPDATE_RELOAD));
    if (perform_refresh) scr->refresh();
}

void SEC_db_interface::update_positions(const SEC_dbcb *) {
    displayBindingHelixPositions = aw_root->awar(AWAR_SECEDIT_DISPPOS_BINDING)->read_int();
    displayEcoliPositions        = aw_root->awar(AWAR_SECEDIT_DISPPOS_ECOLI)->read_int();

    gfx->sec_root->nail_cursor();
    gfx->request_update(SEC_UPDATE_SHOWN_POSITIONS);
    if (perform_refresh) scr->refresh();
}

void SEC_db_interface::relayout(const SEC_dbcb *) {
    SEC_root *root = secroot();
    root->reread_display_params(awroot(), Host);
    root->nail_cursor();
    gfx->request_update(SEC_UPDATE_RECOUNT);
    if (perform_refresh) scr->refresh();
}

void SEC_db_interface::refresh(const SEC_dbcb *) {
    SEC_root *root = secroot();
    root->reread_display_params(awroot(), Host);
    if (perform_refresh) scr->refresh();
}

void SEC_root::set_cursor(int abspos, bool performRefresh) {
    if (performRefresh) {
        nail_cursor();
        int nailedPos = nailedAbsPos;

        cursorAbsPos = abspos;

        db->canvas()->refresh();

        nailedAbsPos = nailedPos;
        position_cursor(false, false);
    }
    else {
        cursorAbsPos = abspos;
    }
}

void SEC_db_interface::cursor_changed(const SEC_dbcb *) {
    SEC_root *root    = secroot();
    int       seq_pos = bio2info(aw_root->awar_int(AWAR_CURSOR_POSITION)->read_int());

    root->set_cursor(seq_pos, perform_refresh);
}

void SEC_db_interface::alilen_changed(const SEC_dbcb *) {
#if defined(WARN_TODO)
#warning @@@reread alilen
#warning test changing ali length!
#endif
    gfx->request_update(SEC_UPDATE_RELOAD);
    if (perform_refresh) scr->refresh();
}

// --------------------------------------------------------------------------------

static const char *update_pos_awars[] = {
    AWAR_SECEDIT_DISPPOS_BINDING,
    AWAR_SECEDIT_DISPPOS_ECOLI,
    NULL
};

static const char *relayout_awars[] = {
    AWAR_SECEDIT_DIST_BETW_STRANDS,
    NULL
};

static const char *refresh_awars[] = {
    AWAR_SECEDIT_SHOW_HELIX_NRS,
    AWAR_SECEDIT_SHOW_BONDS,
    AWAR_SECEDIT_SHOW_CURPOS,
    AWAR_SECEDIT_HIDE_BASES,
    AWAR_SECEDIT_SHOW_ECOLI_POS,
    AWAR_SECEDIT_DISPLAY_SEARCH,
    AWAR_SECEDIT_DISPLAY_SAI,
    AWAR_SECEDIT_SHOW_STR_SKELETON,
    AWAR_SECEDIT_SKELETON_THICKNESS,
    AWAR_SECEDIT_BOND_THICKNESS,
    AWAR_EDIT_RIGHTWARD,
    ED4_AWAR_SEARCH_RESULT_CHANGED,
#if defined(DEBUG)
    AWAR_SECEDIT_SHOW_DEBUG,
#endif // DEBUG
    NULL
};

void SEC_db_interface::bind_awars(const char **awars, SEC_dbcb *cb) {
    RootCallback awarcb = makeRootCallback(sec_dbcb, cb);
    for (int i = 0; awars[i]; ++i) {
        aw_root->awar(awars[i])->add_callback(awarcb);
    }
}

// --------------------------------------------------------------------------------

static void create_awars(AW_root *aw_root, AW_default def) {
    aw_root->awar_int(AWAR_SECEDIT_BASELINEWIDTH, 0, def)->set_minmax(0, 10);
    aw_root->awar_string(AWAR_FOOTER);

    {
        char *dir = strdup(GB_path_in_arbprop("secondary_structure"));
        AW_create_fileselection_awars(aw_root, AWAR_SECEDIT_SAVEDIR, dir, ".ass", "noname.ass");
        free(dir);
    }

    aw_root->awar_float(AWAR_SECEDIT_DIST_BETW_STRANDS,  1, def)->set_minmax(0.01, 40);
    aw_root->awar_int  (AWAR_SECEDIT_SKELETON_THICKNESS, 1, def)->set_minmax(1,    10);
    aw_root->awar_int  (AWAR_SECEDIT_BOND_THICKNESS,     1, def)->set_minmax(1,    10);

#if defined(DEBUG)
    aw_root->awar_int(AWAR_SECEDIT_SHOW_DEBUG,        0,                def);
#endif // DEBUG
    aw_root->awar_int(AWAR_SECEDIT_SHOW_HELIX_NRS,    0,                def);
    aw_root->awar_int(AWAR_SECEDIT_SHOW_ECOLI_POS,    0,                def);
    aw_root->awar_int(AWAR_SECEDIT_SHOW_STR_SKELETON, 0,                def);
    aw_root->awar_int(AWAR_SECEDIT_HIDE_BASES,        0,                def);
    aw_root->awar_int(AWAR_SECEDIT_SHOW_BONDS,        SHOW_HELIX_BONDS, def);
    aw_root->awar_int(AWAR_SECEDIT_SHOW_CURPOS,       SHOW_ABS_CURPOS,  def);
    aw_root->awar_int(AWAR_SECEDIT_DISPLAY_SAI,       0,                def);
    aw_root->awar_int(AWAR_SECEDIT_DISPLAY_SEARCH,    0,                def);
    aw_root->awar_int(AWAR_SECEDIT_DISPPOS_BINDING,   0,                def);
    aw_root->awar_int(AWAR_SECEDIT_DISPPOS_ECOLI,     1,                def);

    for (int i = 0; i<PAIR_TYPES; ++i) {
        PairDefinition& pd = pairdef[i];
        aw_root->awar_string(pd.awar_pairs,    pd.default_pairs);
        aw_root->awar_string(pd.awar_pairchar, pd.default_pairchar);
    }
}

// --------------------------------------------------------------------------------

SEC_db_interface::SEC_db_interface(SEC_graphic *Gfx, AWT_canvas *Scr, ED4_plugin_host& host_)
    : sequence(0)
    , Host(host_)
    , displayEcoliPositions(false)
    , ecoli_seq(0)
    , Ecoli(0)
    , displayBindingHelixPositions(true)
    , helix_nr(0)
    , helix_pos(0)
    , Helix(0)
    , gfx(Gfx)
    , scr(Scr)
    , gb_main(gfx->gb_main)
    , aw_root(gfx->aw_root)
    , perform_refresh(false)
{
    GB_transaction ta(gb_main);

    create_awars(aw_root, AW_ROOT_DEFAULT);

    aliname    = GBT_get_default_alignment(gb_main);
    sec_assert(aliname);
    ali_length = GBT_get_alignment_len(gb_main, aliname);
    displayPos = new bool[ali_length];
    bonddef    = new SEC_bond_def(GBT_get_alignment_type(gb_main, aliname));

    // awar and DB callbacks:

    sequence_cb       = new SEC_dbcb(this, &SEC_db_interface::reload_sequence);
    ecoli_cb          = new SEC_dbcb(this, &SEC_db_interface::reload_ecoli);
    helix_cb          = new SEC_dbcb(this, &SEC_db_interface::reload_helix);
    updatepos_cb      = new SEC_dbcb(this, &SEC_db_interface::update_positions);
    relayout_cb       = new SEC_dbcb(this, &SEC_db_interface::relayout);
    refresh_cb        = new SEC_dbcb(this, &SEC_db_interface::refresh);
    cursorpos_cb      = new SEC_dbcb(this, &SEC_db_interface::cursor_changed);
    alilen_changed_cb = new SEC_dbcb(this, &SEC_db_interface::alilen_changed);

    aw_root->awar_string(AWAR_SPECIES_NAME,    "", gb_main)->add_callback(makeRootCallback(sec_dbcb, sequence_cb));
    aw_root->awar_string(AWAR_CURSOR_POSITION, "", gb_main)->add_callback(makeRootCallback(sec_dbcb, cursorpos_cb));

    bind_awars(update_pos_awars, updatepos_cb);
    bind_awars(relayout_awars, relayout_cb);
    bind_awars(refresh_awars, refresh_cb);

    bind_bonddef_awars(this);

    {
        GBDATA *gb_alignment     = GBT_get_alignment(gb_main, aliname);
        GBDATA *gb_alignment_len = GB_search(gb_alignment, "alignment_len", GB_FIND);
        sec_assert(gb_alignment_len);
        GB_add_callback(gb_alignment_len, GB_CB_CHANGED, makeDatabaseCallback(sec_dbcb, alilen_changed_cb));
    }

    sequence_cb->call();
    ecoli_cb->call();
    helix_cb->call();
    updatepos_cb->call();
    relayout_cb->call();
    refresh_cb->call();

    toggler = 0;

    perform_refresh = true;
}

SEC_db_interface::~SEC_db_interface() {
    free(aliname);

    delete [] displayPos;

    delete sequence;    sequence = 0;

    delete ecoli_seq;   ecoli_seq = 0;
    delete Ecoli;       Ecoli     = 0;

    delete helix_nr;    helix_nr  = 0;
    delete helix_pos;   helix_pos = 0;
    delete Helix;       Helix     = 0;

    delete sequence_cb;         sequence_cb       = 0;
    delete ecoli_cb;            ecoli_cb          = 0;
    delete helix_cb;            helix_cb          = 0;
    delete updatepos_cb;        updatepos_cb      = 0;
    delete relayout_cb;         relayout_cb       = 0;
    delete refresh_cb;          refresh_cb        = 0;
    delete cursorpos_cb;        cursorpos_cb      = 0;
    delete alilen_changed_cb;   alilen_changed_cb = 0;

    delete toggler;     toggler = 0;

    delete bonddef;     bonddef = 0;
}

SEC_root *SEC_db_interface::secroot() const { return gfx->sec_root; }

// --------------------------------------------------------------------------------

inline bool gap(char c) { return c == '-' || c == '.'; }

void SEC_db_interface::update_shown_positions() {
    shown = 0;

    sec_assert(sequence && Helix);

    for (size_t pos = 0; pos<ali_length; ++pos) {
        bool isPaired = Helix->pairtype(pos) != HELIX_NONE;
        char base     = baseAt(pos);

        if (isPaired) {
            if (displayBindingHelixPositions) {
                displayPos[pos] = true;
            }
            else {
                char oppoBase = baseAt(Helix->opposite_position(pos));

                displayPos[pos] = !(gap(base) && gap(oppoBase)); // display if one side of helix has base
            }
        }
        else {
            displayPos[pos] = !gap(base);
        }

        shown += displayPos[pos];
    }

    // add hidden ecoli positions
    if (displayEcoliPositions && ecoli_seq) {
        for (size_t pos = 0; pos<ali_length; ++pos) {
            if (!displayPos[pos] && !gap(ecoli_seq->data(pos))) {
                displayPos[pos] = true;
                shown++;
            }
        }
    }
}

void SEC_db_interface::init_toggler() const {
    if (!toggler) toggler = new SEC_structure_toggler(gb_main, aliname, gfx);
}

void SEC_root::update_shown_positions() {
    db->update_shown_positions();
}

// --------------------------------------------------------------------------------

