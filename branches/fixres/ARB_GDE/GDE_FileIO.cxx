#include "GDE_proto.h"
#include <limits.h>
#include <aw_msg.hxx>
#include <algorithm>

static void Regroup(NA_Alignment& alignment) {
    size_t j;
    size_t group;
    int last;

    for (j=0; j<alignment.numelements; j++) {
        alignment.element[j].groupf = NULL;
        alignment.element[j].groupb = NULL;
    }

    for (group = 1; group <= alignment.numgroups; group++) {
        last = -1;
        for (j=0; j<alignment.numelements; j++)
            if (alignment.element[j].groupid == group) {
                if (last != -1) {
                    alignment.element[j].groupb    = &(alignment.element[last]);
                    alignment.element[last].groupf = &(alignment.element[j]);
                }
                last = j;
            }
    }
}

char *Calloc(int count, int size) { // @@@ eliminate
    // More robust memory management routines
    return (char*)ARB_calloc(count, size);
}

char *Realloc(char *block, int size) { // @@@ eliminate
    ARB_realloc(block, size);
    return block;
}

void Cfree(char *block) // @@@ replace by free
{
    if (block) {
        free(block);
    }
    else
        Warning("Error in Cfree, NULL block");
    return;
}

static void ReadNA_Flat(char *filename, NA_Alignment& data) {
    size_t j;
    int jj, curelem=0, offset;
    char buffer[GBUFSIZ];
    char in_line[GBUFSIZ];

    NA_Sequence *this_elem;

    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Cannot open %s.\n", filename);
    }
    else {
        for (; fgets(in_line, GBUFSIZ, file) != 0;)
        {
            if (in_line[0] == '#' ||
                in_line[0] == '%' ||
                in_line[0] == '"' ||
                in_line[0] == '@')
            {
                offset = 0;
                for (j=0; j<strlen(in_line); j++)
                {
                    if (in_line[j] == '(')
                    {
                        sscanf((char*)
                               &(in_line[j+1]), "%d", &offset);
                        in_line[j] = '\0';
                    }
                }

                curelem = data.numelements++;
                if (curelem == 0)
                {
                    data.element=(NA_Sequence*)Calloc(5, sizeof(NA_Sequence));
                    data.maxnumelements = 5;
                }
                else if (curelem==data.maxnumelements)
                {
                    data.maxnumelements *= 2;
                    data.element = (NA_Sequence*)Realloc((char*)data.element, data.maxnumelements*sizeof(NA_Sequence));
                }

                InitNASeq(&(data.element[curelem]),
                          in_line[0] == '#' ? DNA :
                          in_line[0] == '%' ? PROTEIN :
                          in_line[0] == '"' ? TEXT :
                          in_line[0] == '@' ? MASK : TEXT);
                this_elem = &(data.element[curelem]);
                if (in_line[strlen(in_line)-1] == '\n')
                    in_line[strlen(in_line)-1] = '\0';
                strncpy_terminate(this_elem->short_name, in_line+1, SIZE_SHORT_NAME);
                this_elem->offset = offset;
            }
            else if (in_line[0] != '\n')
            {
                size_t strl = strlen(in_line);
                for (j=0, jj=0; j<strl; j++)
                    if (in_line[j] != ' ' && in_line[j] != '\n' &&
                        in_line[j] != '\t')
                        buffer[jj++] = in_line[j];

                if (data.element[curelem].rmatrix)
                    Ascii2NA(buffer, jj, data.element[curelem].rmatrix);
                AppendNA((NA_Base*)buffer, jj, &(data.element[curelem]));
            }
        }
        fclose(file);

        for (j=0; j<data.numelements; j++) {
            data.maxlen = std::max(data.maxlen, data.element[j].seqlen + data.element[j].offset);
        }

        NormalizeOffset(data);
        Regroup(data);
    }
}

/*
  LoadFile():
  Load the given filename into the given dataset.  Handle any
  type conversion needed to get the data into the specified data type.
  This routine is used in situations where the format and datatype is known.

  Copyright (c) 1989-1990, University of Illinois board of trustees.  All
  rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
  Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
  Carl Woese.

  Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
  All rights reserved.
*/

