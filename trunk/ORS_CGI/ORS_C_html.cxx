/*
#################################
#                               #
#    ORS_CLIENT:  HTML          #
#    html output functions      #
#                               #
#################################

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


/*****************************************************
  Output error message (readable in html and stout)
*****************************************************/
void quit_with_error(const char * message) {
	if (JAVA) {
		printf("%c%s",33,message);
	} else {
		ors_gl.message = strdup(message);
		OC_output_html_page("error");		//! error message
	}
	exit(0);
}

/*************************************
  convert special characters to html
**************************************/
char * a2html(char *text) {
	int i;
	char *buffer, *pos, *replace;
	int len=strlen(text);
	pos = buffer = new char[len*2+1];

	for (i=0; i<len; i++) {
		switch(text[i]) {
			case '<':	replace="&#60;"; break;
			case '>':	replace="&#62;"; break;
			default:	replace=0;
//TODO: Tabelle machen fuer alle ASCII-Zeichen mit String oder 0
		}
		if (replace) {
			strcpy(pos,replace);
			pos+=strlen(replace);
		}
		else {
			*pos = text[i];
			pos++;
		}
	}
	*pos = 0;
	return strdup(buffer);
}

/**********************************
  print header for html transport
***********************************/
void print_header(void) {
	if (JAVA)
		printf("Content-type: text/plain\n\n");
	else
		printf("Content-type: text/html\n\n");
}

/*****************************************************************************
  OUTPUT LIST AS HTML OPTIONS
	outputs a 1-seperated list as html option list
	selected may be a string (the selected option) or NULL
	mode  0: show full path as selection (and return full path)
	mode  1: show only basename, but return full path
	mode 10: mode 0 without (/|PUBLIC) mapping
	mode 11: mode 1 without (/|PUBLIC) mapping
*****************************************************************************/
void output_list_as_html_options(char *list, char *selected, int public_mode) {
	output_list_as_html_options_special(list, selected, public_mode, 1, 1);
}

/*****************************************************************************
  OUTPUT LIST AS HTML OPTIONS
		additional functionality: some = 2: take every 2nd field
					  ignore = 1: jump over 1st field
		mode 0: output as option list
		mode 1: output as table
*****************************************************************************/
void output_list_as_html_options_special(char *list, char *selected, int public_mode, int from, int every) {

	char *read=list, *end;
	int i;

	if (!list) return;
	if (!*list && selected && *selected) {
		printf("<OPTION VALUE=\"%s\">%s", selected, selected);
		return;
	}

	// ignore fields to start from "from" field
	if (from > 1)
		for (i=1; i<from; i++) {
			end=strchr(read,1);
			if (!end) end=strchr(read,0);
			if (!end) break;
			read=end+1;
		}

	while (1) {
		end=strchr(read,1);
		if (end && *(end+1) == 0) break;  // this happens if there is a trailing 1
		if (!end) end=strchr(read,0);
		if (!end) break;  // never reached
		printf("<OPTION VALUE=\"");
		fwrite(read,end - read,1,stdout);  // output to end marker position
		putchar('"');

		if (selected &&
		    !ORS_strncmp(read,selected) &&
		    strlen(selected) == end-read) printf(" SELECTED");
		putchar('>');

		if (*read == '/' && *(read+1) <= 1 && public_mode < 2) printf("PUBLIC");
		else {
			if (public_mode == 0 || public_mode == 10)
				fwrite(read,end - read,1,stdout);  // output full path
			else if (public_mode == 1 || public_mode == 11) {
				char memo=*end;
				*end=0;
				char *pos=strrchr(read,'/');
				*end=memo;
				if (!pos) fwrite(read,end - read,1,stdout);  	// no slash contained
				else fwrite(pos+1,end - pos-1,1,stdout); 	// basename
			}
			else printf("public_mode %i unknown",public_mode);
		}

		printf("\n");
		if (!*end) break;
		read=end+1;

		// jump over "every" fields
		if (every > 1)
		for (i=0; i<every-1; i++) {
			end=strchr(read,1);
			if (!end) end=strchr(read,0);
			if (!end || !*end) break;
			read=end+1;
		}
	}
}
/*****************************************************************************
  OUTPUT LIST AS TABLE
		columns: how many cols has this table
		header_col: which col is header (0 = none, 999=all)
		public_mode: yes = map / -> PUBLIC
		from: start from field
		ignore: after reading "header_col" fields, ignore some fields
*****************************************************************************/
// gets start and returns end

