/* #############################################

	ORS_CLIENT  MAIN PART

	java_mode: output only binary data
	     else: output html pages

   ############################################# */

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


struct gl_struct ors_gl;

/*********************************************
  Fill structures with environment variables
**********************************************/
void get_indata(void) {
	char *request_method;
	char *indata;

	ors_gl.remote_host = getenv("REMOTE_HOST");
	if (!ors_gl.remote_host) quit_with_error(ORS_export_error("REMOTE_HOST not set"));

	ors_gl.remote_user = getenv("REMOTE_USER");
	if (ors_gl.remote_user == NULL) ors_gl.remote_user = "unknown";

	ors_gl.path_info   = getenv("PATH_INFO");
	if (ors_gl.path_info == NULL) ors_gl.path_info = "";

	request_method = getenv("REQUEST_METHOD");
	// GET means: contents are in variable QUERY_STRING
	if (!strcmp(request_method,"GET")) {
		indata=strdup(getenv("QUERY_STRING"));
	}
	// POST means: read contents from stdin
	else if (!strcmp(request_method,"POST")) {
		char *temp;
		if(!(temp=getenv("CONTENT_LENGTH")))
			quit_with_error(ORS_export_error("CONTENT_LENGTH not set"));
		int len=atoi(temp);
		indata=new char[len+1];
		indata[len]=0;
		fread(indata,len,1,stdin);
	}
	else quit_with_error(ORS_export_error("Unknown REQUEST_METHOD: %s",request_method));

	char *pos1, *pos2;
	int i;

	int ands    = ORS_str_char_count(indata, '&'); // number of '&'
	int slashes = ORS_str_char_count(ors_gl.path_info,'/');

	ors_gl.cgi_vars = (char **)calloc(sizeof(char *),ands+2+slashes + 10 ); // 10 more for set_cgi_var!

	// transform indata into cgi string array
	for (i=0, pos1 = indata; pos1; pos1=pos2, i++) {
		pos2	=	strchr(pos1,'&');
		if (pos2) 	*(pos2++) = 0;
		ors_gl.cgi_vars[i] = pos1;
	}

	// transform path_info into cgi string array (appending)
	// take only: /taken/taken/dismissed
	for (pos1 = ors_gl.path_info + 1; pos1; pos1=pos2, i++) {
		pos2 = strchr(pos1,'/');
		if (pos2) {
			char memo = (*pos2);
			(*pos2) = 0;
			ors_gl.cgi_vars[i] = strdup(pos1);
			(*pos2) = memo;
			pos2++;
		}
	}

	// translate characters and character codings (pos1=read, pos2=write)
	// + = ' ', %hh = hexcode
	char hex[]="0x00", c;
	int h;
	for (i=0, ors_gl.cgi_vars[i]; pos1=pos2=ors_gl.cgi_vars[i]; i++) {
		while (1) {
			switch(*pos1) {
				case '+': c=' '; pos1++; break;
				case '%': hex[2]=*(pos1+1); hex[3]=*(pos1+2); hex[4]=0;
			        	  pos1+=3;
				          sscanf(hex, "%i", &h);
				          c=(char) h;
				          break;
				default : c=*pos1; pos1++;
			}
			(*pos2)=c;
			if (c == 0) break;
			pos2++;
		}
	}

}

/*****************************************************************************
  CGI_VAR
	Returns value of cgi-variable "name=value"
  		if name is "bla|blubb", either bla or blubb are taken,
  		depending on which one is not ""; bla has higher priority
*****************************************************************************/
char * cgi_var(char *name) {
	int i;
	int len;
	char *pos=name, *pos2;

	while (1) {
		pos2 = strchr(pos+1,'|');
		len=strlen(name);
		if (!pos2) pos2 = name + len;

		for (i=0; ors_gl.cgi_vars[i]; i++) {
			if (!strncasecmp(ors_gl.cgi_vars[i],pos,pos2-pos) && ors_gl.cgi_vars[i][pos2-pos] == '='
									  && ors_gl.cgi_vars[i][pos2-pos+1])
				return &ors_gl.cgi_vars[i][pos2-pos+1];
		}
		if (*pos2 == 0) break;
		pos = pos2+1;
	}
	return "";  // important! never return NULL!
}

