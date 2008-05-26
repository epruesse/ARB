#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>

#include <seer.hxx>
#include <seer_interface.hxx>

extern GBDATA *GLOBAL_gb_main;
SeerInterfaceCreator *seerInterfaceCreator = 0;

char *sequences_to_load_every_time[] = {
    "HELIX",
    "HELIX_NR",
    "ECOLI",
    "GLOBAL_GAPS",
    0
};


const char *SEER_open(const char *username, const char *userpasswd){
// returns errorstring or 0
    SeerInterface *si;
    if (seerInterfaceCreator){
	si = seerInterfaceCreator();
    }else{
	si = new SeerInterface(); // dummy implementation
    }
    
    seer_global.interface = si;
    SeerInterfaceError *si_error = si->beginSession(username,userpasswd);
    if (si_error){
	return si_error->errorString;
    }
    seer_global.alignment_hash = GBS_create_hash(256, GB_MIND_CASE);
    seer_global.attribute_hash = GBS_create_hash(256, GB_MIND_CASE);
    seer_global.table_hash = GBS_create_hash(256, GB_MIND_CASE);
    
    return 0;
}

void SEER_logout(){
    if (seer_global.interface){
	seer_global.interface->endSession();
    }
    exit(0);
}

int SEER_retrieve_data_rek(GB_HASH *attribute_hash,GBDATA *gb_species, SeerInterfaceData *id,int deep, int test_only, int filterAttributes){
    switch(id->type()){
    case SIDT_STRING:{
	SeerInterfaceAttribute *at = (SeerInterfaceAttribute *)GBS_read_hash(attribute_hash,id->key);
	if (filterAttributes && !at) return 0;	// probably masked out
	if (test_only) return 1;
	GBDATA *gb_elem = GB_create(gb_species,id->key,GB_STRING);
	GB_write_string(gb_elem,id->toDataString()->value);
	if (at && !at->changeable){
	    GB_write_security_levels(gb_elem,0,7,7);
	}
	break;
    }
    case SIDT_LINK:{
	SeerInterfaceAttribute *at = (SeerInterfaceAttribute *)GBS_read_hash(attribute_hash,id->key);
	if (!at) return 0;	// probably masked out
	if (!at->requested)return 0; // nobody wants this attribute
	if (test_only) return 1;
	GBDATA *gb_elem = GB_create(gb_species,id->key,GB_LINK);
	SeerInterfaceDataLink *datalink = id->toDataLink();
	char buffer[4096];
	sprintf(buffer,"T:%s:%s",datalink->linktoTableName,datalink->linktoTableId);
	GB_write_link(gb_elem,buffer);
	if (!at->changeable){
	    GB_write_security_levels(gb_elem,0,7,7);
	}
	break;
    }
    case SIDT_SET:{
	SeerInterfaceDataSet *set = id->toDataSet();
	SeerInterfaceData *el;
	int result =0;
	for (el = set->firstData(); // test weather there are subentries to write
	     el;
	     el = set->nextData()){
	    result += SEER_retrieve_data_rek(attribute_hash,0,el,deep+1,1,filterAttributes);
	}
	if (test_only) return result;
	
	if (result>0){
	    GBDATA *gb_cont = gb_species;
	    if (deep>0) gb_cont = GB_create_container(gb_species,id->key);
	    for (el = set->firstData();
		 el;
		 el = set->nextData()){
		SEER_retrieve_data_rek(attribute_hash,gb_cont,el,deep+1,0,filterAttributes);
	    }
	}	
	break;
    }
    }
    return 0;
}

void seer_retrieve_sequences(GBDATA *gb_species,const char *arb_alignment_name,SeerInterfaceDataStringSet *sequences){
    SeerInterfaceDataString *seq = sequences->firstData();
    if (!seq) return;
    GBDATA *gb_ali = GB_create_container(gb_species,arb_alignment_name);
    GBDATA *gb_data = GB_create(gb_ali,"data",GB_STRING);
    GB_write_string(gb_data,seq->value);
    while ((seq = sequences->nextData())){
	gb_data = GB_create(gb_ali,seq->key,GB_STRING);
	if (!gb_data){
	    GB_warning("%s",GB_get_error());
	    continue;
	}
	GB_write_string(gb_data,seq->value);
    }
}


void seer_put_species_into_arb(const char *arb_alignment_name, const char */*alignmentname*/, SeerInterfaceSequenceData *sd,int load_sequence_flag){
    GBDATA *gb_species = GBT_create_species(gb_main,sd->uniqueID);
    if (GB_entry(gb_species,SEER_KEY_FLAG)){
	GB_warning("Species '%s' not loaded, 'cause already in the database",sd->uniqueID);
	return;
    }
    GBDATA *gbd = GB_create(gb_species,SEER_KEY_FLAG,GB_INT); // mark species as created
    GB_write_int(gbd,GB_read_clock(gbd)); // save creation date
    GB_set_temporary(gbd);		// don't save this flag

    SEER_retrieve_data_rek(seer_global.attribute_hash,gb_species,&sd->attributes,0,0,1);
    if (load_sequence_flag){
	seer_retrieve_sequences(gb_species,arb_alignment_name,&sd->sequences);
    }
}