void OC_output_html_table::create(){
	memset((char *)this,0,sizeof(*this));
	from_field	= 1;
	align_mode	= TABLE_ALIGN_LEFT;
	table_command	= 1;
}

void OC_output_html_table::output(char *list, int fields, char *header, char *headline) {

	char 	*start, *end;
	static 	char buffer[255];
	int 	column, row=0;
	int 	columns;	// printable cols (not list fields)
	int 	i;
	int	first_time=1;

	if (!list || !*list) {
		printf("(empty list)\n");
		return;
	}
	if (fields > OC_MAX_HTML_TABLE_WIDTH) columns = OC_MAX_HTML_TABLE_WIDTH;

	if (table_command) {
		printf("<TABLE");
		if      (border_width == 0) 	printf(">\n");
		else if (border_width == 1) 	printf(" BORDER>\n");
		else 				printf(" BORDER=%i>\n",border_width);
	}

	// ignore list fields from the start
	if (from_field > 1)
		for (i=1; i<from_field; i++) {
			if (!OC_find_next_chr(&start, &end, 1)) break;
		}

	// OUTPUT TABLE
	start = list; end = 0;
	while (1) {

		// transfer data from list into fields
		int list_col=1;
		column = 0;
		columns = 0;
		while (1) {
			column ++;
			if (virtual_field[column]) {	// virtual field: contains ONLY a col function (and no content)
				columns++;
			} else {
				if (list_col > fields) break;	// already read last list field & no more function columns
				while (list_col <= fields && exclude_field[list_col]) {	// jump over exluded list fields
					list_col++;
				}
				if (list_col > fields) continue;	// last+1 list field reached but there may be still virtual columns
				if (invisible_field[list_col])	invisible[column] = 1;
				else				invisible[column] = 0;

				if (OC_find_next_chr(&start, &end, 1)) {;	// read content
					strncpy(buffer, start, end - start);	// store content
					buffer[end - start] = 0;
					delete content[column];
					content[column] = strdup(buffer);
				}
				else break;	// EOList

				list_col++;
				columns++;
			}
		}

		if (first_time) {
			first_time = 0;
			// output HEADLINE if available (needs "columns")
			if (headline) {
				printf("<TR><TH COLSPAN=%i>%s</TR>\n",columns,headline);
			}

			// output HEADER if available
			if (header) {
				char *start, *end;

				start = header; end = 0;
				printf("<TR ALIGN=");
				if (align_mode == TABLE_ALIGN_RIGHT) printf("RIGHT>");
				else if (align_mode == TABLE_ALIGN_CENTER) printf("CENTER>");
				else printf("LEFT>");
				while (1) {
					if (!OC_find_next_chr(&start, &end, ',')) break;
					printf("<TH>");
					fwrite(start,end - start,1,stdout);
				}
				printf("</TR>\n");
			}
		}

		// output contents
		printf("<TR ALIGN=");
		if (align_mode == TABLE_ALIGN_RIGHT) printf("RIGHT>");
		else if (align_mode == TABLE_ALIGN_CENTER) printf("CENTER>");
		else printf("LEFT>");
		row++;
		for (column=1; column<=columns; column++) {

			if (invisible[column]) continue;
			if (is_header[column])	printf("<th>");
			else			printf("<td>");

			if (f_set[column])
				f[column](content[other_col[column]], col_mode[column], col_param[column], row, column); 	// start column function

			// output content - mapped or raw
			if (!f_set[column]) {
				if (is_map_public && !strcmp(content[column],"/"))	printf("PUBLIC");
				else 							printf("%s",content[column]);
			}
		}

		printf("</TR>\n");

		// jump over "ignore" fields
		if (ignore_fields)
		for (i=0; i<ignore_fields; i++) {
			if (!OC_find_next_chr(&start, &end, 1)) break;
		}
		if (!end || !*end) break;
	}
	if (table_command) 	printf("</TABLE>");
}
	/*****************************************************************************
	  HTML: WWW_HOME
	*****************************************************************************/
	// content 	= userpath
	// mode		= 1: <a href=www_home>content</a>
	//		  2: <a href=www_home>full name</a>
	//		  3: <a href=www_home>param</a>
	// param	=
