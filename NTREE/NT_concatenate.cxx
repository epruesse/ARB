/*=======================================================================================*/
/*                                                                                       */
/*    File       : NT_concatenate.cxx                                                    */
/*    Purpose    : 1.Concatenatenation of sequences or alignments                        */
/*                 2.Merging the fields of similar species and creating a new species    */
/*    Time-stamp : Thu Nov 21 2002                                                       */
/*    Author     : Yadhu Kumar (yadhu@mikro.biologie.tu-muenchen.de)                     */
/*    web site   : http://www.arb-home.de/                                               */
/*                                                                                       */
/*        Copyright Department of Microbiology (Technical University Munich)             */
/*                                                                                       */
/*=======================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <memory.h>
#include <iostream.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include <awtc_rename.hxx>
#include <awt_tree.hxx>
#include "awtlocal.hxx"
#include <aw_question.hxx>
#include "nt_concatenate.hxx"
#include <nt_sort.hxx>   // for sorting database entries

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define nt_assert(bed) arb_assert(bed)


using namespace std;

extern GBDATA *gb_main;
static AW_selection_list *con_alignment_list;
static AW_selection_list *db_alignment_list;
static AW_repeated_question *ask_about_missing_alignment;
static int mergeSimilarConcatenateAlignments = 0;

/*--------------------------creating and initializing AWARS----------------------------------------*/
void createConcatenationAwars(AW_root *aw_root, AW_default aw_def){
    aw_root->awar_string( AWAR_CON_SEQUENCE_TYPE,      "ami" ,                        aw_def);
    aw_root->awar_string( AWAR_CON_NEW_ALIGNMENT_NAME, "ali_concatenated_alignment" , aw_def);
    aw_root->awar_string( AWAR_CON_ALIGNMENT_SEPARATOR,"XXX",                         aw_def);
    aw_root->awar_string( AWAR_CON_DB_ALIGNS,          "" ,                           aw_def);
    aw_root->awar_string( AWAR_CON_CONCAT_ALIGNS,      "" ,                           aw_def);
    aw_root->awar_string( AWAR_CON_DUMMY,              "" ,                           aw_def);
    aw_root->awar_string( AWAR_CON_MERGE_FIELD,        "full_name" ,                  aw_def);
    aw_root->awar_string( AWAR_CON_STORE_SIM_SP_NO,    "merged_species" ,             aw_def);
}

/*------------------------Selecting alignments from the database for concatenation----------------------*/
void createSelectionList_callBack(struct conAlignStruct *cas){
	GBDATA *gb_alignment;
	GBDATA *gb_alignment_name;
	GBDATA *gb_alignment_type;
	char   *alignment_name;
	char   *alignment_type;

    GB_push_transaction(gb_main);  //opening a transaction if not opened yet

    AW_root *aw_root = cas->aws->get_root();
    char *ali_type   = aw_root->awar(AWAR_CON_SEQUENCE_TYPE)->read_string();
    ali_type         = GBS_global_string_copy("%s=", ali_type);

	cas->aws->clear_selection_list(cas->db_id); //clearing the selection list

	for (gb_alignment = GB_search(gb_main,"presets/alignment",GB_FIND);
         gb_alignment;
         gb_alignment = GB_find(gb_alignment,"alignment",0,this_level|search_next))
        {
            gb_alignment_type = GB_find(gb_alignment,"alignment_type",0,down_level);
            gb_alignment_name = GB_find(gb_alignment,"alignment_name",0,down_level);
            alignment_type    = GB_read_string(gb_alignment_type);
            alignment_name    = GB_read_string(gb_alignment_name);

            char *str = GBS_string_eval(alignment_type, ali_type, 0);   //selecting the alignments of the selected sequence type
            if (!*str){
                cas->aws->insert_selection( cas->db_id, alignment_name, alignment_name );   //inserting to the selection list
            }
            free(str);
            free(alignment_type);
            free(alignment_name);
        }
	cas->aws->insert_default_selection( cas->db_id, "????", "????" );
	cas->aws->update_selection_list( cas->db_id );

    GB_pop_transaction(gb_main); //closing a transaction
    free(ali_type);
}

static void createSelectionList_callBack_gb(GBDATA*,int *cl_cas, GB_CB_TYPE cbtype) {
    if (cbtype == GB_CB_CHANGED) {
        struct conAlignStruct *cas = (struct conAlignStruct*)cl_cas;
        createSelectionList_callBack(cas);
    }
}

static void createSelectionList_callBack_awar(AW_root *aw_root, AW_CL cl_cas) {
    struct conAlignStruct *cas = (struct conAlignStruct*)cl_cas;
    gb_assert(aw_root==cas->aws->get_root());
    createSelectionList_callBack(cas);
    //clear the selected alignments and set default to ????
	cas->aws->clear_selection_list(con_alignment_list);
	cas->aws->insert_default_selection(con_alignment_list,"????", "????" );
    cas->aws->update_selection_list(con_alignment_list);
}


