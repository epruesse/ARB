/* 
###################################
#                                 #
#   ORS_SERVER: CLIENT FUNCTIONS  #
#   Functions, invoked by Client  #
#                                 #
################################### */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ors_server.h>
#include <ors_client.h>
#include "ors_s.h"
#include <arbdb.h>
#include <ors_prototypes.h>
#include <server.h>
#include <ors_lib.h>

#include <client.h>
#include <servercntrl.h>
#include <struct_man.h>
    
#include "ors_s_common.hxx"
#include "ors_s_proto.hxx"



/*********************************************************
  Client Function: STORE QUERY
		return query# if user's access is ok 
		return 0 if not
**********************************************************/
int store_query(ORS_local *locs) {
	char *userpath;
	userpath=OS_find_dailypw(locs);
	if (!*userpath) return 0;

	return 0;
}

/*********************************************************
  Client Function: LOGIN
		return dailypw if user's access is ok 
**********************************************************/
extern "C" aisc_string calc_dailypw(ORS_local *locs) {
	static aisc_string dailypw = 0;
	char root_password[7];
	root_password[1]='a';
	root_password[3]='w';
	root_password[5]='0';
	root_password[4]=root_password[5]+1;
	root_password[0]='o';
	root_password[2]=root_password[0]+1;
	root_password[6]=0;
	

	if (!locs->userpath)    return "";   // must be save: no NULL strings!
	if (!locs->password)    return "";
	if (!locs->remote_host) return "";
	if (!locs->remote_user) return "";

	//  TODO:      - evtl: pruefen, ob der host vom user aus erlaubt ist
	//                (Host-Liste des Benutzers, von der aus er darf)
	// wenn nicht:	return "";

	//delete dailypw;
	dailypw = gen_dailypw( locs->debug );

	if (strcasecmp(locs->userpath,"root") || strcmp(locs->password,root_password)) {
		char *userpath = OS_find_user_and_password(locs->userpath, locs->password);
		delete locs->userpath;
		locs->userpath = userpath;
		
		if (!userpath) {
			write_logfile(locs,"wrong password");
			return "";
		}
		locs->username = OS_read_user_info_string(locs->userpath, "username");	// to have data ready for mainmenu page
		locs->www_home = OS_read_user_info_string(locs->userpath, "www_home");
	}
	write_logfile(locs, "login");

	ORS_main *pm = (ORS_main *)locs->mh.parent->parent;

	// user login: create new entry 
	struct passwd_struct *pws = (struct passwd_struct *)calloc(sizeof(struct passwd_struct),1);
	{
		pws->userpath    = strdup(locs->userpath);
		pws->remote_host = strdup(locs->remote_host);
		pws->login_time  = GB_time_of_day();		
	}

	GBS_write_hash((GB_HASH*)pm->pwds,dailypw,(long)pws);

	return dailypw;
}
/************************************************************
  Client Function: LOGOUT
*************************************************************/
extern "C" int logout_user(ORS_local *locs, char *dailypw) {

	ORS_main *pm = (ORS_main *)locs->mh.parent->parent;

	struct passwd_struct *pws = (struct passwd_struct *)GBS_read_hash((GB_HASH*)pm->pwds,dailypw);
	if (!pws) {
		delete locs->error;
		locs->error=strdup("Your daily password is not valid. You are already logged out!");
		return 1;
	}

	locs->userpath = pws->userpath;
	write_logfile(locs, "logout");

// TODO: sinnvolles Entfernen aus der hashtabelle
	pws->userpath = strdup("");
//	delete pws;
//	pws=0;
	return 0;
}


