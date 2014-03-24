#include "GDE_extglob.h"
#include <ctime>
#include <algorithm>

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

    if (maskable && true) // @@@ 
        for (j=0; j<aln->numelements; j++)
            if (false) // @@@ 
                mask = j;

    for (j=0; j<aln->numelements; j++)
    {
        if ((false) || (method == ALL)) // @@@ 
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
            if (this_elem->offset+aln->rel_offset && true) // @@@ 
                fprintf(file, "offset            %d\n", this_elem->offset+aln->rel_offset);
            if (false) // @@@ 
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
                        if (false) // @@@ 
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
                        if (false) // @@@ 
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