conAlignStruct* createSelectionList(GBDATA *gb_main,AW_window *aws, const char *awarName){

#ifdef DEBUG
    static bool ran=false;
    gb_assert(!ran);
    ran=true;                 //prevents calling this function for the second time
#endif

	GBDATA *gb_presets;
    AW_root *aw_root  = aws->get_root();
    char    *ali_type = aw_root->awar(AWAR_CON_SEQUENCE_TYPE)->read_string();  //reading sequence type from the concatenation window
    ali_type          = GBS_global_string_copy("%s=", ali_type);            // copying the awar with '=' appended to it

	db_alignment_list = aws->create_selection_list(awarName,0,"",10,20);

	conAlignStruct *cas = 0;
	cas          = new conAlignStruct; // don't free (used in callback)
	cas->aws     = aws;
	cas->gb_main = gb_main;
	cas->db_id   = db_alignment_list;
	cas->seqType  = 0; if (ali_type) cas->seqType = strdup(ali_type);  //assigning the sequence type to struct cas

	createSelectionList_callBack(cas); // calling callback to get the alingments in the database

	GB_push_transaction(gb_main);
	gb_presets = GB_search(gb_main,"presets",GB_CREATE_CONTAINER);
	GB_add_callback(gb_presets, GB_CB_CHANGED, createSelectionList_callBack_gb, (int *)cas);
	GB_pop_transaction(gb_main);

    free(ali_type);
    return cas;
}

// -------------------------------------
//      #if 0 /* yadhus version : */
// -------------------------------------
#if 0 /* yadhus version : */

void selectAlignment(AW_window *aws){
    AW_root *aw_root = aws->get_root();

    char *selected_alignment = aw_root->awar(AWAR_CON_DB_ALIGNS)->read_string();
    if (!selected_alignment || !selected_alignment[0] || selected_alignment[0] == '?') return;
    // NOTE 4 Yadhu :
    // return above is really bad, because
    // free(selected_alignment) below is not reached if selected_alignment is i.e. "?????"
    // the same problem occurs with the returns below.

    int listCount  = aws->get_no_of_entries(con_alignment_list);
    int left_index = aws->get_index_of_element(db_alignment_list, selected_alignment);

    //browse thru the entries and insert the selection if not in the selected list
    if(listCount>1){
        const char *listEntry = con_alignment_list->first_element(); if(!listEntry || !listEntry[0]) return;
        for(int i = 1; i<listCount; i++){
            if (GBS_strscmp(listEntry,selected_alignment)!= 0){
                listEntry = con_alignment_list->next_element();
            }
            else return;
        }
        aws->insert_selection(con_alignment_list, selected_alignment, selected_alignment);
    }
    else {
        aws->insert_selection(con_alignment_list, selected_alignment, selected_alignment);
    }
    aws->update_selection_list(con_alignment_list);

    aws->select_index(db_alignment_list, AWAR_CON_DB_ALIGNS, left_index+1); // go down 1 position on left side
    aw_root->awar(AWAR_CON_CONCAT_ALIGNS)->write_string(selected_alignment); // position right side to newly added alignment

    free(selected_alignment);
}

#endif

// MARKER4YADHU_2
//
// fixed version :
void selectAlignment(AW_window *aws){
    AW_root *aw_root            = aws->get_root();
    char    *selected_alignment = aw_root->awar(AWAR_CON_DB_ALIGNS)->read_string();

    if (selected_alignment && selected_alignment[0] != 0 && selected_alignment[0] != '?') {
        int  left_index  = aws->get_index_of_element(db_alignment_list, selected_alignment);
        bool entry_found = false;

        // browse thru the entries and insert the selection if not in the selected list
        for (const char *listEntry = con_alignment_list->first_element();
             !entry_found && listEntry;
             listEntry = con_alignment_list->next_element())
        {
            if (strcmp(listEntry, selected_alignment) == 0) {
                entry_found = true; // entry does already exist in right list
            }
        }

        if (!entry_found) { // if not yet in right list
            aws->insert_selection(con_alignment_list, selected_alignment, selected_alignment);
            aws->update_selection_list(con_alignment_list);
        }

        aws->select_index(db_alignment_list, AWAR_CON_DB_ALIGNS, left_index+1); // go down 1 position on left side
        aw_root->awar(AWAR_CON_CONCAT_ALIGNS)->write_string(selected_alignment); // position right side to newly added or already existing alignment
    }
    free(selected_alignment);
}


void selectAllAlignments(AW_window *aws){
    const char *listEntry = db_alignment_list->first_element();
    aws->clear_selection_list(con_alignment_list);

    while (listEntry) {
        aws->insert_selection(con_alignment_list, listEntry, listEntry);
        listEntry = db_alignment_list->next_element();
    }
	aws->insert_default_selection(con_alignment_list,"????", "????" );
    aws->update_selection_list(con_alignment_list);
}

