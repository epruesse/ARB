#include "GDE_extglob.h"
#include <ctime>
#include <algorithm>

/*
  Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
  All rights reserved.
*/

static void AdjustGroups(NA_Alignment *aln)
{
    size_t i;
    size_t j;
    int c, done=FALSE;

#ifdef HGL
    return;
#else
    for (c=0; c<200 && !done; c++)
    {
        for (j=1; j<=aln->numgroups; j++)
        {
            done = FALSE;
            for (i=0; i<aln->numelements; i++)
            {
                if (aln->element[i].groupid == j)
                {
                    if (aln->element[i].groupf!=NULL ||
                       aln->element[i].groupb!=NULL)
                        done = TRUE;
                    else
                        aln->element[i].groupid = 0;
                }
            }
            if (done == FALSE)
            {
                for (i=0; i<aln->numelements; i++)
                    if (aln->element[i].groupid == aln->numgroups)
                        aln->element[i].groupid = j;
                aln->numgroups--;
            }
        }
        if (aln->numgroups == 0)
            done = TRUE;
    }
    return;
#endif
}

static void StripSpecial(char *string) {
    int j, len;

    len = strlen(string);
    for (j=0; j<len; j++)
    {
        if (string[j] == '"')
            string[j] = '`';
        else if (string[j] == '{')
            string[j] = '(';
        else if (string[j] == '}')
            string[j] = ')';
    }
}


static void RemoveQuotes(char *string)
{
    int i, j, len;

    len = strlen(string);
    for (j=0; j<len; j++) {
        if (string[j] == '"') string[j] = ' ';
    }

    for (j=0; string[j]==' ' && j<(int)strlen(string); j++) ;

    len = strlen(string);
    for (i=0; i<len - j; i++) string[i] = string[i+j];

    for (j=strlen(string)-1; j>=0 && (string[j]=='\n'||string[j]==' '); j--) {
        string[j] = '\0';
    }

}

static int OverWrite(NA_Sequence *thiss, NA_Alignment *aln) {
    /* OverWrite(), overwrite all non-default data from a sequence entry
     * onto any entry with the same ID or short name.
     */
    size_t j;
    int indx = -1;
    NA_Sequence *that;
    for (j=0; j<aln->numelements; j++)
    {
        if (Find2(thiss->id, aln->element[j].id) != -1)
            if (Find2(aln->element[j].id, thiss->id) != -1)
                indx = j;
    }
    if (indx == -1)
        for (j=0; j<aln->numelements; j++)
        {
            if (Find2(thiss->short_name, aln->element[j].short_name) != -1)
                if (Find2(aln->element[j].short_name, thiss->short_name) != -1)
                    indx = j;
        }
    if (indx != -1)
    {
        that = &(aln->element[indx]);
        if (thiss->seq_name[0])
            strcpy(that->seq_name, thiss->seq_name);
        if (thiss->barcode[0])
            strcpy(that->barcode, thiss->barcode);
        if (thiss->contig[0])
            strcpy(that->contig, thiss->contig);
        if (thiss->membrane[0])
            strcpy(that->membrane, thiss->membrane);
        if (thiss->authority[0])
            strcpy(that->authority, thiss->authority);
        if (thiss->short_name[0])
            strcpy(that->short_name, thiss->short_name);
        if (thiss->description[0])
            strcpy(that->description, thiss->description);
        if (thiss->sequence)
        {
            free(that->sequence);
            that->sequence = thiss->sequence;
            that->seqlen = thiss->seqlen;
            that->seqmaxlen = thiss->seqmaxlen;
        }
        if (thiss->baggage)
        {
            that->baggage_len += thiss->baggage_len;
            that->baggage_maxlen += thiss->baggage_maxlen;
            if (that->baggage)
                that->baggage =
                    Realloc(that->baggage, that->baggage_maxlen*sizeof(char));
            else
                that->baggage = Calloc(that->baggage_maxlen, sizeof(char));
            strncat(that->baggage, thiss->baggage, that->baggage_maxlen);
        }
        if (thiss->comments)
        {
            that->comments_len += thiss->comments_len;
            that->comments_maxlen += thiss->comments_maxlen;
            if (that->comments)
                that->comments =
                    Realloc(that->comments, that->comments_maxlen*sizeof(char));
            else
                that->comments = Calloc(that->comments_maxlen, sizeof(char));
            strncat(that->comments, thiss->comments, that->comments_maxlen);
        }
        if (thiss->cmask)
        {
            free(that->cmask);
            that->cmask = thiss->cmask;
        }
        if (thiss->offset != that->offset)
            that->offset = thiss->offset;
        if (thiss->attr != 0)
            that->attr = thiss->attr;
        if (thiss->groupid != 0)
        {
            that->groupid = thiss->groupid;
        }
        that->groupb = NULL;
        that->groupf = NULL;
    }

    return (indx);
}

