#include "GDE_extglob.h"
#include <ctime>
#include <algorithm>

/*
  Copyright (c) 1989-1990, University of Illinois board of trustees.  All
  rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
  Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
  Carl Woese.

  Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
  all rights reserved.

  Copyright (c) 1993, Steven Smith, all rights reserved.

*/

static bool CheckType(char *seq, int len) {
    /*   CheckType:  Check base composition to see if the sequence
     *   appears to be an amino acid sequence.
     */
    int j, count1 = 0, count2 = 0;

    for (j=0; j<len; j++)
        if (((seq[j]|32) < 'z') && ((seq[j]|32) > 'a'))
        {
            count1++;
            if (strchr("ACGTUNacgtun", seq[j]) == NULL)
                count2++;
        }

    return ((count2 > count1/4) ? true : false);
}

// ARB
struct ARB_TIME {
    int yy;
    int mm;
    int dd;
    int hr;
    int mn;
    int sc;
};

static void AsciiTime(void *b, char *asciitime)
{
    ARB_TIME *a=(ARB_TIME*)b;
    int j;
    char temp[GBUFSIZ];

    a->dd = 0;
    a->yy = 0;
    a->mm = 0;
    sscanf(asciitime, "%d%5c%d", &(a->dd), temp, &(a->yy));
    temp[5] = '\0';
    for (j=0; j<12; j++)
        if (strcmp(temp, GDEmonth[j]) == 0)
            a->mm = j+1;
    if (a->dd <0 || a->dd > 31 || a->yy < 0 || a->mm > 11)
        SetTime(a);
    return;
}
// ENDARB

GB_ERROR ReadGen(char *filename, NA_Alignment& dataset) {
    GB_ERROR  error = NULL;
    FILE     *file  = fopen(filename, "r");
    if (!file) {
        error = GB_IO_error("reading", filename);
    }
    else {
        bool    done         = false;
        size_t  len          = 0;
        bool    IS_REALLY_AA = false;
        char    in_line[GBUFSIZ], c;
        char   *buffer       = 0, *gencomments = NULL, fields[8][GBUFSIZ];
        size_t  buflen       = 0;
        int     genclen      = 0, curelem = 0, n = 0;
        int     start_col    = -1;

        NA_Sequence *this_elem = 0;

        for (; fgets(in_line, GBUFSIZ, file) != 0;)
        {
            if (in_line[strlen(in_line)-1] == '\n')
                in_line[strlen(in_line)-1] = '\0';
            if (Find(in_line, "LOCUS"))
            {
                curelem   = Arbdb_get_curelem(dataset);
                this_elem = &(dataset.element[curelem]);
                n         = sscanf(in_line, "%s %s %s %s %s %s %s %s",
                                   fields[0], fields[1], fields[2], fields[3], fields[4],
                                   fields[5], fields[6], fields[7]);

                if (IS_REALLY_AA)
                {
                    InitNASeq(this_elem, PROTEIN);
                }
                else if (Find(in_line, "DNA"))
                {
                    InitNASeq(this_elem, DNA);
                }
                else if (Find(in_line, "RNA"))
                {
                    InitNASeq(this_elem, RNA);
                }
                else if (Find(in_line, "MASK"))
                {
                    InitNASeq(this_elem, MASK);
                }
                else if (Find(in_line, "TEXT"))
                {
                    InitNASeq(this_elem, TEXT);
                }
                else if (Find(in_line, "PROT"))
                {
                    InitNASeq(this_elem, PROTEIN);
                }
                else
                    InitNASeq(this_elem, DNA);

                strncpy_terminate(this_elem->short_name, fields[1], SIZE_SHORT_NAME);
                AsciiTime(&(this_elem->t_stamp.origin), fields[n-1]);
                this_elem->attr = DEFAULT_X_ATTR;

                if (Find(in_line, "Circular"))
                    this_elem->attr |= IS_CIRCULAR;

                gencomments = NULL;
                genclen = 0;
            }
            else if (Find(in_line, "DEFINITION"))
                strncpy_terminate(this_elem->description, in_line+12, SIZE_DESCRIPTION);

            else if (Find(in_line, "AUTHOR"))
                strncpy_terminate(this_elem->authority, in_line+12, SIZE_AUTHORITY);

            else if (Find(in_line, "  ORGANISM"))
                strncpy_terminate(this_elem->seq_name, in_line+12, SIZE_SEQ_NAME);

            else if (Find(in_line, "ACCESSION"))
                strncpy_terminate(this_elem->id, in_line+12, SIZE_ID);

            else if (Find(in_line, "ORIGIN"))
            {
                done = false;
                len = 0;
                for (; !done && fgets(in_line, GBUFSIZ, file) != 0;)
                {
                    if (in_line[0] != '/')
                    {
                        if (buflen == 0) {
                            buflen = GBUFSIZ;
                            ARB_calloc(buffer, buflen);
                        }
                        else if (len+strlen(in_line) >= buflen) {
                            size_t new_buflen = buflen+GBUFSIZ;
                            ARB_recalloc(buffer, buflen, new_buflen);
                            buflen            = new_buflen;
                        }
                        // Search for the fist column of data (whitespace-number-whitespace)data
                        if (start_col == -1)
                        {
                            for (start_col=0; in_line[start_col] == ' ' || in_line[start_col] == '\t'; start_col++) ;
                            for (start_col++; strchr("1234567890", in_line[start_col]) != NULL; start_col++) ;
                            for (start_col++; in_line[start_col] == ' ' || in_line[start_col] == '\t'; start_col++) ;
                        }
                        for (int j=start_col; (c = in_line[j]) != '\0'; j++)
                        {
                            if ((c != '\n') && ((j-start_col + 1) % 11 != 0)) {
                                buffer[len++] = c;
                            }
                        }
                    }
                    else
                    {
                        AppendNA((NA_Base*)buffer, len, &(dataset.element[curelem]));
                        for (size_t j=0; j<len; j++) buffer[j] = '\0';
                        len = 0;
                        done = true;
                        dataset.element[curelem].comments = gencomments;
                        dataset.element[curelem].comments_len= genclen - 1;
                        dataset.element[curelem].comments_maxlen = genclen;

                        gencomments = NULL;
                        genclen = 0;
                    }
                }
                /* Test if sequence should be converted by the translation table
                 * If it looks like a protein...
                 */
                if (dataset.element[curelem].rmatrix && !IS_REALLY_AA) {
                    IS_REALLY_AA = CheckType((char*)dataset.element[curelem]. sequence, dataset.element[curelem].seqlen);

                    if (!IS_REALLY_AA)
                        Ascii2NA((char*)dataset.element[curelem].sequence,
                                 dataset.element[curelem].seqlen,
                                 dataset.element[curelem].rmatrix);
                    else {
                        // Force the sequence to be AA
                        dataset.element[curelem].elementtype = PROTEIN;
                        dataset.element[curelem].rmatrix = NULL;
                        dataset.element[curelem].tmatrix = NULL;
                        dataset.element[curelem].col_lut = Default_PROColor_LKUP;
                    }
                }
            }
            else if (Find(in_line, "ZZZZZ"))
            {
                free(gencomments);
                genclen = 0;
            }
            else
            {
                if (gencomments == NULL)
                {
                    gencomments = ARB_strdup(in_line);
                    genclen = strlen(gencomments)+1;
                }
                else
                {
                    genclen += strlen(in_line)+1;
                    ARB_realloc(gencomments, genclen);
                    strncat(gencomments, in_line, GBUFSIZ);
                    strncat(gencomments, "\n", GBUFSIZ);
                }
            }
        }
        free(buffer);
        fclose(file);
    }
    for (size_t j=0; j<dataset.numelements; j++) {
        dataset.maxlen = std::max(dataset.maxlen,
                                  dataset.element[j].seqlen+dataset.element[j].offset);
    }
    return error;
}

