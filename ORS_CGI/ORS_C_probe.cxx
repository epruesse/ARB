/*
#################################
#                               #
#    ORS_CLIENT:  PROBE         #
#    probe functions            #
#                               #
################################# */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>
#include <arbdb.h>

#include <ors_client.h>
#include <client.h>
#include <servercntrl.h>

#include <cat_tree.hxx>
#include "ors_lib.h"
#include "ors_c.hxx"
#include "ors_c_proto.hxx"

/*********************************************************
  SAVE PROBEDB
**********************************************************/
void OC_save_probedb(void) {

	char *locs_error = 0;

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW,		ors_gl.dailypw,
		LOCAL_REMOTE_HOST, 	ors_gl.remote_host,
		LOCAL_SAVE_PROBEDB,	"",
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (save_probedb)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR       , &locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (save_probedb2)"));
	}
	OC_server_error_if_not_empty(locs_error);
}

/*********************************************************
  PUT PROBE FIELD
	send probe data field to server
**********************************************************/
void OC_put_probe_field(char *field_section, char *field_name, char *field_data) {

	char *locs_error = 0;
	static bytestring bs = {0,0};
	delete bs.data;

	static char empty_str[1];
	empty_str[0] = 0;
	if (!field_section) 	field_section = empty_str;
	if (!field_name) 	field_name = empty_str;
	if (!field_data) 	field_data = empty_str;

	bs.data = strdup(field_data);
	bs.size = strlen(bs.data)+1;

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_PROBE_FIELD_DATA,		bs,
		LOCAL_PROBE_FIELD_SECTION,	field_section,
		LOCAL_PUT_PROBE_FIELD,		field_name,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (put_probe_field:%s/%s)",field_section,field_name));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR       , &locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (put_probe_field2:%s/%s)",field_section,field_name));
	}
	OC_server_error_if_not_empty(locs_error);
}
/*********************************************************
  PUT PROBE FIELD TA_ID
	send probe data field to server
**********************************************************/
void OC_put_probe_ta_id(int ta_id) {

	char *locs_error = 0;

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_PROBE_TA_ID,	ta_id,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (put_probe_ta_id)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR       , &locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (put_probe_ta_id2)"));
	}
	OC_server_error_if_not_empty(locs_error);
}

/*********************************************************
  GET PROBE FIELD (of a pre-selected probe)
	get probe data field from server
**********************************************************/
char *OC_get_probe_field(char *field_section, char *field_name) {

	char *locs_error = 0;
	char *field_data;
	static bytestring bs = {0,0};
	delete bs.data;

	static char *empty_str="";
	if (!field_section) field_section = empty_str;

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_PROBE_FIELD_SECTION,	field_section,
		LOCAL_PROBE_FIELD_NAME,		field_name,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (get_probe_field:%s/%s)",field_section,field_name));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR, 			&locs_error,
		LOCAL_GET_PROBE_FIELD,		&bs,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (get_probe_field2:%s/%s)",field_section,field_name));
	}
	OC_server_error_if_not_empty(locs_error);
	return bs.data;
}

/*********************************************************
  GET PROBE FIELD TA_ID (of a pre-selected probe)
	get probe data field from server
**********************************************************/
int OC_get_probe_ta_id(void) {

	char *locs_error = 0;
	int field_data;

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR, 			&locs_error,
		LOCAL_PROBE_TA_ID,		&field_data,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (get_probe_ta_id)"));
	}
	OC_server_error_if_not_empty(locs_error);
	return field_data;
}

/*********************************************************
  WORK ON PROBE
	create/modify/delete probe (data was sent by ^)
**********************************************************/
void OC_work_on_probe(char *action) {

	char *locs_error = 0;
	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW,		ors_gl.dailypw,
		LOCAL_REMOTE_HOST, 	ors_gl.remote_host,
		LOCAL_WORK_ON_PROBE,		action,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (work_on_probe:%s)",action));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR       , &locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (work_on_probe2:%s)",action));
	}
	OC_server_error_if_not_empty(locs_error);

	ors_gl.message = new char[256];
	if (!strcmp(action,"create"))		sprintf(ors_gl.message,"Your probe has been created, %s.",ors_gl.userpath);
	else sprintf(ors_gl.message,"Probe has been %sd.",action);
  	OC_output_html_page("success");
  	exit(0);
}

