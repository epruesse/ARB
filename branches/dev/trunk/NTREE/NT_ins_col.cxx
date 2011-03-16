// =============================================================== //
//                                                                 //
//   File      : NT_ins_col.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <arbdbt.h>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <arb_defs.h>

#define nt_assert(bed) arb_assert(bed)

extern GBDATA *GLOBAL_gb_main;     /* muss existieren */

void create_insertchar_variables(AW_root *root, AW_default db1) {
    root->awar_int   (AWAR_CURSOR_POSITION,    info2bio(0),  GLOBAL_gb_main);
    root->awar_int   ("insertchar/nchar",      0,  db1)->set_minmax(0, 999000);
    root->awar_string("insertchar/characters", "", db1);
}

void awt_inserchar_event(AW_window *aws, AW_CL awcl_mode) {
    int mode = (int)awcl_mode; // 1 = insert, -1 = delete
    nt_assert(mode == -1 || mode == 1);

    AW_root *root    = aws->get_root();
    long     pos     = bio2info(root->awar(AWAR_CURSOR_POSITION)->read_int());
    long     nchar   = root->awar("insertchar/nchar")->read_int() * mode;
    char    *deletes = root->awar("insertchar/characters")->read_string();

    GB_ERROR error = GB_begin_transaction(GLOBAL_gb_main);
    if (!error) {
        char *alignment = GBT_get_default_alignment(GLOBAL_gb_main);

        if (alignment) {
            error             = GBT_insert_character(GLOBAL_gb_main, alignment, pos, nchar, deletes);
            if (!error) error = GBT_check_data(GLOBAL_gb_main, 0);
        }
        else {
            error = "no alignment found";
        }
        free(alignment);
    }

    GB_end_transaction_show_error(GLOBAL_gb_main, error, aw_message);
    free(deletes);
}

AW_window *create_insertchar_window(AW_root *root, AW_default /*def*/) {
    static AW_window_simple *aws = 0;
    if (aws) return aws;
    aws = new AW_window_simple;

    aws->init(root, "INSERT_COLUMN", "INSERT CHAR");

    aws->load_xfig("inschar.fig");
    aws->button_length(8);

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(AW_POPUP_HELP, (AW_CL)"insdelchar.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->label_length(27);

    aws->at("pos");
    aws->label("Sequence Position");
    aws->create_input_field(AWAR_CURSOR_POSITION, 6);

    aws->at("len");
    aws->label("How many Characters");
    aws->create_input_field("insertchar/nchar", 6);

    aws->at("characters");
    aws->label("Delete Only (% = all)");
    aws->create_input_field("insertchar/characters", 6);

    aws->callback(awt_inserchar_event, (AW_CL)1);
    aws->at("insert");
    aws->create_button("INSERT", "INSERT", "I");

    aws->callback(awt_inserchar_event, (AW_CL)-1);
    aws->at("delete");
    aws->create_button("DELETE", "DELETE", "D");

    return (AW_window *)aws;
}
