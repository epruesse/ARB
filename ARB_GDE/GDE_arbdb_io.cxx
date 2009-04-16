#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "AW_rename.hxx"

static int Arbdb_get_curelem(NA_Alignment *dataset)
{
    int curelem;
    curelem = dataset->numelements++;
    if (curelem == 0) {
        dataset->element = (NA_Sequence *) Calloc(5, sizeof(NA_Sequence));
        dataset->maxnumelements = 5;
    }
    else if (curelem == dataset->maxnumelements) {
        (dataset->maxnumelements) *= 2;
        dataset->element = (NA_Sequence *) Realloc((char *)dataset->element,
                                                   dataset->maxnumelements * sizeof(NA_Sequence));
    }
    return curelem;
}

extern int Default_PROColor_LKUP[],Default_NAColor_LKUP[];

static int InsertDatainGDE(NA_Alignment *dataset,GBDATA **the_species,unsigned char **the_names,
                           unsigned char **the_sequences, unsigned long numberspecies,
                           unsigned long maxalignlen, AP_filter *filter, GapCompression compress,
                           bool cutoff_stop_codon)
{
    GBDATA      *gb_name;
    GBDATA      *gb_species;
    GBDATA      *gbd;
    int          newfiltercreated = 0;
    NA_Sequence *this_elem;

    gde_assert((the_species==0) != (the_names==0));

    if (filter==0) {
        filter = new AP_filter;
        filter->init(maxalignlen);
        newfiltercreated=1;
    }
    else {
        size_t fl = filter->filter_len;
        if (fl < maxalignlen) {
            aw_message("Warning Your filter is shorter than the alignment len");
            maxalignlen = fl;
        }
    }

    size_t *seqlen=(size_t *)calloc((unsigned int)numberspecies,sizeof(size_t));
    // sequences may have different length
    {
        unsigned long i;
        for (i=0;i<numberspecies;i++) {
            seqlen[i] = strlen((char *)the_sequences[i]);
        }
    }

    if (cutoff_stop_codon) {
        unsigned long i;
        for (i=0;i<numberspecies;i++) {
            uchar *seq        = the_sequences[i];
            uchar *stop_codon = (uchar*)strchr((char*)seq, '*');
            if (stop_codon) {
                long pos     = stop_codon-seq;
                long restlen = maxalignlen-pos;
                memset(stop_codon, '.', restlen);
            }
        }
    }

    // store (compressed) sequence data in array:
    uchar **sequfilt = (uchar**)calloc((unsigned int)numberspecies+1,sizeof(uchar*));

    if (compress==COMPRESS_ALL) { // compress all gaps and filter positions
        long          len = filter->real_len;
        unsigned long i;

        for (i=0;i<numberspecies;i++) {
            sequfilt[i]   = (uchar*)calloc((unsigned int)len+1,sizeof(uchar));
            long newcount = 0;
            for (unsigned long col=0;(col<maxalignlen);col++) {
                char c = the_sequences[i][col];
                if (!c) break;
                if ((filter->filter_mask[col]) && (c!='-') && (c!='.')) {
                    sequfilt[i][newcount++] = c;
                }
            }
        }
    }
    else {
        if (compress==COMPRESS_VERTICAL_GAPS || // compress vertical gaps (and '.')
            compress == COMPRESS_NONINFO_COLUMNS) // and additionally all columns containing no info (only N or X)
        {
            size_t i;
            bool   isInfo[256];

            for (i=0; i<256; i++) isInfo[i] = true;
            isInfo[uint('-')] = false;
            isInfo[uint('.')] = false;
            if (compress == COMPRESS_NONINFO_COLUMNS) {
                GB_alignment_type type = GBT_get_alignment_type(dataset->gb_main, dataset->alignment_name);
                switch (type) {
                    case GB_AT_RNA:
                    case GB_AT_DNA:
                        isInfo[uint('N')] = false;
                        isInfo[uint('n')] = false;
                        break;
                    case GB_AT_AA:
                        isInfo[uint('X')] = false;
                        isInfo[uint('x')] = false;
                        break;
                    default:
                        gde_assert(0);
                        break;
                }
            }

            for (i=0; i<maxalignlen; i++) {
                if (filter->filter_mask[i]) {
                    bool wantColumn = false;

                    for (size_t n=0; n<numberspecies; n++) {
                        if (i < seqlen[n]) {
                            if (isInfo[uint(the_sequences[n][i])]) {
                                wantColumn = true; // have info -> take column
                                break;
                            }
                        }
                    }
                    if (!wantColumn) {
                        filter->filter_mask[i] = 0;
                        filter->real_len--;
                    }
                }
            }
        }

        long   len = filter->real_len;
        size_t i;

        for (i=0;i<numberspecies;i++) {
            int  c;
            long newcount = 0;
            
            sequfilt[i]      = (uchar*)malloc((unsigned int)len+1);
            sequfilt[i][len] = 0;
            memset(sequfilt[i],'.',len); // Generate empty sequences

            size_t col;
            for (col=0; (col<maxalignlen) && (c=the_sequences[i][col]); col++) {
                if (filter->filter_mask[col]) {
                    sequfilt[i][newcount++] = filter->simplify[c];
                }
            }
        }
    }

    {
        GB_transaction  dummy(GLOBAL_gb_main);
        char           *str   = filter->to_string();
        GB_ERROR        error = GBT_write_string(GLOBAL_gb_main, AWAR_GDE_EXPORT_FILTER, str);
        delete [] str;

        if (error) aw_message(error);
    }

    free(seqlen);

    long number    = 0;
    int  curelem;
    int  bad_names = 0;

    if (the_species) {
        for (gb_species = the_species[number]; gb_species; gb_species = the_species[++number] ) {
            if ((number/10)*10==number) {
                if (aw_status((double)number/(double)numberspecies)) {
                    return 1;
                }
            }
            gb_name = GB_entry(gb_species,"name");

            curelem = Arbdb_get_curelem(dataset);
            this_elem = &(dataset->element[curelem]);
            InitNASeq(this_elem,RNA);
            this_elem->attr = DEFAULT_X_ATTR;
            this_elem->gb_species = gb_species;

            strncpy(this_elem->short_name,GB_read_char_pntr(gb_name),31);

            if (AWTC_name_quality(this_elem->short_name) != 0) bad_names++;

            gbd = GB_entry(gb_species,"author");
            if (gbd)    strncpy(this_elem->authority,GB_read_char_pntr(gbd),79);
            gbd = GB_entry(gb_species,"full_name");
            if (gbd)    strncpy(this_elem->seq_name,GB_read_char_pntr(gbd),79);
            gbd = GB_entry(gb_species,"acc");
            if (gbd) {
                strncpy(this_elem->id,GB_read_char_pntr(gbd),79);
            }
            {
                AppendNA((NA_Base *)sequfilt[number],strlen((const char *)sequfilt[number]),this_elem);
                freeset(sequfilt[number], 0);
            }

            this_elem->comments = strdup("no comments");
            this_elem->comments_maxlen = 1 + (this_elem->comments_len = strlen(this_elem->comments));

            //          if (this_elem->rmatrix &&
            //              IS_REALLY_AA == AW_FALSE) {
            //                  if (IS_REALLY_AA == AW_FALSE)
            //                          Ascii2NA((char *)this_elem->sequence,
            //                                   this_elem->seqlen,
            //                                   this_elem->rmatrix);
            //                  else
            /*
             * Force the sequence to be AA
             */
            {
                this_elem->elementtype = TEXT;
                this_elem->rmatrix = NULL;
                this_elem->tmatrix = NULL;
                this_elem->col_lut = Default_PROColor_LKUP;
            }
            //          }
        }

    }
    else {      // use the_names
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
            if (AWTC_name_quality(this_elem->short_name) != 0) bad_names++;

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

    if (bad_names) {
        aw_message(GBS_global_string("Problematic names found: %i\n"
                                     "External program call may fail or produce invalid results.\n"
                                     "You might want to use 'Generate new names' and read the associated help.",
                                     bad_names));
    }
    
    {
        unsigned long i;
        for (i=0;i<dataset->numelements;i++) {
            dataset->maxlen = MAX(dataset->maxlen,
                                  dataset->element[i].seqlen+dataset->element[i].offset);
        }
        for (i=0;i<numberspecies;i++)
        {
            delete sequfilt[i];
        }
        free(sequfilt);
    }
    if (newfiltercreated) delete filter;
    return 0;
}

void ReadArbdb_plain(char *filename,NA_Alignment *dataset,int type) {
    AWUSE(filename); AWUSE(type);
    ReadArbdb(dataset, true, NULL, COMPRESS_NONE, false);
}

int ReadArbdb2(NA_Alignment *dataset, AP_filter *filter, GapCompression compress, bool cutoff_stop_codon) {
    dataset->gb_main = GLOBAL_gb_main;
    
    GBDATA **the_species;
    long     maxalignlen;
    long     numberspecies = 0;
    uchar  **the_sequences;
    uchar  **the_names;
    char    *error         = gde_cgss.get_sequences(gde_cgss.THIS,
                                                    the_species, the_names, the_sequences,
                                                    numberspecies, maxalignlen);

    gde_assert((the_species==0) != (the_names==0));

    if (error) {
        aw_message(error);
        return 1;
    }

    InsertDatainGDE(dataset,0,the_names,(unsigned char **)the_sequences,numberspecies,maxalignlen,filter,compress, cutoff_stop_codon);
    long i;
    for (i=0;i<numberspecies;i++) {
        delete the_sequences[i];
    }
    delete the_sequences;
    if (the_species) delete the_species;
    else delete the_names;

    return 0;
}



int ReadArbdb(NA_Alignment *dataset, bool marked, AP_filter *filter, GapCompression compress, bool cutoff_stop_codon) {
    GBDATA *gb_species_data;
    GBDATA *gb_species;

    /*ARB_NT END*/
    dataset->gb_main = GLOBAL_gb_main;

    /* Alignment choosen ? */

    gb_species_data = GB_entry(dataset->gb_main,"species_data");
    ErrorOut5(gb_species_data!=0,"species_data not found");

    long     maxalignlen   = GBT_get_alignment_len(GLOBAL_gb_main,dataset->alignment_name);
    GBDATA **the_species;
    long     numberspecies = 0;
    long     missingdata   = 0;

    if (marked) gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);
    else gb_species        = GBT_first_species_rel_species_data(gb_species_data);

    while(gb_species) {
        if (GBT_read_sequence(gb_species,dataset->alignment_name)) numberspecies++;
        else missingdata++;

        if (marked) gb_species = GBT_next_marked_species(gb_species);
        else gb_species        = GBT_next_species(gb_species);
    }

    if (missingdata) {
        aw_message(GBS_global_string("Skipped %li species which did not contain data in '%s'", missingdata, dataset->alignment_name));
    }

    the_species   = (GBDATA**)calloc((unsigned int)numberspecies+1,sizeof(GBDATA*));
    numberspecies = 0;
    
    if (marked) gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);
    else gb_species        = GBT_first_species_rel_species_data(gb_species_data);

    while(gb_species) {
        if (GBT_read_sequence(gb_species,dataset->alignment_name)) {
            the_species[numberspecies]=gb_species;
            numberspecies++;
        }

        if (marked) gb_species = GBT_next_marked_species(gb_species);
        else gb_species        = GBT_next_species(gb_species);
    }

    maxalignlen = GBT_get_alignment_len(GLOBAL_gb_main,dataset->alignment_name);

    char **the_sequences = (char**)calloc((unsigned int)numberspecies+1, sizeof(char*));

    long i;
    for (i=0;the_species[i];i++) {
        the_sequences[i]= (char *)malloc((size_t)maxalignlen+1);
        the_sequences[i][maxalignlen] = 0;
        memset(the_sequences[i],'.',(size_t)maxalignlen);
        const char *data = GB_read_char_pntr( GBT_read_sequence(the_species[i],dataset->alignment_name));
        int size = strlen(data);
        if (size > maxalignlen) size = (int)maxalignlen;
        strncpy(the_sequences[i],data,size);
        //printf("%s \n",the_sequences[i]);
    }
    InsertDatainGDE(dataset,the_species,0,(unsigned char **)the_sequences, numberspecies,maxalignlen,filter,compress, cutoff_stop_codon);
    for (i=0;i<numberspecies;i++) {
        free(the_sequences[i]);
    }
    free(the_sequences);
    free(the_species);

    return 0;
}