void ReadGDE(char *filename, NA_Alignment *dataset) {
    int          done               = FALSE;
    size_t       len                = 0, j=0;
    int          success, temp = 0;
    char         in_line[GBUFSIZ];
    char        *buffer, *line;
    size_t       buflen             = GBUFSIZ;
    int          curelem = 0;
    NA_Sequence *this_elem          = NULL, temp_elem;
    FILE        *file;

    ErrorOut5(0!=(file = fopen(filename, "r")), "No such file");

    for (; fgets(in_line, GBUFSIZ, file) != 0;)
    {
        for (line = in_line; line[0]==' ' || line[0] == '\t'; line++) ;

        if (Find2(line, "{")==0)
        {
            this_elem = &temp_elem;
            InitNASeq(this_elem, DNA);
            this_elem->offset = -(dataset->rel_offset);
        }
        else if (Find2(line, "type")==0)
        {
            if (Find(line, "DNA"))
            {
                this_elem->elementtype = DNA;
                this_elem->tmatrix = Default_DNA_Trans;
                this_elem->rmatrix = Default_NA_RTrans;
            }
            else if (Find(line, "RNA"))
            {
                this_elem->elementtype = RNA;
                this_elem->tmatrix = Default_RNA_Trans;
                this_elem->rmatrix = Default_NA_RTrans;
            }
            else if (Find(line, "MASK"))
            {
                this_elem->elementtype = MASK;
                this_elem->rmatrix = NULL;
                this_elem->tmatrix = NULL;
                this_elem->col_lut = NULL;
            }
            else if (Find(line, "TEXT"))
            {
                this_elem->elementtype = TEXT;
                this_elem->rmatrix = NULL;
                this_elem->tmatrix = NULL;
                this_elem->col_lut = NULL;
            }
            else if (Find(line, "PROT"))
            {
                this_elem->elementtype = PROTEIN;
                this_elem->rmatrix = NULL;
                this_elem->tmatrix = NULL;
                this_elem->col_lut = Default_PROColor_LKUP;
            }
            /*
              this_elem->attr = DEFAULT_X_ATTR;
            */
        }
        else if (Find2(line, "circular")==0)
        {
            sscanf(line, "%*s %d", &temp);
            if (temp == 1)
            {
                this_elem->attr |= IS_CIRCULAR;
            }
            else
            {
                this_elem->attr &= ~IS_CIRCULAR;
            }
        }
        else if (Find2(line, "orig_direction")==0)
        {
            sscanf(line, "%*s %d", &temp);
            if (temp == 1)
            {
                this_elem->attr |= IS_ORIG_5_TO_3;
                this_elem->attr &= ~IS_ORIG_3_TO_5;
            }
            else
            {
                this_elem->attr |= IS_ORIG_3_TO_5;
                this_elem->attr &= ~IS_ORIG_5_TO_3;
            }
        }
        else if (Find2(line, "direction")==0)
        {
            sscanf(line, "%*s %d", &temp);
            if (temp == 1)
            {
                this_elem->attr |= IS_5_TO_3;
                this_elem->attr &= ~IS_3_TO_5;
            }
            else
            {
                this_elem->attr |= IS_3_TO_5;
                this_elem->attr &= ~IS_5_TO_3;
            }
        }
        else if (Find2(line, "orig_strand")==0)
        {
            sscanf(line, "%*s %d", &temp);
            if (temp == 1)
            {
                this_elem->attr |= IS_ORIG_PRIMARY;
                this_elem->attr &= ~IS_ORIG_SECONDARY;
            }
            else
            {
                this_elem->attr |= IS_ORIG_SECONDARY;
                this_elem->attr &= ~IS_ORIG_PRIMARY;
            }
        }
        else if (Find2(line, "strandedness")==0)
        {
            sscanf(line, "%*s %d", &temp);
            if (temp == 1)
            {
                this_elem->attr |= IS_PRIMARY;
                this_elem->attr &= ~IS_SECONDARY;
            }
            else
            {
                this_elem->attr |= IS_SECONDARY;
                this_elem->attr &= ~IS_PRIMARY;
            }
        }
        else if (Find2(line, "creator")==0)
        {
            sscanf(line, "%*s %[^\n]", this_elem->authority);
            RemoveQuotes(this_elem->authority);
        }
        else if (Find2(line, "longname")==0)
        {
            sscanf(line, "%*s %[^\n]", this_elem->seq_name);
            RemoveQuotes(this_elem->seq_name);
        }
        else if (Find2(line, "descrip")==0)
        {
            sscanf(line, "%*s %[^\n]", this_elem->description);
            RemoveQuotes(this_elem->description);
        }
        else if (Find2(line, "name")==0)
        {
            sscanf(line, "%*s %[^\n]", this_elem->short_name);
            RemoveQuotes(this_elem->short_name);
        }
        else if (Find2(line, "group-ID")==0)
        {
            sscanf(line, "%*s %zu", &(this_elem->groupid));
            dataset->numgroups = std::max(this_elem->groupid, dataset->numgroups);
        }
        else if (Find2(line, "sequence-ID")==0)
        {
            sscanf(line, "%*s %[^\n]", this_elem->id);
            RemoveQuotes(this_elem->id);
        }
        else if (Find2(line, "barcode")==0)
        {
            sscanf(line, "%*s %[^\n]", this_elem->barcode);
            RemoveQuotes(this_elem->barcode);
        }
        else if (Find2(line, "membrane")==0)
        {
            sscanf(line, "%*s %[^\n]", this_elem->membrane);
            RemoveQuotes(this_elem->membrane);
        }
        else if (Find2(line, "contig")==0)
        {
            sscanf(line, "%*s %[^\n]", this_elem->contig);
            RemoveQuotes(this_elem->contig);
        }
        else if (Find2(line, "creation-date")==0)
        {
            sscanf(line, "%*s %2d%*c%2d%*c%2d%*c%2d%*c%2d%*c%2d\n",
                   &(this_elem->t_stamp.origin.mm),
                   &(this_elem->t_stamp.origin.dd),
                   &(this_elem->t_stamp.origin.yy),
                   &(this_elem->t_stamp.origin.hr),
                   &(this_elem->t_stamp.origin.mn),
                   &(this_elem->t_stamp.origin.sc));
        }
        else if (Find2(line, "offset")==0)
        {
            sscanf(line, "%*s %d", &(this_elem->offset));
            this_elem->offset -= dataset->rel_offset;
        }
        else if (Find2(line, "comments")==0)
        {
            if (this_elem->comments_maxlen == 0)
                buflen = 2048;
            else
                buflen = this_elem->comments_maxlen;

            done = FALSE;
            len = this_elem->comments_len;

            for (; line[0] != '"'; line++)
                if (line[0] == '\0')
                    ErrorOut5(0, "Error in input file");
            line++;
            buffer = Calloc(buflen, sizeof(char));
            for (; !done;)
            {
                for (j=0; j<strlen(line); j++)
                {
                    if (len+strlen(line) >= buflen)
                    {
                        buflen *= 2;
                        buffer = Realloc(buffer,
                                         buflen*sizeof(char));
                    }
                    if (line[j] == '"') done = TRUE;

                    else
                        buffer[len++] = line[j];
                }
                // Check pad with null
                buffer[len] = '\0';
                if (!done)
                {
                    if (fgets(in_line, GBUFSIZ, file) == 0)
                        done = TRUE;
                    line = in_line;
                }
            }
            this_elem->comments = buffer;
            this_elem->comments_len = strlen(buffer);
            this_elem->comments_maxlen = buflen;
            RemoveQuotes(this_elem->comments);
        }
        else if (Find2(line, "sequence")==0)
        {
            buflen = GBUFSIZ;
            done = FALSE;
            len = 0;

            buffer = Calloc(buflen, sizeof(char));
            for (; line[0] != '"'; line++)
                if (line[0] == '\0')
                    ErrorOut5(0, "Error in input file");

            line++;
            for (; !done;)
            {
                for (j=0; j<strlen(line); j++)
                {
                    if (len+strlen(line) >= buflen)
                    {
                        buflen *= 2;
                        buffer = Realloc(buffer,
                                         buflen*sizeof(char));
                    }
                    if (line[j] == '"') done = TRUE;

                    else
                    {
                        // If not text, ignore spaces...
                        if (this_elem->elementtype != TEXT)
                        {
                            if (line[j]!=' ' && line[j] !=
                               '\t' && line[j] != '\n')
                                buffer[len++] = line[j];
                        }
                        else
                            if (line[j] != '\t' && line[j] != '\n')
                                buffer[len++] = line[j];
                    }
                }
                if (!done)
                {
                    if (fgets(in_line, GBUFSIZ, file) == 0)
                        done = TRUE;
                    line = in_line;
                }
            }
            if (this_elem->rmatrix) {
                for (j=0; j<len; j++) {
                    buffer[j]=this_elem->rmatrix[(unsigned char)buffer[j]];
                }
            }
            this_elem->sequence = (NA_Base*)buffer;
            this_elem->seqlen = len;
            this_elem->seqmaxlen = buflen;
        }

        else if (Find2(line, "}")==0)
        {
            if (this_elem->id[0] == '\0')
                strncpy_terminate(this_elem->id, uniqueID(), SIZE_ID);
            if (this_elem->short_name[0] == '\0')
                strncpy_terminate(this_elem->short_name, this_elem->id, SIZE_SHORT_NAME);
            if (this_elem->seqlen == 0)
                this_elem->protect =
                    PROT_BASE_CHANGES+
                    PROT_GREY_SPACE+
                    PROT_WHITE_SPACE+
                    PROT_TRANSLATION;

            // Make a new sequence entry...
            success     = -1;
            if (OVERWRITE)
                success = OverWrite(this_elem, dataset);

            if (success == -1)
            {
                curelem = dataset->numelements++;
                if (curelem == 0)
                {
                    dataset->element=(NA_Sequence*)
                        Calloc(5, sizeof(NA_Sequence));
                    dataset->maxnumelements = 5;
                }
                else if (curelem==dataset->maxnumelements)
                {
                    (dataset->maxnumelements) *= 2;
                    dataset->element = (NA_Sequence*)
                        Realloc((char*)dataset->element,
                                dataset->maxnumelements * sizeof(NA_Sequence));
                }
                dataset->element[curelem] = *this_elem;
            }
        }
        else if (this_elem != NULL)
        {
            if (this_elem->baggage == NULL)
            {
                this_elem->baggage = strdup(line);
                this_elem->baggage_maxlen =
                    this_elem->baggage_len =
                    strlen(this_elem->baggage)+1;
            }
            else
            {
                this_elem->baggage_len += strlen(line)+1;
                this_elem->baggage = Realloc(
                                             this_elem->baggage, this_elem->baggage_len *
                                             sizeof(char));
                this_elem->baggage_maxlen =
                    this_elem->baggage_len;

                strncat(this_elem->baggage, line, GBUFSIZ);
            }
        }
    }

    fclose(file);
    NormalizeOffset(dataset);
    Regroup(dataset);
    AdjustGroups(dataset);
    return;
}


