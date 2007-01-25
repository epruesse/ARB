#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "convert.h"
#include "global.h"

extern int	warning_out;

/* --------------------------------------------------------------
*	Function init_alma().
*		Init. ALMA data.
*/
void init_alma()	{
	Freespace(&(data.alma.id));
	Freespace(&(data.alma.filename));
	Freespace(&(data.alma.sequence));
	data.alma.format=UNKNOWN;
	data.alma.defgap='-';
	data.alma.num_of_sequence=0;
	/* init. nbrf too */
	Freespace(&(data.nbrf.id));
	Freespace(&(data.nbrf.description));
}
/* ----------------------------------------------------------------
*	Function alma_to_macke().
*		Convert from ALMA format to Macke format.
*/
void
alma_to_macke(inf, outf)
     char	*inf, *outf;
{
	FILE        *IFP, *ofp;
    FILE_BUFFER  ifp;
	char         temp[TOKENNUM];
	int	         indi, total_num;

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp,
                "Cannot open input file %s, EXIT.", inf);
		error(46, temp);
	}
    ifp              = create_FILE_BUFFER(inf, IFP);
	if(Lenstr(outf) <= 0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp,
                "Cannot open output file %s, EXIT.", outf);
		error(47, temp);
	}

	init();
	/* macke format seq irrelenvant header */
	macke_out_header(ofp);
	for(indi=0; indi<3; indi++)	{
		FILE_BUFFER_rewind(ifp);
		init_seq_data();
		init_macke();
		init_alma();
		while(alma_in(ifp)!=EOF)	{
			data.numofseq++;
			if(atom()) {
			/* convert from alma form to macke form */
				switch(indi)	{
				case 0:
				/* output seq display format */
					macke_out0(ofp, ALMA);
					break;
				case 1:
					/* output seq information */
					macke_out1(ofp);
					break;
				case 2:
					/* output seq data */
					macke_out2(ofp);
					break;
				default: ;
				}
			} else error(48,
			"Conversion from alma to macke fails, Exit.");
			init_alma();
			init_macke();
		}
		total_num = data.numofseq;
		if(indi==0)	{
			fprintf(ofp, "#-\n");
			/* no warning messages for next loop */
			warning_out = 0;
		}
	}	/* for each seq; loop */
	warning_out = 1;	/* resume warning messages */

#ifdef log
	fprintf(stderr,
		"Total %d sequences have been processed\n",
			total_num);
#endif
}
/* ----------------------------------------------------------------
*	Function alma_to_genbank().
*		Convert from ALMA format to GenBank format.
*/
void
alma_to_genbank(inf, outf)
     char	*inf, *outf;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	char	     temp[TOKENNUM];

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp,
                "Cannot open input file %s, EXIT.", inf);
		error(73, temp);
	}
    ifp              = create_FILE_BUFFER(inf, IFP);
	if(Lenstr(outf) <= 0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp,
                "Cannot open output file %s, EXIT.", outf);
		error(74, temp);
	}

	init();
	init_seq_data();
	init_macke();
	init_genbank();
	init_alma();
	while(alma_in(ifp)!=EOF)	{
		data.numofseq++;
		if(atom()&&mtog()) {
			genbank_out(ofp);
			init_alma();
			init_macke();
			init_genbank();
		}
	}	/* for each seq; loop */

#ifdef log
	fprintf(stderr,
		"Total %d sequences have been processed\n",
			data.numofseq);
