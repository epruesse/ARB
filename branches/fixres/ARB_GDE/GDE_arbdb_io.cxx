#include "GDE_proto.h"

#include <aw_msg.hxx>
#include <arb_progress.h>
#include <AW_rename.hxx>
#include <AP_filter.hxx>
#include <aw_awar_defs.hxx>

#include <algorithm>

// AISC_MKPT_PROMOTE:#ifndef GDE_EXTGLOB_H
// AISC_MKPT_PROMOTE:#include "GDE_extglob.h"
// AISC_MKPT_PROMOTE:#endif

typedef unsigned int UINT;

int Arbdb_get_curelem(NA_Alignment& dataset) {
    int curelem = dataset.numelements++;
    if (curelem == 0) {
        dataset.maxnumelements = 5;
        dataset.element        = (NA_Sequence *)ARB_alloc(dataset.maxnumelements * sizeof(*dataset.element));
    }
    else if (curelem == dataset.maxnumelements) {
        dataset.maxnumelements *= 2;
        ARB_realloc(dataset.element, dataset.maxnumelements);
    }
    return curelem;
}

static void set_constant_fields(NA_Sequence *this_elem) {
    this_elem->attr            = DEFAULT_X_ATTR;
    this_elem->comments        = ARB_strdup("no comments");
    this_elem->comments_maxlen = 1 + (this_elem->comments_len = strlen(this_elem->comments));
    this_elem->rmatrix         = NULL;
    this_elem->tmatrix         = NULL;
    this_elem->col_lut         = Default_PROColor_LKUP;
}

static void AppendNA_and_free(NA_Sequence *this_elem, uchar *& sequfilt) {
    AppendNA((NA_Base *)sequfilt, strlen((const char *)sequfilt), this_elem);
    freenull(sequfilt);
}

