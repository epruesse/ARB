/*
#################################
#                               #
#    ORS_SERVER:  USERDB        #
#    manage user database       #
#                               #
################################# */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

#include <ors_server.h>
#include <ors_lib.h>
#include <arbdb.h>
#include <arbdbt.h>

#include "ors_s_common.hxx"
#include "ors_s_proto.hxx"

#define ROOT_USERPATH "/"

GBDATA *gb_userdb;

// class functions
//void ugl_struct::clear(void){
//	memset((char *)this,0,sizeof(ugl_struct));
//}
// contructor
//ugl_struct::ugl_struct(void){
//	clear();
//}
//
//ugl_struct user_gl;

// loeschen mit user_gl.clear();

/*************************************************************************************
  open the user database
*************************************************************************************/
GB_ERROR OS_open_userdb(void){

	char *name = ORS_read_a_line_in_a_file(ORS_LIB_PATH "CONFIG","USER_DB"); //arb.user.db
	if (!name) ORS_export_error("Missing 'USER_DB' in '" ORS_LIB_PATH "CONFIG'");

	gb_userdb = GB_open(name,"rwc");
	if (!gb_userdb) return GB_get_error();
	return 0;
}

/*************************************************************************************
  close the user database (save changes)
*************************************************************************************/
extern "C" GB_ERROR OS_save_userdb(void){
	static long last_saved=0;
	last_saved = GB_read_clock(gb_userdb);
	return GB_save(gb_userdb,0,"a");
}

/*****************************************************************************
  CAN READ USER??   Return true or false
*****************************************************************************/
int OS_can_read_user(ORS_local *locs) {
	static char *userpath = 0;
	delete userpath;
	userpath = OS_read_user_info_string(locs->userpath,"userpath");
	if (!userpath) {
		delete locs->error;
		locs->error = strdup(ORS_export_error("user '%s' does not exist.",locs->userpath));
		return 0;
	}
	return 1;
}

/*****************************************************************************
  CAN READ SEL_USER??   Return true or false
*****************************************************************************/
int OS_can_read_sel_user(ORS_local *locs) {
	static char *userpath = 0;
	delete userpath;
	userpath = OS_read_user_info_string(locs->sel_userpath,"userpath");
	if (!userpath) {
		delete locs->error;
		locs->error = strdup(ORS_export_error("sel_user '%s' does not exist.",locs->sel_userpath));
		return 0;
	}
	return 1;
}

/*****************************************************************************
  USER EXISTS??   Return true or false
*****************************************************************************/
int OS_user_exists_in_userdb(char *userpath) {
	static char *user = 0;
	delete user;
	user = OS_read_user_info_string(userpath,"userpath");
	if (!user)	return 0;
	else		return 1;
}

