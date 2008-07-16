#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <aw_awars.hxx>

#include <ntree.hxx>
#include <seer.hxx>
#include <seer_interface.hxx>

void SEER_strip_arb_file(){
    /* remove all SAI
     * remove all Species data, keep names
     * remove all Tables
     */
    GB_begin_transaction(gb_main);
    GB_push_my_security(gb_main);
    {/* ********** strip species **************/
        int cnt = 0;
        for (GBDATA *gb_species = GBT_first_species(gb_main); gb_species; gb_species = GBT_next_species(gb_species)) {
            GBDATA *gb_next;
            for (GBDATA *gb_elem = GB_child(gb_species); gb_elem; gb_elem = gb_next) {
                gb_next = GB_nextChild(gb_elem);
                const char *key = GB_read_key(gb_elem);
                if (strcmp(key,"name")){
                    GB_delete(gb_elem);
                }
            }
            cnt ++;
        }
        aw_message(GBS_global_string("%i species stripped",cnt));
    }
    {/* ********** delete sai **************/
        int cnt = 0;
        GBDATA *gb_sai_data = GB_entry(gb_main,"extended_data");
        GBDATA *gb_sai;
        while ((gb_sai = GB_child(gb_sai_data))){
            GB_delete(gb_sai);
            cnt++;
        }
        aw_message(GBS_global_string("%i sai deleted",cnt));
    }
    {/* ********** delete tables **************/
        GBDATA *gb_table;
        int cnt = 0;
        while ( (gb_table = GB_search(gb_main,"table_data/table",GB_FIND))){
            GB_delete(gb_table);
            cnt ++;
        }
        aw_message(GBS_global_string("%i tables deleted",cnt));
    }
    {   /* ***************** delete alignments *****************/
        GBDATA *gb_ali;
        while ( (gb_ali = GB_search(gb_main,"presets/alignment",GB_FIND))){
            GB_delete(gb_ali);
        }
    }
    { /*********************** delete attributes *************/
        GBDATA *gb_ck = GB_search(gb_main,CHANGE_KEY_PATH,GB_FIND);
        if (gb_ck) GB_delete(gb_ck);
    }
    GB_pop_my_security(gb_main);
    GB_commit_transaction(gb_main);
}

const char *SEER_opens_arb(AW_root *awr, SEER_OPENS_ARB_TYPE type){
    switch (type){
        case SEER_OPENS_NEW_ARB:{
            awr->awar( AWAR_DB_PATH )->write_string( "noname.arb");
            gb_main = GBT_open("noname.arb","wcd",0);
            break;
        }
        case SEER_OPENS_ARB_SKELETON:
        case SEER_OPENS_ARB_FULL:{
            char *fname = awr->awar(AWAR_DB_PATH)->read_string();
            gb_main = GBT_open(fname,"rw",0);
            delete fname;
            if (gb_main && type == SEER_OPENS_ARB_SKELETON){
                SEER_strip_arb_file();
            }
            break;
        }
    }
    if (gb_main) return 0;
    {
        GB_transaction var(gb_main);
        seer_global.transaction_counter_at_login = GB_read_clock(gb_main);      // needed to save
    }
    return GB_get_error();
}

long seer_extract_attributes(const char *,long val){
    SeerInterfaceAttribute *siat = (SeerInterfaceAttribute *)val;
    if (siat->requested){
        awt_add_new_changekey(gb_main,siat->name,GB_STRING);
    }
    return val;
}

long seer_sort_attributes(const char *,long val0,const char *,long val1){
    SeerInterfaceAttribute *siat0 = (SeerInterfaceAttribute *)val0;
    SeerInterfaceAttribute *siat1 = (SeerInterfaceAttribute *)val1;
    if (siat1->sortindex > siat0->sortindex ) return -1;
    if (siat1->sortindex < siat0->sortindex ) return 1;
    return 0;
}
SeerInterfaceTableDescription *seer_local_td;

long seer_extract_table_field(const char *,long val){
    GBDATA *gb_table = GBT_open_table(gb_main,seer_local_td->tablename,1);
    SeerInterfaceAttribute *at = (SeerInterfaceAttribute *)val;
    at->requested = 1;
    GB_TYPES type = GB_STRING;
    if (at->type = SIAT_LINK) type = GB_LINK;
    GBT_open_table_field(gb_table,at->name,type);
    GBDATA *gb_desc = GB_search(gb_table,"description",GB_STRING);
    GB_write_string(gb_desc,at->pubCmnt);
    return val;
}

long seer_extract_table(const char *,long val){
    SeerInterfaceTableDescription *td = (SeerInterfaceTableDescription *)val;
    GBT_open_table(gb_main,td->tablename,0);
    seer_local_td = td;
    GBS_hash_do_sorted_loop(td->attribute_hash,seer_extract_table_field,seer_sort_attributes);
    return val;
}

