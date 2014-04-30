// =============================================================== //
//                                                                 //
//   File      : NT_import.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "NT_local.h"

#include <dbui.h>
#include <GEN.hxx>
#include <mg_merge.hxx>
#include <macros.hxx>

#include <awti_import.hxx>
#include <awt.hxx>
#include <awt_canvas.hxx>

#include <aw_awar.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>

#include <arbdbt.h>

static void nt_seq_load_cb(AW_root *awr) {
    GLOBAL_gb_dst     = GLOBAL.gb_main;
    AW_window *aww    = DBUI::create_species_query_window(awr, GLOBAL.gb_main);
    DBUI::unquery_all();
    GB_ERROR   error  = MERGE_sequences_simple(awr);
    if (!error) error = NT_format_all_alignments(GLOBAL.gb_main);
    DBUI::query_update_list();
    if (!error) aww->activate();

    AWTI_cleanup_importer(); // closes import DB

    GLOBAL_gb_src = NULL;
    GLOBAL_gb_dst = NULL;
}


void NT_import_sequences(AW_window *aww, AW_CL, AW_CL) {
    /*! Opens the "Import Sequences" dialog from the ARB main window (ARB_NTREE)
     */

    AW_root *awr = aww->get_root();

    awr->awar_int(AWAR_READ_GENOM_DB, IMP_PLAIN_SEQUENCE); // value is overwritten below

    nt_assert(!GLOBAL_gb_src);
    AWTI_open_import_window(aww->get_root(), "", false, GLOBAL.gb_main, makeRootCallback(nt_seq_load_cb));
    GLOBAL_gb_src = AWTI_peek_imported_DB();

    nt_assert(got_macro_ability(awr));

    // change awar values (import window just opened!)

    int gb_main_is_genom_db;
    {
        GB_transaction t1(GLOBAL.gb_main);
        GB_transaction t2(GLOBAL_gb_src);

        gb_main_is_genom_db = GEN_is_genome_db(GLOBAL.gb_main, 0);
        IF_ASSERTION_USED(int gb_merge_is_genom_db =) GEN_is_genome_db(GLOBAL_gb_src, gb_main_is_genom_db);

        nt_assert(gb_main_is_genom_db == gb_merge_is_genom_db);
    }


    awr->awar(AWAR_READ_GENOM_DB)->write_int(gb_main_is_genom_db ? IMP_GENOME_FLATFILE : IMP_PLAIN_SEQUENCE);

    {
        GB_transaction ta(GLOBAL.gb_main);

        char *ali_name = GBT_get_default_alignment(GLOBAL.gb_main);
        char *ali_type = GBT_get_alignment_type_string(GLOBAL.gb_main, ali_name);

        if (!ali_type) aw_message(GB_await_error());

        AWTI_import_set_ali_and_type(awr, ali_name, ali_type, GLOBAL.gb_main);

        free(ali_type);
        free(ali_name);
    }
}

