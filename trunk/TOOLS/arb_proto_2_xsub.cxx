#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>

int main(int argc, char **argv)
{
    if (argc <= 2) {
	fprintf(stderr,"arb_proto_2_xsub converts GB_prototyes to perl interface\n");
	fprintf(stderr,"Usage: arb_proto_2_xsub <prototypes.h> <xs-header>\n");
	fprintf(stderr,"<xs-header> may contain prototypes. Those will not be overwritten!!!\n");
	return(-1);
    }
    char *data = GB_read_file(argv[1]);
    if (!data){
	GB_print_error();
	exit(EXIT_FAILURE);
    }

    /* read old version (i.e. ARB.xs.default)
       and put all existing functions to exclude hash */

    char *head = GB_read_file(argv[2]);
    printf("%s",head); /* inserting the *.xs.default in the output *.xs file*/
	
    GB_HASH *exclude_hash = GBS_create_hash(1024,0); /*prepare list for excluded functions from xs header*/
    {
	char *tok;
	/*   initializer            cond  updater                */
	for (tok = strtok(head,"\n");tok;tok = strtok(NULL,"\n")){
	    if ( !strncmp(tok,"P2A_",4)
		 ||  !strncmp(tok,"P2AT_",5)){  /* looks like the if-branch is entered for every token*/
		char *fn = GBS_string_eval(tok,"(*=",0);
		GBS_write_hash(exclude_hash,fn,1);
		delete fn;
	    }
	}
    }

    /*parsing of proto.h and replacing substrings*/
    data = GBS_string_eval(data,
			   "\nchar=\nschar"	// strdupped char
			   ":const ="
			   ":GB_CSTR =char \\*"
			   ":GB_ERROR =char \\*"
			   ":GB_CPNTR =char \\*"
			   ":GB_ULONG =long "
			   ":enum gb_call_back_type =char \\*"
			   ":GB_TYPES =char \\*"
			   ":GB_BOOL =int "
			   ":GB_UNDO_TYPE =char \\*"
			   ,0);
			   

    char *tok;
    void *gb_out = GBS_stropen(100000);
    void *gbt_out = GBS_stropen(100000);
	
    for (tok = strtok(data,";\n");tok;tok= strtok(0,";\n")){
      //      	fprintf(stderr,"Hmmm='%s'\n",tok);

      if (strpbrk(tok,"#{}")) continue;
	// ignore blocks like:
	//
	//#ifdef __cplusplus
	//extern "C" {
	//#endif
	//#ifdef __cplusplus
	//}
	//#endif

	/* exclude some functions  because of type problems */
	if (strstr(tok,"NOT4PERL")) continue;
	if (strstr(tok,"struct")) continue;
	if (strstr(tok,"GBDATA_SET")) continue;
	if (strstr(tok,"UINT4 *")) continue;
	if (strstr(tok,"FLOAT *")) continue;
	if (strstr(tok,"...")) continue;
	if (strstr(tok,"GBQUARK")) continue;
	if (strstr(tok,"GB_HASH")) continue;
	if (strstr(tok,"GBT_TREE")) continue;
	if (strstr(tok,"GB_Link_Follower")) continue;
	if (strstr(tok,"GB_alignment_type")) continue;
	if (strstr(tok,"GB_COMPRESSION_MASK")) continue;
	if (strstr(tok,"float *")) continue;
	if (strstr(tok,"**")) continue;
	if (!GBS_string_cmp(tok,"*(*(*)(*",0)) continue; // no function parameters
	if (strstr(tok,"GB_CB")) continue; // this is a function parameters as well	

	//	fprintf(stderr,"Good='%s'\n",tok);

	/* extract function type */
	char *sp = strchr(tok,' ');
	const char *type = 0;
	{
	  if (!sp) {  // is the expected declaration format "type name(..)" ??
	      fprintf(stderr, "Space expected in '%s'\n",tok);
	      exit(EXIT_FAILURE);
	    }
	  if (sp[1] == '*') sp++;	// function type
	  int c = sp[1];                // what the hell is this for ??????
	  sp[1] = 0;
	  type = strdup(tok);
	  sp[1] = c;
	  

	}
	    
	/* check type */
	GB_BOOL const_char = GB_FALSE;
	GB_BOOL free_flag = GB_FALSE;
	if (!strcmp(type,"char *"))		const_char = GB_TRUE;
	if (!strncmp(type,"schar",5)){
	    free_flag = GB_TRUE;
	    type++;
	}
	    
	if (!strcmp(type,"float"))		type = "double";
	if (!strcmp(type,"GB_alignment_type"))		type = "double";
	    
	    
	tok = sp+1;
	sp = strchr(tok,' ');
	if (!sp) {
	  fprintf(stderr, "Space expected in '%s'\n",tok);	  
	  exit(EXIT_FAILURE);
	}
	*(sp++) = 0;

	/* exclude some funtions */

	char *func_name = tok;
	if (!strcmp(func_name,"GBT_add_data")) continue;
	
	void *out = 0;
	if (!strncmp(func_name,"GB_",3)) out = gb_out;
	if (!strncmp(func_name,"GBT_",4)) out = gbt_out;
	if (!out) continue;
		
	char *perl_func_name = GBS_string_eval(func_name,"GBT_=P2AT_:GB_=P2A_",0);
	if (GBS_read_hash(exclude_hash,perl_func_name)) continue;
	GBS_write_hash(exclude_hash,perl_func_name,1); // don't list functions twice

	
	char *line = GBS_string_eval(sp,"P_(=:(=:)=",0);
	char *p;
	void *params = GBS_stropen(1000);
	void *args = GBS_stropen(1000);
	for (p=line; line; line=p ){
	    p = strchr(line,',');
	    if (p) *(p++) = 0;
	    if (p && *p == ' ') p++;
	    if (strcmp(line,"void")){
		GBS_strcat(params,"	");
		GBS_strcat(params,line);
		GBS_strcat(params,"\n");
		char *arp = strrchr(line,' ');
		if (arp) {
		    arp++;
		    if (arp[0] == '*') arp++;
		    GBS_strcat(args,",");
		    GBS_strcat(args,arp);
		}
	    }
	}
	char *sargs = GBS_strclose(args,0);
	if (sargs[0] == ',') sargs++;
	char *sparams = GBS_strclose(params,0);
	GBS_strnprintf(out,1000,"%s\n",type);
	GBS_strnprintf(out,1000,"%s(%s)\n%s\n",perl_func_name,sargs,sparams);

	if (!strncmp(type,"void",4)){
	    GBS_strnprintf(out,100,"	PPCODE:\n");
	    GBS_strnprintf(out,1000,"	%s(%s);\n\n",	func_name,sargs);
	}else{
	    GBS_strnprintf(out,100,"	CODE:\n");
	    if (const_char){
		GBS_strnprintf(out,1000,	"	RETVAL = (char *)%s(%s);\n",		(char *)func_name,sargs);
	    }else if(free_flag){
		GBS_strnprintf(out,1000,	"	if (static_pntr) free(static_pntr);\n");
		GBS_strnprintf(out,1000,	"	static_pntr = %s(%s);\n",		func_name,sargs);
		GBS_strnprintf(out,1000,	"	RETVAL = static_pntr;\n");
	    }else{
		GBS_strnprintf(out,1000,	"	RETVAL = %s(%s);\n",			func_name,sargs);
	    }
	    GBS_strnprintf(out,	   1000,	"	OUTPUT:\n	RETVAL\n\n");
	}
    }
    printf("%s",GBS_strclose(gb_out,0));
    printf("MODULE = ARB   PACKAGE = BIO  PREFIX = P2AT_\n\n");
    printf("%s",GBS_strclose(gbt_out,0));

    return EXIT_SUCCESS;
}
