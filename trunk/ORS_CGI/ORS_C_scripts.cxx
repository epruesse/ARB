#include <stdio.h>
# include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <sys/types.h>
# include <sys/wait.h>
# include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>


#include <arbdb.h>
#include <ors_client.h>

#include <cat_tree.hxx>
#include "ors_lib.h"
#include "ors_c_java.hxx"
#include "ors_c.hxx"
#include "ors_c_proto.hxx"


char *ors_remove_leading_spaces(char *data){
	while (*data == ' ' || *data == '\t') data++;	// remove leading spaces
	return data;
}

FILE *ors_tcp_open(char *mach_name, int socket_id) {

	int      so;
	struct in_addr  addr;	/* union -> u_long  */
	struct		hostent *he;
	char           *err;
	struct sockaddr_in so_ad;
	memset((char *)&so_ad,0,sizeof(struct sockaddr_in));
	so = socket(PF_INET, SOCK_STREAM, 0);
	if (!(he = gethostbyname(mach_name))) {
		ORS_export_error("Unknown host: %s", mach_name);
		return NULL;
	}
	addr.s_addr = *(int *) (he->h_addr);
	so_ad.sin_addr = addr;
	so_ad.sin_family = AF_INET;
	so_ad.sin_port = htons(socket_id);	/* @@@ = pb_socket  */

	if (connect(so, (struct sockaddr *)&so_ad, 16)) {
		ORS_export_error("Cannot connect to %s:%i errno %i",
			mach_name,socket_id,errno);
		return NULL;
	}

	return fdopen(so,"w");
}


char *ors_form_2_java(char *data){		// make it clean !!!
	char *nl;
	putc('F',stdout);			// Form Tag
	for (nl = ""; nl; data = nl){
		nl = strchr(data,'\n');
		if (nl) *(nl++) = 0;
		char *s = ors_remove_leading_spaces(data);
		if (*s == '#') continue;
		if (!strlen(s)) continue;
		char *eq = strchr(s,'=');		// looking for the = sign
		if (!eq) {
			eq = s + strlen(s);
		}
		char *p;
		for (p=s;p<eq;p++) *p = toupper(*p);	// Convert keyword to uppercase !!

		fwrite(s, strlen(s),1,stdout);
		putc('\n',stdout);
	}
	return 0;
}

GB_ERROR ors_ovp_2_java(char *id, char *data, char *path_of_tree,FILE *out){
	CAT_FIELDS catfield;
	if (!ORS_strncasecmp(id,"name")) catfield = CAT_FIELD_NAME;
	else if (!ORS_strncasecmp(id,"full")) catfield = CAT_FIELD_FULL_NAME;
	else if (!ORS_strncasecmp(id,"acc")) catfield = CAT_FIELD_ACC;
	else if (!ORS_strncasecmp(id,"strain")) catfield = CAT_FIELD_STRAIN;
	else {
		return ORS_export_error("Sorry: Your ovp: type field should be appended by:\n"
		"	either name,full_name,strain,acc to identify the link into the tree\n"
		"	your appendix was '%s'", id);
	}
	struct T2J_transfer_struct *transfer = T2J_read_query_result_from_data(data,catfield);
	if (!transfer){
		return  GB_get_error();
	}

	GB_ERROR error = T2J_transform(path_of_tree,0,transfer,ors_gl.focus,out);
	return error;
}

