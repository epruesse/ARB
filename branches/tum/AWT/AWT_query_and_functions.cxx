#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include "awt.hxx"
#include "awtlocal.hxx"

/***************** Create the database query box and functions *************************/


enum AWT_EXT_QUERY_TYPES {
    AWT_EXT_QUERY_NONE,
    AWT_EXT_QUERY_COMPARE_LINES,
    AWT_EXT_QUERY_COMPARE_WORDS
};

awt_query_struct::awt_query_struct(void){
	memset((char *)this,0,sizeof(awt_query_struct));
	select_bit = 1;
}

long awt_count_queried_species(struct adaqbsstruct *cbs){
    long count = 0;
    GBDATA *gb_species;
    for (	gb_species = GBT_first_species(cbs->gb_main);
		gb_species;
		gb_species = GBT_next_species(gb_species)){
	if (IS_QUERIED(gb_species,cbs)) {
	    count ++;
	}
    }
    return count;
}

long awt_query_update_list(void *dummy, struct adaqbsstruct *cbs)
{
	AWUSE(dummy);
	GB_push_transaction(cbs->gb_main);
	char buffer[128],*p;
	int	count;
	char	*key = cbs->aws->get_root()->awar(cbs->awar_key)->read_string();

	cbs->aws->clear_selection_list(cbs->result_id);
	count = 0;
	GBDATA *gb_species;
	for (		gb_species = GBT_first_species(cbs->gb_main);
			gb_species;
			gb_species = GBT_next_species(gb_species)){

		if (IS_QUERIED(gb_species,cbs)) {
			if (count < AWT_MAX_QUERY_LIST_LEN ){
				GBDATA *gb_name = GB_find(gb_species,"name",0,down_level);
				if (!gb_name) continue;		// a guy with no name
				char flag = ' ';
				if (GB_read_flag(gb_species)) flag = '*';
				sprintf(buffer,"%c %-12.12s ",
					flag, GB_read_char_pntr(gb_name));
				p = buffer + strlen(buffer);
				GBDATA *gb_key = GB_search(gb_species,key,GB_FIND);
				if (gb_key) {
					char *data = GB_read_as_string(gb_key);
					if (data){
						sprintf(p,":%.70s",data);
						free(data);
					}
				}
				cbs->aws->insert_selection( cbs->result_id, buffer,
					GB_read_char_pntr(gb_name) );
			}else if (count == AWT_MAX_QUERY_LIST_LEN){
				cbs->aws->insert_selection( cbs->result_id, 
					"********* List truncated *********","" );
			}
			count ++;
		}
	}
	cbs->aws->insert_default_selection( cbs->result_id, "End of list", "" );
	cbs->aws->update_selection_list( cbs->result_id );
	delete key;
	cbs->aws->get_root()->awar(cbs->awar_count)->write_int( (long)count);
	GB_pop_transaction(cbs->gb_main);
	return count;
}
				// Mark listed species
				// mark = 1 -> mark listed
				// mark | 8 -> don'd change rest
void awt_do_mark_list(void *dummy, struct adaqbsstruct *cbs, long mark)
	{
	AWUSE(dummy);
	GB_push_transaction(cbs->gb_main);
	GBDATA *gb_species;
	for (		gb_species = GBT_first_species(cbs->gb_main);
			gb_species;
			gb_species = GBT_next_species(gb_species)){

		if (IS_QUERIED(gb_species,cbs)) {
			GB_write_flag(gb_species,mark & 1);
		}else{
		    if ((mark & 8) == 0){
			GB_write_flag(gb_species,1-(mark&1));
		    }
		}
	}
	awt_query_update_list(0,cbs);
	GB_pop_transaction(cbs->gb_main);
}

void awt_unquery_all(void *dummy, struct adaqbsstruct *cbs){
	AWUSE(dummy);
	GB_push_transaction(cbs->gb_main);
	GBDATA *gb_species;
	for (		gb_species = GBT_first_species(cbs->gb_main);
			gb_species;
			gb_species = GBT_next_species(gb_species)){
	    CLEAR_QUERIED(gb_species,cbs);
	}
	awt_query_update_list(0,cbs);
	GB_pop_transaction(cbs->gb_main);
}

void awt_delete_species_in_list(void *dummy, struct adaqbsstruct *cbs)
	{
	AWUSE(dummy);
	GB_begin_transaction(cbs->gb_main);
	GBDATA *gb_species,*gb_next;
	GBDATA *gb_species_data = GB_search(cbs->gb_main,
			"species_data",GB_CREATE_CONTAINER);
	long	cnt = 0;
	for (		gb_species = GBT_first_species_rel_species_data(gb_species_data);
			gb_species;
			gb_species = GBT_next_species(gb_species)){
		if (IS_QUERIED(gb_species,cbs)) cnt++;
	}

	sprintf(AW_ERROR_BUFFER,"ARE YOU SURE TO DELETE %li SPECIES",cnt);
	if (aw_message(AW_ERROR_BUFFER,"OK,CANCEL")){
		GB_abort_transaction(cbs->gb_main);
		return;
	}
	GB_ERROR error = 0;
	for (		gb_species = GBT_first_species_rel_species_data(gb_species_data);
			gb_species;
			gb_species = gb_next){

		gb_next = GBT_next_species(gb_species);
		if (IS_QUERIED(gb_species,cbs)) {
			error = GB_delete(gb_species);
		}
		if (error) break;
	}
	if (error) {
		GB_abort_transaction(cbs->gb_main);
		aw_message(error);
	}else{
		awt_query_update_list(dummy,cbs);
		GB_commit_transaction(cbs->gb_main);
	}

}

