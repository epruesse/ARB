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


/**************************** UPLOAD SEQUENCE ***********************************/

long seer_upload_species_attributes_rek(SeerInterfaceDataSet *sd, GBDATA *gb_species, int transaction_limit,
				    const char *arb_alignment_name, const char *prefix,char *end_of_prefix ){
    // return the number of items actually new
    GBDATA *gbd;
    int items_read = 0;
    for (gbd = GB_child(gb_species); gbd; gbd = GB_nextChild(gbd)) {
	const char *key = GB_read_key_pntr(gbd);
	sprintf(end_of_prefix,GB_read_key_pntr(gbd));

	if (!strcmp(prefix,"name")) continue;
	if (!strcmp(prefix,SEER_KEY_FLAG)) continue;
	SeerInterfaceAttribute *at = (SeerInterfaceAttribute *)GBS_read_hash(seer_global.arb_attribute_hash,prefix);
	if (!at) continue;
	if (at->element_for_upload==0) continue;

	if (GB_read_clock(gbd) <= transaction_limit){
	    continue;
	}

	GB_TYPES type = GB_read_type(gbd);
	if (type == GB_DB){
	    if (prefix == end_of_prefix && !strncmp(key,"ali_",4)){ // it's a sequence
		continue;
	    }else{
		SeerInterfaceDataSet *subset = new SeerInterfaceDataSet();
		subset->key = strdup(key);
		sd->addData(subset);
		items_read += seer_upload_species_attributes_rek(subset,gbd,transaction_limit,arb_alignment_name,prefix,end_of_prefix + strlen(end_of_prefix));
	    }
	    continue;
	}
	if (type == GB_STRING){
	    SeerInterfaceDataString *dataString = new SeerInterfaceDataString(key,GB_read_char_pntr(gbd));
	    dataString->importQualityLevel = SQIM_MACHINE_GENERATED;
	    sd->addData(dataString);
	    items_read++;
	}
    }
    *end_of_prefix = 0;
    return items_read;
}

long seer_upload_species_sequence(SeerInterfaceDataStringSet *ss, GBDATA *gb_species, int transaction_limit, const char *arb_alignment_name ){
    GBDATA *gb_ali = GB_entry(gb_species, arb_alignment_name);
    if (!gb_ali) return 0;
    if (GB_read_type(gb_ali) != GB_DB) return 0;
    GBDATA *gb_data;
    int count_exported_items = 0;
    if (GB_read_clock(gb_ali) <= transaction_limit) return 0;

    for (gb_data = GB_child(gb_ali); gb_data; gb_data = GB_nextChild(gb_data)) {
	if (GB_read_clock(gb_data) < transaction_limit) continue;
	GB_TYPES type = GB_read_type(gb_data);
	const char *key = GB_read_key_pntr(gb_data);
	if (!strcmp(key,"data")) key = "Sequence";

	switch(type){
	case GB_STRING:
	    ss->addData(new SeerInterfaceDataString(key,GB_read_char_pntr(gb_data)));
	    break;
	case GB_BITS:
	    ss->addData(new SeerInterfaceDataString(key,GB_read_bits_pntr(gb_data,'-','+')));
	    break;
	default:
	    break;
	}
	ss->importQualityLevel = SQIM_MACHINE_GENERATED;
	count_exported_items++;
    }
    return count_exported_items;
}


GB_ERROR seer_upload_one_species(SeerInterfaceSequenceData *sd,GBDATA *gb_species, int transaction_limit,
				 const char *arb_alignment_name,const char *alignment_name, int sequence_flag,int SAI_flag ){
    char buffer[4096];
    buffer[0] = 0;
    int count = seer_upload_species_attributes_rek(&sd->attributes, gb_species, transaction_limit,
						   arb_alignment_name, &buffer[0],&buffer[0] );
    if (sequence_flag){
	count += seer_upload_species_sequence(&sd->sequences,gb_species,transaction_limit,arb_alignment_name);
    }
    if (SAI_flag){
	sd->sequenceType = SISDT_ETC;
    }else{
	sd->sequenceType = SISDT_REAL_SEQUENCE;
    }
    if (count){
	sd->print();
	SeerInterfaceError *error;
	while (error = seer_global.interface->checkSequence(alignment_name,sd)){
	    if (error->errorType != SIET_QUALITY_ERROR){
		if (aw_message(error->errorString,"Cancel Species,Cancel All") == 1){
		    return error->errorString;
		}
		return 0;
	    }
	    switch(aw_message(error->errorString,"CANCEL SPECIES,CANCEL ATTRIBUTE,STORE ATTRIBUTE")){
	    case 0:
		delete sd;
		delete error;
		return 0;
	    case 1:
		error->errorSource->importQualityLevel = SQIM_MASKED_OUT;
		break;
	    case 2:
		error->errorSource->importQualityLevel = SQIM_DOUBLE_CHECKED;
		break;
	    }
	    delete error;
	    error = 0;
	}
	error = seer_global.interface->storeSequence(alignment_name,sd);
	if (error){
	    aw_message(error->errorString);
	    delete error;
	    error = 0;
	}
    }
    delete sd;

}

