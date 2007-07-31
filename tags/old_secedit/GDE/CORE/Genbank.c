#include <sys/time.h>
#include <stdio.h>
/* #include <malloc.h> */
#include <time.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/panel.h>
#include "menudefs.h"
#include "defines.h"

/*
Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
all rights reserved.

Copyright (c) 1993, Steven Smith, all rights reserved.

*/
struct gde_time_struct
{
	int yy;
	int mm;
	int dd;
	int hr;
	int mn;
	int sc;
};


static void AsciiTime(a,asciitime)
	struct gde_time_struct *a;
	char *asciitime;
{
	int j;
	char temp[GBUFSIZ];
	extern char month[12][6];

	a->dd = 0;
	a->yy = 0;
	a->mm = 0;
	sscanf(asciitime,"%d%5c%d",&(a->dd),temp,&(a->yy));
	temp[5] = '\0';
	for(j=0;j<12;j++)
		if(strcmp(temp,month[j]) == 0)
			a->mm = j+1;
	if(a->dd <0 || a->dd > 31 || a->yy < 0 || a->mm > 11)
		SetTime(a);
	return;
}

ReadGen(filename,dataset,type)
char *filename;
NA_Alignment *dataset;
int type;
{
	register int done = FALSE,len = 0, j=0;
	int count,IS_REALLY_AA = FALSE;
	char Inline[GBUFSIZ],c;
	char *buffer,*gencomments = NULL,fields[8][GBUFSIZ];
	int buflen = 0,genclen = 0,curelem = 0,n = 0,flag = 0;
	int start_col = -1;

	NA_Sequence *this_elem;
	FILE *file;
	extern int Default_DNA_Trans[], Default_RNA_Trans[];
	extern int Default_NA_RTrans[];
	extern int Default_PROColor_LKUP[],Default_NAColor_LKUP[];

	ErrorOut1("No such file",file = fopen(filename,"r"));

	for(;fgets(Inline,GBUFSIZ,file) != 0;)
	{
		if(Inline[strlen(Inline)-1] == '\n')
			Inline[strlen(Inline)-1] = '\0';
		if(Find(Inline,"LOCUS"))
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
				    Realloc(dataset->element,
				    dataset->maxnumelements * sizeof(NA_Sequence));
			}
			this_elem = &(dataset->element[curelem]);
			n = sscanf(Inline,"%s %s %s %s %s %s %s %s",
			    fields[0],fields[1],fields[2],fields[3],fields[4],
			    fields[5],fields[6],fields[7]);
			if(IS_REALLY_AA)
			{
				InitNASeq(this_elem,PROTEIN);
			}
			else if(Find(Inline,"DNA"))
			{
				InitNASeq(this_elem,DNA);
			}
			else if(Find(Inline,"RNA"))
			{
				InitNASeq(this_elem,RNA);
			}
			else if(Find(Inline,"MASK"))
			{
				InitNASeq(this_elem,MASK);
			}
			else if(Find(Inline,"TEXT"))
			{
				InitNASeq(this_elem,TEXT);
			}
			else if(Find(Inline,"PROT"))
			{
				InitNASeq(this_elem,PROTEIN);
			}
			else
				InitNASeq(this_elem,DNA);

			strncpy(this_elem->short_name,fields[1],31);
			AsciiTime(&(this_elem->t_stamp.origin),fields[n-1]);
			this_elem->attr = DEFAULT_X_ATTR;

			if( Find(Inline, "Circular") )
				this_elem->attr |= IS_CIRCULAR;

			gencomments = NULL;
			genclen = 0;
		}
		else if(Find(Inline,"DEFINITION"))
			strncpy(this_elem->description,&(Inline[12]),79);

		else if(Find(Inline,"AUTHOR"))
			strncpy(this_elem->authority,&(Inline[12]),79);

		else if(Find(Inline,"  ORGANISM"))
			strncpy(this_elem->seq_name,&(Inline[12]),79);

		else if(Find(Inline,"ACCESSION"))
			strncpy(this_elem->id,&(Inline[12]),79);

		else if(Find(Inline,"ORIGIN"))
		{
			done = FALSE;
			len = 0;
			for(;done == FALSE && fgets(Inline,GBUFSIZ,file) != 0;)
			{
				if(Inline[0] != '/')
				{
					if(buflen == 0)
					{
						buflen = GBUFSIZ;
						buffer = Calloc(sizeof(char) ,
						    buflen);
					}

					else if (len+strlen(Inline) >= buflen)
					{
						buflen += GBUFSIZ;
						buffer = Realloc(buffer,
						    sizeof(char)*buflen);
						for(j=buflen-GBUFSIZ
						    ;j<buflen;j++)
							buffer[j] = '\0';
					}
/*
*	Search for the fist column of data (whitespace-number-whitespace)data
*/
					if(start_col == -1)
					{
	                    for(start_col=0; Inline[start_col] == ' ' ||
							Inline[start_col] == '\t';start_col++);

	                    for(start_col++;strchr("1234567890",
							Inline[start_col]) != NULL;start_col++);

	                    for(start_col++; Inline[start_col] == ' ' ||
							Inline[start_col] == '\t';start_col++);

					}
                    for(j=start_col;(c = Inline[j]) != '\0';j++)
					{
						if((c != '\n') &&
						    ((j-start_col + 1) % 11 !=0))
							buffer[len++] = c;
					}
				}
				else
				{
					AppendNA(buffer,len,&(dataset->
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
*		Test if sequence should be converted by the translation table
*		If it looks like a protein...
*/
			if(dataset->element[curelem].rmatrix &&
			    IS_REALLY_AA == FALSE)
			{
				IS_REALLY_AA = CheckType(dataset->element[curelem].
				    sequence,dataset->element[curelem].seqlen);

				if(IS_REALLY_AA == FALSE)
					Ascii2NA(dataset->element[curelem].sequence,
					    dataset->element[curelem].seqlen,
					    dataset->element[curelem].rmatrix);
				else
/*
*		Force the sequence to be AA
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
		else if (Find(Inline,"ZZZZZ"))
		{
			Cfree(gencomments);
			genclen = 0;
		}
		else
		{
			if (gencomments == NULL)
			{
				gencomments = String(Inline);
				genclen = strlen(gencomments)+1;
			}
			else
			{
				genclen += strlen(Inline)+1;
				gencomments = Realloc(gencomments,genclen *
				    sizeof(char));
				strncat(gencomments,Inline,GBUFSIZ);
				strncat(gencomments,"\n",GBUFSIZ);
			}
		}
	}
	Cfree(buffer);
	fclose(file);
	for(j=0;j<dataset->numelements;j++)
		dataset->maxlen = MAX(dataset->maxlen,
		    dataset->element[j].seqlen+dataset->element[j].offset);
	return;
}



WriteGen(aln,filename,method,maskable)
NA_Alignment *aln;
char *filename;
int method,maskable;
{
	int i,j,k,mask = -1;
	FILE *file;
	NA_Sequence *this_elem;
	extern char month[12][6];
	char c;
	if(aln == NULL)
		return;
	if(aln->na_ddata == NULL)
		return;

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
		if((aln->element[j].selected && j!=mask && method != SELECT_REGION)
		||(aln->element[j].subselected && method == SELECT_REGION)
		|| (method == ALL))
		{
			this_elem = &(aln->element[j]);
			fprintf(file,
			    "LOCUS       %10s%8d bp    %4s  %10s   %2d%5s%4d\n",
			    this_elem->short_name,this_elem->seqlen+this_elem->offset,
			    (this_elem->elementtype == DNA) ? "DNA":
			    (this_elem->elementtype ==RNA)?"RNA":
			    (this_elem->elementtype == MASK)?"MASK":
			    (this_elem->elementtype == PROTEIN)?"PROT":"TEXT",
				this_elem->attr & IS_CIRCULAR?"Circular":"",
			    this_elem->t_stamp.origin.dd,
			    month[this_elem->t_stamp.origin.mm-1],
			    this_elem->t_stamp.origin.yy>1900?this_elem->t_stamp.origin.yy:
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
	return;
}

SetTime(struct gde_time *a)
{
	struct tm *tim,*localtime();
	long clock;

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
*	CheckType:  Check base composition to see if the sequence
*	appears to be an amino acid sequence.  If it is, pass back
*	TRUE, else FALSE.
*/
CheckType(seq,len)
char *seq;
int len;
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
