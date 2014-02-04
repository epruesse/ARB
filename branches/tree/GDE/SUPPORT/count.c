/* 
*       Copyright 1991 Steven Smith at the Harvard Genome Lab.
*       All rights reserved.
*/
#include <math.h>
#include "Flatio.c"

#define FALSE 0
#define TRUE 1

#define JUKES 0
#define OLSEN 1
#define NONE 2

#define Min(a,b) (a)<(b)?(a):(b)

int width,start,jump,usecase,sim,correction;
int tbl,numseq,num,denom,special;
char argtyp[255],argval[255];

float   acwt=1.0, agwt=1.0, auwt=1.0, ucwt=1.0, ugwt=1.0, gcwt=1.0;

float dist[200][200];

struct data_format data[10000];
float parta[200], partc[200], partg[200], partu[200], setdist();

main(ac,av)
int ac;
char **av;
{
	int i,j,k;
	extern int ReadFlat();
	FILE *file;

	width = 1;
	jump = 1;
	if(ac==1)
	{
		fprintf(stderr,
		    "usage: %s [-sim] [-case] [-c=<none,olsen,jukes>] ",av[0]);
		fprintf(stderr,"[-t] alignment_flat_file\n");
		exit(1);
	}
	for(j=1;j<ac-1;j++)
	{
		getarg(av,j,argtyp,argval);
		if(strcmp(argtyp,"-s=") == 0)
		{
			j++;
			sscanf(argval,"%d",&start);
			start --;
		}
		else if(strcmp(argtyp,"-m=") == 0)
		{
			j++;
			sscanf(argval,"%d",&width);
		}
		else if(strcmp(argtyp,"-j=") == 0)
		{
			j++;
			sscanf(argval,"%d",&jump);
		}
		else if(strcmp(argtyp,"-case") == 0)
			usecase = TRUE;

		else if(strcmp(argtyp,"-sim") == 0)
			sim = TRUE;

		else if(strcmp(argtyp,"-c=") == 0)
		{
			if(strcmp(argval,"olsen") == 0)
				correction = OLSEN;

			else if(strcmp(argval,"none") == 0)
				correction = NONE;

			else if(strcmp(argval,"jukes") == 0)
				correction = JUKES;

			else
				fprintf(stderr,"Correction type %s %s\n",
				    argval,"unknown, using JUKES");
		}
		else if(strcmp("-t",argtyp) == 0)
			tbl = TRUE;

		else if(strcmp("-ac=",argtyp) == 0 || strcmp("-ca=",argtyp)== 0)
		{
			j++;
			sscanf(argval,"%f",&acwt);
			special = TRUE;
		}
		else if(strcmp("-au=",argtyp) == 0 || strcmp("-ua=",argtyp)== 0)
		{
			j++;
			sscanf(argval,"%f",&auwt);
			special = TRUE;
		}
		else if(strcmp("-ag=",argtyp) == 0 || strcmp("-ga=",argtyp)== 0)
		{
			j++;
			sscanf(argval,"%f",&agwt);
			special = TRUE;
		}
		else if(strcmp("-uc=",argtyp) == 0 || strcmp("-cu=",argtyp)== 0)
		{
			j++;
			sscanf(argval,"%f",&ucwt);
			special = TRUE;
		}
		else if(strcmp("-ug=",argtyp) == 0 || strcmp("-gu=",argtyp)== 0)
		{
			j++;
			sscanf(argval,"%f",&ugwt);
			special = TRUE;
		}
		else if(strcmp("-gc=",argtyp) == 0 || strcmp("-cg=",argtyp)== 0)
		{
			j++;
			sscanf(argval,"%f",&gcwt);
			special = TRUE;
		}
		else if(strcmp("-transition=",argtyp) == 0)
		{
			j++;
			sscanf(argval,"%f",&ucwt);
			agwt = ucwt;
			special = TRUE;
		}
		else if(strcmp("-transversion=",argtyp) == 0)
		{
			j++;
			sscanf(argval,"%f",&gcwt);
			ugwt = gcwt;
			acwt = gcwt;
			auwt = gcwt;
			special = TRUE;
		}
	}


	file = fopen(av[ac-1],"r");
	if ((file == NULL) || (ac == 1))
	{
		fprintf(stderr,"Error opening input file %s\n",av[ac-1]);
		exit(1);
	}

	numseq = ReadFlat(file,data,10000);

	fclose(file);
	SetPart();

	for(j=0;j<numseq-1;j++)
		for(k=j+1;k<numseq;k++)
		{
			Compare(j,k,&num,&denom);
			dist[j][k] = setdist(num,denom,j,k);
		}

	Report();
	exit(0);
}


