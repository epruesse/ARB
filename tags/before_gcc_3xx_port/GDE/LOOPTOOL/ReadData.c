#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/canvas.h>
#include <stdio.h>
#include <string.h>
/* #include <malloc.h> */
#include "loop.h"
#include "globals.h"

ReadSeqSet(dataset)
DataSet *dataset;
{
	int i,j,taxanum=0,len,pos,flag=TRUE;
	char inlin[132],temp[132],seq[20000],*name;

	for(i=0;fgets(inlin,132,infile);i++)
	{
		for(;(pos=Find("LOCUS",inlin)) == -1 && flag;
			flag = (int)fgets(inlin,132,infile));
		sscanf((inlin+5+pos),"%s",temp);
		name=malloc(strlen(temp));
		strcpy(name,temp);
		len=0;
		for(;flag && Find("ORIGIN",inlin) == -1;
			flag =(int)fgets(inlin,132,infile));
		for(fgets(inlin,132,infile);
			flag && Find("//",inlin)== -1;
			flag = (int)fgets(inlin,132,infile))
		{
			for(j=0;inlin[j]!='\0';j++)
				if(inlin[j] != ' ' && inlin[j]!='\t'
				 && (inlin[j] <'0' || inlin[j] >'9') &&
				inlin[j] != '\n')
					seq[len++]=inlin[j];
		}
		seq[len]='\0';
		if(Find(temp,"HELIX")!= -1 ||
			Find("Helix",temp)!= -1 ||
			Find(temp,"helix")!= -1 )
		{
			dataset->helix.name=name;
			ErrorOut("malloc failed in ReadData",
			dataset ->helix.sequence = malloc(len));
			strcpy(dataset->helix.sequence,seq);
			dataset->helix.len=len;
		}
		else
		{
			ErrorOut("malloc failed in ReadData",
			dataset ->taxa[taxanum].sequence = malloc(len));
			strcpy(dataset->taxa[taxanum].sequence,seq);
			dataset->taxa[taxanum].len=len;
			dataset->taxa[taxanum++].name=name;
		}
	}
	dataset->siz = taxanum;
	dataset->len = dataset -> helix.len;
}

Find(a,b)
char a[],b[];
{
	int i,j,range,fail=TRUE;
	range=strlen(b)-strlen(a);
	if(range<0)
		return(-1);
	for(j=0;j<=range && fail;j++)
	{
		fail=FALSE;
		for(i=0;(a[i] != '\0') && (fail == FALSE);i++)
			if (a[i] != b[j+i])
				fail=TRUE;
	}
	return(fail ? -1:j);
}



Translate(dset,seqnum,blist,seqlen)
DataSet *dset;
Base **blist;
int seqnum,*seqlen;
{
	int pos=0,j,pair;
	char *helix,*seq;
	int stk[10000],stkp;

	Mxdepth = 0;
	*seqlen=dset->taxa[seqnum].len;
	ErrorOut("Error in malloc (Translate)",
	(*blist)=(Base*)calloc(*seqlen, sizeof(Base)));
	helix=dset->helix.sequence;
	seq=dset->taxa[seqnum].sequence;
	seqname=dset->taxa[seqnum].name;
	for(j=0;j<dset->taxa[seqnum].len;j++)
		if((seq[j]!='-') && (seq[j] != '.'))
		{
			(*blist)[pos].pair= -1;
			if(j<dset->len)
			{
				(*blist)[pos].depth=stkp;
				if(helix[j] =='[')
				{
					stk[stkp++]=pos;
					(*blist)[pos].depth=stkp;
				}
				if(helix[j] == ']')
				{
					pair=stk[--stkp];
					(*blist)[pair].pair=pos;
					(*blist)[pos].pair=pair;
				}
				if(stkp>Mxdepth)
					Mxdepth = stkp;
			}
			(*blist)[pos].known=FALSE;
			(*blist)[pos].dir=CW;
			(*blist)[pos].attr=0;
			(*blist)[pos].size=9;
			(*blist)[pos].dforw.pair= -1;
			(*blist)[pos].dback.pair= -1;
			(*blist)[pos].pos=NULL;
			(*blist)[pos].posnum= -1;
			(*blist)[pos].rel_loc=j;
			(*blist)[pos++].nuc=seq[j];
		}
	(*blist)[0].x=0.0;
	(*blist)[0].y=0.0;
	(*blist)[0].known=TRUE;
	(*blist)[pos-1].x=3.0;
	(*blist)[pos-1].y=0.0;
	(*blist)[pos-1].known=TRUE;

	*seqlen=pos;
	for(j=19;j<pos;j+=20)
	{
		baselist[j].label = (Label*)malloc(sizeof(Label));
		baselist[j].label->dist = 1.5;
		baselist[j].label->distflag = TRUE;
		sprintf(baselist[j].label->text,"%d",j+1);
		baselist[j].attr |= BOLD;
	}
	baselist[0].label = (Label*)malloc(sizeof(Label));
	baselist[0].label->dist = 1.5;
	baselist[0].label->distflag = TRUE;
	sprintf(baselist[0].label->text,"%d",1);
	baselist[0].attr |= BOLD;

	ddepth = Mxdepth+5;
	return;
}