void seer_upload_all_species(AW_root *awr,int sequence_flag, int SAI_flag){
    GBDATA              *gb_species;
    GBDATA              *gb_next_species;
    SEER_SAVE_WHAT_TYPE  save_what          = (SEER_SAVE_WHAT_TYPE)awr->awar(AWAR_SEER_UPLOAD_WHAT)->read_int();
    SEER_SAVE_TYPE       save_type          = (SEER_SAVE_TYPE)awr->awar(AWAR_SEER_UPLOAD_TYPE)->read_int();
    char                *alignment_name     = awr->awar(AWAR_SEER_ALIGNMENT_NAME)->read_string();
    char                *arb_alignment_name = GBS_global_string_copy("ali_%s",alignment_name);

    aw_openstatus("Sequence Data");
    int max_i = 1;
    if (save_what == SSWT_ALL || SAI_flag) {
	gb_species = (SAI_flag) ? GBT_first_SAI(gb_main):GBT_first_species(gb_main);
	if (gb_species){
	    max_i = GB_number_of_subentries(GB_get_father(gb_species));
	}
    }else{
	gb_species = GBT_first_marked_species(gb_main);
	if (gb_species){
	    max_i = GB_number_of_marked_subentries(GB_get_father(gb_species));
	}
    }
    int i = 0;
    for (;gb_species;gb_species = gb_next_species){
	if (save_what == SSWT_ALL || SAI_flag){
	    gb_next_species = (SAI_flag)? GBT_next_SAI(gb_species) : GBT_next_species(gb_species);
	}else{
	    gb_next_species = GBT_next_marked_species(gb_species);
	}
	if (aw_status(double(i++)/double(max_i))) break; // user break requested
        GBDATA *gb_key = GB_entry(gb_species, SEER_KEY_FLAG);
	int transaction_limit;
	if (!gb_key){		// source of data is not SEER
	    if (save_type == SST_SINCE_LOGIN) continue;
	    transaction_limit = seer_global.save_transaction_counter_limit;
	}else{
	    transaction_limit = GB_read_int(gb_key);
	}
	if (GB_read_clock(gb_species) <= transaction_limit) continue; // whole species not changed

        GBDATA *gb_name = GB_entry(gb_species,"name");
	if (!gb_name) continue;	// name not set
	SeerInterfaceSequenceData *sd = new SeerInterfaceSequenceData(GB_read_char_pntr(gb_name));
	GB_ERROR error = seer_upload_one_species(sd,gb_species,transaction_limit,arb_alignment_name,alignment_name,sequence_flag,SAI_flag);
	if (error) {		// break on error
	    aw_message(error);
	    break;
	}
    }
    aw_closestatus();
    delete alignment_name;
    delete arb_alignment_name;
}

/**************************** UPLOAD ATTRIBUTES *********************************/
long seer_attributes_to_seer(const char *,long val){
    SeerInterfaceAttribute *at = (SeerInterfaceAttribute *)val;
    if (at->element_for_upload){
	at->print();
	SeerInterfaceError *error = seer_global.interface->checkAttributeName(at,SQIM_TEMPORARY);
	if (error){
	    if (aw_message(error->errorString,"Cancel Attribute, Force Upload") == 0){ // cancel
		at->element_for_upload = 0;
	    }else{
		delete error;
		error = seer_global.interface->storeAttributeName(at,SQIM_DOUBLE_CHECKED);
		if (error){
		    aw_message(error->errorString);
		    at->element_for_upload = 0;
		}
	    }
	    delete error;
	}else{
	    error = seer_global.interface->storeAttributeName(at,SQIM_DOUBLE_CHECKED);
	    if (error) aw_message(error->errorString);
	}
    }
    return val;
}


