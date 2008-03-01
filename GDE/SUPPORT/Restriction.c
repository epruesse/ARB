/* 
*       Copyright 1991 Steven Smith at the Harvard Genome Lab.
*       All rights reserved.
*/
#include "Flatio.c"


main(ac,av)
int ac;
char **av;
{
	struct data_format data[10000];
	FILE *file;
	int i,j,k,color,numseqs,numenzymes,nextpos,len;
	char enzymes[80][80],dummy[80];
	if(ac<3)
	{
		fprintf(stderr,"Usage: %s enzyme_file seq_file\n",av[0]);
		exit(-1);
	}
	file = fopen(av[2],"r");
	if(file == NULL)
		exit(-1);

	numseqs = ReadFlat(file,data,10000);

	file = fopen(av[1],"r");
	if(file == NULL)
		exit(-1);

	for(numenzymes = 0;
		fscanf(file,"%s %s",enzymes[numenzymes],dummy)>0;
			numenzymes++);

	for(i=0;i<numseqs;i++)
	{
/*
		if(numseqs>1)
*/
		printf("name:%s\n",data[i].name);
		printf("length:%zu\n",strlen(data[i].nuc));
		if(numseqs>1)
			printf("nodash:\n");
		printf("start:\n");
		for(j=0;j<data[i].length;)
		{
			for(;data[i].nuc[j] == '-' && j<data[i].length;)
			{
				printf("8\n");
				j++;
			}
			if((nextpos = FindNext(data[i].nuc,j,enzymes,numenzymes
			,&len,&color)) != -1)
			{
				for(k=j;k<nextpos;k++)
					printf("8\n");
				for(k=j+nextpos;k<j+nextpos+len;k++)
					printf("%d\n",color);
				j=nextpos+len;
			}
			else
			for(;j<data[i].length;j++)
				 printf("8\n");
		}
	}
	exit(0);
}


FindNext(target,offset,enzymes,numenzymes,match_len,color)
char *target,enzymes[][80];
int numenzymes,*match_len,*color;
{
        int i,j,k,closest,len1,dif,flag = FALSE;
	closest = strlen(target);
	*match_len = 0;
	for(k=0;k<numenzymes;k++)
	{
	        dif = (strlen(target)) - (len1 = strlen(enzymes[k])) +1;

        	if(len1>0)
                	for(flag = FALSE,j=offset;j<dif && flag == FALSE;j++)
                	{
                        	flag = TRUE;
                        	for(i=0;i<len1 && flag;i++)
				{
                                	flag = Comp(enzymes[k][i],target[i+j])?
					TRUE:FALSE;
				}
                	}
		if(j-1<closest)
		{
			closest = j-1;
			*color = k%6+1;
			*match_len = strlen(enzymes[k]);
		}
	}
	if(closest + *match_len < strlen(target))
	        return(closest);
	else
		return(-1);
}

Comp(a,b)
char a,b;
{
        static int CtoB[128]={
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x01,0xe,0x02,0x0d,0,0,0x04,0x0b,0,0,0x0c,0,0x03,0x0f,0,0,0,0x05,0x06,
        0x08,0x08,0x07,0,0x09,0xa,0,0,0,0,0,0,0,0x01,0x0e,0x02,0x0d,0,0,0x04,
        0x0b,0,0,0x0c,0,0x03,0x0f,0,0,0,0x05,0x06,0x08,0x08,0x07,0,0x09,0x0a,
        0,0,0,0,0x00,0
        };

        static int BtoC[128] =
        {
        '-','A','C','M','G','R','S','V','T','W','Y','H','K','D','B','N',
        '~','a','c','m','g','r','s','v','t','w','y','h','k','d','b','n',
        '-','A','C','M','G','R','S','V','T','W','Y','H','K','D','B','N',
        '~','a','c','m','g','r','s','v','t','w','y','h','k','d','b','n',
        '-','A','C','M','G','R','S','V','T','W','Y','H','K','D','B','N',
        '~','a','c','m','g','r','s','v','t','w','y','h','k','d','b','n',
        '-','A','C','M','G','R','S','V','T','W','Y','H','K','D','B','N',
        '~','a','c','m','g','r','s','v','t','w','y','h','k','d','b','n',
        };


        return ((CtoB[a]) & (CtoB[b]));
}
