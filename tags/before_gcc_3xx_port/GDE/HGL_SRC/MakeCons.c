#include <stdio.h>
/* #include <malloc.h> */
#include "global_defs.h"

main(ac,av)
     int ac;
     char *av[];
{
    Sequence cons;          /* master alignment */
    Sequence *master;       /* Current Walking sets to add  */
    int cursize, maxsize = 10, ii;
    char str[2], cons_type;
    FILE *file, *consout_fp, *maskout_fp;
    int conserved_color, variable_color, partial_color, major_perc;

    if(ac == 1)
    {
	fprintf(stderr, "Usage:\n");
	fprintf(stderr,
		"%s %s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n\t%s\n",
		av[0],
		"sequence-file",
		"[-iupac]            IUPAC consensus. Default",
		"[-majority percent] Majority consensus (default percent: 75)",
		"[-maskv colorv]     Variable position color",
		"[-maskc colorc]     Conserved position color",
		"[-maskp colorp]     Partially conserved color",
		"[-consout output-consensus]  Default: stdout",
		"[-maskout output-mask]");
	exit (0);
    }

    InitRecord(&cons);

    if((file = fopen(av[1],"r")) == NULL)
    {
	fprintf(stderr, "Can't open sequence-file %s.\n", av[1]);
	exit(1);
    }


    master = (Sequence*)Calloc(maxsize,sizeof(Sequence));

    cursize = 0;
    while(ReadRecord(file,&(master[cursize])) != -1)
    {
	SeqNormal(&master[cursize]);
	if(++cursize == maxsize)
        {
            maxsize *= 2;
            master = (Sequence*)
	      Realloc(master, maxsize*sizeof(Sequence));
        }

	master[cursize].group_number = 99999;
    }

    fclose(file);

    cons_type = ' '; /* 'i':IUPAC,  'm':majority, 'k':mask */
    consout_fp = stdout;
    maskout_fp = NULL;
    conserved_color = 8; /* black */
    variable_color = 3; /* red */

    ii = 2;
    while(ii < ac)
    {
	if(strcmp(av[ii], "-iupac") == 0)
	{
	    cons_type = 'i';
	}
	else if(strcmp(av[ii], "-majority") == 0)
	{
	    cons_type = 'm';
	    ii++;
	    major_perc = atoi(av[ii]);
	}
	else if(strcmp(av[ii], "-maskv") == 0)
	{
	    variable_color = atoi(av[++ii]);
	    if(cons_type == ' ')
	      cons_type = 'k';
	}
	else if(strcmp(av[ii], "-maskc") == 0)
	{
	    conserved_color = atoi(av[++ii]);
	    if(cons_type == ' ')
	      cons_type = 'k';
	}
	else if(strcmp(av[ii], "-maskp") == 0)
	{
	    partial_color = atoi(av[++ii]);
	    if(cons_type == ' ')
	      cons_type = 'k';
	}
	else if(strcmp(av[ii], "-consout") == 0)
	{
	    if((consout_fp = fopen(av[++ii], "w")) == NULL)
	    {
		fprintf(stderr, "Can't open output file %s.\n",av[ii]);
		consout_fp = stdout;
	    }
	}
	else if(strcmp(av[ii], "-maskout") == 0)
	{
	    if((maskout_fp = fopen(av[++ii], "w")) == NULL)
	    {
		fprintf(stderr, "Can't open output file %s.\n",av[ii]);
	    }
	}
	else
	{
	    fprintf(stderr, "Invalid flag %s\n", av[ii]);
	}
	ii++;
    }

    if(cons_type == ' ')
      cons_type = 'i';

    if(cons_type != 'k')
    {
	if(maskout_fp != NULL)
	{
	    /* Useful only when output to GDE. */
	    fprintf(maskout_fp, "length:%d\n", cons.seqlen);
	    fprintf(maskout_fp, "start:\n");
	}

	if((cons_type == 'i' &&
	    MakeConsensus(master,cursize,&cons,0,cons_type)==FALSE) ||
	   (cons_type == 'm' &&
	    MajorityCons(master,cursize,&cons,0, major_perc) == FALSE))
	{
	    fprintf(stderr, "Failed to make consensus.\n");
	    exit(1);
	}
	WriteRecord(consout_fp, &cons, NULL, 0);
    }
    else
    {
	if(MakeScore(master,cursize,&cons,0) == FALSE)
	{
	    fprintf(stderr, "Failed to make consensus.\n");
	    exit(1);
	}

	/*WriteRecord(stdout, &cons, NULL, 0);
	printf("\n\n");*/

	if(maskout_fp == NULL)
	{
	    maskout_fp = stdout;
	}

	fprintf(maskout_fp, "length:%d\n", cons.seqlen);
	fprintf(maskout_fp, "start:\n");

	for(ii = 0; ii < cons.seqlen; ii++)
	{
	    switch(cons.c_elem[ii])
	    {
	      case 'F':
		fprintf(maskout_fp, "%d\n", partial_color);
		break;
	      case 'E':
		fprintf(maskout_fp, "%d\n", conserved_color);
		break;
	      default:
		fprintf(maskout_fp, "%d\n", variable_color);
		break;
	    }
	}
    }
    fclose(consout_fp);
    fclose(maskout_fp);
}