long seer_reset_arb_attribute_element_for_upload_flag(const char *,long val){
    SeerInterfaceAttribute *at = (SeerInterfaceAttribute *)val;
    at->element_for_upload = 0;
    return val;
}
void SEER_export_data_cb(AW_window *aws){
    AW_root *awr = aws->get_root();
    GB_begin_transaction(gb_main);
    GB_ERROR error = 0;
    seer_global.interface->beginTransaction();
    {				// set diff mode
	SEER_SAVE_TYPE save_type = (SEER_SAVE_TYPE)awr->awar(AWAR_SEER_UPLOAD_TYPE)->read_int();
	switch(save_type){
	case SST_SINCE_LOGIN:
	    seer_global.save_transaction_counter_limit = seer_global.transaction_counter_at_login;
	    seer_global.save_external_data_too = GB_FALSE;
	    break;
	case SST_QUICK_FILE:
	    seer_global.save_transaction_counter_limit = 0;
	    seer_global.save_external_data_too = GB_TRUE;
	    break;
 	case SST_ALL:
	    seer_global.save_transaction_counter_limit = -1;
	    seer_global.save_external_data_too = GB_TRUE;
	    break;
	}
    }
    {		// save attribute list
	const char *name;
	GBS_hash_do_loop(seer_global.arb_attribute_hash, seer_reset_arb_attribute_element_for_upload_flag);
	for ( name = seer_global.export_attribute_sellection_list->first_selected();
	      name;
	      name = seer_global.export_attribute_sellection_list->next_element()){
	    SeerInterfaceAttribute *at;
	    at = (SeerInterfaceAttribute*)GBS_read_hash(seer_global.arb_attribute_hash,name);
	    if (!at){
		AW_ERROR("Attribut '%s' in selection list but non in attribut hash",name);
		continue;
	    }
	    at->element_for_upload = 1;
	}
	GBS_hash_do_sorted_loop(seer_global.arb_attribute_hash,seer_attributes_to_seer,seer_sort_attributes);
    }
    {
	int save_sequences = awr->awar(AWAR_SEER_SAVE_SEQUENCES)->read_int();
	seer_upload_all_species(awr,save_sequences,0);	// sequences
	seer_upload_all_species(awr,save_sequences,1);	// SAI
    }
    SeerInterfaceError *ie = seer_global.interface->readFromArbDirectly(gb_main);
    if (ie){
	aw_message(ie->errorString);
	delete ie;
    }else{
	seer_global.seer_upload_window->hide();
    }
    ie = seer_global.interface->commitTransaction();
    if (ie){
	aw_message(ie->errorString);
	delete ie;
    }
    GB_commit_transaction(gb_main);
}

void seer_select_all_export_attributes(AW_window *,AW_CL selectFlag){
    if (selectFlag){
	seer_global.export_attribute_sellection_list->selectAll();
    }else{
	seer_global.export_attribute_sellection_list->deselectAll();
    }
}

long seer_free_attribute_pointers(const char *,long val){
    SeerInterfaceAttribute *at = (SeerInterfaceAttribute *)val;
    delete at;
    return 0;
}

long seer_insert_arb_attribute_in_selection(const char *,long val){
    SeerInterfaceAttribute *at = (SeerInterfaceAttribute *)val;
    if (at->element_for_upload){
	seer_global.seer_upload_window->insert_selection(seer_global.export_attribute_sellection_list,at->name,at->name);
    }
    return val;
}

void seer_check_upload_attribute_list(AW_window *aws){
    SeerInterfaceAttribute *at;
    GB_begin_transaction(gb_main);
    {				//  ******* set up arb attribute hash **************
	if (seer_global.arb_attribute_hash){
	    GBS_hash_do_loop(seer_global.arb_attribute_hash,seer_free_attribute_pointers);
	    GBS_free_hash(seer_global.arb_attribute_hash);
	}
	seer_global.arb_attribute_hash = GBS_create_hash(256, GB_MIND_CASE);
	GBDATA *gb_key;
	GBDATA *key_name;
	GB_ERROR error = 0;
	GBDATA *gb_key_data;
	gb_key_data = GB_search(gb_main,CHANGE_KEY_PATH,GB_CREATE_CONTAINER);

	int i = 0;
        for (gb_key = GB_entry(gb_key_data,CHANGEKEY); gb_key; gb_key = GB_nextEntry(gb_key)) {
	    GBDATA *gb_type = GB_search(gb_key,CHANGEKEY_TYPE,GB_INT);
	    if (GB_read_int(gb_type) != GB_STRING) continue;
	    at = new SeerInterfaceAttribute();

	    key_name = GB_search(gb_key,CHANGEKEY_NAME,GB_STRING);
	    at->name = GB_read_string(key_name);

	    at->pubCmnt = 0;
	    at->type = SIAT_STRING;
	    at->sortindex = i++;
	    at->element_for_upload = 1;
	    GBS_write_hash(seer_global.arb_attribute_hash, at->name, (long)at);
	}

	// Check against the database !!!!!!!!!!!!!!!!!!!!!!!!!!!
	//
	//

    }
    {	// put arb_attribute_hash to selection list
	aws->clear_selection_list(seer_global.export_attribute_sellection_list);
	GBS_hash_do_sorted_loop(seer_global.arb_attribute_hash,seer_insert_arb_attribute_in_selection,seer_sort_attributes);
	aws->update_selection_list(seer_global.export_attribute_sellection_list);
    }
    {;				// check sai updata
    }
    {	;			// check for table updates
    }
    GB_commit_transaction(gb_main);
}


