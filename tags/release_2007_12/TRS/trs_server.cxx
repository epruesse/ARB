#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>

char *TRS_map_file(const char *path,int writeable);
#define GB_map_file TRS_map_file
#	include <cat_tree.hxx>
#undef GB_map_file

#include "tree_lib.hxx"
#include "trs_proto.h"

static void quit_with_error(char *error) __attribute__((noreturn));
static void quit_with_error(char *error) {
	fprintf(stdout,"%s\n",error);
	exit(-1);
}

static char *get_indata(int argc, char **argv) {
	char *indata = 0;
	char *request_method;

	request_method = getenv("REQUEST_METHOD");
	// GET means: contents are in variable QUERY_STRING

	if (request_method && argc<=1){
		printf("Content-type:	text/html\n\n");
		if (!strcmp(request_method,"GET")) {
			indata=strdup(getenv("QUERY_STRING"));
		} else if (!strcmp(request_method,"POST")) {
			char *temp;
			if(!(temp=getenv("CONTENT_LENGTH")))
				quit_with_error(TRS_export_error("CONTENT_LENGTH not set"));
			int len=atoi(temp);
			indata=new char[len+1];
			indata[len]=0;
			fread(indata,len,1,stdin);
		}
	} else {	// read the command line
		if (argc>1){
			indata = strdup(argv[1]);
		}else{
			indata = 0;
		}
	}

	return indata;
}


static long convert_cgi_to_hash(char *indata){		// destroys indata
	long hash = TRS_create_hash(10);
	char *p;
	for (p = strtok(indata,"&");p; p = strtok(0,"&")){
		char *out = strdup(p);
		int i,j=0;
		for ( i=0;p[i];i++ ){
			if (p[i]=='+'){
				out[j++] = ' ';
				continue;
			}
			if (p[i] == '%'){
				int ch;
				int save = p[i+3];
				p[i+3] = 0;
				sscanf(p+i+1,"%x", &ch);
				p[i+3] = save;
				out[j++] = ch;
				i+=2;
			}else{
				out[j++] = p[i];
			}
		}
		out[j] = 0;

		char *eq = strchr(out,'=');
		if (!eq) continue;
		*(eq) = 0;
		TRS_write_hash(hash,out,(long)(eq+1));
	}
	return hash;
}

/*
static long trs_show_hash(char *key,long val){
	printf("%s:%s\n",key,(char *)val);
	return val;
}
*/