int WriteGDE(NA_Alignment *aln, char *filename, int method, int maskable)
{
    int i;
    size_t j;
    int k, mask = -1;
    FILE *file;
    NA_Sequence *this_elem;

    if (aln == NULL) return (1);

    file = fopen(filename, "w");
    if (file == NULL)
    {
        Warning("Cannot open file for output");
        return (1);
    }

    if (maskable && method != SELECT_REGION)
        for (j=0; j<aln->numelements; j++)
            if (aln->element[j].elementtype == MASK &&
               aln->element[j].selected)
                mask = j;

    for (j=0; j<aln->numelements; j++)
    {
        if ((aln->element[j].selected && (int)j!=mask && method!=SELECT_REGION)
           || (method == ALL)
           || (aln->element[j].subselected && method == SELECT_REGION))
        {
            this_elem = &(aln->element[j]);
            fprintf(file, "{\n");
            if (this_elem->short_name[0])
                fprintf(file, "name      \"%s\"\n", this_elem->short_name);
            switch (this_elem->elementtype)
            {
                case DNA:
                    fprintf(file, "type              \"DNA\"\n");
                    break;
                case RNA:
                    fprintf(file, "type              \"RNA\"\n");
                    break;
                case PROTEIN:
                    fprintf(file, "type              \"PROTEIN\"\n");
                    break;
                case MASK:
                    fprintf(file, "type              \"MASK\"\n");
                    break;
                case TEXT:
                    fprintf(file, "type              \"TEXT\"\n");
                    break;
            }
            if (this_elem->seq_name[0])
                fprintf(file, "longname  %s\n", this_elem->seq_name);

            if (this_elem->id[0])
                fprintf(file, "sequence-ID       \"%s\"\n", this_elem->id);
            RemoveQuotes(this_elem->barcode);
            RemoveQuotes(this_elem->contig);

            if (this_elem->barcode[0])
                fprintf(file, "barcode           \"%s\"\n", this_elem->barcode);
            if (this_elem->membrane[0])
                fprintf(file, "membrane          \"%s\"\n", this_elem->membrane);
            if (this_elem->contig[0])
                fprintf(file, "contig            \"%s\"\n", this_elem->contig);
            if (this_elem->description[0])
                fprintf(file, "descrip           \"%s\"\n", this_elem->description);
            if (this_elem->authority[0])
                fprintf(file, "creator           \"%s\"\n", this_elem->authority);
            if (this_elem->groupid)
                fprintf(file, "group-ID          %zu\n", this_elem->groupid);
            if (this_elem->offset+aln->rel_offset && method!=SELECT_REGION)
                fprintf(file, "offset            %d\n", this_elem->offset+aln->rel_offset);
            if (method == SELECT_REGION)
            {
                /* If selecting a region, the offset should be moved to the first
                 * non-'0' space in the mask.
                 */
                for (k=this_elem->offset; k<aln->selection_mask_len && aln->selection_mask[k] == '0'; k++) ;
                fprintf(file, "offset        %d\n", aln->rel_offset+k);
            }
            if (this_elem->t_stamp.origin.mm != 0)
                fprintf(file,
                        "creation-date      %2d/%2d/%2d %2d:%2d:%2d\n",
                        this_elem->t_stamp.origin.mm,
                        this_elem->t_stamp.origin.dd,
                        (this_elem->t_stamp.origin.yy)>1900 ?
                        (this_elem->t_stamp.origin.yy-1900) :
                        (this_elem->t_stamp.origin.yy),
                        this_elem->t_stamp.origin.hr,
                        this_elem->t_stamp.origin.mn,
                        this_elem->t_stamp.origin.sc);
            if ((this_elem->attr & IS_ORIG_5_TO_3) &&
               ((this_elem->attr & IS_ORIG_3_TO_5) == 0))
                fprintf(file, "orig_direction    1\n");

            if ((this_elem->attr & IS_CIRCULAR))
                fprintf(file, "circular  1\n");

            if ((this_elem->attr & IS_5_TO_3) &&
               ((this_elem->attr & IS_3_TO_5) == 0))
                fprintf(file, "direction 1\n");

            if ((this_elem->attr & IS_ORIG_3_TO_5) &&
               ((this_elem->attr & IS_ORIG_5_TO_3) == 0))
                fprintf(file, "orig_direction    -1\n");

            if ((this_elem->attr & IS_3_TO_5) &&
               ((this_elem->attr & IS_5_TO_3) == 0))
                fprintf(file, "direction -1\n");

            if ((this_elem->attr & IS_ORIG_PRIMARY) &&
               ((this_elem->attr & IS_ORIG_SECONDARY) == 0))
                fprintf(file, "orig_strand       1\n");

            if ((this_elem->attr & IS_PRIMARY) &&
               ((this_elem->attr & IS_SECONDARY) == 0))
                fprintf(file, "strandedness      1\n");

            if (((this_elem->attr & IS_ORIG_PRIMARY) == 0) &&
               (this_elem->attr & IS_ORIG_SECONDARY))
                fprintf(file, "orig_strand       2\n");

            if (((this_elem->attr & IS_PRIMARY) == 0) &&
               (this_elem->attr & IS_SECONDARY))
                fprintf(file, "strandedness      2\n");

            if (this_elem->comments != NULL)
            {
                StripSpecial(this_elem->comments);
                fprintf(file, "comments  \"%s\"\n", this_elem->comments);
            }
            if (this_elem->baggage != NULL)
            {
                if (this_elem->
                   baggage[strlen(this_elem->baggage)-1] == '\n')
                    fprintf(file, "%s", this_elem->baggage);
                else
                    fprintf(file, "%s\n", this_elem->baggage);
            }
            fprintf(file, "sequence  \"");
            if (this_elem->tmatrix)
            {
                if (mask == -1)
                {
                    for (k=this_elem->offset; k<this_elem->seqlen+this_elem->offset; k++)
                    {
                        if (k%60 == 0)
                            putc('\n', file);
                        if (method == SELECT_REGION)
                        {
                            if (aln->selection_mask[k] == '1')
                                putc(this_elem->tmatrix[getelem(this_elem, k)],
                                     file);
                        }
                        else
                            putc(this_elem->tmatrix[getelem(this_elem, k)],
                                 file);
                    }
                }
                else
                {
                    for (i=0, k=this_elem->offset; k<this_elem->seqlen+this_elem->offset; k++)
                        if (aln->element[mask].seqlen+this_elem->offset>k)
                            if ((char)getelem(&(aln->element[mask]), k) != '0'
                               && ((char)getelem(&(aln->element[mask]), k) != '-'))
                            {
                                if (i%60 == 0)
                                    putc('\n', file);
                                putc(this_elem->tmatrix[getelem(this_elem, k)],
                                     file);
                                i++;
                            }
                }
                fprintf(file, "\"\n");
            }
            else
            {
                if (mask == -1)
                {
                    for (k=this_elem->offset; k<this_elem->seqlen+this_elem->offset; k++)
                    {
                        if (k%60 == 0)
                            putc('\n', file);
                        if (method == SELECT_REGION)
                        {
                            if (aln->selection_mask[k] == '1')
                                putc(getelem(this_elem, k), file);
                        }
                        else
                            putc(getelem(this_elem, k), file);
                    }
                }
                else
                {
                    for (i=0, k=this_elem->offset; k<this_elem->seqlen+this_elem->offset; k++)
                        if (((aln->element[mask].seqlen)+(aln->element[mask].
                                                         offset)) > k)
                            if ((char)getelem(&(aln->element[mask]), k) == '1')
                            {
                                if (i%60 == 0)
                                    putc('\n', file);
                                putc(getelem(this_elem, k), file);
                                i++;
                            }
                }
                fprintf(file, "\"\n");
            }
            fprintf(file, "}\n");
        }
    }
    fclose(file);
    return (0);
}