/*****************************************************************************
  CGI_VAR OR NIL
	Returns value of cgi-variable "name=value",
	"hello" when name="bla|blubb" and bla="" and blubb="hello"
	or nil if not avail
*****************************************************************************/
char * cgi_var_or_nil(char *name) {
	int i;
	int len;
	char *pos=name, *pos2;
	char *found_empty_string=0;

	while (1) {
		pos2 = strchr(pos+1,'|');
		len=strlen(name);
		if (!pos2) pos2 = name + len;

		for (i=0; ors_gl.cgi_vars[i]; i++) {
			if (!strncasecmp(ors_gl.cgi_vars[i],pos,pos2-pos) && ors_gl.cgi_vars[i][pos2-pos] == '=') {
				if (ors_gl.cgi_vars[i][pos2-pos+1]) return &ors_gl.cgi_vars[i][pos2-pos+1];
				if (!found_empty_string) found_empty_string=&ors_gl.cgi_vars[i][pos2-pos+1];
			}
		}
		if (*pos2 == 0) break;
		pos = pos2+1;
	}
	if (found_empty_string) return found_empty_string;
	return 0;
}
///////////////////////////
//	char *vars[] = {
//		"",
//		0
//
//	}
//		for (i=0; ors_gl.cgi_vars[i]; i++) {
//			if (!strncasecmp(ors_gl.cgi_vars[i],name,len) && ors_gl.cgi_vars[i][len] == '=')
//				return strdup(&ors_gl.cgi_vars[i][len+1]);
//		}

/*****************************************************************************
  SET CGI_VAR
		set value of existing cgi-var or create new cgi var
							does strdup
*****************************************************************************/
void set_cgi_var(char *name, char *content) {
	int i;
	int len;

	if (!name) return;
	len = strlen(name);

	// find position or new place
	for (i=0; ors_gl.cgi_vars[i]; i++) {
		if (!strncasecmp(ors_gl.cgi_vars[i],name,len) && ors_gl.cgi_vars[i][len] == '=') {
			break;
		}
	}

	void *file=GBS_stropen(10000);  // open memory file
	GBS_strcat(file, name);
	GBS_strcat(file, "=");
	GBS_strcat(file, content);
	ors_gl.cgi_vars[i] = GBS_strclose(file);
}

/*****************************************************************************
  Read a cgi_var and look its value up in a lib_file
					returns result or quits_with_error
*****************************************************************************/
char *OC_cgi_var_2_filename(char *cv, char *lib_file, char *kontext) {
	char *seek_name = cgi_var(cv);
	if (!*seek_name) quit_with_error(ORS_export_error("cgi_var '%s' missing at %s",cv,kontext));
	char *result=ORS_read_a_line_in_a_file(GBS_global_string_copy(ORS_LIB_PATH "%s",lib_file) ,seek_name);
	if (!*result) quit_with_error(ORS_export_error("'%s' not found in " ORS_LIB_PATH "%s at %s",seek_name,lib_file,kontext));
	return result;
}

/***********************************************
 ***** Debugging Function: Print Variables *****
 ***********************************************/
void print_content_lines(char * title) {
	int i;

	printf("<B> Debug-Info(%s) </B><BR><DIR>",title);
	printf("PathInfo=%s, DailyPw=%s, BT=%s UserPath=%s, RemoteUser=%s, RemoteHost=%s, Sel_UserPath=%s, Sel_UserName=%s<P>\n",
			ors_gl.path_info, ors_gl.dailypw, ors_gl.bt, ors_gl.userpath, ors_gl.remote_user, ors_gl.remote_host,
			ors_gl.sel_userpath, ors_gl.sel_username);

	for (i=0; ors_gl.cgi_vars[i] != NULL; i++) {
		printf(" <LI>[%2d] : %s\n<BR>",i,ors_gl.cgi_vars[i]);
	}
	printf("</DIR>");

}

/**********************************************************
 ***** Debugging Function: Export Environment for OCD *****
 **********************************************************/
void save_environment_for_debugger(void) {
	FILE *out;

	if ((out = fopen("/tmp/ors.env", "w")) == NULL) return;

	fprintf(out,"REQUEST_METHOD='GET'; export REQUEST_METHOD\n",ors_gl.remote_host);
	fprintf(out,"REMOTE_HOST='%s'; export REMOTE_HOST\n",ors_gl.remote_host);
	fprintf(out,"PATH_INFO='%s'; export PATH_INFO\n",ors_gl.path_info);

	fprintf(out,"QUERY_STRING='");
	int i;
	for (i=0; ors_gl.cgi_vars[i] != NULL; i++) {
		fprintf(out,"%s&",ors_gl.cgi_vars[i]);
	}
	fprintf(out,"bla'; export QUERY_STRING\n");

	fclose(out);

}