int WriteGen(NA_Alignment& aln, char *filename, int method)
{
    int i;
    size_t j;
    int k;
    NA_Sequence *this_elem;

    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        Warning("Cannot open file for output");
        return (1);
    }

    for (j=0; j<aln.numelements; j++)
    {
        this_elem = &(aln.element[j]);
        if (method == ALL)
        {
            fprintf(file,
                    "LOCUS       %10s%8d bp    %4s  %10s   %2d%5s%4d\n",
                    this_elem->short_name, this_elem->seqlen+this_elem->offset,
                    (this_elem->elementtype == DNA) ? "DNA" :
                    (this_elem->elementtype == RNA) ? "RNA" :
                    (this_elem->elementtype == MASK) ? "MASK" :
                    (this_elem->elementtype == PROTEIN) ? "PROT" : "TEXT",
                    this_elem->attr & IS_CIRCULAR ? "Circular" : "",
                    this_elem->t_stamp.origin.dd,
                    GDEmonth[this_elem->t_stamp.origin.mm-1],
                    (this_elem->t_stamp.origin.yy>1900) ? this_elem->t_stamp.origin.yy :
                    this_elem->t_stamp.origin.yy+1900);

            if (this_elem->description[0])
                fprintf(file, "DEFINITION  %s\n", this_elem->description);
            if (this_elem->seq_name[0])
                fprintf(file, "  ORGANISM  %s\n", this_elem->seq_name);
            if (this_elem->id[0])
                fprintf(file, " ACCESSION  %s\n", this_elem->id);
            if (this_elem->authority[0])
                fprintf(file, "   AUTHORS  %s\n", this_elem->authority);
            if (this_elem->comments)
                fprintf(file, "%s\n", this_elem->comments);
            fprintf(file, "ORIGIN");
            if (this_elem->tmatrix)
            {
                for (i=0, k=0; k<this_elem->seqlen+this_elem->offset; k++)
                {
                    if (i%60 == 0)
                        fprintf(file, "\n%9d", i+1);
                    if (i%10 == 0)
                        fprintf(file, " ");
                    fprintf(file, "%c", this_elem->tmatrix
                            [getelem(this_elem, k)]);
                    i++;
                }
            }
            else
            {
                for (i=0, k=0; k<this_elem->seqlen+this_elem->offset; k++)
                {
                    if (i%60 == 0)
                        fprintf(file, "\n%9d", i+1);
                    if (i%10 == 0)
                        fprintf(file, " ");
                    fprintf(file, "%c", getelem(this_elem, k));
                    i++;
                }
            }
            fprintf(file, "\n//\n");
        }
    }
    fclose(file);
    return (0);
}


void SetTime(void *b)
{
    ARB_TIME *a=(ARB_TIME*)b;
    struct tm *tim;
    time_t clock;

    clock = time(0);
    tim = localtime(&clock);

    a->yy = tim->tm_year;
    a->mm = tim->tm_mon+1;
    a->dd = tim->tm_mday;
    a->hr = tim->tm_hour;
    a->mn = tim->tm_min;
    a->sc = tim->tm_sec;
    return;
}

