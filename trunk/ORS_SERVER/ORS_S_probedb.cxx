/*
#################################
#                               #
#    ORS_SERVER:  PROBEDB       #
#    manage probe database      #
#                               #
################################# */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

// #include <malloc.h>
#include <math.h>

#include <ors_lib.h>
#include <ors_server.h>
#include <ors_client.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <ors_prototypes.h>
#include <server.h>
#include <servercntrl.h>
#include <struct_man.h>

#include "ors_s.h"
#include "ors_s_common.hxx"
#include "ors_s_proto.hxx"

GBDATA *gb_probedb;

struct OS_probe {		// sorting element
	char *probe_id;
	char *author_date;
	char *author;
	char *probe_name;
	char *probe_info;
} OS_probe;

/*****************************************************************************
  READ PROBEDB FIELDS from ors_lib
*****************************************************************************/
void OS_read_probedb_fields_into_pdb_list(ORS_main *ors_main) {
	static char buffer[256];
	static char buffer2[100];
	FILE 	*file;
	char	*p;
	char 	*tmp;
	OS_pdb_list *pdb_list_elem;

	file = fopen(ORS_LIB_PATH "probedb_fields","r");
	if (!file) ORS_export_error("Can't open file: " ORS_LIB_PATH "probedb_fields");

	for (	p=fgets(buffer,255,file);
		p;
		p = fgets(p,255,file)) {
		if (buffer[0] == '#' || ORS_str_char_count(buffer, ',') == 0) continue;

		pdb_list_elem = create_OS_pdb_list();	// create list element

		tmp = strtok(buffer,",");	// extract section/name or name
		if (strchr(tmp,'/')) {
			pdb_list_elem->section = strdup(strtok(tmp,"/"));
			tmp += strlen(pdb_list_elem->section) + 1;
		} else {
			pdb_list_elem->section = strdup("");
		}
		sprintf(buffer2,"p_%s",tmp);
		pdb_list_elem->name    = strdup(buffer2);

		if (!strcmp(pdb_list_elem->section,"")) sprintf(buffer2,"%s",pdb_list_elem->name);
		else					sprintf(buffer2,"%s__%s",pdb_list_elem->section,pdb_list_elem->name);
		pdb_list_elem->db_name = strdup(buffer2);

		if (buffer) tmp = buffer + strlen(buffer) + 1;
		pdb_list_elem->typ 		= strdup(strtok(tmp,","));
		if (tmp) tmp += strlen(tmp)+1;
		pdb_list_elem->width 		= atoi(strtok(tmp,","));
		if (tmp) tmp += strlen(tmp)+1;
		pdb_list_elem->height 		= atoi(strtok(tmp,","));
		if (tmp) tmp += strlen(tmp)+1;
		pdb_list_elem->rights 		= strdup(strtok(tmp,","));

		// virtual field: smash it!
		if (strchr(pdb_list_elem->rights,'V')) {
			delete pdb_list_elem->section;
			delete pdb_list_elem->name;
			delete pdb_list_elem->db_name;
			delete pdb_list_elem->typ;
			delete pdb_list_elem->rights;
			delete pdb_list_elem;
			continue;
		}

		if (tmp) tmp += strlen(tmp)+1;
		pdb_list_elem->init 		= strdup(strtok(tmp,","));
		if (tmp) tmp += strlen(tmp)+1;
		pdb_list_elem->label 		= strdup(strtok(tmp,","));
		if (tmp) tmp += strlen(tmp)+1;
		pdb_list_elem->help_page 	= strdup(strtok(tmp,","));
		if (tmp) tmp += strlen(tmp)+1;
		pdb_list_elem->features 	= strdup(strtok(tmp,","));
		pdb_list_elem->content 		= 0;

		aisc_link((dllpublic_ext*)(&(ors_main->ppdb_list)),(dllheader_ext*)pdb_list_elem); 	// put element into list
	}
	fclose(file);
}

