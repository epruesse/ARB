
#include <stdio.h>
#include "convert.h"
#include "global.h"

/* ------------------------------------------------------------
*	Function init_paup().
*		Init. paup data.
*/
void init_paup() {
    free_sequence_data(data.paup.ntax);
	data.paup.ntax = 0;
	data.paup.nchar = 0;
}
/* -------------------------------------------------------------
*	Function to_paup()
*		Convert from some format to PAUP format.
*/
void
to_paup(inf, outf, informat)
     char	*inf, *outf;
     int	informat;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	int	         maxsize, current, total_seq, first_line;
	int	         out_of_memory, indi;
	char	     temp[TOKENNUM], eof;
	char	    *name, *today;

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp,
                "Cannot open input file %s, exit\n", inf);
		error(64, temp);
	}
    ifp              = create_FILE_BUFFER(inf, IFP);
	if(Lenstr(outf) <= 0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp,
                "Cannot open output file %s, exit\n", outf);
		error(65, temp);
	}
	maxsize = 1;
	out_of_memory = 0;
	name = NULL;
	init();
	init_paup();
	paup_print_header(ofp);
	total_seq = 0;
	do	{
		if(informat==ALMA)	{
			init_alma();
			eof=alma_in(ifp);
		} else if(informat==GENBANK)	{
			init_genbank();
			eof=genbank_in_locus(ifp);
		} else if(informat==EMBL||informat==PROTEIN) {
			init_embl();
			eof=embl_in_id(ifp);
		} else if(informat==MACKE)	{
			init_macke();
			eof=macke_in_name(ifp);
		} else error(63,
	"UNKNOW input format when converting to PAUP format.");
		if(eof==EOF) break;
		if(informat==ALMA)	{
			alma_key_word
				(data.alma.id, 0, temp, TOKENNUM);
		} else if(informat==GENBANK)	{
			genbank_key_word
				(data.gbk.locus, 0, temp, TOKENNUM);
		} else if(informat==EMBL||informat==PROTEIN)	{
			embl_key_word
				(data.embl.id, 0, temp, TOKENNUM);
		} else if(informat==MACKE)	{
			Cpystr(temp, data.macke.seqabbr);
		} else error(118,
		"UNKNOW input format when converting to PAUP format.");

		total_seq++;

		if((name = Dupstr(temp))==NULL&&temp!=NULL)
			{ out_of_memory=1; break; }
		paup_verify_name(&name);

		if(data.seq_length>maxsize) maxsize = data.seq_length;
        if (!realloc_sequence_data(total_seq)) { out_of_memory = 1; break; }

        data.ids[total_seq-1]     = name;
		data.seqs[total_seq-1]    = (char*)Dupstr(data.sequence);
        data.lengths[total_seq-1] = Lenstr(data.sequence);
	} while(!out_of_memory);

	if(out_of_memory)	{
		/* cannot hold all seqs into mem. */
		fprintf(stderr,
	"Rerun the conversion throught one seq. by one seq. base.\n");

		destroy_FILE_BUFFER(ifp); fclose(ofp);
		to_paup_1x1(inf, outf, informat);
		return;
	}
	current = 0;
	while(maxsize>current)	{
		first_line = 0;
		for(indi=0; indi<total_seq; indi++)	{
			if(current<data.lengths[indi]) first_line++;
			paup_print_line(data.ids[indi], data.seqs[indi], data.lengths[indi], current, (first_line==1), ofp);

 			/* Avoid repeating */
			if(first_line==1) first_line++;
		}
		current +=(SEQLINE-10);
		if(maxsize>current) fprintf(ofp, "\n");
	}

#ifdef log
	fprintf(stderr, "Total %d sequences have been processed\n", total_seq);
