#include <stdio.h>
/* #include <malloc.h> */
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/notice.h>
#include <xview/panel.h>
#include "menudefs.h"
#include "defines.h"

/*
LoadData():
	Load a data set from the command line argument.

Copyright (c) 1989, University of Illinois board of trustees.  All rights
reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/
	extern NA_Alignment *DataSet;
	extern int DataType,FileFormat,Default_DNA_Trans[],Default_RNA_Trans[];
	extern int Default_NA_RTrans[],Default_PROColor_LKUP[],Default_NAColor_LKUP[];

	extern Frame frame;
	extern Canvas EditCan,EditNameCan;
	extern char FileName[];

void LoadData(filename)
char *filename;
{
	FILE *file;
	NA_Alignment *DataNaAln;
	char temp[1024];
/*
*	Get file name, determine the file type, and away we go..
*/
	if(Find2(filename,"gde")!=0)
		strcpy(FileName,filename);

#ifdef USE_ARB
	if (strstr(filename,".arb") || strchr(filename, ':')) {	/* ARBDB TYPE */
		if (DataSet == NULL) {
			DataSet = (NA_Alignment *) Calloc(1,
						      sizeof(NA_Alignment));
			DataNaAln = DataSet;
			DataSet->rel_offset = 0;
		} else{
			DataNaAln = DataSet;
		}
		DataType = NASEQ_ALIGN;
		FileFormat = ARBDB;
		LoadFile(filename, DataNaAln,
			 DataType, FileFormat);

		sprintf(temp,"Remote ARBDB access (%s)",filename);
		xv_set(frame,
		    FRAME_LABEL, temp,
		    0);
		return;
	}
#endif

	if( (file=fopen(filename,"r"))!=0 )
	{
		FindType(filename,&DataType,&FileFormat);
		switch(DataType)
		{
		case NASEQ_ALIGN:
			if(DataSet == NULL)
			{
				DataSet = (NA_Alignment*)Calloc(1,
				    sizeof(NA_Alignment));
				DataNaAln =(NA_Alignment*)DataSet;
				DataSet->rel_offset = 0;
			}
			else
				DataNaAln = (NA_Alignment*)DataSet;

			LoadFile(filename,DataNaAln,
			    DataType,FileFormat);

			break;
		default:
			break;
		}
	}
	fclose(file);
	sprintf(temp,"Genetic Data Environment 2.2 (%s)",FileName);
	xv_set(frame,
	    FRAME_LABEL, temp,
	    0);
	return;
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

void LoadFile(filename,dataset,type,format)
     char         *filename;
     NA_Alignment *dataset;
     int           type,format;
{
	extern int DataType;

	if (DataType != type)
		fprintf(stderr,"Warning, datatypes do not match.\n");
/*
Handle the overwrite/create/merge dialog here.
*/
	switch(format)
	{
	case NA_FLAT:
		ReadNA_Flat(filename,dataset,type);
		dataset->format = GDE;
		break;

	case GENBANK:
		ReadGen(filename,dataset,type);
		dataset->format = GENBANK;
		break;

#ifdef USE_ARB
	case ARBDB:
		ReadArbdb(filename,dataset,type);
		dataset->format = ARBDB;
		break;
#endif
	case GDE:
		ReadGDE(filename,dataset,type);
		dataset->format = GDE;
		break;
	case COLORMASK:
		ReadCMask(filename);

	default:
		break;
	}
	return;
}



/*
 *	Print error message, and die (if code is not zero)
 */
void ErrorOut1(code,string)
     int         code;
     const char *string;
{
	if (code != 0)
	{
            fprintf(stderr,"Error:%s\n",string);
            fflush(stderr);
            gde_assert(0);
            exit(1);
	}
	return;
}


/*
*	More robust memory management routines
*/
char *Calloc(count,size)
int count,size;
{
	char       *temp;
#ifdef SeeAlloc
	extern int  TotalCalloc;
	TotalCalloc += count*size;
	fprintf(stderr,"Calloc %d %d\n",count*size,TotalCalloc);
#endif
	temp         = calloc(count,size);
	ErrorOut1(temp == 0,"Cannot allocate memory");
	return(temp);
}