void clearAlignmentList(AW_window *aws){
    aws->clear_selection_list(con_alignment_list);
	aws->insert_default_selection(con_alignment_list,"????", "????" );
    aws->update_selection_list(con_alignment_list);
}

void removeAlignment(AW_window *aws){

    AW_root *aw_root = aws->get_root();

    char *selected_alignment = aw_root->awar(AWAR_CON_CONCAT_ALIGNS)->read_string();
    if (!selected_alignment || !selected_alignment[0]) return; // @@@ FIXME:  selected_alignment is not free'd in some case

    int index = aws->get_index_of_element(con_alignment_list, selected_alignment); // save old position

    aws->delete_selection_from_list(con_alignment_list, selected_alignment);
	aws->insert_default_selection(con_alignment_list,"????", "????" );
    aws->update_selection_list(con_alignment_list);

    aws->select_index(con_alignment_list, AWAR_CON_CONCAT_ALIGNS, index); // restore old position
    aw_root->awar(AWAR_CON_DB_ALIGNS)->write_string(selected_alignment); // set left seletion to deleted alignment

    free(selected_alignment);
}

void shiftAlignment(AW_window *aws, long int direction){

    AW_root *aw_root = aws->get_root();
    AW_selection_list *temp_list = aws->create_selection_list(AWAR_CON_DUMMY, "","");

    char *selected_alignment = aw_root->awar(AWAR_CON_CONCAT_ALIGNS)->read_string();
    if (!selected_alignment || !selected_alignment[0] || selected_alignment == NULL) return; // @@@ FIXME:  selected_alignment is not free'd in some case

    int curr_index        = 0;
    int sel_element_index = aws->get_index_of_element(con_alignment_list, selected_alignment);
    const char *listEntry = con_alignment_list->first_element();
    const char *temp_listEntry = 0;

    aws->clear_selection_list(temp_list);

    while(listEntry){
        switch(direction) {
        case MOVE_UP:       //shifting alignments upwards
            if (sel_element_index == curr_index+1){
                aws->insert_selection(temp_list, selected_alignment, selected_alignment);
                temp_listEntry = aws->get_element_of_index(con_alignment_list, curr_index++);
                if(temp_listEntry) aws->insert_selection(temp_list, temp_listEntry, temp_listEntry);
            }
            else {
                temp_listEntry = aws->get_element_of_index(con_alignment_list, curr_index);
                if(temp_listEntry) aws->insert_selection(temp_list, temp_listEntry, temp_listEntry);
            }
            curr_index++;
            break;

        case MOVE_DOWN:    //shifting alignments downwards
            if (sel_element_index == curr_index){
                temp_listEntry = aws->get_element_of_index(con_alignment_list, ++curr_index);
                if(temp_listEntry) aws->insert_selection(temp_list, temp_listEntry, temp_listEntry);
                aws->insert_selection(temp_list, selected_alignment, selected_alignment);
            }
            else {
                temp_listEntry = aws->get_element_of_index(con_alignment_list, curr_index);
                if(temp_listEntry) aws->insert_selection(temp_list, temp_listEntry, temp_listEntry);
            }
            curr_index++;
            break;
        }
        listEntry = con_alignment_list->next_element();
    }

    aws->insert_default_selection(temp_list,"????", "????" );
    aws->update_selection_list(temp_list);

    aws->clear_selection_list(con_alignment_list);
    listEntry = temp_list->first_element();

    while (listEntry) {
        aws->insert_selection(con_alignment_list, listEntry, listEntry);
        listEntry = temp_list->next_element();
    }
    aws->insert_default_selection(con_alignment_list,"????", "????" );
    aws->update_selection_list(con_alignment_list);
    free(selected_alignment);
}