void OC_html_www_home(char *content, int mode, char *param, int row, int col) {

		if (!content || !*content) {
			printf("(empty)");
			return;
		}
		printf("<A HREF=\"http://%s\">",OC_read_user_field_if_userpath_set(content, "www_home"));

		if (mode == 1) {
			printf("%s",content);
		} else if (mode == 2) {
			printf("%s",OC_read_user_field_if_userpath_set(content, "username"));
		} else if (mode == 3) {
			printf("%s",param);
		}
		else printf("unknown mode %i for www_home",mode);
		printf("</A>");

		row = row; 	col = col;	// not needed
	};

	/*****************************************************************************
	  HTML: PROBE_ID LINK
	*****************************************************************************/
	// content 	= probe_id
	// param	= e.g. "EDIT", "CLONE"
void OC_html_probe_id_link(char *content, int mode, char *param, int row, int col) {

		if (!param) param = "(no param)";
		if (!ORS_strcmp(param, "CLONE")) {
			printf("<A HREF=\"http:new_html?html=probe_clone&cloned_probe_id=%s\">%s</A> ", content, param);
						// for makdoku: new_html="probe_clone"		//! update fields of a cloned probe
		} else {
			if      (!ORS_strcmp(param, "EDIT")) 	new_html="probe_upd";		//! update fields of a probe
			else if (!ORS_strcmp(param, "DELETE")) 	new_html="probe_del";		//! really delete this probe?
			else					new_html="probe_view";		//! view fields of a probe
			printf("<A HREF=\"http:probe_detail?html=%s&probe_id=%s\">%s</A>", new_html, content, param);
		}
		row = row; 	col = col;	mode = mode; 	// not needed
	};

	/*****************************************************************************
	  HTML: USERPATH LINK
	*****************************************************************************/
	// content 	= userpath
	// param	= "INFO"
	void OC_html_userpath_link(char *content, int mode, char *param, int row, int col) {
		char *new_html;

		if (!content || !*content) {
			printf("(empty)");
			return;
		}
		printf("<A HREF=\"http:sel_user_info?sel_userpath=%s\">",content);
		if (!param) param = "(no param)";
		else printf("%s", param);
		printf("</A>");
		row = row; 	col = col;	mode = mode; 	// not needed
	};


/*******************************************************************************
	OUTPUT 2 LISTS AS A TABLE
			with tabwidth for 1st column, "between" characters
			and end-of-line characters
********************************************************************************/
void output_2_lists_as_text_table(char *list1, int width1, char *between, char *list2, char *eol) {
	char *read1=list1, *read2=list2, *end1, *end2;
	if (!list1) return;
	if (!list2) return;

	while (1) {
		end1=strchr(read1,1);
		if (!end1) end1=strchr(read1,0);
		if (!end1) break;  // never reached
		fwrite(read1,end1 - read1,1,stdout);  // output to end marker position
		int i;
		if (width1 > 0)
		for (i=0; i < width1 - (end1 - read1); i++) putchar(' ');	// reach tabzone
		printf("%s",between);

		end2=strchr(read2,1);
		if (!end2) end2=strchr(read2,0);
		if (!end2) break;  // never reached
		fwrite(read2,end2 - read2,1,stdout);  // output to end marker position

		printf("%s",eol);


		if (!*end1) break;
		if (!*end2) break;
		read1=end1+1;
		read2=end2+1;
	}
}

/*******************************************************************************
	OUTPUT LIST AS A TEXT TABLE WITH 2 COLUMNS
			with tabwidth for 1st column, "between" characters
			and end-of-line characters
********************************************************************************/
void output_list_as_2_col_table(char *list1, int width1, char *between, char *eol) {

	char *read=list1,  *end;
	if (!list1) return;

	while (1) {
		end=strchr(read,1);
		if (!end) end=strchr(read,0);
		if (!end) break;  // never reached
		fwrite(read,end - read,1,stdout);  // output to end marker position
		int i;
		if (width1 > 0)
		for (i=0; i < width1 - (end - read); i++) putchar(' ');	// reach tabzone
		printf("%s",between);

		if (!*end) break;
		read=end+1;

		end=strchr(read,1);
		if (!end) end=strchr(read,0);
		if (!end) break;  // never reached
		fwrite(read,end - read,1,stdout);  // output to end marker position

		printf("%s",eol);

		if (!*end) break;
		read=end+1;
	}
}