/*********************************************************
  Client Function: READ USER FIELD 
**********************************************************/
extern "C" aisc_string read_user_field(ORS_local *locs) {
	// must be save: no NULL strings!
	if (!locs->fieldname) {
		delete locs->error;
		locs->error=strdup("fieldname NULL");
		return "";
	}    
	if (!locs->sel_userpath) {
		delete locs->error;
		locs->error=strdup("sel_userpath NULL");
		return "";
	}
	if (!locs->dailypw) {
		delete locs->error;
		locs->error=strdup("dailypw NULL");
		return "";
	}

	char *userpath;
	userpath=OS_find_dailypw(locs);
	if (!*userpath) return "";

	if (!locs->is_superuser && !ORS_is_parent_or_equal(userpath,locs->sel_userpath)
				&& strcmp(locs->fieldname,"www_home")		// exceptions: these fields are always allowed
				&& strcmp(locs->fieldname,"mail_addr")
				&& strcmp(locs->fieldname,"username")
								) {
		delete locs->error;
		locs->error=strdup(ORS_export_error("You are not authorisized to read data of user '%s'",locs->sel_userpath));
		return "";
	}

	return OS_read_user_info_string(locs->sel_userpath, locs->fieldname);
}

/******************************************************************
  Client Function: DAILYPW -> USERDATA
  		get dailypw and return corresponding userpath and data
*******************************************************************/
extern "C" aisc_string dailypw_2_userpath(ORS_local *locs) {

	char *userpath;
	userpath=OS_find_dailypw(locs);
	if (!*userpath) return "";

	locs->username 		= OS_read_user_info_string(userpath,"username");
	if (!locs->username) {
		OS_locs_error(locs,"Can't read user info");
		return "";
	}
	locs->pub_exist_max 	= OS_read_user_info_string(userpath,"pub_exist_max");
	locs->pub_content_max 	= OS_read_user_info_string(userpath,"pub_content_max");
	locs->pub_exist_def 	= OS_read_user_info_string(userpath,"pub_exist_def");
	locs->pub_content_def 	= OS_read_user_info_string(userpath,"pub_content_def");
	locs->mail_addr		= OS_read_user_info_string(userpath,"mail_addr");
	locs->www_home 		= OS_read_user_info_string(userpath,"www_home");
	locs->max_users		= OS_read_user_info_int   (userpath,"max_users");
	locs->curr_users	= OS_read_user_info_int   (userpath,"curr_users");
	locs->max_user_depth	= OS_read_user_info_int   (userpath,"max_user_depth");
	locs->is_author		= OS_read_user_info_int   (userpath,"is_author");
	locs->is_superuser	= OS_read_user_info_int   (userpath,"is_superuser");
	locs->user_info		= OS_read_user_info_string(userpath,"user_info");
	locs->user_own_info	= OS_read_user_info_string(userpath,"user_own_info");
	locs->user_ta_id	= OS_read_user_info_int   (userpath,"ta_id");
	return userpath;
}

/******************************************************************
  Client Function: DAILYPW, SEL_USERPATH -> SEL_USERDATA
  		get dailypw and sel_userpath,
		return corresponding sel_user data 
*******************************************************************/
extern "C" aisc_string get_sel_userdata(ORS_local *locs) {

	char *userpath;
	userpath=OS_find_dailypw(locs);
	if (!*userpath) return "";

	if (!locs->sel_userpath) {
		OS_locs_error(locs, "Sel_userpath not set (get_sel_userdata)");
		return "";
	}

	//int may_read_all = (locs->is_superuser || ORS_is_parent_or_equal(userpath,locs->sel_userpath));
	
	locs->sel_username	=	OS_read_user_info_string(locs->sel_userpath,"username");
	if (!locs->sel_username) {
		delete locs->error;
		locs->error = strdup(ORS_export_error("User '%s' does not exist (get_sel_userdata)",locs->sel_userpath));
		return "";
	}
	locs->sel_user		=	OS_read_user_info_string    (locs->sel_userpath,"user");
	locs->sel_www_home	=	OS_read_user_info_string    (locs->sel_userpath,"www_home");
	locs->sel_mail_addr	=	OS_read_user_info_string    (locs->sel_userpath,"mail_addr");
	locs->sel_user_info	=	OS_read_user_info_string    (locs->sel_userpath,"user_info");
	locs->sel_user_own_info	=	OS_read_user_info_string    (locs->sel_userpath,"user_own_info");
	locs->sel_is_author	=	OS_read_user_info_int(locs->sel_userpath,"is_author");
	locs->sel_is_superuser	=	OS_read_user_info_int(locs->sel_userpath,"is_superuser");

	// if (locs->is_superuser || ORS_is_parent_or_equal(userpath,locs->sel_userpath)) {
		locs->sel_pub_exist_max	=	OS_read_user_info_string    (locs->sel_userpath,"pub_exist_max");
		locs->sel_pub_content_max =	OS_read_user_info_string    (locs->sel_userpath,"pub_content_max");
		locs->sel_max_users	=	OS_read_user_info_int(locs->sel_userpath,"max_users");
		locs->sel_curr_users	=	OS_read_user_info_int(locs->sel_userpath,"curr_users");
		locs->sel_max_user_depth=	OS_read_user_info_int(locs->sel_userpath,"max_user_depth");
		locs->sel_user_ta_id	= 	OS_read_user_info_int(locs->sel_userpath,"ta_id");
	// }

	return userpath;
}