/*********************************************************
  SELECT PROBE
	select probe including sending of 3 data fields
	if more fields are required, use put_probe_field before
**********************************************************/
void OC_probe_select(char *probe_id ) {

	char *locs_error = 0;
	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW,		ors_gl.dailypw,
		LOCAL_REMOTE_HOST, 	ors_gl.remote_host,
		LOCAL_PROBE_SELECT,	probe_id,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (select_probe)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR, &locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (select_probe2)"));
	}
	OC_server_error_if_not_empty(locs_error);
}

/*********************************************************
  PROBE QUERY
	probe query including sending of 3 data fields
	if more fields are required, use put_probe_field before
**********************************************************/
char * OC_probe_query(int max_count) {

	static bytestring result = {0,0};
	delete result.data;

	char *locs_error = 0;
	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW,			ors_gl.dailypw,
		LOCAL_REMOTE_HOST, 		ors_gl.remote_host,
		LOCAL_PROBE_LIST_TYPE,		10,
		LOCAL_PROBE_LIST_MAX_COUNT,  	max_count,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (probe_query)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_PROBE_QUERY,	&result,
		LOCAL_ERROR, 		&locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (probe_query2)"));
	}
	OC_server_error_if_not_empty(locs_error);

	return strdup(result.data);
}

/*********************************************************
  GET PROBE LIST
**********************************************************/
char * OC_get_probe_list(int type, int count) {

	char *locs_error = 0;
	static bytestring result = {0,0};
	delete result.data;

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_PROBE_LIST_TYPE,		type,
		LOCAL_PROBE_LIST_MAX_COUNT,	count,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (get_probe_list)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_GET_PROBE_LIST,	&result,
		LOCAL_ERROR, 		&locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (get_probe_list2)"));
	}
	OC_server_error_if_not_empty(locs_error);

	return result.data;
}

/*****************************************************************************
  CREATE NEW PDB-LIST ELEMENT
*****************************************************************************/
OC_pdb_list *OC_create_pdb_list_elem(void) {

	OC_pdb_list *THIS		= (OC_pdb_list *)calloc(sizeof(OC_pdb_list), 1);
	if(!THIS) quit_with_error(ORS_export_error("Malloc error in OC_create_pdb_list_elem!"));
	return THIS;
}

/*****************************************************************************
  INSERT FIELD IN PDB-LIST (insert elem into list "head")
*****************************************************************************/
void OC_pdb_list_insert(OC_pdb_list **head, OC_pdb_list *elem) {

	OC_pdb_list *temp;

	if (!*head) { // empty list
		*head = elem;
		elem->next = elem;
		elem->prev = elem;
	} else {
		temp = (*head)->prev;  // last elem
		temp->next = elem;
		elem->prev = temp;
		elem->next = *head;
		(*head)->prev = elem;
	}
}

/*****************************************************************************
  DELETE FIELD FROM PDB-LIST
*****************************************************************************/
void OC_pdb_list_delete(OC_pdb_list *head, OC_pdb_list *elem) {

	OC_pdb_list *temp;

	if (!head) return;
	if (elem->next == elem) {
		delete elem;
		head = 0;
	} else {
		if (head == elem) { // first elem
			head = elem->next;
			delete elem;
		} else {
			elem->prev->next = elem->next;
			delete elem;
		}
	}
}

/*****************************************************************************
  POINTER TO NEXT ELEMENT
*****************************************************************************/
OC_pdb_list *OC_next_pdb_list_elem(OC_pdb_list *head, OC_pdb_list *elem) {

	if (elem->next == head) return 0;
	else return elem->next;
}

/*****************************************************************************
  POINTER TO NEXT ELEMENT (special for ors_gl.pdb_list
*****************************************************************************/
OC_pdb_list *OC_next_ors_gl_pdb_list_elem(OC_pdb_list *elem) {

	if (elem->next == ors_gl.pdb_list) return 0;
	else return elem->next;
}