GB_HASH *awt_generate_species_hash(GBDATA *gb_main, char *key,int split)
	{
        GB_HASH *hash = GBS_create_hash(10000,1); // @@@ ralf 16/02/2000
	GBDATA *gb_species;
	GBDATA *gb_name;
	char	*keyas;

	for (	gb_species = GBT_first_species(gb_main);
		gb_species;
		gb_species = GBT_next_species(gb_species)){
		gb_name = GB_search(gb_species,key,GB_FIND);
		if (!gb_name) continue;
		keyas = GB_read_as_string(gb_name);
		if (keyas && strlen(keyas)) {
		    if (split){
			char *t;
			for (t = strtok(keyas," ");
			     t;
			     t = strtok(0," "))
			{
			    if (strlen(t) ){
				GBS_write_hash(hash,t,(long)gb_species);
			    }
			}
		    }else{
			GBS_write_hash(hash,keyas,(long)gb_species);
		    }
		delete keyas;
		}
	}
	return hash;
}


	    
void awt_do_query(void *dummy, struct adaqbsstruct *cbs,AW_CL ext_query)
	{
	AWUSE(dummy);
	char	*key = cbs->aws->get_root()->awar(cbs->awar_key)->read_string();
	if (!strlen(key)) {
		delete key;
		aw_message("ERROR: To perfom a query you have to select a field and enter a search string");
		return;
	}


	GB_push_transaction(cbs->gb_main);
	char	*query = cbs->aws->get_root()->awar(cbs->awar_query)->read_string();
	if (cbs->gb_ref && !strlen(query)){
	    if (!ext_query) ext_query = AWT_EXT_QUERY_COMPARE_LINES;
	}
	
	AWT_QUERY_MODES	mode = (AWT_QUERY_MODES)cbs->aws->get_root()->awar(cbs->awar_ere)->read_int();
	AWT_QUERY_TYPES	type = (AWT_QUERY_TYPES)cbs->aws->get_root()->awar(cbs->awar_by)->read_int();
	int	hit;
	GBDATA *gb_key;
	GB_HASH *ref_hash = 0;

	if (cbs->gb_ref && ( ext_query == AWT_EXT_QUERY_COMPARE_LINES || ext_query == AWT_EXT_QUERY_COMPARE_WORDS)) {
		GB_push_transaction(cbs->gb_ref);
		ref_hash = awt_generate_species_hash(cbs->gb_ref,key, ext_query == AWT_EXT_QUERY_COMPARE_WORDS);
	}

	GBDATA *gb_species_data = GB_search(cbs->gb_main,	"species_data",GB_CREATE_CONTAINER);
	GBDATA *gb_species;
	int rek = 0;
	GBQUARK keyquark = -1;
	if (GB_first_non_key_char(key)) {
		rek = 1;
	}else{
		keyquark = GB_key_2_quark(gb_species_data,key);
	}

	for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
	     gb_species;
	     gb_species = GBT_next_species(gb_species)){
		switch(mode) {
		case	AWT_QUERY_GENERATE:	CLEAR_QUERIED(gb_species,cbs); break;
		case	AWT_QUERY_ENLARGE:	if (IS_QUERIED(gb_species,cbs)) goto awt_do_que_cont;
						break;	// already marked;
		case	AWT_QUERY_REDUCE:	if (!IS_QUERIED(gb_species,cbs))goto awt_do_que_cont;
						break;	// already unmarked;
		}
		hit = 0;
		switch(type) {
			case AWT_QUERY_MARKED:
				hit = GB_read_flag(gb_species);
				break;
			case AWT_QUERY_MATCH:
			case AWT_QUERY_DOAWT_MATCH:
				if (rek) {
					gb_key = GB_search(gb_species,key,GB_FIND);
				}else{
					gb_key = GB_find_sub_by_quark(gb_species,keyquark,0,0);
				}
				switch(ext_query){
				case AWT_EXT_QUERY_NONE:
					if (gb_key) {
						char	*data;
						data = GB_read_as_string(gb_key);
						switch (query[0]) {
						case '>':
						    if (atoi(data)> atoi(query+1))
							hit = 1;
						    break;
						case '<':
						    if (atoi(data) < atoi(query+1))
							hit = 1;
						    break;
						default:
						    if (! GBS_string_cmp(data,query,1) ) hit = 1;
						    break;
						}
						delete data;
					}else{
						hit = (strlen(query) == 0);
					}
					break;
				case AWT_EXT_QUERY_COMPARE_LINES:
				case AWT_EXT_QUERY_COMPARE_WORDS:
					if (gb_key) {
						char	*data;
						GBDATA *gb_ref_pntr = 0;
						data = GB_read_as_string(gb_key);
						if (!data || !data[0]) {
						    delete data;
						    break;
						}
						if (ext_query == AWT_EXT_QUERY_COMPARE_WORDS){
							char *t;
							for (	t = strtok(data," ");
								t;
								t = strtok(0," "))
							{
							    gb_ref_pntr = 	(GBDATA *)GBS_read_hash(ref_hash,t);
							    if (gb_ref_pntr){
								if (cbs->look_in_ref_list) {
								    if (IS_QUERIED(gb_ref_pntr,cbs)) hit = 1;
								}else{	
								    hit = 1;
								}
							    }
							}
						}else{
						    gb_ref_pntr = 	(GBDATA *)GBS_read_hash(ref_hash,data);
						    if (gb_ref_pntr){
                                if (cbs->look_in_ref_list) {
                                    if (IS_QUERIED(gb_ref_pntr,cbs)) hit = 1;
                                }else{	
                                    hit = 1;
                                }
						    }
						}
						delete data;
					}
					break;
				}
		
				if (type == AWT_QUERY_DOAWT_MATCH) hit = 1-hit;
				break;
		}
		if (hit) SET_QUERIED(gb_species,cbs);
		else 	CLEAR_QUERIED(gb_species,cbs);

		awt_do_que_cont:;
	}
	delete key;
	delete query;
	awt_query_update_list(0,cbs);
	GB_pop_transaction(cbs->gb_main);
	if (ref_hash){
		GBS_free_hash(ref_hash);
		GB_pop_transaction(cbs->gb_ref);
	}
}