GB_ERROR ORS_file_2_java(char *command, char *tree, FILE *fd){
						// transforms the input into java readable form
	void *memfile;			// First of all readin everything !!!!!
	memfile = GBS_stropen(100000);	// start with 0.1 meg of memory
	int i;
	register int c;
	while ( (c = getc(fd)) != EOF) GBS_chrcat(memfile,c);
	int size = GBS_memoffset(memfile);		// sizeof data
	char *data = GBS_strclose(memfile);
	// ** Now I would like to use perl, instead I have to use this ugly C
	// Lets check the content type first
	char *nl = strchr(data,'\n');
	if (!nl) return ORS_export_error("Your script '%s' has no output (%s)!!!",command,data);
	*(nl++) = 0;
	if ( ORS_strncasecmp(data,"content-type:")){
		return ORS_export_error("Your script '%s' has no content-type but '%s'",
			command,data);
	}
	int source_is_java = JAVA;


	data = ors_remove_leading_spaces(strchr(data,':')+1);
	if ( !ORS_strncasecmp(data,"ascii")){
		if (!source_is_java) 	printf("Content-type: text/plain\n\n%s",nl);
		else			printf("A%s",nl);		// write ASCII-TAG + DATA
		return 0;
	}else 	if ( !ORS_strncasecmp(data,"error")){
		if (!source_is_java) 	printf("Content-type: text/plain\n\n%s",nl);
		else			printf("E%s",nl);		// write ASCII-TAG + DATA
		return 0;
	}else 	if ( !ORS_strncasecmp(data,"form")){
		return ors_form_2_java(nl);	// parse the form that means removing all spaces!!
	}else 	if ( !ORS_strncasecmp(data,"link")){
		printf("l%s",nl);		// write ASCII-TAG + DATA
		return 0;
	}else 	if ( !ORS_strncasecmp(data,"ovp/")){
		if (source_is_java) {				// from java to java
			return ors_ovp_2_java(data+4,nl,tree,stdout);
		}else{
								// from html to java so try to open java socket
			char *server_name = cgi_var("java_server");
			char *server_port = cgi_var("java_server_port");
			if (!*server_name) {
				return ORS_export_error("No java_server cgi_var found");
			}
			if (!*server_port) {
				return ORS_export_error("No java_server_port cgi_var found");
			}
			int port = atoi(server_port);
			if (!port) {		// goto local file
				char tmpfile[2048];
				char *ffile = strdup(ors_gl.dailypw);
				ORS_C_make_clean(ffile);
				sprintf(tmpfile,ORS_TMP_PATH "ovp_%s",ffile);
				delete ffile;
				FILE *out = fopen(tmpfile,"w");
				if (!out) {
					return ORS_export_error("Cannot write to tmp file '%s'",tmpfile);
				}
				ors_ovp_2_java(data+4,nl,tree,out);
				fclose(out);
				printf("Goto the java window and press 'Get last netscape query'\n");
			}else{
				FILE *out = ors_tcp_open(server_name,port);
				if (!out) {
					return ORS_export_error("Cannot connect to '%s:%i'",server_name,port);
				}
				ors_ovp_2_java(data+4,nl,tree,out);
				fclose(out);
				printf("Goto the java window\n");
				return NULL;
			}
		}
				// parse the form that means removing all spaces!!
		return 0;
	} 		// No its time to goto netscape !!!!!!!

	if (source_is_java){
			char tmpname[1024];		// temporary filename
			sprintf(tmpname,ORS_TMP_PATH "ors_%i",getpid());
			FILE *out = fopen(tmpname,"w");
			if (!out) {
				return ORS_export_error("Sorry: I could not write html page to '%s'",tmpname);
			}
			fprintf(out, "Content-Type: %s\n\n%s", data,nl);
			fclose(out);
			char *pathinfo = strdup(getenv("PATH_INFO"));
			char *lshlash = strrchr(pathinfo,'/');
			if (lshlash) *lshlash = 0;		// remove last element
			else *pathinfo = 0;
			sprintf(tmpname,"ors_%i",getpid());

			fprintf(stdout,"Lhttp://%s:%s%s%s/ors_tmp_html=%s",
				getenv("SERVER_NAME"),getenv("SERVER_PORT"),
				getenv("SCRIPT_NAME"),
				pathinfo,
				tmpname);
			return NULL;
	}else{
		printf("Content-Type: %s\n\n%s", data,nl);		// html to html
		return NULL;
	}

//	return ORS_export_error("I cannot identify your content type: '%s'\n"
//			"	Valid types are ascii,error,gif,form,link,ovp/\n"
//			"	Command was '%s'",
//				data,command);

}


	// simply returns the error message
	// 0 if everything was successfull