long seer_load_table(const char *,long val){
    SeerInterfaceTableDescription *td = (SeerInterfaceTableDescription *)val;
    GBDATA *gb_table = GBT_open_table(gb_main,td->tablename,0);
    seer_global.interface->queryTableData(seer_global.alignment_name,td->tablename);
    int max_i = seer_global.interface->numberofMatchingTableData();
    aw_openstatus("Loading table");
    int i = 0;

    SeerInterfaceTableData *tdata;
    for (tdata = seer_global.interface->firstTableData();
         tdata;
         tdata = seer_global.interface->nextTableData()){
        if (aw_status(double(i++)/double(max_i))) break;
        GBDATA *gb_table_entry = GBT_find_table_entry(gb_table,tdata->uniqueID);
        if (gb_table_entry){
            aw_message(GB_export_error("Table '%s' not loaded, duplicated version",tdata->uniqueID));
            continue;
        }
        gb_table_entry = GBT_open_table_entry(gb_table,tdata->uniqueID);
        SEER_retrieve_data_rek(td->attribute_hash,gb_table_entry,&tdata->attributes,0,0,0);
        delete tdata;
    }

    aw_closestatus();
    return val;
}

GBDATA *seer_table_link_follower(GBDATA *gb_main_local,GBDATA */*gb_link*/, const char *link){
    /* check for existing table entries, else load table on demand */
    GBDATA *gb_table;
    const char *sep;
    char table_name[256];

    sep = strchr(link,':');
    if (!sep){
        GB_export_error("Link '%s' is missing second ':' tag");
        return 0;
    }
    strncpy(table_name,link,sep-link);
    table_name[sep-link] = 0;
    gb_table = GBT_open_table(gb_main_local,table_name,1); // read only table

    if (!gb_table){
        aw_message(GB_export_error("Table '%s' does not exist",table_name));
        return 0;
    }
    const char *uniqueId = sep + 1;
    GBDATA *gb_table_entry =  GBT_find_table_entry(gb_table,uniqueId);
    if (gb_table_entry) return gb_table_entry;

    SeerInterfaceTableDescription *td = (SeerInterfaceTableDescription *)GBS_read_hash(seer_global.table_hash,table_name);
    if (!td){
        aw_message(GB_export_error("Table '%s' is not supported by SEER",link));
        return 0;               // this table is not supported by SEER
    }

    // load table on demand ???? transaction ???
    SeerInterfaceTableData *tdata = seer_global.interface->querySingleTableData(table_name,uniqueId);
    if (!tdata) return 0;
    gb_table_entry = GBT_open_table_entry(gb_table,tdata->uniqueID);
    SEER_retrieve_data_rek(td->attribute_hash,gb_table_entry,&tdata->attributes,0,0,0);
    delete tdata;
}



const char *SEER_populate_tables(AW_root *awr){
    /****************** alignment first *******************/
    GB_transaction transaction_scope(gb_main);

    while(1){
        char *alignment_name = awr->awar(AWAR_SEER_ALIGNMENT_NAME)->read_string();
        SeerInterfaceAlignment *sial = (SeerInterfaceAlignment *)GBS_read_hash(seer_global.alignment_hash,alignment_name);
        if (!sial){
            aw_message("No sequences loaded");
            delete alignment_name;
            break;
        }
        char arb_ali_name[256];
        sprintf(arb_ali_name,"ali_%s",alignment_name);
        char *type;
        switch(sial->typeofAlignment){
            case SIAT_DNA: type = "dna";break;
            case SIAT_PRO: type = "pro";break;
            case SIAT_RNA: type = "rna";break;
        }
        {
            seer_global.alignment_name = alignment_name;
            GBT_set_default_alignment(gb_main,arb_ali_name);
        }
        seer_global.alignment_type = sial->typeofAlignment;

        GBDATA *gb_ali = GBT_get_alignment(gb_main,arb_ali_name);
        GB_ERROR error = 0;
        if (!gb_ali){
            gb_ali = GBT_create_alignment(gb_main,arb_ali_name,10,0,1,type);
            if (!gb_ali){
                error = GB_get_error();
            }
        }else{
            char *old_type = GBT_get_alignment_type_string(gb_main,arb_ali_name);
            if (strcmp(type,old_type)){
                error = GB_export_error("Incompatible '%s' alignment types: '%s' != '%s'",arb_ali_name,type,old_type);
            }
        }
        if (error) return error;
        break;
    }

    /********************** attributes next ******************/
    {
        awt_add_new_changekey(gb_main,"name",GB_STRING);
        GBS_hash_do_sorted_loop(seer_global.attribute_hash,seer_extract_attributes,seer_sort_attributes);
    }
    /********************** finally tables *******************/
    {
        seer_global.interface->beginReadOnlyTransaction();
        GBS_hash_do_loop(seer_global.table_hash,seer_extract_table);
        seer_global.interface->commitReadOnlyTransaction();
        seer_global.delayed_load_of_tables = awr->awar(AWAR_SEER_TABLE_LOAD_BEHAVIOUR)->read_int();
        if (!seer_global.delayed_load_of_tables){ // load tables right now
            GBS_hash_do_loop(seer_global.table_hash,seer_load_table);
        }else{
            GB_ERROR error = GB_install_link_follower(gb_main,"T",seer_table_link_follower);
            if (error) aw_message(error);
        }
    }
    return 0;
}