static GB_ERROR LoadFile(char *filename, NA_Alignment& dataset, int type, int format) {
    GB_ERROR error = NULL;
    if (DataType != type)
        fprintf(stderr, "Warning, datatypes do not match.\n");
    /*
      Handle the overwrite/create/merge dialog here.
    */
    switch (format)
    {
        case NA_FLAT:
            ReadNA_Flat(filename, dataset);
            dataset.format = GDE;
            break;

        case GENBANK:
            error          = ReadGen(filename, dataset);
            dataset.format = GENBANK;
            break;

        case GDE:
            gde_assert(0); // should no longer occur
            break;

        default:
            break;
    }
    return error;
}

static int FindType(char *name, int *dtype, int *ftype) {
    FILE *file = fopen(name, "r");

    *dtype = 0;
    *ftype = 0;

    int result = 1;
    if (file) {
        /*   Is this a flat file?
         *   Get the first non blank line, see if a type marker shows up.
         */
        char in_line[GBUFSIZ];
        if (fgets(in_line, GBUFSIZ, file)) {
            for (; strlen(in_line)<2 && fgets(in_line, GBUFSIZ, file) != NULL;) ;

            if (in_line[0] == '#' || in_line[0] == '%' ||
                in_line[0]  == '"' || in_line[0] == '@')
            {
                *dtype=NASEQ_ALIGN;
                *ftype=NA_FLAT;
                result = 0;
            }
            else { // try genbank
                fclose(file);
                file = fopen(name, "r");
                *dtype=0;
                *ftype=0;

                if (file) {
                    for (; fgets(in_line, GBUFSIZ, file) != NULL;) {
                        if (Find(in_line, "LOCUS"))
                        {
                            *dtype = NASEQ_ALIGN;
                            *ftype = GENBANK;
                            break;
                        }
                        else if (Find(in_line, "sequence")) { // try GDE
                            *dtype = NASEQ_ALIGN;
                            *ftype = GDE;
                            break;
                        }
                    }
                    result = 0;
                }
            }
        }
        fclose(file);
    }

    return result;
}

void LoadData(char *filen, NA_Alignment& dataset) {
    /* LoadData():
     * Load a data set from the command line argument.
     *
     * Copyright (c) 1989, University of Illinois board of trustees.  All rights
     * reserved.  Written by Steven Smith at the Center for Prokaryote Genome
     * Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
     * Carl Woese.
     * Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
     * All rights reserved.
     */

    // Get file name, determine the file type, and away we go..
    if (Find2(filen, "gde") != 0)
        strcpy(FileName, filen);

    FILE *file = fopen(filen, "r");
    if (file) {
        FindType(filen, &DataType, &FileFormat);
        switch (DataType) {
            case NASEQ_ALIGN: {
                GB_ERROR error = LoadFile(filen, dataset, DataType, FileFormat);
                if (error) aw_message(error);
                break;
            }
            default:
                aw_message(GBS_global_string("Internal error: unknown file type of file %s", filen));
                break;
        }
        fclose(file);
    }
}

void AppendNA(NA_Base *buffer, int len, NA_Sequence *seq)
{
    int curlen=0, j;
    if (seq->seqlen+len >= seq->seqmaxlen)
    {
        if (seq->seqlen>0)
            seq->sequence = (NA_Base*)Realloc((char*)seq->sequence,
                                              (seq->seqlen + len+GBUFSIZ) * sizeof(NA_Base));
        else
            seq->sequence = (NA_Base*)Calloc(1, (seq->seqlen +
                                                len+GBUFSIZ) * sizeof(NA_Base));
        seq->seqmaxlen = seq->seqlen + len+GBUFSIZ;
    }
    // seqlen is the length, and the index of the next free base
    curlen = seq->seqlen + seq->offset;
    for (j=0; j<len; j++)
        putelem(seq, j+curlen, buffer[j]);

    seq->seqlen += len;
    return;
}

void Ascii2NA(char *buffer, int len, int matrix[16]) {
    // if the translation matrix exists, use it to encode the buffer.
    if (matrix != NULL) {
        for (int i=0; i<len; i++) {
            buffer[i] = matrix[(unsigned char)buffer[i]];
        }
    }
}