/*****************************************************************************
  Client Function: LIST OF USERS
			all	    = all users
			my_users    = my sub users, without me
			sel_parents = my sub users, me, without sel and down
			who         = logged in users (userpath, login time)*
			who_my      = user's sub-users which are logged in
*****************************************************************************/
extern "C" aisc_string list_of_users(ORS_local *locs) {
	if (!locs->sel_userpath)    {
		OS_locs_error(locs,"sel_userpath NULL");    // must be save: no NULL strings!
		return strdup("");
	}

	if (!strcmp("my_users",locs->list_type))
		return OS_list_of_subusers(locs->userpath, 2, locs->userpath, NULL);

	else if (!strcmp("sel_parents",locs->list_type))
		return OS_list_of_subusers(locs->userpath, 2, NULL, locs->sel_userpath);

	else if (!strcmp("who",locs->list_type)) {
		if (locs->is_author || locs-> is_superuser)	return OS_who(locs, 0);
		else						return strdup("");
	}

	else if (!strcmp("who_my",locs->list_type)) {
		if (locs->userpath && *locs->userpath)	return OS_who(locs, locs->userpath);
		else					return strdup("");
	}

	else if (!strcmp("all",locs->list_type))
		return OS_list_of_subusers("/", 999, NULL, locs->sel_userpath);
	else {
		OS_locs_error(locs,"unknown user list");
		return strdup("");
	}

}

/************************************************************
  Client Function: WORK ON USER
					modify user 
*************************************************************/
extern "C" int work_on_user(ORS_local *locs, aisc_string /*dummy*/) {
	char *userpath;
	userpath=OS_find_dailypw(locs);
	if (!*userpath) return 0;
	locs->userpath=strdup(userpath);
	delete locs->error;
	if (!ORS_is_parent_or_equal(locs->pub_exist_max, locs->pub_exist_def)) {
		locs->error=ORS_export_error("Existence publicity not valid!");
		return 0;
	}
	if (!ORS_is_parent_or_equal(locs->pub_content_max, locs->pub_content_def)) {
		locs->error=ORS_export_error("Content publicity not valid!");
		return 0;
	}
	locs->error=strdup(OS_update_user(locs));
	return 0;

}