/*************************************************************************************
  INSERT NEW USER into user database
							no authorisation here!
*************************************************************************************/
char *OS_new_user(ORS_local *locs) {

	if (!locs->sel_user || !*(locs->sel_user)) return strdup("sel_user not set.");
	if (!locs->sel_par_userpath || !*(locs->sel_par_userpath)) return strdup("You have to select a parent user.");

	char *new_userpath = OS_construct_sel_userpath(locs->sel_par_userpath, locs->sel_user);

	GB_begin_transaction(gb_userdb);
	GBDATA *gb_userpath = GB_find(gb_userdb,"userpath",new_userpath,down_2_level);
	if (gb_userpath){
		GB_abort_transaction(gb_userdb);
		delete new_userpath;
		return ORS_export_error("user %s already exists",new_userpath);
	}
	if (OS_find_user_and_password(locs->sel_user, locs->sel_password)) {
		GB_abort_transaction(gb_userdb);
		delete new_userpath;
		return ORS_export_error("entered password is reserved - please choose a different one.",new_userpath);
	}


	GBDATA *gb_user = GB_create_container(gb_userdb,"user");
	GBDATA *gb_field;

	gb_field = GB_create(gb_user,"userpath",GB_STRING);		//! unique user identification as path /user1/user2/...
	GB_write_string(gb_field,new_userpath);

	gb_field = GB_create(gb_user,"user",GB_STRING);			//! login name (unique together with password); equal to last element of userpath
	GB_write_string(gb_field,locs->sel_user);

	gb_field = GB_create(gb_user,"username",GB_STRING);		//! user's full name
	GB_write_string(gb_field,locs->sel_username);

	gb_field = GB_create(gb_user,"password",GB_STRING);		//! user's password
	GB_write_string(gb_field,locs->sel_password);

	gb_field = GB_create(gb_user,"is_author",GB_INT);		//! author gets development environment
	GB_write_int(gb_field,locs->sel_is_author);

	gb_field = GB_create(gb_user,"is_superuser",GB_INT);		//! superuser may access any user
	GB_write_int(gb_field,locs->sel_is_superuser);

	gb_field = GB_create(gb_user,"user_info",GB_STRING);		//! info string, set by father
	GB_write_string(gb_field,locs->sel_user_info);

	gb_field = GB_create(gb_user,"user_own_info",GB_STRING);	//! info string, set by user
	GB_write_string(gb_field,"");

	gb_field = GB_create(gb_user,"mail_addr",GB_STRING);		//! mail addresses, seperated by newline
	GB_write_string(gb_field,locs->sel_mail_addr);

	gb_field = GB_create(gb_user,"www_home",GB_STRING);		//! www home page
	GB_write_string(gb_field,locs->sel_www_home);

	gb_field = GB_create(gb_user,"pub_exist_max",GB_STRING);	//! maximum existance level for probe data
	GB_write_string(gb_field,locs->sel_pub_exist_max);

	gb_field = GB_create(gb_user,"pub_content_max",GB_STRING);	//! maximum content level for probe data
	GB_write_string(gb_field,locs->sel_pub_content_max);

	gb_field = GB_create(gb_user,"pub_exist_def",GB_STRING);	//! default existance level for probe data
	GB_write_string(gb_field,new_userpath);   // def=me

	gb_field = GB_create(gb_user,"pub_content_def",GB_STRING);	//! default content level for probe data
	GB_write_string(gb_field,new_userpath); // def=me

	gb_field = GB_create(gb_user,"max_users",GB_INT);		//! maximum number of sub users
	GB_write_int(gb_field,locs->sel_max_users);

	gb_field = GB_create(gb_user,"max_user_depth",GB_INT);		//! maximum depth of sub user tree
	GB_write_int(gb_field,locs->sel_max_user_depth);

	gb_field = GB_create(gb_user,"curr_users",GB_INT);		//! current number of users
	GB_write_int(gb_field,0);

	gb_field = GB_create(gb_user,"ta_id",GB_INT);			//! transaction number
	GB_write_int(gb_field,0);

	GB_commit_transaction(gb_userdb);
	delete new_userpath;
	return strdup("");
}

/*************************************************************************************
  UPDATE USER DATA in user database (PREFERENCES)
							return error message
							no authorisation here!
*************************************************************************************/
char * OS_update_user(ORS_local *locs ) {
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	if (!locs->userpath || !*(locs->userpath)) return strdup("userpath not set.");

	GBDATA *gb_userpath = GB_find(gb_userdb,"userpath",locs->userpath,down_2_level);
	if (!gb_userpath) return strdup("User not found (update_user)");
	GBDATA *gb_user = GB_get_father(gb_userpath);
	GBDATA *gb_field;

	gb_field = GB_find(gb_user,"ta_id",0,down_level);
	int read_ta_id = GB_read_int(gb_field);
	if (read_ta_id != locs->user_ta_id) return strdup("User data has been changed in the meantime. Please reload update page.");

	static char *empty_pw = 0;
	delete empty_pw;
	empty_pw = ORS_crypt("");

	if (locs->password 	  && *locs->password && ORS_strcmp(locs->password, empty_pw))
								OS_write_gb_user_info_string(gb_user,"password",	locs->password);
	if (locs->username  	  && *locs->username)  		OS_write_gb_user_info_string(gb_user,"username", 	locs->username);
	if (locs->user_own_info   && *locs->user_own_info)	OS_write_gb_user_info_string(gb_user,"user_own_info",	locs->user_own_info);
	if (locs->mail_addr 	  && *locs->mail_addr) 		OS_write_gb_user_info_string(gb_user,"mail_addr",	locs->mail_addr);
	if (locs->www_home  	  && *locs->www_home)   	OS_write_gb_user_info_string(gb_user,"www_home", 	locs->www_home);
	if (locs->pub_exist_def   && *locs->pub_exist_def) 	OS_write_gb_user_info_string(gb_user,"pub_exist_def", 	locs->pub_exist_def);
	if (locs->pub_content_def && *locs->pub_content_def) 	OS_write_gb_user_info_string(gb_user,"pub_content_def", locs->pub_content_def);

	OS_write_gb_user_info_int(gb_user,"is_author",		locs->is_author);
	OS_write_gb_user_info_int(gb_user,"is_superuser",	locs->is_superuser);
	OS_write_gb_user_info_int(gb_user,"ta_id",		++read_ta_id);

	return strdup("");
}