void  SEER_upload_to_seer_create_window(AW_window *aww){
    AW_window_simple *aws = (AW_window_simple *)seer_global.seer_upload_window;
    AW_root *aw_root = aww->get_root();
    if (!aws){
	aw_root->awar_int(AWAR_SEER_UPLOAD_WHAT,SSWT_ALL);
	aw_root->awar_int(AWAR_SEER_UPLOAD_TYPE,SST_SINCE_LOGIN);
	aw_root->awar_int(AWAR_SEER_SAVE_SEQUENCES,1);
	aw_root->awar_int(AWAR_SEER_SAVE_SAI,1);

	seer_global.seer_upload_window = aws = new AW_window_simple;
	aws->init( aw_root, "UPLOAD_ARB_2_SEER", "SEER UPLOAD_DATA", 400, 100 );
	aws->load_xfig("seer/upload.fig");


	aws->callback( AW_POPDOWN);
	aws->at("close");
	aws->create_button("CANCEL","CANCEL","C");

	aws->at("help");
	aws->callback(AW_POPUP_HELP,(AW_CL)"seer/select_upload.hlp");
	aws->create_button("HELP","HELP","H");

	aws->button_length(0);
	aws->at("logo");
	aws->create_button(0,"#seer/seer.bitmap");


	aws->at("save_what");
	aws->create_toggle_field(AWAR_SEER_UPLOAD_WHAT,1);{
	    aws->insert_toggle("All","A",SSWT_ALL);
	    aws->insert_toggle("Marked","M",SSWT_MARKED);
	} aws->update_toggle_field();

	aws->at("save_diff");
	aws->create_toggle_field(AWAR_SEER_UPLOAD_TYPE,0);{
	    aws->insert_toggle("Jesus Christ's birth","A",SST_ALL);
	    aws->insert_toggle("Status of last full arb file","M",SST_QUICK_FILE);
	    aws->insert_toggle("Login","M",SST_SINCE_LOGIN);
	} aws->update_toggle_field();

	/* ************ Read the table list *************/
	{
	    char at_buffer[256];
	    char awar_buffer[256];
	    SeerInterfaceTableDescription *td;
	    int table_nr = 0;
	    const char *key;
	    for (GBS_hash_first_element(seer_global.table_hash,&key,(long *)&td);
		 td;
		 GBS_hash_next_element(seer_global.table_hash,&key,(long *)&td)){
		table_nr = td->sort_key;
		sprintf(at_buffer,"table_%i",table_nr);
		sprintf(awar_buffer,"%s/%s",AWAR_SEER_UPLOAD_TABLE_PREFIX,td->tablename);

		aws->at(at_buffer);
		aw_root->awar_int(awar_buffer,0);
		aws->create_toggle(awar_buffer);
	    }
	    /* ************ init attribute list *************/
	    {
		aws->at("attributes");
		AW_selection_list *sellst = aws->create_multi_selection_list();
		seer_global.export_attribute_sellection_list = sellst;
		//aws->update_selection_list(sellst);  // later done by seer_check_upload_attribute_list
	    }
	    aws->at("select");
	    aws->callback(seer_select_all_export_attributes,1);
	    aws->create_button("INVERT_ALL_ATTRIBUTES","Invert All");

	    aws->at("deselect");
	    aws->callback(seer_select_all_export_attributes,0);
	    aws->create_button("DESELECT_ALL_ATTRIBUTES","Deselect All");

	    aws->at("auto");
	    aws->callback(seer_check_upload_attribute_list);
	    aws->create_button("AUTO_SELECT_ATTRIBUTES","Auto Select");

	    aws->at("sequence_save");
	    aws->create_toggle(AWAR_SEER_SAVE_SEQUENCES);

	    aws->at("SAI_save");
	    aws->create_toggle(AWAR_SEER_SAVE_SAI);

	    aws->at("go");
	    aws->button_length(15);
	    aws->callback(SEER_export_data_cb);
	    aws->create_button("TO_SEER","UPLOAD");
	}
	seer_global.seer_upload_window = aws;
    }
    seer_check_upload_attribute_list(aws);
    aws->show();

}
