#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <aw_awars.hxx>
#include <awt_tree.hxx>

#include "gde.hxx"
#include "GDE_def.h"
#include "GDE_menu.h"
#include "GDE_extglob.h"

long filter_true(char *filter)
{
	long aretrue=0;
	for(long i=0;filter[i];i++)
		if(filter[i]=='1') aretrue++;
	return(aretrue);
}

long istrue_filter(char *filter,long col)
{
	if(filter[col]=='0') return(0);
	else return(1);
}

int Arbdb_get_curelem(NA_Alignment *dataset)
{
	int curelem;
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

static int InsertDatainGDE(NA_Alignment *dataset,GBDATA **the_species,unsigned char **the_names,unsigned char **the_sequences,
                           unsigned long numberspecies,unsigned long maxalignlen,AP_filter *filter, long compress, bool cutoff_stop_codon)
{
    GBDATA *gb_name;
    GBDATA *gb_species;
    GBDATA *gbd;
    int newfiltercreated=0;
    NA_Sequence *this_elem;

    gde_assert((the_species==0) != (the_names==0));

    if(filter==0)
    {
        filter = new AP_filter;
        filter->init(maxalignlen);
        newfiltercreated=1;
    }else{
        size_t fl = filter->filter_len;
        if (fl < maxalignlen){
            aw_message("Warning Your filter is shorter than the alignment len");
            maxalignlen = fl;
        }
    }

    size_t *seqlen=(size_t *)calloc((unsigned int)numberspecies,sizeof(size_t));
    // sequences may have different length
    {
        unsigned long i;
        for(i=0;i<numberspecies;i++){
            seqlen[i] = strlen((char *)the_sequences[i]);
        }
    }

    // added by RALF : 
    if (cutoff_stop_codon) {
        unsigned long i;
        for(i=0;i<numberspecies;i++) {
            uchar *seq        = the_sequences[i];
            uchar *stop_codon = (uchar*)strchr((char*)seq, '*');
            if (stop_codon) {
                long pos     = stop_codon-seq;
                long restlen = maxalignlen-pos;
                memset(stop_codon, '.', restlen);
            }
        }
    }

    uchar **sequfilt=(uchar**)calloc((unsigned int)numberspecies+1,sizeof(uchar*));
    if(compress==2)  // compress all gaps and filter positions
    {
        long len=filter->real_len;
        unsigned long i;
        for(i=0;i<numberspecies;i++)
        {
            sequfilt[i]=(uchar*)calloc((unsigned int)len+1,sizeof(uchar));
            char c;
            long newcount=0;
            for(unsigned long col=0;(col<maxalignlen);col++)	{
                if(!(c=the_sequences[i][col]))	break;
                if ( (filter->filter_mask[col]) && (c!='-') && (c!='.') )
                    sequfilt[i][newcount++]=the_sequences[i][col];
            }
        }
    }else{
        if(compress==1){  // compress vertical gaps (and '.')
            long allaregaps;
            unsigned long i;
            for(i=0;i<maxalignlen;i++)	{
                if (filter->filter_mask[i]) {
                    allaregaps=1;
                    for(size_t n=0;n<numberspecies;n++){
                        if(i >= seqlen[n]) continue; // end of sequence exceeded
                        char c=the_sequences[n][i];
                        if((c!='-')&&(c!='.')){
                            allaregaps=0;
                            break;
                        }
                    }
                    if(allaregaps){
                        filter->filter_mask[i]=0;
                        filter->real_len--;
                    }
                }
            }
        }
        long len=filter->real_len;
        size_t i;
        for(i=0;i<numberspecies;i++)
        {
            int c;
            long newcount=0;
            sequfilt[i]=(uchar*)malloc((unsigned int)len+1);
            sequfilt[i][len] = 0;
            memset(sequfilt[i],'.',len);		// Generate empty sequences

            size_t col;
            for(col=0;(col<maxalignlen)&&(c=the_sequences[i][col]);col++){
                if ( filter->filter_mask[col] )
                    sequfilt[i][newcount++]=filter->simplify[c];
            }
        }
    }

    {
        GBDATA *gb_filter;
        GB_transaction dummy(gb_main);
        gb_filter = GB_search(gb_main,AWAR_GDE_EXPORT_FILTER,GB_STRING);
        char *string = filter->to_string();
        GB_write_string(gb_filter,string);
        delete [] string;
    }

    free(seqlen);

    long number=0;
    int	curelem;

    if (the_species) {
        for (gb_species = the_species[number]; gb_species; gb_species = the_species[++number] ) {
            if((number/10)*10==number)
                if(aw_status((double)number/(double)numberspecies))
                {
                    return(1);
                }
            gb_name = GB_find(gb_species,"name",0,down_level);

            curelem = Arbdb_get_curelem(dataset);
            this_elem = &(dataset->element[curelem]);
            InitNASeq(this_elem,RNA);
            this_elem->attr = DEFAULT_X_ATTR;
            this_elem->gb_species = gb_species;

            strncpy(this_elem->short_name,GB_read_char_pntr(gb_name),31);

            gbd = GB_find(gb_species,"author",0,down_level);
            if (gbd)	strncpy(this_elem->authority,GB_read_char_pntr(gbd),79);
            gbd = GB_find(gb_species,"full_name",0,down_level);
            if (gbd)	strncpy(this_elem->seq_name,GB_read_char_pntr(gbd),79);
            gbd = GB_find(gb_species,"acc",0,down_level);
            if (gbd){
                strncpy(this_elem->id,GB_read_char_pntr(gbd),79);
            }
            {
                AppendNA((NA_Base *)sequfilt[number],strlen((const char *)sequfilt[number]),this_elem);
                free(sequfilt[number]);
                sequfilt[number] = 0;
            }

            this_elem->comments = strdup("no comments");
            this_elem->comments_maxlen = 1 + (this_elem->comments_len = strlen(this_elem->comments));

            //		if (this_elem->rmatrix &&
            //		    IS_REALLY_AA == AW_FALSE) {
            //			if (IS_REALLY_AA == AW_FALSE)
            //				Ascii2NA((char *)this_elem->sequence,
            //					 this_elem->seqlen,
            //					 this_elem->rmatrix);
            //			else
            /*
             * Force the sequence to be AA
             */
            {
                this_elem->elementtype = TEXT;
                this_elem->rmatrix = NULL;
                this_elem->tmatrix = NULL;
                this_elem->col_lut = Default_PROColor_LKUP;
            }
            //		}
        }
    }
    else {	// use the_names
        unsigned char *species_name;

        for (species_name=the_names[number]; species_name; species_name=the_names[++number]) {
            if ((number/10)*10==number) {
                if (aw_status((double)number/(double)numberspecies)) {
                    return 1;
                }
            }

            curelem = Arbdb_get_curelem(dataset);
            this_elem = &(dataset->element[curelem]);
            InitNASeq(this_elem, RNA);
            this_elem->attr = DEFAULT_X_ATTR;
            this_elem->gb_species = 0;

            strncpy((char*)this_elem->short_name, (char*)species_name, 31);
            this_elem->authority[0] = 0;
            this_elem->seq_name[0] = 0;
            this_elem->id[0] = 0;

            {
                AppendNA((NA_Base *)sequfilt[number],strlen((const char *)sequfilt[number]),this_elem);
                delete sequfilt[number]; sequfilt[number] = 0;
            }

            this_elem->comments = strdup("no comments");
            this_elem->comments_maxlen = 1 + (this_elem->comments_len = strlen(this_elem->comments));

            {
                this_elem->elementtype = TEXT;
                this_elem->rmatrix = NULL;
                this_elem->tmatrix = NULL;
                this_elem->col_lut = Default_PROColor_LKUP;
            }
        }
    }

    {
        unsigned long i;
        for(i=0;i<dataset->numelements;i++)
            dataset->maxlen = MAX(dataset->maxlen,
                                  dataset->element[i].seqlen+dataset->element[i].offset);
    }
    {
        unsigned long i;
        for(i=0;i<numberspecies;i++)
        {
            delete sequfilt[i];
        }
        free(sequfilt);
    }
    if(newfiltercreated) delete filter;
    return(0);
}

void ReadArbdb_plain(char *filename,NA_Alignment *dataset,int type)
{
	AWUSE(filename); AWUSE(type);
	ReadArbdb(dataset,(long)1,0,0, 0);
}

int ReadArbdb2(NA_Alignment *dataset,AP_filter *filter,long compress, bool cutoff_stop_codon)
{
	dataset->gb_main = gb_main;
	GBDATA **the_species;
	long maxalignlen;
	long numberspecies=0;
	uchar **the_sequences;
	uchar **the_names;
	char *error = gde_cgss.get_sequences(gde_cgss.THIS,
                                         the_species, the_names, the_sequences,
                                         numberspecies, maxalignlen);

	gde_assert((the_species==0) != (the_names==0));

	if (error) {
		aw_message(error);
		return 1;
	}

	InsertDatainGDE(dataset,0,the_names,(unsigned char **)the_sequences,numberspecies,maxalignlen,filter,compress, cutoff_stop_codon);
	long i;
	for(i=0;i<numberspecies;i++)
	{
		delete the_sequences[i];
	}
	delete the_sequences;
	if (the_species) delete the_species;
	else delete the_names;

	return 0;
}



int ReadArbdb(NA_Alignment *dataset,long marked,AP_filter *filter,long compress, bool cutoff_stop_codon)
{
	GBDATA *gb_species_data;
	GBDATA *gb_species;


	/*ARB_NT END*/
	dataset->gb_main = gb_main;

    /* Alignment choosen ? */


	gb_species_data = GB_find(dataset->gb_main,"species_data",0,down_level);
	ErrorOut5(gb_species_data!=0,"species_data not found");

	long maxalignlen=GBT_get_alignment_len(gb_main,dataset->alignment_name);

	GBDATA **the_species;
	long numberspecies=0;
	long missingdata=0;

	if(marked) gb_species=GBT_first_marked_species_rel_species_data(gb_species_data);
	else gb_species=GBT_first_species_rel_species_data(gb_species_data);
	while(gb_species)
	{
		if(GBT_read_sequence(gb_species,dataset->alignment_name)) numberspecies++;
                else missingdata++;
		if(marked) gb_species = GBT_next_marked_species(gb_species);
		else gb_species       = GBT_next_species(gb_species);
	}

        if (missingdata) {
            aw_message(GBS_global_string("Skipped %li species which did not contain data in '%s'", missingdata, dataset->alignment_name));
        }

	the_species=(GBDATA**)calloc((unsigned int)numberspecies+1,sizeof(GBDATA*));

	numberspecies=0;
	if(marked) gb_species=GBT_first_marked_species_rel_species_data(gb_species_data);
	else gb_species=GBT_first_species_rel_species_data(gb_species_data);
	while(gb_species)
	{
		if(GBT_read_sequence(gb_species,dataset->alignment_name))
		{
			the_species[numberspecies]=gb_species;
			numberspecies++;
		}
		if(marked) gb_species=GBT_next_marked_species(gb_species);
		else gb_species=GBT_next_species(gb_species);
	}

	maxalignlen=GBT_get_alignment_len(gb_main,dataset->alignment_name);

	char **the_sequences=(char**)calloc((unsigned int)numberspecies+1,
	                                    sizeof(char*));

	long i;
	for(i=0;the_species[i];i++) {
		the_sequences[i]= (char *)malloc((size_t)maxalignlen+1);
		the_sequences[i][maxalignlen] = 0;
		memset(the_sequences[i],'.',(size_t)maxalignlen);
		char *data = GB_read_char_pntr( GBT_read_sequence(the_species[i],dataset->alignment_name));
		int size = strlen(data);
		if (size > maxalignlen) size = (int)maxalignlen;
		strncpy(the_sequences[i],data,size);
		//printf("%s \n",the_sequences[i]);
	}
	InsertDatainGDE(dataset,the_species,0,(unsigned char **)the_sequences, numberspecies,maxalignlen,filter,compress, cutoff_stop_codon);
	for(i=0;i<numberspecies;i++) {
		free(the_sequences[i]);
	}
	free(the_sequences);
	free(the_species);

	return 0;
}

int WriteArbdb(NA_Alignment *aln,char *filename,int method,int maskable)
{
	filename=filename;
	method=method;
	maskable=maskable;

	size_t	j;
	int k;
	GBDATA *gb_data;
	GBDATA *gb_name;
	GBDATA *gb_species_data;
	GBDATA	*gbd;
	char *buffer;
	NA_Sequence *this_elem;

	if (!aln->gb_main) {
		Warning("You cannot create an ARBDB Database");
		return (1);
	}
	GB_begin_transaction(aln->gb_main);
	gb_species_data = GB_find(aln->gb_main,"species_data",0,down_level);
	for (j = 0; j < aln->numelements; j++) {
		this_elem = &(aln->element[j]);
		if (!this_elem->gb_species) {
			gb_name = GB_find(gb_species_data, "name", this_elem->short_name, down_2_level);
			if (gb_name) {
				this_elem->gb_species = GB_get_father(gb_name);
			} else {
				this_elem->gb_species =
					GBT_create_species_rel_species_data(gb_species_data,
                                                        this_elem->short_name);
			}
		}
		gb_data = GBT_add_data(this_elem->gb_species,aln->alignment_name, "data", GB_STRING);
		buffer = (char *) calloc(sizeof(char),
                                 this_elem->seqlen + this_elem->offset+1);
		for (k = 0; k < this_elem->seqlen + this_elem->offset; k++) {
			buffer[k] = this_elem->tmatrix[getelem(this_elem, k)];
		}
		GB_write_string(gb_data, buffer);
		free(buffer);
		gbd = GB_search(this_elem->gb_species, "full_name", GB_STRING);
		GB_write_string(gbd, this_elem->seq_name);
		gbd = GB_search(this_elem->gb_species, "acc", GB_STRING);
		GB_write_string(gbd, this_elem->id);

	}
	GB_commit_transaction(aln->gb_main);
	if (GB_read_clients(aln->gb_main)>=0) {
		GB_save(aln->gb_main,0,"b");
	}
	return 0;
}

/*void Updata_Arbdb(Panel_item item,Event *event)
  {
  extern NA_Alignment *DataSet;
  WriteArbdb(DataSet,0,ALL,AW_FALSE);
  }*/


int getelem(NA_Sequence *a,int b)
{
	if(a->seqlen == 0)
		return(-1);
	if(b<a->offset || (b>a->offset+a->seqlen))
		switch(a->elementtype)
		{
            case DNA:
            case RNA:
                return(0);
            case PROTEIN:
            case TEXT:
                return('~');
            case MASK:
                return('0');
            default:
                return('-');
		}
	else
		return(a->sequence[b-a->offset]);
}
/*------Added by Scott Ferguson, Exxon Research & Engineering Co. ---------*/
int isagap(NA_Sequence *a,int b)
{
	int j,newsize;
	newsize=0;
	j=0;
	NA_Base *temp;
	temp=0;

	if (b < a->offset) return(1);

	/*	Check to see if base at given position is a gap */
	switch (a->elementtype) {
		case MASK:
			if (a->sequence[b-a->offset] == '0') return(1);
			else return(0);
		case DNA:
		case RNA:
			if (a->sequence[b-a->offset] == '\0') return(1);
			else return(0);
		case PROTEIN:
			if (a->sequence[b-a->offset] == '-') return(1);
			else return(0);
		case TEXT:
		default:
			if (a->sequence[b-a->offset] == ' ') return(1);
			else return(0);
	}
}
/*-END:-Added by Scott Ferguson, Exxon Research & Engineering Co. ---------*/



void putelem(NA_Sequence *a,int b,NA_Base c)
{
	int j,newsize;
	newsize=0;
	NA_Base *temp;

	if(b>=(a->offset+a->seqmaxlen))
		Warning("Putelem:insert beyond end of sequence space ignored");
	else if(b >= (a->offset))
		a->sequence[b-(a->offset)] = c;
	else
	{
		temp =(NA_Base*)Calloc(a->seqmaxlen+a->offset-b,
                               sizeof(NA_Base));
		switch (a->elementtype)
		{
			/*
             *	Pad out with gap characters fron the point of insertion to the offset
             */
            case MASK:
                for(j=b;j<a->offset;j++)
                    temp[j-b]='0';
                break;
            case DNA:
            case RNA:
                for(j=b;j<a->offset;j++)
                    temp[j-b]='\0';
                break;
            case PROTEIN:
                for(j=b;j<a->offset;j++)
                    temp[j-b]='-';
                break;
            case TEXT:
            default:
                for(j=b;j<a->offset;j++)
                    temp[j-b]=' ';
                break;
		}

		for(j=0;j<a->seqmaxlen;j++)
			temp[j+a->offset-b] = a->sequence[j];
		Cfree((char*)a->sequence);
		a->sequence = temp;
		a->seqlen += (a->offset - b);
		a->seqmaxlen +=(a->offset - b);
		a->offset = b;
		a->sequence[0] = c;
	}
	return;
}

void putcmask(NA_Sequence *a,int b,int c)
{
	int j,newsize;
	int *temp;
	j=0;newsize=0;temp=0;
	if(b >= (a->offset) )
		a->cmask[b-(a->offset)] = c;
	return;
}
