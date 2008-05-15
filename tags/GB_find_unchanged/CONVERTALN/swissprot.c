
#include <stdio.h>
#include "convert.h"
#include "global.h"
/* ---------------------------------------------------------- */
/*	Function init_pm_data().
/*		Init macke and swissprot data.
*/
void
init_pm_data()	{
	void	init_macke(), init_protein();

	init_macke();
	init_protein();
}
/* ------------------------------------------------------------ */
/*	Function init_protein().
/*		Initialize protein entry.
*/
void
init_protein()	{

	int	indi;
	void	Freespace();
	char	*Dupstr();

	/* initialize protein format */
	Freespace(&(data.protein.id));
	Freespace(&(data.protein.date));
	Freespace(&(data.protein.definition));
	Freespace(&(data.protein.formname));
	Freespace(&(data.protein.accession));
	Freespace(&(data.protein.keywords));
	for(indi=0; indi<data.protein.numofref; indi++)	{
		Freespace(&(data.protein.reference[indi].author));
		Freespace(&(data.protein.reference[indi].title));
		Freespace(&(data.protein.reference[indi].journal));
		Freespace(&(data.protein.reference[indi].processing));
	}
	Freespace(&(data.protein.reference));
	Freespace(&(data.protein.comments));
	data.protein.id=Dupstr("\n");
	data.protein.date=Dupstr("\n");
	data.protein.definition=Dupstr("\n");
	data.protein.formname=Dupstr("\n");
	data.protein.accession=Dupstr("\n");
	data.protein.keywords=Dupstr("\n");
	data.protein.numofref=0;
	data.protein.reference=NULL;
	data.protein.comments=Dupstr("");
}
/* ---------------------------------------------------------- */
/*	Function protein_to_macke().
/*	Convert from Protein format to Macke format.
*/
void
protein_to_macke(inf, outf)
char	*inf, *outf;
{
	FILE	*ifp, *ofp, *fopen();
	char	temp[TOKENNUM], protein_in(); 
	void	init(), init_pm_data(), init_seq_data(); 
	void	macke_out_header(), macke_out0(), macke_out1(), macke_out2();
	void	error();
	int	indi, ptom(), total_num;

	if((ifp=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s, exit\n", inf);
		error(93, temp);
	}
	if(Lenstr(outf)<=0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s, exit\n", outf);
		error(95, temp);
	}

	/* seq irelenvant header */
	init();
	macke_out_header(ofp);
	for(indi=0; indi<3; indi++)	{
		FILE_BUFFER_rewind(ifp);
		init_seq_data();
		init_pm_data();
		while(protein_in(ifp)!=EOF)	{
			data.numofseq++;
			if(ptom()) { 
			/* convert from protein form to macke form */
				switch(indi)	{
				case 0:
					/* output seq display format */
					macke_out0(ofp, PROTEIN);
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
			} else error(82, 
				"Conversion from protein to macke fails, Exit");
			init_pm_data();
		}
		total_num = data.numofseq;
		if(indi==0)	fprintf(ofp, "#-\n");
	}	/* for each seq; loop */

#ifdef log
fprintf(stderr, "Total %d sequences have been processed\n", total_num);
#endif

}
/* --------------------------------------------------------------- */
/*	Function protein_in().
/*		Read in one protein entry.
*/
char
protein_in(fp)
FILE	*fp; 
{
	char	line[LINENUM], key[TOKENNUM], temp[LINENUM];
	char	*Fgetline(), *eof, eoen;
	char	*protein_id(), *protein_definition(); 
	char	*protein_accession(), *protein_date(), *protein_source(); 
	char	*protein_keywords(), *protein_reference(); 
	char	*protein_author(), *protein_title(), *protein_version();
	char	*protein_processing();
	char	*protein_comments(), *protein_origin(); 
	char	*protein_skip_unidentified();
	void	protein_key_word(), warning(), error();
	int	Lenstr();

	eoen=' '; /* end-of-entry, set to be 'y' after '//' is read */
	for(eof=Fgetline(line, LINENUM, fp); eof!=NULL&&eoen!='y'; ) {
		if(Lenstr(line)<=1) {
			eof=Fgetline(line, LINENUM, fp); 
			continue;	/* empty line, skip */
		}
		protein_key_word(line, 0, key, TOKENNUM);
		eoen='n';
		if((Cmpstr(key, "ID"))==EQ)	{
			eof = protein_id(line, fp);
		} else if((Cmpstr(key, "DT"))==EQ)	{
			eof = protein_date(line, fp);
		} else if((Cmpstr(key, "DE"))==EQ)	{
			eof = protein_definition(line, fp);
		} else if((Cmpstr(key, "OS"))==EQ)	{
			eof = protein_source(line, fp);
		} else if((Cmpstr(key, "AC"))==EQ)	{
			eof = protein_accession(line, fp);
		} else if((Cmpstr(key, "KW"))==EQ)	{
			eof = protein_keywords(line, fp);
		} else if((Cmpstr(key, "RA"))==EQ)	{
			eof = protein_author(line, fp);
		} else if((Cmpstr(key, "RP"))==EQ)	{
			eof = protein_processing(line, fp);
		} else if((Cmpstr(key, "RT"))==EQ)	{
			eof = protein_title(line, fp);
		} else if((Cmpstr(key, "RL"))==EQ)	{
			eof = protein_reference(line, fp);
		} else if((Cmpstr(key, "RN"))==EQ)	{
			eof = protein_version(line, fp);
		} else if((Cmpstr(key, "CC"))==EQ)	{
			eof = protein_comments(line, fp);
		} else if((Cmpstr(key, "SQ"))==EQ)		{
			eof = protein_origin(line, fp);
			eoen = 'y';
		} else {	/* unidentified key word */
			eof = protein_skip_unidentified(key, line, fp);
		}
		/* except "SQ", at the end of all the other cases, a
		/* new line has already read in, so no further read is 
		/* necessary*/
	}	/* for loop to read an entry line by line */

	if(eoen=='n') 
		error(42, "Reach EOF before one entry is read, Exit");
	
	if(eof==NULL)	return(EOF);
	else	return(EOF+1);

}
/* --------------------------------------------------------------- */
/*	Function protein_in_id().
/*		Read in one protein entry with id and seq only.
*/
char
protein_in_id(fp)
FILE	*fp; 
{
	char	line[LINENUM], key[TOKENNUM], temp[LINENUM];
	char	*Fgetline(), *eof, eoen;
	char	*protein_id(), *protein_origin(); 
	char	*protein_skip_unidentified();
	void	protein_key_word(), warning(), error();
	int	Lenstr();

	eoen=' '; /* end-of-entry, set to be 'y' after '//' is read */
	for(eof=Fgetline(line, LINENUM, fp); eof!=NULL&&eoen!='y'; ) {
		if(Lenstr(line)<=1) {
			eof=Fgetline(line, LINENUM, fp); 
			continue;	/* empty line, skip */
		}
		protein_key_word(line, 0, key, TOKENNUM);
		eoen='n';
		if((Cmpstr(key, "ID"))==EQ)	{
			eof = protein_id(line, fp);
		} else if((Cmpstr(key, "SQ"))==EQ)		{
			eof = protein_origin(line, fp);
			eoen = 'y';
		} else {	/* unidentified key word */
			eof = protein_skip_unidentified(key, line, fp);
		}
		/* except "SQ", at the end of all the other cases, a
		/* new line has already read in, so no further read is 
		/* necessary*/
	}	/* for loop to read an entry line by line */

	if(eoen=='n') 
		error(87, "Reach EOF before one entry is read, Exit");
	
	if(eof==NULL)	return(EOF);
	else	return(EOF+1);

}
/* ---------------------------------------------------------------- */
/*	Function protein_key_word().
/*		Get the key_word from line beginning at index.
*/
void
protein_key_word(line, index, key, length)
char	*line; 
int	index;
char	*key;
int	length;
{
	int	indi, indj;

	if(line==NULL)	{	key[0]='\0';	return;	}
	for(indi=index, indj=0; (index=indi)<length&&line[indi]!=' '
	&&line[indi]!='\t'&&line[indi]!='\n'&&line[indi]!='\0'; 
	indi++, indj++)	
		key[indj] = line[indi];
	key[indj] = '\0';
}
/* ------------------------------------------------------------ */
/*	Function protein_chcek_blanks().
/*		Check if there is (numb) blanks at beginning of line.
*/
int
protein_check_blanks(line, numb)
char	*line;
int	numb;
{
	int	blank=1, indi, indk;
	
	for(indi=0; blank&&indi<numb; indi++) {
		if(line[indi]!=' '&&line[indi]!='\t') blank=0;
		if(line[indi]=='\t') { 
			indk=indi/8+1; indi=8*indk+1; 
		}
	}

	return(blank);
}
/* ---------------------------------------------------------------- */
/*	Function protein_continue_line().
/*		if there are (numb) blanks at the beginning of line,
/*			it is a continue line of the current command.
*/
char
*protein_continue_line(pattern, string, line, fp)
char	*pattern, **string, *line;
FILE	*fp;
{
	int	Lenstr(), Cmpstr(), len, ind; 
	int	protein_check_blanks(), Skip_white_space();
	char	key[TOKENNUM], *eof, temp[LINENUM], *Catstr(); 
	char	*Fgetline();
	void	Cpystr(), protein_key_word(), Append_rp_eoln();

	/* check continue lines */
	for(eof=Fgetline(line, LINENUM, fp); 
	eof!=NULL; eof=Fgetline(line, LINENUM, fp))	{
		if(Lenstr(line)<=1)	continue;
		protein_key_word(line, 0, key, TOKENNUM);
		if(Cmpstr(pattern, key)!=EQ)	break;
		ind=Skip_white_space(line, p_nonkey_start);
		Cpystr(temp, (line+ind));
		Append_rp_eoln(string, temp); 
	}	/* end of continue line checking */
	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_id().
/*		Read in protein ID lines.
*/
char
*protein_id(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space(), Lenstr();
	char	*eof, *protein_continue_line(), *Dupstr();
	void	error(), Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	Freespace(&(data.protein.id));
	data.protein.id = Dupstr(line+index);
	eof = (char*)protein_continue_line("ID", &(data.protein.id), line, fp);

	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_date().
/*		Read in protein DATE lines.
*/
char
*protein_date(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space(), Lenstr();
	char	*eof, *protein_continue_line(), *Dupstr(), *dummy;
	void	error(), Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	Freespace(&(data.protein.date));
	data.protein.date = Dupstr(line+index);
	dummy = Dupstr(" ");
	eof = (char*)protein_continue_line("DT", &(dummy), line, fp);
	Freespace(&dummy);

	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_source().
/*		Read in protein DE lines.
*/
char
*protein_source(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space();
	char	*eof, *protein_continue_line(), *Dupstr();
	void	Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	Freespace(&(data.protein.formname));
	data.protein.formname = Dupstr(line+index);
	eof = protein_continue_line("OS", &(data.protein.formname), 
		line, fp);

	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_definition().
/*		Read in protein DE lines.
*/
char
*protein_definition(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space();
	char	*eof, *protein_continue_line(), *Dupstr();
	void	Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	Freespace(&(data.protein.definition));
	data.protein.definition = Dupstr(line+index);
	eof = protein_continue_line("DE", &(data.protein.definition), line, fp);

	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_accession().
/*		Read in protein ACCESSION lines.
*/
char
*protein_accession(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space();
	char	*eof, *protein_continue_line(), *Dupstr();
	void	Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	Freespace(&(data.protein.accession));
	data.protein.accession = Dupstr(line+index);
	eof = protein_continue_line("AC", &(data.protein.accession), line, fp);

	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_processing().
/*		Read in protein RP lines.
*/
char
*protein_processing(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space();
	char	*eof, *protein_continue_line(), *Dupstr();
	void	Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	Freespace(&(data.protein.reference[data.protein.numofref-1].processing));	
	data.protein.reference[data.protein.numofref-1].processing = Dupstr(line+index);
	eof = (char*)protein_continue_line("RP", 
		&(data.protein.reference[data.protein.numofref-1].processing), line, fp);

	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_keywords().
/*		Read in protein KEYWORDS lines.
*/
char
*protein_keywords(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space();
	char	*eof, *protein_continue_line(), *Dupstr();
	void	Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	Freespace(&(data.protein.keywords));
	data.protein.keywords = Dupstr(line+index);
	eof = (char*)protein_continue_line("KW", &(data.protein.keywords), 
		line, fp);

	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_author().
/*		Read in protein RL lines.
*/
char
*protein_author(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space();
	char	*eof, *protein_continue_line(), *Dupstr();
	void	Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	Freespace(&(data.protein.reference[data.protein.numofref-1].author));	
	data.protein.reference[data.protein.numofref-1].author = Dupstr(line+index);
	eof = (char*)protein_continue_line("RA", 
		&(data.protein.reference[data.protein.numofref-1].author), line, fp);

	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_title().
/*		Read in protein RT lines.
*/
char
*protein_title(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space();
	char	*eof, *protein_continue_line(), *Dupstr();
	void	Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	Freespace(&(data.protein.reference[data.protein.numofref-1].title));	
	data.protein.reference[data.protein.numofref-1].title = Dupstr(line+index);
	eof = (char*)protein_continue_line("RT", 
		&(data.protein.reference[data.protein.numofref-1].title), line, fp);

	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_reference().
/*		Read in protein RL lines.
*/
char
*protein_reference(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space();
	char	*eof, *protein_continue_line(), *Dupstr();
	void	Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	Freespace(&(data.protein.reference[data.protein.numofref-1].journal));	
	data.protein.reference[data.protein.numofref-1].journal = Dupstr(line+index);
	eof = (char*)protein_continue_line("RL", 
		&(data.protein.reference[data.protein.numofref-1].journal), line, fp);

	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_version().
/*		Read in protein RN lines.
*/
char
*protein_version(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space();
	char	*eof, *protein_continue_line(), *Dupstr();
	char	*Reallocspace(), *Fgetline();
	void	Freespace();

	index = Skip_white_space(line, p_nonkey_start);
	if(data.protein.numofref==0)	{
		data.protein.numofref++;
		data.protein.reference = (Emblref*)calloc(1,sizeof(Emblref)*1);
		data.protein.reference[0].author = Dupstr("");
		data.protein.reference[0].title = Dupstr("");
		data.protein.reference[0].journal = Dupstr("");
		data.protein.reference[0].processing = Dupstr("");
	} else {
		data.protein.numofref++;
		data.protein.reference = (Emblref*)Reallocspace(data.protein.reference,
			sizeof(Emblref)*(data.protein.numofref));
		data.protein.reference[data.protein.numofref-1].author = Dupstr("");
		data.protein.reference[data.protein.numofref-1].title = Dupstr("");
		data.protein.reference[data.protein.numofref-1].journal = Dupstr("");
		data.protein.reference[data.protein.numofref-1].processing = Dupstr("");
	}
	eof=Fgetline(line, LINENUM, fp);
	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_comments().
/*		Read in protein comment lines.
*/
char
*protein_comments(line, fp)
char	*line;
FILE	*fp;
{
	int	index, Skip_white_space(), len, Lenstr();
	char	*eof, *Fgetline(), *protein_continue_line(), *Dupstr();
	void	Freespace(), Append();

	if(Lenstr(data.protein.comments)<=1)
		Freespace(&(data.protein.comments));
	for(; line[0]='C'&&line[1]=='C'; eof=Fgetline(line, LINENUM, fp))
		Append(&(data.protein.comments), line+5);
	return(eof);
}
/* ---------------------------------------------------------------- */
/*	Function protein_skip_unidentified().
/*		if there are (numb) blanks at the beginning of line,
/*			it is a continue line of the current command.
*/
char
*protein_skip_unidentified(pattern, line, fp)
char	*pattern, *line;
FILE	*fp;
{
	int	Lenstr(), Cmpstr(); 
	char	*Fgetline(), *eof;
	char	key[TOKENNUM];
	void	protein_key_word();

	/* check continue lines */
	for(eof=Fgetline(line, LINENUM, fp); 
	eof!=NULL; eof=Fgetline(line, LINENUM, fp))	{
		protein_key_word(line, 0, key, TOKENNUM);
		if(Cmpstr(key, pattern)!=EQ) break;
	}	/* end of continue line checking */
	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function protein_origin().
/*		Read in protein sequence data.
*/
char
*protein_origin(line, fp)
char	*line;
FILE	*fp;
{
	char	*Fgetline(), *eof, *Reallocspace();
	int	index;

	data.seq_length = 0;
	/* read in whole sequence data */
	for(eof=Fgetline(line, LINENUM, fp); 
	eof!=NULL&&line[0]!='/'&&line[1]!='/'; 
	eof=Fgetline(line, LINENUM, fp)) 
	{
		for(index=5; line[index]!='\n'&&line[index]!='\0'; 
		index++)	{
			if(line[index]!=' '&&data.seq_length>=data.max) 	{
				data.max += 100;
				data.sequence = (char*)Reallocspace(
					data.sequence,
				(unsigned)(sizeof(char)*data.max));
			}
			if(line[index]!=' ')
				data.sequence[data.seq_length++] 
					= line[index];
		}
		data.sequence[data.seq_length] = '\0';
	}
	return(eof);
}
/* -------------------------------------------------------------- */
/*	Function ptom().
/*		Convert from Protein format to Macke format.
*/
int
ptom()	{
	void	protein_key_word(), error();
	int	Lenstr(), indj, indk, remnum;
	char	temp[LONGTEXT], *Dupstr(), *Reallocspace();
	void	Freespace();

	/* copy seq abbr, assume every entry in protein must end with \n\0 */
	/* no '\n' at the end of the string */
	protein_key_word(data.protein.id, 0, temp, TOKENNUM);
	Freespace(&(data.macke.seqabbr));
	data.macke.seqabbr = Dupstr(temp);
	/* copy name */
	Freespace(&(data.macke.name));
	data.macke.name = Dupstr(data.protein.formname);
	/* copy date---DD-MMM-YYYY\n\0  */
	Freespace(&(data.macke.date));
	data.macke.date = Dupstr(data.protein.date);
	/* copy protein entry (accession has higher priority) */	
	if(Lenstr(data.protein.accession)>1)	{
		Freespace(&(data.macke.acs));
		data.macke.acs = Dupstr(data.protein.accession); 
	}
	if(data.protein.numofref>0)	{	
		if(Lenstr(data.protein.reference[0].journal)>1)	{
			Freespace(&(data.macke.journal));
			data.macke.journal = Dupstr(data.protein.reference[0].journal);
		}
		if(Lenstr(data.protein.reference[0].title)>1)	{
			Freespace(&(data.macke.title));
			data.macke.title = Dupstr(data.protein.reference[0].title);
		}
		if(Lenstr(data.protein.reference[0].author)>1)	{
			Freespace(&(data.macke.author));
			data.macke.author = Dupstr(data.protein.reference[0].author);
		}
	}
	/* the rest of data are put into remarks, rem:..... */
	remnum=0;
	for(indj=1; indj<data.protein.numofref; indj++)	{
		if(Lenstr(data.protein.reference[indj].journal)>1) {
		sprintf(temp, "jour:%s", data.protein.reference[indj].journal);
		data.macke.remarks = (char**)Reallocspace(data.macke.remarks,
			sizeof(char*)*(remnum+1));
		data.macke.remarks[remnum++] = Dupstr(temp);
		}	/* not empty */
		if(Lenstr(data.protein.reference[indj].author)>1) {
		sprintf(temp, "auth:%s", data.protein.reference[indj].author);
		data.macke.remarks = (char**)Reallocspace(data.macke.remarks,
			sizeof(char*)*(remnum+1));
		data.macke.remarks[remnum++] = Dupstr(temp);
		} /* not empty author field */
		if(Lenstr(data.protein.reference[indj].title)>1) {
		sprintf(temp, "title:%s", data.protein.reference[indj].title);
		data.macke.remarks = (char**)Reallocspace(data.macke.remarks,
			sizeof(char*)*(remnum+1));
		data.macke.remarks[remnum++] = Dupstr(temp);
		}	/* not empty title field */
		if(Lenstr(data.protein.reference[indj].processing)>1) {
		sprintf(temp, "processing:%s", data.protein.reference[indj].processing);
		data.macke.remarks = (char**)Reallocspace(data.macke.remarks,
			sizeof(char*)*(remnum+1));
		data.macke.remarks[remnum++] = Dupstr(temp);
		}	/* not empty processing field */
	}	/* loop for copying other reference */
	/* copy keywords as remark */
	if(Lenstr(data.protein.keywords)>1)	{
		sprintf(temp, "KEYWORDS:%s", data.protein.keywords);
		data.macke.remarks = (char**)Reallocspace(data.macke.remarks,
			sizeof(char*)*(remnum+1));
		data.macke.remarks[remnum++] = Dupstr(temp);
	}
/* Maybe redudantly */
	if(Lenstr(data.protein.comments)>1)	{
		for(indj=0, indk=0; data.protein.comments[indj]!='\0'; indj++)
		{
			temp[indk++] = data.protein.comments[indj];
			if(data.protein.comments[indj]=='\n') {
				temp[indk] = '\0';
				data.macke.remarks = (char**)Reallocspace
					(data.macke.remarks,
					sizeof(char*)*(remnum+1));
				data.macke.remarks[remnum++]
					= Dupstr(temp);
				indk=0;
			}	/* new remark line */
		}	/* for loop to find other remarks */
	}	/* other comments */
	data.macke.numofrem = remnum;
	return(1);
}
/* ---------------------------------------------------------- */
/*	Function protein_to_genbank().
/*	Convert from Protein format to genbank format.
*/
void
protein_to_genbank(inf, outf)
char	*inf, *outf;
{
	FILE	*ifp, *ofp, *fopen();
	char	temp[TOKENNUM], protein_in(); 
	void	init(), init_genbank(), init_macke(), init_protein();
	void	init_seq_data(); 
	void	genbank_out();
	void	error();
	int	indi, ptom(), mtog(), total_num;

	if((ifp=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s, exit\n", inf);
		error(94, temp);
	}
	if(Lenstr(outf)<=0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s, exit\n", outf);
		error(96, temp);
	}

	/* seq irelenvant header */
	init();
	/* rewind(ifp); */
	init_genbank();
	init_macke();
	init_protein();
	while(protein_in(ifp)!=EOF)	{
		data.numofseq++;
		if(ptom()&&mtog()) genbank_out(ofp);
		init_genbank();
		init_macke();
		init_protein();
	}

#ifdef log
fprintf(stderr, "Total %d sequences have been processed\n", total_num);
#endif

}
/* ---------------------------------------------------------------- */
/*	Function protein_to_paup().
/*		Convert from Swissprot file to paup file.
*/
void
protein_to_paup(inf, outf)
char	*inf, *outf;
{
	FILE	*ifp, *ofp, *fopen();
	int	Lenstr(), maxsize, current, total_seq, first_line;
	char	protein_in_id(), temp[TOKENNUM], *name;
	char	*Dupstr(), *today_date(), *today;
	void	init(), init_paup(), init_seq_data(), paup_print_line();
	void	error(), init_protein(), protein_key_word(), Freespace();

	if((ifp=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s, exit\n", inf);
		error(80, temp);
	}
	if(Lenstr(outf)<=0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s, exit\n", outf);
		error(81, temp);
	}
	maxsize = 1; current = 0;
	name = NULL;
	init_paup();
	paup_print_header(ofp);
	while(maxsize>current)	{
		init();
		FILE_BUFFER_rewind(ifp);
		total_seq = 0;
		/* first time read input file */
		first_line = 0;
		while(protein_in_id(ifp)!=EOF)	{
			Freespace(&name);
			protein_key_word(data.protein.id, 0, temp, TOKENNUM);
			name = Dupstr(temp);
			if(data.seq_length>maxsize) 
				maxsize = data.seq_length;
			if(current<data.seq_length) first_line++;
			paup_print_line(name, data.sequence, current,
				(first_line==1), ofp);	
			if(first_line==1) first_line++;	/* avoid repeating */
			init_paup();  
			init_protein();
			total_seq++;
		}
		current += (SEQLINE - 10);
		if(maxsize>current) fprintf(ofp, "\n");
	}	/* print block by block */
	fprintf(ofp, "      ;\nENDBLOCK;\n");
	rewind(ofp);
	fprintf(ofp, "#NEXUS\n");
	today = today_date();
	if(today[Lenstr(today)-1]=='\n') today[Lenstr(today)-1] = '\0';
	fprintf(ofp, "[! RDP - the Ribsomal Database Project, (%s).]\n", today);
	fprintf(ofp, "[! To get started, send HELP to rdp@info.mcs.anl.gov ]\n");
	fprintf(ofp, "BEGIN DATA;\n   DIMENSIONS\n");
	fprintf(ofp, "      NTAX = %6d\n      NCHAR = %6d\n      ;\n", total_seq, maxsize);

#ifdef log
	fprintf(stderr, "Total %d sequences have been processed\n", total_seq);
#endif

	fclose(ifp); fclose(ofp);
}