#endif
}
/* ----------------------------------------------------------------
*	Function alma_in().
*		Read in one ALMA sequence data.
*/
char
alma_in(fp)
FILE_BUFFER fp;
{
	char	eoen, *eof, line[LINENUM];
/* 	char	temp[LINENUM]; */
	char	key[TOKENNUM], *format;
	int	indi, len, index;
/* 	int	alma_key_word(); */
/* 	void	alma_one_entry(), Freespace(), error(); */

	/* end-of-entry; set to be 'y' after reaching end of entry */
	format=NULL;
	eoen = ' ';
	for(eof=Fgetline(line, LINENUM, fp); eof!=NULL&&eoen!='y'; )
	{
		if(Lenstr(line)<=1)	{
			eof=Fgetline(line, LINENUM, fp);
			continue;	/* skip empty line */
		}
		index = alma_key_word(line, 0, key, TOKENNUM);
		eoen='n';
		if((Cmpstr(key, "NXT ENTRY"))==EQ)	{
		/* do nothing, just indicate beginning of entry */
		} else if((Cmpstr(key, "ENTRY ID"))==EQ)	{
			alma_one_entry(line, index, &(data.alma.id));
			for(indi=0, len=Lenstr(data.alma.id); indi<len;
			indi++)
				if(data.alma.id[indi]==' ')
					data.alma.id[indi]='_';
		} else if((Cmpstr(key, "SEQUENCE"))==EQ)	{
			alma_one_entry(line, index,
				&(data.alma.filename));
		} else if((Cmpstr(key, "FORMAT"))==EQ)	{
			alma_one_entry(line, index, &format);
			if((Cmpstr(format, "NBRF"))==EQ)
				data.alma.format=NBRF;
			else if((Cmpstr(format, "UWGCG"))==EQ
				||(Cmpstr(format, "GCG"))==EQ)
				data.alma.format=GCG;
			else if((Cmpstr(format, "EMBL"))==EQ)
				data.alma.format=EMBL;
			else if((Cmpstr(format, "STADEN"))==EQ)
				data.alma.format=STADEN;
			else	{
				sprintf(line,
		"Unidentified file format %s in ALMA format, Exit.",
				format);
				error(49, line);
			}
			Freespace(&(format));
		} else if((Cmpstr(key, "DEFGAP"))==EQ)	{
			/* skip white spaces */
			for(;line[index]!='['&&line[index]!='\n'
			&&line[index]!='\0'; index++) ;
			if(line[index]=='[')
				data.alma.defgap = line[index+1];
		} else if((Cmpstr(key, "GAPS"))==EQ)	{
			eof=alma_in_gaps(fp);
			eoen='y';
		} else if((Cmpstr(key, "END FILE"))==EQ)	{
			return(EOF);
		}
		if(eoen!='y') eof=Fgetline(line, LINENUM, fp);
	}
	if(eof==NULL) return(EOF);
	else return(EOF+1);
}
/* ----------------------------------------------------------------
*	Function alma_key_word().
*		Get the key_word from line beginning at index.
*/
int
alma_key_word(line, index, key, length)
char	*line;
int	index;
char	*key;
int	length;
{
	int	indi, indj;

	if(line==NULL)	{ key[0]='\0'; return(index); }
	for(indi=index, indj=0;
	(index-indi)<length&&line[indi]!='>'
	&&line[indi]!='\n'&&line[indi]!='\0';
	indi++, indj++)
		key[indj] = line[indi];
	key[indj] = '\0';
	if(line[indi]=='>')	return(indi+1);
	else return(indi);
}
/* --------------------------------------------------------------
*	Function alma_one_entry().
*		Read in one ALMA entry lines.
*/
void
alma_one_entry(line, index, datastring)
char	*line;
int	index;
char	**datastring;
{
	int	indi, length;
/* 	void	replace_entry(); */

	index = Skip_white_space(line, index);

	for(indi=index, length=Lenstr(line+index)+index;
	indi<length; indi++)
		if(line[indi]=='\n') line[indi]='\0';

	replace_entry(datastring, line+index);
}
/* -------------------------------------------------------------
*	Function alma_in_gaps().
*		Get sequence data and merge with gaps(alignment).
*/
char
*alma_in_gaps(fp)
FILE_BUFFER fp;
{
	int	return_num, gaps, residues, count=0;
	int	indi, numofseq, bases_not_matching;
	char	line[LINENUM], gap_chars[LINENUM];
	char	*eof;
/* 	void	alma_in_sequence(), warning(); */

	alma_in_sequence();

	numofseq = 0;

	bases_not_matching=0;
	/* alignment, merger with gaps information */
    {
        while (1) {
            char *gotLine = Fgetline(line, LINENUM, fp);
            if (!gotLine) break;

            return_num = sscanf(line, "%d %d %s", &gaps, &residues, gap_chars);
            if (return_num == 0 || gaps == -1) {
                FILE_BUFFER_back(fp, line); 
                break;
            }

            data.alma.sequence = (char*)Reallocspace(data.alma.sequence, (unsigned)(sizeof(char)*(numofseq+gaps+1)));

            for(indi=0; indi<gaps; indi++) data.alma.sequence[numofseq+indi] = gap_chars[1];

            numofseq += gaps;

            if(residues>(data.seq_length-count)) bases_not_matching = 1;

            data.alma.sequence = (char*)Reallocspace (data.alma.sequence, (unsigned)(sizeof(char)*(numofseq+residues+1)));

            /* fill with gap if seq data is not enough as required */
            for(indi=0; indi<residues; indi++) {
                if(count>=data.seq_length) data.alma.sequence[numofseq+indi] = gap_chars[1];
                else                       data.alma.sequence[numofseq+indi] = data.sequence[count++];
            }

            numofseq += residues;
                
        }
    }

	if(bases_not_matching)	{
		sprintf(line,
                "Bases number in ALMA file is larger than that in data file %s",
                data.alma.filename);

		warning(142, line);
	}

	if(count<data.seq_length)	{
		sprintf(line,
                "Bases number in ALMA file is less than that in data file %s",
                data.alma.filename);

		warning(143, line);

		/* Append the rest of the data at the end */
		data.alma.sequence = (char*)Reallocspace (data.alma.sequence, (unsigned)(sizeof(char)*(numofseq+data.seq_length-count+1)));

		for(indi=0; count<data.seq_length; indi++) data.alma.sequence[numofseq++] = data.sequence[count++];
	}
	data.alma.sequence[numofseq]='\0';

	/* update sequence data */
	if(numofseq>data.max)	{
		data.max=numofseq;
		data.sequence=(char*)Reallocspace(data.sequence, (unsigned)(sizeof(char)*data.max));
	}

	data.seq_length = numofseq;

	for(indi=0; indi<numofseq; indi++) data.sequence[indi]=data.alma.sequence[indi];

	data.alma.num_of_sequence = numofseq;

	if(return_num!=0) {
		/* skip END ENTRY> line */
		if(Fgetline(line, LINENUM, fp)==NULL) eof=NULL;
		else eof = line;
	} else eof = NULL;

	return(eof);
}
/* -------------------------------------------------------------
*	Function alma_in_sequence().
*		Read in sequence data.
*/
void
alma_in_sequence()	{

	char	     temp[LINENUM];
    FILE        *IFP; /* ex-infile */
    FILE_BUFFER  ifp;

	if((IFP=fopen(data.alma.filename, "r"))==NULL) {
		sprintf(temp, "Cannot open file %s, Exit.", data.alma.filename);
		error(51, temp);
	}

    ifp = create_FILE_BUFFER(data.alma.filename, IFP);

	if(data.alma.format==NBRF)	{
		nbrf_in(ifp);
	} else if(data.alma.format==GCG)	{
		gcg_in(ifp);
	} else if(data.alma.format==EMBL)	{
		init_embl();
		embl_in(ifp);
	} else if(data.alma.format==STADEN)	{
		staden_in(ifp);
	} else {
		sprintf(temp, "Unidentified file format %d in ALMA file, Exit.", data.alma.format);
		error(50, temp);
	}
	destroy_FILE_BUFFER(ifp);
}
/* --------------------------------------------------------------
*	Function nbrf_in().
*		Read in nbrf data.
*/
void
nbrf_in(fp)
FILE_BUFFER fp;
{
	int	length, index, reach_end;
	char	line[LINENUM], temp[TOKENNUM], *eof;
/* 	char	*Fgetline(), *Dupstr(); */
/* 	char	*Reallocspace(); */
/* 	void	Cpystr(), replace_entry(), error(); */

	if((eof=Fgetline(line, LINENUM, fp))==NULL)
		error(52,
		"Cannot find id line in NBRF file, Exit.");
	Cpystr(temp, line+4);
	length=Lenstr(temp);
	if(temp[length-1]=='\n') temp[length-1]='\0';

	replace_entry(&(data.nbrf.id), temp);

	if((eof=Fgetline(line, LINENUM, fp))==NULL)
		error(54,
		"Cannot find description line in NBRF file, Exit.");

	replace_entry(&(data.nbrf.description), line);

	/* read in sequence data */
	data.seq_length = 0;
	for(eof=Fgetline(line, LINENUM, fp), reach_end='n';
	eof!=NULL&&reach_end!='y';
	eof=Fgetline(line, LINENUM, fp))
	{
		for(index=0; line[index]!='\n'&&line[index]!='\0';
		index++)
		{
			if(line[index]=='*') {
				reach_end='y';
				continue;
			}
			if(line[index]!=' '&&data.seq_length>=data.max){
				data.max += 100;

				data.sequence = (char*)Reallocspace
				(data.sequence,
				(unsigned)(sizeof(char)*data.max));
			}
			if(line[index]!=' ')
				data.sequence[data.seq_length++]
					= line[index];
		}	/* scan through each line */
		data.sequence[data.seq_length] = '\0';
	}	/* for each line */
}
/* ----------------------------------------------------------------
*	Function gcg_in().
*		Read in one data entry in UWGCG format.
*/
void
gcg_in(fp)
     FILE_BUFFER	fp;
{
	char line[LINENUM];

    while (Fgetline(line, LINENUM, fp)) {
        char *two_dots = strstr(line, "..");
        if (two_dots) {
            FILE_BUFFER_back(fp, two_dots+2);
            genbank_origin(line, fp);
        }
    }
}
/* ----------------------------------------------------------------
*	Fnction staden_in().
*		Read in sequence data in STADEN format.
*/
void
staden_in(fp)
FILE_BUFFER fp;
{
	char	line[LINENUM];
	int	len, start, indi;

	data.seq_length=0;
	while(Fgetline(line, LINENUM, fp)!=NULL)	{
		/* skip empty line */
		if((len=Lenstr(line))<=1) continue;

		start = data.seq_length;
		data.seq_length += len;
		line[len-1]='\0';

		if(data.seq_length>data.max)	{
			data.max += (data.seq_length + 100);
			data.sequence = (char*)Reallocspace
				(data.sequence,
				(unsigned)(sizeof(char)*data.max));
		}

		for(indi=0; indi<len; indi++)
			data.sequence[start+indi] = line[indi];
	}
}
/* -------------------------------------------------------------
*	Function atom().
*		Convert one ALMA entry to Macke entry.
*/
int
atom()
{
/* 	int	indi, len; */
/* 	char	*Dupstr(), *Reallocspace(); */
/* 	void	replace_entry(), error(); */

	if(data.alma.format==NBRF)	{
		data.macke.numofrem=1;
		data.macke.remarks=(char**)Reallocspace
			((char*)data.macke.remarks,
			(unsigned)(sizeof(char*)));
		data.macke.remarks[0]
			=(char*)Dupstr(data.nbrf.description);
	} else if(data.alma.format==EMBL)	{
		etom();
	} else if(data.alma.format==GCG)	{
	} else if(data.alma.format==STADEN)	{
	} else {
		error(53,
		"Unidentified format type in ALMA file, Exit.");
	}

	replace_entry(&(data.macke.seqabbr), data.alma.id);

	return(1);
}
/* -------------------------------------------------------------
*	Function embl_to_alma().
*		Convert from EMBL to ALMA.
*/
void
embl_to_alma(inf, outf)
     char	*inf, *outf;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	char	     temp[TOKENNUM];

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s, EXIT.", inf);
		error(134, temp);
	}
    ifp = create_FILE_BUFFER(inf, IFP);
	if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s, EXIT.", outf);
		error(135, temp);
	}

	init();
	init_embl();
	init_alma();
	alma_out_header(ofp);

	while(embl_in(ifp)!=EOF)	{
		if(data.numofseq>0) fprintf(ofp, "\n");
		data.numofseq++;
		if(etoa()) 	{
			FILE *outfile = alma_out(ofp, EMBL);
			embl_out(outfile);
			fclose(outfile);
		}
		init_embl();
		init_alma();
	}

	fprintf(ofp, "END FILE>\n");