/*************************************************************************************
  open the probe database
*************************************************************************************/
GB_ERROR OS_open_probedb(void){

	char *name = ORS_read_a_line_in_a_file(ORS_LIB_PATH "CONFIG","PROBE_DB");
	if (!name) ORS_export_error("Missing 'PROBE_DB' in '" ORS_LIB_PATH "CONFIG'");

	gb_probedb = GB_open(name,"rwc");
	if (!gb_probedb) return GB_get_error();
	return 0;
}

/*************************************************************************************
  close the probe database (save changes)
*************************************************************************************/
extern "C" GB_ERROR OS_save_probedb(void){
	static long last_saved=0;
	last_saved = GB_read_clock(gb_probedb);
	return GB_save(gb_probedb,0,"a");
}

/*************************************************************************************
  INSERT NEW PROBE into probe database
							no authorisation here!
*************************************************************************************/
char *OS_new_probe(ORS_local *locs) {
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends
	ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;
	OS_pdb_list *pdb_elem;

	pdb_elem = OS_find_pdb_list_elem_by_name(ors_main->pdb_list, "", "p_probe_name");
	if (!(pdb_elem->content && *pdb_elem->content)) {
		delete locs->error;
		locs->error=strdup("You have to provide an internal probe name!");
	}
	pdb_elem = OS_find_pdb_list_elem_by_name(ors_main->pdb_list, "", "p_probe_info");
	if (!(pdb_elem->content && *pdb_elem->content)) {
		delete locs->error;
		locs->error=strdup("You have to provide a probe name!");
	}

	locs->probe_id = OS_create_new_probe_id(locs->userpath);

	GBDATA *GB_probe = GB_create_container(gb_probedb,"probe");
	//GBDATA *gb_field;

	OS_write_gb_probe_info_string(GB_probe, "probe_id", locs->probe_id);	//! unique id string for all probes of all users

	for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
		OS_write_gb_probe_info_string(GB_probe, pdb_elem->db_name, pdb_elem->content);
	}
	return "";
}
/*************************************************************************************
  UPDATE PROBE DATA in probe database
							return error message
							no authorisation here!
*************************************************************************************/
char * OS_update_probe(ORS_local *locs ) {
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends

	static char buffer[40];
	ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;
	OS_pdb_list *pdb_elem;

	if (!locs->userpath || !*(locs->userpath)) return "userpath not set (update_probe).";

	pdb_elem = OS_find_pdb_list_elem_by_name(ors_main->pdb_list, "", "p_probe_name");
	if (!(pdb_elem->content && *pdb_elem->content)) {
		delete locs->error;
		locs->error=strdup("You have to provide an internal probe name!");
	}
	pdb_elem = OS_find_pdb_list_elem_by_name(ors_main->pdb_list, "", "p_probe_info");
	if (!(pdb_elem->content && *pdb_elem->content)) {
		delete locs->error;
		locs->error=strdup("You have to provide a probe name!");
	}


	GBDATA *gb_probe_id = GB_find(gb_probedb,"probe_id",locs->probe_id,down_2_level);
	if (!gb_probe_id) return "Probe not found (update_probe)";
	GBDATA *gb_probe = GB_get_father(gb_probe_id);
	GBDATA *gb_field;

	gb_field = GB_search(gb_probe,"ta_id",GB_INT);
	int read_ta_id = GB_read_int(gb_field);
	if (read_ta_id != locs->probe_ta_id) return "Probe data has been changed in the meantime. Please reload update page.";

	char *last_section=0;
	static char *read_data=0;
	for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
		if (!pdb_elem->content) continue;
		delete read_data;
		read_data = OS_read_gb_probe_info_string(gb_probe, pdb_elem->db_name);
		if (!ORS_strcmp(read_data, pdb_elem->content)) continue;
		OS_write_gb_probe_info_string(gb_probe, pdb_elem->db_name, pdb_elem->content);

		if (ORS_strcmp(pdb_elem->section, last_section)) {
			// this line is for makedoku; GB_probe("section__last_mod") //! last modification date and time for each section
			sprintf(buffer,"%s__p_last_mod", pdb_elem->section);
			OS_write_gb_probe_info_string(gb_probe, buffer, ORS_time_and_date_string(DATE_DMY, 0));
			last_section = pdb_elem->section;
		}
	}

	return "";
}
/*************************************************************************************
  DELETE PROBE DATA in probe database
							return error message
							no authorisation here!
*************************************************************************************/
char * OS_delete_probe(ORS_local *locs) {
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends

	//static char buffer[40];
	//ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;
	//OS_pdb_list *pdb_elem;

	if (!locs->userpath || !*(locs->userpath)) return "userpath not set (delete_probe).";


	GBDATA *gb_probe_id = GB_find(gb_probedb,"probe_id",locs->probe_id,down_2_level);
	if (!gb_probe_id) return "Probe not found (delete_probe)";
	GBDATA *gb_probe = GB_get_father(gb_probe_id);
	GBDATA *gb_field;

	gb_field = GB_search(gb_probe,"ta_id",GB_INT);
	int read_ta_id = GB_read_int(gb_field);
	if (read_ta_id != locs->probe_ta_id) return "Probe data has been changed in the meantime. Please reload delete page.";

	/* not necessary!
	for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
		gb_field = GB_find(gb_probe,pdb_elem->db_name,0,down_level);
		if (gb_field) GB_delete(gb_field);
	}
	gb_field = GB_find(gb_probe,"ta_id",0,down_level);
	if (gb_field) GB_delete(gb_field);
	*/

	GB_delete(gb_probe);

	return "";
}

