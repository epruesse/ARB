/*
#################################
#                               #
#    ORS_CLIENT:  PT            #
#    PT database functions      #
#                               #
################################# */
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>
#include <arbdb.h>

#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>

#include <cat_tree.hxx>
#include "ors_lib.h"
//#include "ors_c_java.hxx"
#include "ors_c.hxx"
#include "ors_c_proto.hxx"

struct pdgl_struct {
	aisc_com *link;
	T_PT_LOCS locs;
	T_PT_MAIN com;
	} pd_gl;

// results of probe match
struct result_struct {
	T_PT_MATCHLIST match_list;
	long match_list_cnt;
	bytestring bs;
	} pt_result;


/************************************
  create communication with server
*************************************/
int init_pt_local_struct()
{
    const char *user = GB_getenvUSER();

    if( aisc_create(pd_gl.link, PT_MAIN, pd_gl.com,
                    MAIN_LOCS, PT_LOCS, &pd_gl.locs,
                    LOCS_USER, user,
                    NULL)){
        return 1;
    }
    return 0;
}

/****************************
  return name of server #n
*****************************/
char *probe_pt_look_for_server(int server_nr)	// 0 = 16s 1 = 23s ...
	{
	char choice[256];
	sprintf(choice,"ARB_PT_SERVER%i",server_nr);
	return ORS_read_a_line_in_a_file(ORS_LIB_PATH "CONFIG",choice);
}

/******************************************************************************
  PROBE MATCH:
 		Send probe request to server and receive probe match results
*******************************************************************************/
void probe_match (	int server_nr, char *probe_seq, int max_mismatches,
			int weighted_mis, int reversed, int complement,
			int max_hits)
{
	char	*servername;
	char	*locs_error;

	if( !(servername=(char *)probe_pt_look_for_server(server_nr)) ){
		quit_with_error (ORS_export_error("PT_SERVER #%i is unknown (see: ors_lib/servers)",server_nr));
	}

	// Open Connection
	pd_gl.link = (aisc_com *)aisc_open(servername, &pd_gl.com,AISC_MAGIC_NUMBER);

	if (!pd_gl.link) {
		quit_with_error (ORS_export_error("PT_SERVER #%i (%s) not up. Please contact system administrator.",server_nr,servername));
	}


	// Initialize server
	if (init_pt_local_struct() ) {
		quit_with_error (ORS_export_error("Lost contact to PT_SERVER - dammit!"));
	}

	if (aisc_nput(pd_gl.link,PT_LOCS, pd_gl.locs,
		LOCS_MATCH_REVERSED,		reversed,
		LOCS_MATCH_SORT_BY,		weighted_mis,
		LOCS_MATCH_COMPLEMENT,		complement,
		LOCS_MATCH_MAX_MISMATCHES,	max_mismatches,
		LOCS_MATCH_MAX_SPECIES,		max_hits,
		LOCS_SEARCHMATCH,		probe_seq,
		0)) {
		quit_with_error ("Connection to PT_SERVER lost (2)");
	}

	pt_result.bs.data = 0;

	aisc_get( pd_gl.link, PT_LOCS, pd_gl.locs,
		LOCS_MATCH_LIST,	&pt_result.match_list,		// short names of species
		LOCS_MATCH_LIST_CNT,	&pt_result.match_list_cnt,	// count of species short names
		LOCS_MATCH_STRING,	&pt_result.bs,			// output of pt_Server
		LOCS_ERROR,		&locs_error,
		0);
	if (*locs_error) {
		quit_with_error(ORS_export_error("Error in PT_SERVER communication (5): %s",locs_error));
	}
	free(locs_error);

	aisc_close(pd_gl.link);
	free (servername); servername = 0;

}

/******************************************************************************
  PROBE FIND:
 		Send probe request to server and receive probe match results
*******************************************************************************/
#if 0

