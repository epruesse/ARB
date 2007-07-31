#include <stdio.h>
#include "convert.h"
#include "global.h"

#define PRTLENGTH	62

/* ---------------------------------------------------------------
*	Function to_printable()
*		Convert from some format to PRINTABLE format.
*/
void
to_printable(inf, outf, informat)
     char	*inf, *outf;
     int	informat;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	int	         maxsize, current, total_seq, length;
	int	         out_of_memory, indi, index, *base_nums, base_count, start;
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
	total_seq = 0;
	base_nums=NULL;
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
		} else error(48,
		"UNKNOW input format when converting to PRINABLE format.");
		if(eof==EOF) break;
		if(informat==ALMA)	{
			alma_key_word(data.alma.id, 0, temp, TOKENNUM);
		} else if(informat==GENBANK)	{
			genbank_key_word(data.gbk.locus, 0, temp, TOKENNUM);
		} else if(informat==EMBL||informat==PROTEIN)	{
			embl_key_word(data.embl.id, 0, temp, TOKENNUM);
		} else if(informat==MACKE)	{
			Cpystr(temp, data.macke.seqabbr);
		} else error(120,
		"UNKNOW input format when converting to PRINABLE format.");
		total_seq++;
        
		if((name = Dupstr(temp))==NULL&&temp!=NULL) { out_of_memory=1; break; }
		if(data.seq_length>maxsize) maxsize = data.seq_length;
        
        if (!realloc_sequence_data(total_seq)) { out_of_memory = 1; break; }

        base_nums=(int*)Reallocspace((char *)base_nums, sizeof(int)*total_seq);
		if(base_nums==NULL)	{ out_of_memory=1; break; }

		data.ids[total_seq-1]     = name;
		data.seqs[total_seq-1]    = (char*)Dupstr(data.sequence);
        data.lengths[total_seq-1] = Lenstr(data.sequence);
		base_nums[total_seq-1]    = 0;

	} while(!out_of_memory);

	if(out_of_memory)	{	/* cannot hold all seqs into mem. */
		fprintf(stderr,
		"Rerun the conversion throught one seq. by one seq. base.\n");
		destroy_FILE_BUFFER(ifp); fclose(ofp);
		to_printable_1x1(inf, outf, informat);
		return;
	}
	current = 0;
	while(maxsize>current)	{
		for(indi=0; indi<total_seq; indi++)	{
			length = Lenstr(data.seqs[indi]);
			for(index=base_count=0;
			index<PRTLENGTH&&(current+index)<length; index++)
				if(data.seqs[indi][index+current]!='~'&&
				data.seqs[indi][index+current]!='-'&&
				data.seqs[indi][index+current]!='.')
					base_count++;

			/* check if the first char is base or not */
			if(current<length&&data.seqs[indi][current]!='~'&&
			data.seqs[indi][current]!='-'&&
			data.seqs[indi][current]!='.')
				start = base_nums[indi]+1;
			else start = base_nums[indi];

			printable_print_line(data.ids[indi],
				data.seqs[indi], current, start, ofp);
			base_nums[indi] += base_count;
		}
		current += PRTLENGTH;
		if(maxsize>current) fprintf(ofp, "\n\n");
	}

	Freespace((char **)&(base_nums));

#ifdef log
	fprintf(stderr, "Total %d sequences have been processed\n", total_seq);
#endif

	destroy_FILE_BUFFER(ifp); fclose(ofp);
}
/* ---------------------------------------------------------------
*	Function to_printable_1x1()
*		Convert from one foramt to PRINTABLE format, one seq by one seq.
*/
void
to_printable_1x1(inf, outf, informat)
     char 	*inf, *outf;
     int	informat;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	int	         maxsize, current, total_seq;
	int	         base_count, index;
	char	     temp[TOKENNUM], eof;
	char	    *name;

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s, exit\n", inf);
		error(125, temp);
	}
    ifp              = create_FILE_BUFFER(inf, IFP);
	if(Lenstr(outf) <= 0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s, exit\n", outf);
		error(126, temp);
	}
	maxsize = 1; current = 0;
	name = NULL;
	while(maxsize>current)	{
		init();
		FILE_BUFFER_rewind(ifp);
		total_seq = 0;
		do	{	/* read in one sequence */
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
			} else error(129,
		"UNKNOW input format when converting to PRINTABLE format.");
			if(eof==EOF) break;
			if(informat==ALMA)	{
				alma_key_word(data.alma.id, 0, temp, TOKENNUM);
			} else if(informat==GENBANK)	{
				genbank_key_word(data.gbk.locus, 0, temp, TOKENNUM);
			} else if(informat==EMBL||informat==PROTEIN)	{
				embl_key_word(data.embl.id, 0, temp, TOKENNUM);
			} else if(informat==MACKE)	{
				macke_key_word(data.macke.name, 0, temp, TOKENNUM);
			} else error(131,
		"UNKNOW input format when converting to PRINTABLE format.");
			Freespace(&name);
			name = Dupstr(temp);
			if(data.seq_length>maxsize)
				maxsize = data.seq_length;
			for(index=base_count=0;
			index<current&&index<data.seq_length; index++) {
				if(data.sequence[index]!='~'
				&&data.sequence[index]!='.'
				&&data.sequence[index]!='-') base_count++;
			}
			/* check if the first char is a base or not */
			if(current<data.seq_length&&data.sequence[current]!='~'
			&&data.sequence[current]!='.'
			&&data.sequence[current]!='-') base_count++;

			/* find if there any non-gap char in the next 62
			* char of the seq. data
			* #### count no need to be the first base num
			for(index=current, count=0;
			count==0&&index<data.seq_length
			&&index<(current+PRTLENGTH); index++)
				if(data.sequence[index]!='~'
				&&data.sequence[index]!='.'
				&&data.sequence[index]!='-')
					{ base_count++; count++; }
			*/

			printable_print_line(name, data.sequence, current,
				base_count, ofp);
			total_seq++;
		} while(1);
		current += PRTLENGTH;
		if(maxsize>current) fprintf(ofp, "\n\n");
	}	/* print block by block */

	destroy_FILE_BUFFER(ifp); fclose(ofp);

#ifdef log
	fprintf(stderr, "Total %d sequences have been processed\n", total_seq);
#endif

}
/* ------------------------------------------------------------
*	Function printable_print_line().
*		print one printable line.
*/
void
printable_print_line(id, sequence, start, base_count, fp)
char	*id, *sequence;
int	start;
int	base_count;
FILE	*fp;
{
	int	indi, index, count, bnum, /*length,*/ seq_length;

	fprintf(fp, " ");
	if((bnum=Lenstr(id))>10)	{
		/* truncate if length of id is greater than 10 */
		for(indi=0; indi<10; indi++)
			fprintf(fp, "%c", id[indi]);
			bnum = 1;
	} else	{
		fprintf(fp, "%s", id);
		bnum = 10 - bnum + 1;
	}
	/* fill in the blanks to make up 10 chars id spaces */
	seq_length = Lenstr(sequence);
	if(start<seq_length)
		for(indi=0; indi<bnum; indi++)
			fprintf(fp, " ");
	else {	fprintf(fp, "\n"); return;	}
	fprintf(fp, "%4d ", base_count);
	for(index=start, count=0; count<PRTLENGTH&&index<seq_length; index++)	{
		fprintf(fp, "%c", sequence[index]);
		count++;
	}	/* printout sequence data */
	fprintf(fp, "\n");
}