/*************************************************************************************
  READ PROBE FIELD INFORMATION from probe database
							return data or NULL
							no authorisation here!
*************************************************************************************/
char * OS_read_probe_info(char *probe_id, char *fieldname ){
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends
	GBDATA *gb_probe_id = GB_find(gb_probedb,"probe_id",probe_id,down_2_level);
	if (!gb_probe_id) return NULL;
	GBDATA *gb_probe = GB_get_father(gb_probe_id);
	GBDATA *gb_field = GB_find(gb_probe,fieldname,0,down_level);
	if (!gb_field) return NULL;
	return GB_read_string(gb_field);
}

/*************************************************************************************
  WRITE PROBE FIELD INFORMATION into user database WITH EXISTING GB_PROBE
	a non existing field is being created (if content is not empty)
							return error message or NULL
							no authorisation here!
*************************************************************************************/
GB_ERROR OS_write_gb_probe_info_string(GBDATA *gb_probe, char *fieldname, char *content){
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends

	GBDATA *gb_field = GB_find(gb_probe,fieldname,0,down_level);
	if (!gb_field) 	{
		if (!content || !*content) return 0;
		gb_field = GB_create(gb_probe,fieldname,GB_STRING);
	}

	return GB_write_string(gb_field,content);
}

/*************************************************************************************
  READ PROBE FIELD INFORMATION from user database WITH EXISTING GB_PROBE
							return value or NULL
							no authorisation here!
*************************************************************************************/
char * OS_read_gb_probe_info_string(GBDATA *gb_probe, char *fieldname){
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends

	GBDATA *gb_field = GB_find(gb_probe,fieldname,0,down_level);
	if (!gb_field) 	return 0;

	return GB_read_string(gb_field);
}


/*****************************************************************************
  POINTER TO NEXT PDB ELEMENT
*****************************************************************************/
OS_pdb_list *OS_next_pdb_list_elem(OS_pdb_list *head, OS_pdb_list *elem) {
	if (elem->next == head) return 0;
	else return elem->next;
}

