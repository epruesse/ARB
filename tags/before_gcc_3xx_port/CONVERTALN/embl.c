#include <stdio.h>
#include <stdlib.h>
#include "convert.h"
#include "global.h"

extern int	warning_out;

/* ----------------------------------------------------------
*	Function init_pm_data().
*		Init macke and embl data.
*/
void
init_em_data()	{
/* 	void	init_macke(), init_embl(); */

	init_macke();
	init_embl();
}
/* ------------------------------------------------------------
*	Function init_embl().
*		Initialize embl entry.
*/
void
init_embl()	{

	int	indi;
/* 	void	Freespace(); */
/* 	char	*Dupstr(); */

	/* initialize embl format */
	Freespace(&(data.embl.id));
	Freespace(&(data.embl.dateu));
	Freespace(&(data.embl.datec));
	Freespace(&(data.embl.description));
	Freespace(&(data.embl.os));
	Freespace(&(data.embl.accession));
	Freespace(&(data.embl.keywords));

	for(indi=0; indi<data.embl.numofref; indi++)	{
		Freespace(&(data.embl.reference[indi].author));
		Freespace(&(data.embl.reference[indi].title));
		Freespace(&(data.embl.reference[indi].journal));
		Freespace(&(data.embl.reference[indi].processing));
	}
	Freespace(&(data.embl.reference));
	Freespace(&(data.embl.dr));
	Freespace(&(data.embl.comments.orginf.source));
	Freespace(&(data.embl.comments.orginf.cc));
	Freespace(&(data.embl.comments.orginf.formname));
	Freespace(&(data.embl.comments.orginf.nickname));
	Freespace(&(data.embl.comments.orginf.commname));
	Freespace(&(data.embl.comments.orginf.hostorg));
	Freespace(&(data.embl.comments.seqinf.RDPid));
	Freespace(&(data.embl.comments.seqinf.gbkentry));
	Freespace(&(data.embl.comments.seqinf.methods));
	Freespace(&(data.embl.comments.others));

	data.embl.id=Dupstr("\n");
	data.embl.dateu=Dupstr("\n");
	data.embl.datec=Dupstr("\n");
	data.embl.description=Dupstr("\n");
	data.embl.os=Dupstr("\n");
	data.embl.accession=Dupstr("\n");
	data.embl.keywords=Dupstr("\n");
	data.embl.dr=Dupstr("\n");
	data.embl.numofref=0;
	data.embl.reference=NULL;
	data.embl.comments.orginf.exist=0;
	data.embl.comments.orginf.source=Dupstr("\n");
	data.embl.comments.orginf.cc=Dupstr("\n");
	data.embl.comments.orginf.formname=Dupstr("\n");
	data.embl.comments.orginf.nickname=Dupstr("\n");
	data.embl.comments.orginf.commname=Dupstr("\n");
	data.embl.comments.orginf.hostorg=Dupstr("\n");
	data.embl.comments.seqinf.exist=0;
	data.embl.comments.seqinf.RDPid=Dupstr("\n");
	data.embl.comments.seqinf.gbkentry=Dupstr("\n");
	data.embl.comments.seqinf.methods=Dupstr("\n");
	data.embl.comments.others=NULL;
	data.embl.comments.seqinf.comp5=' ';
	data.embl.comments.seqinf.comp3=' ';
}
/* ---------------------------------------------------------------
*	Function embl_in().
*		Read in one embl entry.
*/
char
embl_in(fp)
FILE	*fp;
{
	char	line[LINENUM], key[TOKENNUM];
	char	*eof, eoen;
/* 	char	*embl_date(); */
/* 	char	*embl_version(); */
/* 	char	*embl_comments(), *embl_origin(); */
/* 	char	*embl_skip_unidentified(); */
/* 	char	*Dupstr(), *temp; */
/* 	void	embl_key_word(), warning(), error(), Append_char(); */
/* 	void	replace_entry(), embl_verify_title(); */
	int	refnum;

	eoen=' '; /* end-of-entry, set to be 'y' after '//' is read */

	for(eof=Fgetline(line, LINENUM, fp); eof!=NULL&&eoen!='y'; ) {
		if(Lenstr(line)<=1) {
			eof=Fgetline(line, LINENUM, fp);
			continue;	/* empty line, skip */
		}

		embl_key_word(line, 0, key, TOKENNUM);
		eoen='n';

		if((Cmpstr(key, "ID"))==EQ)	{
			eof = embl_one_entry(line, fp,
				&(data.embl.id), key);

		} else if((Cmpstr(key, "DT"))==EQ)	{
			eof = embl_date(line, fp);

		} else if((Cmpstr(key, "DE"))==EQ)	{
			eof = embl_one_entry(line, fp,
				&(data.embl.description), key);

		} else if((Cmpstr(key, "OS"))==EQ)	{
			eof = embl_one_entry(line, fp,
				&(data.embl.os), key);

		} else if((Cmpstr(key, "AC"))==EQ)	{
			eof = embl_one_entry(line, fp,
				&(data.embl.accession), key);

		} else if((Cmpstr(key, "KW"))==EQ)	{
			eof = embl_one_entry(line, fp,
				&(data.embl.keywords), key);

			/* correct missing '.' */
			if(Lenstr(data.embl.keywords)<=1) {
				replace_entry(&(data.embl.keywords), ".\n");
			} else Append_char(&(data.embl.keywords), '.');

		} else if((Cmpstr(key, "DR"))==EQ)	{
			eof = embl_one_entry(line, fp,
				&(data.embl.dr), key);

		} else if((Cmpstr(key, "RA"))==EQ)	{

			refnum = data.embl.numofref-1;
			eof = embl_one_entry(line, fp,
			&(data.embl.reference[refnum].author), key);
			Append_char
			(&(data.embl.reference[refnum].author), ';');

		} else if((Cmpstr(key, "RT"))==EQ)	{

			refnum = data.embl.numofref-1;
			eof = embl_one_entry(line, fp,
			&(data.embl.reference[refnum].title), key);

			/* Check missing '"' at the both ends */
			embl_verify_title(refnum);

		} else if((Cmpstr(key, "RL"))==EQ)	{

			refnum = data.embl.numofref-1;
			eof = embl_one_entry(line, fp,
			&(data.embl.reference[refnum].journal), key);
			Append_char
			(&(data.embl.reference[refnum].journal), '.');

		} else if((Cmpstr(key, "RP"))==EQ)	{

			refnum = data.embl.numofref-1;
			eof = embl_one_entry(line, fp,
			&(data.embl.reference[refnum].processing), key);

		} else if((Cmpstr(key, "RN"))==EQ)	{

			eof = embl_version(line, fp);

		} else if((Cmpstr(key, "CC"))==EQ)	{

			eof = embl_comments(line, fp);

		} else if((Cmpstr(key, "SQ"))==EQ)		{

			eof = embl_origin(line, fp);
			eoen = 'y';

		} else {	/* unidentified key word */
			eof = embl_skip_unidentified(key, line, fp);
		}
		/* except "ORIGIN", at the end of all the other cases,
		* a new line has already read in, so no further read
		* is necessary */

	}	/* for loop to read an entry line by line */

	if(eoen=='n')
		error(83, "Reach EOF before one entry is read, Exit");

	if(eof==NULL)	return(EOF);
	else	return(EOF+1);

}
/* ---------------------------------------------------------------
*	Function embl_in_id().
*		Read in one embl entry.
*/
char
embl_in_id(fp)
FILE	*fp;
{
	char	line[LINENUM], key[TOKENNUM];
	char	*eof, eoen;
/* 	char	*embl_one_entry(), *embl_origin(); */
/* 	char	*embl_skip_unidentified(); */
/* 	void	embl_key_word(), warning(), error(); */
/* 	int	Lenstr(); */

	eoen=' ';
	/* end-of-entry, set to be 'y' after '//' is read */

	for(eof=Fgetline(line, LINENUM, fp); eof!=NULL&&eoen!='y'; )
	{
		if(Lenstr(line)<=1) {
			eof=Fgetline(line, LINENUM, fp);
			continue;	/* empty line, skip */
		}

		embl_key_word(line, 0, key, TOKENNUM);
		eoen='n';

		if((Cmpstr(key, "ID"))==EQ)	{
			eof = embl_one_entry(line, fp,
				&(data.embl.id), key);
		} else if((Cmpstr(key, "SQ"))==EQ)	{
			eof = embl_origin(line, fp);
			eoen = 'y';
		} else {	/* unidentified key word */
			eof = embl_skip_unidentified(key, line, fp);
		}
		/* except "ORIGIN", at the end of all the other cases,
           a new line has already read in, so no further read
           is necessary */

	}	                        /* for loop to read an entry line by line */

	if(eoen=='n')
		error(84, "Reach EOF before one entry is read, Exit");

	if(eof==NULL)	return(EOF);
	else	return(EOF+1);

}
/* ----------------------------------------------------------------
*	Function embl_key_word().
*		Get the key_word from line beginning at index.
*/
void
embl_key_word(line, index, key, length)
char	*line;
int	index;
char	*key;
int	length;		/* max size of key word */
{
	int	indi, indj;

	if(line==NULL)	{	key[0]='\0'; return;	}
	for(indi=index, indj=0; (index-indi)<length&&line[indi]!=' '
	&&line[indi]!='\t'&&line[indi]!='\n'&&line[indi]!='\0';
	indi++, indj++)
		key[indj] = line[indi];
	key[indj] = '\0';
}
/* ------------------------------------------------------------
*	Function embl_check_blanks().
*		Check if there is (numb) blanks at beginning of line.
*/
int
embl_check_blanks(line, numb)
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
/* ----------------------------------------------------------------
*	Function embl_continue_line().
*		if there are (numb) blanks at the beginning of line,
*			it is a continue line of the current command.
*/
char
*embl_continue_line(pattern, string, line, fp)
char	*pattern, **string, *line;
FILE	*fp;
{
	int	ind;
/* 	int	embl_check_blanks(), Skip_white_space(); */
	char	key[TOKENNUM], *eof, temp[LINENUM];
/* 	void	Cpystr(), embl_key_word(), Append_rp_eoln(); */

	/* check continue lines */
	for(eof=Fgetline(line, LINENUM, fp);
	eof!=NULL; eof=Fgetline(line, LINENUM, fp))	{

		if(Lenstr(line)<=1)	continue;

		embl_key_word(line, 0, key, TOKENNUM);

		if(Cmpstr(pattern, key)!=EQ)	break;

		/* remove end-of-line, if there is any */
		ind=Skip_white_space(line, p_nonkey_start);
		Cpystr(temp, (line+ind));
		Append_rp_eoln(string, temp);

	}	/* end of continue line checking */

	return(eof);
}
/* --------------------------------------------------------------
*	Function embl_one_entry().
*		Read in one embl entry lines.
*/
char
*embl_one_entry(line, fp, entry, key)
char	*line;
FILE	*fp;
char	**entry, *key;
{
	int	index;
	char	*eof;
/* 	void	error(), replace_entry(); */

	index = Skip_white_space(line, p_nonkey_start);
	replace_entry(entry, line+index);
	eof = (char*)embl_continue_line(key, entry, line, fp);

	return(eof);
}
/* ---------------------------------------------------------------
*	Function embl_verify_title().
*		Verify title.
*/
void
embl_verify_title(refnum)
int	refnum;
{
	int	len;
	char	*temp;
/* 	void	Append(), Append_char(), replace_entry(); */

	Append_char(&(data.embl.reference[refnum].title), ';');

	len=Lenstr(data.embl.reference[refnum].title);
	if(len>2&&(data.embl.reference[refnum].title[0]!='"'
	||data.embl.reference[refnum].title[len-3]!='"')) {
		if(data.embl.reference[refnum].title[0]!='"')
			temp=(char*)Dupstr("\"");
		else temp=(char*)Dupstr("");
		Append(&temp, data.embl.reference[refnum].title);
		if((len>2&&data.embl.reference[refnum].title[len-3]
		!='"'))
		{
			len=Lenstr(temp);
			temp[len-2]='"';
			Append_char(&temp, ';');
		}
		replace_entry(&(data.embl.reference[refnum].title),
			temp);
	}
}
/* --------------------------------------------------------------
*	Function embl_date().
*		Read in embl DATE lines.
*/
char
*embl_date(line, fp)
char	*line;
FILE	*fp;
{
	int	index;
	char	*eof, key[TOKENNUM];
/* 	void	error(), embl_key_word(), replace_entry(); */
/* 	void	warning(); */

	index = Skip_white_space(line, p_nonkey_start);
	replace_entry(&(data.embl.dateu), line+index);

	eof = Fgetline(line, LINENUM, fp);
	embl_key_word(line, 0, key, TOKENNUM);
	if((Cmpstr(key, "DT"))==EQ)	{
		index = Skip_white_space(line, p_nonkey_start);
		replace_entry(&(data.embl.datec), line+index);
		/* skip the rest of DT lines */
		do	{
			eof = Fgetline(line, LINENUM, fp);
			embl_key_word(line, 0, key, TOKENNUM);
		} while(eof!=NULL&&Cmpstr(key, "DT")==EQ);
		return(eof);
	} else {
		/* always expect more than two DT lines */
		warning(33, "one DT line is missing");
		return(eof);
	}
}
/* --------------------------------------------------------------
*	Function embl_version().
*		Read in embl RN lines.
*/
char
*embl_version(line, fp)
char	*line;
FILE	*fp;
{
	int	index;
	char	*eof;
/* 	char	*Reallocspace(), *Fgetline(); */

	index = Skip_white_space(line, p_nonkey_start);
	if(data.embl.numofref==0)	{
		data.embl.numofref = 1;
		data.embl.reference = (Emblref*)calloc(1,sizeof(Emblref)*1);
		data.embl.reference[0].author = Dupstr("");
		data.embl.reference[0].title = Dupstr("");
		data.embl.reference[0].journal = Dupstr("");
		data.embl.reference[0].processing = Dupstr("");
	} else {
		data.embl.numofref++;
		data.embl.reference = (Emblref*)Reallocspace(data.embl.reference, sizeof(Emblref)*(data.embl.numofref));
		data.embl.reference[data.embl.numofref-1].author = Dupstr("");
		data.embl.reference[data.embl.numofref-1].title = Dupstr("");
		data.embl.reference[data.embl.numofref-1].journal = Dupstr("");
		data.embl.reference[data.embl.numofref-1].processing = Dupstr("");
	}
	eof = Fgetline(line, LINENUM, fp);
	return(eof);
}
/* --------------------------------------------------------------
*	Function embl_comments().
*		Read in embl comment lines.
*/
char
*embl_comments(line, fp)
char	*line;
FILE	*fp;
{
	int	index, offset;
/* 	int	Skip_white_space(); */
	char	*eof;
	char	key[TOKENNUM];
/* 	char	*Dupstr(); */
/* 	void	Append(); */

	for(;line[0]=='C'&&line[1]=='C';)	{

		index = Skip_white_space(line, 5);
		offset = embl_comment_key(line+index, key);
		index = Skip_white_space(line, index+offset);
		if(Cmpstr(key, "Source of strain:")==EQ) {
			eof = embl_one_comment_entry
				(fp, &(data.embl.comments.orginf.source),
					line, index);

		} else if(Cmpstr(key, "Culture collection:")==EQ) {

			eof = embl_one_comment_entry
				(fp, &(data.embl.comments.orginf.cc),
					line, index);

		} else if(Cmpstr(key, "Former name:")==EQ) {

			eof = embl_one_comment_entry
				(fp, &(data.embl.comments.orginf.formname),
					line, index);

		} else if(Cmpstr(key, "Alternate name:")==EQ) {

			eof = embl_one_comment_entry
				(fp, &(data.embl.comments.orginf.nickname),
					line, index);

		} else if(Cmpstr(key, "Common name:")==EQ) {

			eof = embl_one_comment_entry
				(fp, &(data.embl.comments.orginf.commname),
					line, index);

		} else if(Cmpstr(key, "Host organism:")==EQ) {

			eof = embl_one_comment_entry
				(fp, &(data.embl.comments.orginf.hostorg),
					line, index);

		} else if(Cmpstr(key, "RDP ID:")==EQ) {
			eof = embl_one_comment_entry
				(fp, &(data.embl.comments.seqinf.RDPid),
					line, index);

		} else if(Cmpstr(key,
			"Corresponding GenBank entry:")==EQ) {

			eof = embl_one_comment_entry
				(fp, &(data.embl.comments.seqinf.gbkentry),
					line, index);

		} else if(Cmpstr(key, "Sequencing methods:")==EQ) {

			eof = embl_one_comment_entry
				(fp, &(data.embl.comments.seqinf.methods),
					line, index);

		} else if(Cmpstr(key, "5' end complete:")==EQ) {

			sscanf(line+index, "%s", key);
			if(key[0]=='Y')
				data.embl.comments.seqinf.comp5 = 'y';
			else data.embl.comments.seqinf.comp5 = 'n';

			eof=Fgetline(line, LINENUM, fp);

		} else if(Cmpstr(key, "3' end complete:")==EQ) {

			sscanf(line+index, "%s", key);
			if(key[0]=='Y')
				data.embl.comments.seqinf.comp3 = 'y';
			else data.embl.comments.seqinf.comp3 = 'n';

			eof=Fgetline(line, LINENUM, fp);

		} else if(Cmpstr(key,
			"Sequence information ")==EQ) {

			/* do nothing */
			data.embl.comments.seqinf.exist = 1;

			eof=Fgetline(line, LINENUM, fp);

		} else if(Cmpstr(key, "Organism information")==EQ) {

			/* do nothing */
			data.embl.comments.orginf.exist = 1;

			eof=Fgetline(line, LINENUM, fp);

		} else {	/* other comments */

			if(data.embl.comments.others == NULL)	{
				data.embl.comments.others
					=(char*)Dupstr(line+5);

			} else	Append(&(data.embl.comments.others),
					line+5);
			eof=Fgetline(line, LINENUM, fp);

		}
	}
	return(eof);
}
/* ----------------------------------------------------------------
*	Function embl_skip_unidentified().
*		if there are (numb) blanks at the beginning of line,
*		it is a continue line of the current command.
*/
char
*embl_skip_unidentified(pattern, line, fp)
char	*pattern, *line;
FILE	*fp;
{
/* 	int	Lenstr(), Cmpstr(); */
	char	*eof;
	char	key[TOKENNUM];
/* 	void	embl_key_word(); */

	/* check continue lines */
	for(eof=Fgetline(line, LINENUM, fp);
	eof!=NULL; eof=Fgetline(line, LINENUM, fp))	{
		embl_key_word(line, 0, key, TOKENNUM);
		if(Cmpstr(key, pattern)!=EQ) break;
	}	/* end of continue line checking */
	return(eof);
}
/* -------------------------------------------------------------
*	Function embl_comment_key().
*		Get the subkey_word in comment lines beginning
*			at index.
*/
int
embl_comment_key(line, key)
char	*line;
char	*key;
{
	int	indi, indj;

	if(line==NULL)	{ key[0]='\0'; return(0);	}

	for(indi=indj=0;
	line[indi]!=':'&&line[indi]!='\t'&&line[indi]!='\n'
	&&line[indi]!='\0'&&line[indi]!='('; indi++, indj++)
		key[indj] = line[indi];

	if(line[indi]==':') key[indj++] = ':';

	key[indj] = '\0';

	return(indi+1);
}
/* ------------------------------------------------------------
*	Function embl_one_comment_entry().
*		Read in one embl sub-entry in comments lines.
*		If not RDP defined comment, should not call
*			this function.
*/
char *embl_one_comment_entry(fp, datastring, line, start_index)
FILE	*fp;
char	**datastring, *line;
int	start_index;
{
	int	index;
/* 	int	Skip_white_space(), Lenstr(), Blank_num(); */
	char	*eof, temp[LINENUM];
/* 	char	*eof, *Fgetline(), *Dupstr(), temp[LINENUM]; */
/* 	void	replace_entry(), Cpystr(), Append_rp_eoln(); */

	index = Skip_white_space(line, start_index);
	replace_entry(datastring, line+index);

	/* check continue lines */
	for(eof=Fgetline(line, LINENUM, fp);
	eof!=NULL&&line[0]=='C'&&line[1]=='C'
	&&Blank_num(line+2)>=COMMCNINDENT+COMMSKINDENT;
	eof=Fgetline(line, LINENUM, fp))	{

		/* remove end-of-line, if there is any */
		index=Skip_white_space(line, p_nonkey_start+COMMSKINDENT+COMMCNINDENT);
		Cpystr(temp, (line+index));
		Append_rp_eoln(datastring, temp);

	}	                        /* end of continue line checking */

    return eof;
}
/* --------------------------------------------------------------
*	Function embl_origin().
*		Read in embl sequence data.
*/
char
*embl_origin(line, fp)
char	*line;
FILE	*fp;
{
	char	*eof;
	int	index;

	data.seq_length = 0;
	/* read in whole sequence data */
	for(eof=Fgetline(line, LINENUM, fp);
	eof!=NULL&&line[0]!='/'&&line[1]!='/';
	eof=Fgetline(line, LINENUM, fp))
	{
		for(index=5; line[index]!='\n'&&line[index]!='\0';
		index++)	{
			if(line[index]!=' '
			&&data.seq_length>=data.max) {
				data.max += 100;
				data.sequence = (char*)
					Reallocspace(data.sequence,
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
/* ----------------------------------------------------------------
 *	Function embl_out().
 *		Output EMBL data.
 */
void embl_out(fp)
     FILE *fp;
{
/* 	int	Lenstr(); */
/* 	int	base_a, base_t, base_g, base_c, base_other; */
	int	indi /*, indj, indk, len*/;
/* 	void 	embl_print_lines(), embl_out_comments(); */
/* 	void 	embl_out_origin(); */

	if(Lenstr(data.embl.id)>1)
		fprintf(fp, "ID   %sXX\n", data.embl.id);
	if(Lenstr(data.embl.accession)>1)	{
		embl_print_lines(fp, "AC", data.embl.accession, SEPNODEFINED, "");
		fprintf(fp, "XX\n");
	}

	/* Date */
	if(Lenstr(data.embl.dateu)>1)
		fprintf(fp, "DT   %s", data.embl.dateu);
	if(Lenstr(data.embl.datec)>1)
		fprintf(fp, "DT   %sXX\n", data.embl.datec);
	if(Lenstr(data.embl.description)>1)	{
        embl_print_lines(fp, "DE", data.embl.description, SEPNODEFINED, "");
		fprintf(fp, "XX\n");
	}

	if(Lenstr(data.embl.keywords)>1)	{
		embl_print_lines(fp, "KW", data.embl.keywords, SEPDEFINED, ";");
		fprintf(fp, "XX\n");
	}

	if(Lenstr(data.embl.os)>1)	{
		embl_print_lines(fp, "OS", data.embl.os, SEPNODEFINED, "");
		fprintf(fp, "OC   No information.\n");
		fprintf(fp, "XX\n");
	}

	/* Reference */
	for(indi=0; indi<data.embl.numofref; indi++)	{
		fprintf(fp, "RN   [%d]\n", indi+1);
		if(Lenstr(data.embl.reference[indi].processing)>1)
			fprintf(fp, "RP   %s",
			data.embl.reference[indi].processing);
		if(Lenstr(data.embl.reference[indi].author)>1)
			embl_print_lines(fp, "RA", data.embl.reference[indi].author, SEPDEFINED, ",");
		if(Lenstr(data.embl.reference[indi].title)>1)
			embl_print_lines(fp, "RT", data.embl.reference[indi].title, SEPNODEFINED, "");
		else fprintf(fp, "RT   ;\n");
		if(Lenstr(data.embl.reference[indi].journal)>1)
			embl_print_lines(fp, "RL", data.embl.reference[indi].journal, SEPNODEFINED, "");
		fprintf(fp, "XX\n");
	}

	if(Lenstr(data.embl.dr)>1)	{
		embl_print_lines(fp, "DR", data.embl.dr, SEPNODEFINED, "");
		fprintf(fp, "XX\n");
	}

	embl_out_comments(fp);
	embl_out_origin(fp);
}
/* ----------------------------------------------------------------
 *	Function embl_print_lines().
 *		Print EMBL entry and wrap around if line over
 *			EMBLMAXCHAR.
 */
void embl_print_lines(fp, key, data, flag, separators)
     FILE       *fp;
     const char *key;
     char       *data;
     int         flag;
     const char *separators;
{
	int	indi, indj, indk, len;
	int	ibuf;

	len = Lenstr(data)-1;
	/* indi: first char of the line */
	/* indj: num of char, excluding the first char, of the line */
	for(indi=0; indi<len; indi+=(indj+1))	{
		indj=EMBLMAXCHAR;
		if((Lenstr(data+indi))>EMBLMAXCHAR) {

			ibuf = indj;

			/* searching for proper termination of a line */
			for(;indj>0
			&&((!flag&&!last_word(data[indi+indj]))
			||(flag&&!is_separator
			(data[indi+indj], separators)));
			indj--)	;

			if(indj==0)	indj=ibuf;
			else if(data[indi+indj+1]==' ') indj++;

			fprintf(fp, "%s   ", key);

			for(indk=0; indk<indj; indk++)
				fprintf(fp, "%c", data[indi+indk]);

		/* leave out the last space, if there is any */
			if(data[indi+indj]!=' ')
				fprintf(fp, "%c", data[indi+indj]);

			fprintf(fp, "\n");

		} else fprintf(fp, "%s   %s", key, data+indi);
	}
}
/* -------------------------------------------------------
*	Function embl_out_comments().
*		Print out the comments part of EMBL format.
*/
void
embl_out_comments(fp)
FILE	*fp;
{
	int	indi, len;
/* 	void	embl_print_comment(); */

	if(data.embl.comments.orginf.exist==1)	{
		fprintf(fp, "CC   Organism information\n");

		if(Lenstr(data.embl.comments.orginf.source)>1)
			embl_print_comment(fp, "Source of strain: ", data.embl.comments.orginf.source, COMMSKINDENT, COMMCNINDENT);

		if(Lenstr(data.embl.comments.orginf.cc)>1)
			embl_print_comment(fp, "Culture collection: ", data.embl.comments.orginf.cc, COMMSKINDENT, COMMCNINDENT);

		if(Lenstr(data.embl.comments.orginf.formname)>1)
			embl_print_comment(fp, "Former name: ", data.embl.comments.orginf.formname, COMMSKINDENT, COMMCNINDENT);

		if(Lenstr(data.embl.comments.orginf.nickname)>1)
			embl_print_comment(fp, "Alternate name: ", data.embl.comments.orginf.nickname, COMMSKINDENT, COMMCNINDENT);

		if(Lenstr(data.embl.comments.orginf.commname)>1)
			embl_print_comment(fp, "Common name: ", data.embl.comments.orginf.commname, COMMSKINDENT, COMMCNINDENT);

		if(Lenstr(data.embl.comments.orginf.hostorg)>1)
			embl_print_comment(fp, "Host organism: ", data.embl.comments.orginf.hostorg, COMMSKINDENT, COMMCNINDENT);

	}	/* organism information */

	if(data.embl.comments.seqinf.exist==1) {

		fprintf(fp,
			"CC   Sequence information (bases 1 to %d)\n",
				data.seq_length);

		if(Lenstr(data.embl.comments.seqinf.RDPid)>1)
			embl_print_comment(fp, "RDP ID: ",
			data.embl.comments.seqinf.RDPid,
			COMMSKINDENT, COMMCNINDENT);

		if(Lenstr(data.embl.comments.seqinf.gbkentry)>1)
			embl_print_comment(fp, "Corresponding GenBank entry: ",
			data.embl.comments.seqinf.gbkentry,
			COMMSKINDENT, COMMCNINDENT);

		if(Lenstr(data.embl.comments.seqinf.methods)>1)
			embl_print_comment(fp, "Sequencing methods: ",
			data.embl.comments.seqinf.methods,
			COMMSKINDENT, COMMCNINDENT);

		if(data.embl.comments.seqinf.comp5=='n')
			fprintf(fp,
			"CC     5' end complete: No\n");

		else if(data.embl.comments.seqinf.comp5=='y')
			fprintf(fp,
			"CC     5' end complete: Yes\n");

		if(data.embl.comments.seqinf.comp3=='n')
			fprintf(fp,
			"CC     3' end complete: No\n");

		else if(data.embl.comments.seqinf.comp3=='y')
			fprintf(fp,
			"CC     3' end complete: Yes\n");

	}	/* sequence information */

	if(Lenstr(data.embl.comments.others)>1)	{
		for(indi=0, len=Lenstr(data.embl.comments.others);
		indi<len; indi++){
		if((indi>0&&data.embl.comments.others[indi-1]=='\n')
			||indi==0)
				fprintf(fp, "CC   ");
			fprintf(fp, "%c",
				data.embl.comments.others[indi]);
		}
		fprintf(fp, "XX\n");
	}
}
/* --------------------------------------------------------------
 *	Fucntion embl_print_comment().
 *		Print one embl comment  line, wrap around if over
 *			column 80.
 */
void embl_print_comment(fp, key, string, offset, indent)
     FILE	    *fp;
     const char	*key;
     char	    *string;
     int	     offset, indent;
{
	int	first_time=1, indi, indj, indk, indl;
	int	len;

	len = Lenstr(string)-1;
	for(indi=0; indi<len; indi+=(indj+1)) {

		if(first_time)
			indj=EMBLMAXCHAR-offset-Lenstr(key)-1;
		else indj=EMBLMAXCHAR-offset-indent-1;

		fprintf(fp, "CC   ");

		if(!first_time)	{
			for(indl=0; indl<(offset+indent); indl++)
				fprintf(fp, " ");
		} else {
			for(indl=0; indl<offset; indl++)
				fprintf(fp, " ");
			fprintf(fp, "%s", key);
			first_time = 0;
		}
		if(Lenstr(string+indi)>indj) {

		/* search for proper termination of a line */
			for(; indj>=0&&!last_word(string[indj+indi]);
			indj--) ;

			/* print left margine */
			if(string[indi]==' ') indk = 1;
			else indk = 0;

			for(; indk<indj; indk++)
				fprintf(fp, "%c", string[indi+indk]);

		/* leave out the last space, if there is any */
			if(string[indi+indj]!=' ')
				fprintf(fp, "%c", string[indi+indj]);
			fprintf(fp, "\n");

		} else fprintf(fp, "%s", string+indi);

	}	/* for each char */
}
/* -----------------------------------------------------
*	Function embl_out_origin().
*		Print out the sequence data of EMBL format.
*/
void
embl_out_origin(fp)
FILE	*fp;
{
	int	base_a, base_c, base_t, base_g, base_other;
	int	indi, indj, indk;
/* 	void	count_base(); */

	/* print seq data */
	count_base(&base_a, &base_t, &base_g, &base_c, &base_other);

	fprintf(fp,
	"SQ   Sequence %d BP; %d A; %d C; %d G; %d T; %d other;\n",
	data.seq_length, base_a, base_c, base_g, base_t, base_other);

	for(indi=0, indj=0, indk=1; indi<data.seq_length; indi++)
	{
		if((indk % 60)==1) fprintf(fp, "     ");
		fprintf(fp, "%c", data.sequence[indi]);
		indj++;
		if((indk % 60)==0) { fprintf(fp, "\n"); indj=0; }
		else if(indj==10&&indi!=(data.seq_length-1))
			{ fprintf(fp, " "); indj=0; }
		indk++;
	}
	if((indk % 60)==1)  fprintf(fp, "//\n");
	else fprintf(fp, "\n//\n");
}
/* ----------------------------------------------------------
*	Function embl_to_macke().
*	Convert from Embl format to Macke format.
*/
void
embl_to_macke(inf, outf, format)
char	*inf, *outf;
int	format;
{
	FILE	*ifp, *ofp;
	char	temp[TOKENNUM];
/* 	void	init(), init_em_data(), init_seq_data(); */
/* 	void	macke_out_header(), macke_out0(); */
/* 	void	macke_out1(), macke_out2(); */
/* 	void	error(); */
	int	indi, total_num;

	if((ifp=fopen(inf, "r"))==NULL)	{
		sprintf(temp,
		"Cannot open input file %s, exit\n", inf);
		error(98, temp);
	}
	if(Lenstr(outf)<=0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp,
		"Cannot open output file %s, exit\n", outf);
		error(97, temp);
	}

	init();
	/* macke format seq irelenvant header */
	macke_out_header(ofp);
	for(indi=0; indi<3; indi++)	{
		rewind(ifp);
		init_seq_data();
		init_em_data();
		while(embl_in(ifp)!=EOF)	{
			data.numofseq++;
			if(etom()) {
		/* convert from embl form to macke form */
				switch(indi)	{
				case 0:
				/* output seq display format */
					macke_out0(ofp, format);
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
			} else error(4,
		"Conversion from embl to macke fails, Exit");
			init_em_data();
		}
		total_num = data.numofseq;
		if(indi==0)	{
			fprintf(ofp, "#-\n");
			/* no warning messages for next loop */
			warning_out = 0;
		}
	}	/* for each seq; loop */

	warning_out = 1;

#ifdef log
	fprintf(stderr, "Total %d sequences have been processed\n",
		total_num);
#endif
}
/* ------------------------------------------------------------
*	Function etom().
*		Convert from embl format to Macke format.
*/
int etom()	{
/* 	void init_genbank(); */
/* 	int	etog(), gtom(); */

	init_genbank();
	if (etog()) return gtom();
    return 0;
}
/* -------------------------------------------------------------
*	Function embl_to_embl().
*		Print out EMBL data.
*/
void
embl_to_embl(inf, outf, format)
char	*inf, *outf;
int	format;
{
	FILE	*ifp, *ofp;
	char	temp[TOKENNUM];
/* 	void	init(), init_seq_data(), init_embl(); */
/* 	void	error(), warning(); */

	if((ifp=fopen(inf, "r"))==NULL)	{
		sprintf(temp,
		"Cannot open input file %s\n", inf);
		error(27, temp);
	}
	if(Lenstr(outf)<=0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp,
		"Cannot open output file %s\n", outf);
		error(28, temp);
	}
	init();
	init_seq_data();
	init_embl();

#ifdef log
	fprintf(stderr, "Start converting...\n");
#endif

	while(embl_in(ifp)!=EOF)	{
		data.numofseq++;
		embl_out(ofp);
		init_embl();
	}

#ifdef log
	fprintf(stderr,
	"Total %d sequences have been processed\n",
		data.numofseq);
#endif

}
/* -------------------------------------------------------------
*	Function embl_to_genbank().
*		Convert from EMBL format to genbank format.
*/
void
embl_to_genbank(inf, outf, format)
char	*inf, *outf;
int	format;
{
	FILE	*ifp, *ofp;
	char	temp[TOKENNUM];
/* 	void	init(), init_seq_data(), init_genbank(), init_embl(); */
/* 	void	error(), warning(); */
/* 	int	etog(); */

	if((ifp=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s\n", inf);
		error(30, temp);
	}
	if(Lenstr(outf)<=0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s\n", outf);
		error(31, temp);
	}
	init();
	init_seq_data();
	init_genbank();
	init_embl();

#ifdef log
	fprintf(stderr, "Start converting...\n");
#endif

	while(embl_in(ifp)!=EOF)	{
		data.numofseq++;
		if(etog()) genbank_out(ofp);
		else error(32,
		"Conversion from macke to genbank fails, Exit");
		init_genbank();
		init_embl();
#ifdef log
		if((data.numofseq % 50)==0)
		fprintf(stderr, "%d sequences are converted...\n",
			data.numofseq);
#endif

	}

#ifdef log
	fprintf(stderr,
	"Total %d sequences have been processed\n", data.numofseq);
#endif
}
/* -------------------------------------------------------------
*	Function etog()
*		Convert from embl to genbank format.
*/
int
etog()	{

	int	indi;
	char	key[TOKENNUM], temp[LONGTEXT];
	char	t1[TOKENNUM], t2[TOKENNUM], t3[TOKENNUM];
/* 	char	*Dupstr(), *genbank_date(); */
/* 	void	embl_key_word(), Cpystr(); */
/* 	void	etog_comments(), etog_reference(), Append_char(); */
/* 	void	replace_entry(); */

	embl_key_word(data.embl.id, 0, key, TOKENNUM);
	if(Lenstr(data.embl.dr)>1)	{
		/* get short_id from DR line if there is RDP def. */
		Cpystr(t3, "dummy");
		sscanf(data.embl.dr, "%s %s %s", t1, t2, t3);
		if(Cmpstr(t1, "RDP;")==EQ)	{
			if(Cmpstr(t3, "dummy")!=EQ)	{
				Cpystr(key, t3);
			} else Cpystr(key, t2);
			key[Lenstr(key)-1]='\0'; /* remove '.' */
		}
	}
	Cpystr(temp, key);

	/* LOCUS */
	for(indi=Lenstr(temp); indi<13; temp[indi++]=' ') ;
	if(Lenstr(data.embl.dateu)>1)	{
		sprintf((temp+10),
		"%7d bp    RNA             RNA       %s\n",
		data.seq_length, genbank_date(data.embl.dateu));
	} else sprintf((temp+10),
		"7%d bp    RNA             RNA       %s\n",
		data.seq_length, genbank_date(today_date()));
	replace_entry(&(data.gbk.locus), temp);

	/* DEFINITION */
	if(Lenstr(data.embl.description)>1)	{

		replace_entry(&(data.gbk.definition),
			data.embl.description);

		/* must have a period at the end */
		Append_char(&(data.gbk.definition), '.');
	}

	/* SOURCE and DEFINITION if not yet defined */
	if(Lenstr(data.embl.os)>1)	{
		replace_entry(&(data.gbk.source),
			data.embl.os);

		replace_entry(&(data.gbk.organism),
			data.embl.os);

		if(Lenstr(data.embl.description)<=1)	{
			replace_entry(&(data.gbk.definition),
				data.embl.os);
		}
	}

	/* COMMENT GenBank entry */
	if(Lenstr(data.embl.accession)>1)
		replace_entry(&(data.gbk.accession),
			data.embl.accession);

	if(Lenstr(data.embl.keywords)>1&&data.embl.keywords[0]!='.')
		replace_entry(&(data.gbk.keywords),
			data.embl.keywords);

	/* convert reference */
	etog_reference();

	/* convert comments */
	etog_comments();

	return(1);
}
/* ---------------------------------------------------------------------
*	Function etog_reference().
*		Convert reference from EMBL to GenBank format.
*/
void
etog_reference()	{

	int	indi, len, start, end;
	char	temp[LONGTEXT];
/* 	char	*etog_author(), *etog_journal(); */

	data.gbk.numofref = data.embl.numofref;

	data.gbk.reference
	= (Reference*)calloc(1,sizeof(Reference)*data.embl.numofref);

	for(indi=0; indi<data.embl.numofref; indi++)	{

		if(Lenstr(data.embl.reference[indi].processing)>1) {
			sscanf(data.embl.reference[indi].processing,
				"%d %d", &start, &end);
			end *= -1; /* will get negative from sscanf */
			sprintf(temp, "%d  (bases %d to %d)\n", (indi+1), start, end);
			data.gbk.reference[indi].ref
				= (char*)Dupstr(temp);
		} else {
			sprintf(temp, "%d\n", (indi+1));
			data.gbk.reference[indi].ref = (char*)Dupstr(temp);
		}

		if(Lenstr(data.embl.reference[indi].title)>1
		&&data.embl.reference[indi].title[0]!=';') {

			/* remove '"' and ';', if there is any */
			len = Lenstr(data.embl.reference[indi].title);
			if(len>2&&data.embl.reference[indi].title[0]=='"'
			&&data.embl.reference[indi].title[len-2]==';'
			&&data.embl.reference[indi].title[len-3]=='"') {
				data.embl.reference[indi].title[len-3]='\n';
				data.embl.reference[indi].title[len-2]='\0';
				data.gbk.reference[indi].title = (char*)
				Dupstr(data.embl.reference[indi].title+1);
				data.embl.reference[indi].title[len-3]='"';
				data.embl.reference[indi].title[len-2]=';';
			} else data.gbk.reference[indi].title = (char*)
				Dupstr(data.embl.reference[indi].title);

		} else data.gbk.reference[indi].title
			= (char*)Dupstr("\n");

		/* GenBank AUTHOR */
		if(Lenstr(data.embl.reference[indi].author)>1)

			data.gbk.reference[indi].author
		= etog_author(data.embl.reference[indi].author);

		else data.gbk.reference[indi].author
			= (char*)Dupstr("\n");

		if(Lenstr(data.embl.reference[indi].journal)>1)

			data.gbk.reference[indi].journal
		= (char*)etog_journal(data.embl.reference[indi].journal);

		else data.gbk.reference[indi].journal
			= (char*)Dupstr("\n");

		data.gbk.reference[indi].standard
			= (char*)Dupstr("\n");
	}
}
/* -----------------------------------------------------------------
*	Function etog_author().
*		Convert EMBL author format to Genbank author format.
*/
char
*etog_author(string)
char	*string;
{
	int	indi, /*indj,*/ indk, len, index;
	char	token[TOKENNUM], *author;
/* 	void	Append(); */

	author=(char*)Dupstr("");
	for(indi=index=0,len=Lenstr(string)-1; indi<len; indi++, index++)	{
		if(string[indi]==','||string[indi]==';')	{
			token[index--]='\0';
			if(string[indi]==',') {
                            if(Lenstr(author)>0) Append(&(author), ",");
                        }
			else if(Lenstr(author)>0) {
                            Append(&(author), " and");
                        }
			/* search backward to find the first blank and replace the blank by ',' */
			for(indk=0; index>0&&indk==0; index--)
				if(token[index]==' ')	{
					token[index]=',';
					indk=1;
				}
			Append(&(author), token);
			index = (-1);
		} else token[index]=string[indi];
	}
	Append(&(author), "\n");
	return(author);
}
/* ------------------------------------------------------------
*	Function etog_journal().
*		Convert jpurnal part from EMBL to GenBank format.
*/
char
*etog_journal(string)
char	*string;
{
	int	indi, len, index;
	char	*journal, *new_journal;
	char	token[TOKENNUM];
/* 	void	Freespace(), Append(); */

	sscanf(string, "%s", token);
	if(Cmpstr(token, "(in)")==EQ||Cmpstr(token, "Submitted")==EQ
	||Cmpstr(token, "Unpublished")==EQ)	{
		/* remove '.' */
		string[Lenstr(string)-2]='\n';
		string[Lenstr(string)-1]='\0';
		new_journal = (char*)Dupstr(string);
		string[Lenstr(string)-1]='.';
		string[Lenstr(string)]='\n';
		return(new_journal);
	}
	journal=(char*)Dupstr(string);
	for(indi=0, len=Lenstr(journal); indi<len; indi++)	{
		if(journal[indi]==':')	{
			journal[indi]='\0';
			new_journal=(char*)Dupstr(journal);
			journal[indi]=':';
			Append(&new_journal, ", ");
			index=indi+1;
		}
		if(journal[indi]=='(')	{
			journal[indi]='\0';
			Append(&new_journal, journal+index);
			Append(&new_journal, " ");
			journal[indi]='(';
			index=indi;
		}
		if(journal[indi]=='.')	{
			journal[indi]='\0';
			Append(&new_journal, journal+index);
			journal[indi]='.';
		}
	}
	Freespace(&journal);
	Append(&new_journal, "\n");
	return(new_journal);
}
/* ---------------------------------------------------------------
*	Function etog_comments().
*		Convert comment part from EMBL to GenBank.
*/
void
etog_comments()	{

/* 	int	Lenstr(); */
/* 	void	replace_entry(); */
/* 	char	*Dupsstr(); */

	/* RDP defined Organism Information comments */
	data.gbk.comments.orginf.exist = data.embl.comments.orginf.exist;

	if(Lenstr(data.embl.comments.orginf.source)>1)
		replace_entry(&(data.gbk.comments.orginf.source),
			data.embl.comments.orginf.source);

	if(Lenstr(data.embl.comments.orginf.cc)>1)
		replace_entry(&(data.gbk.comments.orginf.cc),
			data.embl.comments.orginf.cc);

	if(Lenstr(data.embl.comments.orginf.formname)>1)
		replace_entry(&(data.gbk.comments.orginf.formname),
			data.embl.comments.orginf.formname);

	if(Lenstr(data.embl.comments.orginf.nickname)>1)
		replace_entry(&(data.gbk.comments.orginf.nickname),
			data.embl.comments.orginf.nickname);

	if(Lenstr(data.embl.comments.orginf.commname)>1)
		replace_entry(&(data.gbk.comments.orginf.commname),
			data.embl.comments.orginf.commname);

	if(Lenstr(data.embl.comments.orginf.hostorg)>1)
		replace_entry(&(data.gbk.comments.orginf.hostorg),
			data.embl.comments.orginf.hostorg);

	/* RDP defined Sequence Information comments */
	data.gbk.comments.seqinf.exist = data.embl.comments.seqinf.exist;

	if(Lenstr(data.embl.comments.seqinf.RDPid)>1)
		replace_entry(&(data.gbk.comments.seqinf.RDPid),
			data.embl.comments.seqinf.RDPid);

	if(Lenstr(data.embl.comments.seqinf.gbkentry)>1)
		replace_entry(&(data.gbk.comments.seqinf.gbkentry),
			data.embl.comments.seqinf.gbkentry);

	if(Lenstr(data.embl.comments.seqinf.methods)>1)
		replace_entry(&(data.gbk.comments.seqinf.methods),
			data.embl.comments.seqinf.methods);

	data.gbk.comments.seqinf.comp5 = data.embl.comments.seqinf.comp5;

	data.gbk.comments.seqinf.comp3 = data.embl.comments.seqinf.comp3;

	/* other comments */
	if(Lenstr(data.embl.comments.others)>1)
		replace_entry(&(data.gbk.comments.others),
			data.embl.comments.others);
}
/* ----------------------------------------------------------------
*	Function genbank_to_embl().
*		Convert from genbank to EMBL.
*/
void
genbank_to_embl(inf, outf)
char	*inf, *outf;
{
	FILE	*ifp, *ofp;
	char	temp[TOKENNUM];
/* 	char	*Dupstr(); */
/* 	void	init(), init_embl(), embl_out(); */
/* 	void	init_genbank(), error(); */
/* 	int	gtoe(); */

	if((ifp=fopen(inf, "r"))==NULL)	{
		sprintf(temp,
		"Cannot open input file %s, exit\n", inf);
		error(132, temp);
	}
	if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp,
		"Cannot open output file %s, exit\n", outf);
		error(133, temp);
	}
	init();
	init_genbank();
	init_embl();
	rewind(ifp);
	while(genbank_in(ifp)!=EOF)	{
		data.numofseq++;
		if(gtoe()) embl_out(ofp);
		init_genbank();
		init_embl();
	}

#ifdef log
	fprintf(stderr,
	"Total %d sequences have been processed\n",
		data.numofseq);
#endif

	fclose(ifp);	fclose(ofp);
}
/* ------------------------------------------------------------
*	Function gtoe().
*		Genbank to EMBL.
*/
int
gtoe()	{
/* 	void	genbank_key_word(), replace_entry(); */
/* 	void	Cpystr(), Upper_case(); */
/* 	void	genbank_key_word(), Append(), Append_char(); */
/* 	void 	gtoe_comments(), gtoe_reference(); */
	char	token[TOKENNUM], temp[LONGTEXT], rdpid[TOKENNUM];
/* 	char	*Dupstr(), *today_date(), *genbank_date(); */
	int	indi /*,length*/;

	genbank_key_word(data.gbk.locus, 0, token, TOKENNUM);
	Cpystr(temp, token);
	/* Adjust short-id, EMBL short_id always upper case */
	Upper_case(temp);
	if(Lenstr(token)>9) indi=9; else indi=Lenstr(token);
	for(; indi<10; indi++)	temp[indi]=' ';
	sprintf(temp+10, "preliminary; RNA; UNA; %d BP.\n",
		data.seq_length);
	replace_entry(&(data.embl.id), temp);

	/* accession number */
	if(Lenstr(data.gbk.accession)>1)
		/* take just the accession num, no version num. */
		replace_entry(&(data.embl.accession),
			data.gbk.accession);

	/* date */
	if(Lenstr(data.gbk.locus)<61)	{
		Cpystr(token, genbank_date(today_date()));
	} else	{
		for(indi=0; indi<11; indi++)
			token[indi]=data.gbk.locus[indi+50];
		token[11]='\0';
	}
	sprintf(temp,
		"%s (Rel. 1, Last updated, Version 1)\n", token);
	replace_entry(&(data.embl.dateu), temp);
	sprintf(temp, "%s (Rel. 1, Created)\n", token);
	replace_entry(&(data.embl.datec), temp);

	/* description */
	if(Lenstr(data.gbk.definition)>1)
		replace_entry(&(data.embl.description),
			data.gbk.definition);

	/* EMBL KW line */
	if(Lenstr(data.gbk.keywords)>1)	{

		replace_entry(&(data.embl.keywords),
			data.gbk.keywords);
		Append_char(&(data.embl.keywords), '.');

	} else replace_entry(&(data.embl.keywords), ".\n");

	/* EMBL OS line */
	if(Lenstr(data.gbk.organism)>1)
		replace_entry(&(data.embl.os),
			data.gbk.organism);

	/* reference */
	gtoe_reference();

	/* EMBL DR line */
	sscanf(data.gbk.locus, "%s", token);	/* short_id */
	if(Lenstr(data.gbk.comments.seqinf.RDPid)>1)	{
		sscanf(data.gbk.comments.seqinf.RDPid, "%s", rdpid);
		sprintf(temp, "RDP; %s; %s.\n", rdpid, token);
	} else	sprintf(temp, "RDP; %s.\n", token);
	replace_entry(&(data.embl.dr), temp);

	gtoe_comments();

	return(1);
}
/* ------------------------------------------------------------------
*	Function gtoe_reference().
*		Convert references from GenBank to EMBL.
*/
void
gtoe_reference()	{

	int	indi, start, end, refnum;
	char	token[TOKENNUM];
	char	t1[TOKENNUM], t2[TOKENNUM], t3[TOKENNUM];
/* 	char	*gtoe_author(), *gtoe_journal(); */
/* 	void	embl_verify_title(), Append_char(); */

	data.embl.numofref=data.gbk.numofref;

	if(data.gbk.numofref>0)	{
		data.embl.reference = (Emblref*)malloc
			(sizeof(Emblref)*data.gbk.numofref);
	}

	for(indi=0; indi<data.gbk.numofref; indi++)	{

		data.embl.reference[indi].title
			= (char*)Dupstr
			(data.gbk.reference[indi].title);

		embl_verify_title(indi);

		data.embl.reference[indi].journal
			= (char*)gtoe_journal
			(data.gbk.reference[indi].journal);

		/* append a '.' if there is not any. */
		Append_char(&(data.embl.reference[indi].journal), '.');

		data.embl.reference[indi].author
			= gtoe_author(data.gbk.reference[indi].author);

		/* append a ';' if there is not any. */
		Append_char(&(data.embl.reference[indi].author), ';');

		/* create processing information */
		start=end=0;
		if (data.gbk.reference[indi].ref) {
			sscanf(data.gbk.reference[indi].ref,
				"%d %s %d %s %d %s",
				&refnum, t1, &start, t2, &end, t3);
		}else{
			start = 0; end = 0;
		}

		if(start!=0||end!=0)
		{
			sprintf(token, "%d-%d\n",
				start, end);
			data.embl.reference[indi].processing
				= (char*)Dupstr(token);
		} else 	/* else change nothing about RP */
			data.embl.reference[indi].processing
				= (char*)Dupstr("\n");

	}	/* for each reference */
}
/* ----------------------------------------------------------------
*	Function gtoe_author().
*		Convert GenBank author to EMBL author.
*/
char
*gtoe_author(author)
char	*author;
{
	int	indi, len, index, odd;
	char	*auth, *string;
/* 	void	Append(), Freespace(); */

	/* replace " and " by ", " */
	auth = (char*)Dupstr(author);
	if((index=find_pattern(auth, " and "))>0)	{
		auth[index]='\0';
		string=(char*)Dupstr(auth);
		auth[index]=' '; /* remove '\0' for free space later */
		Append(&(string), ",");
		Append(&(string), auth+index+4);
	} else string=(char*)Dupstr(author);

	for(indi=0,len=Lenstr(string), odd=1; indi<len; indi++) {
		if(string[indi]==',') {
			if (odd)	{
				string[indi] = ' ';
				odd=0;
			}
            else {
                odd=1;
            }
        }
    }

	Freespace(&auth);
	return(string);

}
/* ------------------------------------------------------------
*	Function gtoe_journal().
*		Convert GenBank journal to EMBL journal.
*/
char
*gtoe_journal(string)
char	*string;
{
	char	token[TOKENNUM], *journal;
	int	indi, indj, index, len;
/* 	void	Append_char(); */

	sscanf(string, "%s", token);
	if(Cmpstr(token, "(in)")==EQ||Cmpstr(token, "Unpublished")==EQ
	||Cmpstr(token, "Submitted")==EQ)	{
		journal=(char*)Dupstr(string);
		Append_char(&journal, '.');
		return(journal);
	}

	journal=(char*)Dupstr(string);
	for(indi=indj=index=0, len=Lenstr(journal); indi<len; indi++, indj++) {
		if(journal[indi]==',')	{
			journal[indi]=':';
			indi++;	/* skip blank after ',' */
			index=1;
		} else if(journal[indi]==' '&&index) {
			indj--;
		} else journal[indj]=journal[indi];
	}

	journal[indj]='\0';
	Append_char(&journal, '.');
	return(journal);
}
/* ---------------------------------------------------------------
*	Function gtoe_comments().
*		Convert comment part from GenBank to EMBL.
*/
void
gtoe_comments()	{
/* 	int	Lenstr(); */
/* 	void	replace_entry(); */
/* 	char	*Dupsstr(); */

	/* RDP defined Organism Information comments */
	data.embl.comments.orginf.exist = data.gbk.comments.orginf.exist;

	if(Lenstr(data.gbk.comments.orginf.source)>1)
		replace_entry(&(data.embl.comments.orginf.source),
			data.gbk.comments.orginf.source);

	if(Lenstr(data.gbk.comments.orginf.cc)>1)
		replace_entry(&(data.embl.comments.orginf.cc),
			data.gbk.comments.orginf.cc);

	if(Lenstr(data.gbk.comments.orginf.formname)>1)
		replace_entry(&(data.embl.comments.orginf.formname),
			data.gbk.comments.orginf.formname);

	if(Lenstr(data.gbk.comments.orginf.nickname)>1)
		replace_entry(&(data.embl.comments.orginf.nickname),
			data.gbk.comments.orginf.nickname);

	if(Lenstr(data.gbk.comments.orginf.commname)>1)
		replace_entry(&(data.embl.comments.orginf.commname),
			data.gbk.comments.orginf.commname);

	if(Lenstr(data.gbk.comments.orginf.hostorg)>1)
		replace_entry(&(data.embl.comments.orginf.hostorg),
			data.gbk.comments.orginf.hostorg);

	/* RDP defined Sequence Information comments */
	data.embl.comments.seqinf.exist = data.gbk.comments.seqinf.exist;

	if(Lenstr(data.gbk.comments.seqinf.RDPid)>1)
		replace_entry(&(data.embl.comments.seqinf.RDPid),
			data.gbk.comments.seqinf.RDPid);

	if(Lenstr(data.gbk.comments.seqinf.gbkentry)>1)
		replace_entry(&(data.embl.comments.seqinf.gbkentry),
			data.gbk.comments.seqinf.gbkentry);

	if(Lenstr(data.gbk.comments.seqinf.methods)>1)
		replace_entry(&(data.embl.comments.seqinf.methods),
			data.gbk.comments.seqinf.methods);

	data.embl.comments.seqinf.comp5 = data.gbk.comments.seqinf.comp5;

	data.embl.comments.seqinf.comp3 = data.gbk.comments.seqinf.comp3;

	/* other comments */
	if(Lenstr(data.gbk.comments.others)>1)
		replace_entry(&(data.embl.comments.others),
			data.gbk.comments.others);
}
/* ---------------------------------------------------------------
*	Function macke_to_embl().
*		Convert from macke to EMBL.
*/
void
macke_to_embl(inf, outf)
char	*inf, *outf;
{
	FILE	*ifp1, *ifp2, *ifp3, *ofp;
	char	temp[TOKENNUM];
/* 	void	init(), init_seq_data(), init_genbank(); */
/* 	void	init_macke(), init_embl(), error(), embl_out(); */
/* 	int	mtog(), gtoe(), partial_mtoe(); */

	if((ifp1=fopen(inf, "r"))==NULL
	||(ifp2=fopen(inf, "r"))==NULL||
	(ifp3=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s\n", inf);
		error(99, temp);
	}
	if(Lenstr(outf)<=0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s\n", outf);
		error(43, temp);
	}
	init();
	init_seq_data();
	init_genbank();
	init_macke();
	init_embl();

#ifdef log
	fprintf(stderr, "Start converting...\n");
#endif

	while(macke_in(ifp1, ifp2, ifp3)!=EOF)	{

		data.numofseq++;

		/* partial_mtoe() is particularly handling
         * subspecies information, not converting whole
         * macke format to embl */

		if(mtog()&&gtoe()&&partial_mtoe()) embl_out(ofp);
		else error(14,
		"Conversion from macke to embl fails, Exit");

		init_genbank();
		init_embl();
		init_macke();

#ifdef log
		if((data.numofseq % 50)==0)
		fprintf(stderr, "%d sequences are converted...\n",
				data.numofseq);
#endif

	}

#ifdef log
	fprintf(stderr,
	"Total %d sequences have been processed\n", data.numofseq);
#endif

}
/* --------------------------------------------------------------
*	Function partial_mtoe().
*		Handle subspecies information when converting from
*			Macke to EMBL.
*/
int partial_mtoe()	{

	int	indj, indk;
/* 	int	not_ending_mark(); */
/* 	void	Freespace(), Append(), Append_rm_eoln(); */

	if(Lenstr(data.macke.strain)>1) {
		if((indj=find_pattern(data.embl.comments.others, "*source:"))>=0 &&
           (indk=find_pattern (data.embl.comments.others+indj, "strain="))>=0)
        {
			; /* do nothing */
        }
		else {
			if(Lenstr(data.embl.comments.others)<=1) Freespace(&(data.embl.comments.others));
			Append(&(data.embl.comments.others), "*source: strain=");
			Append(&(data.embl.comments.others), data.macke.strain);
			if (not_ending_mark(data.embl.comments.others[Lenstr(data.embl.comments.others)-2])) {
				Append_rm_eoln (&(data.embl.comments.others), ";\n");
            }
		}
    }

	if(Lenstr(data.macke.subspecies)>1) {

		if((indj=find_pattern (data.embl.comments.others, "*source:"))>=0 &&
           ((indk=find_pattern(data.embl.comments.others+indj, "subspecies="))>=0 ||
            (indk=find_pattern(data.embl.comments.others+indj, "sub-species="))>=0 ||
            (indk=find_pattern(data.embl.comments.others+indj, "subsp.="))>=0))
        {
			;/* do nothing */
        }
		else {
			if (Lenstr(data.embl.comments.others)<=1) Freespace (&(data.embl.comments.others));

			Append(&(data.embl.comments.others), "*source: subspecies=");
			Append(&(data.embl.comments.others), data.macke.subspecies);

			if (not_ending_mark(data.embl.comments.others[Lenstr(data.embl.comments.others)-2])) {
                Append_rm_eoln (&(data.embl.comments.others), ";\n");
            }
		}
    }

	return(1);
}
