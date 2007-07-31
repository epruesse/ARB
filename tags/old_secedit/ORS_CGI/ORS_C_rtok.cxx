/* 
########################################
#                                      #
#    ORS_CLIENT:  REPLACEMENT TOKENS   #
#    html output functions             #
#                                      #
######################################## 
attention: beware of too many if else branches!!

there are comments in this file, which are used for automatic documentation generating!
they look like: //! that's the comment
they have to be on the same line with the variable or whatever is to be docu'd
*/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <arbdb.h>

#include <ors_client.h>
#include <client.h>
#include <servercntrl.h>
#include <ors_lib.h>

#include "cat_tree.hxx"
#include "ors_c_java.hxx"
#include "ors_c.hxx"
#include "ors_c_proto.hxx"


/*******************************************************************************
	Send Html Page (with replacements)
********************************************************************************/
void OC_output_html_page(char * new_html) {

	char *html=GBS_string_eval(new_html,"*=../templates/*1.html",0);  // append ".html"
	
	char *page=GB_read_file(html);
	
	if (!page) {
		if (!ORS_strcmp(new_html,"error")) {
			printf("ERROR: %s\nerror.html not available for this message\n",ors_gl.message);
			exit(0);
		}
		quit_with_error(ORS_export_error("HTML page not found: %s",html));
	}

	if (!ORS_strcmp(new_html,"error")) {
		if (ors_gl.message) 
			OC_write_log_message(ORS_export_error("error: %s",ors_gl.message));
		else
			OC_write_log_message("error!");
	}
	else if (!ORS_strcmp(new_html,"success")) {
		if (ors_gl.message) 
			OC_write_log_message(ORS_export_error("ok: %s",ors_gl.message));
		else
			OC_write_log_message("ok!");
	}
	else OC_write_log_message(ORS_export_error("html: %s",new_html));

	OC_output_html_buffer(page, html);
	
	/***************************
	  append EDIT button etc!!!
	****************************/
	int i;
	
	if (ors_gl.is_author || !ORS_strncmp(new_html,"develop/") || !ORS_strncmp(new_html,"develop/")) {
		printf("<HTML> <FORM METHOD=\"GET\" ACTION=\"http:");
		int count=ORS_str_char_count(ors_gl.path_info,'/');
		for (i=0; i<count; i++) printf("../");
		printf("ed.sh\"> \n");
		printf("<INPUT NAME=\"edit\" TYPE=\"hidden\" VALUE=\"%s.html\">\n",new_html);
		printf("<INPUT TYPE=\"submit\" VALUE=\"EDIT\"> \n ");
		
	}
	if (ors_gl.is_author) {
		printf("<B> <A HREF=http:");
		if (strchr(new_html,'/')) printf("../");   // if old pathinfo had had a path instead of just a name
		int count=ORS_str_char_count(ors_gl.path_info,'/');
		for (i=0; i<count; i++) printf("../");
		printf("../make.html>MAKE </A> - \n");
		
		printf("<A HREF=http:");
		if (strchr(new_html,'/')) printf("../");
		count=ORS_str_char_count(ors_gl.path_info,'/');
		for (i=0; i<count; i++) printf("../");
		printf("../index.html>LOGIN </A>\n");
		printf(" \n");
		
		printf("<BR> <TEXTAREA COLS=70 ROWS=12>\n");
		printf("%s",a2html(GB_read_file(html)));
		printf("</TEXTAREA><BR>%s.html</FORM>",new_html);
		printf("\n");
	}


}
	