/************************************************************
  Client Function: WORK ON SEL_USER
				create/modify/delete sel_user 
*************************************************************/
extern "C" void work_on_sel_user(ORS_local *locs, char *action) {
	
	char *userpath=0;
	static char *error=0;
	delete error;

	userpath=OS_find_dailypw(locs);		// is user logged in?
	if (!*userpath) return;

	if (!action || !*action) {
		OS_locs_error(locs, "action not set.");
		return;
	}
	if (!OS_can_read_user(locs)) return;  	// does user exist in database?

	/*********************************************************************
	  CREATE SEL_USER
	*********************************************************************/
	if (!strcasecmp(action,"CREATE")) {

		locs->sel_userpath = OS_construct_sel_userpath(locs->sel_par_userpath, locs->sel_user);
		OS_locs_error_pointer( locs, OS_allowed_to_create_user( locs->userpath, locs->sel_userpath ) );
		if (*locs->error) return;

		OS_locs_error_pointer( locs, OS_new_user(locs) );
		if (*locs->error) return;

		OS_change_curr_user_counts(locs->sel_userpath, +1);	// increment all parents of sel_userpath

		// int curr_users = OS_read_user_info_int(locs->userpath,"curr_users");
		// OS_write_user_info_int(locs->userpath,"curr_users",++curr_users);	// increment number of current users
		// TODO: actually there is no curr_user_depth; but if there is - it must be increased here!
	}
	/*********************************************************************
	  MODIFY SEL_USER
	*********************************************************************/
	else if (!strcasecmp(action,"MODIFY")) {
		if (!locs->sel_userpath || !*(locs->sel_userpath)) {
			OS_locs_error(locs, "sel_userpath not set.");
			return;
		}
		
		// must be father
		if (!ORS_is_parent_of(userpath,locs->sel_userpath) && !locs->is_superuser) {
			delete locs->error;
			locs->error=strdup(ORS_export_error("You are not authorisized to modify data of user '%s'.",locs->sel_userpath));
			return;
		}
		OS_locs_error_pointer(locs, OS_update_sel_user(locs));
	}
	/*********************************************************************
	  DELETE SEL_USER
	*********************************************************************/
	else if (!strcasecmp(action,"DELETE") ) {
		if (!locs->sel_userpath || !*(locs->sel_userpath)) {
			OS_locs_error(locs, "sel_userpath not set.");
			return;
		}
		if (!ORS_is_parent_of(userpath,locs->sel_userpath)) {	// must be father
			OS_locs_error_pointer (locs, ORS_export_error("You are not authorisized to modify data of user '%s'.",locs->sel_userpath));
			return;
		}

		// sub-users?
		if (OS_user_has_sub_users(locs->sel_userpath)) OS_locs_error(locs, "user has sub-users and can therefor not be deleted.");

		// probes?
		int count=OS_count_probes_of_author(locs->sel_userpath);
		if (count) {
			OS_locs_error(locs, ORS_export_error(
						"user %s is creator of %i probe(s) and can therefor not be deleted.", locs->sel_userpath, count));
			return;
		}
		// TODO: what about ownership in a section?


		OS_change_curr_user_counts(locs->sel_userpath, -1);	// decrement all parents of sel_userpath

		OS_delete_sel_user(locs);	// delete the user from database
	}

	else OS_locs_error(locs, "unknown action (wosu)");
	return;
}

/************************************************************
  Client Function: SAVE USER DATABASE
*************************************************************/
extern "C" void save_userdb(ORS_local *locs, char */*dummy*/) {
	char *userpath;
	userpath=OS_find_dailypw(locs);
	if (!*userpath) {
		printf("userdb save: user unknown!\n");
		return;
	}

	OS_save_userdb();
	if (MSG) printf("%s user %s requested userdb save.\n", ORS_time_and_date_string(DATE_TIME,0), userpath);
}


/*******************************************************************************
 ===============================================================================
                  P R O B E        P R O B E       P R O B E
 ===============================================================================
 *******************************************************************************/


/************************************************************
  Client Function: WORK ON PROBE
				create/update/delete probe 
*************************************************************/
extern "C" void work_on_probe(ORS_local *locs, char *action) {

	delete locs->userpath;
	locs->userpath = strdup(OS_find_dailypw(locs));
	if (!*(locs->userpath)) return;

	if (!strcasecmp(action,"DELETE")) {
		if (OS_owner_of_probe(locs))				// am i owner?
			OS_delete_probe(locs);
		return;
	}

	ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;
	OS_pdb_list *pdb_elem_exist   = OS_find_pdb_list_elem_by_name(ors_main->pdb_list, "", "p_pub_exist");
	OS_pdb_list *pdb_elem_content = OS_find_pdb_list_elem_by_name(ors_main->pdb_list, "", "p_pub_content");

	locs->pub_exist_max   = OS_read_user_info_string(locs->userpath, "pub_exist_max");
	locs->pub_content_max = OS_read_user_info_string(locs->userpath, "pub_content_max");

	if (!ORS_is_parent_or_equal(locs->pub_exist_max, pdb_elem_exist->content)) {
		delete locs->error;
		if (!pdb_elem_exist->content) pdb_elem_exist->content = strdup("");
		locs->error = ORS_export_error("You are not allowed to set existance publicity to '%s'.", pdb_elem_exist->content);
		return;
	}
	if (!ORS_is_parent_or_equal(locs->pub_content_max, pdb_elem_content->content)) {
		delete locs->error;
		if (!pdb_elem_content->content) pdb_elem_content->content = strdup("");
		locs->error = ORS_export_error("You are not allowed to set content publicity to '%s'.", pdb_elem_content->content);
		return;
	}

	if (!strcasecmp(action,"CREATE")) {
		if (OS_owners_of_pdb_list_allowed(locs, action))	// validate owners
			OS_new_probe(locs);
	}
	else if (!strcasecmp(action,"UPDATE")) {
		if (OS_owners_of_pdb_list_allowed(locs, action))	// validate owners
			OS_update_probe(locs);
	}
}