int main(int argc, char **argv){
	char *data = get_indata(argc,argv);
	char	*error = 0;
	if (!data){
		fprintf(stderr,
			"Syntax:	%s tree_name=xxx&tree_dir=yyyy&\n"
			"	query=tree	get the tree structure of yyyy/xxxx.otb\n"
			"	query=branch_lengths get the branch lengths\n"
			"	query=node_names	node Names\n"
			"	query=full_name1	First word of fullname\n"
			"	query=full_name2	Rest of fullname\n"
			"	getnames=RRRRR		Get the names of the selected species\n"
			"					RRRR is genareted by the TrsTree\n"
			"	getnode=RRRRR		Gets the name of the deepest nodes which\n"
			"						contains all selected\n"
			"	getfullnames=RRRRR	Get the fullnames seperated by ####\n"
			"	getaccession=RRR		Get the accession numbers \n"
			"	toOTF=name		Translate stdin Lines like 'name#+#value' into otf\n"
			"	toOTF=fullname		Translate stdin Lines like 'fullname#+#value' into otf\n"
			"	toOTF=accession		Translate stdin Lines like 'accession#+#value' into otf\n"
			"\n"
			"	toSelection=name	Translate stdin Lines like 'name#+#' into compressed selection string\n"
			"	toSelection=fullname	Translate stdin Lines like 'fullname#+#' into compressed selection string\n"
			"	toSelection=accession	Translate stdin Lines like 'accession#+#' into compressed selection string\n"
			"\n"
			"	toNewick=modified nodes	Translate OTB format to newick, modified nodes comes from TrsTree\n"
			"		[stripNewick=grouped_nodes]	Replace all grouped nodes by [GROUP=sizeofgroup]'name'\n"
			"		[prune=selected_nodes]		Remove all nodes but selected, selected are from TrsTree\n"
			,argv[0]);
		exit(0);
	}



	long hash = convert_cgi_to_hash(strdup(data));
	//TRS_hash_do_loop(hash, trs_show_hash);

	char *tree_name = (char *)TRS_read_hash(hash,"tree_name");
	if (!tree_name){
		quit_with_error(TRS_export_error("No Parameter 'tree_name' found: '%s'",data));
	}
	const char *tree_dir = (const char *)TRS_read_hash(hash,"tree_dir");
	if (!tree_dir) tree_dir =  ".";
	char *tree_path = (char *)calloc(strlen(tree_dir) + strlen(tree_name) + 10,1);	// Look in ../irs_lib/%s.otb
	sprintf(tree_path,"%s/%s.otb",tree_dir, tree_name);

	char *query = (char *)TRS_read_hash(hash,"query");
	if (query){
		if (!strcmp(query,"tree")){
			error = T2J_send_bit_coded_tree(tree_path,stdout);
		}else if (!strcmp(query,"branch_lengths")){
			error = T2J_send_branch_lengths(tree_path,stdout);
		}else if (!strcmp(query,"node_names")){
			error = T2J_transfer_group_names(tree_path,stdout);
		}else if (!strcmp(query,"full_name1")){
			error = T2J_transfer_fullnames1(tree_path,stdout);
		}else if (!strcmp(query,"full_name2")){
			error = T2J_transfer_fullnames2(tree_path,stdout);
		}
	}

	char *get_names = (char *)TRS_read_hash(hash,"getnames");
	if (!error && get_names){
		double relgroup;
		char *selection = T2J_get_selection(tree_path,get_names,"",1,CAT_FIELD_NAME,0,0,&relgroup);
		if (!selection) error = TRS_get_error();
		else		printf("%s\n",selection);
	}
	char *get_fullnames = (char *)TRS_read_hash(hash,"getfullnames");
	if (!error && get_fullnames){
		double relgroup;
		char *selection = T2J_get_selection(tree_path,get_fullnames,"",1,CAT_FIELD_FULL_NAME,0,0,&relgroup);
		if (!selection) error = TRS_get_error();
		else	printf("%s\n",selection);
	}

	char *get_acc = (char *)TRS_read_hash(hash,"getaccession");
	if (!error && get_acc){
		double relgroup;
		char *selection = T2J_get_selection(tree_path,get_acc,"",1,CAT_FIELD_ACC,0,0,&relgroup);
		if (!selection) error = TRS_get_error();
		else	printf("%s\n",selection);
	}

	char *get_node = (char *)TRS_read_hash(hash,"getnode");
	if (!error && get_node){
		CAT_node_id focus;
		char *nodename = 0;
		double relgroup;
		char *selection = T2J_get_selection(tree_path,get_node,"",1,CAT_FIELD_ACC,&focus,&nodename,&relgroup);
		if (!selection) error = TRS_get_error();
		else	printf("%s:%G\n",nodename,relgroup);
	}
	int otf = 1;
	char *toOTF = (char *)TRS_read_hash(hash,"toOTF");
	if (! toOTF){
	    otf = 0;			// not otf format
	    toOTF = (char *)TRS_read_hash(hash,"toSelection");
	}
	if (!error && toOTF){
		CAT_FIELDS catfield;
		if (!strcmp(toOTF,"name")) catfield = CAT_FIELD_NAME;
		else if (!strcmp(toOTF,"fullname")) catfield = CAT_FIELD_FULL_NAME;
		else if (!strcmp(toOTF,"accession")) catfield = CAT_FIELD_ACC;
		else {
			error = TRS_export_error("Sorry: invalid toOTF Value:\n"
			"	only name,fullname,accession is allowed");
			goto end;
		}
		char *dat = TRS_read_file("-");
		struct T2J_transfer_struct *transfer = T2J_read_query_result_from_data(dat,catfield);
		if (!transfer){
			error =  TRS_get_error();
			goto end;
		}
		if ( otf != 0 ){
		    error = T2J_transform(1,tree_path,transfer,0,stdout);
		}else{
		    error = T2J_transform(0,tree_path,transfer,0,stdout);
		}
	}
	{
	    char *toNewick = (char *)TRS_read_hash(hash,"toNewick");
	    if (!error && toNewick){
		char *stripNewick = (char *)TRS_read_hash(hash,"stripNewick");
		char *prune = (char *)TRS_read_hash(hash,"prune");
		error = T2J_send_newick_tree(tree_path,toNewick,prune,stripNewick,stdout);
	    }
	}

end:
	if (error) {
		quit_with_error(error);
	}
	return 0;
}
