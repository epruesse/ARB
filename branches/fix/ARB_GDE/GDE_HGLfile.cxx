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

int WriteGDE(NA_Alignment *aln, char *filename, int method) {
    if (aln == NULL) return (1);

    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        Warning("Cannot open file for output");
        return (1);
    }

    for (size_t j=0; j<aln->numelements; j++)
    {
        if (method == ALL)
        {
            NA_Sequence *this_elem = &(aln->element[j]);
            // @@@ code below should be in method of NA_Sequence (far too many 'this_elem->')
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
            if (this_elem->offset+aln->rel_offset)
                fprintf(file, "offset            %d\n", this_elem->offset+aln->rel_offset);

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
                for (int k=this_elem->offset; k<this_elem->seqlen+this_elem->offset; k++)
                {
                    if (k%60 == 0) putc('\n', file);
                    putc(this_elem->tmatrix[getelem(this_elem, k)], file);
                }
                fprintf(file, "\"\n");
            }
            else
            {
                for (int k=this_elem->offset; k<this_elem->seqlen+this_elem->offset; k++)
                {
                    if (k%60 == 0)
                        putc('\n', file);
                    putc(getelem(this_elem, k), file);
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