/************************************************************
  Client Function: GET PROBE LIST
	type 1: author_datum, probe_name, specifity ("my probes")
	type 2: author, probe_name, specifity	("probes")
*************************************************************/
extern "C" bytestring *get_probe_list(ORS_local *locs) {
	static bytestring bs = {0,0};
	delete bs.data;

	delete locs->userpath;
	locs->userpath = strdup(OS_find_dailypw(locs));

	if (	locs->probe_list_type == 1 ||
		locs->probe_list_type == 2) {
		return OS_list_of_probes(locs, locs->userpath, locs->probe_list_type, locs->probe_list_max_count);
	} else {
		delete locs->error;
		locs->error=ORS_export_error("Probe list type %i is not implemented.",locs->probe_list_type);
		bs.data = strdup("");
		bs.size = 1;
		return &bs;
	}
}

/************************************************************
  Client Function: PROBE SELECT
				select probe via probe_ID
*************************************************************/
extern "C" void probe_select(ORS_local *locs, char *probe_id) {
	delete locs->userpath;
	locs->userpath = strdup(OS_find_dailypw(locs));

	// GBDATA *probe = GB_find(gb_probedb,"probe_id",probe_id,down_2_level);
	delete locs->probe_id;
	locs->probe_id = strdup(probe_id);
}

/************************************************************
  Client Function: GET PROBE FIELD
		return probe of field of preselected probe
*************************************************************/
extern "C" bytestring * get_probe_field(ORS_local *locs) {
	static bytestring bs = {0,0};
	delete bs.data;

	char *result;
	static char *read=0;
	delete locs->userpath;
	locs->userpath = strdup(OS_find_dailypw(locs));

	bs.data = strdup("");		// empty bytestring if error
	bs.size = 1;

	if (!locs->probe_id) return &bs;

	ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;
	OS_pdb_list *pdb_elem = OS_find_pdb_list_elem_by_name(ors_main->pdb_list, locs->probe_field_section, locs->probe_field_name);
	if (!pdb_elem || !locs->probe_id) return &bs;
	
	delete read; read = OS_read_probe_info(locs->probe_id, "p_pub_exist");	// not even exist allowed
	if (!OS_read_access_allowed(locs->userpath, read)) return &bs;
	delete read; read = OS_read_probe_info(locs->probe_id, "p_pub_content");  // no content allowed
	if (!OS_read_access_allowed(locs->userpath, read) && !strchr(pdb_elem->rights,'E')) return &bs;

	result = OS_read_probe_info(locs->probe_id, pdb_elem->db_name);

	// empty owner field: user p_author instead
	if (!ORS_strcmp(pdb_elem->name,"p_owner") && (!result || !*result)) {
		result = OS_read_probe_info(locs->probe_id, "p_author");
	}

	if (result)  {
		delete bs.data;
		bs.data = result;
		bs.size = strlen(bs.data)+1;
	}

	return &bs;
}

/************************************************************
  Client Function: PROBE QUERY
				return probe result list
*************************************************************/
extern "C" bytestring * probe_query(ORS_local *locs) {
	static bytestring bs = {0,0};
	delete bs.data;

	delete locs->userpath;
	locs->userpath = strdup(OS_find_dailypw(locs));

	return OS_list_of_probes(locs, locs->userpath, locs->probe_list_type, locs->probe_list_max_count);
}