#endif

	fprintf(ofp, "      ;\nENDBLOCK;\n");
	/* rewrite output header */
	rewind(ofp);
	fprintf(ofp, "#NEXUS\n");
	today = today_date();
	if(today[Lenstr(today)-1]=='\n')
		today[Lenstr(today)-1] = '\0';

	fprintf(ofp,
	"[! RDP - the Ribsomal Database Project, (%s).]\n", today);

	fprintf(ofp,
	"[! To get started, send HELP to rdp@info.mcs.anl.gov ]\n");

	fprintf(ofp, "BEGIN DATA;\n   DIMENSIONS\n");
	fprintf(ofp,
		"      NTAX = %6d\n      NCHAR = %6d\n      ;\n",
			total_seq, maxsize);

	destroy_FILE_BUFFER(ifp); fclose(ofp);
}
/* ---------------------------------------------------------------
*	Function to_paup_1x1()
*		Convert from ALMA format to PAUP format,
*			one seq by one seq.
*/
void
to_paup_1x1(inf, outf, informat)
     char	*inf, *outf;
     int	informat;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	int	         maxsize, current, total_seq, first_line;
	char	     temp[TOKENNUM], eof;
	char	*name, *today;

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s, exit\n", inf);
		error(121, temp);
	}
    ifp              = create_FILE_BUFFER(inf, IFP);
	if(Lenstr(outf) <= 0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s, exit\n", outf);
		error(122, temp);
	}
	maxsize = 1; current = 0;
	name = NULL;
	paup_print_header(ofp);
	while(maxsize>current)	{
		init();
		FILE_BUFFER_rewind(ifp);
		total_seq = 0;
		first_line = 0;
		do	{	/* read in one sequence */
			init_paup();
			if(informat==ALMA)	{
				init_alma();
				eof=alma_in(ifp);
			} else if(informat==GENBANK)	{
				init_genbank();
				eof=genbank_in_locus(ifp);
			} else if(informat==EMBL||informat==PROTEIN) {
				init_embl();
				eof=embl_in_id(ifp);
			} else if(informat==MACKE)	{
				init_macke();
				eof=macke_in_name(ifp);
			} else error(127,
		"UNKNOW input format when converting to PAUP format.");

			if(eof==EOF) break;
			if(informat==ALMA)	{
				alma_key_word
				(data.alma.id, 0, temp, TOKENNUM);
			} else if(informat==GENBANK)	{
				genbank_key_word
				(data.gbk.locus, 0, temp, TOKENNUM);
			} else if(informat==EMBL||informat==PROTEIN) {
				embl_key_word
				(data.embl.id, 0, temp, TOKENNUM);
			} else if(informat==MACKE)	{
				macke_key_word
				(data.macke.name, 0, temp, TOKENNUM);
			} else error(70,
		"UNKNOW input format when converting to PAUP format.");

			Freespace(&name);
			name = Dupstr(temp);
			paup_verify_name(&name);

			if(data.seq_length>maxsize)
				maxsize = data.seq_length;

			if(current<data.seq_length) first_line++;

			paup_print_line(name, data.sequence, data.seq_length, current, (first_line==1), ofp);

			/* Avoid repeating */
			if(first_line==1) first_line++;

			total_seq++;

		} while(1);
		current += (SEQLINE-10);
		if(maxsize>current) fprintf(ofp, "\n");
	}	/* print block by block */

#ifdef log
	fprintf(stderr, "Total %d sequences have been processed\n", total_seq);