/*****************************************************************************
  FIND PDB ELEMENT BY NAME
						return 0 if not found
*****************************************************************************/
OS_pdb_list *OS_find_pdb_list_elem_by_name(OS_pdb_list *head, char *section, char *name) {
	OS_pdb_list *pdb_elem;

	for (pdb_elem = head; pdb_elem; pdb_elem = OS_next_pdb_list_elem(head, pdb_elem)) {
		if (!ORS_strcmp(pdb_elem->name, name) && !ORS_strcmp(pdb_elem->section, section)) return pdb_elem;
	}
	return 0;
}

/*****************************************************************************
  NEW PROBE ID
						strdup(next_probe_id)
*****************************************************************************/
char *OS_create_new_probe_id(char *userpath) {
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends

	static char buffer[20];
	int next_probe_id;

	GBDATA *gb_id = GB_search(gb_probedb, "next_probe_id", GB_INT);		// creates field if not available
	next_probe_id = GB_read_int(gb_id);
	GB_write_int(gb_id, ++next_probe_id);
	sprintf(buffer,"%i", next_probe_id);

	return strdup(buffer);
}

/*****************************************************************************
  LIST OF PROBES
	type 1: probe_id, author_datum, probe_name, specifity ("my probes")
	type 2: probe_id, author, probe_name, specifity	("probes")
	type 10: field search like type 2
*****************************************************************************/

	/*****************************************************************************
	  SORT: COMPARE PROBE STRUCTURES by date by probe_name (format: dd.mm.yyyy)
	*****************************************************************************/
	int OS_probe_struct_date_cmp(struct OS_probe *elem2, struct OS_probe *elem1) {
		char *date1 = elem1->author_date;
		char *date2 = elem2->author_date;
		if (!date1 || !*date1) return strcmp("","bla");
		if (!date2 || !*date2) return strcmp("bla","");
		char *mon1  = strchr(date1,'.') + 1;
		char *mon2  = strchr(date2,'.') + 1;
		char *year1 = strchr(mon1+1,'.') + 1;
		char *year2 = strchr(mon2+1,'.') + 1;
		int result;

		result = strncmp(year1, year2, 4);
		if (result) return result;
		result = strncmp(mon1, mon2, 2);
		if (result) return result;
		result = strncmp(date1, date2, 2);
		if (result) return result;
		return strcasecmp(elem2->probe_name, elem1->probe_name);
	}
	/*****************************************************************************
	  SORT: COMPARE PROBE STRUCTURES by author by probe_name
	*****************************************************************************/
	int OS_probe_struct_author_cmp(struct OS_probe *elem1, struct OS_probe *elem2) {
		if (!elem1) return strcmp("","a");
		if (!elem2) return strcmp("a","");
		int result;
		result = strcmp(elem1->author, elem2->author);
		if (result) return result;
		return strcasecmp(elem1->probe_name, elem2->probe_name);
	}

