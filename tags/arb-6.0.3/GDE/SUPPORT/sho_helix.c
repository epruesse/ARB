#include "Flatio.c"
#define BLACK 8
#define RED 3
#define BLUE 6
#define YELLOW 1
#define AQUA 4
main()
{
	struct data_format data[10000];
	int i,j,k,numseqs,mask = -1;
	int pair[20000],stack[20000],spt = 0;
	char ch;

	numseqs = ReadFlat(stdin,data,10000);
        if(numseqs == 0)
                exit(1);
	for(j=0;j<numseqs;j++)
		if(data[j].type == '"')
			mask = j;
	if(mask == -1)
		exit(1);

	for(j=0;j<data[mask].length;j++)
	{
		if(data[mask].nuc[j] == '[')
			stack[spt++] = j;
		else if(data[mask].nuc[j] == ']')
		{
			i = stack[--spt];
			pair[j] = i;
			pair[i] = j;
		}
		else
			pair[j] = -1;
	}

	for(j=0;j<numseqs;j++)
		if(j!=mask)
		{
			printf("name:%s\nlength:%d\nstart:\n",
				data[j].name,data[j].length);
			i = MIN(data[mask].length,data[j].length);
			for(k=0;k<i;k++)
				if(pair[k] != -1)
					printf("%d\n",match(data[j].nuc[k],
						data[j].nuc[pair[k]]));
				else
					printf("8\n");
			for(k=0;k<data[j].length - data[mask].length;k++)
					printf("8\n");
		}
	exit(0);
}
			


int match(a,b)
char a,b;
{
	char aa,bb;
	aa=a|32;
	bb=b|32;

	fprintf(stderr,"%c %c\n",aa,bb);

	if(a=='-' || a=='~')
	{
		if((b=='-') || (b=='~'))
			return(BLACK);
		else
			return(RED);
	}
	else if(aa=='a' && (bb=='t' || bb=='u'))
		return(BLUE);
	else if(bb=='a' && (aa=='t' || aa=='u'))
		return(BLUE);
	else if(bb=='c' && aa=='g' )
		return(BLUE);
	else if(bb=='g' && aa=='c' )
		return(BLUE);
	else if(aa=='g' && (bb=='t' || bb=='u'))
		return(AQUA);
	else if(bb=='g' && (aa=='t' || aa=='u'))
		return(AQUA);
	else return(YELLOW);
}

