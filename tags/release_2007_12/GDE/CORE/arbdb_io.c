#include <stdio.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/notice.h>

#include <arbdb.h>
#include <arbdbt.h>
#include "menudefs.h"
#include "defines.h"
	extern Frame frame;

int
Arbdb_get_curelem(NA_Alignment *dataset)
{
	int             curelem;
	curelem = dataset->numelements++;
	if (curelem == 0) {
		dataset->element = (NA_Sequence *)
			Calloc(5, sizeof(NA_Sequence));
		dataset->maxnumelements = 5;
	} else if (curelem == dataset->maxnumelements) {
		(dataset->maxnumelements) *= 2;
		dataset->element = (NA_Sequence *)
			Realloc((char *)dataset->element,
			     dataset->maxnumelements * sizeof(NA_Sequence));
	}
	return curelem;
}

extern int Default_PROColor_LKUP[],Default_NAColor_LKUP[];

void ReadArbdb(filename,dataset,type)
char *filename;
NA_Alignment *dataset;
int type;
{
	NA_Sequence *this_elem;
	int	curelem;
	int	j;
	GBDATA *gb_species;
	GBDATA *gb_ali;
	GBDATA *gb_data;
	GBDATA *gb_name;
	GBDATA *gbd;
	int	IS_REALLY_AA;

	if (dataset->gb_main) GB_close(dataset->gb_main);
	dataset->gb_main = GB_open(filename,"rwt");
	if (!dataset->gb_main) {
		ErrorOut1(0,"File not found");
	}
	GB_begin_transaction(dataset->gb_main);
	GB_change_my_security(dataset->gb_main,6,"passwd");
	dataset->use = GBT_get_default_alignment(dataset->gb_main);
	IS_REALLY_AA = GBT_is_alignment_protein(dataset->gb_main,dataset->use);
	for (	gb_species = GBT_first_marked_species(dataset->gb_main);
		gb_species;
		GB_release(gb_species),
		gb_species = GBT_next_marked_species(gb_species) ) {
		gb_ali = GB_find(gb_species,dataset->use,0,down_level);
		if (!gb_ali) continue;
		gb_data = GB_find(gb_ali,"data",0,down_level);
		if (!gb_data) continue;
		gb_name = GB_find(gb_species,"name",0,down_level);

		curelem = Arbdb_get_curelem(dataset);
		this_elem = &(dataset->element[curelem]);
		if (IS_REALLY_AA) {
			InitNASeq(this_elem,PROTEIN);
		}else{
			InitNASeq(this_elem,RNA);
		}
		this_elem->attr = DEFAULT_X_ATTR;
		this_elem->gb_species = gb_species;

		strncpy(this_elem->short_name,GB_read_char_pntr(gb_name),31);


		gbd = GB_find(gb_species,"author",0,down_level);
		if (gbd)
			strncpy(this_elem->authority,GB_read_char_pntr(gbd),79);
		gbd = GB_find(gb_species,"full_name",0,down_level);
		if (gbd)
			strncpy(this_elem->seq_name,GB_read_char_pntr(gbd),79);
		gbd = GB_find(gb_species,"acc",0,down_level);
		if (gbd)
			strncpy(this_elem->id,GB_read_char_pntr(gbd),79);
		AppendNA((NA_Base *)GB_read_char_pntr(gb_data),
				GB_read_string_count(gb_data),
				this_elem);
		this_elem->comments = strdup("no comments");
		this_elem->comments_maxlen = 1 +
		(this_elem->comments_len = strlen("no comments"));

		if (this_elem->rmatrix &&
		    IS_REALLY_AA == FALSE) {
			if (IS_REALLY_AA == FALSE)
				Ascii2NA((char *)this_elem->sequence,
					 this_elem->seqlen,
					 this_elem->rmatrix);
			else
				/*
				 * Force the sequence to be AA
				 */
			{
				this_elem->elementtype = PROTEIN;
				this_elem->rmatrix = NULL;
				this_elem->tmatrix = NULL;
				this_elem->col_lut =
					Default_PROColor_LKUP;
			}
		}
	}
	GB_commit_transaction(dataset->gb_main);
	for(j=0;j<dataset->numelements;j++)
		dataset->maxlen = MAX(dataset->maxlen,
		    dataset->element[j].seqlen+dataset->element[j].offset);
	return;
}