bytestring *OS_list_of_probes(ORS_local *locs, char *userpath, int list_type, int list_count) {
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends

	static bytestring bs = {0,0};
	delete bs.data;

	GBDATA *gb_probe_field;
	char *read_data=0;
	GBDATA *gb_probe;
	GBDATA *gb_field;

	ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;
	OS_pdb_list *pdb_elem;

	const int MAX_SORT=500;
	struct OS_probe *probes[MAX_SORT];	// for sorting
	int num_probes=0;


	if (MSG) printf("\nOS_list_of_probes(%s, type=%i, count=%i)\n",userpath,list_type,list_count);
	void *file=GBS_stropen(10000);  // open memory file

	// all probes of same author (that is: userpath), sorted by author_date
	if (list_type == 1) {
		for (	gb_probe_field = GB_find(gb_probedb, "p_author", userpath, down_2_level);	//! author of probe (userpath)
			gb_probe_field;
			gb_probe_field = GB_find(gb_probe, "p_author", userpath, down_level | search_next) ) { // search 1 level down but parallel

			gb_probe = GB_get_father(gb_probe_field);

			// allowed to see existance?
			gb_field = GB_find(gb_probe,"p_pub_exist",0,down_level);
			delete read_data;
			read_data = GB_read_string(gb_field);
			if (!OS_read_access_allowed(locs->userpath, read_data)) continue;

			struct OS_probe *probe_struct = (struct OS_probe *)calloc(sizeof(struct OS_probe), 1);
			probes[num_probes++] = probe_struct;

			gb_field = GB_find(gb_probe,"probe_id",0,down_level);
			if (gb_field) probe_struct->probe_id 		= GB_read_string(gb_field);

			gb_field = GB_find(gb_probe,"p_author_date",0,down_level);		//! creation date of this probe
			if (gb_field) probe_struct->author_date 	= GB_read_string(gb_field);

			gb_field = GB_find(gb_probe,"p_probe_name",0,down_level);		//! name of this probe
			if (gb_field) probe_struct->probe_name 	= GB_read_string(gb_field);

			gb_field = GB_find(gb_probe,"p_probe_info",0,down_level);		//! short description of probe
			if (gb_field) probe_struct->probe_info 	= GB_read_string(gb_field);

			if (num_probes > MAX_SORT) break;
		}

		// sort probes by date
		GBT_quicksort( (void **)probes, 0, num_probes, (long (*)(void *, void *, char *cd )) OS_probe_struct_date_cmp, 0);

		// create list for cgi client
		int nr;
		for (nr=0; nr<num_probes; nr++) {
			if (list_count && nr >= list_count) break;

			GBS_strcat(file, probes[nr]->probe_id);
			GBS_chrcat(file, 1);
			GBS_strcat(file, probes[nr]->author_date);
			GBS_chrcat(file, 1);
			GBS_strcat(file, probes[nr]->probe_name);
			GBS_chrcat(file, 1);
			GBS_strcat(file, probes[nr]->probe_info);
			GBS_chrcat(file, 1);

			// free memory!
			delete probes[nr]->probe_id;
			delete probes[nr]->author_date;
			delete probes[nr]->probe_name;
			delete probes[nr]->probe_info;
			delete probes[nr];
		}
	}

	// all probes of same author (that is: userpath), sorted by author_date
	else if (list_type == 2) {
		for (	gb_probe_field = GB_find(gb_probedb, "p_author", "*", down_2_level);	//! author of probe (userpath)
			gb_probe_field;
			gb_probe_field = GB_find(gb_probe, "p_author", "*", down_level | search_next) ) { // search 1 level down but parallel

			gb_probe = GB_get_father(gb_probe_field);

			// allowed to see existance?
			gb_field = GB_find(gb_probe,"p_pub_exist",0,down_level);
			delete read_data;
			read_data = GB_read_string(gb_field);
			if (!OS_read_access_allowed(locs->userpath, read_data)) continue;

			struct OS_probe *probe_struct = (struct OS_probe *)calloc(sizeof(struct OS_probe), 1);
			probes[num_probes++] = probe_struct;

			gb_field = GB_find(gb_probe,"probe_id",0,down_level);
			if (gb_field) probe_struct->probe_id 		= GB_read_string(gb_field);

			gb_field = GB_find(gb_probe,"p_author",0,down_level);			//! author of this probe
			if (gb_field) probe_struct->author 		= GB_read_string(gb_field);

			gb_field = GB_find(gb_probe,"p_probe_name",0,down_level);
			if (gb_field) probe_struct->probe_name 	= GB_read_string(gb_field);

			gb_field = GB_find(gb_probe,"p_probe_info",0,down_level);
			if (gb_field) probe_struct->probe_info 	= GB_read_string(gb_field);

			if (num_probes > MAX_SORT) break;
		}

		// sort probes by date
		GBT_quicksort( (void **)probes, 0, num_probes, (long (*)(void *, void *, char *cd )) OS_probe_struct_author_cmp, 0);

		// create list for cgi client
		int nr;
		for (nr=0; nr<num_probes; nr++) {
			if (list_count && nr >= list_count) break;

			GBS_strcat(file, probes[nr]->probe_id);
			GBS_chrcat(file, 1);
			GBS_strcat(file, probes[nr]->author);
			GBS_chrcat(file, 1);
			GBS_strcat(file, probes[nr]->probe_name);
			GBS_chrcat(file, 1);
			GBS_strcat(file, probes[nr]->probe_info);
			GBS_chrcat(file, 1);

			// free memory!
			delete probes[nr]->probe_id;
			delete probes[nr]->author;
			delete probes[nr]->probe_name;
			delete probes[nr]->probe_info;
			delete probes[nr];
		}
	}


	// FIELD SEARCH

	else if (list_type == 10){
		int found, found_any;

		if (MSG) {
			printf("OS_list_of_probes[search]:\n");
			for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
				if (pdb_elem->content)
				printf("	%s='%s'\n",pdb_elem->db_name,pdb_elem->content);
			}
		}

		int search_any_only=1;	// no content field?
		for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
			if (pdb_elem->content && *(pdb_elem->content)) { search_any_only=0; break; }
		}
		if (MSG) printf("OS_list_of_probes[search]: search_any_only=%i\n",search_any_only);

		for (	gb_probe = GB_find(gb_probedb, "probe", 0, down_level);
			gb_probe;
			gb_probe = GB_find(gb_probe, "probe", 0, this_level | search_next) ) { // search 1 level down but parallel

			gb_field = GB_find(gb_probe,"p_pub_exist",0,down_level);
			delete read_data;
			read_data = GB_read_string(gb_field);
			if (!OS_read_access_allowed(locs->userpath, read_data)) continue;

			if (search_any_only) {
				found_any=0;
				if (locs->search_any_field && *locs->search_any_field)
				for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
					if (!pdb_elem->content) continue;	// only search fields of html page
					if (	!ORS_strcmp(pdb_elem->name,"p_last_mod") ||
						!ORS_strcmp(pdb_elem->name,"p_author_date") ||
						!ORS_strcmp(pdb_elem->name,"p_author_time") ) continue;
					if (GB_find(gb_probe, pdb_elem->db_name, locs->search_any_field, down_level)) { found_any=1; break; };
				}
				if (!found_any) continue;
			} else {

				// match search fields
				if (search_any_only) found=0;
						else found=1;
				found_any=0;
				for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
					if (!pdb_elem->content) continue;	// only search fields of html page
					if (	!ORS_strcmp(pdb_elem->name,"p_last_mod") ||
						!ORS_strcmp(pdb_elem->name,"p_author_date") ||
						!ORS_strcmp(pdb_elem->name,"p_author_time") ) continue;
					if (locs->search_any_field && *locs->search_any_field &&
						GB_find(gb_probe, pdb_elem->db_name, locs->search_any_field, down_level)) { found_any=1; };
					if (!pdb_elem->content || !*(pdb_elem->content)) continue;
					if (!GB_find(gb_probe,pdb_elem->db_name,pdb_elem->content,down_level)) { found=0; break; };
				}
				if (!found) continue;
				if (locs->search_any_field && *locs->search_any_field && !found_any) continue;
			}


			struct OS_probe *probe_struct = (struct OS_probe *)calloc(sizeof(struct OS_probe), 1);
			probes[num_probes++] = probe_struct;

			gb_field = GB_find(gb_probe,"probe_id",0,down_level);
			if (gb_field) probe_struct->probe_id 		= GB_read_string(gb_field);

			gb_field = GB_find(gb_probe,"p_author",0,down_level);			//! author of this probe
			if (gb_field) probe_struct->author 		= GB_read_string(gb_field);

			gb_field = GB_find(gb_probe,"p_probe_name",0,down_level);
			if (gb_field) probe_struct->probe_name 	= GB_read_string(gb_field);

			gb_field = GB_find(gb_probe,"p_probe_info",0,down_level);
			if (gb_field) probe_struct->probe_info 	= GB_read_string(gb_field);

			if (num_probes > MAX_SORT) break;
		}

		// sort probes by date
		GBT_quicksort( (void **)probes, 0, num_probes, (long (*)(void *, void *, char *cd )) OS_probe_struct_author_cmp, 0);

		// create list for cgi client
		int nr;
		for (nr=0; nr<num_probes; nr++) {
			if (list_count && nr >= list_count) break;

			if (!probes[nr]->probe_name) probes[nr]->probe_name=strdup("");
			if (!probes[nr]->probe_info) probes[nr]->probe_info=strdup("");

			GBS_strcat(file, probes[nr]->probe_id);
			GBS_chrcat(file, 1);
			GBS_strcat(file, probes[nr]->author);
			GBS_chrcat(file, 1);
			GBS_strcat(file, probes[nr]->probe_name);
			GBS_chrcat(file, 1);
			GBS_strcat(file, probes[nr]->probe_info);
			GBS_chrcat(file, 1);

			// free memory!
			delete probes[nr]->probe_id;
			delete probes[nr]->author;
			delete probes[nr]->probe_name;
			delete probes[nr]->probe_info;
			delete probes[nr];
		}
	}
	// if (!gb_field) return leerer bytestring;

	bs.data = GBS_strclose(file);
	bs.size = strlen(bs.data)+1;
	return &bs;

}