void awt_copy_selection_list_2_queried_species(struct adaqbsstruct *cbs, AW_selection_list* id){
	GB_transaction dummy(cbs->gb_main);
	GBDATA *gb_species;
	for (gb_species = GBT_first_species(cbs->gb_main);
	     gb_species;
	     gb_species = GBT_next_species(gb_species)){
	    CLEAR_QUERIED(gb_species,cbs);
	}
	GBDATA_SET *set = cbs->aws->selection_list_to_species_set(cbs->gb_main,id);
	if (set){
	    int i;
	    for (i=0;i<set->nitems;i++){
		SET_QUERIED(set->items[i],cbs);
	    }
	    GB_delete_set(set);
	}
	awt_query_update_list(0,cbs);	
}


void awt_search_equal_entries(AW_window *,struct adaqbsstruct *cbs,int tokenize){
    char	*key = cbs->aws->get_root()->awar(cbs->awar_key)->read_string();
    if (!strlen(key)) {
	delete key;
	aw_message("ERROR: To perfom a query you have to select a field and enter a search string");
	return;
    }
    GB_transaction dumy(cbs->gb_main);
    GBDATA *gb_species_data = GB_search(cbs->gb_main,	"species_data",GB_CREATE_CONTAINER);
    GBDATA *gb_species;
    GB_HASH	*hash = GBS_create_hash(GB_number_of_subentries(gb_species_data),1);
    for (		gb_species = GBT_first_species(cbs->gb_main);
			gb_species;
			gb_species = GBT_next_species(gb_species)){
	CLEAR_QUERIED(gb_species,cbs);
	GB_write_flag(gb_species,0);
	GBDATA *gb_key = GB_search(gb_species,key,GB_FIND);
	if (!gb_key) continue;
	char *data = GB_read_as_string(gb_key);
	if(!data) continue;
	if (tokenize){
	    char *s;
	    for (s=strtok(data,",; \t."); s ; s = strtok(0,",; \t.")){
		GBDATA *gb_old = (GBDATA *)GBS_read_hash(hash,s);
		if (gb_old){
		    SET_QUERIED(gb_old,cbs);
		    SET_QUERIED(gb_species,cbs);
		    GB_write_flag(gb_species,1);
		}else{
		    GBS_write_hash(hash,s,(long)gb_species);
		}
	    }
	}else{
	    GBDATA *gb_old = (GBDATA *)GBS_read_hash(hash,data);
	    if (gb_old){
		SET_QUERIED(gb_old,cbs);
		SET_QUERIED(gb_species,cbs);
		GB_write_flag(gb_species,1);
	    }else{
		GBS_write_hash(hash,data,(long)gb_species);
	    }
	}
	delete data;
    }
    GBS_free_hash(hash);
    awt_query_update_list(0,cbs);	
}

/***************** Pars fields *************************/