/*****************************************************************************
	OUTPUT PDB-LIST
*****************************************************************************/
void OC_output_pdb_list(OC_pdb_list *list, char *select, char *how) {

	OC_pdb_list *pointer, *owner_elem;
	static 	char *section_memo="";
	char 	*owner, *main_owner;
	int 	table  = !strcmp(how,"table");
	int 	search = (!strcmp(select,"search"));
	char 	*search_mode = cgi_var("search");
	static	char *sections_string=0;

	// owner of main section is 'p_owner' or 'p_author'
//for (pointer = list; pointer; pointer=OC_next_pdb_list_elem(list, pointer)) {
//	if (pointer->section) printf("sec='%s' ",pointer->section);
//	if (pointer->name) printf("name='%s' ",pointer->name);
//	if (pointer->content) printf("cont='%s'",pointer->content);
//	printf("<P>");
//}

	owner_elem = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_owner");
	if (owner_elem && owner_elem->content && *(owner_elem->content)) main_owner = owner_elem->content;
	else {
		owner_elem = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, "", "p_author");
		if (owner_elem && owner_elem->content && *(owner_elem->content)) main_owner = owner_elem->content;
		else {
			if (!strcmp(select,"create")) 	main_owner = ors_gl.userpath;
			else 				main_owner = "/";
		}
	}
	owner = main_owner;

	printf("<A HREF=\"main\"></A>");
	if (search) {
		printf("<CENTER><B>Search in all fields for phrase:</B> <INPUT NAME=\"search_any_field\" SIZE=30>");
		OC_output_html_buffer(" <A HREF=\"##..HREF/##help/search_any_field.html\">HELP</A>", 0);
		printf("</CENTER><BR>\n");
	}

	// ====== create and output links to sections ======
	if (!strcmp(search_mode,"simple")) sections_string = strdup("");
	else {
		void *mem_file=GBS_stropen(10000);  // open memory file
		int found=0;
		int number_of=0;
		GBS_strcat(mem_file, "\n\n<FONT SIZE=+1><B>");
		GBS_strcat(mem_file, "\n<A HREF=\"#main\">main</A> ");
		for (pointer = list; pointer; pointer=OC_next_pdb_list_elem(list, pointer)) {
			if (!pointer->section) continue;
			if (ORS_strcmp(section_memo, pointer->section) || pointer->next == list) {
				// if ((found || strcmp(select,"show")) && section_memo) {
				if (found  && section_memo) {
					GBS_strcat(mem_file, "<A HREF=\"http:#");
					GBS_strcat(mem_file, section_memo);
					GBS_strcat(mem_file, "\">");
					GBS_strcat(mem_file, section_memo);
					GBS_strcat(mem_file, "</A> ");
					number_of++;
				}
				found = 0;
				section_memo = pointer->section;
			}
			if (pointer->content && *(pointer->content)
					&& ORS_strcmp(pointer->name,"p_owner")
					&& ORS_strcmp(pointer->name,"p_last_mod") ) found=1;
		}
		GBS_strcat(mem_file, "</B></FONT>");
		delete sections_string;
		sections_string = GBS_strclose(mem_file);
		if (number_of > 1) 	printf("<CENTER>sections: %s</CENTER><P>",sections_string);
		else 			sections_string = 0;
	}

	if (table) printf("<table>");

	// ====== output table ======
	for (pointer = list; pointer; pointer=OC_next_pdb_list_elem(list, pointer)) {
		// VEREINBARUNG: alle sektionen gleich beim createn if (!strcmp(select,"create") && pointer->section) continue;

		if (search) {
			if (!strcmp(search_mode,"simple") && !strchr(pointer->rights,'s')) continue;
			if (!strcmp(search_mode,"std")    && !strchr(pointer->rights,'s') && !strchr(pointer->rights,'S')) continue;
		}

		if (pointer->section && strcmp(pointer->section, section_memo)) {
			section_memo = pointer->section;

			// get owner of this section (or use owner of main section)
			owner_elem = OS_find_pdb_list_elem_by_name(ors_gl.pdb_list, pointer->section, "p_owner");
			if (owner_elem && owner_elem->content && *(owner_elem->content)) owner = owner_elem->content;
										    else owner = main_owner;
			if (table) printf("</table>");

			if (!strcmp(select,"show")) {
				// is next section empty? -> jump it over
				OC_pdb_list *temp;
				int found;
				found=0;
				for (temp = pointer; temp; temp=OC_next_pdb_list_elem(list, temp)) {
					if (ORS_strcmp(temp->section, section_memo)) break;   // end of section
					if (temp->content && *temp->content
							  && ORS_strcmp(temp->name,"p_owner")
							  && ORS_strcmp(temp->name,"p_last_mod") ) {found=1; break; } // content found
				}
				if (!found) {
					if (temp) {
						pointer = temp->prev;	// jump over this section
						continue;
					}
					else break;
				}
			}

			// not empty -> output next section
			printf("<A NAME=\"%s\"></A>",pointer->section);	// href on same page
			printf("<HR>");
			if (sections_string && *sections_string) printf("<CENTER>[ %s ]</CENTER>",sections_string);
			OC_output_html_buffer("##BULLET_BIG##", 0);
			// printf("<FONT SIZE=+2><B> Section: %s</B></FONT> ",pointer->section);
			printf("<FONT SIZE=+2><B> %s</B></FONT> ",pointer->section);
			printf("<BR>");
			if (table) printf("<table>");
		}

		OC_output_pdb_list_elem(pointer, select, how, owner);
	}
	if (!strcmp(how,"table")) printf("</table>");
}