void seer_put_SAI_into_arb(const char *arb_alignment_name, const char *alignmentname, SeerInterfaceSequenceData *sd,int /*load_sequence_flag*/){
    GBDATA *gb_species = GBT_create_SAI(gb_main,sd->uniqueID);
    int ns = GB_number_of_subentries(gb_species);
    if (ns >1){		// old species
	GB_warning("Species '%s' not loaded, 'cause already in the database",sd->uniqueID);
	return;
    }

    GBDATA *gbd = GB_create(gb_species,SEER_KEY_FLAG,GB_INT); // mark species as created
    GB_write_int(gbd,GB_read_clock(gbd)); // save creation date

    SEER_retrieve_data_rek(seer_global.attribute_hash,gb_species,&sd->attributes,0,0,0);
    if (alignmentname){
	seer_retrieve_sequences(gb_species,arb_alignment_name,&sd->sequences);
    }
}

void seer_put_into_arb(const char *arb_alignment_name, const char *alignmentname, SeerInterfaceSequenceData *sd,int load_sequence_flag){
    if (sd->sequenceType == SISDT_REAL_SEQUENCE){
	seer_put_species_into_arb(arb_alignment_name,alignmentname,sd,load_sequence_flag);
    }else{
	seer_put_SAI_into_arb(arb_alignment_name,alignmentname,sd,load_sequence_flag);
    }
}

void seer_load_single_sequence_data(const char *arb_alignment_name, const char *alignmentname, const char *name, int load_sequence_flag){
    GBDATA *gb_sai = GBT_find_SAI(gb_main,name);
    if (gb_sai) return;		// already loaded
    SeerInterfaceSequenceData *sd = seer_global.interface->querySingleSequence(alignmentname,name);
    if (!sd){
	GB_warning("Cannot find '%s' in the OS database",name);
	return;
    }
    seer_put_into_arb(arb_alignment_name,alignmentname,sd,load_sequence_flag);
    delete sd;
}

void SEER_load_marked(const char *alignmentname,int load_sequence_flag){
    GB_ERROR error = 0;
    GB_begin_transaction(gb_main);
    seer_global.interface->beginReadOnlyTransaction();

    char arb_alignment_name[256];
    if (alignmentname){
	sprintf(arb_alignment_name,"ali_%s",alignmentname);
    }
    aw_openstatus("Query Marked Species");
    int max_i = GBT_count_marked_species(gb_main);
    int i = 0;
    GBDATA *gb_species;
    for (gb_species = GBT_first_marked_species(gb_main);
	 gb_species;
	 gb_species = GBT_next_marked_species(gb_species)){
        if (GB_entry(gb_species,SEER_KEY_FLAG)) continue; // already loaded
        GBDATA *gb_name = GB_entry(gb_species,"name");
	if (!gb_name) continue;
	char *name = GB_read_string(gb_name);
	seer_load_single_sequence_data(arb_alignment_name,alignmentname,name,load_sequence_flag);
	delete name;
	if (aw_status(double(i)/double(max_i))) break;
	i++;
    }
    aw_closestatus();
    seer_global.interface->commitReadOnlyTransaction();
    if (error){
	GB_warning(error);
	GB_abort_transaction(gb_main);
    }else{
	GBT_check_data(gb_main,0);
	GB_commit_transaction(gb_main);
    }
}

void SEER_create_skeleton_for_tree(GBT_TREE *tree){
    if (tree->is_leaf){
	if (!tree->gb_node){
	    if (!tree->name) return;
	    GBT_create_species(gb_main,tree->name);
	}
	return;
    }
    SEER_create_skeleton_for_tree(tree->leftson);
    SEER_create_skeleton_for_tree(tree->rightson);
}


const char *SEER_query(const char *alignmentname, const char *attribute,const char *querystring,int load_sequence_flag){
    GB_ERROR error = 0;
    GB_begin_transaction(gb_main);
    seer_global.interface->beginReadOnlyTransaction();
    seer_global.interface->queryDatabase(alignmentname,attribute,querystring);

    SeerInterfaceSequenceData *sd;
    char arb_alignment_name[256];
    if (alignmentname){
	sprintf(arb_alignment_name,"ali_%s",alignmentname);
    }
    
    // filter attributes missing
    // get global SAI sequences
    if (load_sequence_flag){
	char **sq;
	for ( sq = &sequences_to_load_every_time[0];
	      *sq;
	      sq++){
	    seer_load_single_sequence_data(arb_alignment_name,alignmentname,*sq,load_sequence_flag);
	}
    }
    
    // get the sequences
    {
	int i=0;
	int max_i = seer_global.interface->numberofMatchingSequences();
	for (sd = seer_global.interface->firstSequenceData();
	     sd;
	     sd = seer_global.interface->nextSequenceData()){
	/********************** Search species in the database and check weather it's really empty ******/
	seer_put_species_into_arb(arb_alignment_name,alignmentname, sd,load_sequence_flag);
	delete sd;
	if (GB_status(double(i)/double(max_i))) break;
	i++;
	}
    }
    seer_global.interface->commitReadOnlyTransaction();
    if (error){
	GB_warning(error);
	GB_abort_transaction(gb_main);
    }else{
	GBT_check_data(gb_main,0);
	GB_commit_transaction(gb_main);
    }
    return error;
}


