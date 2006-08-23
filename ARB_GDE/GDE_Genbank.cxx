#include <sys/time.h>
#include <stdio.h>
// #include <malloc.h>
#include <string.h>
#include <time.h>

//#include <xview/xview.h>
//#include <xview/panel.h>

#include <arbdb.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>

#include "gde.hxx"
#include "GDE_menu.h"
#include "GDE_def.h"
#include "GDE_extglob.h"

/*
  Copyright (c) 1989-1990, University of Illinois board of trustees.  All
  rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
  Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
  Carl Woese.

  Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
  all rights reserved.

  Copyright (c) 1993, Steven Smith, all rights reserved.

*/

int CheckType(char *seq,int len);

void ReadGen(char *filename,NA_Alignment *dataset,int type)
{
    register int done = FALSE;
    size_t len = 0;
    size_t j = 0;
    int count,IS_REALLY_AA = FALSE;
    char in_line[GBUFSIZ],c;
    char *buffer=0,*gencomments = NULL,fields[8][GBUFSIZ];
    size_t buflen = 0;
    int genclen = 0,curelem = 0,n = 0;
    int start_col = -1;
    type=0;count=0;

    NA_Sequence *this_elem =0;
    FILE *file;

    ErrorOut5(0 != (file = fopen(filename,"r")),"No such file");

    for(;fgets(in_line,GBUFSIZ,file) != 0;)
    {
        if(in_line[strlen(in_line)-1] == '\n')
            in_line[strlen(in_line)-1] = '\0';
        if(Find(in_line,"LOCUS"))
        {
            curelem = dataset->numelements++;
            if(curelem == 0)
            {
                dataset->element=(NA_Sequence*)
                    Calloc(5,sizeof(NA_Sequence));
                dataset->maxnumelements = 5;
            }
            else if (curelem==dataset->maxnumelements)
            {
                (dataset->maxnumelements) *= 2;
                dataset->element =(NA_Sequence*)
                    Realloc((char*)dataset->element,
                            dataset->maxnumelements * sizeof(NA_Sequence));
            }
            this_elem = &(dataset->element[curelem]);
            n = sscanf(in_line,"%s %s %s %s %s %s %s %s",
                       fields[0],fields[1],fields[2],fields[3],fields[4],
                       fields[5],fields[6],fields[7]);
            if(IS_REALLY_AA)
            {
                InitNASeq(this_elem,PROTEIN);
            }
            else if(Find(in_line,"DNA"))
            {
                InitNASeq(this_elem,DNA);
            }
            else if(Find(in_line,"RNA"))
            {
                InitNASeq(this_elem,RNA);
            }
            else if(Find(in_line,"MASK"))
            {
                InitNASeq(this_elem,MASK);
            }
            else if(Find(in_line,"TEXT"))
            {
                InitNASeq(this_elem,TEXT);
            }
            else if(Find(in_line,"PROT"))
            {
                InitNASeq(this_elem,PROTEIN);
            }
            else
                InitNASeq(this_elem,DNA);

            strncpy(this_elem->short_name,fields[1],31);
            AsciiTime(&(this_elem->t_stamp.origin),fields[n-1]);
            this_elem->attr = DEFAULT_X_ATTR;

            if( Find(in_line, "Circular") )
                this_elem->attr |= IS_CIRCULAR;

            gencomments = NULL;
            genclen = 0;
        }
        else if(Find(in_line,"DEFINITION"))
            strncpy(this_elem->description,&(in_line[12]),79);

        else if(Find(in_line,"AUTHOR"))
            strncpy(this_elem->authority,&(in_line[12]),79);

        else if(Find(in_line,"  ORGANISM"))
            strncpy(this_elem->seq_name,&(in_line[12]),79);

        else if(Find(in_line,"ACCESSION"))
            strncpy(this_elem->id,&(in_line[12]),79);

        else if(Find(in_line,"ORIGIN"))
        {
            done = FALSE;
            len = 0;
            for(;done == FALSE && fgets(in_line,GBUFSIZ,file) != 0;)
            {
                if(in_line[0] != '/')
                {
                    if(buflen == 0)
                    {
                        buflen = GBUFSIZ;
                        buffer = Calloc(sizeof(char) ,
                                        buflen);
                    }

                    else if (len+strlen(in_line) >= buflen)
                    {
                        buflen += GBUFSIZ;
                        buffer = Realloc((char*)buffer,
                                         sizeof(char)*buflen);
                        for(j=buflen-GBUFSIZ
                                ;j<buflen;j++)
                            buffer[j] = '\0';
                    }
                    /*
                     *  Search for the fist column of data (whitespace-number-whitespace)data
                     */
                    if(start_col == -1)
                    {
                        for(start_col=0; in_line[start_col] == ' ' ||
                                in_line[start_col] == '\t';start_col++);

                        for(start_col++;strchr("1234567890",
                                               in_line[start_col]) != NULL;start_col++);

                        for(start_col++; in_line[start_col] == ' ' ||
                                in_line[start_col] == '\t';start_col++);

                    }
                    for(j=start_col;(c = in_line[j]) != '\0';j++)
                    {
                        if((c != '\n') &&
                           ((j-start_col + 1) % 11 !=0))
                            buffer[len++] = c;
                    }
                }
                else
                {
                    AppendNA((NA_Base*)buffer,len,&(dataset->
                                                    element[curelem]));
                    for(j=0;j<len;j++)
                        buffer[j] = '\0';
                    len = 0;
                    done = TRUE;
                    dataset->element[curelem].comments
                        = gencomments;
                    dataset->element[curelem].comments_len=
                        genclen - 1;
                    dataset->element[curelem].
                        comments_maxlen = genclen;

                    gencomments = NULL;
                    genclen = 0;
                }
            }
            /*
             *      Test if sequence should be converted by the translation table
             *      If it looks like a protein...
             */
            if(dataset->element[curelem].rmatrix &&
               IS_REALLY_AA == FALSE)
            {
                IS_REALLY_AA = CheckType((char*)dataset->element[curelem].
                                         sequence,dataset->element[curelem].seqlen);

                if(IS_REALLY_AA == FALSE)
                    Ascii2NA((char*)dataset->element[curelem].sequence,
                             dataset->element[curelem].seqlen,
                             dataset->element[curelem].rmatrix);
                else
                    /*
                     *      Force the sequence to be AA
                     */
                {
                    dataset->element[curelem].elementtype = PROTEIN;
                    dataset->element[curelem].rmatrix = NULL;
                    dataset->element[curelem].tmatrix = NULL;
                    dataset->element[curelem].col_lut =
                        Default_PROColor_LKUP;
                }
            }
        }
        else if (Find(in_line,"ZZZZZ"))
        {
            Cfree(gencomments);
            genclen = 0;
        }
        else
        {
            if (gencomments == NULL)
            {
                gencomments = String(in_line);
                genclen = strlen(gencomments)+1;
            }
            else
            {
                genclen += strlen(in_line)+1;
                gencomments = Realloc((char*)gencomments,genclen *
                                      sizeof(char));
                strncat(gencomments,in_line,GBUFSIZ);
                strncat(gencomments,"\n",GBUFSIZ);
            }
        }
    }
    Cfree(buffer);
    fclose(file);
    for(j=0;j<dataset->numelements;j++)
        dataset->maxlen = MAX(dataset->maxlen,
                              dataset->element[j].seqlen+dataset->element[j].offset);
    //return;
}