void ors_client_error(char *error) {
	printf("Content-type:  error=\n\n%s\n", error);
}

GB_ERROR
ORS_C_exec_script(char *tree, char *command)
{
	FILE           *tfp;
	struct stat     finfo;
	int             pfd[2];
	register int    n, x;
	int             pid;

	char *select = strdup(cgi_var("selection"));
	if (*select){
		char buffer[1024];
		char buffer2[1024];
		char *group_name = NULL;
		double relsel;		// relativ number of selected nodes 1.0 means all subnodes
		char *second = strchr(select,'_');
		if (second) {
			*(second++) = 0;
			char *seconrange = T2J_get_selection(tree,second, "SELECTED2=", 0,
				NULL,NULL,NULL);
			if (!seconrange) return GB_get_error();	// no tree
			putenv(seconrange);
			char *seconlist = T2J_get_selection(tree,second, "SELECTED2_NAMES=", 1,
				NULL,NULL,NULL);
			putenv(seconlist);
		}
		char *primerange = T2J_get_selection(tree,select, "SELECTED=", 0,NULL,NULL,NULL);
		if (!primerange) return GB_get_error();	// no tree
		putenv(primerange);
		char *primelist = T2J_get_selection(tree,select, "SELECTED_NAMES=", 1,
			&ors_gl.focus, & group_name, & relsel);
		putenv(primelist);
		if (group_name) {
			sprintf(buffer, "SELECTED_NODE=%s", group_name );
			putenv(buffer);
		}else{
			putenv("SELECTED_NODE=");
		}
		sprintf(buffer2,"SELECTED_REL=%G",relsel);
		putenv(buffer2);
	}

	while (command[0] == '/')
		command++;
	//remove leading /
		if (GBS_find_string(command, "..", 0)) {
		return ORS_export_error(
			      "It is not allowed to place any '..' in an ors_script path: '%s'",
			      command);
	}
	fflush(stdout);

	if (pipe(pfd) < 0)
		return ("Server Error: could not open pipe");
	if ((pid = fork()) < 0) {
		return ("Server Error: could not fork");
	} else if (!pid) {
		char           *argv0;

		close(pfd[0]);
		if (pfd[1] != STDOUT_FILENO) {
			dup2(pfd[1], STDOUT_FILENO);
			close(pfd[1]);
		}
		if (chdir(ORS_SCRIPT_PATH ".")) {
			char mydir[202];
			getcwd(mydir,200);
			ors_client_error( ORS_export_error(
				" Could not change current Directory to: '%s'\n"
				" 	Now my directory is '%s'\n",
						ORS_SCRIPT_PATH,
						mydir));
			exit(1);
		}
		char *path = strrchr(command,'/');
		if (path) {
			*path = 0;
			if (chdir(command)) {
				char mydir[202];
				getcwd(mydir,200);
				ors_client_error( ORS_export_error(
					" Could not change current Directory to: %s\n"
					" 	Now my directory is %s\n",
						command,
						mydir));
				exit(1);
			}
			char *h = path;
			path = command;
			command = h+1;
		}
		char *ncomm = GBS_string_eval(command,"*=./*",0);
		ors_gl.cgi_vars[0] = command;
		if (execv(ncomm, ors_gl.cgi_vars) == -1) {
			ors_client_error( ORS_export_error("Couldn't execute ors_scripts %s%s/%s",
				ORS_SCRIPT_PATH,path,command) );
			exit(1);
		}
	} else {
		close(pfd[1]);
	}
	tfp = fdopen(pfd[0], "r");
	GB_ERROR error = ORS_file_2_java(command, tree, tfp);
	fclose(tfp);
	waitpid(pid, NULL, 0);
	//remove the zoombie from the process list
		return error;
}
