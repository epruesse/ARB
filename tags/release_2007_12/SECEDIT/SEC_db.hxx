// =============================================================== //
//                                                                 //
//   File      : sec_db.hxx                                        //
//   Purpose   : DB interface                                      //
//   Time-stamp: <Fri Sep/14/2007 13:19 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SEC_DB_HXX
#define SEC_DB_HXX

#ifndef _CPP_CSTDIO
#include <cstdio>
#endif

#ifndef ARBDB_H
#include <arbdb.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef BI_HELIX_HXX
#include <BI_helix.hxx>
#endif

#ifndef SEC_DEFS_HXX
#include "SEC_defs.hxx"
#endif

// --------------------------------------------------------------------------------

#define AWAR_SECEDIT_BASELINEWIDTH "secedit/baselinewidth"
#define AWAR_SECEDIT_SAVEDIR       "secedit/savedir"

#define AWAR_SECEDIT_DIST_BETW_STRANDS  "secedit/layout/dist_betw_strands"
#define AWAR_SECEDIT_SKELETON_THICKNESS "secedit/layout/skelton_thickness"
#define AWAR_SECEDIT_BOND_THICKNESS     "secedit/layout/bond_thickness"
#define AWAR_SECEDIT_SHOW_DEBUG         "secedit/layout/show_debug_info"
#define AWAR_SECEDIT_SHOW_HELIX_NRS     "secedit/layout/show_helix_numbers"
#define AWAR_SECEDIT_SHOW_ECOLI_POS     "secedit/layout/show_ecoli_pos"
#define AWAR_SECEDIT_SHOW_STR_SKELETON  "secedit/layout/show_structure_skeleton"
#define AWAR_SECEDIT_HIDE_BASES         "secedit/layout/hide_bases"
#define AWAR_SECEDIT_SHOW_BONDS         "secedit/layout/show_bonds"
#define AWAR_SECEDIT_SHOW_CURPOS        "secedit/layout/show_cursor_pos"
#define AWAR_SECEDIT_DISPLAY_SAI        "secedit/layout/display_sai"
#define AWAR_SECEDIT_DISPLAY_SEARCH     "secedit/layout/display_search"
#define AWAR_SECEDIT_DISPPOS_BINDING    "secedit/layout/disppos_binding"
#define AWAR_SECEDIT_DISPPOS_ECOLI      "secedit/layout/disppos_ecoli"

#define AWAR_SECEDIT_STRONG_PAIRS   "secedit/layout/pairs/strong"       // Bonds
#define AWAR_SECEDIT_NORMAL_PAIRS   "secedit/layout/pairs/normal"
#define AWAR_SECEDIT_WEAK_PAIRS     "secedit/layout/pairs/weak"
#define AWAR_SECEDIT_NO_PAIRS       "secedit/layout/pairs/no"
#define AWAR_SECEDIT_USER_PAIRS     "secedit/layout/pairs/user"

#define AWAR_SECEDIT_STRONG_PAIR_CHAR   "secedit/layout/pairs/strong_char"
#define AWAR_SECEDIT_NORMAL_PAIR_CHAR   "secedit/layout/pairs/normal_char"
#define AWAR_SECEDIT_WEAK_PAIR_CHAR     "secedit/layout/pairs/weak_char"
#define AWAR_SECEDIT_NO_PAIR_CHAR       "secedit/layout/pairs/no_char"
#define AWAR_SECEDIT_USER_PAIR_CHAR     "secedit/layout/pairs/user_char"

// --------------------------------------------------------------------------------

struct SEC_dbcb;
class  SEC_db_interface;

class SEC_seq_data { // represents a sequence (or SAI)
    GBDATA         *gb_name;
    GBDATA         *gb_data;
    const SEC_dbcb *change_cb;
    size_t          len;
    char           *Data;

public:
    SEC_seq_data(GBDATA *gb_item, const char *aliname, const SEC_dbcb *reload_item);
    ~SEC_seq_data();

    bool valid() const { return Data; }
    size_t length() const { sec_assert(valid()); return len; }
    char data(size_t abspos) const {
        sec_assert(abspos<length());
        return Data[abspos];
    }

    const char *sequence() const { sec_assert(valid()); return Data; }
};

class BI_helix;
class BI_ecoli_ref;
class SEC_graphic;
class AWT_canvas;
class AW_root;
class ED4_sequence_terminal;
class SEC_bond_def;
class SEC_root;
class SEC_structure_toggler;

class SEC_db_interface : Noncopyable {
    SEC_seq_data                *sequence; // 0 = no sequence selected
    const ED4_sequence_terminal *seqTerminal;

    bool          displayEcoliPositions; // whether to display ecoli positions
    SEC_seq_data *ecoli_seq;        // 0 = no ecoli found or not used
    BI_ecoli_ref *Ecoli;

    SEC_bond_def *bonddef;

    bool          displayBindingHelixPositions; // whether to display all binding helix positions
    SEC_seq_data *helix_nr;
    SEC_seq_data *helix_pos;
    BI_helix     *Helix;

    mutable SEC_structure_toggler *toggler;

    SEC_graphic *gfx;
    AWT_canvas  *ntw;
    GBDATA      *gb_main;
    AW_root     *aw_root;

    char   *aliname;
    size_t  ali_length;

    bool *displayPos;           // contains true for displayed positions
    size_t shown; // how many positions are shown

    SEC_dbcb *sequence_cb;
    SEC_dbcb *ecoli_cb;
    SEC_dbcb *helix_cb;
    SEC_dbcb *updatepos_cb;
    SEC_dbcb *relayout_cb;
    SEC_dbcb *refresh_cb;
    SEC_dbcb *cursorpos_cb;
    SEC_dbcb *alilen_changed_cb;

    bool perform_refresh; // perform canvas refresh in callbacks ?

    void reload_sequence(const SEC_dbcb *cb);
    void reload_ecoli(const SEC_dbcb *cb);
    void reload_helix(const SEC_dbcb *cb);
    void update_positions(const SEC_dbcb *cb);
    void relayout(const SEC_dbcb *cb);
    void refresh(const SEC_dbcb *cb);
    void cursor_changed(const SEC_dbcb *cb);
    void alilen_changed(const SEC_dbcb *cb);

    void bind_awars(const char **awars, SEC_dbcb *cb);
    
public:
    SEC_db_interface(SEC_graphic *Gfx, AWT_canvas *Ntw);
    ~SEC_db_interface();

    void update_shown_positions();

    bool canDisplay() const { return Helix && sequence; }
    size_t length() const { return ali_length; }

    bool shallDisplayPosition(size_t abspos) const {
        sec_assert(abspos<length());
        return displayPos[abspos];
    }
    char baseAt(size_t abspos) const {
        sec_assert(abspos<length());
        return sequence->data(abspos);
    }

    AW_root *awroot() const { return aw_root; }
    GBDATA *gbmain() const { return gb_main; }
    SEC_graphic *graphic() const { return gfx; }
    SEC_root *secroot() const;
    AWT_canvas *canvas() const { return ntw; }
    BI_helix *helix() const { return Helix; }
    BI_ecoli_ref *ecoli() const { return Ecoli; }
    const ED4_sequence_terminal *get_seqTerminal() const { return seqTerminal; }
    SEC_bond_def *bonds() const { return bonddef; }
    SEC_structure_toggler *structure() const { return toggler; }

    GB_ERROR init_toggler() const;
};



#else
#error sec_db.hxx included twice
#endif // SEC_DB_HXX