void awt_do_pars_list(void *dummy, struct adaqbsstruct *cbs)
	{
	AWUSE(dummy);
	GB_ERROR error = 0;

	char *key = cbs->aws->get_root()->awar(cbs->awar_parskey)->read_string();
	if (!strcmp(key,"name")){
		if (aw_message(
			"WARNING WARNING WARNING You now try to rename the species\n"
			"	The name is used to link database entries and trees\n"
			"	->	 ALL TREES WILL BE LOST\n"
			"	->	The new names MUST be UNIQUE"
			"		if not you may corrupt the database"
			,"Let's Go,Cancel") ) return;
	}


	if (!strlen(key)) error = "Please select a valid key";

	char *command = cbs->aws->get_root()->awar(cbs->awar_parsvalue)->read_string();
	if (!error && !strlen(command)) {
		error = "Please enter your command";
	}

	if (error) {
		if (strlen(error)) aw_message(error);
		delete command;
		delete key;
		return;
	}
	
	GB_begin_transaction(cbs->gb_main);
	GBDATA *gb_key_data = GB_search(cbs->gb_main,CHANGE_KEY_PATH,GB_CREATE_CONTAINER);
	GBDATA *gb_species;
	GBDATA *gb_key_name;

	while (!(gb_key_name  = GB_find(gb_key_data,CHANGEKEY_NAME,key,down_2_level))) {
		sprintf(AW_ERROR_BUFFER,"The destination field '%s' does not exists",key);
		if (aw_message(AW_ERROR_BUFFER,"Create Field (Type STRING),Cancel")){
			GB_abort_transaction(cbs->gb_main);
			delete command;
			delete key;
			return;
		}
		GB_ERROR error2 = awt_add_new_changekey(cbs->gb_main,key,GB_STRING);
		if (error2){
			aw_message(error2);
			GB_abort_transaction(cbs->gb_main);
			delete command;
			delete key;
			return;
		}
	}

	GBDATA *gb_key_type = GB_find(gb_key_name,CHANGEKEY_TYPE,0,this_level);

	long	count= 0, ncount = cbs->aws->get_root()->awar(cbs->awar_count)->read_int();
	char *deftag = cbs->aws->get_root()->awar(cbs->awar_deftag)->read_string();
	char *tag = cbs->aws->get_root()->awar(cbs->awar_tag)->read_string();
	{
	    long use_tag = cbs->aws->get_root()->awar(cbs->awar_use_tag)->read_int();
	    if (!use_tag || !strlen(tag)) {
		delete tag;
		tag = 0;
	    }
	}
	int double_pars = cbs->aws->get_root()->awar(cbs->awar_double_pars)->read_int();
	    
	aw_openstatus("Pars Fields");

	for (gb_species = GBT_first_species(cbs->gb_main);
	     gb_species;
	     gb_species = GBT_next_species(gb_species)){

		if (IS_QUERIED(gb_species,cbs)) {
			if (aw_status((count++)/(double)ncount)) {
				error = "Operation aborted";
				break;
			}
			GBDATA *gb_new = GB_search(gb_species, key,GB_FIND);
			char *str = 0;
			if (gb_new) {
				str = GB_read_as_string(gb_new);
			}else{
				str = strdup("");
			}
			char *parsed = 0;
			if (double_pars){
			    char *com2 = 0;
			    parsed = 0;
			    com2 = GB_command_interpreter(cbs->gb_main, str,command,gb_species);
			    if (com2){
				if (tag){
				    parsed = GBS_string_eval_tagged_string(cbs->gb_main, "",deftag,tag,0,com2,gb_species);
				}else{
				    parsed = GB_command_interpreter(cbs->gb_main, "",com2,gb_species);
				}
			    }
			    delete com2;
			}else{
			    if (tag){
				parsed = GBS_string_eval_tagged_string(cbs->gb_main, str,deftag,tag,0,command,gb_species);
			    }else{
				parsed = GB_command_interpreter(cbs->gb_main, str,command,gb_species);
			    }
			}
			if (!parsed) {
				error = GB_get_error();
				delete str;
				break;
			}
			if (!strcmp(parsed,str)){	// nothing changed
				delete str;
				delete parsed;
				continue;
			}
			if (gb_new && !strlen(parsed) ) {
				error = GB_delete(gb_new);
			}else {
				if (!gb_new) {
					gb_new = GB_search(gb_species, key,
						GB_read_int(gb_key_type));
					if (!gb_new){
						error = GB_get_error();
					}
				}
				if (!error) {
					error = GB_write_as_string(gb_new,parsed);
				}
			}
			delete str;
			delete parsed;
		}
		if (error) break;
	}
	aw_closestatus();
	delete tag;
	delete deftag;
	if (error) {
		GB_abort_transaction(cbs->gb_main);
		aw_message(error);
	}else{
		GB_commit_transaction(cbs->gb_main);
	}
	delete key;
	delete command;
}

void awt_predef_prg(AW_root *aw_root, struct adaqbsstruct *cbs){
	char *str = aw_root->awar(cbs->awar_parspredefined)->read_string();
	char *brk = strchr(str,'#');
	if (brk) {
		*(brk++) = 0;
		char *kv = str;
		if (!strcmp(str,"ali_*/data")){
		    GB_transaction valid_transaction(cbs->gb_main);
		    char *use = GBT_get_default_alignment(cbs->gb_main);
		    kv = strdup(GBS_global_string("%s/data",use));
		    delete use;
		}
		aw_root->awar(cbs->awar_parskey)->write_string( kv);
		if (kv != str) delete kv;
		aw_root->awar(cbs->awar_parsvalue)->write_string( brk);
	}else{
		aw_root->awar(cbs->awar_parsvalue)->write_string( str);
	}
	delete str;
}