void probe_find (int server_nr, char *probe_seq)
{
	char	*servername;
	char	*locs_error;

	if( !(servername=(char *)probe_pt_look_for_server(server_nr)) ){
		quit_with_error (ORS_export_error("PT_SERVER #%i is unknown (see: ors_lib/servers)",server_nr));
	}

	// Open Connection
	pd_gl.link = (aisc_com *)aisc_open(servername, (int *)&pd_gl.com,AISC_MAGIC_NUMBER);

	if (!pd_gl.link) {
		quit_with_error (ORS_export_error("PT_SERVER #%i (%s) not up. Please contact system administrator.",server_nr,servername));
	}

	// Initialize server
	if (init_pt_local_struct() ) 	quit_with_error (ORS_export_error("Lost contact to PT_SERVER - dammit!"));

	if (aisc_nput(pd_gl.link,PT_LOCS, pd_gl.locs,
		LOCS_PROBE_FIND,	probe_seq,
		0)) {
		quit_with_error ("Connection to PT_SERVER lost (8)");
	}

	pt_result.bs.data = 0;

	aisc_get( pd_gl.link, PT_LOCS, pd_gl.locs,
		LOCS_MATCH_LIST,	&pt_result.match_list,		// short names of species
		LOCS_MATCH_LIST_CNT,	&pt_result.match_list_cnt,	// count of species short names
		LOCS_MATCH_STRING,	&pt_result.bs,			// output of pt_Server
		LOCS_ERROR,		&locs_error,
		0);
	if (*locs_error) {
		quit_with_error(ORS_export_error("Error in PT_SERVER communication (7): %s",locs_error));
	}
	free(locs_error);

	aisc_close(pd_gl.link);
	free (servername); servername = 0;

}
#endif

/******************************************************************************

  OUTPUT RESULTS OF PROBE MATCH AS HTML PAGE (with references)

  output looks like: header line, name line, main line, name line, main line,...
	name line: 8 characters with name
	main line: 8 char name (right aligned) space full name space ...

*******************************************************************************/
void output_html_match_result(void) {
	char *start, *end, *name;
	int line=0;
	if (!pt_result.bs.data) return;

	printf("<PRE>");
	for (start=pt_result.bs.data; *start;  ) {
		end=strchr(start,1);
		if (!end) end = start + strlen(start);
		// name line
		if (line != 0 && (line % 2) == 1) {
			name=start;
		}
		else {

			if (line) {
				printf("<A HREF=\"sp=");
				if (name[7]==1) name[7]=' ';   // correct buggy output of server
				if (name[6]==1) name[6]=' ';
				if (name[5]==1) name[5]=' ';
				if (name[4]==1) name[4]=' ';
				if (name[3]==1) name[3]=' ';
				fwrite(name,8,1,stdout);  // output name (left aligned)
				printf("/GS\">");
				fwrite(name,8,1,stdout);  // output name (left aligned)
				printf("</A>");
				fwrite(start + 9,end - start - 9,1,stdout);  // output to eol
			}
			else {
				fwrite(start,end - start,1,stdout);  // output to eol
				printf("\n");
			}

			printf("\n");
		}

		if (*(end)) start=end+1; else start=end;
		line++;
	}

	printf("</PRE>");
}



char *output_java_match_result(char *filename,char *path_of_tree){
	char *start = strchr(pt_result.bs.data,1);
	if (!start){
		quit_with_error(ORS_export_error("No hits"));
	}

	FILE *out = fopen(filename,"w");
	if (!out){
		quit_with_error(ORS_export_error("Cannot write to file '%s'",filename));
	}
	struct T2J_transfer_struct *transfer = T2J_read_query_result_from_pts(pt_result.bs.data);
	if (!transfer)		quit_with_error( GB_get_error() );
	GB_ERROR error = T2J_transform(path_of_tree,0,transfer,ors_gl.focus,out);

	fclose(out);
	GB_set_mode_of_file(filename,00666);
	return 0;
}

void free_results(void) {
	delete pt_result.bs.data; pt_result.bs.data = 0;
}
