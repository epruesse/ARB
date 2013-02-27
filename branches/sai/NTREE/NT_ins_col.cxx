// =============================================================== //
//                                                                 //
//   File      : NT_ins_col.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ntree.hxx"
#include <arbdbt.h>
#include <insdel.h>
#include <aw_root.hxx>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <awt_sel_boxes.hxx>
#include <arb_defs.h>

#define nt_assert(bed) arb_assert(bed)

#define AWAR_INSDEL     "insdel/"
#define TMP_AWAR_INSDEL "tmp/" AWAR_INSDEL

#define AWAR_INSDEL_AMOUNT    AWAR_INSDEL "nchar"
#define AWAR_INSDEL_DELETABLE AWAR_INSDEL "characters"
#define AWAR_INSDEL_RANGE     AWAR_INSDEL "range"
#define AWAR_INSDEL_SAI       AWAR_INSDEL "sai"
#define AWAR_INSDEL_CONTAINS  AWAR_INSDEL "contains"
#define AWAR_INSDEL_SAI_CHARS AWAR_INSDEL "saichars"
#define AWAR_INSDEL_AFFECTED  AWAR_INSDEL "affected"
#define AWAR_INSDEL_WHAT      TMP_AWAR_INSDEL "what"
#define AWAR_INSDEL_DIRECTION AWAR_INSDEL "direction"

enum UseRange { RANGES, SINGLE_COLUMNS };
enum SaiContains { CONTAINS, DOESNT_CONTAIN };
enum InsertWhere { INFRONTOF, BEHIND };
enum InsdelMode { INSERT, DELETE };

static void range_changed_cb(AW_root *root) {
    UseRange use = UseRange(root->awar_int(AWAR_INSDEL_RANGE)->read_int());
    const char *what;
    switch (use) {
        case RANGES:         what = "selected ranges";  break;
        case SINGLE_COLUMNS: what = "selected columns"; break;
    }
    root->awar_string(AWAR_INSDEL_WHAT)->write_string(what);
}

void create_insertDeleteColumn_variables(AW_root *root, AW_default props) {
    root->awar_int   (AWAR_CURSOR_POSITION,  info2bio(0),    GLOBAL.gb_main);
    root->awar_int   (AWAR_INSDEL_AMOUNT,    0,              props)->set_minmax(0, 9999999);
    root->awar_string(AWAR_INSDEL_DELETABLE, "-.",           props);
    root->awar_int   (AWAR_INSDEL_RANGE,     RANGES,         props)->add_callback(range_changed_cb);
    root->awar_string(AWAR_INSDEL_SAI,       "",             props);
    root->awar_int   (AWAR_INSDEL_CONTAINS,  DOESNT_CONTAIN, props);
    root->awar_string(AWAR_INSDEL_SAI_CHARS, "-.",           props);
    root->awar_int   (AWAR_INSDEL_AFFECTED,  0,              props);
    root->awar_string(AWAR_INSDEL_WHAT,      "???",          props);
    root->awar_int   (AWAR_INSDEL_DIRECTION, BEHIND,         props);

    range_changed_cb(root);
}

static void insdel_event(AW_window *aws, AW_CL cl_insdelmode) {
    InsdelMode  mode = InsdelMode(cl_insdelmode);
    AW_root    *root = aws->get_root();

    long  pos     = bio2info(root->awar(AWAR_CURSOR_POSITION)->read_int());
    long  nchar   = root->awar(AWAR_INSDEL_AMOUNT)->read_int();
    char *deletes = root->awar(AWAR_INSDEL_DELETABLE)->read_string();

    if (mode == DELETE) nchar = -nchar;

    GB_ERROR error = GB_begin_transaction(GLOBAL.gb_main);
    if (!error) {
        char *alignment = GBT_get_default_alignment(GLOBAL.gb_main);

        if (alignment) {
            error             = ARB_insdel_columns(GLOBAL.gb_main, alignment, pos, nchar, deletes);
            if (!error) error = GBT_check_data(GLOBAL.gb_main, 0);
        }
        else {
            error = "no alignment found";
        }
        free(alignment);
    }

    GB_end_transaction_show_error(GLOBAL.gb_main, error, aw_message);
    free(deletes);
}

static void insdel_sai_event(AW_window *aws, AW_CL cl_insdelmode) {
    aw_message("not impl");
}

AW_window *create_insertDeleteColumn_window(AW_root *root) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(root, "INSDEL_COLUMNS", "Insert/delete columns");

        aws->load_xfig("insdel.fig");
        aws->button_length(8);

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(AW_POPUP_HELP, (AW_CL)"insdel.hlp");
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
        aws->callback(insdel_event, (AW_CL)INSERT); aws->create_button("INSERT", "INSERT", "I");
        aws->callback(insdel_event, (AW_CL)DELETE); aws->create_button("DELETE", "DELETE", "D");

        return aws;
    }
}

AW_window *create_insertDeleteBySAI_window(AW_root *root, AW_CL cl_gbmain) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        GBDATA *gb_main = (GBDATA*)cl_gbmain;
        aws             = new AW_window_simple;

        aws->init(root, "INSDEL_BY_SAI", "Insert/delete using SAI");

        aws->load_xfig("insdel_sai.fig");
        aws->button_length(8);

        aws->callback((AW_CB0)AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(AW_POPUP_HELP, (AW_CL)"insdel_sai.hlp");
        aws->at("help");
        aws->create_button("HELP", "HELP", "H");

        aws->at("select");
        aws->create_option_menu(AWAR_INSDEL_RANGE);
        aws->insert_option("ranges",  "r", RANGES);
        aws->insert_option("columns", "c", SINGLE_COLUMNS);
        aws->update_option_menu();

        aws->button_length(25);

        aws->at("sai");
        awt_create_SAI_selection_button(gb_main, aws, AWAR_INSDEL_SAI);

        aws->at("contains");
        aws->create_option_menu(AWAR_INSDEL_CONTAINS);
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
        aws->callback(insdel_sai_event, (AW_CL)DELETE);
        aws->create_button("DELETE", "DELETE", "D");

        aws->at("deletable");
        aws->create_input_field(AWAR_INSDEL_DELETABLE, 7);

        aws->at("insert");
        aws->callback(insdel_sai_event, (AW_CL)INSERT);
        aws->create_button("INSERT", "INSERT", "I");

        aws->at("amount");
        aws->create_input_field(AWAR_INSDEL_AMOUNT, 7);

        aws->at("direction");
        aws->create_option_menu(AWAR_INSDEL_DIRECTION);
        aws->insert_option("in front of", "f", INFRONTOF);
        aws->insert_option("behind",      "b", BEHIND);
        aws->update_option_menu();

        aws->button_length(15);
        aws->at("what0"); aws->create_button(0, AWAR_INSDEL_WHAT);
        aws->at("what1"); aws->create_button(0, AWAR_INSDEL_WHAT);
        aws->at("what2"); aws->create_button(0, AWAR_INSDEL_WHAT);

        return aws;
    }
}