/*****************************************************************************
  CHANGE USERPATH
		userpath is being changed
		all probes of this user have to be changed
*****************************************************************************/
//void OS_probe_change_of_userpath(char *old_userpath, char *new_userpath) {
	// TODO: funktion wird gebraucht!
//}

/*****************************************************************************
  READ ACCESS allowed
		return 1 if user is allowed to read probe, otherwise 0
*****************************************************************************/
int OS_read_access_allowed(char *userpath, char *publicity) {

	return (ORS_is_parent_or_equal(userpath, publicity) || ORS_is_parent_of(publicity, userpath));
}

/*****************************************************************************
  VALIDATE OWNERS
		mode="create":	fields have been created
		mode="update":	fields have been updated (only test changes)
			return 1 if owner updates are ok, otherwise 0
			sets locs->error message
*****************************************************************************/
int OS_owners_of_pdb_list_allowed(ORS_local *locs, char *mode) {
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends
	OS_pdb_list *pdb_elem;
	static char *read_data=0;
	ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;

	//int create = (!ORS_strcmp(mode,"create"));
	int update = (!ORS_strcmp(mode,"update"));

	GBDATA *gb_probe_id;
	GBDATA *gb_probe;
	if (update) {
		gb_probe_id = GB_find(gb_probedb,"probe_id",locs->probe_id,down_2_level);
		if (!gb_probe_id) return 0;
		gb_probe = GB_get_father(gb_probe_id);
	}

	for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
		if (!pdb_elem->content) continue;
		if (ORS_strcmp(pdb_elem->name, "p_owner")) continue;	// only test owner fields
		if (update) {
			delete read_data;
			read_data = OS_read_gb_probe_info_string(gb_probe, pdb_elem->db_name);
			if (!ORS_strcmp(read_data, pdb_elem->content)) continue;	// data was not changed
		}
		if (!ORS_is_parent_or_equal(locs->userpath, pdb_elem->content)) {	// am I privileged?
			delete locs->error;
			locs->error = strdup(ORS_export_error(
					"you are not allowed to set user '%s' as owner (you are not his parent user).", pdb_elem->content));
			return 0;
		}
		if (!OS_user_exists_in_userdb(pdb_elem->content)) {			// and user exists?
			delete locs->error;
			locs->error = strdup(ORS_export_error("owner '%s' does not exist in user database.", pdb_elem->content));
			return 0;
		}
	}
	return 1;
}