AW_window *create_awt_open_parser(AW_root *aw_root, struct adaqbsstruct *cbs)
	{
	AW_window_simple *aws = 0;
	aws = new AW_window_simple;
	aws->init( aw_root, "MODIFY_DATABASE_FIELD", "MODIFY DATABASE FIELD",     600, 0 );
	aws->load_xfig("awt/parser.fig");

	aws->at("close");
	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE","CLOSE","C");

	aws->at("help");
	aws->callback(AW_POPUP_HELP,(AW_CL)"mod_field_list.hlp");
	aws->create_button("HELP","HELP","H");

	aws->at("helptags");
	aws->callback(AW_POPUP_HELP,(AW_CL)"tags.hlp");
	aws->create_button("HELP_TAGS", "HELP TAGS","H");

	aws->at("usetag");	aws->create_toggle(cbs->awar_use_tag);
	aws->at("deftag");	aws->create_input_field(cbs->awar_deftag);
	aws->at("tag");		aws->create_input_field(cbs->awar_tag);
	    
	aws->at("double");	aws->create_toggle(cbs->awar_double_pars);

	awt_create_selection_list_on_scandb(cbs->gb_main,aws,cbs->awar_parskey,AWT_PARS_FILTER, "field",0);

	aws->at("go");
	aws->callback((AW_CB1)awt_do_pars_list,(AW_CL)cbs);
	aws->create_button("GO", "GO","G");

	aws->at("parser");
	aws->create_text_field(cbs->awar_parsvalue);

	aws->at("pre");
	AW_selection_list *id = 	aws->create_selection_list(cbs->awar_parspredefined);
	char *filename = AWT_unfold_path("lib/sellists/mod_fields*.sellst","ARBHOME");
	GB_ERROR error = aws->load_selection_list(id,filename);
	delete filename;
	if (error) aw_message(error);
	else{
		aws->get_root()->awar(cbs->awar_parspredefined)->add_callback(
			(AW_RCB1)awt_predef_prg,(AW_CL)cbs);
	}
	return (AW_window *)aws;

}


/***************** Multi set fields *************************/

void awt_do_set_list(void *dummy, struct adaqbsstruct *cbs, long append)
{
    AWUSE(dummy);

    GB_ERROR error = 0;

    char *key = cbs->aws->get_root()->awar(cbs->awar_setkey)->read_string();
    if (!strcmp(key,"name")) error = "You cannot set the names field";

    char *value = cbs->aws->get_root()->awar(cbs->awar_setvalue)->read_string();
    if (!strlen(value)) {
	delete value;
	value = 0;
    }

	
    GB_begin_transaction(cbs->gb_main);
    GBDATA *gb_key_data = GB_search(cbs->gb_main,CHANGE_KEY_PATH,GB_CREATE_CONTAINER);
    GBDATA *gb_species;
    GBDATA *gb_key_name = GB_find(gb_key_data,CHANGEKEY_NAME,key,down_2_level);
    if (!gb_key_name) {
	sprintf(AW_ERROR_BUFFER,"The destination field '%s' does not exists",key);
	aw_message();
	delete value;
	delete key;
	GB_commit_transaction(cbs->gb_main);
	return;
    }

    GBDATA *gb_key_type = GB_find(gb_key_name,CHANGEKEY_TYPE,0,this_level);
    for (gb_species = GBT_first_species(cbs->gb_main);
	 gb_species;
	 gb_species = GBT_next_species(gb_species)){

	if (IS_QUERIED(gb_species,cbs)) {
	    GBDATA *gb_new = GB_search(gb_species, key,GB_FIND);
	    if (gb_new) {
		if (value){
		    if (append){
			char *old = GB_read_as_string(gb_new);
			if (old){
			    void *strstr = GBS_stropen(1024);
			    GBS_strcat(strstr,old);				
			    GBS_strcat(strstr,value);
			    char *v = GBS_strclose(strstr,0);
			    error = GB_write_as_string(gb_new,v);
			    delete v;
			}else{
			    char *name = GBT_read_string(gb_species,"name");
			    error = GB_export_error("Field '%s' of species '%s' has incombatible type",key,name);
			    delete name;
			}
		    }else{

			error = GB_write_as_string(gb_new,value);
		    }
		}else{
		    if (!append){
			error = GB_delete(gb_new);
		    }
		}
	    }else {
		gb_new = GB_search(gb_species, key, GB_read_int(gb_key_type));
		if (!gb_new){
		    error = GB_get_error();
		}
	   
	    	if (!error) {
			error = GB_write_as_string(gb_new,value);
	    	}
	    }
	}
	if (error) break;
    }
    if (error) {
	GB_abort_transaction(cbs->gb_main);
	aw_message(error);
    }else{
	GB_commit_transaction(cbs->gb_main);
    }
    delete key;
    delete value;
}

AW_window *create_awt_do_set_list(AW_root *aw_root, struct adaqbsstruct *cbs)
	{
	AW_window_simple *aws = 0;
	aws = new AW_window_simple;
	aws->init( aw_root, "SET_DATABASE_FIELD_OF_LISTED","SET MANY FIELDS", 600, 0 );
	aws->load_xfig("ad_mset.fig");

	aws->at("close");
	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE", "CLOSE","C");

	aws->at("help");
	aws->callback( AW_POPUP_HELP,(AW_CL)"write_field_list.hlp");
	aws->create_button("HELP", "HELP","H");

	awt_create_selection_list_on_scandb(cbs->gb_main,aws,cbs->awar_setkey,
			AWT_NDS_FILTER, "box",0);
	aws->at("create");
	aws->callback((AW_CB)awt_do_set_list,(AW_CL)cbs,0);
	aws->create_button("SET_SINGLE_FIELD_OF_LISTED","WRITE");
	aws->at("do");
	aws->callback((AW_CB)awt_do_set_list,(AW_CL)cbs,1);
	aws->create_button("APPEND_SINGLE_FIELD_OF_LISTED","APPEND");

	aws->at("val");
	aws->create_text_field(cbs->awar_setvalue,2,2);
	return (AW_window *)aws;

}