void SeqNorm(NA_Sequence *seq)
{
    // Normalize seq (remove leading indels in the sequence;
    int len, j, shift_width, trailer;
    char *sequence;
    len = seq->seqlen;

    sequence = (char*)seq->sequence;

    if (len == 0) return;

    if (seq->tmatrix) {
        for (shift_width=0; (shift_width<len) && ((sequence[shift_width]&15) == '\0'); shift_width++) ;
    }
    else {
        for (shift_width=0; (shift_width<len) && (sequence[shift_width] == '-'); shift_width++) ;
    }

    for (j=0; j<len-shift_width; j++) {
        sequence[j] = sequence[j+shift_width];
    }

    seq->seqlen -= shift_width;
    seq->offset += shift_width;
    for (trailer=seq->seqlen-1;
         (sequence[trailer] == '-' || sequence[trailer] == '\0') && trailer>=0;
         trailer--)
    {
        sequence[trailer] = '\0';
    }
    seq->seqlen = trailer+1;
    return;
}

/* ALWAYS COPY the result from uniqueID() to a char[32],
 * (strlen(hostname)+1+10).  Memory is lost when the function
 * is finished.
 */

char *uniqueID()
{
    static char vname[32];
    time_t *tp;
    static int cnt = 0;

    tp = (time_t *)Calloc(1, sizeof(time_t));

    time(tp);
    sprintf(vname, "host:%d:%ld", cnt, *tp);
    cnt++;
    Cfree((char*)tp);
    return (vname);
}