/*****************************************************************************
  AM I OWNER OF THIS PROBE?
			return 1 if userpath is owner or superuser, otherwise 0
			sets locs->error message
*****************************************************************************/
int OS_owner_of_probe(ORS_local *locs) {
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends
	OS_pdb_list *pdb_elem;
	static char *read_data=0;

	ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;

	GBDATA *gb_probe_id = GB_find(gb_probedb,"probe_id",locs->probe_id,down_2_level);
	if (!gb_probe_id) return 0;

	GBDATA *gb_probe = GB_get_father(gb_probe_id);
	//GBDATA *gb_field;
	int is_superuser = OS_read_user_info_int(locs->userpath, "is_superuser");

	for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
		if (strcmp(pdb_elem->name, "p_owner")) continue;		// test only owner fields
		delete read_data;
		read_data = OS_read_gb_probe_info_string(gb_probe, pdb_elem->db_name);

		// if (!ORS_is_parent_or_equal(locs->userpath, pdb_elem->content)) {	// am I privileged?
		if (!is_superuser && ORS_strcmp(locs->userpath, pdb_elem->content)) {	// am I privileged?
			delete locs->error;
			locs->error = strdup("you are not owner of this probe.");
			return 0;
		}
	}
	return 1;
}

/*****************************************************************************
  NUMBER OF PROBES OF A USER
							return total
*****************************************************************************/
int OS_count_probes_of_author(char *userpath) {
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends

	GBDATA *gb_probe_field;
	GBDATA *gb_probe;
	int	total=0;

	// search all probes
	for (	gb_probe_field = GB_find(gb_probedb, "p_author", userpath, down_2_level);
		gb_probe_field;
		gb_probe_field = GB_find(gb_probe, "p_author", userpath, down_level | search_next) ) { // search 1 level down but parallel

		gb_probe = GB_get_father(gb_probe_field);

		total++;
	}
	return total;
}