/*************************************************************************************
  UPDATE SEL_USER DATA in user database
							return strdup(error message)
							no authorisation here!
*************************************************************************************/
char * OS_update_sel_user(ORS_local *locs ) {
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	if (!locs->sel_userpath || !*(locs->sel_userpath)) return strdup("sel_userpath not set.");

	GBDATA *gb_userpath = GB_find(gb_userdb,"userpath",locs->sel_userpath,down_2_level);
	if (!gb_userpath) return strdup("User not found (update_sel_user)");
	GBDATA *gb_user = GB_get_father(gb_userpath);
	GBDATA *gb_field;

	gb_field = GB_find(gb_user,"ta_id",0,down_level);
	int read_ta_id = GB_read_int(gb_field);
	if (read_ta_id != locs->sel_user_ta_id)
		return strdup("User data has been changed in the meantime. Please reload your update page.");

	// parent was changed
	static char *new_userpath = 0;
	delete new_userpath;
	if (locs->sel_par_userpath && *locs->sel_par_userpath) {
		new_userpath = OS_construct_sel_userpath(locs->sel_par_userpath, locs->sel_user);

		if (strcmp(locs->sel_userpath,new_userpath)) {	// parent was changed
			GBDATA *gb_does_exist = GB_find(gb_userdb,"userpath",new_userpath,down_2_level);
			if (gb_does_exist){
				return ORS_export_error("You can not change parent to %s, because user %s already exists.",
												locs->sel_par_userpath, new_userpath);
			}

			return OS_change_sel_parent_user(locs, locs->sel_userpath, new_userpath);
		}
	}

	static char *empty_pw = 0;
	delete empty_pw;
	empty_pw = ORS_crypt("");

	static char *read_data = 0;
	delete read_data;

	if (locs->sel_password 	  && *locs->sel_password && ORS_strcmp(locs->sel_password, empty_pw))
									OS_write_gb_user_info_string(gb_user,"password",	locs->sel_password);
	if (locs->sel_username  	&& *locs->sel_username)  	OS_write_gb_user_info_string(gb_user,"username", 	locs->sel_username);
	if (locs->sel_user_info 	&& *locs->sel_user_info)	OS_write_gb_user_info_string(gb_user,"user_info",	locs->sel_user_info);
	if (locs->sel_mail_addr 	&& *locs->sel_mail_addr) 	OS_write_gb_user_info_string(gb_user,"mail_addr",	locs->sel_mail_addr);
	if (locs->sel_www_home  	&& *locs->sel_www_home)		OS_write_gb_user_info_string(gb_user,"www_home", 	locs->sel_www_home);
	if (locs->sel_pub_exist_max   	&& *locs->sel_pub_exist_max) {
									OS_write_gb_user_info_string(gb_user,"pub_exist_max", 	locs->sel_pub_exist_max);
		if (!ORS_is_parent_or_equal(locs->sel_pub_exist_max, read_data=OS_read_user_info_string(new_userpath, "pub_exist_def") ) )
									OS_write_gb_user_info_string(gb_user,"pub_exist_def", 	locs->sel_pub_exist_max);
	}
	if (locs->sel_pub_content_max 	&& *locs->sel_pub_content_max) {
									OS_write_gb_user_info_string(gb_user,"pub_content_max", locs->sel_pub_content_max);
		if (!ORS_is_parent_or_equal(locs->sel_pub_content_max, read_data=OS_read_user_info_string(new_userpath, "pub_content_def") ) )
									OS_write_gb_user_info_string(gb_user,"pub_content_def", locs->sel_pub_content_max);
	}

	OS_write_gb_user_info_int(gb_user,"max_users",		locs->sel_max_users);
	OS_write_gb_user_info_int(gb_user,"max_user_depth",	locs->sel_max_user_depth);
	OS_write_gb_user_info_int(gb_user,"is_author",		locs->sel_is_author);
	OS_write_gb_user_info_int(gb_user,"is_superuser",	locs->sel_is_superuser);
	OS_write_gb_user_info_int(gb_user,"ta_id",		++read_ta_id);

	return strdup("");
}

