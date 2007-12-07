/*
#################################
#                               #
#    ORS_CLIENT:  USER          #
#    user functions             #
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

#include "cat_tree.hxx"
#include "ors_c_java.hxx"
#include "ors_c.hxx"
#include "ors_c_proto.hxx"

/*********************************************************
  SAVE USERDB
**********************************************************/
void OC_save_userdb(void) {

	char *locs_error = 0;

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW,		ors_gl.dailypw,
		LOCAL_REMOTE_HOST, 	ors_gl.remote_host,
		LOCAL_SAVE_USERDB,	"",
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (save_userdb)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR       , &locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (save_userdb2)"));
	}
	OC_server_error_if_not_empty(locs_error);
}

/*********************************************************
  LOGIN USER
  send userdata and receive dailypw if userdata is valid
**********************************************************/
void OC_login_user(char * password) {

	char *locs_error = 0;
	ors_gl.dailypw   = 0;  // server might free variable
	static char *crypted_pw = 0;
	delete crypted_pw;

	crypted_pw = ORS_crypt(password);

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_USERPATH,		ors_gl.userpath,
		LOCAL_PASSWORD,		crypted_pw,
		LOCAL_REMOTE_HOST, 	ors_gl.remote_host,
		LOCAL_REMOTE_USER, 	ors_gl.remote_user,
		LOCAL_DEBUG, 		ors_gl.debug, NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (login_user1)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_CALC_DAILYPW,	&ors_gl.dailypw,
		LOCAL_USERNAME, 	&ors_gl.username,
		LOCAL_WWW_HOME, 	&ors_gl.www_home,
		LOCAL_ERROR       , &locs_error,
 		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (login_user2)"));
	}
	OC_server_error_if_not_empty(locs_error);
}

/*********************************************************
  LOGOUT
**********************************************************/
void OC_logout_user(void) {

	char *locs_error = 0;

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_LOGOUT_USER,	ors_gl.dailypw,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (logout1)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR       , &locs_error,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (logout2)"));
	}
	OC_server_error_if_not_empty(locs_error);
	OC_output_html_page("logout");		//! logout message with nice good bye picture
	exit (0);
}

/****************************************************************
  DAILYPW -> USERDATA
  send dailypw to server; receive user data if dailypw is valid
*****************************************************************/
void OC_dailypw_2_userpath(void) {

	char *locs_error = 0;

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW,    	ors_gl.dailypw,
		LOCAL_REMOTE_HOST, 	ors_gl.remote_host,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (3)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW_2_USERPATH, 	&ors_gl.userpath,
		LOCAL_USERNAME,			&ors_gl.username,
		LOCAL_PUB_EXIST_MAX,		&ors_gl.pub_exist_max,
		LOCAL_PUB_CONTENT_MAX,		&ors_gl.pub_content_max,
		LOCAL_PUB_EXIST_DEF,		&ors_gl.pub_exist_def,
		LOCAL_PUB_CONTENT_DEF,		&ors_gl.pub_content_def,
		LOCAL_MAIL_ADDR,		&ors_gl.mail_addr,
		LOCAL_WWW_HOME,			&ors_gl.www_home,
		LOCAL_USER_INFO,		&ors_gl.user_info,
		LOCAL_USER_OWN_INFO,		&ors_gl.user_own_info,
		LOCAL_MAX_USERS,		&ors_gl.max_users,
		LOCAL_CURR_USERS,		&ors_gl.curr_users,
 		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (4)"));
	}
	// too many fields (>=16)
	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_MAX_USER_DEPTH,		&ors_gl.max_user_depth,
		LOCAL_IS_AUTHOR,		&ors_gl.is_author,
		LOCAL_IS_SUPERUSER,		&ors_gl.is_superuser,
		LOCAL_USER_TA_ID,		&ors_gl.user_ta_id,
		LOCAL_ERROR, 			&locs_error,
 		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (14)"));
	}
	OC_server_error_if_not_empty(locs_error);
}