int getelem(NA_Sequence *a,int b) {
    if (a->seqlen == 0) return -1;
    
    if (b<a->offset || (b>a->offset+a->seqlen)) {
        switch(a->elementtype) {
            case DNA:
            case RNA:  return 0;
            case PROTEIN:
            case TEXT: return '~';
            case MASK: return '0';
            default:   return '-';
        }
    }

    return a->sequence[b-a->offset];
}

void putelem(NA_Sequence *a,int b,NA_Base c) {
    int      j;
    NA_Base *temp;

    if (b>=(a->offset+a->seqmaxlen)) {
        Warning("Putelem:insert beyond end of sequence space ignored");
    }
    else if (b >= (a->offset)) {
        a->sequence[b-(a->offset)] = c;
    }
    else {
        temp =(NA_Base*)Calloc(a->seqmaxlen+a->offset-b, sizeof(NA_Base));
        switch (a->elementtype) {
            /*
             *  Pad out with gap characters fron the point of insertion to the offset
             */
            case MASK:
                for (j=b;j<a->offset;j++) temp[j-b]='0';
                break;
            case DNA:
            case RNA:
                for (j=b;j<a->offset;j++) temp[j-b]='\0';
                break;
            case PROTEIN:
                for (j=b;j<a->offset;j++) temp[j-b]='-';
                break;
            case TEXT:
            default:
                for (j=b;j<a->offset;j++) temp[j-b]=' ';
                break;
        }

        for (j=0;j<a->seqmaxlen;j++) temp[j+a->offset-b] = a->sequence[j];
        Cfree((char*)a->sequence);
        
        a->sequence     = temp;
        a->seqlen      += (a->offset - b);
        a->seqmaxlen   += (a->offset - b);
        a->offset       = b;
        a->sequence[0]  = c;
    }
}