/*************************************************************************************
  CHANGE PARENT of SEL_USER  in user database
							return error message
							no authorisation here!
*************************************************************************************/
char * OS_change_sel_parent_user(ORS_local *locs, char *old_userpath, char *new_userpath) {
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	GBDATA *gb_userpath = GB_find(gb_userdb,"userpath",old_userpath,down_2_level);
	if (!gb_userpath) return strdup("User not found (change_sel_par)");
	GBDATA *gb_user = GB_get_father(gb_userpath);

	// change all authors and owners of all probes
	OS_probe_user_transfer(locs, old_userpath, new_userpath, "*");

	// change user itself
	GBDATA *gb_field = GB_find(gb_user,"userpath",0,down_level);
	GB_write_string(gb_field,new_userpath);

	return strdup("");
}

/*************************************************************************************
  DELETE SEL_USER DATA from user database
							return error message
							no authorisation here!
*************************************************************************************/
char * OS_delete_sel_user(ORS_local *locs ) {
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	if (!locs->sel_userpath || !*(locs->sel_userpath)) return strdup("sel_userpath not set.");

	GBDATA *gb_userpath = GB_find(gb_userdb,"userpath",locs->sel_userpath,down_2_level);
	if (!gb_userpath) return strdup("User not found (update_sel_user)");
	GBDATA *gb_user = GB_get_father(gb_userpath);

	GB_delete(gb_user);
    return 0;
}

/*************************************************************************************
  READ USER FIELD INFORMATION from user database (INTEGER)
							return data or -1
							no authorisation here!
*************************************************************************************/
int OS_read_user_info_int(char *userpath, char *fieldname){
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends
	GBDATA *gb_userpath = GB_find(gb_userdb,"userpath",userpath,down_2_level);
	if (!gb_userpath) return -1;
	GBDATA *gb_user = GB_get_father(gb_userpath);
	GBDATA *gb_field = GB_find(gb_user,fieldname,0,down_level);
	if (!gb_field) return -1;
	return GB_read_int(gb_field);
}

/*************************************************************************************
  READ USER FIELD INFORMATION from user database
							return data or NULL
							no authorisation here!
*************************************************************************************/
char * OS_read_user_info_string(char *userpath, char *fieldname){
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends
	GBDATA *gb_userpath = GB_find(gb_userdb,"userpath",userpath,down_2_level);
	if (!gb_userpath) return NULL;
	GBDATA *gb_user = GB_get_father(gb_userpath);
	GBDATA *gb_field = GB_find(gb_user,fieldname,0,down_level);
	if (!gb_field) return NULL;
	return GB_read_string(gb_field);
}
/*************************************************************************************
  FIND USER via USER + PASSWORD from user database
							return userpath or NULL
*************************************************************************************/
char * OS_find_user_and_password(char *user, char *password){
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	//char *userpath=0;
	GBDATA *gb_userpath;
	GBDATA *gb_user;
	GBDATA *gb_field;

	// slash contained: look for full userpath
	if (strchr(user,'/')) {
		gb_userpath = GB_find(gb_userdb,"userpath",user,down_2_level);
		if (!gb_userpath) return NULL;
		gb_user  = GB_get_father(gb_userpath);
		gb_field = GB_find(gb_user,"password",0,down_level);
		if (!gb_field) return NULL;
		if (strcmp(GB_read_string(gb_field),password)) return NULL;
		return strdup(user);
	}

	// user + password: look for a record with both matching
        for (gb_userpath = GB_find(gb_userdb,"user",user,down_2_level);
             gb_userpath;
             gb_userpath = GB_find(gb_user,"user",user,down_level|search_next) ) {

                gb_user  = GB_get_father(gb_userpath);
		gb_field = GB_find(gb_user,"password",0,down_level);
		if (!strcmp(GB_read_string(gb_field),password)) {
                	gb_user  = GB_get_father(gb_userpath);
			gb_field = GB_find(gb_user,"userpath",0,down_level);
			return GB_read_string(gb_field);
		}
        }
	return NULL;
}

