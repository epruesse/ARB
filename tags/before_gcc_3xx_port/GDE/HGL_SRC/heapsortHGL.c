#include <stdio.h>
#include "global_defs.h"

main(argc, argv)
int argc;
char **argv;
{
    int ii, seq_size, seq_maxsize, *order;
    char Pkey[32], Skey[32];
    Sequence *seq_set;
    FILE *fp;

    if(argc == 1)
    {
	fprintf(stderr, "\n%s\n%s\n%s\n%s\n%s\n%s\n",
		"Description:",
		"  Sorts HGL records by primary and secondary keys, output the",
		"      result to stdout.  Sort in descending order if 'decs' is specified.",
		"  Valid keys are:",
		"    type name sequence-ID creator offset barcode group-ID",
		"    seqlen film membrane contig probing-date creation-date",
		"    autorad-date");

	fprintf(stderr, "\nUsage: \n");
	fprintf(stderr,"heapsortHGL filename primaryKey [secondaryKey] [decs]\n\n");
	exit(0);
    }
    
    if(argc == 2)
    {
	fprintf(stderr, "Primary Key is Required.\n");
	exit(1);
    }
    strcpy(Pkey, argv[2]);
    Skey[0] = '\0';
    if(argc > 3 && strcmp(argv[3], "decs") != 0)
    {
	strcpy(Skey, argv[3]);
    }
    seq_size = 0;
    seq_maxsize = 64;
    seq_set = (Sequence *)Calloc(seq_maxsize, sizeof(Sequence));

    if((fp=fopen(argv[1], "r")) == NULL)
    {
	fprintf(stderr, "Can't open file %s.\n", argv[1]);
	exit(1);
    }
    
    while(ReadRecord(fp, &(seq_set[seq_size])) != -1)
    {
	SeqNormal(&(seq_set[seq_size]));
	if(++seq_size == seq_maxsize)
	{
	    seq_maxsize *= 2;
	    seq_set = (Sequence *)
	      Realloc(seq_set, seq_maxsize*sizeof(Sequence));
	}
    }

    order = (int *)Calloc(seq_size, sizeof(int));
    for(ii = 0; ii<seq_size; ii++)
      order[ii] = ii;
	    
    heapsort(seq_set, seq_size, Pkey, Skey, &order);

    if(strcmp(argv[argc-1], "decs") == 0)
    {
	/* output in descending order. */
	for(ii = seq_size-1; ii >= 0; ii--)
	  WriteRecord(stdout, &(seq_set[order[ii]]), NULL, 0);
    }
    else
    {
	/* output in ascending order. */
	for(ii = 0; ii<seq_size; ii++)
	  WriteRecord(stdout, &(seq_set[order[ii]]), NULL, 0);
    }
}