/***************** Multi set fields *************************/

void awt_do_set_protection(void *dummy, struct adaqbsstruct *cbs)
	{
	AWUSE(dummy);

	char *key = cbs->aws->get_root()->awar(cbs->awar_setkey)->read_string();
	
	GB_begin_transaction(cbs->gb_main);
	GBDATA *gb_key_data = GB_search(cbs->gb_main,CHANGE_KEY_PATH,GB_CREATE_CONTAINER);
	GBDATA *gb_species;
	GBDATA *gb_key_name = GB_find(gb_key_data,CHANGEKEY_NAME,key,down_2_level);
	if (!gb_key_name) {
		sprintf(AW_ERROR_BUFFER,"The destination field '%s' does not exists",key);
		aw_message();
		delete key;
		GB_commit_transaction(cbs->gb_main);
		return;
	}
	int level = cbs->aws->get_root()->awar(cbs->awar_setprotection)->read_int();
	GB_ERROR error = 0;
	for (gb_species = GBT_first_species(cbs->gb_main);
	     gb_species;
	     gb_species = GBT_next_species(gb_species)){

		if (IS_QUERIED(gb_species,cbs)) {
			GBDATA *gb_new = GB_search(gb_species, key,GB_FIND);
			if (!gb_new) continue;
			error = GB_write_security_delete(gb_new,level);
			error = GB_write_security_write(gb_new,level);
		}
		if (error) break;
	}
	if (error) {
		GB_abort_transaction(cbs->gb_main);
		aw_message(error);
	}else{
		GB_commit_transaction(cbs->gb_main);
	}
	delete key;
}