/*************************************************************************************
  WRITE USER FIELD INFORMATION into user database
	a non existing field is being created
							return error message or NULL
							no authorisation here!
*************************************************************************************/
GB_ERROR OS_write_user_info_string(char *userpath, char *fieldname, char *content){
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends
	GBDATA *gb_userpath = GB_find(gb_userdb,"userpath",userpath,down_2_level);
	if (!gb_userpath) return "Userpath not found (1)!!";

	GBDATA *gb_user = GB_get_father(gb_userpath);
	GBDATA *gb_field = GB_find(gb_user,fieldname,0,down_level);
	if (!gb_field) 	gb_field = GB_create(gb_user,fieldname,GB_STRING);

	return GB_write_string(gb_field,content);
}
/*************************************************************************************
  WRITE USER FIELD INFORMATION (INTEGER) into user database
	a non existing field is being created
							return error message or NULL
							no authorisation here!
*************************************************************************************/
GB_ERROR OS_write_user_info_int(char *userpath, char *fieldname, int content){
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends
	GBDATA *gb_userpath = GB_find(gb_userdb,"userpath",userpath,down_2_level);
	if (!gb_userpath) return "userpath not found (2)!!";

	GBDATA *gb_user = GB_get_father(gb_userpath);
	GBDATA *gb_field = GB_find(gb_user,fieldname,0,down_level);
	if (!gb_field) 	gb_field = GB_create(gb_user,fieldname,GB_INT);

	return GB_write_int(gb_field,content);
}
/*************************************************************************************
  WRITE USER FIELD INFORMATION into user database WITH EXISTING GB_USER
	a non existing field is being created
							return error message or NULL
							no authorisation here!
*************************************************************************************/
GB_ERROR OS_write_gb_user_info_string(GBDATA *gb_user, char *fieldname, char *content){
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	GBDATA *gb_field = GB_find(gb_user,fieldname,0,down_level);
	if (!gb_field) 	gb_field = GB_create(gb_user,fieldname,GB_STRING);

	return GB_write_string(gb_field,content);
}
/*************************************************************************************
  WRITE USER FIELD INFORMATION INT into user database WITH EXISTING GB_USER
	a non existing field is being created
							return error message or NULL
							no authorisation here!
*************************************************************************************/
GB_ERROR OS_write_gb_user_info_int(GBDATA *gb_user, char *fieldname, int content){
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	GBDATA *gb_field = GB_find(gb_user,fieldname,0,down_level);
	if (!gb_field) 	gb_field = GB_create(gb_user,fieldname,GB_INT);

	return GB_write_int(gb_field,content);
}

/*************************************************************************************
  validate dailypw and return user data if valid
*************************************************************************************/
char * OS_read_dailypw_info(char *dailypw, char *fieldname){
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends
	GBDATA *gb_dailypw = GB_find(gb_userdb,"dailypw",dailypw,down_2_level);
	if (!gb_dailypw) return ORS_export_error("Dailypw %s does not exists",dailypw);

	GBDATA *gb_user  = GB_get_father(gb_dailypw);
	GBDATA *gb_date = GB_find(gb_user,"dailypw_date",0,down_level);
	if (!gb_date) return ORS_export_error("Dailypw_date does not exist");
	GB_read_string(gb_date);
	// TODO: if (gb_date < today)

	GBDATA *gb_field = GB_find(gb_user,fieldname,0,down_level);
	if (!gb_field) return 0;
	return GB_read_string(gb_field);
}