#ifdef log
	fprintf(stderr,
		"Total %d sequences have been processed\n",
			data.numofseq);
#endif

	destroy_FILE_BUFFER(ifp);	fclose(ofp);
}
/* ------------------------------------------------------------
*	Function genbank_to_alma().
*		Convert from GenBank to ALMA.
*/
void
genbank_to_alma(inf, outf)
     char	*inf, *outf;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	char	     temp[TOKENNUM];

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp,
                "Cannot open input file %s, EXIT.", inf);
		error(61, temp);
	}
    ifp = create_FILE_BUFFER(inf, IFP);
	if((ofp=fopen(outf, "w")) == NULL)	{
		sprintf(temp,
                "Cannot open output file %s, EXIT.", outf);
		error(62, temp);
	}

	init();
	init_genbank();
	init_embl();
	init_alma();
	alma_out_header(ofp);

	while(genbank_in(ifp)!=EOF)	{
		if(data.numofseq>0) fprintf(ofp, "\n");
		data.numofseq++;
		if(gtoe()&&etoa()) 	{
			FILE *outfile = alma_out(ofp, EMBL);
			embl_out(outfile);
			fclose(outfile);
		}
		init_genbank();
		init_embl();
		init_alma();
	}

	fprintf(ofp, "END FILE>\n");