WriteArbdb(aln,filename,method,maskable)
     NA_Alignment *aln;
     char         *filename;
     int           method,maskable;
{
	int	j,k;
/* 	GBDATA *gb_ali; */
	GBDATA *gb_data;
/* 	GBDATA *gb_name; */
	GBDATA *gb_species_data;
	GBDATA	*gbd;
	long	clock;
	long	ali_len;
	char *buffer;
	NA_Sequence *this_elem;

	if (!aln->gb_main) {
		Warning("You cannot create an ARBDB Database");
		return (1);
	}

	GB_begin_transaction(aln->gb_main);
	ali_len = GBT_get_alignment_len(aln->gb_main,aln->use);
	GB_change_my_security(aln->gb_main,6,0);
	gb_species_data = GB_find(aln->gb_main,"species_data",0,down_level);
	for (j = 0; j < aln->numelements; j++) {
		GBDATA *gb_species;
		this_elem = &(aln->element[j]);
		gb_species = GBT_find_species_rel_species_data(gb_species_data, this_elem->short_name);

		if (!this_elem->gb_species) {
			if (gb_species) {	/* new element that already exists !!!!*/
				int select_mode = notice_prompt(frame,NULL,NOTICE_MESSAGE_STRINGS,
					GBS_global_string(
					"You created a species '%s' which already exists\n"
					"Do you wish to ...",this_elem->short_name),NULL,
					NOTICE_BUTTON,"overwrite existing species",0,
					NOTICE_BUTTON,"overwrite existing sequence only",1,
					NOTICE_BUTTON,"not write this sequence",2,
				NULL);
				switch(select_mode) {
				case 0:
					GB_delete(gb_species);
				case 1:
					this_elem->gb_species =
						GBT_create_species_rel_species_data(gb_species_data,
						     this_elem->short_name);
						this_elem->transaction_nr =
							GB_read_clock(this_elem->gb_species);
					break;
				case 2:
					this_elem->gb_species = 0;
					continue;
				}
			}else{
				int select_mode = notice_prompt(frame,NULL,NOTICE_MESSAGE_STRINGS,
					GBS_global_string(
					"You created a species '%s' which does not exists\n"
					"Do you wish to ...",this_elem->short_name),NULL,
					NOTICE_BUTTON,"Create species",0,
					NOTICE_BUTTON,"or do not write this sequence",1,
				NULL);
				switch(select_mode) {
				case 0:
					this_elem->gb_species =
						GBT_create_species_rel_species_data(gb_species_data,
						     this_elem->short_name);
						this_elem->transaction_nr =
							GB_read_clock(this_elem->gb_species);
					break;
				default:
					continue;
				}
			}
		}else if (this_elem->gb_species != gb_species) {	/* a copied species */
			if (gb_species) {
				int select_mode = notice_prompt(frame,NULL,NOTICE_MESSAGE_STRINGS,
					GBS_global_string(
					"You created/renamed a species to '%s' which already exists\n"
					"Do you wish to ...",this_elem->short_name),NULL,
					NOTICE_BUTTON,"delete old species",0,
					NOTICE_BUTTON,"or do not write this sequence",1,
				NULL);
				switch(select_mode) {
				case 0:
					GB_delete(gb_species);
					this_elem->gb_species =
						GBT_create_species_rel_species_data(gb_species_data,
						     this_elem->short_name);
						this_elem->transaction_nr =
							GB_read_clock(this_elem->gb_species);
					break;
				default:
					continue;
				}
			}else{
				int select_mode = notice_prompt(frame,NULL,NOTICE_MESSAGE_STRINGS,
					GBS_global_string(
					"You created/renamed a species to '%s'\n"
					"Do you wish to ...",this_elem->short_name),NULL,
					NOTICE_BUTTON,"Create species",0,
					NOTICE_BUTTON,"or do not write this sequence",1,
				NULL);
				switch(select_mode) {
				case 0:
					this_elem->gb_species =
						GBT_create_species_rel_species_data(gb_species_data,
						     this_elem->short_name);
					this_elem->transaction_nr =
						GB_read_clock(this_elem->gb_species);

					break;
				default:
					continue;
				}
			}
		}


		if (!this_elem->gb_species) continue;

		clock = GB_read_clock(this_elem->gb_species);
		if (clock > this_elem->transaction_nr) {
			int select_mode = notice_prompt(frame,NULL,NOTICE_MESSAGE_STRINGS,
				GBS_global_string(
				"Anybody has changed the species '%s' meanwhile\n"
				"This may also indicate duplicated sequences in your editor\n"
				"Do you wish to ...",this_elem->short_name),NULL,
				NOTICE_BUTTON,"Update species nethertheless",0,
				NOTICE_BUTTON,"leave existing sequence intact",1,
			NULL);
			if(select_mode) continue;
		}

		gb_data = GBT_add_data(this_elem->gb_species,
				       aln->use, "data", GB_STRING);
		buffer = (char *) calloc(sizeof(char),
				     this_elem->seqlen + this_elem->offset+1);
		if (this_elem->tmatrix) {
			for (k = 0; k < this_elem->seqlen + this_elem->offset; k++) {
				buffer[k] = this_elem->tmatrix[getelem(this_elem, k)];
			}
		}else{
			for (k = 0; k < this_elem->seqlen + this_elem->offset; k++) {
				buffer[k] = getelem(this_elem, k);
			}
		}
		GBT_write_sequence(gb_data,aln->use, ali_len, buffer);
		free(buffer);
		if (this_elem->seq_name) {
			gbd = GB_search(this_elem->gb_species, "full_name", GB_STRING);
			GB_write_string(gbd, this_elem->seq_name);
		}
		if (this_elem->id) {
			gbd = GB_search(this_elem->gb_species, "acc", GB_STRING);
			GB_write_string(gbd, this_elem->id);
		}
		this_elem->transaction_nr = GB_read_clock(this_elem->gb_species);
	}	/* for j */
	GB_commit_transaction(aln->gb_main);
	if (GB_read_clients(aln->gb_main)>=0) {
		GB_save(aln->gb_main,0,"b");
	}
	return 0;
}

void Updata_Arbdb(item,event)
Panel_item item;
Event *event;
{
	extern NA_Alignment *DataSet;
	WriteArbdb(DataSet,0,ALL,FALSE);
}