/*---------------------------------------- Concatenation function ----------------------------------*/
void concatenateAlignments(AW_window *aws){

	GB_push_transaction(gb_main);
    AW_root  *aw_root = aws->get_root();

	GB_ERROR  error               = 0;
	GBDATA   *gb_species          = 0;
	GBDATA   *gb_presets          = 0;
    GBDATA   *gb_alignment_exists = 0;
    GBDATA   *gb_new_alignment    = 0;

    ask_about_missing_alignment = new  AW_repeated_question;

	char *new_ali_name        = aw_root->awar(AWAR_CON_NEW_ALIGNMENT_NAME)->read_string();
    char *seq_type            = aw_root->awar(AWAR_CON_SEQUENCE_TYPE)->read_string();
    char *alignment_separator = aw_root->awar(AWAR_CON_ALIGNMENT_SEPARATOR)->read_string();

    const char *ali_name;
	long new_alignment_len   = 0;
	int no_of_sel_alignments = aws->get_no_of_entries(con_alignment_list);

	for(int i = 1; i<no_of_sel_alignments; i++){  //computing the length for the new concatenated alignment
		ali_name           =  aws->get_element_of_index(con_alignment_list, i);
		new_alignment_len += GBT_get_alignment_len(gb_main, ali_name);
	}

	error = GBT_check_alignment_name(new_ali_name);
    if(error) goto ERROR;

	gb_presets          = GB_search(gb_main, "presets", GB_CREATE_CONTAINER);
	gb_alignment_exists = GB_find(gb_presets, "alignment_name", new_ali_name, down_2_level);

	if (gb_alignment_exists){    // check wheather new alignment exists or not, if yes prompt user to overwrite the existing alignment; if no create an empty alignment
		int ans = aw_message(GBS_global_string("Do you want to overwrite the existing data in alignment \" %s \"", new_ali_name), "YES,NO", false);
		if(ans){
			goto ERROR;
		}
		else {
			gb_new_alignment = GBT_get_alignment(gb_main,new_ali_name);
			if (!gb_new_alignment) error = GB_get_error();
		}
	}
	else {
		gb_new_alignment = GBT_create_alignment(gb_main,new_ali_name,new_alignment_len,0,0,seq_type);
		if (!gb_new_alignment) error = GB_get_error();
	}

    for (gb_species = GBT_first_marked_species(gb_main); gb_species; gb_species = GBT_next_marked_species(gb_species)){
		void *str_seq = GBS_stropen(new_alignment_len+1000);			/* create output stream */
        const char *concatenated_ali_seq_data;
        int found =0; int missing = 0; int ali_len = 0;
        ali_name = con_alignment_list->first_element();

        while(ali_name){             // concatenation of the selected alignments in the database
			GBDATA *gb_seq_data = GBT_read_sequence(gb_species, ali_name);
            if(gb_seq_data) {
				char *str_data = GB_read_char_pntr(gb_seq_data);
                GBS_strcat(str_seq,str_data);
                ++found;
			}
            else {
                gb_assert(ask_about_missing_alignment);
                char *question = GBS_global_string_copy("\"%s\" alignment doesn`t exist in \"%s\"!", ali_name, GB_read_string(GB_find(gb_species, "full_name", 0, down_level)));
                int skip_ali = ask_about_missing_alignment->get_answer(question, "Insert Gaps for Missing Alignment,Skip Missing Alignment", "all", true);
                if(!skip_ali){
                    ali_len = GBT_get_alignment_len(gb_main, ali_name);
                    for( int j = 0; j<ali_len; j++){  GBS_strcat(str_seq,"."); }
                }
                ++missing;
                free(question);
            }
            ali_name = con_alignment_list->next_element();
            if(ali_name) GBS_strcat(str_seq,alignment_separator);
		}
		concatenated_ali_seq_data = GBS_strclose(str_seq, 0);
		GBDATA *gb_data = GBT_add_data(gb_species, new_ali_name, "data", GB_STRING);
		GB_write_string(gb_data, concatenated_ali_seq_data);
        aw_message(GBS_global_string("%d alignment(s) were concatenated to \"%s\" < %d alignment(s) were missing!).",found,GB_read_string(GB_find(gb_species, "full_name", 0, down_level)),missing));
	}

    {
        GBDATA *gb_extended = GBT_create_SAI(gb_main,"Alignment Information");
        GBDATA *gb_data     = GBT_add_data(gb_extended, new_ali_name,"data", GB_STRING);
        void *ali_str       = GBS_stropen(GBT_get_alignment_len(gb_main,new_ali_name));
        ali_name            = con_alignment_list->first_element();

        while (ali_name){
            int alignment_len = GBT_get_alignment_len(gb_main,ali_name);
            int ali_str_len   = strlen(ali_name);
            for(int pos = 0; pos<alignment_len; pos++){
                if(pos<5)  GBS_strcat(ali_str,"<");
                else if(pos>=(alignment_len-5))  GBS_strcat(ali_str,">");
                else if(pos==(alignment_len/2 - ali_str_len/2)) { GBS_strcat(ali_str,ali_name); pos+=ali_str_len-1;}
                else  GBS_strcat(ali_str,"=");
            }
            ali_name = con_alignment_list->next_element();
            if(ali_name) GBS_strcat(ali_str,alignment_separator);
        }
        char *ali_info_SAI = GBS_strclose(ali_str, 0);
        error  = GB_write_string(gb_data,ali_info_SAI);
        free(ali_info_SAI);
    }

	if (!error){
		char *nfield = GB_strdup(GBS_global_string("%s/data",new_ali_name));
		awt_add_new_changekey( gb_main,nfield,GB_STRING);
		free(nfield);
	    GB_pop_transaction(gb_main);
	}else{
        ERROR :
            GB_abort_transaction(gb_main);
	}
    if(error) aw_message(error);


	free(new_ali_name);
    free(seq_type);
    free(alignment_separator);
}