/****************************************************************
  DAILYPW, SEL_USERPATH -> SEL_USER'S DATA
  send my dailypw and the desired sel_userpath to server;
  receive all sel_user data if my access is valid
*****************************************************************/
void OC_get_sel_userdata_from_server(void) {

	char *locs_error = 0;

	// at creation time there is no sel_userpath
	// --> we only set init values
	if (!ors_gl.sel_userpath) {
		if (ors_gl.sel_user) {
			if (!ors_gl.sel_par_userpath)
				ors_gl.sel_par_userpath = strdup(ors_gl.userpath);
			ors_gl.sel_pub_exist_max   = strdup(ors_gl.sel_user);
			ors_gl.sel_pub_content_max = strdup(ors_gl.sel_user);
		}
		return;
	}

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW,		ors_gl.dailypw,
		LOCAL_REMOTE_HOST, 	ors_gl.remote_host,
		LOCAL_SEL_USERPATH,	ors_gl.sel_userpath,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (dpw2sel_ud1)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, 	ors_gl.locs,
		LOCAL_GET_SEL_USERDATA,		&ors_gl.userpath,
		// LOCAL_USERNAME,			&ors_gl.username,
		LOCAL_SEL_USER,			&ors_gl.sel_user,
		LOCAL_SEL_USERNAME,		&ors_gl.sel_username,
		LOCAL_SEL_WWW_HOME,		&ors_gl.sel_www_home,
		LOCAL_SEL_USER_INFO,		&ors_gl.sel_user_info,
		LOCAL_SEL_USER_OWN_INFO,	&ors_gl.sel_user_own_info,
		LOCAL_SEL_MAIL_ADDR,		&ors_gl.sel_mail_addr,
		LOCAL_SEL_PUB_EXIST_MAX,	&ors_gl.sel_pub_exist_max,
		LOCAL_SEL_PUB_CONTENT_MAX,	&ors_gl.sel_pub_content_max,
 		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (dpw2sel_ud2)"));
	}
	if (aisc_get(ors_gl.link, ORS_LOCAL, 	ors_gl.locs,
		LOCAL_SEL_MAX_USERS,		&ors_gl.sel_max_users,
		LOCAL_SEL_MAX_USER_DEPTH,	&ors_gl.sel_max_user_depth,
		LOCAL_SEL_IS_AUTHOR,		&ors_gl.sel_is_author,
		LOCAL_SEL_IS_SUPERUSER,		&ors_gl.sel_is_superuser,
		LOCAL_SEL_USER_TA_ID,		&ors_gl.sel_user_ta_id,
		LOCAL_ERROR, 			&locs_error,
 		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (dpw2sel_ud3)"));
	}
	OC_server_error_if_not_empty(locs_error);

	// calculate sel_par_userpath and sel_user
	if (ors_gl.sel_userpath) {
		delete ors_gl.sel_par_userpath;
		char *pos=strrchr(ors_gl.sel_userpath,'/');

		if (pos && pos > ors_gl.sel_userpath) {
			*pos=0;
			ors_gl.sel_par_userpath = strdup(ors_gl.sel_userpath);
			*pos='/';
		}
		else ors_gl.sel_par_userpath = "/";

		//if (!ors_gl.sel_user || !*ors_gl.sel_user) {	// only calculate sel_user when not read from cgi_var
			delete ors_gl.sel_user;
			if (pos < ors_gl.sel_userpath + strlen(ors_gl.sel_userpath) - 1)
				ors_gl.sel_user = strdup(pos + 1);
			else
				ors_gl.sel_user = strdup(ors_gl.sel_userpath);
		//}
	}
}


