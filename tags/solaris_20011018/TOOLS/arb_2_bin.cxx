#include <stdio.h>
#include <stdlib.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <string.h>
int main(int argc, char **argv)
	{
	GB_ERROR error;
	char *in;
	char *out;
	char rtype[256];
	char *rtypep = rtype;
	char wtype[256];
	char *wtypep = wtype;
	char *opt_tree = 0;
	int ci = 1;
	int nidx = 0;
	int test = 0;
	GBDATA *gb_main;
	memset(rtype,0,10);
	memset(wtype,0,10);
	*(wtypep++) = 'b';
	*(rtypep++) = 'r';
	*(rtypep++) = 'w';
	if (argc <= 1) {
		GB_export_error(	"\narb_2_bin: 	Converts a database to binary format\n"
					"		Syntax:	   arb_2_bin [-m] [-c][tree_xxx] database [newdatabase]\n"
					"	       	Options:	-m		create map file too\n"
					"	       	Options:	-r		try to repair destroyed database\n"
					"	       	Options:	-c[tree_xxx]	optimize database using tree_xxx or largest tree"
		    );
		GB_print_error();
		return(-1);
	}
	while (argv[ci][0] == '-'){
	    if (!strcmp(argv[ci],"-m")){
		ci++;
		*(wtypep++) = 'm';
	    }
	    if (!strcmp(argv[ci],"-r")){
		ci++;
		*(rtypep++) = 'R';
	    }
	    if (!strncmp(argv[ci],"-c",2)){
		opt_tree = argv[ci]+2;
		
		ci++;
	    }
	    if (!strncmp(argv[ci],"-i",2)){
		nidx = atoi(argv[ci]+2);
		ci++;
	    }
	    if (!strncmp(argv[ci],"-t",2)){
		test = 1;
		ci++;
	    }
	}
	
	in = argv[ci++];
	if (ci >= argc) {
		out = in;
	}else{
		out = argv[ci++];
	}
	gb_main = GBT_open(in,rtype,0);
	if (!gb_main){
		GB_print_error();
		return (-1);
	}
	if (opt_tree){
	    char *ali_name;
	    {
		GB_transaction dummy(gb_main);
		ali_name = GBT_get_default_alignment(gb_main);
	    }
	    if (!strlen(opt_tree)) opt_tree = 0;
	    error = GBT_compress_sequence_tree2(gb_main,opt_tree,ali_name);
	    if (error) {
		GB_print_error();
	    }
	}
	GB_set_next_main_idx(nidx);

 	if (test){
 	    GB_ralfs_test(gb_main);
 	}
	
	error = GB_save(gb_main,out,wtype);
	if (error){
		GB_print_error();
		return -1;
	}
	return 0;
}