#endif

	fprintf(ofp, "      ;\nENDBLOCK;\n");
	/* rewrite output header */
	rewind(ofp);
	fprintf(ofp, "#NEXUS\n");
	today = today_date();
	if(today[Lenstr(today)-1]=='\n') today[Lenstr(today)-1] = '\0';

	fprintf(ofp,
	"[! RDP - the Ribsomal Database Project, (%s).]\n", today);

	fprintf(ofp,
	"[! To get started, send HELP to rdp@info.mcs.anl.gov ]\n");

	fprintf(ofp, "BEGIN DATA;\n   DIMENSIONS\n");
	fprintf(ofp,
	"      NTAX = %6d\n      NCHAR = %6d\n      ;\n",
		total_seq, maxsize);

	destroy_FILE_BUFFER(ifp); fclose(ofp);
}
/* -----------------------------------------------------------
*	Function paup_verify_name().
*		Verify short_id in PUAP format.
*/
void
paup_verify_name(string)
char	**string;
{
	int	indi, len, index;
	char	temp[TOKENNUM];
/* 	void	Freespace(); */

	for(indi=index=0,len=Lenstr((*string)); indi<len&&index==0;
	indi++)
		if((*string)[indi]=='*'||(*string)[indi]=='('
		||(*string)[indi]==')'||(*string)[indi]=='{'
		||(*string)[indi]=='/'||(*string)[indi]==','
		||(*string)[indi]==';'||(*string)[indi]=='_'
		||(*string)[indi]=='='||(*string)[indi]==':'
		||(*string)[indi]=='\\'||(*string)[indi]=='\'')
			index=1;

	if(index==0)	return;
	else	{
		temp[0]='\'';
		for(indi=0, index=1; indi<len; indi++, index++)	{
			temp[index]=(*string)[indi];
			if((*string)[indi]=='\'')
				temp[++index]='\'';
		}
		temp[index++]='\'';
		temp[index]='\0';
		Freespace(string);
		(*string)=(char*)Dupstr(temp);
	}
}
/* --------------------------------------------------------------
 *	Function paup_print_line().
 *		print paup file.
 */
void
paup_print_line(string, sequence, seq_length, index, first_line, fp)
     char *string, *sequence;
     int   seq_length;
     int   index, first_line;
     FILE *fp;
{
    int indi, indj, length;

    length = SEQLINE-10;

    fputs("      ", fp);
    /* truncate if length of seq ID is greater than 10 */
    for(indi=0; indi<10 && string[indi]; indi++) fputc(string[indi], fp);

    if (seq_length == 0) seq_length = Lenstr(sequence);

    if (index<seq_length) {
        for (; indi<11; indi++) fputc(' ', fp);

        for (indi=indj=0; indi<length; indi++)	{
            if ((index+indi)<seq_length) {
                fputc(sequence[index+indi], fp);
                indj++;
                if(indj==10 && indi<(length-1) && (indi+index)<(seq_length-1)) {
                    fputc(' ', fp);
                    indj = 0;
                }
            }
            else break;
        }
    }

	if(first_line)
		fprintf(fp, " [%d - %d]", index+1, (index+indi));

	fputc('\n', fp);
}
/* ----------------------------------------------------------
*	Function paup_print_header().
*		Print out the header of each paup format.
*/
void
paup_print_header(ofp)
FILE	*ofp;
{
	char	*today;

	fprintf(ofp, "#NEXUS\n");
	today = today_date();
	if(today[Lenstr(today)-1]=='\n')
		today[Lenstr(today)-1] = '\0';

	fprintf(ofp,
	"[! RDP - the Ribsomal Database Project, (%s).]\n",
		today);

	fprintf(ofp,
	"[! To get started, send HELP to rdp@info.mcs.anl.gov ]\n");

	fprintf(ofp, "BEGIN DATA;\n   DIMENSIONS\n");

	fprintf(ofp,
	"      NTAX =       \n      NCHAR =       \n      ;\n");

	fprintf(ofp, "   FORMAT\n      LABELPOS = LEFT\n");

	fprintf(ofp,
	"      MISSING = .\n      EQUATE = \"%s\"\n",
		data.paup.equate);

	fprintf(ofp,
"      INTERLEAVE\n      DATATYPE = RNA\n      GAP = %c\n      ;\n",
		data.paup.gap);

	fprintf(ofp,
	"   OPTIONS\n      GAPMODE = MISSING\n      ;\n   MATRIX\n");
}