static void addSpeciesToConcatenateList(void **speciesConcatenateListPtr,GB_CSTR species_name){

    GBDATA *gb_species_data = GB_search(gb_main, "species_data",  GB_CREATE_CONTAINER);
    GBDATA *gb_species      = GBT_find_species_rel_species_data(gb_species_data, species_name);

    if (gb_species) {
        speciesConcatenateList *sclp = (speciesConcatenateList*)speciesConcatenateListPtr;
        speciesConcatenateList scl   = new SPECIES_ConcatenateList;
        scl->species                 = gb_species;
        scl->species_name            = strdup(species_name);
        scl->next                    = *sclp;
        *sclp                        = scl;
    }
}

static void freeSpeciesConcatenateList(speciesConcatenateList scl){
    while (scl) {
        speciesConcatenateList next = scl->next;
        free(scl->species_name);
        delete scl;
        scl = next;
    }
}

GBDATA *concatenateFieldsCreateNewSpecies(AW_window *, GBDATA *gb_species, speciesConcatenateList scl){

    GB_ERROR error = 0;
    GB_push_transaction(gb_main);

    GBDATA *gb_species_data = GB_search(gb_main, "species_data",  GB_CREATE_CONTAINER);
    char *new_species_name = 0;

    GBDATA *gb_species_full_name = GB_find(gb_species, "full_name", 0, down_level);
    GB_CSTR str_full_name        = GB_read_string(gb_species_full_name);

    error = AWTC_generate_one_name(gb_main, str_full_name, 0, new_species_name, false);

    if (!error) {   // name was created
        if (GBT_find_species_rel_species_data(gb_species_data, new_species_name)!=0) { //if the name is not unique create unique name
            char *uniqueName = AWTC_makeUniqueShortName(new_species_name, gb_species_data);
            free(new_species_name);
            new_species_name = uniqueName;
            if (!new_species_name) error = "No short name created.";
        }
    }

    /*--------------------getting the species releated data --------------------*/
    GBDATA *gb_species_name   = GB_find(gb_species, "name", 0, down_level);
    char   *str_name          = GB_read_string(gb_species_name);
    GBDATA *gb_species_source = GBT_find_species_rel_species_data(gb_species_data, str_name);
    GBDATA *gb_new_species    = 0;
    free(str_name);

    if (!error) {
        gb_new_species = GB_create_container(gb_species_data, "species");
        error = GB_copy(gb_new_species, gb_species_source); // copy first found species to create a new species
    }
    if (!error) {
        GBDATA *gb_name = GB_search(gb_new_species, "name", GB_STRING);
        error = GB_write_string(gb_name, new_species_name); // insert new 'name'
    }
    if (!error) {
        GBDATA *gb_full_name = GB_search(gb_new_species, "full_name", GB_STRING);
        error = GB_write_string(gb_full_name, str_full_name); // insert new 'full_name'
    }

    long id = 0;
    char buffer[100];

    if(!error) {
        speciesConcatenateList speciesList = scl;
        char **ali_names = GBT_get_alignment_names(gb_main);
        for( ; speciesList; speciesList = speciesList->next){
            for(int no_of_alignments = 0; ali_names[no_of_alignments]!=0; no_of_alignments++){
                GBDATA *gb_seq_data = GBT_read_sequence(speciesList->species, ali_names[no_of_alignments]);
                if(gb_seq_data){
                    char *seq_data  = GB_read_char_pntr(gb_seq_data);
                    GBDATA *gb_data = GBT_add_data(gb_new_species, ali_names[no_of_alignments], "data", GB_STRING);
                    error = GB_write_string(gb_data, seq_data);
                    if(!error) id += GBS_checksum(seq_data,1,".-");  //creating checksum of the each aligned sequence to generate new accession number
                }
                if(error) error = GB_export_error("Can't create alignment '%s'", ali_names[no_of_alignments]);
            }
        }
        sprintf(buffer,"ARB_%lX",id);
        GBT_free_names(ali_names);
    }

    if (!error) {
        char *doneFields = strdup(";name;"); // all fields which are already merged
        int   doneLen    = strlen(doneFields);
        speciesConcatenateList sl = scl;
        int  sl_length = 0; while(scl) { sl_length++; scl=scl->next; } //counting no. of similar species stored in the list
        int *fieldStat = new int[sl_length]; // 0 = not used yet ; -1 = doesn't have field ; 1..n = field content (same number means same content)

        while (sl && !error) { // with all species do..
            char *newFields  = GB_get_subfields(sl->species);
            char *fieldStart = newFields; // points to ; before next field

            while (fieldStart[1] && !error) { // with all subfields of the species do..
                char *fieldEnd = strchr(fieldStart+1, ';');
                gb_assert(fieldEnd);
                char behind = fieldEnd[1]; fieldEnd[1] = 0;

                if (strstr(doneFields, fieldStart)==0) { // field is not merged yet
                    char *fieldName = fieldStart+1;
                    int   fieldLen  = int(fieldEnd-fieldName);

                    gb_assert(fieldEnd[0]==';');
                    fieldEnd[0] = 0;

                    GBDATA *gb_field = GB_search(sl->species, fieldName, GB_FIND);
                    gb_assert(gb_field); // field has to exist, cause it was found before
                    int type = gb_field->flags.type; //GB_TYPE(gb_field);

                    if (type==GB_STRING) { // we only merge string fields
                        int i; int doneSpecies = 0; int nextStat = 1;

                        for (i=0; i<sl_length; i++) { fieldStat[i] = 0;} // clear field status

                        while (doneSpecies<sl_length) { // since all species in list were handled
                            speciesConcatenateList sl2 = sl; i = 0;

                            while (sl2) {
                                if (fieldStat[i]==0) {
                                    gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                    if (gb_field) {
                                        char *content = GB_read_as_string(gb_field);
                                        speciesConcatenateList sl3 = sl2->next;
                                        fieldStat[i] = nextStat;
                                        int j = i+1; doneSpecies++;

                                        while (sl3) {
                                            if (fieldStat[j]==0) {
                                                gb_field = GB_search(sl3->species, fieldName, GB_FIND);
                                                if (gb_field) {
                                                    char *content2 = GB_read_as_string(gb_field);
                                                    if (strcmp(content, content2)==0) { // if contents are the same, they get the same status
                                                        fieldStat[j] = nextStat;
                                                        doneSpecies++;
                                                    }
                                                    free(content2);
                                                } else {
                                                    fieldStat[j] = -1;
                                                    doneSpecies++;
                                                }
                                            }
                                            sl3 = sl3->next; j++;
                                        }
                                        free(content); nextStat++;
                                    } else {
                                        fieldStat[i] = -1; // field does not exist here
                                        doneSpecies++;
                                    }
                                }
                                sl2 = sl2->next; i++;
                            }
                            if(!sl2) break;
                        }
                        gb_assert(nextStat!=1); // this would mean that none of the species contained the field
                        {
                            char *new_content     = 0;
                            int   new_content_len = 0;

                            if (nextStat==2) { // all species contain same field content or do not have the field
                                speciesConcatenateList sl2 = sl;
                                while (sl2) {
                                    gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                    if (gb_field) {
                                        new_content = GB_read_as_string(gb_field);
                                        new_content_len = strlen(new_content);
                                        break;
                                    }
                                    sl2 = sl2->next;
                                }
                            }
                            else { // different field contents
                                int actualStat;
                                for (actualStat=1; actualStat<nextStat; actualStat++) {
                                    int names_len = 1; // open bracket
                                    speciesConcatenateList sl2 = sl;
                                    char *content = 0; i = 0;

                                    while (sl2) {
                                        if (fieldStat[i]==actualStat) {
                                            names_len += strlen(sl2->species_name)+1;
                                            if (!content) {
                                                gb_field = GB_search(sl2->species, fieldName, GB_FIND);
                                                gb_assert(gb_field);
                                                content = GB_read_as_string(gb_field);
                                            }
                                        }
                                        sl2 = sl2->next; i++;
                                    }
                                    gb_assert(content);
                                    int add_len = names_len+1+strlen(content);
                                    char *whole = (char*)malloc(new_content_len+1+add_len+1);
                                    gb_assert(whole);
                                    char *add = new_content ? whole+sprintf(whole, "%s ", new_content) : whole;
                                    sl2 = sl; i = 0;
                                    int first = 1;
                                    while (sl2) {
                                        if (fieldStat[i]==actualStat) {
                                            add += sprintf(add, "%c%s", first ? '{' : ';', sl2->species_name);
                                            first = 0;
                                        }
                                        sl2 = sl2->next; i++;
                                    }
                                    add += sprintf(add, "} %s", content);

                                    free(content);
                                    free(new_content);
                                    new_content = whole;
                                    new_content_len = strlen(new_content);
                                }
                            }

                            if (new_content) {
                                GBDATA *gb_new_field = GB_search(gb_new_species, fieldName, GB_STRING);
                                if (gb_new_field) {
                                    error = GB_write_string(gb_new_field, new_content);
                                }
                                else {
                                    error = GB_export_error("Can't create field '%s'", fieldName);
                                }
                                free(new_content);
                            }
                        }
                    }

                    // mark field as done:
                    char *new_doneFields = (char*)malloc(doneLen+fieldLen+1+1);
                    sprintf(new_doneFields, "%s%s;", doneFields, fieldName);
                    doneLen += fieldLen+1;
                    free(doneFields);
                    doneFields = new_doneFields;
                    fieldEnd[0] = ';';
                }
                fieldEnd[1] = behind;
                fieldStart = fieldEnd;
            }
            free(newFields);
            sl = sl->next;
        }
        free(doneFields);
        delete [] fieldStat;
    }

    GB_write_flag(gb_new_species, 1);    //marking the new species created
    GBDATA *gb_acc = GB_search(gb_new_species,"acc",GB_STRING);
    GB_write_string(gb_acc,buffer); //inserting new accession number

    if (error)   GB_abort_transaction(gb_main);
    else         GB_pop_transaction(gb_main);

    if (error) aw_message(error, "OK");
    return gb_new_species;
}

