#include "global_defs.h"

main(ac,av)
     int ac;
     char **av;
{
    Sequence *data;
    int i,j,k,numseqs=0,maxlen = 0,minlen=999999999;
    int lines_printed, Success, maxsize, ss;
    int width,scale = 0;
    int len[1000];
    FILE *infile;
    char a,b, style[32];
    int WIDTH;
    
    if(ac == 1)
    {
	fprintf(stderr,"Usage:%s\n", av[0]);
	fprintf(stderr, "  -in alignment_file [-width max_line_width (50)]\n");
	fprintf(stderr, "  [-scale scale (1)] [-style comp|poster (comp)]\n");
	exit(0);
    }
    
    for(i = 1; i < ac; i+=2)
    {
	if(av[i][0] != '-')
	{
	    fprintf(stderr, "\nInvalid flag %s.\n", av[i]);
	    fprintf(stderr, "Type %s for usage.\n\n", av[0]);
	    exit(1);
	}
	if(i+1 == ac)
	{
	    fprintf(stderr, "\nMissing value for flag %s.\n\n", av[i]);
	    exit(1);
	}
    }

    i = 1;
    scale = 1;
    WIDTH = 50;
    infile = NULL;
    strcpy(style, "comp");
    while(i < ac)
    {
	if(strcmp(av[i], "-in") == 0)
	{
	    if((infile = fopen(av[++i],"r")) == NULL)
	    { 
		fprintf(stderr,"Cannot open %s\n",av[i]); 
		exit(1); 
	    }
	}
	else if(strcmp(av[i], "-scale") == 0)
	{
	    sscanf(av[++i],"%d",&scale);
	    if(scale == 0)
	      scale = 1;
	}
	else if(strcmp(av[i], "-width") == 0)
	{
	    sscanf(av[++i], "%d", &WIDTH);
	}
	else if(strcmp(av[i], "-style") == 0)
	{
	    strcpy(style, av[++i]);
	}
	else
	{
	    fprintf(stderr, "\nUnknow flag %s\n\n", av[i]);
	    exit(1);
	}
	i++;
    }

    if(infile == NULL)
    {
	fprintf(stderr, "\nWhat do you want to print.%c\n\n", 7);
	exit(1); 
    }
    
    /*
     *		Read in alignment...
     */
    maxsize = 64;
    data = (Sequence *)Calloc(maxsize, sizeof(Sequence));
    for(numseqs = 0;Success != -1;numseqs++)
    {
	Success = ReadRecord(infile,&(data[numseqs]));
	if(numseqs == maxsize-1)
	{
	    maxsize *= 2;
	    data = (Sequence*)Realloc(data,
				      maxsize*sizeof(Sequence));
	}         
	
	for(j=0; j<data[numseqs].seqlen; j++)
	{
	    if(data[numseqs].c_elem[j] == 'u')
	      data[numseqs].c_elem[j] = 't';
	}
    }
    if(Success == FALSE)
      ErrorOut(0,"Problem reading sequences");
    numseqs --;
    
    /*
     *		Done.  Alignment is now in data[].
     */
    
    fclose(infile);
    
    if(numseqs == 0) exit(1);
    
    width = WIDTH*scale;

    for(k=0;k<numseqs;k++)
    { 
	if(data[k].seqlen > 0 || strcmp(style, "poster") == 0)
	{
	    minlen = MIN(minlen,data[k].offset); 
	    maxlen = MAX(maxlen,data[k].seqlen+data[k].offset); 
	}
    }

    for(j=minlen;j<maxlen;j+=width)
    {
	lines_printed = FALSE;
	for (i=0;i<numseqs;i++)
	{
	    data[i].name[19] = '\0';

	    if(strcmp(style, "comp") == 0 &&
	       (data[i].offset > j+width  || 
		data[i].offset+data[i].seqlen<j));
	    else
	    {
		lines_printed = TRUE;
		printf("\n%20s%5d ", data[i].name,
		       indx(j,&(data[i])));
		for(k=j;k<j+width;k+=scale)
		{
		    if((k<data[i].seqlen+data[i].offset)
		       && (k>=data[i].offset))
		      if(scale == 1)
			putchar(data[i].c_elem[k-data[i].offset]);
		      else 
			putchar('-');
		    else putchar(' ');
		}
	    }
	}
	if(lines_printed)
	{
	    printf("\n                          ");
	    ss = j+1;
	    while(ss < MIN(j+WIDTH*scale, maxlen+1))
	    {
		printf("|---------");
		ss += 10*scale;
	    }

	    printf("\n                          ");
	    ss = j+1;
	    while(ss < MIN(j+WIDTH*scale, maxlen+1))
	    {
		printf("%-10d",ss);
		ss += 10*scale;
	    }
	    printf("\n");
	}
	if(strcmp(style, "poster") == 0)
	  printf("\f");
    }
    putchar('\n');
    exit(0);
}


int indx(pos,seq)
     int pos;
     Sequence *seq;
{
    int j,count=0;
    if(pos < seq->offset)
      return (0);
    if(pos>seq->offset+seq->seqlen)
      pos = seq->offset+seq->seqlen;
    pos -= seq->offset;
    for(j=0;j<pos;j++)
      if(seq->c_elem[j] != '-')
	if(seq->c_elem[j] != '~')
	  count++;
    return (count);
}