char *Realloc(block,size)
char *block;
int size;
{
	char       *temp;
#ifdef SeeAlloc
	extern int  TotalRealloc;
	TotalRealloc += size;
	fprintf(stderr,"Realloc %d\n",TotalRealloc);
#endif
	temp          = realloc(block,size);
	ErrorOut1(temp == 0,"Cannot change memory size");
	return(temp);
}

void Cfree(block)
char* block;
{
	if (block) cfree(block);
	else Warning("Error in Cfree, NULL block");
}



/*
*	same as strdup
*/
char *String(string)
char *string;
{
	char *temp;

	temp = Calloc(strlen(string)+1,sizeof(char));
	strcpy(temp,string);
	return(temp);
}


int FindType(name,dtype,ftype)
char *name;
int *dtype,*ftype;
{
	FILE *file;
	char Inline[GBUFSIZ];

	file = fopen(name,"r");
	*dtype=0;
	*ftype=0;

	if (file == NULL)
		return(1);

	/*
*	Is this a flat file?
*	Get the first non blank line, see if a type marker shows up.
*/
	fgets(Inline,GBUFSIZ,file);
	for(;strlen(Inline)<2 && fgets(Inline,GBUFSIZ,file) != NULL;);
	if(Inline[0] == '#' || Inline[0] == '%' ||
	    Inline[0] == '"' || Inline[0] == '@' )
	{
		*dtype=NASEQ_ALIGN;
		*ftype=NA_FLAT;
	}

	/*
*	Else, try genbank
*/
	else
	{
		fclose(file);
		file = fopen(name,"r");
		*dtype=0;
		*ftype=0;

		if (file == NULL)
			return(1);

		for(;fgets(Inline,GBUFSIZ,file) != NULL;)
			if(Find(Inline,"LOCUS"))
			{
				*dtype=NASEQ_ALIGN;
				*ftype=GENBANK;
				fclose(file);
				return(0);
			}
		/*
*	and last, try GDE
*/
			else if(Find(Inline,"sequence"))
			{
				*dtype = NASEQ_ALIGN;
				*ftype = GDE;
				fclose(file);
				return(0);
			}
			else if(Find(Inline,"start:"))
			{
				*dtype = NASEQ_ALIGN;
				*ftype = COLORMASK;
				fclose(file);
				return(0);
			}
	}

	fclose(file);
	return(0);
}

void AppendNA(buffer,len,seq)
NA_Base *buffer;
int len;
NA_Sequence *seq;
{
	int curlen=0,j;
/* 	NA_Base *temp; */

	if(seq->seqlen+len >= seq->seqmaxlen)
	{
		if(seq->seqlen>0)
			seq->sequence = (NA_Base*)Realloc(seq->sequence,
			    (seq->seqlen + len+GBUFSIZ) * sizeof(NA_Base));
		else
			seq->sequence = (NA_Base*)Calloc(1,(seq->seqlen +
			    len+GBUFSIZ) * sizeof(NA_Base));
		seq->seqmaxlen = seq->seqlen + len+GBUFSIZ;
	}
	/*
*	seqlen is the length, and the index of the next free
*	base
*/
	curlen = seq->seqlen + seq->offset;
	for(j=0;j<len;j++)
		putelem(seq,j+curlen,buffer[j]);

	seq->seqlen += len;
	return;
}

void Ascii2NA(buffer,len,matrix)
     char *buffer;
     int   len;
     int   matrix[16];
{
	/*
*	if the translation matrix exists, use it to
*	encode the buffer.
*/
	register int i;
	if(matrix != NULL)
		for(i=0;i<len;i++)
			buffer[i] = matrix[buffer[i]];
	return;
}

