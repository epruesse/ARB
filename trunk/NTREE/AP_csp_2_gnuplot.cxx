#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt.hxx>
#include <awt_tree.hxx>
#include <awt_csp.hxx>
#include "ap_csp_2_gnuplot.hxx"

extern GBDATA *gb_main;


void AP_csp_2_gnuplot_cb(AW_window *aww, AW_CL cspcd){
	GB_transaction dummy(gb_main);
	AWT_csp *csp = (AWT_csp *)cspcd;
	GB_ERROR error = 0;
	AP_filter filter;
	char *filterstring = aww->get_root()->awar(AP_AWAR_FILTER_FILTER)->read_string();
	char *alignment_name =  aww->get_root()->awar(AP_AWAR_FILTER_ALIGNMENT)->read_string();
	long alignment_length = GBT_get_alignment_len(gb_main,alignment_name);
	error = filter.init(filterstring,"0",alignment_length);
	delete alignment_name;
	delete filterstring;

	if (error) { aw_message(error); return ; }
	error = csp->go(&filter);
	if (error) { aw_message(error); return ; }

	if (!csp->seq_len) { aw_message("Please select column statistic"); return; };

	char *fname = aww->get_root()->awar(AP_AWAR_CSP_FILENAME)->read_string();
	if (strlen(fname) < 1) {
		delete fname;
		aw_message("Please enter file name");
		return;
	}
	FILE *out = fopen(fname,"w");
	if (!out) {
		aw_message(GB_export_error("Cannot write to file '%s'",fname));
		delete fname;
	}
	delete fname;
	char *type = aww->get_root()->awar(AP_AWAR_CSP_SUFFIX)->read_string();
	long smooth = aww->get_root()->awar(AP_AWAR_CSP_SMOOTH)->read_int()+1;
	double val;
	double smoothed = 0;
	unsigned int j;
	float *f[4];
	if (type[0] == 'f') {		// sort frequencies
		int wf;
		int c = 0;
		for (wf = 0; wf <256 && c <4; wf++) {
			if (csp->frequency[wf]) f[c++] = csp->frequency[wf];
		}
		int k,l;
		for (k=3;k>0;k--) {
			for (l=0;l<k;l++) {
				for (j=0;j<csp->seq_len; j++) {
					if (f[l][j] > f[l+1][j]) {
						float h;
						h = f[l][j];
						f[l][j] = f[l+1][j];
						f[l+1][j] = h;					
					}
				}
			}
		}

	}

	for (j=0;j<csp->seq_len; j++) {
		if (!csp->weights[j]) continue;
		fprintf(out,"%i ",j);
		if (!strcmp(type,"all")){
			fprintf(out," %f %f %f %f\n",
				csp->frequency['A'][j],
				csp->frequency['C'][j],
				csp->frequency['G'][j],
				csp->frequency['U'][j]);
			continue;
		}
		if (!strcmp(type,"gc_gnu")) {
			val = ( csp->frequency['G'][j] + csp->frequency['C'][j] ) /
				 (double)( csp->frequency['A'][j] + csp->frequency['C'][j] +
				 csp->frequency['G'][j] + csp->frequency['U'][j] );
		}else if (!strcmp(type,"ga_gnu")) {
			val = ( csp->frequency['G'][j] + csp->frequency['A'][j] ) /
				 (double)( csp->frequency['A'][j] + csp->frequency['C'][j] +
				 csp->frequency['G'][j] + csp->frequency['U'][j] );
		}else if (!strcmp(type,"rate_gnu")) {
			val = csp->rates[j];
		}else if (!strcmp(type,"tt_gnu")) {
			val = csp->ttratio[j];
		}else if (!strcmp(type,"f1_gnu")) {
				val = f[3][j];
		}else if (!strcmp(type,"f2_gnu")) {
				val = f[2][j];
		}else if (!strcmp(type,"f3_gnu")) {
				val = f[1][j];
		}else if (!strcmp(type,"f4_gnu")) {
				val = f[0][j];
		}else if (!strcmp(type,"helix_gnu")) {
			val = csp->is_helix[j];
		}else{
			val = 0;
		}
		smoothed = val/smooth + smoothed *(smooth-1)/(smooth);
		fprintf(out,"%f\n",smoothed);
	}
	delete type;
	fclose(out);
	aww->get_root()->awar(AP_AWAR_CSP_DIRECTORY)->touch();	// reload sel box
}

