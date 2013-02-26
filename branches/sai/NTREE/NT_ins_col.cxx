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
#include <arb_defs.h>

#define nt_assert(bed) arb_assert(bed)

#define AWAR_INSDEL "insdel/"

#define AWAR_INSDEL_AMOUNT    AWAR_INSDEL "nchar"
#define AWAR_INSDEL_DELETABLE AWAR_INSDEL "characters"

void create_insertDeleteColumn_variables(AW_root *root, AW_default db1) {
    root->awar_int   (AWAR_CURSOR_POSITION,  info2bio(0), GLOBAL.gb_main);
    root->awar_int   (AWAR_INSDEL_AMOUNT,    0,           db1)->set_minmax(0, 9999999);
    root->awar_string(AWAR_INSDEL_DELETABLE, "",          db1);
}

enum InsdelMode { INSERT, DELETE };

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
            error             = ARB_insert_character(GLOBAL.gb_main, alignment, pos, nchar, deletes);
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

AW_window *create_insertDeleteColumn_window(AW_root *root, AW_default /*def*/) {
    static AW_window_simple *aws = 0;
    if (aws) return aws;
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
    aws->callback(insdel_event, (AW_CL)INSERT);
    aws->create_button("INSERT", "INSERT", "I");

    aws->callback(insdel_event, (AW_CL)DELETE);
    aws->create_button("DELETE", "DELETE", "D");

    return (AW_window *)aws;
}