/*#####################################
  ##            M A I N              ##
  #####################################*/

int main(int /*argc*/, char **/*argv*/) {

	char *debug_command;
	char *html_new;
	memset((char*)&ors_gl,0,sizeof(struct gl_struct));

	print_header();  	// output transport header
	get_indata(); 		// analyse input and create string array ors_gl.cgi_vars

	ors_gl.dailypw  = cgi_var("dailypw");		//! daily password
	ors_gl.bt  =	cgi_var("bt");			//! name of base tree
	html_new 	= cgi_var("html");		//! new html page to be displayed
	debug_command	= cgi_var("debug");  		//! if not empty: debug mode (output debugging information)
	if (debug_command && *debug_command) ors_gl.debug=1;
	if (*cgi_var("java")) ors_gl.java_mode=1; 	//! if not empty: answer request in Java binary mode (instead of html text mode)


	save_environment_for_debugger();
	if (JAVA){
		GB_ERROR error = ORS_C_try_to_do_java_function();
		if (error) quit_with_error(error);
		exit (0);
	}


	init_server_communication();

	if (!JAVA) { // Debug-Mode: Output Variables
		if (ors_gl.debug) {print_content_lines("ANFANG"); }
	}

	// Login Page
	if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/LOGIN")) {		//! log into user databank
		ors_gl.userpath = cgi_var("userpath");	//! userpath from login page - all other pages look it up via dailypw
		OC_login_user(cgi_var("password"));
	}
	// standard <A HREF>
	else if (!strcmp( &ors_gl.path_info[strlen(ors_gl.path_info)-5],".html") ) {
		ors_gl.path_info[strlen(ors_gl.path_info)-5]=0;
		OC_output_html_page(&ors_gl.path_info[1]);
		exit(0);
	}
	else OC_dailypw_2_userpath();

	if (ors_gl.userpath == NULL || *(ors_gl.userpath) == 0 ||
            ors_gl.dailypw  == NULL || *(ors_gl.dailypw)  == 0) {
		ors_gl.userpath="";  // no admin functionality in html output!
		OC_output_html_page("unauth");		//! user authorisation failed
		exit(0);
	}

	ors_gl.sel_userpath  	= cgi_var("sel_userpath_man|sel_userpath"); 	//! selected user's path; manual entry is stronger than selection
	ors_gl.sel_userpath2 	= cgi_var("sel_userpath2_man|sel_userpath2");	//! 2nd selected user's path; destination user (e.g. for probe transfer)
	ors_gl.sel_user 	= cgi_var("sel_user");				//! pure name of selected user (without path)
	ors_gl.action 		= cgi_var("action");				//! determines specific action for pages with multiple purposes
	ors_gl.search 		= cgi_var("search");				//! determines search mode for search page
	ors_gl.probe_id 	= cgi_var("probe_id");				//! probe_id of current probe

	if (!strcmp(html_new,"user_pre") && !ORS_strcmp(ors_gl.userpath,"/guest") ) {
		// this line is read by makedoku: OC_output_html_page("user_pre") //! user's preferences - not visible as guest
		ors_gl.message = "Guest is not allowed to change his preferences.";
		OC_output_html_page("error");
		exit (0);
	}
	if (!strcmp(html_new,"usermenu") && ors_gl.max_users == 0 && ors_gl.curr_users == 0) {
		// this line is read by makedoku: OC_output_html_page("usermenu") //! main page for work on user functions - only visible if allowed to have sub-users
		ors_gl.message = "You are not allowed to have sub-users. <P>If you want to create sub-users, contact one of your parent users.";
		OC_output_html_page("error");
		exit (0);
	}

	// OC_write_log_message("");
	/*********************************************************************
	  Interpret pathinfo
	*********************************************************************/

	//////// SEL_USER_DETAIL /////////
	if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/SEL_USER_DETAIL")) {	//! read sel_user detail data from server for page display
		if (*ors_gl.sel_userpath) {
			OC_get_sel_userdata_from_server();
			char *par_userpath = cgi_var("sel_par_userpath");
			if (*par_userpath) ors_gl.sel_par_userpath = par_userpath;	// par_userpath was set
		} else {
			ors_gl.sel_par_userpath 	= cgi_var("sel_par_userpath");
			if (!ORS_strcmp(ors_gl.action,"user_create") && !*ors_gl.sel_par_userpath) {
				quit_with_error("To create a new user you have to select a parent user!");
			}
			ors_gl.sel_userpath 		= OC_build_sel_userpath(ors_gl.sel_par_userpath, ors_gl.sel_user);
			ors_gl.sel_pub_exist_max 	= strdup(ors_gl.sel_userpath);	// start value!
			ors_gl.sel_pub_content_max 	= strdup(ors_gl.sel_userpath);
		}
	}


	//////// MODIFY PREFERENCES /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/USER_PREF")) {	//! set user's preferences (values are used to change user's data)
		ors_gl.mail_addr    	= cgi_var("mail_addr");		//! user's mail addresses (seperated linewise)
		ors_gl.www_home    	= cgi_var("www_home");		//! user's www home page
		ors_gl.user_own_info    = cgi_var("user_own_info");	//! information on user (set by himself)
		ors_gl.username     	= cgi_var("username");		//! user's fullname
		ors_gl.pub_exist_def 	= cgi_var("pub_exist_def");	//! default value for existance publicity of user's data
		ors_gl.pub_content_def	= cgi_var("pub_content_def");	//! default value for content publicity of user's data
		ors_gl.user_ta_id	= atoi(cgi_var("user_ta_id"));	//! transaction id for a user database record

		int user_class 		= atoi(cgi_var("user_class"));	//! class of user (std user, author or superuser)
		if (user_class & 1 && ors_gl.is_author)    ors_gl.is_author    = 1; else ors_gl.is_author    = 0;
		if (user_class & 2 && ors_gl.is_superuser) ors_gl.is_superuser = 1; else ors_gl.is_superuser = 0;

		char *password2;
		ors_gl.password 	= cgi_var("password");		//! password, entered by user (login, preferences)
		password2 		= cgi_var("password2");		//! reentered password (must be equal)
		if (	 (!*(ors_gl.password) || !*(password2) )
		     && !(!*(ors_gl.password) && !*(password2) ) )
					quit_with_error(ORS_export_error("You have to enter password two times!"));
		if (ORS_strcmp(ors_gl.password,password2))
					quit_with_error(ORS_export_error("Reentered password differs!"));
		work_on_user();
		exit (0); // never reached
	}

	//////// CREATE/MODIFY USER (depending on cgi_var("action") /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/SEL_USER_EDIT")) {	//! create/modify sel_user, depending on cgi_var "action"
		ors_gl.sel_par_userpath 	= cgi_var("sel_par_userpath");	//! sel_user's parent userpath (his userpath without his name)
		ors_gl.sel_username     	= cgi_var("sel_username");	//! sel_user's fullname
		ors_gl.sel_mail_addr    	= cgi_var("sel_mail_addr");	//! sel_user's mail addresses (seperated linewise)
		ors_gl.sel_www_home    		= cgi_var("sel_www_home");	//! sel_user's www home page
		ors_gl.sel_user_info    	= cgi_var("sel_user_info");	//! information on sel_user (set by parent user)
		ors_gl.sel_pub_exist_max 	= cgi_var("sel_pub_exist_max");		//! sel_user's maximum level of existance publicity
		ors_gl.sel_pub_content_max     	= cgi_var("sel_pub_content_max");	//! sel_user's maximum level of content publicity
		ors_gl.sel_max_users    	= atoi(cgi_var("sel_max_users"));	//! sel_user's maximum number of sub-users
		ors_gl.sel_max_user_depth    	= atoi(cgi_var("sel_max_user_depth"));	//! sel_user's maximum level for creating sub-users
		ors_gl.sel_user_ta_id		= atoi(cgi_var("sel_user_ta_id"));	//! transaction id for a sel_user database record
		int sel_user_class 		= atoi(cgi_var("sel_user_class"));	//! user class of sel_user (std user, author or superuser)
		if (sel_user_class & 1) ors_gl.sel_is_author    = 1; else ors_gl.sel_is_author    = 0;
		if (sel_user_class & 2) ors_gl.sel_is_superuser = 1; else ors_gl.sel_is_superuser = 0;

		if (!*ors_gl.action) quit_with_error(ORS_export_error("SEL_USER_EDIT: no action was set before!"));
		char *password2;
		ors_gl.sel_password = cgi_var("sel_password");			//! password, entered for sel_user (create/modify sel_user)
		password2           = cgi_var("sel_password2");			//! reentered password (must be equal to password)
		if (*(ors_gl.sel_password) && *(password2) && strcmp(ors_gl.sel_password,password2))
			quit_with_error(ORS_export_error("Reentered password differs!"));

		if (!strcasecmp(ors_gl.action,"USER_CREATE")) {
			if (!*(ors_gl.sel_password)) 			quit_with_error(ORS_export_error("You have to setup a password!"));
			if (!*(password2)) 				quit_with_error(ORS_export_error("Please reenter the password!"));
			work_on_sel_user("CREATE");
		}
		else if (!strcasecmp(ors_gl.action,"USER_MODIFY")) {
			work_on_sel_user("MODIFY");

		}else	quit_with_error(ORS_export_error("SEL_USER_EDIT: unknown action '%s'!",ors_gl.action));

		exit (0); // never reached
	}

	//////// DELETE USER  /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/SEL_USER_DELETE")) {		//! delete selected user from user database
		work_on_sel_user("DELETE");
		exit (0);  // never reached
	}

	//////// SEL_USER INFO  /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/SEL_USER_INFO")) {		//! read ORS information on selected user and display it on the ORS user info page
		if (*ors_gl.sel_userpath)	OC_get_sel_userdata_from_server();  // read sel_user data for page display
		OC_output_html_page("user_info");					//! show details of user (public information for anyone)
		exit (0);
	}

	//////// NEW_PROBE /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/NEW_PROBE") ||	//! verify entered probe sequence and calculate target sequence
		 !ORS_strncase_tail_cmp(ors_gl.path_info,"/CLONE_PROBE")) {	//! verify entered probe sequence and calculate target sequence (clone)
		OC_read_probedb_fields_into_pdb_list();
		ors_gl.sequence   	= cgi_var("p_sequence");		//! sequence information of probe
		ors_gl.target_seq 	= cgi_var("p_target_seq");		//! target sequence information of probe (inverted sequence)
		ors_gl.allowed_bases 	= cgi_var("p_allowed_bases");		//! these bases are allowed when entered sequence is verified
		if (*ors_gl.sequence == 0 && *ors_gl.target_seq == 0) 	quit_with_error("Please enter either sequence or target sequence.");
		if (*ors_gl.sequence) 	OC_normalize_seq(ors_gl.sequence, ors_gl.allowed_bases);
		if (*ors_gl.target_seq)	OC_normalize_seq(ors_gl.target_seq, ors_gl.allowed_bases);
		if (*ors_gl.sequence && *ors_gl.target_seq && !ORS_seq_matches_target_seq(ors_gl.sequence, ors_gl.target_seq, 1))
			quit_with_error("Sequence and target sequence do not match! Just enter one field, the other is being calculated.");

		OC_calculate_seq_and_target_seq(&ors_gl.sequence, &ors_gl.target_seq, ors_gl.allowed_bases);
		set_cgi_var("p_sequence", ors_gl.sequence);	// write back to environment

		// does this sequence already exist?
		if (!ORS_strcmp(cgi_var("anyway"), "")) { 	//! if set to != "": don't care about warning (e.g. create probe although sequence exists)
			OC_get_pdb_fields_from_cgi_vars("all");		// get values from html page (here: only sequence)
			OC_send_pdb_fields_to_server("entered");	// send the sequence to probedb server
			ors_gl.list_of_probes = OC_probe_query(100);	// and look for equal seqs

			if (ors_gl.list_of_probes && *ors_gl.list_of_probes) {
				OC_output_html_page("probe_seq_exists");	//! show probes with equal sequence information
				exit(0);
			}
		}

		set_cgi_var("p_target_seq", ors_gl.target_seq);	// write back AFTER seq testing (otherwise target_seq would be searched for, too)

		// no, is a new sequence
		OC_pdb_list *pdb_4gc2at = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_4gc_2at");
		pdb_4gc2at->content = ORS_calculate_4gc_2at(ors_gl.sequence);
		OC_pdb_list *pdb_gc_ratio = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_gc_ratio");
		pdb_gc_ratio->content = ORS_calculate_gc_ratio(ors_gl.sequence);

		// clone probe: use old sequence
		if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/CLONE_PROBE")) {
			OC_get_pdb_fields_from_server(cgi_var("cloned_probe_id"), "clone" );	//! probe_id of to be cloned probe
			OC_pdb_list *pdb_seq = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_sequence");
			OC_pdb_list *pdb_tar = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_target_seq");
			pdb_seq->content = strdup(ors_gl.sequence);
			pdb_tar->content = strdup(ors_gl.target_seq);
			OC_pdb_list *pdb_4gc2at = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_4gc_2at");
			pdb_4gc2at->content = ORS_calculate_4gc_2at(pdb_seq->content);
			OC_pdb_list *pdb_gc_ratio = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_gc_ratio");
			pdb_gc_ratio->content = ORS_calculate_gc_ratio(pdb_seq->content);
		}
	}

	//////// NEW_PROBE_DATA /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/NEW_PROBE_DATA")) {		//! create a new probe using the entered fields
		OC_read_probedb_fields_into_pdb_list();		// create ors_gl.pdb_list if not done yet
		OC_get_pdb_fields_from_cgi_vars("create");	// get values from html page
		OC_send_pdb_fields_to_server("create");		// and send them to probedb server
		OC_work_on_probe("create");
	}
	//////// PROBE_UPDATE /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/PROBE_UPDATE")) {		//! modify probe using probe_id
		OC_read_probedb_fields_into_pdb_list();		// create ors_gl.pdb_list if not done yet
		OC_probe_select(ors_gl.probe_id);
		OC_get_pdb_fields_from_cgi_vars("all");		// get values from html page
		OC_send_pdb_fields_to_server("entered");
		OC_work_on_probe("update");
	}
	//////// PROBE_DELETE /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/PROBE_DELETE")) {		//! delete probe using probe_id
		OC_probe_select(ors_gl.probe_id);
		OC_work_on_probe("delete");
	}
	//////// PROBE_SELECT /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/PROBE_SELECT")) {		//! find an existing probe and set probe_id for the next page
	}

	//////// PROBE_FIELD_SEARCH /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/PROBE_FIELD_SEARCH")) {	//! find existing probes by field values (using wildcards)
		OC_read_probedb_fields_into_pdb_list();		// create ors_gl.pdb_list if not done yet
		OC_get_pdb_fields_from_cgi_vars("all");		// get values from html page
		OC_send_pdb_fields_to_server("entered");	// and send the entered fields to probedb server
		ors_gl.list_of_probes = OC_probe_query(100);
		OC_output_html_page("probe_search_result");	//! show probe result list of probe field search
		exit(0);
	}

	//////// PROBE_DETAIL /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/PROBE_DETAIL")) {		//! provide all probe fields for display on html page
		OC_read_probedb_fields_into_pdb_list();		// create ors_gl.pdb_list if not done yet
		OC_get_pdb_fields_from_server(ors_gl.probe_id, "all");
		OC_pdb_list *pdb_seq = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_sequence");
		OC_pdb_list *pdb_tar = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_target_seq");
		if (!ors_gl.target_seq || !*ors_gl.target_seq) {
			OC_calculate_seq_and_target_seq(&pdb_seq->content, &pdb_tar->content, NULL);
		}
		OC_pdb_list *pdb_4gc2at = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_4gc_2at");
		pdb_4gc2at->content = ORS_calculate_4gc_2at(pdb_seq->content);
		OC_pdb_list *pdb_gc_ratio = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_gc_ratio");
		pdb_gc_ratio->content = ORS_calculate_gc_ratio(pdb_seq->content);
	}

	//////// PROBE_USER_TRANSFER /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/PROBE_USER_TRANSFER")) {	//! transfer all probes of sel_userpath to sel_userpath2

		OC_probe_user_transfer(	ors_gl.sel_userpath, ors_gl.sel_userpath2, "*");	// all probes // TODO: select probe
		ors_gl.message = ORS_export_error("Probes of user %s have been transferred to user %s.", ors_gl.sel_userpath, ors_gl.sel_userpath2 );
		OC_output_html_page("success");
		exit (0);
	}

	//////// PROBE_MATCH /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/PROBE_MATCH")) {		//! request probe match query result from server

		//if (!ORS_strcmp(ors_gl.userpath,"/guest")) {
		//	ors_gl.message = "You are not allowed to use probe match function.";
		//	OC_output_html_page("error");
		//	exit (0);
		//}

		char *search_string	= cgi_var("search_string"), 	//! entered search string for probe match
		     *c_complement	= "",
		     *c_reversed  	= cgi_var("reversed"),		//! probe match parameter: reversed order
		     *c_weighted_mis	= cgi_var("weighted_mis"),	//! probe match parameter: weighted mismatches
		     *c_max_hits  	= cgi_var("max_hits"), 		//! probe match parameter: maximum number of hits
		     *c_mismatches	= cgi_var("mismatches"),	//! probe match parameter: number of mismatches
		     *c_server_nr	= cgi_var("server_nr");		//! number of desired server (as in ors_lib/server_names)

		int len       = strlen(search_string),
		    server_nr = atoi(c_server_nr);

		int mismatches   = 0,
		    reversed     = 0,
		    complement   = 0,
		    weighted_mis = 0,
		    max_hits     = atoi(c_max_hits);

		if (strcmp(c_mismatches,"none")) mismatches=atoi(c_mismatches);
		if (*c_reversed)     reversed     = 1;
		if (*c_complement)   complement   = 1;
		if (*c_weighted_mis) weighted_mis = 1;
		if (max_hits == 0)    max_hits     = 1000; // TODO: zentralen Defaultwert (auch in die html-Seite)

		if (!*search_string)
			quit_with_error(ORS_export_error("You must specify a search string!"));
		// length = 10; for each length+3 one more mismatch allowed
		if (mismatches != 0 && (len-10) / 3 < mismatches)
			quit_with_error(ORS_export_error("Too much mismatches for a sequence length of %i. Maximum is %i.",len,(len-10)/3 ));

		probe_match(	server_nr, search_string, mismatches,
				weighted_mis, reversed, complement,
				max_hits);
		if (*cgi_var("java_server")) {
			char tmpfile[2048];
			char *ffile = strdup(ors_gl.dailypw);
			ORS_C_make_clean(ffile);
			sprintf(tmpfile,ORS_TMP_PATH "ovp_%s",ffile);
			delete ffile;
			char *bin_tree_file = OC_cgi_var_2_filename("bt","tree_names","BIN_TREE");
			char *bin_tree_path = GBS_global_string_copy(ORS_LIB_PATH "%s",bin_tree_file);

			output_java_match_result(tmpfile,bin_tree_path);
			OC_output_html_page("look_java");
		}else{
			OC_output_html_page("match_result");	//! show result list for probe match query (with html references to details)
		}
		exit (0);
	}
	//////// SAVE_PROBEDB /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/SAVE_PROBEDB")) {		//! request a database save for probe db
		OC_save_probedb();
		ors_gl.message = "probe database has been saved.";
		OC_output_html_page("success");
		exit (0);
	}

	//////// SAVE_USERDB /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/SAVE_USERDB")) {		//! request a database save for user db
		OC_save_userdb();
		ors_gl.message = "user database has been saved.";
		OC_output_html_page("success");
		exit (0);
	}

	////////// GET SPECIES: Show species details /////////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/GS")) {		//! get species details
		ors_gl.sel_species = cgi_var("sp");	//! short name of selected species

		OC_output_html_page("species_detail");	//! show species details for selected species
		exit (0);
	}

	//////// LOGOUT /////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/LOGOUT")) {		//! logout from user database; dailypw is no longer valid
		OC_logout_user();
		exit (0); // never reached
	}

	////////// SEND MAIL TO AUTHOR /////////////
	else if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/MAIL")) {		//!  send mail to author, never used
		char *subject, *body;
		subject = cgi_var("subject");		//! subject of mail message
		body    = cgi_var("body");		//! body of mail message

		// TODO: mail verschicken

		OC_output_html_page("mail_sent");		//! status message: mail was sent
		exit (0);
	}



	if (!JAVA) {
		if (html_new && *html_new) {
			if ((!ORS_strcmp(ors_gl.userpath,"guest") || !ORS_strcmp(ors_gl.userpath,"/guest")) && !ORS_strcmp(html_new,"mainmenu"))
				OC_output_html_page("mainmenu_guest");
			else
				OC_output_html_page(html_new);
		} else {
			printf("KEINE SEITE ANGEWAEHLT <P> \n");
			printf("PATH_INFO=%s <P>\n",ors_gl.path_info);
		}

		if (ors_gl.debug) {print_content_lines("ENDE"); }
	}

	aisc_close(ors_gl.link);

	return 0;
}