AW_window *AP_open_csp_2_gnuplot_window( AW_root *root ){

	GB_transaction dummy(gb_main);
	AWT_csp *csp = new AWT_csp(gb_main,root,AP_AWAR_CSP_NAME);
	AW_window_simple *aws = new AW_window_simple;
        aws->init( root, "EXPORT_CSP_TO_GNUPLOT", "Export Column Statistik to GnuPlot", 10,10);
  	aws->load_xfig("cpro/csp_2_gnuplot.fig");

	root->awar_string(AWAR_DEFAULT_ALIGNMENT, "", gb_main);

	root->awar_int(AP_AWAR_CSP_SMOOTH);

	root->awar_string(AP_AWAR_CSP_NAME);
	root->awar_string(AP_AWAR_CSP_ALIGNMENT);
	root->awar(AP_AWAR_CSP_ALIGNMENT)->map(AWAR_DEFAULT_ALIGNMENT);	// csp of the correct al.

	root->awar_string(AP_AWAR_FILTER_NAME);
	root->awar_string(AP_AWAR_FILTER_FILTER);
	root->awar_string(AP_AWAR_FILTER_ALIGNMENT);
	root->awar(AP_AWAR_FILTER_ALIGNMENT)->map(AWAR_DEFAULT_ALIGNMENT);	// csp of the correct al.

	root->awar_string(AP_AWAR_CSP_SUFFIX, "gc_gnu", AW_ROOT_DEFAULT);
	root->awar_string(AP_AWAR_CSP_DIRECTORY);
	root->awar_string(AP_AWAR_CSP_FILENAME, "noname.gc_gnu", AW_ROOT_DEFAULT);

	aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
	aws->create_button("CLOSE","CLOSE","C");			   
	
	aws->at("help");aws->callback(AW_POPUP_HELP,(AW_CL)"csp_2_gnuplot.hlp");
	aws->create_button("HELP","HELP","H");			   

	awt_create_selection_box(aws,AP_AWAR_CSP);

	aws->at("csp");
	create_selection_list_on_csp(aws,csp);

	aws->at("what");
	AW_selection_list* selid = aws->create_selection_list(AP_AWAR_CSP_SUFFIX);
		aws->insert_selection(selid, "G+C Ratio",	"gc_gnu");
		aws->insert_selection(selid, "G+A Ratio",	"ga_gnu");
		aws->insert_selection(selid, "Rate",		"rate_gnu");
		aws->insert_selection(selid, "TT Ratio",	"tt_gnu");
		aws->insert_selection(selid, "Most Frequent Base","f1_gnu");
		aws->insert_selection(selid, "Second Frequent Base","f2_gnu");
		aws->insert_selection(selid, "Third Frequent Base","f3_gnu");
		aws->insert_selection(selid, "Least Frequent Base","f4_gnu");
		aws->insert_selection(selid, "All Frequencies","all");
		aws->insert_selection(selid, "Helix",		"helix_gnu");
	aws->update_selection_list(selid);

	AW_CL filter = awt_create_select_filter(root,gb_main,AP_AWAR_FILTER_NAME);
	aws->at("ap_filter");
	aws->callback(AW_POPUP,(AW_CL)awt_create_select_filter_win,filter);
	aws->create_button("SELECT_FILTER", AP_AWAR_FILTER_NAME);

	aws->at("smooth");
	aws->create_option_menu(AP_AWAR_CSP_SMOOTH);
		aws->insert_option("Dont Smooth","D",0);
		aws->insert_option("Smooth 1","1",1);
		aws->insert_option("Smooth 2","2",2);
		aws->insert_option("Smooth 3","3",3);
		aws->insert_option("Smooth 5","5",5);
		aws->insert_option("Smooth 10","0",10);
	aws->update_option_menu();

	aws->at("save");
	aws->highlight();
	aws->callback(AP_csp_2_gnuplot_cb,(AW_CL)csp);
	aws->create_button("SAVE","SAVE");			   

	return (AW_window *)aws;
}