int WriteNA_Flat(NA_Alignment& aln, char *filename, int method)
{
    if (aln.numelements == 0) return (1);

    size_t  j;
    int     kk;
    int     k, offset;
    char    offset_str[100], buf[100];
    FILE   *file;

    NA_Sequence *seqs = aln.element;

    file = fopen(filename, "w");
    if (file == NULL)
    {
        Warning("Cannot open file for output");
        return (1);
    }

    for (j=0; j<aln.numelements; j++)
    {
        offset = seqs[j].offset;

        if (offset+aln.rel_offset != 0)
            sprintf(offset_str, "(%d)", offset+aln.rel_offset);
        else
            offset_str[0] = '\0';

        if (method == ALL)
        {
            fprintf(file, "%c%s%s\n",
                    seqs[j].elementtype == DNA ? '#' :
                    seqs[j].elementtype == RNA ? '#' :
                    seqs[j].elementtype == PROTEIN ? '%' :
                    seqs[j].elementtype == TEXT ? '"' :
                    seqs[j].elementtype == MASK ? '@' : '"',
                    seqs[j].short_name,
                    (offset+aln.rel_offset  == 0) ? "" : offset_str);
            if (seqs[j].tmatrix)
            {
                for (k=0, kk=0; kk<seqs[j].seqlen; kk++)
                {
                    if ((k)%60 == 0 && k>0)
                    {
                        buf[60] = '\0';
                        fputs(buf, file);
                        putc('\n', file);
                    }
                    buf[k%60] = ((char)seqs[j].tmatrix[(int)getelem(&(seqs[j]), kk+offset)]);
                    k++;
                }
            }
            else
            {
                for (k=0, kk=0; kk<seqs[j].seqlen; kk++)
                {
                    if ((k)%60 == 0 && k>0)
                    {
                        buf[60] = '\0';
                        fputs(buf, file);
                        putc('\n', file);
                    }
                    buf[k%60] = (getelem(&(seqs[j]), kk+offset));
                    k++;
                }
            }
            buf[(k%60)>0 ? (k%60) : 60] = '\0';
            fputs(buf, file);
            putc('\n', file);
        }
    }
    fclose(file);
    return (0);
}

void Warning(const char *s) {
    aw_message(s);
}


void InitNASeq(NA_Sequence *seq, int type) {
    SetTime(&(seq->t_stamp.origin));
    SetTime(&(seq->t_stamp.modify));
    strncpy_terminate(seq->id, uniqueID(), SIZE_ID);
    seq->seq_name[0] = '\0';
    seq->barcode[0] = '\0';
    seq->contig[0] = '\0';
    seq->membrane[0] = '\0';
    seq->authority[0] = '\0';
    seq->short_name[0] = '\0';
    seq->sequence = NULL;
    seq->offset = 0;
    seq->baggage = NULL;
    seq->baggage_len = 0;
    seq->baggage_maxlen = 0;
    seq->comments = NULL;
    seq->comments_len = 0;
    seq->comments_maxlen = 0;
    seq->description[0] = '\0';
    seq->seqlen = 0;
    seq->seqmaxlen = 0;
    seq->attr = IS_5_TO_3 + IS_PRIMARY;
    seq->elementtype = type;
    seq->groupid = 0;
    seq->groupb = NULL;
    seq->groupf = NULL;

    switch (type)
    {
        case DNA:
            seq->tmatrix = Default_DNA_Trans;
            seq->rmatrix = Default_NA_RTrans;
            seq->col_lut = Default_NAColor_LKUP;
            break;
        case RNA:
            seq->tmatrix = Default_RNA_Trans;
            seq->rmatrix = Default_NA_RTrans;
            seq->col_lut = Default_NAColor_LKUP;
            break;
        case PROTEIN:
            seq->tmatrix = NULL;
            seq->rmatrix = NULL;
            seq->col_lut = Default_PROColor_LKUP;
            break;
        case MASK:
        case TEXT:
        default:
            seq->tmatrix = NULL;
            seq->rmatrix = NULL;
            seq->col_lut = NULL;
            break;
    }
    return;
}

void NormalizeOffset(NA_Alignment& aln)
{
    size_t j;
    int offset = INT_MAX;

    for (j=0; j<aln.numelements; j++)
        offset = std::min(offset, aln.element[j].offset);

    for (j=0; j<aln.numelements; j++)
        aln.element[j].offset -= offset;

    aln.maxlen = INT_MIN;
    for (j=0; j<aln.numelements; j++)
        aln.maxlen = std::max(aln.element[j].seqlen+aln.element[j].offset, aln.maxlen);

    gde_assert(aln.maxlen >= 0);

    aln.rel_offset += offset;

    if (aln.numelements == 0) aln.rel_offset = 0;
}
