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

void NT_import_sequences(AW_window *aww,AW_CL,AW_CL){
	if (gb_merge){
		GB_close(gb_merge);
	}
	gb_merge = open_AWTC_import_window(aww->get_root(),"",0,(AW_RCB)nt_seq_load_cb,0,0);
}