/************************************************************
  TRANSFER PROBES of one user to another
			or only one probe if probe_id is set
			transfer from sel_userpath to string
*************************************************************/
void OS_probe_user_transfer(ORS_local *locs, char *from_userpath, char *to_userpath, char *probe_id) {
	GB_transaction dummy(gb_probedb);  // keep transaction open until var scope ends

	GBDATA *gb_probe_field;
	GBDATA *gb_probe;
	static char *read_data;

	ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;
	OS_pdb_list *pdb_elem;

	// change authors
	if (!ORS_strcmp(probe_id, "*")) {	// ...of all probes
		for (	gb_probe_field = GB_find(gb_probedb, "p_author", from_userpath, down_2_level);
			gb_probe_field;
			gb_probe_field = GB_find(gb_probe, "p_author", from_userpath, down_level | search_next) ) { // search 1 level down but parallel

			gb_probe = GB_get_father(gb_probe_field);
			OS_write_gb_probe_info_string(gb_probe, "p_author", to_userpath);
		}
	} else {				// ...of one probe
		gb_probe_field = GB_find(gb_probedb,"probe_id", probe_id, down_2_level);
		gb_probe = GB_get_father(gb_probe_field);
		delete read_data;
		read_data = OS_read_gb_probe_info_string(gb_probe, "p_author");
		if (!ORS_strcmp(read_data, from_userpath)) {
			OS_write_gb_probe_info_string(gb_probe, "p_author", to_userpath);
		}
	}
	// change owners of all sections!
	for (	gb_probe_field = GB_find(gb_probedb, "probe_id", probe_id, down_2_level);
		gb_probe_field;
		gb_probe_field = GB_find(gb_probe, "probe_id", probe_id, down_level | search_next) ) { // search 1 level down but parallel

		gb_probe = GB_get_father(gb_probe_field);


		for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
			if (strcmp(pdb_elem->name, "p_owner")) continue;		// find only owner fields
			delete read_data;
			read_data = OS_read_gb_probe_info_string(gb_probe, pdb_elem->db_name);
			if (!ORS_strcmp(read_data, from_userpath)) {
				OS_write_gb_probe_info_string(gb_probe, pdb_elem->db_name, to_userpath);
			}
		}
	}

	return;

}