#ifdef log
	fprintf(stderr,
		"Total %d sequences have been processed\n",
			data.numofseq);
#endif

	destroy_FILE_BUFFER(ifp);	fclose(ofp);
}
/* -------------------------------------------------------------
*	Function macke_to_alma().
*		Convert from MACKE to ALMA.
*/
void
macke_to_alma(inf, outf)
     char	*inf, *outf;
{
	FILE	    *IFP1, *IFP2, *IFP3, *ofp;
    FILE_BUFFER  ifp1, ifp2, ifp3;
	char	     temp[TOKENNUM];

	if((IFP1=fopen(inf, "r"))==NULL ||
       (IFP2=fopen(inf, "r"))==NULL ||
       (IFP3=fopen(inf, "r"))==NULL)
    {
		sprintf(temp,
		"Cannot open input file %s, EXIT.", inf);
		error(59, temp);
	}

    ifp1 = create_FILE_BUFFER(inf, IFP1);
    ifp2 = create_FILE_BUFFER(inf, IFP2);
    ifp3 = create_FILE_BUFFER(inf, IFP3);

	if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp,
		"Cannot open output file %s, EXIT.", outf);
		error(60, temp);
	}
    
	init();
	init_macke();
	init_genbank();
	init_embl();
	init_alma();
	alma_out_header(ofp);
	while(macke_in(ifp1, ifp2, ifp3)!=EOF)	{
		if(data.numofseq>0) fprintf(ofp, "\n");
		data.numofseq++;
		if(mtog()&&gtoe()&&partial_mtoe()&&etoa()) {
			FILE *outfile = alma_out(ofp, EMBL);
			embl_out(outfile);
			fclose(outfile);
		}
		init_macke();
		init_genbank();
		init_embl();
		init_alma();
	}

	fprintf(ofp, "END FILE>\n");

