#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <awti_import.hxx>
#include <awt_canvas.hxx>
#include <mg_merge.hxx>

#include "nt_import.hxx"
#include "nt_internal.h"
#include "ad_spec.hxx"
#include "GEN.hxx"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define nt_assert(bed) arb_assert(bed)

void nt_seq_load_cb(AW_root *awr, AW_CL, AW_CL){
    gb_dest          = gb_main;
    AW_window *aww   = ad_create_query_window(awr);
    ad_unquery_all();
    GB_ERROR   error = MG_simple_merge(awr);
    if (!error) error = NT_format_all_alignments(gb_main);
    ad_query_update_list();
    if (!error) aww->show();
}


/**
 * Opens the "Import Sequences" dialog from the
 * ARB main window (arb_ntree)
*/
void NT_import_sequences(AW_window *aww,AW_CL,AW_CL){

    if (gb_merge){
        GB_close(gb_merge);
    }

    AW_root *awr = aww->get_root();

    awr->awar_int(AWAR_READ_GENOM_DB, IMP_PLAIN_SEQUENCE); // value is overwritten below

    gb_merge = open_AWTC_import_window(aww->get_root(),"",0,(AW_RCB)nt_seq_load_cb,0,0);

    // change awar values (import window just opened!)
    int gb_main_is_genom_db  = GEN_is_genome_db(gb_main, 0);
    int gb_merge_is_genom_db = GEN_is_genome_db(gb_merge, gb_main_is_genom_db);

    nt_assert(gb_main_is_genom_db == gb_merge_is_genom_db);

#if defined(DEVEL_ARTEM)
    awr->awar(AWAR_READ_GENOM_DB)->write_int(gb_main_is_genom_db ? IMP_GENOME_FLATFILE : IMP_PLAIN_SEQUENCE);
#else    
    awr->awar(AWAR_READ_GENOM_DB)->write_int(gb_main_is_genom_db ? IMP_GENOME_GENEBANK : IMP_PLAIN_SEQUENCE);
#endif // DEVEL_ARTEM

    {
        GB_transaction dummy(gb_main);
        char *ali_name = GBT_get_default_alignment(gb_main);
        char *ali_type = GBT_get_alignment_type_string(gb_main, ali_name);

        AWTC_import_set_ali_and_type(awr, ali_name, ali_type, gb_main);

        free(ali_type);
        free(ali_name);
    }
}

