// =============================================================== //
//                                                                 //
//   File      : NT_ins_col.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"

#include <RangeList.h>
#include <arbdbt.h>
#include <insdel.h>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <awt_sel_boxes.hxx>
#include <arb_defs.h>
#include <awt_config_manager.hxx>

#define AWAR_INSDEL     "insdel/"
#define TMP_AWAR_INSDEL "tmp/" AWAR_INSDEL

#define AWAR_INSDEL_AMOUNT    AWAR_INSDEL "nchar"
#define AWAR_INSDEL_DELETABLE AWAR_INSDEL "characters"
#define AWAR_INSDEL_RANGE     AWAR_INSDEL "range"
#define AWAR_INSDEL_SAI       AWAR_INSDEL "sainame"
#define AWAR_INSDEL_CONTAINS  AWAR_INSDEL "contain"
#define AWAR_INSDEL_SAI_CHARS AWAR_INSDEL "saichars"
#define AWAR_INSDEL_AFFECTED  TMP_AWAR_INSDEL "affected"
#define AWAR_INSDEL_WHAT      TMP_AWAR_INSDEL "what"
#define AWAR_INSDEL_DIRECTION AWAR_INSDEL "direction"

enum SaiContains { DOESNT_CONTAIN, CONTAINS };
enum InsdelMode { INSERT, DELETE };

class StaticData {
    char      *ali;
    RangeList  ranges;

public:
    StaticData() : ali(NULL) {}
    StaticData(const StaticData& other) : ali(nulldup(other.ali)), ranges(other.ranges) {}
    DECLARE_ASSIGNMENT_OPERATOR(StaticData);
    ~StaticData() { free(ali); }

    GB_ERROR track_ali(GBDATA *gb_main) {
        freeset(ali, GBT_get_default_alignment(gb_main));
        return ali ? NULL : "no alignment found";
    }

    const char *get_ali() const { return ali; }

    const RangeList& get_ranges() const { return ranges; }
    void set_ranges(const RangeList& new_ranges) { ranges = new_ranges; }
};

static StaticData SELECTED;

static void cleanup_when_closing(AW_window*) {
    SELECTED = StaticData();
}

static int columns_of(const RangeList& ranges) {
    int count = 0;
    for (RangeList::iterator r = ranges.begin(); r != ranges.end(); ++r) {
        count += r->size();
    }
    return count;
}

static void range_count_update_cb(AW_root *root) {
    UseRange use   = UseRange(root->awar(AWAR_INSDEL_RANGE)->read_int());
    int      count = 0;
    switch (use) {
        case RANGES:         count = SELECTED.get_ranges().size(); break;
        case SINGLE_COLUMNS: count = columns_of(SELECTED.get_ranges()); break;
    }
    root->awar(AWAR_INSDEL_AFFECTED)->write_int(count);
}

static void range_changed_cb(AW_root *root) {
    UseRange    use  = UseRange(root->awar(AWAR_INSDEL_RANGE)->read_int());
    const char *what = NULL;
    switch (use) {
        case RANGES:         what = "selected ranges";  break;
        case SINGLE_COLUMNS: what = "selected columns"; break;
    }
    root->awar(AWAR_INSDEL_WHAT)->write_string(what);
    range_count_update_cb(root);
}

static GB_ERROR update_RangeList(AW_root *root, GBDATA *gb_main) {
    const char     *saiName = root->awar(AWAR_INSDEL_SAI)->read_char_pntr();
    GB_transaction  ta(gb_main);
    GBDATA         *gb_sai  = GBT_expect_SAI(gb_main, saiName);
    GB_ERROR        error   = NULL;

    if (!gb_sai) error = GB_await_error();
    if (!error) error  = SELECTED.track_ali(gb_main);

    if (!error) {
        GBDATA *gb_data = GBT_find_sequence(gb_sai, SELECTED.get_ali());
        if (!gb_data) {
            if (GB_have_error()) error = GB_await_error();
            else error                 = GBS_global_string("SAI '%s' has no data in alignment '%s'", saiName, SELECTED.get_ali());
        }
        else {
            const char *data = GB_read_char_pntr(gb_data);
            if (!data) error = GB_await_error();
            else {
                const char  *chars    = root->awar(AWAR_INSDEL_SAI_CHARS)->read_char_pntr();
                SaiContains  contains = SaiContains(root->awar(AWAR_INSDEL_CONTAINS)->read_int());

                SELECTED.set_ranges(build_RangeList_from_string(data, chars, contains == DOESNT_CONTAIN));
            }
        }
    }
    if (error) {
        SELECTED.set_ranges(RangeList());
        if (!saiName[0]) error = NULL; // do not show "Could not find extended with name ''"
    }
    range_count_update_cb(root);

    return error;
}