/*****************************************************************************
	OUTPUT PDB-LIST ELEMENT
					gets owner of current section
*****************************************************************************/
void OC_output_pdb_list_elem(OC_pdb_list *elem, char *select, char *how, char *owner) {
	static char buffer[255];

	int create 	= (!strcmp(select,"create"));
	int search 	= (!strcmp(select,"search"));
	int update 	= (strchr(elem->rights,'W') != 0);
	int table  	= !strcmp(how,"table");
	int www_home 	= (ORS_contains_word(elem->features, "www_home", ' ') && !search);
	int info_link 	= (ORS_contains_word(elem->features, "info_link", ' ') && !search);
	int line_links 	= (ORS_contains_word(elem->features, "line_links", ' ') && !search);
	int other_probes= ORS_contains_word(elem->features, "author_other_probes", ' ');
	int font_plus_1 = ORS_contains_word(elem->features, "font+1", ' ');
	int font_plus_2 = ORS_contains_word(elem->features, "font+2", ' ');
	int bold	= ORS_contains_word(elem->features, "bold", ' ');
	int select_owner= ORS_contains_word(elem->features, "select_owner", ' ');
	int is_username = ORS_contains_word(elem->features, "username", ' ');
	int tt_font	= ( !strcmp(elem->name,"p_sequence") || !strcmp(elem->name,"p_target_seq") );

	if (www_home && info_link) www_home=0;

	char *content=0;	// content of this field
	// fill content
	if (create) {	// use init values
		delete content;
		if      (!strcmp(elem->init,"author")) 		content=strdup(ors_gl.userpath);
		else if (!strcmp(elem->init,"date")) 		content=ORS_time_and_date_string(DATE_DMY, 0);
		else if (!strcmp(elem->init,"time")) 		content=ORS_time_and_date_string(TIME_HM, 0);
		else if (!strcmp(elem->init,"sequence")) 	content=strdup(cgi_var("p_sequence"));
		else if (!strcmp(elem->init,"target_seq")) 	content=strdup(cgi_var("p_target_seq"));

		else if (strchr(elem->rights,'V'))		content=elem->content;
	}
	else if (!strcmp(select,"show")) {
		content=elem->content;
		update=0;
	}
	else if (search) {
		content=strdup("");
		update=1;
	}
	else {	// use existing values from environment
		delete content;
		if (elem->content && !select_owner)
			content = strdup(elem->content);
		else
			content = strdup("");
	}

	if (update && !search && !create && !(ORS_is_parent_or_equal(ors_gl.userpath, owner) ||
		!strcmp(owner,"")) ) update = 0;	// special case: old data may have "" as owner

	if (table) {
		printf("<TR ALIGN=LEFT><th");
		if (!update) printf(" valign=top");
		printf(">%s:<td>",elem->label);
	}

	if (tt_font) printf("<TT><FONT SIZE=+1>");
	if (font_plus_1) printf("<FONT SIZE=+1>");
	if (font_plus_2) printf("<FONT SIZE=+2>");
	if (bold) printf("<B>");

	// choose owner from option list
	if (select_owner && update) {
		printf("<SELECT NAME=\"sel_%s\" SIZE=1>",elem->db_name);
		if (search) printf("<OPTION VALUE=\"\" SELECTED>\n");	// empty value for search mode
		output_list_as_html_options(list_of_pub_parents(ors_gl.userpath, owner), owner, 10);
		printf("</SELECT>");
	}

	// TEXT
	if (!strcmp(elem->type,"text")) {
		// 1 TEXT LINE
		if (elem->height == 1 || (search)) {
			if (update)	printf("<INPUT NAME=\"%s\" SIZE=%i VALUE=\"", elem->db_name,elem->width);

			if (!update && www_home)  printf(" <A HREF=\"http://%s\">", OC_read_user_field_if_userpath_set(content,"www_home"));
			if (!update && info_link && content) printf(" <A HREF=\"http:sel_user_info?sel_userpath=%s\">",content);
			if (content && !(update && select_owner)) printf("%s",content);
			if (!update && www_home)  printf("</A>");
			if (!update && info_link && content) printf("</A>");

			if (update) {
				printf("\">");
				if (www_home)
					printf(" <A HREF=\"http://%s\">homepage</A>",OC_read_user_field_if_userpath_set(content,"www_home"));
				if (info_link && content)
					printf(" <A HREF=\"http:sel_user_info?sel_userpath=%s\">infopage</A>",content);
			} else if (strcmp(select,"show")) {
				// transport data hidden if contents are only shown
				printf("<INPUT NAME=\"%s\" TYPE=hidden SIZE=%i VALUE=\"", elem->db_name,elem->width);
				if (content) printf("%s",content);
				printf("\">");
			}

		// TEXTAREA
		} else {
			if (update) {
				printf("<TEXTAREA NAME=\"%s\" COLS=%i ROWS=%i>", elem->db_name,elem->width,elem->height);
				if (content) printf("%s",content);
				printf("</TEXTAREA>");
			} else {
				if (content) printf("%s",content);
			}
		}
	}
	// OPTION LIST
	else if (!strcmp(elem->type,"option")) {
		if (update) {
			printf("<SELECT NAME=\"%s\" SIZE=1>",elem->db_name);
			if (search) printf("<OPTION VALUE=\"\" SELECTED>\n");	// empty value for search mode

			if (!strcmp(elem->init,"pub_exist_def")) {
				if (create)	output_list_as_html_options(list_of_pub_parents(ors_gl.pub_exist_max, ors_gl.userpath),
												ors_gl.pub_exist_def, 1);
				else 		output_list_as_html_options(list_of_pub_parents(ors_gl.pub_exist_max, ors_gl.userpath),
												elem->content, 1);
			}
			else if (!strcmp(elem->init,"pub_content_def")) {
				if (create)	output_list_as_html_options(list_of_pub_parents(ors_gl.pub_content_max, ors_gl.userpath),
												ors_gl.pub_content_def, 1);
				else 		output_list_as_html_options(list_of_pub_parents(ors_gl.pub_content_max, ors_gl.userpath),
												elem->content, 1);
			}
			else if (!strcmp(elem->init,"section_type"))	{
				if (!ORS_strcmp(elem->content,""))	printf("<OPTION VALUE=\"\" SELECTED>\n");
				else					printf("<OPTION VALUE=\"\">\n");
				output_list_as_html_options(OC_read_file_into_list(
								ORS_LIB_PATH "section_types"), //! possible type names for etc section
								elem->content, 0);
			}
			else if (!strcmp(elem->init,"target_gene")) 	{
				if (create)	OC_output_html_buffer("##OPTION_TARGET_GENE##", 0);
				else	output_list_as_html_options(OC_read_file_into_list(
								ORS_LIB_PATH "target_genes"), //! possible target gene names for main section
								elem->content, 0);
			}
			printf("</SELECT>");
		} else {
			if (!ORS_strcmp(content,"/")) 	printf("PUBLIC");
			else				if (content) printf("%s",content);
		}
	}
	// LINKS (lines, every line a link)
	else if (!strcmp(elem->type,"links")) {
		if (update) {
			printf("<TEXTAREA NAME=\"%s\" COLS=%i ROWS=%i>", elem->db_name,elem->width,elem->height);
			if (content) printf("%s",content);
			printf("</TEXTAREA>");
		} else {
			OC_output_links(content, '\n', "", "<BR>");
		}
	}
	// UNKNOWN
	else {
		printf("TYPE %s IS NOT IMPLEMENTED",elem->type);
	}

	if (is_username && !search) {
		printf(" %s ", OC_read_user_field_if_userpath_set(content,"username"));
	}

	if (bold) printf("</B>");
	if (font_plus_1) printf("</FONT>");
	if (font_plus_2) printf("</FONT>");
	if (tt_font) printf("</TT></FONT>");


	// HELP PAGE
	if (elem->help_page && *(elem->help_page)) {
		if (table) printf("<td>");
		sprintf(buffer,"<A HREF=\"##..HREF/##help/%s.html\">HELP</A>",elem->help_page);
		OC_output_html_buffer(buffer, 0);
	}
	if (table) printf("</TR>\n");
	else printf("<BR>");
}