#ifdef log
	fprintf(stderr,
		"Total %d sequences have been processed\n",
			data.numofseq);
#endif

	destroy_FILE_BUFFER(ifp1);	destroy_FILE_BUFFER(ifp2);
	destroy_FILE_BUFFER(ifp3);	fclose(ofp);
}
/* -------------------------------------------------------------
*	Function etoa().
*		Convert from EMBL to ALMA format.
*/
int
etoa()
{
	char	temp[TOKENNUM], t1[TOKENNUM], t2[TOKENNUM], t3[TOKENNUM];
/* 	char	*Dupstr(), *Catstr(); */
/* 	void	embl_key_word(), Cpystr(); */
/* 	void	replace_entry(); */

	embl_key_word(data.embl.id, 0, temp, TOKENNUM);
	if(Lenstr(data.embl.dr)>1)      {
		/* get short_id from DR line if there is RDP def. */
		Cpystr(t3, "dummy");
		sscanf(data.embl.dr, "%s %s %s", t1, t2, t3);
		if(Cmpstr(t1, "RDP;")==EQ)      {
			if(Cmpstr(t3, "dummy")!=EQ)     {
				Cpystr(temp, t3);
			} else Cpystr(temp, t2);
				temp[Lenstr(temp)-1]='\0'; /* remove '.' */
		}
	}
	replace_entry(&(data.alma.id), temp);

	return(1);
}
/* -------------------------------------------------------------
*	Function alma_out_header().
*		Output alma format header.
*/
void
alma_out_header(fp)
FILE	*fp;
{
	fprintf(fp, "SCREEN WIDTH>   80\n");
	fprintf(fp, "ID COLUMN>    1\n");
	fprintf(fp, "LINK COLUMN>   10\n");
	fprintf(fp, "INFO LINE>   24\n");
	fprintf(fp, "ACTIVE WINDOW>    1    1\n");
	fprintf(fp,
	"SEQUENCE WINDOW>    1    1    1   23   12   80\n");
	fprintf(fp, "SCROLL DISTANCE>    1    1   50\n");
	fprintf(fp, "SEQ NUMBERS>    1    1    0    0\n");
	fprintf(fp, "CURSOR SCREEN>    1    1   11   54\n");
	fprintf(fp, "CURSOR ALIGNMENT>    1    1    9  143\n");
	fprintf(fp, "LINK ACTIVE>    2\n");
	fprintf(fp, "SCROLL>    2\n");
	fprintf(fp, "MULTI WINDOWS>F\n");
	fprintf(fp, "COLOURS>F\n");
	fprintf(fp, "UPDATE INT#>T\n");
	fprintf(fp, "HOMOLOGY>F\n");
	fprintf(fp, "PRINT>\n");
	fprintf(fp, "FILE_A>\n");
	fprintf(fp, "FILE_B>\n");
	fprintf(fp, "FILE_C>\n");
	fprintf(fp, "FILE_D>\n");
	fprintf(fp, "FILE_E>\n");
	fprintf(fp, "ALIGNMENT TITLE>\n");
	fprintf(fp, "PRINT TITLE>    0\n");
	fprintf(fp, "PRINT DEVICE>    0\n");
	fprintf(fp, "SPACERS TOP>    0\n");
	fprintf(fp, "FONT>    0\n");
	fprintf(fp, "FONT SIZE>    0\n");
	fprintf(fp, "PAGE WIDTH>    0\n");
	fprintf(fp, "PRINT FORMAT>    0\n");
	fprintf(fp, "PAGE LENGTH>    0\n");
	fprintf(fp, "SKIP LINES>    0\n");
	fprintf(fp, "COLOURING>    0\n");
	fprintf(fp,
"COLOUR ACTIVE>    0    0    0    0    0    0    0    0    0    0\n");
	fprintf(fp, "RESIDUE B(LINK)>\n");
	fprintf(fp, "RESIDUE R(EVERSE)>\n");
	fprintf(fp, "RESIDUE I(NCREASE)>\n");
	fprintf(fp, "RESIDUE B+R>\n");
	fprintf(fp, "RESIDUE B+I>\n");
	fprintf(fp, "RESIDUE R+I>\n");
	fprintf(fp, "RESIDUE B+R+I>\n");
	fprintf(fp, "HOMOEF_A>    0\n");
	fprintf(fp, "HOMOEF_B>    0\n");
	fprintf(fp, "HOMOEF_C>    0\n");
	fprintf(fp, "HOMOEF_D>    0\n");
	fprintf(fp, "HOMOEF_E>    0\n");
	fprintf(fp, "HOMOEF_F>    0\n");
	fprintf(fp, "HOMOEF_G>    0\n");
	fprintf(fp, "HOMOEF_H>    0\n");
	fprintf(fp, "HOMOEF_I>    0\n");
	fprintf(fp, "HOMOEF_J>    0\n");
	fprintf(fp, "HOMOEF_K>    0\n");
	fprintf(fp, "HOMOEF_L>    0\n");
	fprintf(fp, "HOMOEF_M>    0\n");
	fprintf(fp, "HOMOEF_N>    0\n");
	fprintf(fp, "HOMOEF_O>    0\n");
	fprintf(fp, "HOMOEF_P>    0\n");
	fprintf(fp, "HOMOEF_Q>    0\n");
	fprintf(fp, "HOMOEF_R>    0\n");
	fprintf(fp, "HOMOEF_S>    0\n");
	fprintf(fp, "HOMOEF_T>    0\n");
}
/* -------------------------------------------------------------
*	Function alma_out().
*		Output one alma entry.
*/
FILE *
alma_out(fp, format)
     FILE	*fp;
     int	format;
{
	int	  indi, len;
	char  filename[TOKENNUM];
    FILE *outfile;

	Cpystr(filename, data.alma.id);
	for(indi=0, len=Lenstr(filename); indi<len; indi++)
		if(!isalnum(filename[indi])) filename[indi] = '_';
	Catstr(filename, ".EMBL");

	outfile = alma_out_entry_header(fp, data.alma.id, filename, format);
	alma_out_gaps(fp);
    
    return outfile;
}
/* --------------------------------------------------------------
 *	Function alma_out_entry_header().
 *		Output one ALMA entry header.
*/
FILE *
alma_out_entry_header(fp, entry_id, filename, format_type)
FILE	*fp;
char	*entry_id, *filename;
int	format_type;
{
	char  temp[TOKENNUM];
    FILE *outfile = 0;

	fprintf(fp, "NXT ENTRY>S\n");
	fprintf(fp, "ENTRY ID>%s\n", entry_id);

	if(fopen(filename, "r")!=NULL)	{
		sprintf(temp,
		"file %s is overwritten.", filename);
		warning(55, temp);
	}
	if((outfile=fopen(filename, "w"))==NULL)	{
		sprintf(temp, "Cannot open file: %s, Exit.",
		filename);
		error(56, temp);
	}
	fprintf(fp, "SEQUENCE>%s\n", filename);
	if(format_type==EMBL)
		fprintf(fp, "FORMAT>EMBL\n");
	else if(format_type==NBRF)
		fprintf(fp, "FORMAT>NBRF\n");
	else if(format_type==GCG)
		fprintf(fp, "FORMAT>UWGCG\n");
	else if(format_type==STADEN)
		fprintf(fp, "FORMAT>STADEN\n");
	else	error(57,
		"Unknown format type when writing ALMA format, EXIT.");

	fprintf(fp, "ACCEPT>ALL\n");
	fprintf(fp, "DEFGAP>[-]\n");
	fprintf(fp, "PARAMS>1\n");
	fprintf(fp, "GAPS>\n");

    return outfile;
}
/* --------------------------------------------------------------
 *	Function alma_out_gaps().
*		Output gaps information of one ALMA entry.
*/
void
alma_out_gaps(fp)
FILE	*fp;
{
	int	indi, index, residue, gnum, rnum, tempcount;

	gnum=rnum=0;

	if(data.sequence[0]=='.'||data.sequence[0]=='-'
	||data.sequence[0]=='~')
		residue=0;
	else residue=1;

	tempcount=data.seq_length;
	for(indi=index=0; indi<tempcount; indi++)	{
		if(data.sequence[indi]=='.'
		||data.sequence[indi]=='-'
		||data.sequence[indi]=='~')	{
			if(residue)	{
				fprintf(fp, " %4d %4d [-]\n",
					gnum, rnum);
				gnum=rnum=residue=0;
			}
			gnum++; data.seq_length--;
		} else	{
			residue=1;
			rnum++;
			data.sequence[index++]
				=data.sequence[indi];
		}
	}
	data.sequence[index]='\0';
	fprintf(fp, " %4d %4d [-]\n", gnum, rnum);
	fprintf(fp, "   -1   -1 [-]\n");
	fprintf(fp, "END ENTRY>\n");
}