/*********************************************************
  READ USER FIELD
				only if userpath is set
**********************************************************/
char * OC_read_user_field_if_userpath_set(char *sel_userpath, char *fieldname) {
	if (!sel_userpath || !*sel_userpath) return strdup("");
	return OC_read_user_field(sel_userpath, fieldname);
}
/*********************************************************
  READ USER FIELD
**********************************************************/
char * OC_read_user_field(char *sel_userpath, char *fieldname) {

	char *locs_error = 0;
	char *field_data;

	if (!sel_userpath)	quit_with_error(ORS_export_error("read_user_field: sel_userpath == (null)"));

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW,		ors_gl.dailypw,
		LOCAL_REMOTE_HOST, 	ors_gl.remote_host,
		LOCAL_SEL_USERPATH,	sel_userpath,
		LOCAL_FIELDNAME,	fieldname,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (10)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_READ_USER_FIELD, &field_data,
		LOCAL_ERROR       , &locs_error,
 		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (11)"));
	}
	OC_server_error_if_not_empty(locs_error);
	return strdup(field_data);
}
/*****************************************************************************
  list of subusers (request list from server)
*****************************************************************************/
char * list_of_users(char *keyword) {
	char *locs_error = 0;
	char *list;

	if (!ors_gl.userpath)		quit_with_error(ORS_export_error("Userpath not set!!"));
	if (!ors_gl.sel_userpath)	ors_gl.sel_userpath=""; // not needed
	if (!keyword)			quit_with_error(ORS_export_error("List Type keyword not set!!"));

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_USERPATH,		ors_gl.userpath,
		LOCAL_SEL_USERPATH,	ors_gl.sel_userpath,
		LOCAL_LIST_TYPE,	keyword,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (l_of_su1)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_LIST_OF_USERS, &list,
		LOCAL_ERROR,		&locs_error,
 		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (l_of_su2)"));
	}
	OC_server_error_if_not_empty(locs_error);
	return list;
}
/*****************************************************************************
  WORK ON USER: preferences of user
*****************************************************************************/
void work_on_user(void) {
	char *locs_error = 0, *dummy=0;
	if (!ors_gl.userpath)		quit_with_error(ORS_export_error("Userpath not set!!"));

	static char *crypted_pw = 0;
	delete crypted_pw;
	crypted_pw = ORS_crypt(ors_gl.password);

	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_DAILYPW,    	ors_gl.dailypw,
		LOCAL_REMOTE_HOST, 	ors_gl.remote_host,
		LOCAL_USERNAME,		ors_gl.username,
		LOCAL_PASSWORD,		crypted_pw,
		LOCAL_MAIL_ADDR,	ors_gl.mail_addr,
		LOCAL_WWW_HOME,		ors_gl.www_home,
		LOCAL_USER_OWN_INFO,	ors_gl.user_own_info,
		LOCAL_PUB_EXIST_DEF,	ors_gl.pub_exist_def,
		LOCAL_PUB_CONTENT_DEF,	ors_gl.pub_content_def,
		LOCAL_USER_TA_ID,	ors_gl.user_ta_id,
		LOCAL_WORK_ON_USER, 	"",
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (work_on_user1)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR       , &locs_error,
		 NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (work_on_user2)"));
	}

	ors_gl.message = new char[256];
	sprintf(ors_gl.message,"Your preferences have been set, %s.",ors_gl.username);

	OC_output_html_page("success");		//! successful message
	exit(0);
}