// ARB
typedef struct ARB_TIME_STRUCT
{
    int yy;
    int mm;
    int dd;
    int hr;
    int mn;
    int sc;
} ARB_TIME;

void AsciiTime(void *b,char *asciitime)
{
    ARB_TIME *a=(ARB_TIME*)b;
    int j;
    char temp[GBUFSIZ];
    //extern char month[12][6];

    a->dd = 0;
    a->yy = 0;
    a->mm = 0;
    sscanf(asciitime,"%d%5c%d",&(a->dd),temp,&(a->yy));
    temp[5] = '\0';
    for(j=0;j<12;j++)
        if(strcmp(temp,GDEmonth[j]) == 0)
            a->mm = j+1;
    if(a->dd <0 || a->dd > 31 || a->yy < 0 || a->mm > 11)
        SetTime(a);
    return;
}
// ENDARB

int WriteGen(NA_Alignment *aln,char *filename,int method,int maskable)
{
    int i;
    size_t j;
    int k,mask = -1;
    FILE *file;
    //extern char month[12][6];
    NA_Sequence *this_elem;
    char c;
    if(aln == NULL)
        return(1);
    // ARB
    //if(aln->na_ddata == NULL)
    //  return(1);

    file = fopen(filename,"w");
    if(file == NULL)
    {
        Warning("Cannot open file for output");
        return(1);
    }

    if(maskable && method != SELECT_REGION)
        for(j=0;j<aln->numelements;j++)
            if(aln->element[j].elementtype == MASK &&
               aln->element[j].selected)
                mask = j;

    for(j=0;j<aln->numelements;j++)
    {
        this_elem = &(aln->element[j]);
        if ((aln->element[j].selected && (int)j!=mask && method != SELECT_REGION)
            ||(aln->element[j].subselected && method == SELECT_REGION)
            || (method == ALL))
        {
            fprintf(file,
                    "LOCUS       %10s%8d bp    %4s  %10s   %2d%5s%4d\n",
                    this_elem->short_name,this_elem->seqlen+this_elem->offset,
                    (this_elem->elementtype == DNA) ? "DNA":
                    (this_elem->elementtype ==RNA)?"RNA":
                    (this_elem->elementtype == MASK)?"MASK":
                    (this_elem->elementtype == PROTEIN)?"PROT":"TEXT",
                    this_elem->attr & IS_CIRCULAR?"Circular":"",
                    this_elem->t_stamp.origin.dd,
                    GDEmonth[this_elem->t_stamp.origin.mm-1],
                    (this_elem->t_stamp.origin.yy>1900)?this_elem->t_stamp.origin.yy:
                    this_elem->t_stamp.origin.yy+1900);

            if(this_elem->description[0])
                fprintf(file,"DEFINITION  %s\n",this_elem->description);
            if(this_elem->seq_name[0])
                fprintf(file,"  ORGANISM  %s\n",this_elem->seq_name);
            if(this_elem->id[0])
                fprintf(file," ACCESSION  %s\n",this_elem->id);
            if(this_elem->authority[0])
                fprintf(file,"   AUTHORS  %s\n",this_elem->authority);
            if(this_elem->comments)
                fprintf(file,"%s\n",this_elem->comments);
            fprintf(file,"ORIGIN");
            if(this_elem->tmatrix)
            {
                if(mask == -1)
                {
                    for(i=0,k=0;k<this_elem->seqlen+this_elem->offset;k++)
                    {
                        if(method == SELECT_REGION)
                        {
                            if(aln->selection_mask[k] == '1')
                            {
                                if(i%60 == 0)
                                    fprintf(file,"\n%9d",i+1);
                                if(i%10 == 0)
                                    fprintf(file," ");
                                fprintf(file,"%c",this_elem->tmatrix
                                        [getelem(this_elem,k)]);
                                i++;
                            }
                        }
                        else
                        {
                            if(i%60 == 0)
                                fprintf(file,"\n%9d",i+1);
                            if(i%10 == 0)
                                fprintf(file," ");
                            fprintf(file,"%c",this_elem->tmatrix
                                    [getelem(this_elem,k)]);
                            i++;
                        }
                    }
                }
                else
                {
                    for(k=0;k<this_elem->seqlen+this_elem->offset;k++)
                    {
                        c =(char)getelem(&(aln->element[mask]),k);
                        if(c != '0' && c!= '-')
                        {
                            if(k%60 == 0)
                                fprintf(file,"\n%9d",k+1);
                            if(k%10 == 0)
                                fprintf(file," ");
                            fprintf(file,"%c",this_elem->tmatrix
                                    [getelem(this_elem,k)]);
                        }
                    }
                }
            }
            else
            {
                if(mask == -1)
                {
                    for(i=0,k=0;k<this_elem->seqlen+this_elem->offset;k++)
                    {
                        if(method == SELECT_REGION)
                        {
                            if(aln->selection_mask[k] == '1')
                            {
                                if(i%60 == 0)
                                    fprintf(file,"\n%9d",i+1);
                                if(i%10 == 0)
                                    fprintf(file," ");
                                fprintf(file,"%c", getelem(this_elem,k));
                                i++;
                            }
                        }
                        else
                        {
                            if(i%60 == 0)
                                fprintf(file,"\n%9d",i+1);
                            if(i%10 == 0)
                                fprintf(file," ");
                            fprintf(file,"%c",getelem(this_elem,k));
                            i++;
                        }
                    }
                }
                else
                {
                    for(k=0;k<this_elem->seqlen+this_elem->offset;k++)
                    {
                        c =(char)getelem(&(aln->element[mask]),k);
                        if(c != '0' && c!= '-')
                        {
                            if(k%60 == 0)
                                fprintf(file,"\n%9d",k+1);
                            if(k%10 == 0)
                                fprintf(file," ");
                            fprintf(file,"%c",getelem(this_elem,k));
                        }
                    }
                }
            }
            fprintf(file,"\n//\n");
        }
    }
    fclose(file);
    return(0);
}


void SetTime(void *b)
{
    ARB_TIME *a=(ARB_TIME*)b;
    struct tm *tim; //*localtime();
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

/*
 *   CheckType:  Check base composition to see if the sequence
 *   appears to be an amino acid sequence.  If it is, pass back
 *   TRUE, else FALSE.
 */
int CheckType(char *seq,int len)
{

    int j,count1 = 0,count2 = 0;

    for(j=0;j<len;j++)
        if(((seq[j]|32) < 'z') && ((seq[j]|32) > 'a'))
        {
            count1++;
            if(strchr("ACGTUNacgtun",seq[j]) == NULL)
                count2++;
        }

    return( (count2 > count1/4)?TRUE:FALSE);
}
