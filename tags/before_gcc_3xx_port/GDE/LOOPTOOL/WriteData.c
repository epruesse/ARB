
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/canvas.h>
#include <stdio.h>
#include <string.h>
/* #include <malloc.h> */
#include "loop.h"
#include "globals.h"

SaveTemp()
{
	int i,j;
	FILE *outfile;
	Base base;

	outfile = fopen("loop.temp","w");
	if(outfile == NULL)
	{
		fprintf(stderr,"Sorry, cannot open save file\n");
		return;
	}

	for(i=0;i<seqlen;i++)
	{
		base = baselist[i];
		fprintf(outfile,"index %d\n",base.rel_loc);
		fprintf(outfile,"attr %d\n",base.attr);
		fprintf(outfile,"size %d\n",base.size);
		fprintf(outfile,"dir %d\n",base.dir);
		if(base.label !=NULL)
			fprintf(outfile,"label %s\n",base.label->text);
		if(base.dforw.pair != -1)
			fprintf(outfile,"distance %d %f\n",
			baselist[base.dforw.pair].rel_loc,base.dforw.dist);
		if(base.posnum>0)
		{
			fprintf(outfile,"positional %d\n",base.posnum);
			for(j=0;j<base.posnum;j++)
				fprintf(outfile,"%d %f %f\n",
				baselist[base.pos->pair].rel_loc,
				base.pos->dx,base.pos->dy);
		}
	}
	fclose(outfile);
	return;
}

ImposeTemp()
{
	int i,j,k,nxd,temp;
	double x,y,ftemp;
	char inlin[132];

	for(;fgets(inlin,132,tempfile) != NULL;)
	{
		if(find("index",inlin))
		{
			sscanf(inlin,"%*6c%d",&temp);
			nxd = ReltoAbs(temp);
		}

		else if(find("attr",inlin))
		{
			sscanf(inlin,"%*5c%d",&temp);
			baselist[nxd].attr = temp;
		}

		else if(find("size",inlin))
		{
			sscanf(inlin,"%*5c%d",&temp);
			baselist[nxd].size = temp;
		}

		else if(find("dir",inlin))
		{
			sscanf(inlin,"%*4c%d",&temp);
			baselist[nxd].dir = temp;
		}

		else if(find("distance",inlin))
		{
			sscanf(inlin,"%*9c%d %f",&temp,&ftemp);
			baselist[nxd].dforw.pair = ReltoAbs(temp);
			baselist[nxd].dforw.dist = ftemp;
		}

		else if(find("positional",inlin))
		{
			sscanf(inlin,"%*11c%d",&k);
			for(j=0;j<k;j++)
			{
				baselist[nxd].posnum = -1;
				fscanf(tempfile,"%d %f %f",&temp,&x,&y);
				SetPos(nxd,ReltoAbs(temp),x,y);
			}
		}
	}
	return;
}


ReltoAbs(ndx)
int ndx;
{
	int i,j;
	for(j=0;j<seqlen;j++)
		if(baselist[j].rel_loc == ndx)
			return(j);
	fprintf(stderr," Rel to Abs: not found in %d positions\n",seqlen);
	return(-1);
}

find(a,b)
char *a,*b;
{
	int i,j,len1,len2,dif,flag = FALSE;

	dif = (len2 = strlen(b)) - (len1 = strlen(a));

	for(j=0;j<dif && flag == FALSE;j++)
	{
		flag = TRUE;
		for(i=0;i<len1 && flag;i++)
			flag = (a[i] == b[i+j])?TRUE:FALSE;

	}
	return(flag);
}