WriteNA_Flat(aln,filename,method,maskable)
     NA_Alignment *aln;
     char         *filename;
     int           method,maskable;
{
	int j,kk,mask = -1,k,offset,min_offset= -999999;
	char offset_str[100],buf[100];
	NA_Sequence *seqs;
	FILE *file;
	if(aln == NULL)
		return;
	if(aln->numelements == NULL)
		return;
	seqs = aln->element;

	file = fopen(filename,"w");
	if(file == NULL)
	{
		Warning("Cannot open file for output");
		return(1);
	}
	if(maskable && (method != SELECT_REGION))
	{
		for(j=0;j<aln->numelements;j++)
			if(seqs[j].elementtype == MASK &&
			    seqs[j].selected)
				mask = j;
	}
	for(j=0;j<aln->numelements;j++)
	{
		SeqNorm(&(seqs[j]));
	}

	for(j=0;j<aln->numelements;j++)
	{
		if(method != SELECT_REGION)
			offset = seqs[j].offset;
		else
			for(offset=seqs[j].offset;
			aln->selection_mask[offset] == '0';
			offset++);

		if(offset+aln->rel_offset != 0)
			sprintf(offset_str,"(%d)",offset+aln->rel_offset);
		else
			offset_str[0] = '\0';

		if(((j!=mask) && (seqs[j].selected) && method != SELECT_REGION)
		|| (method == SELECT_REGION && seqs[j].subselected)
		|| method == ALL)
		{
			fprintf(file,"%c%s%s\n",
			    seqs[j].elementtype == DNA?'#':
			    seqs[j].elementtype == RNA?'#':
			    seqs[j].elementtype == PROTEIN?'%':
			    seqs[j].elementtype == TEXT?'"':
			    seqs[j].elementtype == MASK?'@':'"',
			    seqs[j].short_name,
			    (offset+aln->rel_offset  == 0)? "":offset_str);
			if(seqs[j].tmatrix)
			{
				if(mask == -1)
					for(k=0,kk=0;kk<seqs[j].seqlen;kk++)
					{
						if((k)%60 == 0 && k>0)
						{
							buf[60] = '\0';
							fputs(buf,file);
							putc('\n',file);
						}
						if(method == SELECT_REGION)
						{
							if(aln->selection_mask[kk+offset]=='1')
							{
								buf[k%60] =((char)seqs[j].tmatrix[
								(int)getelem( &(seqs[j]),kk+offset) ]);
								k++;
							}
						}
						else
						{
							buf[k%60] =((char)seqs[j].tmatrix[
							(int)getelem( &(seqs[j]),kk+offset) ]);
							k++;
						}
					}
				else
					for(k=0,kk=0;kk<seqs[j].seqlen;kk++)
					{
						if(getelem(&(seqs[mask]),kk+seqs[mask].offset) != '0'
						    && (getelem(&(seqs[mask]),kk+seqs[mask].offset)
						    != '-'))
						{
							if((k++)%60 == 0 && k>1)
							{
								buf[60] = '\0';
								fputs(buf,file);
								putc('\n',file);
							}
							buf[k%60] = ((char)seqs[j].tmatrix
							    [getelem(&(seqs[j]),kk+offset)]);
						}
					}
			}
			else
			{
				if(mask == -1)
					for(k=0,kk=0;kk<seqs[j].seqlen;kk++)
					{
						if((k)%60 == 0 && k>0)
						{
							buf[60] = '\0';
							fputs(buf,file);
							putc('\n',file);
						}
						if(method == SELECT_REGION)
						{
							if(aln->selection_mask[kk+offset]=='1')
							{
								buf[k%60] =(getelem( &(seqs[j]),kk+offset));
								k++;
							}
						}
						else
						{
							buf[k%60] =( getelem( &(seqs[j]),kk+offset) );
							k++;
						}
					}
				else
					for(k=0,kk=0;kk<seqs[j].seqlen;kk++)
					{
						if(getelem(&(seqs[mask]),kk+offset) == '1')
						{
							if((k++)%60 == 0 && k>1)
							{
								buf[60] = '\0';
								fputs(buf,file);
								putc('\n',file);
							}
							buf[k%60] =((char)getelem(&(seqs[j]),
							    kk+offset));
						}
					}
			}
			buf[(k%60)>0 ? (k%60):60] = '\0';
			fputs(buf,file);
			putc('\n',file);
		}
	}
	fclose(file);
	return(0);
}


