#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>

#include <ors_server.h>
#include <ors_lib.h>
#include <arbdb.h>
#include <arbdbt.h>

#include "ors_s_common.hxx"
#include "ors_s_proto.hxx"


/*************************************************************
  write a message to logfile
**************************************************************/
extern "C" void write_logfile(ORS_local *locs, char *comment) {
	char *file=ORS_LIB_PATH "logfile";
	FILE *fd = fopen(file,"a");
	static char *date_time=0;

	delete date_time;
	date_time = ORS_time_and_date_string(DATE_TIME,0);

	if (!locs) {
		fprintf(fd,"%s ",date_time);
		if (comment) fprintf(fd,"\t%s",comment);
		fprintf(fd,"\n");
	} else {
		fprintf(fd,"%s  %s \t",date_time, locs->remote_host);
		if (locs->userpath)
			fprintf(fd,"%s",locs->userpath);
		else
			fprintf(fd,"<unbek>");
		if (comment) fprintf(fd,"\t%s",comment);
		fprintf(fd,"\n");
	}
	
	fclose(fd);
	return;
}

/*************************************************************
  quit with error
**************************************************************/
// this function exists seperately for ors client
void quit_with_error(char *message) {
	printf("SERVER_ERROR: %s", message);
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
		if ((nl = strchr(p,'\n'))) *nl = 0;
		p = p+envlen;
		while ((*p == ' ') || (*p == '\t')) p++;
		p = GBS_eval_env(p);
		return p;
	} else {
		ORS_export_error("Missing '%s' in '%s'",env,filename);
		return 0;
	}
}