void mergeSimilarSpecies(AW_window *aws){

    GB_push_transaction(gb_main);  //open database for transaction
    AW_root  *aw_root = aws->get_root();

    GB_ERROR  error             = 0;
	GBDATA *gb_species          = 0;
    GBDATA *new_species_created = 0;

    char *merge_field_name      = aw_root->awar(AWAR_CON_MERGE_FIELD)->read_string();
    int similarSpeciesFound     = 0;
    speciesConcatenateList scl  = 0; //to build list of similar species

    // MARKER4YADHU_1 :  NT_resort_data_base opens a transaction -> Crash -- please fix
	error = NT_resort_data_base(0,merge_field_name,0,0); // sorting the database entries

    char *new_field_name =aw_root->awar(AWAR_CON_STORE_SIM_SP_NO)->read_string();
    GB_ERROR    error2 = GB_check_key(new_field_name);
    if(!error2) error2 = awt_add_new_changekey(gb_main,new_field_name,GB_STRING);
    if(error2) {
        aw_message(GBS_global_string("\"%s\" field found! Please enter a new field name!",new_field_name));
        goto ERROR;
    }

    for (gb_species = GBT_first_marked_species(gb_main); gb_species; gb_species = GBT_next_marked_species(gb_species)){
		GBDATA  *gb_species_field         = GB_find(gb_species, merge_field_name, 0, down_level);
		if(!gb_species_field) {
            aw_message("Selected ENTRY doesnt contain any data!. Please select a VALID DATABASE FIELD ENTRY!");
            goto ERROR;
        }
		GB_CSTR  gb_species_field_content  = GB_read_string(gb_species_field);
        int similar_species = 0;

        for (GBDATA *gb_species_next =  GBT_next_marked_species(gb_species);
             gb_species_next;
             gb_species_next = GBT_next_marked_species(gb_species_next))
            {
                GBDATA *gb_next_species_field           = GB_find(gb_species_next, merge_field_name, 0, down_level);
                GB_CSTR gb_next_species_field_content   = GB_read_string(gb_next_species_field);
                if(GBS_strscmp(gb_species_field_content,gb_next_species_field_content)==0){
                    GBDATA *gb_species_name = GB_find(gb_species_next, "name", 0, down_level);
                    GB_CSTR species_name    = GB_read_string(gb_species_name);
                    addSpeciesToConcatenateList((void**)&scl,species_name);
                    GB_write_flag(gb_species_next, 0);
                    ++similar_species;
                    similarSpeciesFound = 1;
                }
            }

        if(similarSpeciesFound){
            GBDATA *gb_species_name = GB_find(gb_species, "name", 0, down_level);
            GB_CSTR species_name    = GB_read_string(gb_species_name);
            addSpeciesToConcatenateList((void**)&scl, species_name);
            GB_write_flag(gb_species, 0);
            new_species_created = concatenateFieldsCreateNewSpecies(aws,gb_species,scl);
            gb_assert(new_species_created);
            GBDATA *merged_species_field    = GB_search(new_species_created, new_field_name, GB_STRING);
            char *similar_species_no        = GB_strdup(GBS_global_string("%d species",++similar_species));
            if (merged_species_field) error = GB_write_string(merged_species_field, similar_species_no);
            similarSpeciesFound = 0;
            freeSpeciesConcatenateList(scl); scl = 0;
            free(similar_species_no);
        }
        similar_species = 0;
	}

    for (gb_species = GBT_first_marked_species(gb_main); gb_species; gb_species = GBT_next_marked_species(gb_species)){
		GBDATA  *gb_new_merged_species = GB_find(gb_species, new_field_name, 0, down_level);
        if(gb_new_merged_species)  GB_write_flag(gb_species, 1);
        else                       GB_write_flag(gb_species, 0);
    }

	aw_message(GBS_global_string("%i new species created by taking \"%s\" as a criterion!",GBT_count_marked_species(gb_main),merge_field_name));
    free(merge_field_name);
    free(new_field_name);

 ERROR:
	if (!error)  GB_pop_transaction(gb_main);
	else	     GB_abort_transaction(gb_main);

	if(error) aw_message(error);

    //Concatenating alignments of the merged species if the option is set to MERGE_SIMILAR_CONCATENATE_ALIGNMENTS
    if(mergeSimilarConcatenateAlignments && !error) concatenateAlignments(aws);
}