/*****************************************************************************
  FIND PDB ELEMENT BY NAME
						return 0 if not found
*****************************************************************************/
OC_pdb_list *OS_find_pdb_list_elem_by_name(OC_pdb_list *head, char *section, char *name) {
	OC_pdb_list *pdb_elem;

	for (pdb_elem = head; pdb_elem; pdb_elem = OC_next_pdb_list_elem(head, pdb_elem)) {
		if (!ORS_strcmp(pdb_elem->name, name) && !ORS_strcmp(pdb_elem->section, section)) return pdb_elem;
	}
	return 0;
}


/*****************************************************************************
  READ PROBEDB FIELDS from ors_lib
			probe fields are set to "p_xxxx"
					returns error message or NIL
*****************************************************************************/
void OC_read_probedb_fields_into_pdb_list(void) {
	static char buffer[256];
	static char buffer2[100];
	FILE 	*file;
	char	*p;
	char 	*tmp;
	OC_pdb_list *pdb_list_elem;
	int 	line=0;

	if (ors_gl.pdb_list) return;	// is already filled!

	file = fopen(ORS_LIB_PATH "probedb_fields","r");
	if (!file) quit_with_error(ORS_export_error("Can't open file: " ORS_LIB_PATH "probedb_fields"));

	for (	p=fgets(buffer,255,file);
		p;
		p = fgets(p,255,file)) {

		line++;

		if (buffer[0] != '#' && ORS_str_char_count(buffer, ',') > 0) {
			if (ORS_str_char_count(buffer, ',') != 8) {				// <--- number of fields!
				quit_with_error(ORS_export_error("Wrong number of fields in line %i of " ORS_LIB_PATH "probedb_fields", line));
			}
			pdb_list_elem = OC_create_pdb_list_elem();

			tmp = strtok(buffer,",");	// extract section/name or name
			if (strchr(tmp,'/')) {
				pdb_list_elem->section = strdup(strtok(tmp,"/"));
				tmp+=strlen(tmp)+1;
			} else {
				pdb_list_elem->section = 0;
			}
			sprintf(buffer2,"p_%s",tmp);
			pdb_list_elem->name    = strdup(buffer2);

			if (pdb_list_elem->section) {		// create db_name
				sprintf(buffer2,"%s__%s",pdb_list_elem->section, pdb_list_elem->name);
				pdb_list_elem->db_name = strdup(buffer2);
			} else {
				pdb_list_elem->db_name = strdup(pdb_list_elem->name);
			}

			if (tmp) tmp+=strlen(tmp)+1;
			pdb_list_elem->type 		= strdup(ORS_trim(strtok(tmp,",")));
			if (tmp) tmp+=strlen(tmp)+1;
			pdb_list_elem->width 		= atoi(strtok(tmp,","));
			if (tmp) tmp+=strlen(tmp)+1;
			pdb_list_elem->height 		= atoi(strtok(tmp,","));
			if (tmp) tmp+=strlen(tmp)+1;
			pdb_list_elem->rights 		= strdup(ORS_trim(strtok(tmp,",")));
			if (tmp) tmp+=strlen(tmp)+1;
			pdb_list_elem->init 		= strdup(ORS_trim(strtok(tmp,",")));
			if (tmp) tmp+=strlen(tmp)+1;
			pdb_list_elem->label 		= strdup(ORS_trim(strtok(tmp,",")));
			if (tmp) tmp+=strlen(tmp)+1;
			pdb_list_elem->help_page 	= strdup(ORS_trim(strtok(tmp,",")));
			if (tmp) tmp+=strlen(tmp)+1;
			pdb_list_elem->features 	= strdup(ORS_trim(strtok(tmp,",")));
			pdb_list_elem->content		= 0;

			// printf("name=%s, type=%s, width=%i <BR>",pdb_list_elem->name, pdb_list_elem->type, pdb_list_elem->width);

			OC_pdb_list_insert(&ors_gl.pdb_list, pdb_list_elem); 	// put element into list
		}
	}
	fclose(file);
}

