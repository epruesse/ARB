#include "Flatio.c"
#define WIDTH 50

main()
{
	struct data_format data[10000];
	int i,j,k,numseqs,maxlen = 0,minlen=999999999;
	int lines_printed;
	int len[1000];
	char a,b;

	numseqs = ReadFlat(stdin,data,10000);
        if(numseqs == 0)
                exit(1);

        for(k=0;k<numseqs;k++)
        { 
                minlen = MIN(minlen,data[k].offset); 
                maxlen = MAX(maxlen,data[j].length+data[k].offset); 
        }

	for(j=minlen;j<maxlen;j+=WIDTH)
	{
		lines_printed = FALSE;
		for (i=0;i<numseqs;i++)
		{
			data[i].name[19] = '\0';
			if(((data[i].offset > j+WIDTH)  ||
			(data[i].offset+data[i].length<j)));
			else
			{
				lines_printed = TRUE;
				printf("\n%20s%5d ", data[i].name,
				indx(j,&(data[i])));
				for(k=j;k<j+WIDTH;k++)
				{
				    if((k<data[i].length+data[i].offset)
				    && (k>=data[i].offset))
					putchar(data[i].nuc[k-data[i].offset]);
				    else putchar(' ');
				}
			}
		}
		if(lines_printed)
		{
			printf("\n                          |---------|---------|---------|---------|---------\n");
			printf("                     %6d    %6d    %6d    %6d    %6d\n\n",j+1,j+11,j+21,j+31,j+41);
		}
	}
	putchar('\n');
	exit(0);
}


int indx(pos,seq)
int pos;
struct data_format *seq;
{
	int j,count=0;
	if(pos < seq->offset)
		return (0);
	if(pos>seq->offset+seq->length)
		pos = seq->offset+seq->length;
	pos -= seq->offset;
	for(j=0;j<pos;j++)
		if(seq->nuc[j] != '-')
			if(seq->nuc[j] != '~')
				count++;
	return (count);
}
