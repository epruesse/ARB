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
	int Match = 2,Mismatch = 8;
	int i,j,k,l,numseqs,mis,Case = 32;
	int slen,pcnt,pos;
	int UT = FALSE;
	char c;
	if(ac < 3)
	{
		fprintf(stderr,
"usage: %s search_string %%mismatch [-case] [-match color] [-mismatch color]\n",
		av[0]);
		fprintf(stderr,"		[-u=t]\n");
		exit(0);
	}
	for (j=3;j<ac;j++)
	{
		if(strcmp("-case",av[j]) == 0)
			Case = 0;
		if(strcmp("-match",av[j]) == 0)
			sscanf(av[j+1],"%d",&Match);
		if(strcmp("-u=t",av[j]) == 0)
			UT = TRUE;
		if(strcmp("-mismatch",av[j]) == 0)
			sscanf(av[j+1],"%d",&Mismatch);
	}
	numseqs = ReadFlat(stdin,data,10000);

	slen = strlen(av[1]);
	sscanf(av[2],"%d",&pcnt);
	pcnt *= slen;
	pcnt /= 100;

	if(UT)
		for(j=0;j<=strlen(av[1]);j++)
		{
			if(av[1][j] == 't')
				av[1][j] = 'u';
			if(av[1][j] == 'T')
				av[1][j] = 'U';
		}

	for(i=0;i<numseqs;i++)
	{
		if(UT)
			for(j=0;data[i].nuc[j]!='\0';j++)
			{
				if(data[i].nuc[j] == 't') 
					data[i].nuc[j] = 'u';
				else if(data[i].nuc[j] == 'T') 
					data[i].nuc[j] = 'U';
			}
		printf("name:%s\n",data[i].name);
		printf("length:%zu\n",strlen(data[i].nuc));
		printf("start:\n");
		for(j=0;j<data[i].length;j++)
		{
			mis = 0;
			for(k=0,pos=j;k<slen && pos<data[i].length;k++,pos++)
			{
				c = data[i].nuc[pos];
				for(;(c == ' ' || c == '-' || c == '~') &&
					pos < data[i].length;)
					c = data[i].nuc[++pos];
				c |= Case;

				if(data[i].type == '#')
				{
    				    if(CompIUP(c,(av[1][k]|Case)) == FALSE)
					mis++;
				}
				else
				{
				    if(c!=(av[1][k]|Case))
					mis++;
				}
			}
			if(k == slen && mis<=pcnt)
			{
				for(k=j;k<pos;k++)
					printf("%d\n",Match);
				j = pos-1;
			}
			else
				printf("%d\n",Mismatch);
		}
	}
	exit(0);
}


int CompIUP(a,b)
char a,b;
{
	static int tmatr[16] = {'-','a','c','m','g','r','s','v',
                        't','w','y','h','k','d','b','n'};
 
        static int matr[128] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x01,0xe,0x02,0x0d,0,0,0x04,0x0b,0,0,0x0c,0,0x03,0x0f,0,0,0,0x05,0x06,
        0x08,0x08,0x07,0x09,0x00,0xa,0,0,0,0,0,0,0,0x01,0x0e,0x02,0x0d,0,0,0x04,
        0x0b,0,0,0x0c,0,0x03,0x0f,0,0,0,0x05,0x06,0x08,0x08,0x07,0x09,0x00,0x0a,
        0,0,0,0,0x00,0
        };


	int testa,testb;

	if(a&32 != b&32)
		return(FALSE);

	testa = matr[(int)a];
	testb = matr[(int)b];
	return(testa&testb);
}