Compare(a,b,num,denom)
int a,b,*num,*denom;
{
	int mn,i,j,casefix,match,blank;
	float fnum = 0.0;
	struct data_format *da,*db;
	char ac,bc;

	casefix = (usecase)? 0:32;
	*num = 0;
	*denom = 0;

	da = &data[a];
	db = &data[b];
	mn = Min(da->length,db->length);

	for(j=0;j<mn;j+=jump)
	{
		match = TRUE;
		blank = TRUE;
		for(i=0;i<width;i++)
		{
			ac = da->nuc[j+i] | casefix;
			bc = db->nuc[j+i] | casefix;
			if(ac == 't')
				ac = 'u';
			if(ac == 'T')
				ac = 'U';
			if(bc == 't')
				bc = 'u';
			if(bc == 'T')
				bc = 'U';

			if((ac=='-') || (ac|32)=='n' || (ac==' ') ||
			    (bc== '-') || (bc|32)=='n' || (bc==' '));

			else
			{
				blank = FALSE;
				if(ac != bc)
				{
					match = FALSE;
					switch(ac)
					{
					case 'a':
						if (bc == 'c') fnum+=acwt;
						else if(bc == 'g') fnum+=agwt;
						else if(bc == 'u') fnum+=auwt;
						break;

					case 'c':
						if (bc == 'a') fnum+=acwt;
						else if(bc == 'g') fnum+=gcwt;
						else if(bc == 'u') fnum+=ucwt;
						break;

					case 'g':
						if (bc == 'a') fnum+=agwt;
						else if(bc == 'c') fnum+=gcwt;
						else if(bc == 'u') fnum+=ugwt;
						break;

					case 'u':
						if (bc == 'a') fnum+=auwt;
						else if(bc == 'c') fnum+=ucwt;
						else if(bc == 'g') fnum+=ugwt;
						break;

					case 't':
						if (bc == 'a') fnum+=auwt;
						else if(bc == 'c') fnum+=ucwt;
						else if(bc == 'g') fnum+=ugwt;
						break;

					default:
						break;
					};
				}
			}

			if((blank == FALSE) && match)
			{
				(*num) ++;
				(*denom) ++;
			}
			else if(!blank)
				(*denom) ++;
		}
	}
	if(special)
		(*num) = *denom - (int)fnum;
	return 0;
}


float setdist(num,denom,a,b)
int num,denom,a,b;
{
	float cor;
	switch (correction)
	{
	case OLSEN:
		cor = parta[a]*parta[b]+
		    partc[a]*partc[b]+
		    partg[a]*partg[b]+
		    partu[a]*partu[b];
		break;

	case JUKES:
		cor = 0.25;
		break;

	case NONE:
		cor = 0.0;
		break;

	default:
		cor = 0.0;
		break;
	};

	if(correction == NONE)
		return(1.0 - (float)num/(float)denom);
	else
		return( -(1.0-cor)*log(1.0 / (1.0-cor)*((float)num/(float)denom-cor)));
}


getarg(av,ndx,atype,aval)
char **av,atype[],aval[];
int ndx;
{
	int i,j;
	char c;
	for(j=0;(c=av[ndx][j])!=' ' && c!= '=' && c!= '\0';j++)
		atype[j]=c;
	if (c=='=')
	{
		atype[j++] = c;
		atype[j] = '\0';
	}
	else
	{
		atype[j] = '\0';
		j++;
	}

	if(c=='=')
	{
		for(i=0;(c=av[ndx][j]) != '\0' && c!= ' ';i++,j++)
			aval[i] = c;
		aval[i] = '\0';
	}
	return 0;
}

SetPart()
{
	int a,c,g,u,tot,i,j;
	char nuc;

	for(j=0;j<numseq;j++)
	{
		a=0;
		c=0;
		g=0;
		u=0;
		tot=0;

		for(i=0;i<data[j].length;i++)
		{
			nuc=data[j].nuc[i] | 32;
			switch (nuc)
			{
			case 'a':
				a++;
				tot++;
				break;

			case 'c':
				c++;
				tot++;
				break;

			case 'g':
				g++;
				tot++;
				break;

			case 'u':
				u++;
				tot++;
				break;

			case 't':
				u++;
				tot++;
				break;
			};
		}
		parta[j] = (float)a / (float)tot;
		partc[j] = (float)c / (float)tot;
		partg[j] = (float)g / (float)tot;
		partu[j] = (float)u / (float)tot;
	}
	return 0;
}


Report()
{
	int i,ii,jj,j,k;

	if(tbl)
		printf("#\n#-\n#-\n#-\n#-\n");
	for(jj=0,j=0;j<numseq;j++)
	{
		if(tbl)
			printf("%2d: %-.15s|",jj+1,data[j].name);

		for (i=0;i<j;i++)
		{
			if(sim)
				printf("%6.1f",100 - dist[i][j]*100.0);
			else
				printf("%6.1f",dist[i][j]*100.0);
		}
		printf("\n");
		jj++;
	}
	return 0;
}


int find(b,a)
char *a,*b;
{
	int flag,lenb,lena;
	register i,j;

	flag=0;
	lenb=strlen(b);
	lena=strlen(a);
	for (i=0;((i<lena) && flag==0);i++)
	{
		for(j=0;(j<lenb) && (a[i+j]==b[j]);j++);
		flag=((j==lenb)? 1:0);
	}
	return flag;
}