static void update_RangeList_cb(AW_root *root) {
    aw_message_if(update_RangeList(root, GLOBAL.gb_main));
}

void create_insertDeleteColumn_variables(AW_root *root, AW_default props) {
    root->awar_int   (AWAR_CURSOR_POSITION,  info2bio(0), GLOBAL.gb_main);
    root->awar_int   (AWAR_INSDEL_AMOUNT,    0,           props)->set_minmax(0, 9999999);
    root->awar_string(AWAR_INSDEL_DELETABLE, "-.",        props);
    root->awar_int   (AWAR_INSDEL_RANGE,     RANGES,      props)->add_callback(range_changed_cb);

    root->awar_string(AWAR_INSDEL_SAI,       "",             props)->add_callback(update_RangeList_cb);
    root->awar_int   (AWAR_INSDEL_CONTAINS,  DOESNT_CONTAIN, props)->add_callback(update_RangeList_cb);
    root->awar_string(AWAR_INSDEL_SAI_CHARS, "-.",           props)->add_callback(update_RangeList_cb);

    root->awar_int   (AWAR_INSDEL_AFFECTED,  0,      props);
    root->awar_string(AWAR_INSDEL_WHAT,      "???",  props);
    root->awar_int   (AWAR_INSDEL_DIRECTION, BEHIND, props);

    range_changed_cb(root);
    update_RangeList(root, GLOBAL.gb_main);
}

static void insdel_event(AW_window *aws, InsdelMode mode) {
    GBDATA  *gb_main = GLOBAL.gb_main;
    AW_root *root    = aws->get_root();

    long  pos       = bio2info(root->awar(AWAR_CURSOR_POSITION)->read_int());
    long  nchar     = root->awar(AWAR_INSDEL_AMOUNT)->read_int();
    const char *deletable = root->awar(AWAR_INSDEL_DELETABLE)->read_char_pntr();

    if (mode == DELETE) nchar = -nchar;

    GB_ERROR error    = GB_begin_transaction(gb_main);
    if (!error) error = SELECTED.track_ali(gb_main);
    if (!error) error = ARB_insdel_columns(gb_main, SELECTED.get_ali(), pos, nchar, deletable);
    if (!error) error = GBT_check_data(gb_main, 0);

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

static void insdel_sai_event(AW_window *aws, InsdelMode mode) {
    GBDATA   *gb_main = GLOBAL.gb_main;
    GB_ERROR  error   = GB_begin_transaction(GLOBAL.gb_main);
    if (!error) error = SELECTED.track_ali(gb_main);

    if (!error) {
        AW_root *root = aws->get_root();

        switch (mode) {
            case INSERT: {
                UseRange    units  = UseRange(root->awar(AWAR_INSDEL_RANGE)->read_int());
                InsertWhere where  = InsertWhere(root->awar(AWAR_INSDEL_DIRECTION)->read_int());
                size_t      amount = root->awar(AWAR_INSDEL_AMOUNT)->read_int();

                error = ARB_insert_columns_using_SAI(gb_main, SELECTED.get_ali(), SELECTED.get_ranges(), units, where, amount);
                break;
            }
            case DELETE: {
                const char *deletable = root->awar(AWAR_INSDEL_DELETABLE)->read_char_pntr();

                error = ARB_delete_columns_using_SAI(gb_main, SELECTED.get_ali(), SELECTED.get_ranges(), deletable);
                break;
            }
        }
    }
    if (!error) error = GBT_check_data(gb_main, 0);

    GB_end_transaction_show_error(gb_main, error, aw_message);
}

AW_window *create_insertDeleteColumn_window(AW_root *root) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(root, "INSDEL_COLUMNS", "Insert/delete columns");

        aws->load_xfig("insdel.fig");
        aws->button_length(8);

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("insdel.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        aws->label_length(27);

        aws->at("pos");
        aws->label("Sequence Position");
        aws->create_input_field(AWAR_CURSOR_POSITION, 7);

        aws->at("len");
        aws->label("How many Characters");
        aws->create_input_field(AWAR_INSDEL_AMOUNT, 7);

        aws->at("characters");
        aws->label("Delete Only (% = all)");
        aws->create_input_field(AWAR_INSDEL_DELETABLE, 7);

        aws->auto_space(10, 0);

        aws->at("actions");
        aws->callback(makeWindowCallback(insdel_event, INSERT)); aws->create_button("INSERT", "INSERT", "I");
        aws->callback(makeWindowCallback(insdel_event, DELETE)); aws->create_button("DELETE", "DELETE", "D");
    }
    return aws;
}