/*****************************************************************************
  WORK ON SEL_USER: create/modify/delete user
*****************************************************************************/
extern "C" void work_on_sel_user(char *action) {
  char *locs_error = 0, *dummy=0;

  if (!ors_gl.userpath)		quit_with_error(ORS_export_error("Userpath not set!!"));

  if (!strcasecmp(action,"CREATE") || !strcasecmp(action,"MODIFY")) {

	if (!ors_gl.sel_password)	ors_gl.sel_password ="";
	if (!ors_gl.sel_mail_addr)	ors_gl.sel_mail_addr="";
	if (!ors_gl.sel_user_info)	ors_gl.sel_user_info="";
	if (!ors_gl.sel_username)	ors_gl.sel_username ="";

	static char *crypted_pw = 0;
	delete crypted_pw;
	crypted_pw = ORS_crypt(ors_gl.sel_password);

	/*********************************************************************
	  CREATE SEL_USER
	*********************************************************************/
	if (!strcasecmp(action,"CREATE")) {
		if (!ors_gl.sel_user)		quit_with_error(ORS_export_error("Sel_User not set!!"));
		if (!ors_gl.sel_par_userpath)	quit_with_error(ORS_export_error("Sel_Par_Userpath not set!!"));
		if (!*(ors_gl.sel_username))	quit_with_error(ORS_export_error("You must supply a full name."));
		if (!*(ors_gl.sel_password))	quit_with_error(ORS_export_error("You must supply a password!"));
		if (strlen(ors_gl.sel_password) < PASSWORD_MIN_LENGTH && !ors_gl.is_superuser)
				quit_with_error(ORS_export_error("Password must have at least %i characters!",PASSWORD_MIN_LENGTH));

		if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
			LOCAL_USERPATH,    	ors_gl.userpath,
			LOCAL_SEL_USER,    	ors_gl.sel_user,
			LOCAL_SEL_PAR_USERPATH,	ors_gl.sel_par_userpath,
			LOCAL_SEL_USERNAME,	ors_gl.sel_username,
			LOCAL_SEL_PASSWORD,	crypted_pw,
			LOCAL_SEL_MAIL_ADDR,	ors_gl.sel_mail_addr,
			LOCAL_SEL_WWW_HOME,	ors_gl.sel_www_home,
			LOCAL_SEL_USER_INFO,	ors_gl.sel_user_info,
			LOCAL_SEL_PUB_EXIST_MAX,ors_gl.sel_pub_exist_max,
			LOCAL_SEL_PUB_CONTENT_MAX,ors_gl.sel_pub_content_max,
			NULL)) {
			quit_with_error(ORS_export_error("Server communication failed (work_on_sel_user1)"));
		}
		if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
			LOCAL_SEL_MAX_USERS,	ors_gl.sel_max_users,
			LOCAL_SEL_MAX_USER_DEPTH,ors_gl.sel_max_user_depth,
			LOCAL_SEL_IS_AUTHOR,	ors_gl.sel_is_author,
			LOCAL_SEL_IS_SUPERUSER,	ors_gl.sel_is_superuser,
			LOCAL_SEL_USER_TA_ID,	ors_gl.sel_user_ta_id,
			LOCAL_WORK_ON_SEL_USER,	action,
			NULL)) {
			quit_with_error(ORS_export_error("Server communication failed (work_on_sel_user1)"));
		}

		if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
			LOCAL_ERROR       , &locs_error,
 			NULL)) {
			quit_with_error(ORS_export_error("Server communication failed (work_on_sel_user2)"));
		}
		ors_gl.message = new char[256];
		sprintf(ors_gl.message,"User %s has been created.",ors_gl.sel_user);
	}
	/*********************************************************************
	  MODIFY SEL_USER
	*********************************************************************/
	else if (!strcasecmp(action,"MODIFY")) {
	   	if (!ors_gl.sel_userpath)	quit_with_error(ORS_export_error("Sel_Userpath not set!!"));

		if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
			LOCAL_USERPATH,    	ors_gl.userpath,
			LOCAL_SEL_USERPATH,	ors_gl.sel_userpath,		// old sel_userpath
			LOCAL_SEL_USER,    	ors_gl.sel_user,
			LOCAL_SEL_PAR_USERPATH,	ors_gl.sel_par_userpath,	// sel_user is equal, par_userpath may be new
			LOCAL_SEL_USERNAME,	ors_gl.sel_username,
			LOCAL_SEL_PASSWORD,	crypted_pw,
			LOCAL_SEL_MAIL_ADDR,	ors_gl.sel_mail_addr,
			LOCAL_SEL_WWW_HOME,	ors_gl.sel_www_home,
			LOCAL_SEL_USER_INFO,	ors_gl.sel_user_info,
			LOCAL_SEL_PUB_EXIST_MAX,ors_gl.sel_pub_exist_max,
			LOCAL_SEL_PUB_CONTENT_MAX,ors_gl.sel_pub_content_max,
			NULL)) {
			quit_with_error(ORS_export_error("Server communication failed (work_on_sel_user5)"));
		}
		if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
			LOCAL_SEL_MAX_USERS,	ors_gl.sel_max_users,
			LOCAL_SEL_MAX_USER_DEPTH,ors_gl.sel_max_user_depth,
			LOCAL_SEL_IS_AUTHOR,	ors_gl.sel_is_author,
			LOCAL_SEL_IS_SUPERUSER,	ors_gl.sel_is_superuser,
			LOCAL_SEL_USER_TA_ID,	ors_gl.sel_user_ta_id,
			LOCAL_WORK_ON_SEL_USER,	action,
			NULL)) {
			quit_with_error(ORS_export_error("Server communication failed (work_on_sel_user3)"));
		}

		if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
			LOCAL_ERROR       , &locs_error,
 			NULL)) {
			quit_with_error(ORS_export_error("Server communication failed (work_on_sel_user4)"));
		}
		ors_gl.message = new char[256];
		sprintf(ors_gl.message,"User %s has been modified.",ors_gl.sel_userpath);
	}
  }
  /*********************************************************************
    DELETE SEL_USER
  *********************************************************************/
  else if (!strcasecmp(action,"DELETE")) {
	if (aisc_nput(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_USERPATH,    	ors_gl.userpath,
		LOCAL_SEL_USERPATH,	ors_gl.sel_userpath,
		LOCAL_WORK_ON_SEL_USER,	action,
		NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (work_on_user5)"));
	}

	if (aisc_get(ors_gl.link, ORS_LOCAL, ors_gl.locs,
		LOCAL_ERROR       , &locs_error,
		 NULL)) {
		quit_with_error(ORS_export_error("Server communication failed (work_on_user6)"));
	}
 	ors_gl.message = new char[256];
	sprintf(ors_gl.message,"User %s has been deleted.",ors_gl.sel_userpath);
 }
  else quit_with_error(ORS_export_error("Unknown user action: %s",action));

  if (locs_error && *locs_error) {
	quit_with_error(ORS_export_error("Server error: %s",locs_error));
  }

  OC_output_html_page("success");
  exit(0);

}