/*****************************************************************************
  RETURN LIST OF SUBUSERS of a userpath
		"levels" levels down, excluding "exclude",
				      excluding all down from "exclude_from"
  			list has format as follows:	name 1 name 1 ... 0
*****************************************************************************/
char *OS_list_of_subusers(char *userpath, int levels, char *exclude, char *exclude_from){
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	GBDATA *gb_userpath = GB_find(gb_userdb,"userpath",userpath,down_2_level);
	if (!gb_userpath) return ""; // ORS_export_error("User %s does not exists",userpath);

	char *users[500];
	int num_users=0;
	GBDATA *subuser;
	GBDATA *subuser_name;
	char *newpath, *read_data;
	int my_level_count = ORS_str_char_count(userpath,'/'),
	    level_count,
	    exclude_from_len;

	if (!strcmp(userpath,"/")) newpath = "/*"; // finds userpath
	else {
		newpath = GBS_string_eval(userpath,"*=*1/\\*",0);
		if (!exclude || strcmp(userpath,exclude))
			users[num_users++]=strdup(userpath);  // can't GB_find userpath
	}

	// search all users matching "userpath/*"
	if (exclude_from && !*exclude_from) exclude_from=NULL;
	if (exclude_from) exclude_from_len=strlen(exclude_from);
	for (	subuser_name = GB_find(gb_userdb, "userpath", newpath, down_2_level);
		subuser_name;
									// search 1 level down but parallel
		subuser_name = GB_find(subuser,"userpath", newpath, down_level | search_next) ) {

		read_data=GB_read_string(subuser_name);
		level_count=ORS_str_char_count(read_data,'/');
		// exclude
		if (level_count <= my_level_count + levels
			&& (!exclude || strcmp(read_data,exclude))
			&& (!exclude_from || strncmp(read_data,exclude_from,exclude_from_len))
			) {
			users[num_users++]=read_data;
		}
		if (num_users >= 500) break;

		subuser = GB_get_father(subuser_name);
	}

	// sort array names
	GBT_quicksort( (void **)users, 0, num_users, (long (*)(void *, void *, char *cd )) strcmp,0);

	// convert array into 1 string (seperated by 1)
	int length=0, i;
	static char *result = 0;
	delete result;
	for (i=0; i<num_users; i++) length+=strlen(users[i]);  // count length
	result = (char *)calloc(sizeof(char *),length+num_users+2);
	char *write=result;
	for (i=0; i<num_users; i++) {  // append strings
		strcpy(write,users[i]);
		delete(users[i]);
		write+=strlen(users[i])+1;
		if (i<num_users-1) *(write-1)=1;
	}

	return result;
}


/***********************
  set dailypw for user
***********************/
char *OS_set_dailypw(char */*userpath*/, char */*dailypw*/) {

	return "not implemented: set dailypw for user";
}

/*****************************************************************************
  ALLOWED TO CREATE USER?
			test user and all parent users for needed rights
			(max_users and max_user_depth)
			returns error message or NULL
*****************************************************************************/
char * OS_allowed_to_create_user(char *userpath, char *new_son) {
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	int superuser = OS_read_user_info_int(userpath,"is_superuser");
	if (superuser) return 0;	// superuser has all rights

	int max_users = OS_read_user_info_int(userpath,"max_users");
	if (max_users == 0) 		return strdup("You are not allowed to create a user.");

	int curr_users = OS_read_user_info_int(userpath,"curr_users");
	if (max_users <= curr_users) 	return strdup("You have already created your maximum number of users.");

	int my_slashes = ORS_str_char_count(userpath,'/');
	if (my_slashes == 0) return strdup("You have to be part of the user hierarchie to create a sub-user!");

	int new_slashes = ORS_str_char_count(new_son,'/');
	if (new_slashes <= my_slashes) return strdup("This sub-user is not allowed.");

	int new_depth = new_slashes - my_slashes;
	int max_user_depth = OS_read_user_info_int(userpath,"max_user_depth");
	if (new_depth > max_user_depth) return strdup("You are not allowed to create a sub-user in such a depth of hierarchie.");

	while (1) {
		userpath=ORS_get_parent_of(userpath);
		new_depth++;		// one level up
		if (!userpath) break;	// no more daddies
		max_users      = OS_read_user_info_int(userpath,"max_users");
		curr_users     = OS_read_user_info_int(userpath,"curr_users");
		max_user_depth = OS_read_user_info_int(userpath,"max_user_depth");
		if (max_users <= curr_users) return ORS_export_error("Your parent user %s is not allowed to create more users.", userpath);
		if (new_depth > max_user_depth)
			return ORS_export_error("Your parent user %s is not allowed to have users down to that hierarchie depth.", userpath);
	}
	return 0;
}