static AWT_config_mapping_def insdel_by_SAI_config_def[] = {
    { AWAR_INSDEL_RANGE,     "range" },
    { AWAR_INSDEL_SAI,       "sai" },
    { AWAR_INSDEL_CONTAINS,  "contain" },
    { AWAR_INSDEL_SAI_CHARS, "chars" },
    { AWAR_INSDEL_DELETABLE, "deletable" },
    { AWAR_INSDEL_AMOUNT,    "amount" },
    { AWAR_INSDEL_DIRECTION, "direction" },

    { 0, 0 },
};

static AWT_predefined_config insdel_by_SAI_predef_config[] = {
    {
        "*gaps_by_variability",
        "Use to insert 2 gaps next to all\ncolumns with high variability",
        "amount='2';chars='123';contain='1';direction='1';range='1';sai='POS_VAR_BY_PARSIMONY'",
    },
    {
        "*erase_columns_without_data",
        "selects all columns where \nMAX_FREQUENCY contains '='",
        "chars='=';contain='1';deletable='-.';range='1';sai='MAX_FREQUENCY'",
    },
    { 0, 0, 0 },
};

AW_window *create_insertDeleteBySAI_window(AW_root *root, GBDATA *gb_main) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(root, "INSDEL_BY_SAI", "Insert/delete using SAI");

        aws->load_xfig("insdel_sai.fig");
        aws->button_length(8);

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("insdel_sai.hlp"));
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        aws->at("config");
        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "insdel_by_sai", insdel_by_SAI_config_def, NULL, insdel_by_SAI_predef_config);

        aws->at("select");
        aws->create_option_menu(AWAR_INSDEL_RANGE, true);
        aws->insert_option("ranges",  "r", RANGES);
        aws->insert_option("columns", "c", SINGLE_COLUMNS);
        aws->update_option_menu();

        aws->button_length(25);

        aws->at("sai");
        awt_create_SAI_selection_button(gb_main, aws, AWAR_INSDEL_SAI);

        aws->at("contains");
        aws->create_option_menu(AWAR_INSDEL_CONTAINS, true);
        aws->insert_option("contains",        "c", CONTAINS);
        aws->insert_option("doesn't contain", "d", DOESNT_CONTAIN);
        aws->update_option_menu();

        aws->at("characters");
        aws->create_input_field(AWAR_INSDEL_SAI_CHARS, 18);

        aws->button_length(18);

        aws->at("affected");
        aws->create_button(0, AWAR_INSDEL_AFFECTED, 0, "+");

        aws->button_length(7);

        aws->at("delete");
        aws->callback(makeWindowCallback(insdel_sai_event, DELETE));
        aws->create_button("DELETE", "DELETE", "D");

        aws->at("deletable");
        aws->create_input_field(AWAR_INSDEL_DELETABLE, 7);

        aws->at("insert");
        aws->callback(makeWindowCallback(insdel_sai_event, INSERT));
        aws->create_button("INSERT", "INSERT", "I");

        aws->at("amount");
        aws->create_input_field(AWAR_INSDEL_AMOUNT, 7);

        aws->at("direction");
        aws->create_option_menu(AWAR_INSDEL_DIRECTION, true);
        aws->insert_option("in front of", "f", INFRONTOF);
        aws->insert_option("behind",      "b", BEHIND);
        aws->update_option_menu();

        // add window text which depends on AWAR_INSDEL_RANGE
        aws->button_length(15);
        aws->at("what0"); aws->create_button(0, AWAR_INSDEL_WHAT);
        aws->at("what1"); aws->create_button(0, AWAR_INSDEL_WHAT);
        aws->at("what2"); aws->create_button(0, AWAR_INSDEL_WHAT);

        aws->on_hide(cleanup_when_closing);
    }
    return aws;
}