void Warning(s)
     const char *s;
{
	extern Frame frame;
	extern Panel_item left_foot,right_foot;
	Beep();
	xv_set(frame,FRAME_RIGHT_FOOTER,s,0);
	xv_set(right_foot,PANEL_LABEL_STRING,s,0);
}


InitNASeq(seq,type)
NA_Sequence *seq;
int type;
{
	extern int Default_DNA_Trans[],Default_NA_RTrans[];
	extern int
		Default_NA_RTrans[],Default_PROColor_LKUP[],Default_NAColor_LKUP[];

	SetTime(&(seq->t_stamp.origin));
	SetTime(&(seq->t_stamp.modify));
	strncpy(seq->id,uniqueID(),79);
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
	seq->mask = NULL;
	seq->seqlen = 0;
	seq->seqmaxlen = 0;
	seq->protect = PROT_WHITE_SPACE + PROT_TRANSLATION;
#ifdef HGL
	seq->attr = 0;
#else
	seq->attr = IS_5_TO_3 + IS_PRIMARY;
#endif
	seq->elementtype = type;
	seq->groupid = 0;
	seq->groupb = NULL;
	seq->groupf = NULL;
	seq->cmask = NULL;
	seq->selected = 0;
	seq->subselected = 0;

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


ReadCMask(filename)
char *filename;
{
	extern Frame frame;
	extern NA_Alignment *DataSet;

	char Inline[GBUFSIZ],head[GBUFSIZ],curname[GBUFSIZ],
	temp[GBUFSIZ];
	int IGNORE_DASH = FALSE,offset;
	NA_DisplayData *NAdd;
	NA_Alignment *aln;

	int i,j,k,curlen = 0,*colors,orig_ctype,jj,indx = 0;
	FILE *file;

	if(DataSet == NULL) return;

	NAdd = (NA_DisplayData*)((NA_Alignment*)DataSet)->na_ddata;

	if(NAdd == NULL)
		return;

	aln = (NA_Alignment*)DataSet;

	curname[0] = '\0';
	orig_ctype = NAdd->color_type;
	file = fopen(filename,"r");
	if(file == NULL)
	{
		Warning("File not found");
		Warning(filename);
		return;
	}

	NAdd->color_type = COLOR_ALN_MASK;
	for(;fgets(Inline,GBUFSIZ,file) !=0;)
	{
		if(Find(Inline,"offset:"))
		{
			crop(Inline,head,temp);
			sscanf(temp,"%d",&(aln->cmask_offset));
		}
		else if(Find(Inline,"nodash:"))
			IGNORE_DASH = TRUE;
		else if(Find(Inline,"dash:"))
			IGNORE_DASH = TRUE;
		else if(Find(Inline,"name:"))
		{
			crop(Inline,head,curname);
			curname[strlen(curname)-1] = '\0';
			for(j=0;j<strlen(curname);j++)
				if(curname[j] == '(')
					curname[j] = '\0';
		}
		else if(Find(Inline,"length:"))
		{
			crop(Inline,head,temp);
			sscanf(temp,"%d",&curlen);
		}
		else if(Find(Inline,"start:"))
		{
			indx = -1;
			if(curlen == 0)
			{
				Warning("illegal format in colormask");
				NAdd->color_type = orig_ctype;
				return;
			}
			if(strlen(curname) != 0)
			{
				indx = -1;
				for(j=0;j<aln->numelements;j++)
					if(Find(aln->element[j].short_name,curname)
					    || Find(aln->element[j].id,curname))
					{
						if(aln->element[j].cmask != NULL)
							Cfree(aln -> element[j].cmask);
						colors=(int*)Calloc(aln->element[j]
						    .seqmaxlen+1+aln->element[j].offset
						    ,sizeof(int));
						aln->element[j].cmask = colors;
						NAdd->color_type = COLOR_SEQ_MASK;
						indx = j;
						j = aln->numelements;
					}
				if(indx == -1)
					colors=NULL;
			}
			else
			{
				if(aln->cmask != NULL) Cfree(aln->cmask);
				colors=(int*)Calloc(curlen,sizeof(int));
				aln->cmask = colors;
				aln->cmask_len = curlen;
				NAdd->color_type = COLOR_ALN_MASK;
				for(j=0;j<curlen;j++)
					colors[j] = 12;
			}

			if(IGNORE_DASH && (indx != -1))
			{
				for(jj=0,j=0;(j<curlen) &&
				    (jj<aln->element[indx].seqlen);j++,jj++)
				{
					offset = aln->element[indx].offset;
					if(fgets(Inline,GBUFSIZ,file)==NULL)
					{
						Warning
						    ("illegal format in colormask");
						NAdd->color_type = orig_ctype;
						return;
					}
/*
*	Fixed so that the keyword nodash causes the colormask to be mapped
*	to the sequence, not the alignment.
*
*	The allocated space is equal the seqlen of the matched sequence.
*
*/
					if(aln->element[indx].tmatrix)
						for(;(getelem(&(aln->element[indx]),jj
						    +offset)
						    ==(aln->element[indx].tmatrix['-'])
						    || (getelem(&(aln->element[indx]),jj
						    +offset)
						    ==aln->element[indx].tmatrix['~']))
						    && jj < aln->element[indx].seqlen;)
							colors[jj++] = 12;
					else
						for(;getelem(&(aln->element[indx]),jj
						    +offset)
						    =='-' && jj < aln->element[indx].seqlen;)
							colors[jj++] = 12;

					sscanf(Inline,"%d",&(colors[jj]));
				}
			}
			else if((indx == -1)  && (strlen(curname) != 0))
				for(j=0;j<curlen;j++)
					fgets(Inline,GBUFSIZ,file);
			else
				for(j=0;j<curlen;j++)
				{
					if(fgets(Inline,GBUFSIZ,file)==NULL)
					{
						Warning
						    ("illegal format in colormask");
						NAdd->color_type = orig_ctype;
						return;
					}
					sscanf(Inline,"%d",&(colors[j]));
				}
			IGNORE_DASH = FALSE;
			curname[0] = '\0';
		}

	}
	RepaintAll(TRUE);
	return;
}


ReadNA_Flat(filename,dataset,type)
char *filename;
char *dataset;
int type;
{
	int i, j, jj, c, curelem,offset;
	char name[GBUFSIZ];
	char buffer[GBUFSIZ];
	char origin[GBUFSIZ],ref[GBUFSIZ];
	char Inline[GBUFSIZ],head[GBUFSIZ],tail[GBUFSIZ],temp[GBUFSIZ];
	char curname[GBUFSIZ];

	NA_Sequence *this_elem;
	NA_Alignment *data;
	extern int Default_DNA_Trans[],Default_RNA_Trans[],Default_NA_RTrans[];

	FILE *file;

	curname[0] = '\0';
	data = (NA_Alignment*)dataset;

	file = fopen(filename,"r");
	if(file == NULL)
	{
		fprintf(stderr,"Cannot open %s.\n",filename);
		return;
	}
	for(;fgets(Inline,GBUFSIZ,file) !=0;)
	{
		if(
			Inline[0] == '#' ||
		    Inline[0] == '%' ||
		    Inline[0] == '"' ||
		    Inline[0] == '@'
		    )
		{
			offset = 0;
			for(j=0;j<strlen(Inline);j++)
			{
				if(Inline[j] == '(')
				{
					sscanf((char*)
					    &(Inline[j+1]),"%d",&offset);
					Inline[j] = '\0';
				}
			}

			curelem = data->numelements++;
			if( curelem == 0 )
			{
				data->element=(NA_Sequence*)
				    Calloc(5,sizeof(NA_Sequence));
				data->maxnumelements = 5;
			}
			else if (curelem==data->maxnumelements)
			{
				(data->maxnumelements) *= 2;
				data->element=
				    (NA_Sequence*)Realloc(data->element
				    ,data->maxnumelements*sizeof(NA_Sequence));
			}

			InitNASeq(&(data->element[curelem]),
			    Inline[0] == '#'?DNA:
			    Inline[0] == '%'?PROTEIN:
			    Inline[0] == '"'?TEXT:
			    Inline[0] == '@'?MASK:TEXT);
			this_elem= &(data->element[curelem]);
			if(Inline[strlen(Inline)-1] == '\n')
				Inline[strlen(Inline)-1] = '\0';
			strncpy(this_elem->short_name,(char*)&(Inline[1]),31);
			this_elem->offset = offset;
		}
		else if(Inline[0] != '\n')
		{
			for(j=0,jj=0;j<strlen(Inline);j++)
				if(Inline[j] != ' ' && Inline[j] != '\n' &&
				    Inline[j] != '\t')
					buffer[jj++] = Inline[j];

			if(data->element[curelem].rmatrix)
				Ascii2NA(buffer,jj,data->element[curelem]
				    .rmatrix);
			AppendNA(buffer,jj,&(data->element[curelem]));
		}
	}

	for(j=0;j<data->numelements;j++)
		data->maxlen = MAX(data->maxlen,data->element[j].seqlen +
		    data->element[j].offset);

	for(j=0;j<data->numelements;j++)
		if(data->element[j].seqlen==0)
			data->element[j].protect =
			    PROT_BASE_CHANGES+ PROT_GREY_SPACE+
			    PROT_WHITE_SPACE+ PROT_TRANSLATION;

	NormalizeOffset(data);
	Regroup(data);
	return;
}


WriteStatus(aln,filename,method)
NA_Alignment *aln;
char *filename;
int method;
{
	extern int EditMode,FileFormat;
	extern NA_Alignment *DataSet;
	NA_DisplayData *NAdd;
	NA_Sequence *this_seq;
	int j;
	FILE *file;

	if(DataSet == NULL)
		return;

	NAdd = (NA_DisplayData*)((NA_Alignment*)DataSet)->na_ddata;
	if(NAdd == NULL)
		return;

	file = fopen(filename,"w");
	if (file == NULL)
	{
		Warning("Cannot open status file.");
		return(1);
	}
	fprintf(file,"File_format: %s\n",FileFormat==GENBANK?"genbank":"flat");
	/*
	fprintf(file,"EditMode: %s\n",EditMode==INSERT?"insert":
					"check");
*/

	this_seq = &(aln->element[NAdd->cursor_y]);
	if(this_seq->id != NULL)
		fprintf(file,"sequence-ID %s\n",this_seq->id);
	fprintf(file,"Column: %d\nPos:%d\n",NAdd->cursor_x,NAdd->position);
	switch(this_seq->elementtype)
	{
	case DNA:
	case RNA:
		fprintf(file,"#%s\n",
		    this_seq->short_name);
		break;
	case PROTEIN:
		fprintf(file,"%%%s\n",
		    this_seq->short_name);
		break;
	case MASK:
		fprintf(file,"@%s\n",
		    this_seq->short_name);
		break;
	case TEXT:
		fprintf(file,"%c%s\n",'"',
		    this_seq->short_name);
		break;
	default:
		break;
	}
	if(this_seq->tmatrix)
		for(j=0;j<this_seq->seqlen;j++)
			putc(this_seq->tmatrix[getelem(this_seq,j)],file);
	else
		for(j=0;j<this_seq->seqlen;j++)
			putc(getelem(this_seq,j),file);

	fclose(file);
	return;
}

ReadStatus(filename)
char *filename;
{
	/*
	int i,j;
	FILE *file;
	char Inline[GBUFSIZ],head[GBUFSIZ];
	file = fopen(filename,"r");
	for(;!DONE;)
	{
		fgets(Inline,GBUFSIZ,file);
		if(strlen(Inline) == 0)
			DONE = TRUE;
		else
		{
			sscanf(Inline,"%s",head);
			if(strncmp(head,"Col",3) != 0)
			{
				sscanf(Inline,"%*s %d",head,&(DataSet->nadd->
				cursor_x),&(DataSet->nadd->cursory);
			}
			else if(strncmp(head,"Pos",3) != 0)
			{
			}
		}
	}

*/
}


NormalizeOffset(aln)
NA_Alignment *aln;
{
	int i,j,offset = 99999999;

	for(j=0;j<aln->numelements;j++)
		offset = MIN(offset,aln->element[j].offset);

	for(j=0;j<aln->numelements;j++)
		aln->element[j].offset -= offset;

	aln->maxlen = -999999999;
	for(j=0;j<aln->numelements;j++)
		aln->maxlen = MAX(aln->element[j].seqlen+aln->element[j].offset,
		    aln->maxlen);

	aln->rel_offset += offset;

	if(aln->numelements == 0)
		aln->rel_offset = 0;

	return;
}

WriteCMask(aln,filename,method,maskable)
NA_Alignment *aln;
char *filename;
int method,maskable;
{
	int j,kk,mask = -1,k,offset,min_offset= -999999;
	char offset_str[100];
	int *buf;
	NA_Sequence *seqs;
	FILE *file;
	if(aln == NULL)
		return;
	if(aln->numelements == NULL)
		return;
	seqs = aln->element;

	file = fopen(filename,"w");
	if(file == NULL)
	{
		Warning("Cannot open file for output");
		return(1);
	}
	if(maskable && (method != SELECT_REGION))
	{
		for(j=0;j<aln->numelements;j++)
			if(seqs[j].elementtype == MASK &&
			    seqs[j].selected)
				mask = j;
	}
	for(j=0;j<aln->numelements;j++)
	{
		SeqNorm(&(seqs[j]));
	}

	for(j=0;j<aln->numelements;j++)
	{
		if(method != SELECT_REGION)
			offset = seqs[j].offset;
		else
			for(offset=seqs[j].offset;
			aln->selection_mask[offset] == '0';
			offset++);

		if(offset+aln->rel_offset != 0)
			sprintf(offset_str,"(%d)",offset+aln->rel_offset);
		else
			offset_str[0] = '\0';

		if(((j!=mask) && (seqs[j].selected) && method != SELECT_REGION)
		|| (method == SELECT_REGION && seqs[j].subselected)
		|| method == ALL)
		{
			fprintf(file,"%c%s%s\n",
			    seqs[j].elementtype == DNA?'#':
			    seqs[j].elementtype == RNA?'#':
			    seqs[j].elementtype == PROTEIN?'%':
			    seqs[j].elementtype == TEXT?'"':
			    seqs[j].elementtype == MASK?'@':'"',
			    seqs[j].short_name,
			    (offset+aln->rel_offset  == 0)? "":offset_str);

			if(seqs[j].cmask != NULL)
			{

				buf =(int*) Calloc(seqs[j].seqlen,sizeof(int) );

				if(mask == -1)
				{
					for(k=0,kk=0;kk<seqs[j].seqlen;kk++)
					{
						if(method == SELECT_REGION)
						{
							if(aln->selection_mask[kk+offset]=='1')
								buf[k++] = (getcmask( &(seqs[j]),kk+offset));
						}

						else
							buf[k++] =( getcmask( &(seqs[j]),kk+offset) );
					}
				}
				else
				{
					for(k=0,kk=0;kk<seqs[j].seqlen;kk++)
						if(getelem(&(seqs[mask]),kk+offset) == '1')
							buf[k++] =(getcmask(&(seqs[j]), kk+offset));
/*
*			Looks like k might be one behind?
*/
				}
				fprintf(file,"name:%s\noffset:%d\nlength:%d\nstart:\n",
						seqs[j].short_name,seqs[j].offset,k);

				for(kk = 0; kk < k; kk++)
					fprintf(file, "%d\n", buf[kk]);

				Cfree(buf);
			}
		}
	}
	fclose(file);
	return(0);
}
