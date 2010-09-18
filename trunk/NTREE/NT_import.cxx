// =============================================================== //
//                                                                 //
//   File      : NT_import.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "nt_internal.h"
#include "ad_spec.hxx"

#include <GEN.hxx>
#include <awti_import.hxx>
#include <awt_canvas.hxx>
#include <mg_merge.hxx>
#include <aw_awar.hxx>
#include <aw_root.hxx>
#include <arbdbt.h>

#define nt_assert(bed) arb_assert(bed)

extern GBDATA *GLOBAL_gb_main;

void nt_seq_load_cb(AW_root *awr, AW_CL, AW_CL) {
    GLOBAL_gb_dest    = GLOBAL_gb_main;
    AW_window *aww    = ad_create_query_window(awr);
    ad_unquery_all();
    GB_ERROR   error  = MG_simple_merge(awr);
    if (!error) error = NT_format_all_alignments(GLOBAL_gb_main);
    ad_query_update_list();
    if (!error) aww->activate();
}


void NT_import_sequences(AW_window *aww, AW_CL, AW_CL) {
    /*! Opens the "Import Sequences" dialog from the ARB main window (ARB_NTREE)
     */

    if (GLOBAL_gb_merge) {
#if defined(DEBUG)
        AWT_browser_forget_db(GLOBAL_gb_merge);
#endif // DEBUG
        GB_close(GLOBAL_gb_merge);
    }

    AW_root *awr = aww->get_root();

    awr->awar_int(AWAR_READ_GENOM_DB, IMP_PLAIN_SEQUENCE); // value is overwritten below

    GLOBAL_gb_merge = open_AWTC_import_window(aww->get_root(), "", false, GLOBAL_gb_main, (AW_RCB)nt_seq_load_cb, 0, 0);

    // change awar values (import window just opened!)

    int gb_main_is_genom_db, gb_merge_is_genom_db;
    {
        GB_transaction t1(GLOBAL_gb_main);
        GB_transaction t2(GLOBAL_gb_merge);

        gb_main_is_genom_db  = GEN_is_genome_db(GLOBAL_gb_main, 0);
        gb_merge_is_genom_db = GEN_is_genome_db(GLOBAL_gb_merge, gb_main_is_genom_db);
    }

    nt_assert(gb_main_is_genom_db == gb_merge_is_genom_db);

    awr->awar(AWAR_READ_GENOM_DB)->write_int(gb_main_is_genom_db ? IMP_GENOME_FLATFILE : IMP_PLAIN_SEQUENCE);

    {
        GB_transaction dummy(GLOBAL_gb_main);
        char *ali_name = GBT_get_default_alignment(GLOBAL_gb_main);
        char *ali_type = GBT_get_alignment_type_string(GLOBAL_gb_main, ali_name);

        AWTC_import_set_ali_and_type(awr, ali_name, ali_type, GLOBAL_gb_main);

        free(ali_type);
        free(ali_name);
    }
}