AW_window *create_awt_set_protection(AW_root *aw_root, struct adaqbsstruct *cbs)
	{
	AW_window_simple *aws = 0;
	aws = new AW_window_simple;
	aws->init( aw_root, "SET_PROTECTION_OF_FIELD_OF_LISTED", "SET PROTECTIONS OF FIELDS", 600, 0 );
	aws->load_xfig("awt/set_protection.fig");

	aws->at("close");
	aws->callback( (AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE","CLOSE","C");

	aws->at("help");
	aws->callback( AW_POPUP_HELP,(AW_CL)"set_protection.hlp");
	aws->create_button("HELP", "HELP","H");


	aws->at("prot");
	aws->create_toggle_field(cbs->awar_setprotection,0);
	aws->insert_toggle("0 Temporary","0",0);
	aws->insert_toggle("1 Checked","1",1);
	aws->insert_toggle("2","2",2);
	aws->insert_toggle("3","3",3);
	aws->insert_toggle("4 normal","4",4);
	aws->insert_toggle("5 ","5",5);
	aws->insert_toggle("6 the truth","5",6);
	
	awt_create_selection_list_on_scandb(cbs->gb_main,aws,cbs->awar_setkey,
			AWT_NDS_FILTER, "list",0);

	aws->at("go");
	aws->callback((AW_CB1)awt_do_set_protection,(AW_CL)cbs);
	aws->create_button("SET_PROTECTION_OF_FIELD_OF_LISTED", "SET PROTECTION");

	return (AW_window *)aws;
}

void awt_toggle_flag(AW_window *aww, struct adaqbsstruct *cbs) {
	GB_transaction dummy(cbs->gb_main);
	char *sname = aww->get_root()->awar(cbs->species_name)->read_string();
	if (strlen(sname)){
		GBDATA *gb_species = GBT_find_species(cbs->gb_main,sname);
		if (gb_species) {
			long flag = GB_read_flag(gb_species);
			GB_write_flag(gb_species,1-flag);
		}
	}

	delete sname;
	awt_query_update_list(aww,cbs);
}

/***************** Create the database query box and functions *************************/
struct adaqbsstruct *awt_create_query_box(AW_window *aws, awt_query_struct *awtqs)	// create the query box
{
	static int query_id = 0;
	char buffer[256];
	AW_root *aw_root = aws->get_root();
	GBDATA *gb_main = awtqs->gb_main;
	struct adaqbsstruct *cbs = (struct adaqbsstruct *)calloc(1,
			sizeof(struct adaqbsstruct));
	cbs->gb_main	= awtqs->gb_main;
	cbs->aws = aws;
	cbs->gb_ref	= awtqs->gb_ref;
	cbs->look_in_ref_list	= awtqs->look_in_ref_list;
	cbs->select_bit	= awtqs->select_bit;
	cbs->species_name	= strdup(awtqs->species_name);
	GB_push_transaction(gb_main);
			/*************** Create local AWARS *******************/

	sprintf(buffer,"tmp/arbdb_query_%i/key",query_id);
	cbs->awar_key = strdup(buffer);	
	aw_root->awar_string( cbs->awar_key, "name", AW_ROOT_DEFAULT);

	sprintf(buffer,"tmp/arbdb_query_%i/query",query_id);
	cbs->awar_query = strdup(buffer);	
	aw_root->awar_string( cbs->awar_query, "", AW_ROOT_DEFAULT);

	sprintf(buffer,"tmp/arbdb_query_%i/ere",query_id);
	cbs->awar_ere = strdup(buffer);	
	aw_root->awar_int( cbs->awar_ere, AWT_QUERY_GENERATE);

	sprintf(buffer,"tmp/arbdb_query_%i/count",query_id);
	cbs->awar_count = strdup(buffer);	
	aw_root->awar_int( cbs->awar_count, 0);

	sprintf(buffer,"tmp/arbdb_query_%i/by",query_id);
	cbs->awar_by = strdup(buffer);	
	aw_root->awar_int( cbs->awar_by, AWT_QUERY_MATCH);

	if (awtqs->ere_pos_fig){
		aws->at(awtqs->ere_pos_fig);
		aws->create_toggle_field(cbs->awar_ere,"","");
		aws->insert_toggle("Search species that","G",(int)AWT_QUERY_GENERATE);
		aws->insert_toggle("Add Species that","E",(int)AWT_QUERY_ENLARGE);
		aws->insert_toggle("Keep Species that","R",(int)AWT_QUERY_REDUCE);
		aws->update_toggle_field();
	}
	if (awtqs->by_pos_fig){
		aws->at(awtqs->by_pos_fig);
		aws->create_toggle_field(cbs->awar_by,"","");
		aws->insert_toggle("match the query","M",(int)AWT_QUERY_MATCH);
		aws->insert_toggle("dont match the q.","D",(int)AWT_QUERY_DOAWT_MATCH);
		aws->insert_toggle("are marked","A",(int)AWT_QUERY_MARKED);
		aws->update_toggle_field();
	}

	if (awtqs->qbox_pos_fig){
		awt_create_selection_list_on_scandb(gb_main,aws,cbs->awar_key,
			AWT_NDS_FILTER, awtqs->qbox_pos_fig,awtqs->rescan_pos_fig);
	}
	if (awtqs->key_pos_fig){
		aws->at(awtqs->key_pos_fig);
		aws->create_input_field(cbs->awar_key,12);
	}

	if (awtqs->query_pos_fig){
		aws->at(awtqs->query_pos_fig);
		aws->d_callback((AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_NONE);
		aws->create_input_field(cbs->awar_query,12);
	}

	if (awtqs->result_pos_fig){
		aws->at(awtqs->result_pos_fig);
		if(awtqs->create_view_window) {
			aws->callback(AW_POPUP,awtqs->create_view_window,0);
		}
		aws->d_callback((AW_CB1)awt_toggle_flag,(AW_CL)cbs);
		cbs->result_id = aws->create_selection_list(cbs->species_name,"","",5,5);
		aws->insert_default_selection( cbs->result_id, "end of list", "" );
		aws->update_selection_list( cbs->result_id );
	}
	if (awtqs->count_pos_fig){
		aws->at(awtqs->count_pos_fig);
		aws->label("Hits:");
		aws->create_button(0,cbs->awar_count);	
	}

	aws->button_length(19);
	if (awtqs->do_query_pos_fig){
		aws->at(awtqs->do_query_pos_fig);
		aws->callback((AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_NONE);
		aws->highlight();
		aws->create_button("SEARCH", "SEARCH","Q");
	}
	if (awtqs->do_refresh_pos_fig && !GB_NOVICE){
		aws->at(awtqs->do_refresh_pos_fig);
		aws->callback((AW_CB1)awt_query_update_list,(AW_CL)cbs);
		aws->create_button("REFRESH_HITLIST", "REFRESH","R");
	}
	if (awtqs->do_mark_pos_fig){
		aws->at(awtqs->do_mark_pos_fig);
		aws->help_text("mark.hlp");
		aws->callback((AW_CB)awt_do_mark_list,(AW_CL)cbs,1);
		aws->create_button("MARK_LISTED_UNMARK_REST", "MARK LISTED\nUNMARK REST","M");
	}
	if (awtqs->do_unmark_pos_fig){
		aws->at(awtqs->do_unmark_pos_fig);
		aws->help_text("mark.hlp");
		aws->callback((AW_CB)awt_do_mark_list,(AW_CL)cbs,0);
		aws->create_button("UNMARK_LISTED_MARK_REST","UNMARK LISTED\nMARK REST","U");
	}
	if (awtqs->do_delete_pos_fig){
		aws->at(awtqs->do_delete_pos_fig);
		aws->callback((AW_CB)awt_delete_species_in_list,(AW_CL)cbs,0);
		aws->create_button("DELETE_LISTED","DELETE LISTED","D");
	}
	if (awtqs->do_set_pos_fig && !GB_NOVICE){
		sprintf(buffer,"tmp/arbdb_query_%i/set_key",query_id);
		cbs->awar_setkey = strdup(buffer);	
		aw_root->awar_string( cbs->awar_setkey);

		sprintf(buffer,"tmp/arbdb_query_%i/set_protection",query_id);
		cbs->awar_setprotection = strdup(buffer);	
		aw_root->awar_int( cbs->awar_setprotection, 4);

		sprintf(buffer,"tmp/arbdb_query_%i/set_value",query_id);
		cbs->awar_setvalue = strdup(buffer);	
		aw_root->awar_string( cbs->awar_setvalue);

		aws->at(awtqs->do_set_pos_fig);
		aws->callback(AW_POPUP,(AW_CL)create_awt_do_set_list,(AW_CL)cbs);
		aws->create_button("WRITE_TO_FIELDS_OF_LISTED", "WRITE TO FIELDS\nOF LISTED SP.","S");
	}

	if ( ( awtqs->use_menu || awtqs->open_parser_pos_fig) && !GB_NOVICE){
		sprintf(buffer,"tmp/arbdb_query_%i/tag",query_id);		cbs->awar_tag = strdup(buffer);		aw_root->awar_string( cbs->awar_tag);
		sprintf(buffer,"tmp/arbdb_query_%i/use_tag",query_id);		cbs->awar_use_tag = strdup(buffer);	aw_root->awar_int( cbs->awar_use_tag);
		sprintf(buffer,"tmp/arbdb_query_%i/deftag",query_id);		cbs->awar_deftag = strdup(buffer);	aw_root->awar_string( cbs->awar_deftag);
		sprintf(buffer,"tmp/arbdb_query_%i/double_pars",query_id);	cbs->awar_double_pars = strdup(buffer);	aw_root->awar_int( cbs->awar_double_pars);
		sprintf(buffer,"tmp/arbdb_query_%i/parskey",query_id);		cbs->awar_parskey = strdup(buffer);	aw_root->awar_string( cbs->awar_parskey);
		sprintf(buffer,"tmp/arbdb_query_%i/parsvalue",query_id);	cbs->awar_parsvalue = strdup(buffer);	aw_root->awar_string( cbs->awar_parsvalue);
		sprintf(buffer,"tmp/arbdb_query_%i/awar_parspredefined",query_id);cbs->awar_parspredefined = strdup(buffer);aw_root->awar_string( cbs->awar_parspredefined);

		if (awtqs->use_menu){
		    aws->insert_menu_topic("mod_fields_of_listed","Modify Fields of Listed Sp.","F","mod_field_list.hlp",-1,AW_POPUP,(AW_CL)create_awt_open_parser,(AW_CL)cbs);
		}else{
		    aws->at(awtqs->open_parser_pos_fig);
		    aws->callback(AW_POPUP,(AW_CL)create_awt_open_parser,(AW_CL)cbs);
		    aws->create_button("MODIFY_FIELDS_OF_LISTED", "MODIFY FIELDS\nOF LISTED SP","F");
		}
	}
	if (awtqs->use_menu && !GB_NOVICE){
	    aws->insert_menu_topic("s_prot_of_listed","Set Protection of Fields of Listed Sp.","P","set_protection.hlp",-1,AW_POPUP,(AW_CL)create_awt_set_protection,(AW_CL)cbs);
	    aws->insert_separator();
	    aws->insert_menu_topic("mark_listed_species",	"Mark Listed Species, don't Change Rest","M","mark.hlp",-1,(AW_CB)awt_do_mark_list,(AW_CL)cbs,(AW_CL)1 | 8);
	    aws->insert_menu_topic("mark_listed_unmark_rest",	"Mark Listed Species, Unmark Rest","L","mark.hlp",-1,(AW_CB)awt_do_mark_list,(AW_CL)cbs,(AW_CL)1);
	    aws->insert_menu_topic("unmark_listed",		"Unmark Listed Species, don't Change Rest","U","mark.hlp",-1,(AW_CB)awt_do_mark_list,(AW_CL)cbs,(AW_CL)0 | 8);
	    aws->insert_menu_topic("unmark_listed_mark_rest",	"Unmark Listed Species, Mark Rest","R","mark.hlp",-1,(AW_CB)awt_do_mark_list,(AW_CL)cbs,(AW_CL)0);
	    if (cbs->gb_ref){
		aws->insert_separator();
		if (cbs->look_in_ref_list){
		    aws->insert_menu_topic("search_equal_fields_and_listed_in_I",	"Search Species with an Entry Which Exists in Both Databases and are Listed in the Database I hitlist","S",
					   "compare_lines.hlp",-1,(AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_COMPARE_LINES);
		    aws->insert_menu_topic("search_equal_words_and_listed_in_I","Search Species with an Entry Word Which Exists in Both Databases and are Listed in the Database I hitlist","S",
					   "compare_lines.hlp",-1,(AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_COMPARE_WORDS);
		}else{
		    aws->insert_menu_topic("search_equal_field_in_both_db",	"Search Species with an Entry Which Exists in Both Databases","S",
					   "compare_lines.hlp",-1,(AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_COMPARE_LINES);
		    aws->insert_menu_topic("search_equal_word_in_both_db","Search Species with an Entry Word Which Exists in Both Databases","S",
					   "compare_lines.hlp",-1,(AW_CB)awt_do_query,(AW_CL)cbs,AWT_EXT_QUERY_COMPARE_WORDS);
		}
	    }
	}
	
	query_id++;
	GB_pop_transaction(gb_main);
	return cbs;
}
