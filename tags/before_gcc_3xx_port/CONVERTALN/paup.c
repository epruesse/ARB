
#include <stdio.h>
#include "convert.h"
#include "global.h"

/* ------------------------------------------------------------
*	Function init_paup().
*		Init. paup data.
*/
void init_paup() {

	int	indi;
/* 	void	Freespace(); */

	for(indi=0; indi<data.paup.ntax; indi++)	{
		Freespace(&(data.ids[indi]));
		Freespace(&(data.seqs[indi]));
	}
	Freespace(&(data.ids));
	Freespace(&(data.seqs));
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
	FILE	*ifp, *ofp;
	int	maxsize, current, total_seq, first_line;
	int	out_of_memory, indi;
/* 	int	alma_key_word(); */
/* 	char	alma_in(), genbank_in_locus(); */
/* 	char	macke_in_name(), embl_in_id(); */
	char	temp[TOKENNUM], eof;
	char	*name, *today;
/* 	void	init(), init_paup(), init_seq_data(); */
/* 	void	init_alma(), init_genbank(); */
/* 	void	init_macke(), init_embl(); */
/* 	void	paup_print_line(), to_paup_1x1(); */
/* 	void	error(), Cpystr(); */
/* 	void	genbank_key_word(), embl_key_word(); */
/* 	void	paup_verify_name(), paup_print_header(); */

	if((ifp=fopen(inf, "r"))==NULL)	{
		sprintf(temp,
		"Cannot open input file %s, exit\n", inf);
		error(64, temp);
	}
	if(Lenstr(outf)<=0)	ofp = stdout;
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

		if(data.seq_length>maxsize)
			maxsize = data.seq_length;
		data.ids=(char**)Reallocspace((char *)data.ids,
			sizeof(char*)*total_seq);

		if(data.ids==NULL)	{ out_of_memory=1; break; }
		data.seqs=(char**)Reallocspace((char *)data.seqs,
			sizeof(char*)*total_seq);

		if(data.seqs==NULL)	{ out_of_memory=1; break; }

		data.ids[total_seq-1]=name;
		data.seqs[total_seq-1]=(char*)Dupstr(data.sequence);

	} while(!out_of_memory);

	if(out_of_memory)	{
		/* cannot hold all seqs into mem. */
		fprintf(stderr,
	"Rerun the conversion throught one seq. by one seq. base.\n");

		fclose(ifp); fclose(ofp);
		to_paup_1x1(inf, outf, informat);
		return;
	}
	current = 0;
	while(maxsize>current)	{
		first_line = 0;
		for(indi=0; indi<total_seq; indi++)	{
			if(current<Lenstr(data.seqs[indi]))
				first_line++;
			paup_print_line(data.ids[indi],
				data.seqs[indi], current,
			 	(first_line==1), ofp);

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

	fclose(ifp); fclose(ofp);
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
	FILE	*ifp, *ofp;
	int	maxsize, current, total_seq, first_line;
/* 	int	alma_key_word(); */
	char	temp[TOKENNUM], eof;
	char	*name, *today;

	if((ifp=fopen(inf, "r"))==NULL)	{
		sprintf(temp,
			"Cannot open input file %s, exit\n", inf);
		error(121, temp);
	}
	if(Lenstr(outf)<=0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp,
			"Cannot open output file %s, exit\n", outf);
		error(122, temp);
	}
	maxsize = 1; current = 0;
	name = NULL;
	paup_print_header(ofp);
	while(maxsize>current)	{
		init();
		rewind(ifp);
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

			paup_print_line(name, data.sequence, current,
				(first_line==1), ofp);

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

	fclose(ifp); fclose(ofp);
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
paup_print_line(string, sequence, index, first_line, fp)
char	*string, *sequence;
int	index, first_line;
FILE	*fp;
{
	int	indi, indj, bnum, length, seq_length;

	length = SEQLINE-10;
	fprintf(fp, "      ");
	if(Lenstr(string)>10)	{
		/* truncate if length of seq ID is greater than 10 */
		for(indi=0; indi<10; indi++)
			fprintf(fp, "%c", string[indi]);
		bnum = 1;
	} else	{
		fprintf(fp, "%s", string);
		bnum = 10 - Lenstr(string)+1;
	}
	/* Print out blanks after sequence ID to make up 10 chars. */
	seq_length = Lenstr(sequence);

	if(index<seq_length)
		for(indi=0; indi<bnum; indi++)
			fprintf(fp, " ");

	for(indi=indj=0; indi<length; indi++)	{
		if((index+indi)<seq_length)	{

			fprintf(fp, "%c", sequence[index+indi]);
			indj++;
			if(indj==10&&indi<(length-1)
			&&(indi+index)<(seq_length-1)) {
				fprintf(fp, " ");
				indj=0;
			}
		} else break;
	}

	if(first_line)
		fprintf(fp, " [%d - %d]", index+1, (index+indi));

	fprintf(fp, "\n");
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