/*****************************************************************************
	CLEAR ALL PROBEDB FIELD CONTENTS in server
*****************************************************************************/
void OC_clear_pdb_fields_in_server(void) {
	char *locs_error = 0;
	char *dummy;

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_CLEAR_PROBE_FIELDS,	&dummy,
		LOCAL_ERROR, 			&locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (clear_pdb_fields)"));
	}
	OC_server_error_if_not_empty(locs_error);
}

/*****************************************************************************
	SEND PROBEDB FIELD CONTENTS to server
*****************************************************************************/
void OC_send_pdb_fields_to_server(char *selection) {

	OC_pdb_list *pdb_elem;

	if (!strcmp(selection, "entered")) OC_clear_pdb_fields_in_server();	// remove old contents

	OC_put_probe_ta_id(ors_gl.probe_ta_id);

	if (ors_gl.search_any_field) 	OC_put_probe_field("", "search_any_field", ors_gl.search_any_field);

	for (pdb_elem = ors_gl.pdb_list; pdb_elem; pdb_elem = OC_next_pdb_list_elem(ors_gl.pdb_list, pdb_elem)) {
		// if (!strcmp(selection, "create") && pdb_elem->section) continue;
		if (strchr(pdb_elem->rights,'V')) continue; // virtual field
		if (!strcmp(selection, "entered") && (pdb_elem->content == 0)
// || *(pdb_elem->content) == 0)
) continue;
		OC_put_probe_field(pdb_elem->section, pdb_elem->name, pdb_elem->content);
	}
}

/*****************************************************************************
	GET PROBEDB FIELD CONTENTS from server
*****************************************************************************/
void OC_get_pdb_fields_from_server(char *probe_id, char *selection) {

	OC_pdb_list *pdb_elem;
	int create = (!strcmp(selection, "create"));
	int clone  = (!strcmp(selection, "clone"));

	OC_probe_select(probe_id);

	ors_gl.probe_ta_id = OC_get_probe_ta_id();
	for (pdb_elem = ors_gl.pdb_list; pdb_elem; pdb_elem = OC_next_pdb_list_elem(ors_gl.pdb_list, pdb_elem)) {
		if (create && pdb_elem->section) continue;
		if (strchr(pdb_elem->rights,'V')) continue; // virtual field
		if (clone && (	!strcmp(pdb_elem->name,"p_sequence") 	||
				!strcmp(pdb_elem->name,"p_target_seq") 	||
				!strcmp(pdb_elem->name,"p_4gc_2at") 	) 	) continue;
		pdb_elem->content = strdup(OC_get_probe_field(pdb_elem->section, pdb_elem->name));
	}
}

/*****************************************************************************
	GET PROBEDB FIELD CONTENTS from cgi vars
*****************************************************************************/
void OC_get_pdb_fields_from_cgi_vars(char *selection) {

	static char buffer[60];
	OC_pdb_list *pdb_elem;

	for (pdb_elem = ors_gl.pdb_list; pdb_elem; pdb_elem = OC_next_pdb_list_elem(ors_gl.pdb_list, pdb_elem)) {
		// if (!strcmp(selection, "create") && pdb_elem->section) continue;

		sprintf(buffer,"%s|sel_%s",pdb_elem->db_name, pdb_elem->db_name);	// manual and choose field
		pdb_elem->content = cgi_var_or_nil(buffer);
		if (ors_gl.debug && pdb_elem->content) printf("CGI_VAR(%s)=%s<BR>\n",pdb_elem->db_name,pdb_elem->content);
	}
	ors_gl.search_any_field = cgi_var("search_any_field");	//! search field to search across all data fields
}

