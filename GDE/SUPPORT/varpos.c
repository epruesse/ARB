#include <stdlib.h>
#include "Flatio.c"
#define MAX(a,b) ((a)>(b)?(a):(b))
 
/*
*       Varpos.c- An extremely simple program for showing which positions
*       are varying in an alignment.  Use this as a model for other
*       external functions.
*
*       Read in a flat file alignment, pass back an alignment color
*       mask.
* 
*       Copyright 1991/1992 Steven Smith, Harvard Genome lab. 
* 
*/ 

main(ac,av)
int ac;  
char **av; 
{
	struct data_format data[10000];
        int i,j,k,numseqs,rev = FALSE;
	int maxlen = -99999,
	score = 0,
	minoffset = 99999;
        char ch; 
        if(ac>2) 
        {
                fprintf(stderr,"Usage %s [-rev]<gde_flat_file>gde_color_mask\n",                av[0]); 
                exit(1); 
        } 
        if(ac == 2)
                if(strcmp(av[1],"-rev") == 0) 
                        rev = TRUE;

	numseqs = ReadFlat(stdin,data,10000);

	if(numseqs == 0)
		exit(1);

	for(j=0;j<numseqs;j++)
	{
		if(data[j].length+data[j].offset > maxlen)
			maxlen = data[j].length+data[j].offset;
		if(data[j].offset < minoffset)
			minoffset = data[j].offset;
	}

	printf("length:%d\n",maxlen);
	printf("offset:%d\n",minoffset);
	printf("start:\n");
	for(j=0;j<maxlen;j++)
	{
		int a=0,c=0,g=0,u=0;

		for(k=0;k<numseqs;k++)
			if(data[k].length+data[k].offset > j)
			{
				if(j>data[k].offset)
					ch=data[k].nuc[j-data[k].offset] | 32;
				else
					ch = '-';

				if(ch=='a')a++;
				if(ch=='c')c++;
				if(ch=='g')g++;
				if(ch=='u')u++;
				if(ch=='t')u++;
			}

		score=MAX(a,c);
		score=MAX(score,g);
		score=MAX(score,u);
		if(a+c+g+u)
		{
			if(rev)
				score=(score*6/(a+c+g+u)+8);
			else
				score=((8-score*6/(a+c+g+u))+8);
		}
		else
			score=8;
		printf("%d\n",score);
	}
	exit(0);
}