/*******************************************************************************
	Send Html Buffer (with replacements)
				needs content and name of "this"
********************************************************************************/
void OC_output_html_buffer(char *page, char *html) {

char *pos, *replace;
int i;

while (1) {
	pos=strchr(page,'#');
	if (!pos) break;

	fwrite(page,pos - page,1,stdout);  // output to #-position
	replace=NULL;
	if      (!ORS_strncmp(pos,"##DAILYPW##"))	replace=ors_gl.dailypw;		//! daily password

	else if (!ORS_strncmp(pos,"##READ_USER_FIELD##") && ors_gl.is_superuser) {	//! read a field (name in cgi_var fieldname)
		char *fieldname = cgi_var("fieldname");	 //! name of user database field (superuser only)
		if (*fieldname) replace=OC_read_user_field(ors_gl.sel_userpath,fieldname);
		else replace="Fieldname undefined";
	}
	else if (!ORS_strncmp(pos,"##WHO##")) {						//! list of logged in users (as text)
		if (!ors_gl.list_of_who)
			ors_gl.list_of_who = list_of_users("who"); 
		OC_output_html_table table;
		table.create();
		table.border();
		table.output(ors_gl.list_of_who_my, 4, "userpath,login time,last access,remote host", "who is on");
		// output_list_as_2_col_table(ors_gl.list_of_who, 20, "\t", "\n");
		replace="";
	}
	else if (!ORS_strncmp(pos,"##WHO_MY##")) {					//! list of user's logged in sub-users (as text)
		if (!ors_gl.list_of_who_my)
			ors_gl.list_of_who_my = list_of_users("who_my");
		OC_output_html_table table;
		table.create();
		table.border();
		table.output(ors_gl.list_of_who_my, 4, "userpath,login time,last access,remote host", "currently logged in are");

			// output_list_as_2_col_table(ors_gl.list_of_who_my, 20, "\t", "\n");
		replace="";
	}
	else if (!ORS_strncmp(pos,"##STRING:")) {  					//! output content of cgi_var (or ..._man)
		char *name = pos + 9;
		char *end  = strchr(name,'#');
		if (end && end > name) {
			*end=0;
			char name2[50];
			sprintf(name2,"%s_man|%s",name,name);
			replace=cgi_var(name2);
			*end='#';
		}
		else replace="";
	}
	else if (!ORS_strncmp(pos,"##FILE:")) {						//! output content of file, named by cgi_var
		char *name = pos + 7;
		char *end  = strchr(name,'#');
		if (end && end > name) {
			if (!ORS_strncmp(name,"this.html#")) printf("%s",a2html(GB_read_file(html)));
			else if (!ORS_strncmp(name,"logfile#")) printf("%s",a2html(GB_read_file("ors_lib/logfile")));
			else if (!ORS_strncmp(name,"logfile.cgi#")) printf("%s",a2html(GB_read_file("ors_lib/logfile.cgi")));
			else {
				char name2[50],path[50],name3[50];
				*end=0;
				sprintf(name2,"%s_man|%s",name,name);
				sprintf(path,"%s_path_man|%s_path",name,name);
				sprintf(name3,"%s%s",cgi_var(path),cgi_var(name2));
				*end='#';
				printf("%s",GB_read_file(name3));
			}
		}
		replace="";
	}

	else if (!ORS_strncmp(pos,"##USERPATH##"))	replace=ors_gl.userpath;	//! user's userpath (/user1/user2/me)
	else if (!ORS_strncmp(pos,"##USERNAME##"))	replace=ors_gl.username;	//! user's fullname
	else if (!ORS_strncmp(pos,"##MAIL_ADDR##"))	replace=ors_gl.mail_addr;	//! user's mail addresses (multiple lines)
	else if (!ORS_strncmp(pos,"##MAIL_ADDR_LINKS##")) {				//! user's mail addresses as list of links
		OC_output_links(ors_gl.sel_mail_addr, '\n', "mailto:", "<BR>");
		replace="";
	}
	else if (!ORS_strncmp(pos,"##WWW_HOME##"))	replace=ors_gl.www_home;	//! user's www home page
	else if (!ORS_strncmp(pos,"##USER_INFO##"))	replace=ors_gl.user_info;	//! information on user, set by parent user

	else if (!ORS_strncmp(pos,"##USER_OWN_INFO##"))	replace=ors_gl.user_own_info;	//! user's own information
	else if (!ORS_strncmp(pos,"##MAX_USERS##")) {					//! maximum number of allowed sub-users
		printf("%i",ors_gl.max_users); replace=""; }
	else if (!ORS_strncmp(pos,"##CURR_USERS##")) {					//! current number of sub-users
		printf("%i",ors_gl.curr_users); replace=""; }
	else if (!ORS_strncmp(pos,"##AVAIL_USERS##")) {				//! available number of new sub-users (max_users - curr_users)
		printf("%i",ors_gl.max_users - ors_gl.curr_users); replace=""; }
	else if (!ORS_strncmp(pos,"##MAX_USER_DEPTH##")) {				//! user's maximum number of sub-users levels
		printf("%i",ors_gl.max_user_depth);	replace=""; }
	else if (!ORS_strncmp(pos,"##REMOTE_USER##"))	replace=ors_gl.remote_user;	//! remote user (isn't available from env...)
	else if (!ORS_strncmp(pos,"##REMOTE_HOST##"))	replace=ors_gl.remote_host;	//! remote host (from cgi environment)

	else if (!ORS_strncmp(pos,"##USER_TA_ID##")) {					//! transaction id of user's data
		printf("%i",ors_gl.user_ta_id); replace=""; }
	else if (!ORS_strncmp(pos,"##SEL_USER_TA_ID##")) { 				//! transaction id of sel_user's data
		printf("%i",ors_gl.sel_user_ta_id); replace=""; }

	else if (!ORS_strncmp(pos,"##ACTION##"))	replace=ors_gl.action;		//! defines context (different action on same page)
	else if (!ORS_strncmp(pos,"##SEARCH##")) 
				replace=ORS_str_to_upper(ors_gl.search);		//! search mode (simple, std, full)
	else if (!ORS_strncmp(pos,"##MESSAGE##"))	replace=ors_gl.message;		//! message for message pages (error, success...)
	else if (!ORS_strncmp(pos,"##ORS_MESSAGE##"))	{				//! content of ORS message file (from ors_lib)
		static char *message=0;
		delete message;
		message=GB_read_file("ors_lib/message");
		if (message)
			printf("<MENU><B><PRE>%s</PRE></B></MENU>", message);
		replace="";
	}		 
	else if (!ORS_strncmp(pos,"##REM"))		replace="";			
									//##REM...##	//! remark; characters/lines between are not sent
	else if (!ORS_strncmp(pos,"##DATE_DD.MM.YY##"))					//! date of today in German format DD.MM.YY
							replace=ORS_time_and_date_string(DATE_DMY, 0);
	else if (!ORS_strncmp(pos,"##TIME_HH:MM##")) 					//! time of day in format HH:MM
							replace=ORS_time_and_date_string(TIME_HM, 0);

	else if (!ORS_strncmp(pos,"##AUTHOR_MAIL##")) {					//! author's mail address (from ors_lib)
		replace=ORS_read_a_line_in_a_file(ORS_LIB_PATH "/CONFIG","AUTHOR_MAIL");
	}

	else if (!ORS_strncmp(pos,"##PASSWORD_MIN_LENGTH##")) {					//! author's mail address (from ors_lib)
		printf("%i",PASSWORD_MIN_LENGTH);
		replace="";
	}

	else if (!ORS_strncmp(pos,"##DEBUG##")) {					//! debug mode - "yes" or ""
		if (ors_gl.debug) replace="yes"; else replace=""; }

	else if (!ORS_strncmp(pos,"##DEBUG_BUTTON##")) 	{				//! insert a debug button
		if (ors_gl.is_author) 
			replace="<INPUT TYPE=\"checkbox\" NAME=\"debug\" VALUE=\"1\">\n";
		else
			replace="";
	}

	/*************************************************************
	  BUTTONS 
	*************************************************************/
//	//	else if (!ORS_strncmp(pos,"##BUTTON_USERMENU##")) {
//	//		printf("<A HREF=\"new_html?html=mainmenu\"><IMG SRC=\"");
//	//		print_path_to_images();
//	//		printf("gif/but_usermenu.gif\" ALT=\"USERMENU\"></A>");
//	//		replace="";
//	//	}
	else if (!ORS_strncmp(pos,"##BUTTONS_BEGIN##")) replace="<CENTER><B><FONT SIZE=+1>";	//! html init sequence for buttons
	else if (!ORS_strncmp(pos,"##BUTTONS_END##")) 	replace="</CENTER></B></FONT>";		//! html trailer after buttons
	else if (!ORS_strncmp(pos,"##BUTTON_MAINMENU##")) {					//! button (different for guest and users)
		if (!ORS_strcmp(ors_gl.userpath,"/guest")) {
			replace="<A HREF=\"new_html?html=mainmenu3\">MAIN MENU</A>";	
		} else {
			replace="<A HREF=\"new_html?html=mainmenu\">MAIN MENU</A>";	
		}
	}
	else if (!ORS_strncmp(pos,"##BUTTON_USERMENU##")) replace="<A HREF=\"new_html?html=usermenu\">SUBUSERS</A>";	//! button
	else if (!ORS_strncmp(pos,"##BUTTON_PREFERENCES##")) replace="<A HREF=\"new_html?html=user_pre\">PREFERENCES</A>"; //! button
	else if (!ORS_strncmp(pos,"##BUTTON_HELP##")) 	replace="<A HREF=\"new_html?html=help/main\">HELP</A>";		//! button
	else if (!ORS_strncmp(pos,"##BUTTON_PROBE_MATCH##")) { 					//! button; transfers probe_id to next page
		printf("<A HREF=\"new_html?probe_id=%s&html=probe_match\">PROBE MATCH</A>",ors_gl.probe_id);
		replace=""; }
	else if (!ORS_strncmp(pos,"##BUTTON_LOGOUT##"))   replace="<A HREF=\"logout\">LOGOUT</A>";	//! button



	else if (!ORS_strncmp(pos,"##BULLET_SMALL##")) {						//! hyperlink to small bullet
			OC_output_html_buffer("<IMG SRC=\"##..IMG/##gif/bullet_yellow_small.gif\">",0);
			replace=""; }
	else if (!ORS_strncmp(pos,"##BULLET_BIG##")) {							//! hyperlink to big bullet
			OC_output_html_buffer("<IMG SRC=\"##..IMG/##gif/bullet_red.gif\">",0);
			replace=""; }
	else if (!ORS_strncmp(pos,"##ORS##")) {								//! hyperlink to ors picture
			if (ors_gl.is_author)
				OC_output_html_buffer("<A HREF=\"http:##..HREF/##dict-search.html\"><IMG SRC=\"##..IMG/##gif/ors.gif\" BORDER=0></A>",0);
			else
				OC_output_html_buffer("<IMG SRC=\"##..IMG/##gif/ors.gif\" BORDER=0>",0);
			replace=""; }
	else if (!ORS_strncmp(pos,"##ORS_SMALL##")) {							//! hyperlink to small ors picture
			OC_output_html_buffer("<IMG SRC=\"##..IMG/##gif/ors.gif\" HEIGHT=40 WIDTH=100>",0);
			replace=""; }

	/*************************************************************
	  OPTION LISTS
	*************************************************************/
	else if (!ORS_strncmp(pos,"##OPTION_WHO##")) {				//! option list of logged in users
		if (!ors_gl.list_of_who)
			ors_gl.list_of_who = list_of_users("who"); 
		output_list_as_html_options_special(ors_gl.list_of_who, ors_gl.userpath, 10, 1, 2);
		replace="";
	}
	else if (!ORS_strncmp(pos,"##OPTION_WHO_MY##")) {			//! option list of user's logged in sub-users (which normally includes his father and his brothers - except when root is his father)
		// TODO: geht noch nicht
		if (!ors_gl.list_of_who_my)
			ors_gl.list_of_who_my = list_of_users("who_my"); 
		output_list_as_html_options_special(ors_gl.list_of_who_my, ors_gl.userpath, 10, 1, 2);
		replace="";
	}
	else if (!ORS_strncmp(pos,"##OPTION_SEL_USER_CLASSES##")) {		//! option list of sel_user's possible classes
		int sa=ors_gl.sel_is_author;
		int  a=ors_gl.is_author;
		int ss=ors_gl.sel_is_superuser;
		int  s=ors_gl.is_superuser;

		printf("<OPTION VALUE=\"0\"");
		if (!sa && !ss) printf(" SELECTED");
		printf(">Standard User\n");
		if (sa || a) {
			printf("<OPTION VALUE=\"1\"");
			if (sa && !ss) printf(" SELECTED");
			printf(">Developer\n");
		}
		if (ss || s) {
			printf("<OPTION VALUE=\"2\"");
			if (ss && !sa) printf(" SELECTED");
			printf(">Superuser\n");
		}
		if ((sa && ss) || (a &&s)) {
			printf("<OPTION VALUE=\"3\"");
			if (ss && sa) printf(" SELECTED");
			printf(">Developer and Superuser\n");
		}
		replace="";
	}
	else if (!ORS_strncmp(pos,"##OPTION_USER_CLASSES##")) {		//! option list of user's possible classes
		int  a=ors_gl.is_author;
		int  s=ors_gl.is_superuser;

		printf("<OPTION VALUE=\"0\"");
		if (!a && !s) printf(" SELECTED");
		printf(">Standard User\n");
		if (a) {
			printf("<OPTION VALUE=\"1\"");
			if (a && !s) printf(" SELECTED");
			printf(">Developer\n");
		}
		if (s) {
			printf("<OPTION VALUE=\"2\"");
			if (s && !a) printf(" SELECTED");
			printf(">Superuser\n");
		}
		if (a && s) {
			printf("<OPTION VALUE=\"3\"");
			printf(" SELECTED");
			printf(">Developer and Superuser\n");
		}
		replace="";
	}

	else if (!ORS_strncmp(pos,"##OPTION_MY_USERS##")) {			//! option list of user's sub-users
		if (!ors_gl.list_of_subusers)
			ors_gl.list_of_subusers = list_of_users("my_users"); 
		output_list_as_html_options(ors_gl.list_of_subusers,NULL, 10);
		replace="";
	}

	else if (!ORS_strncmp(pos,"##OPTION_SEL_PARENTS##")) {			//! option list of sel_user's possible parents
		if (!ors_gl.list_of_sel_parents)
			ors_gl.list_of_sel_parents = list_of_users("sel_parents"); 
		output_list_as_html_options(ors_gl.list_of_sel_parents, ors_gl.sel_par_userpath, 10);
		replace="";
	}

	else if (!ORS_strncmp(pos,"##OPTION_ALL_USERS##")) {			//! option list of ALL users (may be many!)
		if (!ors_gl.list_of_all_users) 
			ors_gl.list_of_all_users = list_of_users("all"); 
		output_list_as_html_options(ors_gl.list_of_all_users,NULL, 10);
		replace="";
	}

	else if (!ORS_strncmp(pos,"##OPTION_SEL_PUB_EXIST_PARENTS##")) {	//! option list of sel_user's possible pub_exist levels
		char *temp=0;
		if (!ors_gl.sel_userpath || !*ors_gl.sel_userpath || ors_gl.sel_par_userpath) {
			temp=OC_build_sel_userpath(ors_gl.sel_par_userpath, ors_gl.sel_user);
		}
		else if (ors_gl.sel_userpath) temp=strdup(ors_gl.sel_userpath);
		if (temp) output_list_as_html_options(list_of_pub_parents(ors_gl.pub_exist_max, temp), ors_gl.sel_pub_exist_max, 1);
		replace="";
		delete temp;
	}

	else if (!ORS_strncmp(pos,"##OPTION_SEL_PUB_CONTENT_PARENTS##")) {	//! option list of sel_user's possible pub_content levels
		char *temp=0;
		if (!ors_gl.sel_userpath || !*ors_gl.sel_userpath || ors_gl.sel_par_userpath) {
			temp=OC_build_sel_userpath(ors_gl.sel_par_userpath, ors_gl.sel_user);
		}
		else if (ors_gl.sel_userpath) temp=strdup(ors_gl.sel_userpath);
		if (temp) output_list_as_html_options(list_of_pub_parents(ors_gl.pub_content_max, temp), ors_gl.sel_pub_content_max, 1);
		replace="";
		delete temp;
	}

	else if (!ORS_strncmp(pos,"##OPTION_PUB_EXIST_PARENTS##")) {		//! option list of user's possible default publishing levels (default value is selected)
		output_list_as_html_options(list_of_pub_parents(ors_gl.pub_exist_max, ors_gl.userpath), 
						ors_gl.pub_exist_def, 1);
		replace="";
	}

	else if (!ORS_strncmp(pos,"##OPTION_PUB_CONTENT_PARENTS##")) {		//! opion list of user's possible default publishing levels (default value is selected)
		output_list_as_html_options(list_of_pub_parents(ors_gl.pub_content_max, ors_gl.userpath), 
						ors_gl.pub_content_def, 1);
		replace="";
	}


	else if (!ORS_strncmp(pos,"##OPTION_PROBE_SECTION_TYPES##")) {		//! option list of possible types for etc section
		output_list_as_html_options(OC_read_file_into_list(ORS_LIB_PATH "section_types"), "", 0);
		replace="";
	}

	else if (!ORS_strncmp(pos,"##OPTION_SB_FIELDS##")) {			//! option list of fields of species database
		// TODO:
		// Optionlist der Fields der Species_Datenbank
		// selected ist der erste
		replace="<OPTION VALUE=name> Name\n<OPTION VALUE=\"date\"> Date\n<OPTION VALUE=\"full name\"> Full name\n";
	}

	else if (!ORS_strncmp(pos,"##OPTION_PB_FIELDS##")) {			//! option list of fields of probe database
		// TODO:
		// Optionlist der Fields der Probe_Datenbank
		// selected ist der erste
		replace="<OPTION VALUE=sequence> Sequence\n<OPTION VALUE=\"author\"> Developer\n<OPTION VALUE=\"name\"> Name\n";
	}

	else if (!ORS_strncmp(pos,"##OPTION_PB_FIELDS_ALL##")) {		//! option list of fields of probe database (incl. ALL)
		// TODO:
		// Optionlist der Fields der Probe_Datenbank including ALL
		// selected ist der erste
		replace="<OPTION VALUE=all> A L L\n<OPTION VALUE=sequence> Sequence\n<OPTION VALUE=\"author\"> Developer\n<OPTION VALUE=\"name\"> Name\n";
	}
	else if (!ORS_strncmp(pos,"##OPTION_SERVERS##")) {			//! option list of servers (according to ors_lib)
		char buffer[256];
		FILE 	*file;
		char	*p;

		file = fopen(ORS_LIB_PATH "server_names","r");
		if (!file) quit_with_error("Can't open file: " ORS_LIB_PATH "server_names");

		for (	p=fgets(buffer,255,file);
			p;
			p = fgets(p,255,file)) {

			printf("<OPTION VALUE=%s> %s\n",strtok(buffer," \t\n"),strtok(0," \t\n"));
		}
		fclose(file);
		replace="";
	}

	else if (!ORS_strncmp(pos,"##OPTION_TARGET_GENE##")) {			//! option list of target genes (according to what???)
		// TODO: woher sollen die target genes gelesen werden?	
		replace="<OPTION VALUE=16S> 16S\n<OPTION VALUE=23S> 23S\n<OPTION VALUE=5S> 5S\n<OPTION VALUE=Other> Other\n";
	}

	/*************************************************************
	  USER DATA
	*************************************************************/
	else if (!ORS_strncmp(pos,"##EXIST_PUB_MAX##")) {			//! user's maximum existance publishing level
		replace=ors_gl.pub_exist_max;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##CONTENT_PUB_MAX##")) {			//! user's maximum content publishing level
		replace=ors_gl.pub_content_max;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##EXIST_PUB_DEF##")) {			//! user's default existance publishing level
		replace=ors_gl.pub_exist_def;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##CONTENT_PUB_DEF##")) {			//! user's default content publishing level
		replace=ors_gl.pub_content_def;
		if (!replace) replace="";
	}

	/*************************************************************
	  SEL_USER DATA
	*************************************************************/

	else if (!ORS_strncmp(pos,"##SEL_PUB_EXIST_MAX##")) {			//! sel_user's maximum existance publishing level
		replace=ors_gl.sel_pub_exist_max;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_PUB_CONTENT_MAX##")) {			//! sel_user's maximum content publishing level
		replace=ors_gl.sel_pub_content_max;
		if (!replace) replace="";
	}

//		else if (!ORS_strncmp(pos,"##SEL_PUB_EXIST_DEF##")) {			// ! sel_user's default existance publishing level
//			replace=ors_gl.sel_pub_exist_def;
//			if (!replace) replace="";
//		}
//		else if (!ORS_strncmp(pos,"##SEL_PUB_CONTENT_DEF##")) {			// ! sel_user's default content publishing level
//			replace=ors_gl.sel_pub_content_def;
//			if (!replace) replace="";
//		}

	else if (!ORS_strncmp(pos,"##SEL_USER##")) {				//! sel_user's username (without path)
		replace=ors_gl.sel_user;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_USERPATH##")) {			//! sel_user's userpath (/user1/user2/sel_user)
		replace=ors_gl.sel_userpath;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_USERPATH2##")) {			//! 2nd (destination) sel_user's userpath (/user1/user2/sel_user)
		replace=ors_gl.sel_userpath2;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_PAR_USERPATH##")) {			//! sel_user's parent userpath (/user1/user2)
		replace=ors_gl.sel_par_userpath;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_USERNAME##")) {			//! sel_user's fullname
		replace=ors_gl.sel_username;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_MAIL_ADDR##")) {			//! sel_user's mail addresses (multiple lines)
		replace=ors_gl.sel_mail_addr;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_MAIL_ADDR_LINKS##")) {			//! sel_user's mail addresses as list of links
		OC_output_links(ors_gl.sel_mail_addr, '\n', "mailto:", "<BR>");
		replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_WWW_HOME##")) {			//! sel_user's www home page
		replace=ors_gl.sel_www_home;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_MAX_USERS##")) {			//! sel_user's maximum number of allowed sub-users
		printf("%i",ors_gl.sel_max_users);
		replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_CURR_USERS##")) {			//! sel_user's current number of sub-users
		printf("%i",ors_gl.sel_curr_users);
		replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_MAX_USER_DEPTH##")) {			//! sel_user's maximum number of sub-users levels
		printf("%i",ors_gl.sel_max_user_depth);
		replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_USER_INFO##")) {			//! information on sel_user, set by parent user
		replace=ors_gl.sel_user_info;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_USER_OWN_INFO##")) {			//! information on sel_user, set by himself
		replace=ors_gl.sel_user_own_info;
		if (!replace) replace="";
	}
	else if (!ORS_strncmp(pos,"##SEL_USER_CLASS##")) {			//! sel_user's class (author, superuser)
		int a=ors_gl.sel_is_author;
		int s=ors_gl.sel_is_superuser;

		if (!a && !s) 	printf("Standard User");
		if (a && !s) 	printf("Developer");
		if (!a && s) 	printf("Superuser");
		if (a && s) 	printf("Developer and Superuser");
		replace="";
	}

	/*************************************************************
	  PATH OPERATIONS
	*************************************************************/
	if (!ORS_strncmp(pos,"##..HREF/##")) {			//! compensate cgi-bin-path by repeating ../ often enough
		int count=ORS_str_char_count(ors_gl.path_info,'/');
		if (count)
		for (i=0; i<count-1; i++) printf("../");
		replace="";
	}else if (!ORS_strncmp(pos,"##BT##")) {			//! bin tree name
		replace=ors_gl.bt;
	}else if (!ORS_strncmp(pos,"##..IMG/##")) {			//! compensate cgi-bin-path (for images)
		int count=ORS_str_char_count(ors_gl.path_info,'/');
		for (i=0; i<count+1; i++) printf("../");
		replace="";
	}

	/*************************************************************
	  PROBE AND SPECIES DATA
	*************************************************************/
	else if (!ORS_strncmp(pos,"##PROBE_MATCH_RESULT##")) {		//! result of last probe match query
		output_html_match_result();
		replace=" ";
	}
	else if (!ORS_strncmp(pos,"##PROBE_SEARCH_RESULT##")) {		//! result of last probe field search
		OC_output_html_probe_list(10, "result of field search");
		replace=" ";
	}
	else if (!ORS_strncmp(pos,"##SPECIES_DATA##")) {		//! lovely species data from species server
		// TODO: species data direkt vom species-server holen
		// output_species_data(ors_gl.sel_species);
		printf("<PRE>\n");
		printf("---------------\nlovely data for %s\n--------------\n",ors_gl.sel_species);
		printf("</PRE>\n");
		replace=" ";
	}
	else if (!ORS_strncmp(pos,"##SPECIES_FULLNAME##")) {		//! fullname of species
		// TODO: get species fullname direktly from species-server 
		//       maybe data is already available from species_data
		// output_species_data(ors_gl.sel_species);
		printf("Fullnamus Publicus %s\n",ors_gl.sel_species);
		replace=" ";
	}
	else if (!ORS_strncmp(pos,"##PROBE_LIST_MY_LAST:")) {		//! list of user's last xxx probes with hyperlinks
		char *name = pos + 21;
		char *end  = strchr(name,'#');
		int nr;
		if (end && end > name) {
			*end=0;
			nr = atoi(name);
			*end='#';
		}
		else nr = 10;
		ors_gl.list_of_probes = OC_get_probe_list(1, nr);
		OC_output_html_probe_list(1, "list of probes");
		replace="";
	}
	else if (!ORS_strncmp(pos,"##PROBE_LIST_MY_ALL##")) {		//! complete list of user's probes with hyperlinks
		ors_gl.list_of_probes = OC_get_probe_list(1, 0);
		OC_output_html_probe_list(1, "list of all probes (of my own) (by date)");
		replace="";
	}

	else if (!ORS_strncmp(pos,"##PROBE_LIST_LAST:")) {		//! list of all user's probes with hyperlinks (last xxx)
		char *name = pos + 18;
		char *end  = strchr(name,'#');
		int nr;
		if (end && end > name) {
			*end=0;
			nr = atoi(name);
			*end='#';
		}
		else nr = 10;
		ors_gl.list_of_probes = OC_get_probe_list(2, nr);
		OC_output_html_probe_list(2, "list of probes (by date)");
		replace="";
	}

	else if (!ORS_strncmp(pos,"##PROBE_LIST_ALL##")) {		//! complete list of all user's probes with hyperlinks
		ors_gl.list_of_probes = OC_get_probe_list(2, 0);
		OC_output_html_probe_list(2, "list of probes (all)" );
		replace="";
	}

	else if (!ORS_strncmp(pos,"##PROBE_LIST_SEQ_EXISTS##")) {	//! list of probes with same sequence
		OC_output_html_probe_list(2, "list of probes with same sequence" );
		replace="";
	}

	else if (!ORS_strncmp(pos,"##PROBE_ID##")) replace=ors_gl.probe_id;	//! probe_id of current probe

	else if (!ORS_strncmp(pos,"##PROBE_NAME##")) {				//! name of current probe
		OC_probe_select(cgi_var("probe_id"));
		replace = OC_get_probe_field("", "p_probe_name");
	}

	else if (!ORS_strncmp(pos,"##PROBE_ALLOWED_BASES##")) {			//! string of allowed bases names for probe sequence entry
		replace = ors_gl.allowed_bases;
	}
	else if (!ORS_strncmp(pos,"##PROBE_ENTERED_SEQUENCE##")) {		//! currently entered sequence of probe
		replace = ors_gl.sequence;
	}
	else if (!ORS_strncmp(pos,"##PROBE_SEQUENCE##")) {			//! sequence of current probe
		OC_probe_select(cgi_var("probe_id"));
		replace = OC_get_probe_field("", "p_sequence");
	}
	else if (!ORS_strncmp(pos,"##PROBE_TA_ID##")) {				//! transaction id of probe's data
		printf("%i",ors_gl.probe_ta_id); replace=""; }

	else if (!ORS_strncmp(pos,"##CLONED_PROBE_NAME##")) {			//! name of probe, selected for cloning
		OC_probe_select(cgi_var("cloned_probe_id"));	//! id of probe, selected for cloning
		replace = OC_get_probe_field("", "p_probe_name");
	}

	else if (!ORS_strncmp(pos,"##CLONED_PROBE_ID##")) {			//! probe_id of probe, selected for cloning
		printf("%s",cgi_var("cloned_probe_id")); replace=""; }

	/*************************************************************
	  TABLE CONTENTS
	*************************************************************/
	else if (!ORS_strncmp(pos,"##TABLE_ALL_USERS##")) {			//! table of all users with links to info page and www_home
		if (!ors_gl.list_of_all_users) 
			ors_gl.list_of_all_users = list_of_users("all"); 
		// TODO: table ausgeben
		OC_output_html_table table;
		table.create();
		table.border();
		table.set_col_func(1, OC_html_userpath_link, 2);
		table.set_col_param(1, "INFO");
		table.set_col_func(3, OC_html_www_home, 2, 2);
		table.output(ors_gl.list_of_all_users, 1, ",ORS userpath,Full name", "all users");
		replace="";
	}
	else if (!ORS_strncmp(pos,"##TABLE_PROBE_DATA_CREATE##")) {		//! table of probedb data update fields for creation of a new probe
		OC_read_probedb_fields_into_pdb_list();
		OC_output_pdb_list(ors_gl.pdb_list,"create","table");
		replace="";
	}
	else if (!ORS_strncmp(pos,"##TABLE_PROBE_DATA_UPDATE##")) {		//! table of probedb data update fields (update of existing probe)
		OC_read_probedb_fields_into_pdb_list();
		OC_output_pdb_list(ors_gl.pdb_list,"all","table");
		replace="";
	}
	else if (!ORS_strncmp(pos,"##TABLE_PROBE_DATA_SEARCH##")) {		//! table of probedb data search fields (update of search fields)
		OC_read_probedb_fields_into_pdb_list();
		OC_output_pdb_list(ors_gl.pdb_list,"search","table");
		replace="";
	}
	else if (!ORS_strncmp(pos,"##TABLE_PROBE_DATA_SHOW##")) {		//! table of probedb data fields (view of existing probe)
		OC_read_probedb_fields_into_pdb_list();
		OC_output_pdb_list(ors_gl.pdb_list,"show","table");
		replace="";
	}


	if (replace) {
		printf("%s",replace);	
		page=strchr(pos+2,'#') + 2;  // end of search string
	}
	else {
		putchar('#');
		page=pos+1;
	}
} //while

printf("%s",page);   // output rest of page

}

