
#include <stdio.h>
#include "convert.h"
#include "global.h"

/* ---------------------------------------------------------------
*	Function init_phylip().
*		Initialize genbank entry.
*/
void init_phylip()	{

}
/* ---------------------------------------------------------------
 *	Function to_phylip()
 *		Convert from some format to PHYLIP format.
 */
void to_phylip(inf, outf, informat,readstdin)
     char *inf, *outf;
     int   informat;
     int   readstdin;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	int	         maxsize, current, total_seq;
	int	         out_of_memory, indi;
	char	     temp[TOKENNUM], eof;
	char	    *name;

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s, exit\n", inf);
		error(64, temp);
	}
    ifp              = create_FILE_BUFFER(inf, IFP);
	if(Lenstr(outf) <= 0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s, exit\n", outf);
		error(117, temp);
	}
	maxsize = 1;
	out_of_memory = 0;
	name = NULL;
	init();
	init_phylip();
	total_seq = 0;
	do	{
		if(informat==ALMA)	{
			init_alma();
			eof=alma_in(ifp);
		} else if(informat==GENBANK)	{
			init_genbank();
			eof=genbank_in_locus(ifp);
		} else if(informat==EMBL||informat==PROTEIN)	{
			init_embl();
			eof=embl_in_id(ifp);
		} else if(informat==MACKE)	{
			init_macke();
			eof=macke_in_name(ifp);
		} else error(34, "UNKNOW input format when converting to PHYLIP format.");
		if(eof==EOF) break;
		if(informat==ALMA)	{
			alma_key_word(data.alma.id, 0, temp, TOKENNUM);
		} else if(informat==GENBANK)	{
			genbank_key_word(data.gbk.locus, 0, temp, TOKENNUM);
		} else if(informat==EMBL||informat==PROTEIN)	{
			embl_key_word(data.embl.id, 0, temp, TOKENNUM);
		} else if(informat==MACKE)	{
			Cpystr(temp, data.macke.seqabbr);
		} else error(119, "UNKNOW input format when converting to PHYLIP format.");
		total_seq++;
        
		if((name = Dupstr(temp))==NULL&&temp!=NULL) { out_of_memory=1; break; }
		if(data.seq_length>maxsize) maxsize = data.seq_length;
        
        if (!realloc_sequence_data(total_seq)) { out_of_memory = 1; break; }

        data.ids[total_seq-1]     = name;
		data.seqs[total_seq-1]    = (char*)Dupstr(data.sequence);
        data.lengths[total_seq-1] = Lenstr(data.sequence);
	} while(!out_of_memory);

	if(out_of_memory)	{	/* cannot hold all seqs into mem. */
		fprintf(stderr,
		"Rerun the conversion throught one seq. by one seq. base.\n");
		destroy_FILE_BUFFER(ifp); fclose(ofp);
		to_phylip_1x1(inf, outf, informat);
		return;
	}
	current = 0;
	fprintf(ofp, "%4d %4d", maxsize, current);
	if (readstdin){
	    int c;
            int spaced = 0;
	    while (1) {
		c = getchar();
		if (c == EOF) break; /* read all from stdin now (not only one line)*/
        /* 		if (c == EOF||c=='\n') break; */
                if (!spaced) {
                    fputc(' ', ofp);
                    spaced = 1;
                }
		fputc(c,ofp);
	    }

	}
	fprintf(ofp,"\n");

	while(maxsize>current)	{
		for(indi=0; indi<total_seq; indi++)	{
			phylip_print_line(data.ids[indi], data.seqs[indi], data.lengths[indi], current, ofp);
		}
		if(current==0) current +=(SEQLINE-10);
		else current += SEQLINE;
		if(maxsize>current) fprintf(ofp, "\n");
	}
	/* rewrite output header */
	rewind(ofp);
	fprintf(ofp, "%4d %4d", total_seq, maxsize);

	destroy_FILE_BUFFER(ifp); fclose(ofp);

#ifdef log
	fprintf(stderr, "Total %d sequences have been processed\n", total_seq);