/************************************************************
  Client Function: PUT PROBE FIELD
*************************************************************/
extern "C" void put_probe_field(ORS_local *locs, char *field_name) {
	ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;
	OS_pdb_list *pdb_elem;

	if (!ORS_strcmp(locs->probe_field_section,"") && !ORS_strcmp(field_name, "search_any_field")) {
		// append '*' to search string if not already there
		void *mem_file=GBS_stropen(1000);  // open memory file
		if (locs->probe_field_data.data[0] != '*') GBS_strcat(mem_file, "*");
		GBS_strcat(mem_file, locs->probe_field_data.data);
		if (locs->probe_field_data.data[strlen(locs->probe_field_data.data)-1] != '*') GBS_strcat(mem_file, "*");
		delete locs->search_any_field;
		locs->search_any_field = GBS_strclose(mem_file);
	}
	else {
		pdb_elem = OS_find_pdb_list_elem_by_name(ors_main->pdb_list, locs->probe_field_section, field_name);
		if (!pdb_elem) {
			delete locs->error;
			locs->error=ORS_export_error("received unknown probe field '%s'!",field_name);
			return;
		}
		if (!locs->probe_field_data.data) {
			delete locs->error;
			locs->error=ORS_export_error("data of probe field '%s' was empty!",field_name);
			return;
		}
		delete pdb_elem->content;
		pdb_elem->content = strdup(locs->probe_field_data.data);
	}
	if (MSG) printf("ORS SERVER got field '%s/%s' with '%s'.\n", locs->probe_field_section, field_name, locs->probe_field_data.data);
}

/************************************************************
  Client Function: CLEAR ALL PROBE FIELDS
*************************************************************/
extern "C" void clear_probe_fields(ORS_local *locs) {

	ORS_main *ors_main = (ORS_main *)locs->mh.parent->parent;
	OS_pdb_list *pdb_elem;

	delete locs->search_any_field;
	if (MSG) printf("delete pdb_list->contents ");
	for (pdb_elem = ors_main->pdb_list; pdb_elem; pdb_elem = OS_next_pdb_list_elem(ors_main->pdb_list, pdb_elem)) {
                delete pdb_elem->content;
		pdb_elem->content = 0;
		if (MSG) printf(".");
        }
	if (MSG) printf("\n");
}

/************************************************************
  Client Function: SAVE PROBE DATABASE
*************************************************************/
extern "C" void save_probedb(ORS_local *locs, char */*dummy*/) {
	char *userpath;
	userpath=OS_find_dailypw(locs);
	if (!*userpath) {
		if (MSG) printf("probedb save: user unknown!\n");
		return;
	}

	OS_save_probedb();
	if (MSG) printf("%s user %s requested probedb save.\n", ORS_time_and_date_string(DATE_TIME,0), userpath);
}

/************************************************************
  Client Function: TRANSFER PROBES of one user to another
			transfer from sel_userpath to string
*************************************************************/
extern "C" void probe_user_transfer(ORS_local *locs, char *sel_userpath2) {
	char *userpath;
	userpath=OS_find_dailypw(locs);
	if (!*userpath) {
		delete locs->error;
		locs->error = ORS_export_error("invalid user (probes_trans)");
		if (MSG) printf("%s\n", locs->error);
		return;
	}

	if (!locs->sel_userpath || !*locs->sel_userpath) {
		delete locs->error;
		locs->error = ORS_export_error("sel_userpath not set (probes_trans)");
		if (MSG) printf("%s\n", locs->error);
		return;
	}

	if (!sel_userpath2 || !*sel_userpath2) {
		delete locs->error;
		locs->error = ORS_export_error("string (sel_userpath2) not set (probes_trans)");
		if (MSG) printf("%s\n", locs->error);
		return;
	}

	// if (!ORS_is_parent_or_equal(locs->userpath, locs->sel_userpath) || !ORS_is_parent_or_equal(locs->userpath, sel_userpath2)) {
	if (!locs->is_superuser) {
		delete locs->error;
		locs->error = ORS_export_error("You are not allowed to transfer probes (must be superuser)");
		if (MSG) printf("%s\n", locs->error);
		return;
	}

	if (!locs->probe_id || !*locs->probe_id)
		OS_probe_user_transfer(locs, locs->sel_userpath, sel_userpath2, "*");
	else
		OS_probe_user_transfer(locs, locs->sel_userpath, sel_userpath2, locs->probe_id);
	return;

}
