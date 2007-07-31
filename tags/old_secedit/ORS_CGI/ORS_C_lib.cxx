/* #############################################

	ORS_CLIENT  LIBRARY

   ############################################# */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>
// #include <malloc.h>
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



/*****************************************************************************
  Write Logfile Message
*****************************************************************************/
void OC_write_log_message(char *comment) {
	char *file=ORS_LIB_PATH "logfile.cgi";
	FILE *fd = fopen(file,"a");

	if (!fd) {
		printf("CAN'T OPEN LOGFILE!!!!!\n\n<P>");
		return;
	}

	static char *date_time=0;
	static char *host=0;

	delete date_time;
	date_time = ORS_time_and_date_string(DATE_TIME,0);
	delete host;
	if (ors_gl.remote_host)	host = strdup(ors_gl.remote_host);
	else			host = strdup("");
	char *pos=strchr(host, '.');
	if (pos) *pos=0;

	fprintf(fd,"%s  %s \t", date_time, host);
	if (ors_gl.userpath)
		fprintf(fd,"%s",ors_gl.userpath);
	else
		fprintf(fd,"<unbek>");

	if (ors_gl.path_info) {
		char *pos = strrchr(ors_gl.path_info, '/');
		fprintf(fd," \t%s",pos);
	}
	else
		fprintf(fd," \t");

	if (ors_gl.action)
		fprintf(fd," \t%s",ors_gl.action);
	else
		fprintf(fd," \t");

	if (comment) fprintf(fd,"\t%s",comment);
	fprintf(fd,"\n");

	fclose(fd);
	return;
}

/*****************************************************************************
  quit with server error message if error message is not empty
*****************************************************************************/
void OC_server_error_if_not_empty(char *locs_error) {
	if (locs_error && *locs_error) {
		quit_with_error(ORS_export_error("Server error: %s",locs_error));
	}
}

/***********************
  server communication
************************/
int init_local_com_struct()
{
    const char *user = GB_getenvUSER();

    if( aisc_create(ors_gl.link, ORS_MAIN, ors_gl.com,
                    MAIN_LOCAL, ORS_LOCAL, &ors_gl.locs,
                    LOCAL_WHOAMI, user,
                    NULL)){
        return 1;
    }
    return 0;
}

/*********************
  return server name
**********************/
char *ORS_look_for_server(void)
{
	// return strdup("rock:2996");
	return ORS_read_a_line_in_a_file(ORS_LIB_PATH "CONFIG","ORS_SERVER");
}

/*************************************
  initialize communction with server
**************************************/
void init_server_communication(void)
{
	char *servername;

	if( !(servername=(char *)ORS_look_for_server()) ){
		quit_with_error(ORS_export_error("Cannot find server"));
	}

	ors_gl.link = (aisc_com *)aisc_open(servername, &ors_gl.com,AISC_MAGIC_NUMBER);
	free (servername); servername = 0;

	if (!ors_gl.link) {
		// system("tb/bla -b -d &");
		// sleep(3);
		// quit_with_error(ORS_export_error("Link to server failed. Restarting server... try reloading this page."));
		quit_with_error(ORS_export_error("Link to server failed. Server will be restarted later."));

	}
	if (init_local_com_struct() ) {
		quit_with_error(ORS_export_error("Local comm struct failed"));
	}

	return;
}

/*************************************************************/
/** read a line from file 'fname' which begins with env      */
/*  all $(ENVIRONMENT VARIABLES) are replaced by their value */
/*************************************************************/
char *ORS_read_a_line_in_a_file(char *filename, char *env)
	{
	char buffer[256];
	FILE 	*arb_tcp;
	char	*p;
	char	*nl;
	int	envlen;

	if (!filename) quit_with_error("Got empty filename (read a line in a file)!");


	arb_tcp = fopen(filename,"r");
	if (!arb_tcp) quit_with_error(ORS_export_error("Can't open file: %s",filename));

	envlen = strlen(env);


	for (	p=fgets(buffer,255,arb_tcp);
		p;
		p = fgets(p,255,arb_tcp)){
		if (!strncmp(env,buffer,envlen)) break;
	}
	fclose(arb_tcp);

	if (p){
		if (nl = strchr(p,'\n')) *nl = 0;
		p = p+envlen;
		while ((*p == ' ') || (*p == '\t')) p++;
		p = GBS_eval_env(p);
		return p;
	} else {
		ORS_export_error("Missing '%s' in '%s'",env,filename);
		return 0;
	}
}


/******************************************************
  NORMALIZE SEQ: caps & watch out for frong chars
			quit_with_error on wrong chars
*******************************************************/
void OC_normalize_seq(char *seq, char *allowed_bases) {
	char *pos = seq;
	if (!seq || !*seq) return;
	while (*pos) {
		if (*pos >= 'a' && *pos <= 'z') *pos = *pos - 'a' + 'A';
		if (!strchr(allowed_bases, *pos))
			quit_with_error(ORS_export_error("Base '%c' at position %i is not allowed! Allowed are: '%s' (norm)", *pos, (int)(pos-seq)+1, allowed_bases));
		pos++;
	}
}

/******************************************************
  CALC SEQ & TARGET:
			deletes and mallocs target
			quit_with_error on error
*******************************************************/
void OC_calculate_seq_and_target_seq(char **seq1, char **seq2, char *allowed_bases) {

	char *seq;
	char *tar;
	int direction;
	if (!allowed_bases) allowed_bases="";

	if (*seq1 && **seq1 && *seq2 && **seq2) {
		if (!ORS_seq_matches_target_seq(*seq1, *seq2, 1))
			quit_with_error("Cannot calculate target sequence, because both exist but do not match! (calc_seq)");
		else return;
	}
	// empty one is target
	if (!*seq1 || !**seq1) { tar = *seq1; seq = *seq2; direction=1; }
	                  else { tar = *seq2; seq = *seq1; direction=2; }

	delete tar;
	tar = (char *)calloc(sizeof(char), strlen(seq) + 1);

	char *spos = seq;
	char *tpos = tar + strlen(seq) - 1;

	while (*spos) {
		switch(*spos) {
			case 'A': if (strchr(allowed_bases,'U')) *tpos = 'U'; else *tpos = 'T'; break;
			case 'C': *tpos = 'G'; break;
			case 'G': *tpos = 'C'; break;
			case 'T': *tpos = 'A'; break;
			case 'U': *tpos = 'A'; break;
			case 'M': *tpos = 'K'; break;
			case 'K': *tpos = 'M'; break;
			case 'R': *tpos = 'Y'; break;
			case 'Y': *tpos = 'R'; break;
			case 'S': *tpos = 'S'; break;
			case 'V': *tpos = 'B'; break;
			case 'B': *tpos = 'V'; break;
			case 'D': *tpos = 'H'; break;
			case 'H': *tpos = 'D'; break;
			case 'X': *tpos = 'X'; break;
			case 'N': *tpos = 'X'; break;
			case '.': *tpos = '.'; break;
			default:
			quit_with_error(ORS_export_error(
				"Base '%c' at position %i is not allowed! Allowed are: '%s' (calc_seq)", *spos, (int)(spos-seq)+1, allowed_bases));
		}
		spos++;
		tpos--;
	}

	if (direction == 1) *seq1=tar;
	               else *seq2=tar;

}