/*****************************************************************************
  CONSTRUCT SEL_USERPATH
	add a parent path and a name
							return strdup
*****************************************************************************/
char *OS_construct_sel_userpath(char *sel_par_userpath, char *sel_user) {
	if (!strcmp(sel_par_userpath,"/"))
		return GBS_global_string_copy("/%s",sel_user);
	return GBS_global_string_copy("%s/%s", sel_par_userpath, sel_user);
}

/*****************************************************************************
  WHO: list of logged in users
							returns list
*****************************************************************************/
void *who_file;
ORS_main *who_pm;
char *who_userpath;	// strdup of userpath

//****** sub function for hash_loop: all users
long OS_who_loop_all(const char *key, long val) {
	return OS_who_loop(key, val, 0);
}
long OS_who_loop_user(const char *key, long val) {
	return OS_who_loop(key, val, 1);
}

long OS_who_loop(const char */*key*/, long val, int mode) {	// mode = 0 : all,  mode = 1 : my_users only
	static char *time_string=0;
	struct passwd_struct *pws = (struct passwd_struct *)val;

	if ((unsigned long)(pws->last_access_time) < (GB_time_of_day() - (3600 * 2))) {
		printf("User %s expired.\n", pws->userpath/*, time_string*/);
		return 0;  // remove old user
	}
	if (val) {
		if (mode == 0 || 						// all users
		    ORS_is_parent_or_equal(who_userpath, pws->userpath) ) {	// or sub-users and me
			GBS_strcat(who_file, pws->userpath);
			GBS_chrcat(who_file, 1);
			delete time_string;
			time_string = ORS_time_and_date_string(DATE_TIME, pws->login_time);
			GBS_strcat(who_file, time_string);
			GBS_chrcat(who_file, 1);
			delete time_string;
			time_string = ORS_time_and_date_string(DATE_TIME, pws->last_access_time);
			GBS_strcat(who_file, time_string);
			GBS_chrcat(who_file, 1);
			GBS_strcat(who_file, pws->remote_host);
			GBS_chrcat(who_file, 1);
		}
	}

	return val; // do NOT remove item from list!
}

//*************************
char * OS_who(ORS_local *locs, char *userpath) {
	static char *str = 0;
	who_pm = (ORS_main *)locs->mh.parent->parent;
	who_file=GBS_stropen(10000);  // open memory file
	if (userpath) {
		if (ORS_str_char_count(userpath, '/') >= 2)
			who_userpath = ORS_get_parent_of(userpath);
		else	who_userpath = strdup(userpath);
		GBS_hash_do_loop((GB_HASH*)who_pm->pwds, OS_who_loop_user);
		delete who_userpath;
	}
	else
		GBS_hash_do_loop((GB_HASH*)who_pm->pwds, OS_who_loop_all);
	delete str;
	str = GBS_strclose(who_file);
	return str;	// ...return it
}

/*****************************************************************************
  USER HAS SUB-USERS?
					return 1 if has, otherwise 0
*****************************************************************************/
int OS_user_has_sub_users(char *userpath) {
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	char *user_pattern = ORS_sprintf("%s/*",userpath);
	GBDATA *sub_userpath = GB_find(gb_userdb,"userpath",user_pattern,down_2_level);
	delete user_pattern;
	if (sub_userpath) return 1;
	return 0;
}

/*****************************************************************************
  CHANGE SUB-USER COUNT for a sel_userpath
						change all parents
*****************************************************************************/
void OS_change_curr_user_counts(char *sel_userpath, int additor) {
	GB_transaction dummy(gb_userdb);  // keep transaction open until var scope ends

	while (1) {
		sel_userpath=ORS_get_parent_of(sel_userpath);
		if (!sel_userpath) break;	// no more daddies
		int curr_users = OS_read_user_info_int(sel_userpath,"curr_users");
		if ((additor < 0) && (curr_users < 1)) curr_users = 0;
		else curr_users = curr_users + additor;
		OS_write_user_info_int(sel_userpath,"curr_users",curr_users);
	}
	return;
}