/*----------------------------Creating concatenation window-----------------------------------------*/
AW_window *NT_createConcatenationWindow(AW_root *aw_root, AW_default def){

    createConcatenationAwars(aw_root,def);
    AW_window_simple *aws = new AW_window_simple;

    aws->init( aw_root, "CONCATENATE_ALIGNMENTS", "CONCATENATION WINDOW", 100, 100 );
    aws->load_xfig("concatenate.fig");

    aws->callback( AW_POPUP_HELP,(AW_CL)"concatenate.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("dbAligns");
    conAlignStruct *cas = createSelectionList(gb_main, (AW_window *)aws, AWAR_CON_DB_ALIGNS);

    aws->at("concatAligns");
    con_alignment_list = aws->create_selection_list(AWAR_CON_CONCAT_ALIGNS, "concatAligns","C");
    aws->insert_default_selection(con_alignment_list, "????","????");
    aws->update_selection_list(con_alignment_list);

    aws->at("type");
    aws->create_option_menu(AWAR_CON_SEQUENCE_TYPE);
    aws->insert_option("DNA","d","dna");
    aws->insert_option("RNA","r","rna");
    aws->insert_default_option("PROTEIN","p","ami");
    aws->update_option_menu();
    aw_root->awar(AWAR_CON_SEQUENCE_TYPE)->add_callback(createSelectionList_callBack_awar, (AW_CL)cas); //associating selection list callback to sequence type awar

    aws->at("add");
    aws->button_length(6);
    aws->callback(selectAlignment);
    aws->create_button("ADD","#moveRight.bitmap");

    aws->at("addAll");
    aws->button_length(6);
    aws->callback(selectAllAlignments);
    aws->create_button("ADDALL","#moveAll.bitmap");

    aws->at("remove");
    aws->button_length(6);
    aws->callback(removeAlignment);
    aws->create_button("REMOVE","#moveLeft.bitmap");

    aws->at("up");
    aws->button_length(0);
    aws->callback(shiftAlignment, 1); //passing 1 as argument to shift the alignment upwards
    aws->create_button("up","#moveUp.bitmap",0);

    aws->at("down");
    aws->button_length(0);
    aws->callback(shiftAlignment, 0); //passing 0 as argument to shift the alignment downwards
    aws->create_button("down","#moveDown.bitmap",0);

    aws->at("clearList");
    aws->callback(clearAlignmentList);
    aws->create_button("CLEAR","CLEAR LIST","R");

    aws->at("aliName");
    aws->label_length(15);
    aws->create_input_field(AWAR_CON_NEW_ALIGNMENT_NAME,20);

    aws->at("aliSeparator");
    aws->label_length(5);
    aws->create_input_field(AWAR_CON_ALIGNMENT_SEPARATOR,5);

    aws->at("concatenate");
	aws->callback((AW_CB1)concatenateAlignments,0);
    aws->create_button("CONCATENATE","Create concatenated alignments","A");

    aws->at("merge_species");
	aws->callback(AW_POPUP, (AW_CL)NT_createMergeSimilarSpeciesWindow,0);
    aws->create_button("MERGE_SPECIES","Create new species by merging similar species","M");

    aws->at("merge_concatenate");
	aws->callback(AW_POPUP, (AW_CL)NT_createMergeSimilarSpeciesWindow,(int)MERGE_SIMILAR_CONCATENATE_ALIGNMENTS);
    aws->create_button("MERGE_CONCATENATE","Merge similar species and create concatenated alignments","S");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("cancel");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CANCEL","L");

    aws->show();
    return (AW_window *)aws;
}

AW_window *NT_createMergeSimilarSpeciesWindow(AW_root *aw_root, int option){
	static AW_window_simple *aws = 0;
    mergeSimilarConcatenateAlignments = option;

    if(!aws){  //creates window only once and points to species->Merge similar species (menu) and to concatenation window (button)
        aws =new AW_window_simple;

        struct adawcbstruct *cbs = new adawcbstruct;
        cbs->gb_main = gb_main;
        cbs->aws     = aws;

        aws->init(aw_root, "MERGE_SPECIES", "MERGE SPECIES WINDOW", 100, 100 );
        aws->load_xfig("merge_species.fig");

        aws->callback( AW_POPUP_HELP,(AW_CL)"merge_species.hlp");
        aws->at("help");
        aws->create_button("HELP","HELP","H");

        aws->at("field_select");
        aws->auto_space(0,0);
        aws->callback((AW_CB0)AW_POPDOWN);
        awt_create_selection_list_on_scandb(gb_main,aws, AWAR_CON_MERGE_FIELD, AWT_NDS_FILTER,"field_select", 0,  &AWT_species_selector, 20, 30,true,true,true);

        aws->at("store_sp_no");
        aws->label_length(20);
        aws->create_input_field(AWAR_CON_STORE_SIM_SP_NO,20);

        aws->at("merge");
        aws->callback((AW_CB1)mergeSimilarSpecies,0);
        aws->create_button("MERGE_SIMILAR_SPECIES","MERGE SIMILAR SPECIES","M");

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE","CLOSE","C");
    }

    aws->show();
    return (AW_window *)aws;
}
/*-------------------------------------------------------------------------------------------------------*/
