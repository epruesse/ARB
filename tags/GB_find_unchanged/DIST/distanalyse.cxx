#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#include <math.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>

#include <awt.hxx>

#include <awt_tree.hxx>
#include "dist.hxx"
#include "di_matr.hxx"



char *PHMATRIX::analyse(AW_root *awrdummy)
{
	AWUSE(awrdummy);
	long  row, pos;
	char *sequ, ch, dummy[280];

	long 	act_gci, mean_gci=0;
	float	act_gc, mean_gc, min_gc=9999.9, max_gc=0.0;
	long	act_len, mean_len=0, min_len=9999999, max_len=0;

	if (is_AA) {
		if (nentries> 100) {
			aw_message("A lot of sequences!\n   ==> fast Kimura selected! (instead of PAM)");
			aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(PH_TRANSFORMATION_KIMURA);
		}else{
			aw_message(	"Only limited number of sequences!\n"
					"   ==> slow PAM selected! (instead of Kimura)");
			aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(PH_TRANSFORMATION_PAM);
		}
		return 0;
	}


	//calculate meanvalue of sequencelength:

	for(row=0; row<nentries; row++) {
		act_gci = 0;
		act_len = 0;
		sequ = entries[row]->sequence_parsimony->sequence;
		for(pos=0; pos<tree_root->filter->real_len; pos++) {
			ch = sequ[pos];
			if(ch == AP_C || ch == AP_G) act_gci++;
			if(ch == AP_A || ch == AP_C || ch == AP_G || ch == AP_T) act_len++;
		}
		mean_gci += act_gci;
		act_gc = ((float) act_gci) / act_len;
		if(act_gc < min_gc) min_gc = act_gc;
		if(act_gc > max_gc) max_gc = act_gc;
		mean_len += act_len;
		if(act_len < min_len) min_len = act_len;
		if(act_len > max_len) max_len = act_len;
	}

	if (min_len * 1.3 < max_len) {
	    aw_message("Warning: The length of sequences differ significantly\n"
		       "	be carefull, neighbour joining is sensitiv to\n"
		       "	this kind of error");
	}
	mean_gc = ((float) mean_gci) / mean_len / nentries;
	mean_len /= nentries;


	if(mean_len < 100) {
		aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(PH_TRANSFORMATION_NONE);
		aw_message("Too short sequences!\n   ==> No correction selected!");
		return NULL;
	}

	if(mean_len < 300) {
		aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(PH_TRANSFORMATION_JUKES_CANTOR);
		aw_message("Meanlength shorter than 300\n   ==> Jukes Cantor selected!");
		return NULL;
	}

	if((mean_len < 1000) || ((max_gc / min_gc) < 1.2)) {
		if(mean_len < 1000)
			sprintf(dummy, "Too short sequences for Olsen! \n");
		else
			sprintf(dummy, "Maximal GC (%f) : Minimal GC (%f) < 1.2\n", max_gc, min_gc);
		sprintf(dummy, "%s   ==> Felsenstein selected!", dummy);
		aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(PH_TRANSFORMATION_FELSENSTEIN);
		aw_message(dummy);
		return NULL;
	}

	aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(PH_TRANSFORMATION_OLSEN);
	aw_message("Olsen selected!");
	return NULL;

}