/*****************************************************************************
	OUTPUT LINES AS LINKS
					gets owner of current section
*****************************************************************************/
void OC_output_links(char *list, char seperator, char *prefix, char *eol) {
	if (!list) {
		printf("(empty)%s",eol);
		return;
	}
	char *pos = list;
	char *pos2 = list; // dummy
	while (*pos2) {
		pos2 = strchr(pos,seperator);
		if (!pos2) pos2 = list + strlen(list);
		printf("<A HREF=\"%s", prefix);
		fwrite(pos,pos2 - pos,1,stdout);
		printf("\">");
		fwrite(pos,pos2 - pos,1,stdout);
		printf("</A>%s", eol);
		if (*pos2) pos = pos2 + 1;
	}
}

/*****************************************************************************
  LIST OF PUB PARENTS
	highest = highest userpath, lowest = lowest userpath
	output = all users beetween as list (seperated by 1)
*****************************************************************************/
char *list_of_pub_parents(char *highest, char *lowest) {
	char *hi, *lo;

	if (!highest || !lowest) return "";  // string empty?

	if (strlen(highest) < strlen(lowest)) {
		if (!strncmp(highest, lowest, strlen(highest))) {
			hi=highest;
			lo=lowest;
		}
		else return ""; // hi is not father of lo
	}
	else {
		if (!strncmp(highest, lowest, strlen(lowest))) {
			lo=highest;  // the other way round
			hi=lowest;
		}
		else return ""; // lo is not father of hi

	}
	// hi is now father of lo

	void *file=GBS_stropen(10000);  // open memory file
	if (!strcmp(hi,"/")) {
		GBS_strcat(file, "/");
		if (strcmp(lo,"/"))  GBS_chrcat(file, 1);
				else return GBS_strclose(file);
	}

	char *pos1=lo+strlen(hi);
	char *pos2=lo+strlen(lo);

	while(1) {
		pos2=strchr(pos1,'/');
		if (!pos2) {
			GBS_strcat(file, lo);
			break;
		}
		char memo=*pos2;
		*pos2=0;
		GBS_strcat(file, lo);
		*pos2=memo;
		GBS_chrcat(file, 1);
		pos1=pos2+1;
	}
	return GBS_strclose(file);
}

/*****************************************************************************
  BUILD USERPATH from sel_par_userpath and sel_user
*****************************************************************************/
char *OC_build_sel_userpath(char *dad, char *me)  {
	if (!dad) {
		if (!me) return strdup("");
		return strdup(me);
	}
	if (!strcmp(dad,"/")) return GBS_global_string_copy("/%s", me);
	return GBS_global_string_copy("%s/%s", dad, me);
}

/*****************************************************************************
  Print enough "../" to get images
*****************************************************************************/
void print_path_to_images(void) {
	int i;
	int count=ORS_str_char_count(ors_gl.path_info,'/');
	if (count) for (i=0; i<count+1; i++) printf("../");
}


