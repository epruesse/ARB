#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <awti_import.hxx>

#include <mg_merge.hxx>
#include "nt_import.hxx"
#include "ad_spec.hxx"
#include "GEN.hxx"

void nt_seq_load_cb(AW_root *awr, AW_CL, AW_CL){
	gb_dest = gb_main;
	AW_window *aww = ad_create_query_window(awr);
	ad_unquery_all();
	GB_ERROR error = MG_simple_merge(awr);
	ad_query_update_list();
	if (!error){
	    aww->show();
	}
}


/**
 * Opens the "Import Sequences" dialog from the
 * ARB main window (arb_ntree)
 *
 * O-Ton Harald: Saugehaecke
 */
void NT_import_sequences(AW_window *aww,AW_CL,AW_CL){

	if (gb_merge){
		GB_close(gb_merge);
	}

//     int gb_main_is_genom_db = 0;
//     {
//         GB_transaction  dummy1(gb_main);
//         GBDATA         *gb_main_genom_db          = GB_find(gb_main, GENOM_DB_TYPE, 0, down_level);
//         if (gb_main_genom_db) gb_main_is_genom_db = GB_read_int(gb_main_genom_db);
//     }

    AW_root *awr = aww->get_root();
    awr->awar_int(AWAR_READ_GENOM_DB, 0); // only default
//     awr->awar(AWAR_READ_GENOM_DB)->write_int(gb_main_is_genom_db ? 0 : 2);

	gb_merge = open_AWTC_import_window(aww->get_root(),"",0,(AW_RCB)nt_seq_load_cb,0,0);

    // change awar value (import window just opened!)
    int gb_main_is_genom_db  = GEN_is_genom_db(gb_main, 0);
    int gb_merge_is_genom_db = GEN_is_genom_db(gb_merge, gb_main_is_genom_db);

    gb_assert(gb_main_is_genom_db == gb_merge_is_genom_db);

//     {
//         GB_transaction dummy1(gb_main);
//         GB_transaction dummy2(gb_merge);

//         GBDATA *gb_main_genom_db = GB_find(gb_main, GENOM_DB_TYPE, 0, down_level);
//         if (gb_main_genom_db) gb_main_is_genom_db = GB_read_int(gb_main_genom_db);

//         GBDATA *gb_merge_genom_db = GB_search(gb_merge, GENOM_DB_TYPE, GB_INT);
//         GB_write_int(gb_merge_genom_db, gb_main_is_genom_db);
//     }

    awr->awar(AWAR_READ_GENOM_DB)->write_int(gb_main_is_genom_db ? 0 : 2);
}