__ATTR__USERESULT static int InsertDatainGDE(NA_Alignment&     dataset,
                                             GBDATA          **the_species,
                                             unsigned char   **the_names,
                                             unsigned char   **the_sequences,
                                             unsigned long     numberspecies,
                                             unsigned long     maxalignlen,
                                             const AP_filter  *filter,
                                             GapCompression    compress,
                                             bool              cutoff_stop_codon,
                                             TypeInfo          typeinfo)
{
    GBDATA      *gb_species;
    NA_Sequence *this_elem;
    AP_filter   *allocatedFilter = 0;

    gde_assert(contradicted(the_species, the_names));

    if (filter==0) {
        allocatedFilter = new AP_filter(maxalignlen);
        filter          = allocatedFilter;
    }
    else {
        size_t fl = filter->get_length();
        if (fl < maxalignlen) {
            aw_message("Warning: Your filter is shorter than the alignment len");
            maxalignlen = fl;
        }
    }

    GB_ERROR error = filter->is_invalid();
    if (!error) {
        size_t *seqlen = (size_t *)ARB_calloc((unsigned int)numberspecies, sizeof(size_t));
        // sequences may have different length
        {
            unsigned long i;
            for (i=0; i<numberspecies; i++) {
                seqlen[i] = strlen((char *)the_sequences[i]);
            }
        }

        if (cutoff_stop_codon) {
            unsigned long i;
            fputs("[CUTTING STOP CODONS]\n", stdout);
            for (i=0; i<numberspecies; i++) {
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
        uchar             **sequfilt = (uchar**)ARB_calloc((unsigned int)numberspecies+1, sizeof(*sequfilt));
        GB_alignment_type   alitype  = GBT_get_alignment_type(dataset.gb_main, dataset.alignment_name);

        if (compress==COMPRESS_ALL) { // compress all gaps and filter positions
            long          len = filter->get_filtered_length();
            unsigned long i;

            for (i=0; i<numberspecies; i++) {
                sequfilt[i]   = (uchar*)ARB_calloc((unsigned int)len+1, sizeof(*sequfilt[i]));
                long newcount = 0;
                for (unsigned long col=0; (col<maxalignlen); col++) {
                    char c = the_sequences[i][col];
                    if (!c) break;
                    if ((filter->use_position(col)) && (c!='-') && (c!='.')) {
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
                isInfo[UINT('-')] = false;
                isInfo[UINT('.')] = false;
                if (compress == COMPRESS_NONINFO_COLUMNS) {
                    switch (alitype) {
                        case GB_AT_RNA:
                        case GB_AT_DNA:
                            isInfo[UINT('N')] = false;
                            isInfo[UINT('n')] = false;
                            break;
                        case GB_AT_AA:
                            isInfo[UINT('X')] = false;
                            isInfo[UINT('x')] = false;
                            break;
                        default:
                            gde_assert(0);
                            break;
                    }
                }

                bool  modified     = false;
                char *filterString = filter->to_string();

                for (i=0; i<maxalignlen; i++) {
                    if (filter->use_position(i)) {
                        bool wantColumn = false;

                        for (size_t n=0; n<numberspecies; n++) {
                            if (i < seqlen[n]) {
                                if (isInfo[UINT(the_sequences[n][i])]) {
                                    wantColumn = true; // have info -> take column
                                    break;
                                }
                            }
                        }
                        if (!wantColumn) {
                            filterString[i] = '0';
                            modified        = true;
                        }
                    }
                }

                if (modified) {
                    size_t len = filter->get_length();

                    delete allocatedFilter;
                    filter = allocatedFilter = new AP_filter(filterString, NULL, len);
                }

                free(filterString);
            }

            if (!error) error = filter->is_invalid();

            if (!error) {
                long   len = filter->get_filtered_length();
                size_t i;

                for (i=0; i<numberspecies; i++) {
                    int  c;
                    long newcount = 0;

                    sequfilt[i]      = (uchar*)ARB_alloc((unsigned int)len+1);
                    sequfilt[i][len] = 0;
                    memset(sequfilt[i], '.', len); // Generate empty sequences

                    const uchar *simplify = filter->get_simplify_table();
                    for (size_t col=0; (col<maxalignlen) && (c=the_sequences[i][col]); col++) {
                        if (filter->use_position(col)) {
                            sequfilt[i][newcount++] = simplify[c];
                        }
                    }
                }
            }
        }
        free(seqlen);

        if (!error) {
            GB_transaction ta(db_access.gb_main);

            char *str = filter->to_string();
            error     = GBT_write_string(db_access.gb_main, AWAR_GDE_EXPORT_FILTER, str);
            free(str);
        }

        if (!error) {
            long number    = 0;
            int  curelem;
            int  bad_names = 0;

            int elementtype      = TEXT;
            int elementtype_init = RNA;
            switch (typeinfo) {
                case UNKNOWN_TYPEINFO: gde_assert(0);
                case BASIC_TYPEINFO: break;

                case DETAILED_TYPEINFO:
                    switch (alitype) {
                        case GB_AT_RNA: elementtype = RNA; break;
                        case GB_AT_DNA: elementtype = DNA; break;
                        case GB_AT_AA:  elementtype = PROTEIN; break;
                        default : gde_assert(0); break;
                    }

                    gde_assert(elementtype != TEXT);
                    elementtype_init = elementtype;
                    break;
            }

            if (!error) {
                arb_progress progress("Read data from DB", numberspecies);
                if (the_species) {
                    for (gb_species = the_species[number]; gb_species && !error; gb_species = the_species[++number]) {
                        curelem   = Arbdb_get_curelem(dataset);
                        this_elem = &(dataset.element[curelem]);

                        InitNASeq(this_elem, elementtype_init);
                        this_elem->gb_species = gb_species;

#define GET_FIELD_CONTENT(fieldname,buffer,bufsize) do {                        \
                            gbd = GB_entry(gb_species, fieldname);              \
                            if (gbd) {                                          \
                                const char *val = GB_read_char_pntr(gbd);       \
                                strncpy_terminate(buffer, val, bufsize);        \
                            }                                                   \
                            else buffer[0] = 0;                                 \
                        } while(0)

                        GBDATA *gbd;
                        GET_FIELD_CONTENT("name",      this_elem->short_name, SIZE_SHORT_NAME);
                        GET_FIELD_CONTENT("author",    this_elem->authority,  SIZE_AUTHORITY);
                        GET_FIELD_CONTENT("full_name", this_elem->seq_name,   SIZE_SEQ_NAME);
                        GET_FIELD_CONTENT("acc",       this_elem->id,         SIZE_ID);

                        this_elem->elementtype = elementtype;

                        if (AWTC_name_quality(this_elem->short_name) != 0) bad_names++;
                        AppendNA_and_free(this_elem, sequfilt[number]);
                        set_constant_fields(this_elem);
                        progress.inc_and_check_user_abort(error);
                    }
                }
                else {      // use the_names
                    unsigned char *species_name;

                    for (species_name=the_names[number]; species_name && !error; species_name=the_names[++number]) {
                        curelem   = Arbdb_get_curelem(dataset);
                        this_elem = &(dataset.element[curelem]);

                        InitNASeq(this_elem, elementtype_init);
                        this_elem->gb_species = 0;

                        strncpy(this_elem->short_name, (char*)species_name, SIZE_SHORT_NAME);
                        this_elem->authority[0] = 0;
                        this_elem->seq_name[0]  = 0;
                        this_elem->id[0]        = 0;
                        this_elem->elementtype  = elementtype;

                        if (AWTC_name_quality(this_elem->short_name) != 0) bad_names++;
                        AppendNA_and_free(this_elem, sequfilt[number]);
                        set_constant_fields(this_elem);
                        progress.inc_and_check_user_abort(error);
                    }
                }
            }

            if (!error) {
                if (bad_names) {
                    aw_message(GBS_global_string("Problematic names found: %i\n"
                                                 "External program call may fail or produce invalid results.\n"
                                                 "You might want to use 'Species/Synchronize IDs' and read the associated help.",
                                                 bad_names));
                }

                {
                    unsigned long i;
                    for (i=0; i<dataset.numelements; i++) {
                        dataset.maxlen = std::max(dataset.maxlen,
                                                  dataset.element[i].seqlen+dataset.element[i].offset);
                    }
                    for (i=0; i<numberspecies; i++)
                    {
                        delete sequfilt[i];
                    }
                    free(sequfilt);
                }
            }
        }
    }

    delete allocatedFilter;
    if (error) {
        aw_message(error);
        return 1; // = aborted
    }
    return 0;

}

int ReadArbdb2(NA_Alignment& dataset, AP_filter *filter, GapCompression compress, bool cutoff_stop_codon, TypeInfo typeinfo) {
    // goes to header: __ATTR__USERESULT
    GBDATA **the_species;
    uchar  **the_names;
    uchar  **the_sequences;
    long     maxalignlen;
    long     numberspecies = 0;

    char *error = db_access.get_sequences(the_species, the_names, the_sequences,
                                          numberspecies, maxalignlen);

    gde_assert(contradicted(the_species, the_names));

    if (error) {
        aw_message(error);
        return 1;
    }

    int res = InsertDatainGDE(dataset, 0, the_names, (unsigned char **)the_sequences, numberspecies, maxalignlen, filter, compress, cutoff_stop_codon, typeinfo);
    for (long i=0; i<numberspecies; i++) {
        delete the_sequences[i];
    }
    delete the_sequences;
    if (the_species) delete the_species;
    else delete the_names;

    return res;
}

int ReadArbdb(NA_Alignment& dataset, bool marked, AP_filter *filter, GapCompression compress, bool cutoff_stop_codon, TypeInfo typeinfo) {
    // goes to header: __ATTR__USERESULT

    GBDATA  *gb_species_data = GBT_get_species_data(dataset.gb_main);
    GBDATA **the_species;
    long     numberspecies   = 0;
    long     missingdata     = 0;

    GBDATA *gb_species;
    if (marked) gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);
    else gb_species        = GBT_first_species_rel_species_data(gb_species_data);

    while (gb_species) {
        if (GBT_find_sequence(gb_species, dataset.alignment_name)) numberspecies++;
        else missingdata++;

        if (marked) gb_species = GBT_next_marked_species(gb_species);
        else gb_species        = GBT_next_species(gb_species);
    }

    if (missingdata) {
        aw_message(GBS_global_string("Skipped %li species which did not contain data in '%s'", missingdata, dataset.alignment_name));
    }

    the_species   = (GBDATA**)ARB_calloc((unsigned int)numberspecies+1, sizeof(*the_species));
    numberspecies = 0;

    if (marked) gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);
    else gb_species        = GBT_first_species_rel_species_data(gb_species_data);

    while (gb_species) {
        if (GBT_find_sequence(gb_species, dataset.alignment_name)) {
            the_species[numberspecies]=gb_species;
            numberspecies++;
        }

        if (marked) gb_species = GBT_next_marked_species(gb_species);
        else gb_species        = GBT_next_species(gb_species);
    }

    long   maxalignlen   = GBT_get_alignment_len(db_access.gb_main, dataset.alignment_name);
    char **the_sequences = (char**)ARB_calloc((unsigned int)numberspecies+1, sizeof(*the_sequences));

    for (long i=0; the_species[i]; i++) {
        the_sequences[i] = (char *)ARB_alloc((size_t)maxalignlen+1);
        the_sequences[i][maxalignlen] = 0;
        memset(the_sequences[i], '.', (size_t)maxalignlen);
        const char *data = GB_read_char_pntr(GBT_find_sequence(the_species[i], dataset.alignment_name));
        int size = strlen(data);
        if (size > maxalignlen) size = (int)maxalignlen;
        strncpy_terminate(the_sequences[i], data, size+1);
    }

    int res = InsertDatainGDE(dataset, the_species, 0, (unsigned char **)the_sequences, numberspecies, maxalignlen, filter, compress, cutoff_stop_codon, typeinfo);
    for (long i=0; i<numberspecies; i++) {
        free(the_sequences[i]);
    }
    free(the_sequences);
    free(the_species);

    return res;
}

int getelem(NA_Sequence *a, int b) {
    if (a->seqlen == 0) return -1;

    if (b<a->offset || (b>a->offset+a->seqlen)) {
        switch (a->elementtype) {
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

void putelem(NA_Sequence *a, int b, NA_Base c) {
    if (b>=(a->offset+a->seqmaxlen)) {
        Warning("Putelem:insert beyond end of sequence space ignored");
    }
    else if (b >= (a->offset)) {
        a->sequence[b-(a->offset)] = c;
    }
    else {
        NA_Base *temp = (NA_Base*)ARB_calloc(a->seqmaxlen + a->offset - b, sizeof(*temp));
        switch (a->elementtype) {
            // Pad out with gap characters fron the point of insertion to the offset
            case MASK:
                for (int j=b; j<a->offset; j++) temp[j-b] = '0';
                break;

            case DNA:
            case RNA:
                for (int j=b; j<a->offset; j++) temp[j-b] = '\0';
                break;

            case PROTEIN:
                for (int j=b; j<a->offset; j++) temp[j-b] = '-';
                break;
                
            case TEXT:
            default:
                for (int j=b; j<a->offset; j++) temp[j-b] = ' ';
                break;
        }

        for (int j=0; j<a->seqmaxlen; j++) temp[j+a->offset-b] = a->sequence[j];
        free(a->sequence);

        a->sequence     = temp;
        a->seqlen      += (a->offset - b);
        a->seqmaxlen   += (a->offset - b);
        a->offset       = b;
        a->sequence[0]  = c;
    }
}