#endif

}
/* ---------------------------------------------------------------
*	Function to_phylip_1x1()
*		Convert from one format to PHYLIP format, one seq by one seq.
*/
void
to_phylip_1x1(inf, outf, informat)
     char	*inf, *outf;
     int	informat;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	int	         maxsize, current, total_seq;
	char         temp[TOKENNUM], eof;
	char	    *name;

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s, exit\n", inf);
		error(123, temp);
	}
    ifp              = create_FILE_BUFFER(inf, IFP);
	if(Lenstr(outf) <= 0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s, exit\n", outf);
		error(124, temp);
	}
	maxsize = 1; current = 0;
	name = NULL;
	fprintf(ofp, "%4d %4d\n", maxsize, current);
	while(maxsize>current)	{
		init();
		FILE_BUFFER_rewind(ifp);
		total_seq = 0;
		do	{	/* read in one sequence */
			init_phylip();
			if(informat==ALMA)	{
				init_alma();
				eof=alma_in(ifp);
			} else if(informat==GENBANK)	{
				init_genbank();
				eof=genbank_in_locus(ifp);
			} else if(informat==EMBL||informat==PROTEIN)	{
				init_embl();
				eof=embl_in_id(ifp);
			} else if(informat==MACKE)	{
				init_macke();
				eof=macke_in_name(ifp);
			} else error(128, "UNKNOWN input format when converting to PHYLIP format.");
			if(eof==EOF) break;
			if(informat==ALMA)	{
				alma_key_word(data.alma.id, 0, temp, TOKENNUM);
			} else if(informat==GENBANK)	{
				genbank_key_word(data.gbk.locus, 0, temp, TOKENNUM);
			} else if(informat==EMBL||informat==PROTEIN)	{
				embl_key_word(data.embl.id, 0, temp, TOKENNUM);
			} else if(informat==MACKE)	{
				macke_key_word(data.macke.name, 0, temp, TOKENNUM);
			} else error(130, "UNKNOWN input format when converting to PHYLIP format.");
			Freespace(&name);
			name = Dupstr(temp);
			if(data.seq_length>maxsize) maxsize = data.seq_length;
			phylip_print_line(name, data.sequence, 0, current, ofp);
			total_seq++;
		} while(1);
		if(current==0)	current += (SEQLINE-10);
		else current += SEQLINE;
		if(maxsize>current) fprintf(ofp, "\n");
	}	/* print block by block */

	rewind(ofp);
	fprintf(ofp, "%4d %4d", total_seq, maxsize);

	destroy_FILE_BUFFER(ifp); fclose(ofp);

#ifdef log
	fprintf(stderr, "Total %d sequences have been processed\n", total_seq);
#endif

}
/* --------------------------------------------------------------
 *	Function phylip_print_line().
 *		Print phylip line.
 */
void
phylip_print_line(name, sequence, seq_length, index, fp)
     char *name, *sequence;
     int   seq_length;
     int	index;
     FILE	*fp;
{
	int	indi, indj, length, bnum;

	if(index==0)	{
		if(Lenstr(name)>10)	{
			/* truncate id length of seq ID is greater than 10 */
			for(indi=0; indi<10; indi++) fputc(name[indi], fp);
			bnum = 1;
		} else	{
			fprintf(fp, "%s", name);
			bnum = 10 - Lenstr(name)+1;
		}
		/* fill in blanks to make up 10 chars for ID. */
		for(indi=0; indi<bnum; indi++) fputc(' ', fp);
		length = SEQLINE - 10;
	} else if(index>=data.seq_length) length = 0;
    else length = SEQLINE;

    if (seq_length == 0) seq_length = Lenstr(sequence);
    for(indi=indj=0; indi<length; indi++) {
        if((index+indi)<seq_length) {
            char c= sequence[index+indi];
			if (c=='.') c= '?';
			fputc(c,fp);
			indj++;
			if(indj==10 && (index+indi)<(seq_length-1) && indi<(length-1)) {
				fputc(' ', fp);
				indj=0;
			}
		} else break;
	}
	fprintf(fp, "\n");
}
