#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>
#include <memory.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt_tree.hxx>
#include "dist.hxx"
#include <BI_helix.hxx>
#include <awt_nds.hxx>
#include <awt_csp.hxx>

#include "ph_matr.hxx"


const char *PHMATRIX::save(char *filename,enum PH_SAVE_TYPE type)
{
    FILE *out;
    GB_ERROR error = 0;
    out = fopen(filename,"w");
    if (!out) {
	return "Cannot save your File";
    }
    int row,col;
    switch (type){

	case PH_SAVE_PHYLIP_COMP:
	    {
		fprintf(out,"    %li\n",nentries);
		for (row = 0; row<nentries;row++){
		    fprintf(out,"%-10s ",entries[row]->name);
		    for (col=0; col<row; col++) {
			fprintf(out,"%6f ",matrix->get(row,col));
		    }
		    fprintf(out,"\n");
		}
	    }
	    break;
	case PH_SAVE_READABLE:
	    {
		GB_transaction dummy(gb_main);
		GBDATA *gb_species;
		char *buf;
		char **appendix;
		int	app_size = 20;
		int i,slen;
		int	maxnds = 0;
		appendix = (char **)calloc(sizeof(char *), app_size);
		for (i=0;i<app_size;i++){
		    appendix[i] = (char *)calloc(sizeof(char),(int)nentries);
		    memset(appendix[i],' ',(int)nentries);
		}
		double min = matrix->get(1,0) * 100.0;
		double max = min;
		double sum = 0.0;

		make_node_text_init(gb_main);
		for (row = 0; row<nentries;row++){
		    buf = entries[row]->name;
		    gb_species = GBT_find_species_rel_species_data(gb_species_data,buf);
		    if (gb_species){
			buf = make_node_text_nds(gb_main,gb_species,1,0);
		    }
		    slen = strlen(buf);
		    if (slen > maxnds) maxnds = slen;
		    if (slen > app_size) slen = app_size;
		    for (i=0;i<slen;i++) {
			appendix[i][row] = buf[i];
		    }
		    if (row %4 == 0) fprintf(out,"\n");
		    fprintf(out,"%-20s ",buf);
		    for (col=0; col<nentries; col++) {
			if (col %4 == 0) fprintf(out,"  ");
			double val2 = matrix->get(row,col) * 100.0;
			if (val2 > 99.9 || row == col) {
			    fprintf(out,"%4.0f ",val2);
			}else{
			    fprintf(out,"%4.1f ",val2);
			}
			if (val2 > max) max = val2;
			if (val2 < min) min = val2;
			sum += val;
		    }
		    fprintf(out,"\n");
		}
		for (i=0;i<app_size;i++){
		    for (col = 0;col<maxnds;col++) fputc(' ',out);
		    for (col=0; col<nentries; col++) {
			if (col %4 == 0) fprintf(out,"  ");
			fprintf(out,"  %c  ",appendix[i][col]);
		    }
		    free(appendix[i]);
		    fprintf(out,"\n");
		}
		fprintf(out,"Minimum: %f\n",min);
		fprintf(out,"Maximum: %f\n",max);
		fprintf(out,"Average: %f\n",sum/(nentries*nentries) );
		free((char *)appendix);
	    }
	    break;
	default:
	    error = GB_export_error("Unknown Save Type");
    }
    fclose(out);
    return (char *)error;
}