/*****************************************************************************
	HTML LIST OF USER's PROBES
*****************************************************************************/
void OC_output_html_probe_list(int type, char *headline) {
	char *col_param;
	char *title;

	if      (!ORS_strcmp(ors_gl.action,"update"))	col_param="EDIT";
	else if (!ORS_strcmp(ors_gl.action,"clone"))	col_param="CLONE";
	else if (!ORS_strcmp(ors_gl.action,"delete"))	col_param="DELETE";
	else						col_param="VIEW";

	if (type == 1) {				// list = (probe_id-link, date, int. name, probe name)*
		OC_output_html_table table;
		table.create();
		table.border();
		//table.invisible_list_field(1);
		table.set_col_func(1, OC_html_probe_id_link, 1);
		table.set_col_param(1, col_param);
		if (!ORS_strcmp(ors_gl.action,"delete")) {
			table.set_col_func(2, OC_html_probe_id_link, 1);
			table.set_col_param(2, "VIEW");
			title = ", ,date,int. name,probe name";
		}
		else title = ",date,int. name,probe name";
		table.output(ors_gl.list_of_probes, 4, title, headline);

	}
	else if (type == 2) {				// list = (probe_id-link, author, int. name, probe name)*
		OC_output_html_table table;
		table.create();
		table.border();
		table.set_col_func(1, OC_html_probe_id_link);
		table.set_col_param(1, col_param);
		if (!ORS_strcmp(ors_gl.action,"delete") || !ORS_strcmp(ors_gl.action,"clone")) {
			table.set_col_func(2, OC_html_probe_id_link, 1);
			table.set_col_param(2, "VIEW");
			table.set_col_func(3, OC_html_www_home, 3, 2);
			title = ", ,author,int. name,probe name";
		} else {
			table.set_col_func(2, OC_html_www_home, 2, 2);
			title = ",author,int. name,probe name";
		}
		table.output(ors_gl.list_of_probes, 4, title, headline);

	}
	else if (type == 10) {				// list = (probe_id, author, int. name, probe name)*
		OC_output_html_table table;
		table.create();
		table.border();
		table.set_col_func(1, OC_html_probe_id_link);
		table.set_col_param(1, col_param);
		if (!ORS_strcmp(ors_gl.action,"delete") || !ORS_strcmp(ors_gl.action,"clone")) {
			table.set_col_func(2, OC_html_probe_id_link, 1);
			table.set_col_param(2, "VIEW");
			table.set_col_func(3, OC_html_www_home, 3, 2);
			title = ", ,author,int. name,probe name";
		} else {
			table.set_col_func(2, OC_html_www_home, 2, 2);
			title = ",author,int. name,probe name";
		}
		table.output(ors_gl.list_of_probes, 4, title, headline);

	}
	else quit_with_error(ORS_export_error("probe list table type %i is not implemented",type));
}

/*****************************************************************************
  READ FILE into list
							return a list
*****************************************************************************/
char * OC_read_file_into_list(char *file_name) {
	static char buffer[256];
	FILE 	*file;
	char	*p;
	int	first_time=1;
	void *mem_file=GBS_stropen(10000);  // open memory file

	file = fopen(file_name,"r");
	if (!file) quit_with_error(ORS_export_error("Can't open file: %s", file_name));

	for (	p=fgets(buffer,255,file);
		p;
		p = fgets(p,255,file)) {
		if (buffer[0] == '#' || buffer[0] == 0 || buffer[0] == '\n') continue;

		if (!first_time) GBS_chrcat(mem_file, 1);
		first_time=0;
		if (buffer[strlen(buffer) - 1] == '\n') buffer[strlen(buffer) - 1] = 0;
		GBS_strcat(mem_file, buffer);
	}
	fclose(file);
	return GBS_strclose(mem_file);
}

/*********************************************************
  TRANSFER PROBES from one user to another
				probe_id not used yet
**********************************************************/
void OC_probe_user_transfer(char *from_userpath, char *to_userpath, char *probe_id) {

	char *locs_error = 0;
	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW,			ors_gl.dailypw,
		LOCAL_REMOTE_HOST, 		ors_gl.remote_host,
		LOCAL_SEL_USERPATH, 		from_userpath,
		LOCAL_PROBE_USER_TRANSFER,	to_userpath,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (probe_transfer)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR, &locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (probe_transfer2)"));
	}
	OC_server_error_if_not_empty(locs_error);
}
