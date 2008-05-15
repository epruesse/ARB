/* -------------------- Macke related subroutines -------------- */

#include <stdio.h>
#include "convert.h"
#include "global.h"

#define MACKELIMIT 10000

/* -------------------------------------------------------------
*	Function init_mache().
*	Initialize macke entry.
*/
void
init_macke()	{

/* 	void	Freespace(); */
/* 	char	*Dupstr(); */
	int	indi;

/* initialize macke format */
	Freespace(&(data.macke.seqabbr));
	Freespace(&(data.macke.name));
	Freespace(&(data.macke.atcc));
	Freespace(&(data.macke.rna));
	Freespace(&(data.macke.date));
	Freespace(&(data.macke.nbk));
	Freespace(&(data.macke.acs));
	Freespace(&(data.macke.who));
	for(indi=0; indi<data.macke.numofrem; indi++)	{
		Freespace(&(data.macke.remarks[indi]));
	}
	Freespace((char**)&(data.macke.remarks));
	Freespace(&(data.macke.journal));
	Freespace(&(data.macke.title));
	Freespace(&(data.macke.author));
	Freespace(&(data.macke.strain));
	Freespace(&(data.macke.subspecies));

	data.macke.seqabbr=Dupstr("");
	data.macke.name=Dupstr("\n");
	data.macke.atcc=Dupstr("\n");
	data.macke.rna=Dupstr("\n");
	data.macke.date=Dupstr("\n");
	data.macke.nbk=Dupstr("\n");
	data.macke.acs=Dupstr("\n");
	data.macke.who=Dupstr("\n");
	data.macke.numofrem=0;
	data.macke.rna_or_dna='d';
	data.macke.journal=Dupstr("\n");
	data.macke.title=Dupstr("\n");
	data.macke.author=Dupstr("\n");
	data.macke.strain=Dupstr("\n");
	data.macke.subspecies=Dupstr("\n");
}
/* -------------------------------------------------------------
 *	Function macke_in().
 *		Read in one sequence data from Macke file.
 */
char
macke_in(fp1, fp2, fp3)
     FILE_BUFFER  fp1;
     FILE_BUFFER  fp2;
     FILE_BUFFER  fp3;
{
	static	char	line1[LINENUM];
	static	char	line2[LINENUM];
	static	char	line3[LINENUM];
	static	int	first_time = 1;
	char	oldname[TOKENNUM], name[TOKENNUM];
	char	/*seq[LINENUM], */ key[TOKENNUM], temp[LINENUM];
	char	*eof1, *eof2, *eof3;
/* 	char	*Reallocspace(), *Dupstr(); */
/* 	char	*Fgetline(), *macke_origin(); */
/* 	char	*macke_one_entry_in(); */
	int	numofrem = 0 /*, seqnum*/;
/* 	int	indj; */
	int	index;
/* 	int	Cmpstr(), macke_abbrev(); */
/* 	void	Freespace(), warning(); */

	/* file 1 points to seq. information */
	/* file 2 points to seq. data */
	/* file 3 points to seq. names */
	if(first_time)	{
		/* skip to next "#:" line */
		for(eof1=Fgetline(line1, LINENUM, fp1);
		eof1!=NULL&&(line1[0]!='#'||line1[1]!=':');
		eof1=Fgetline(line1, LINENUM, fp1)) ;

		/* skip all "#" lines to where seq. data is */
		for(eof2=Fgetline(line2, LINENUM, fp2);
		eof2!=NULL
		&&(line2[0]=='\n'||line2[0]==' '||line2[0]=='#');
		eof2=Fgetline(line2, LINENUM, fp2)) ;

		/* skip to "#=" lines */
		for(eof3=Fgetline(line3, LINENUM, fp3);
		eof3!=NULL&&(line3[0]!='#'||line3[1]!='=');
		eof3=Fgetline(line3, LINENUM, fp3)) ;

		first_time = 0;
	} else
		eof3=Fgetline(line3, LINENUM, fp3);

	/* end of reading seq names */
	if(line3[0]!='#'||line3[1]!='=') return(EOF);

	/* skip to next "#:" line or end of file */
	if(line1[0]!='#'||line1[1]!=':')	{
		for( ;eof1!=NULL&&(line1[0]!='#'||line1[1]!=':');
		eof1=Fgetline(line1, LINENUM, fp1)) ;
	}

	/* read in seq name */
	index = macke_abbrev(line3, oldname, 2);
	Freespace(&data.macke.seqabbr);
	data.macke.seqabbr=Dupstr(oldname);

	/* read seq. information */
	for(index=macke_abbrev(line1, name, 2);
	eof1!=NULL&&line1[0]=='#'&&line1[1]==':'
	&&Cmpstr(name, oldname)==EQ; )
	{
		index = macke_abbrev(line1, key, index);
		if(Cmpstr(key, "name")==EQ)	{
			eof1 = macke_one_entry_in(fp1, "name", oldname,
				&(data.macke.name), line1, index);
		} else if(Cmpstr(key, "atcc")==EQ)	{
			eof1 = macke_one_entry_in(fp1, "atcc", oldname,
				&(data.macke.atcc), line1, index);
		} else if(Cmpstr(key, "rna")==EQ)	{
			/* old version entry */
			eof1 = macke_one_entry_in(fp1, "rna", oldname,
				&(data.macke.rna), line1, index);
		} else if(Cmpstr(key, "date")==EQ) 	{
			eof1 = macke_one_entry_in(fp1, "date", oldname,
				&(data.macke.date), line1, index);
		} else if(Cmpstr(key, "nbk")==EQ)	{
			/* old version entry */
			eof1 = macke_one_entry_in(fp1, "nbk", oldname,
				&(data.macke.nbk), line1, index);
		} else if(Cmpstr(key, "acs")==EQ)	{
			eof1 = macke_one_entry_in(fp1, "acs", oldname,
				&(data.macke.acs), line1, index);
		} else if(Cmpstr(key, "subsp")==EQ)	{
			eof1 = macke_one_entry_in(fp1, "subsp", oldname,
				&(data.macke.subspecies), line1, index);
		} else if(Cmpstr(key, "strain")==EQ)	{
			eof1 = macke_one_entry_in(fp1, "strain", oldname,
				&(data.macke.strain), line1, index);
		} else if(Cmpstr(key, "auth")==EQ)	{
			eof1 = macke_one_entry_in(fp1, "auth", oldname,
				&(data.macke.author), line1, index);
		} else if(Cmpstr(key, "title")==EQ)	{
			eof1 = macke_one_entry_in(fp1, "title", oldname,
				&(data.macke.title), line1, index);
		} else if(Cmpstr(key, "jour")==EQ)	{
			eof1 = macke_one_entry_in(fp1, "jour", oldname,
				&(data.macke.journal), line1, index);
		} else if(Cmpstr(key, "who")==EQ)	{
			eof1 = macke_one_entry_in(fp1, "who", oldname,
				&(data.macke.who), line1, index);
		} else if(Cmpstr(key, "rem")==EQ)	{
			data.macke.remarks = (char**)Reallocspace((char*)data.macke.remarks, (unsigned) (sizeof(char*)*(numofrem+1)));
			data.macke.remarks[numofrem++] = Dupstr(line1+index);
			eof1=Fgetline(line1, LINENUM, fp1);
		} else {
			sprintf(temp,
			"Unidentified AE2 key word #%s#\n", key);
			warning(144, temp);
			eof1=Fgetline(line1, LINENUM, fp1);
		}
		if(eof1!=NULL&&line1[0]=='#'&&line1[1]==':')
			index=macke_abbrev(line1, name, 2);
		else index = 0;
	}
	data.macke.numofrem = numofrem;

	/* read in sequence data */
	eof2=macke_origin(oldname, line2, fp2);

	return(EOF+1);
}
/* -----------------------------------------------------------------
 *	Function macke_one_entry_in().
 *		Get one Macke entry.
 */
char
*macke_one_entry_in(fp, key, oldname, var, line, index)
     FILE_BUFFER  fp;
     const char	 *key;
     char	     *oldname, **var, *line;
     int	      index;
{
	char *eof;

	if(Lenstr((*var))>1) Append_rp_eoln(var, line+index);
	else replace_entry(var, line+index);

	eof = (char*)macke_continue_line(key, oldname, var, line, fp);

	return(eof);

}
/* --------------------------------------------------------------
 *	Function macke_continue_line().
 *		Append macke continue line.
 */
char *macke_continue_line(key, oldname, var, line, fp)
     const char	*key;
     char	    *oldname, **var, *line;
     FILE_BUFFER fp;
{
	char	*eof, name[TOKENNUM], newkey[TOKENNUM];
	int	index;
/* 	int	macke_abbrev(); */
/* 	void	Append_rp_eoln(); */

	for(eof=Fgetline(line, LINENUM, fp);
	eof!=NULL; eof=Fgetline(line, LINENUM, fp))	{
		if(Lenstr(line)<=1)	continue;

		index = macke_abbrev(line, name, 0);
		if(Cmpstr(name, oldname)!=EQ)	break;

		index = macke_abbrev(line, newkey, index);
		if(Cmpstr(newkey, key)!=EQ)	break;

		Append_rp_eoln(var, line+index);
	}

	return(eof);
}
/* ----------------------------------------------------------------
 *	Function macke_origin().
 *		Read in sequence data in macke file.
 */
char
*macke_origin(key, line, fp)
     char *key, *line;
     FILE_BUFFER	fp;
{
	int	  index, indj, seqnum;
	char *eof, name[TOKENNUM], seq[LINENUM];

	/* read in seq. data */
	data.seq_length=0;
	/* read in line by line */

	index=macke_abbrev(line, name, 0);
	eof=line;
	for(; eof!=NULL&&Cmpstr(key, name)==EQ; ){
		sscanf(line+index, "%d%s", &seqnum, seq);
		for(indj=data.seq_length; indj<seqnum; indj++)
			if(indj<data.max)
				data.sequence[data.seq_length++]
					= '.';
			else	{
				data.max += 100;
				data.sequence = (char*)Reallocspace
				(data.sequence, (unsigned)
					(sizeof(char)*data.max));
				data.sequence[data.seq_length++]
					= '.';
			}
		for(indj=0; seq[indj]!='\n'&&seq[indj]!='\0'; indj++)
			if(data.seq_length<data.max)
				data.sequence[data.seq_length++]
					= seq[indj];
			else	{
				data.max += 100;
				data.sequence = (char*)Reallocspace
				(data.sequence, (unsigned)
					(sizeof(char)*data.max));
				data.sequence[data.seq_length++]
					= seq[indj];
			}
		data.sequence[data.seq_length] = '\0';
		eof=Fgetline(line, LINENUM, fp);
		if(eof!=NULL) index = macke_abbrev(line, name, 0);

	}	/* reading seq loop */

	return(eof);
}
/* --------------------------------------------------------------
*	Function macke_abbrev().
*		Find the key in Macke line.
*/
int
macke_abbrev(line, key, index)
char	*line, *key;
int	index;
{
	int	indi;

	/* skip white space */
	index = Skip_white_space(line, index);

	for(indi=index; line[indi]!=' '&&line[indi]!=':'
	&&line[indi]!='\t'&&line[indi]!='\n'&&line[indi]!='\0';
	indi++)
		key[indi-index]=line[indi];

	key[indi-index]='\0';
	return(indi+1);
}
/* --------------------------------------------------------------
*	Function macke_rem_continue_line().
*		If there is 3 blanks at the beginning of the line,
*			it is continued line.
*/
int
macke_rem_continue_line(strings, index)
char	**strings;
int	index;
{
	if(strings[index][0]==':'&&strings[index][1]==' '
	&&strings[index][2]==' ')
		return(1);
	else return(0);
}
/* -------------------------------------------------------------
*	Function macke_in_name().
*		Read in next sequence name and data only.
*/
char
macke_in_name(fp)
FILE_BUFFER	fp;
{
	static	char	line[LINENUM];
	static	int	first_time = 1;
	char	/*oldname[TOKENNUM],*/ name[TOKENNUM];
	char	seq[LINENUM]/*, key[TOKENNUM]*/;
	char	*eof;
/* 	char	*Reallocspace(), *Dupstr(); */
/* 	char	*Fgetline(); */
	int	numofrem = 0, seqnum;
	int	index, indj;
/* 	int	Cmpstr(), macke_abbrev(); */
/* 	void	Freespace(); */

/* skip other information, file index points to seq. data */
	if(first_time)	{

		/* skip all "#" lines to where seq. data is */
		for(eof=Fgetline(line, LINENUM, fp);
		eof!=NULL&&line[0]=='#';
		eof=Fgetline(line, LINENUM, fp)) ;

		first_time = 0;

	} else if(line[0]==EOF) {

		line[0]=EOF+1;
		first_time = 1;
		return(EOF);
	}

	/* read in seq. data */
	data.macke.numofrem = numofrem;
	data.seq_length=0;

	/* read in line by line */
	Freespace(&(data.macke.seqabbr));
	for(index=macke_abbrev(line, name, 0),
	data.macke.seqabbr=Dupstr(name);
	line[0]!=EOF&&Cmpstr(data.macke.seqabbr, name)==EQ; )
	{
		sscanf(line+index, "%d%s", &seqnum, seq);
		for(indj=data.seq_length; indj<seqnum; indj++)
			if(data.seq_length<data.max)
				data.sequence[data.seq_length++]
					= '.';
			else	{
				data.max += 100;
				data.sequence = (char*)Reallocspace
				(data.sequence, (unsigned)
					(sizeof(char)*data.max));
				data.sequence[data.seq_length++]
					= '.';
			}
		for(indj=0; seq[indj]!='\n'&&seq[indj]!='\0';
		indj++)
		{
			if(data.seq_length<data.max)
				data.sequence[data.seq_length++]
					= seq[indj];
			else	{
				data.max += 100;
				data.sequence = (char*)Reallocspace
				(data.sequence, (unsigned)
					(sizeof(char)*data.max));
				data.sequence[data.seq_length++]
					= seq[indj];
			}
		}
		data.sequence[data.seq_length] = '\0';

		if((eof=Fgetline(line, LINENUM, fp))!=NULL)
			index = macke_abbrev(line, name, 0);
		else line[0] = EOF;

	}	/* reading seq loop */

	return(EOF+1);
}
/* ------------------------------------------------------------
*	Fucntion macke_out_header().
*		Output the Macke format header.
*/
void
macke_out_header(fp)
FILE	*fp;
{
	char	*date;
/* 	void	Freespace(), Cypstr(); */

	fprintf(fp, "#-\n#-\n#-\teditor\n");
	date = today_date();
	fprintf(fp, "#-\t%s#-\n#-\n", date);
	Freespace(&date);
}
/* ------------------------------------------------------------
*	Fucntion macke_out0().
*		Output the Macke format each sequence format.
*/
void
macke_out0(fp, format)
FILE	*fp;
int	format;
{
/* 	int	Lenstr(); */
	char	token[TOKENNUM], direction[TOKENNUM];
/* 	void	Cpystr(); */

	if(format==PROTEIN)	{
		Cpystr(token, "pro");
		Cpystr(direction, "n>c");
	} else {
		Cpystr(direction, "5>3");
		if(data.macke.rna_or_dna=='r')	Cpystr(token, "rna");
		else Cpystr(token, "dna");
	}
	if(data.numofseq==1) {
		fprintf(fp, "#-\tReference sequence:  %s\n",
			data.macke.seqabbr);
		fprintf(fp, "#-\tAttributes:\n");

		if(Lenstr(data.macke.seqabbr)<8)
			fprintf(fp,
	"#=\t\t%s\t \tin  out  vis  prt   ord  %s  lin  %s  func ref\n",
			data.macke.seqabbr, token, direction);
		else
			fprintf(fp,
	"#=\t\t%s\tin  out  vis  prt   ord  %s  lin  %s  func ref\n",
			data.macke.seqabbr, token, direction);

	} else
		if(Lenstr(data.macke.seqabbr)<8)
			fprintf(fp,
	"#=\t\t%s\t\tin  out  vis  prt   ord  %s  lin  %s  func\n",
			data.macke.seqabbr, token, direction);
		else
			fprintf(fp,
	"#=\t\t%s\tin  out  vis  prt   ord  %s  lin  %s  func\n",
			data.macke.seqabbr, token, direction);
}
/* ---------------------------------------------------------------
*	Fucntion macke_out1().
*		Output sequences information.
*/
void macke_out1(fp)
FILE	*fp;
{
    /* 	void	macke_print_line_78(), macke_print_keyword_rem(); */
	char temp[LINENUM];
 	int	 indi;
    /* 	int	indj, indk, indl, len, maxc, lineno; */
    /* 	int	last_word(), Lenstr(); */
    /* 	int	macke_in_one_line(); */

	if(Lenstr(data.macke.name)>1)	{
		sprintf(temp, "#:%s:name:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.name);
	}
	if(Lenstr(data.macke.strain)>1)	{
		sprintf(temp, "#:%s:strain:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.strain);
	}
	if(Lenstr(data.macke.subspecies)>1)	{
		sprintf(temp, "#:%s:subsp:", data.macke.seqabbr);
		macke_print_line_78
			(fp, temp, data.macke.subspecies);
	}
	if(Lenstr(data.macke.atcc)>1)	{
		sprintf(temp, "#:%s:atcc:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.atcc);
	}
	if(Lenstr(data.macke.rna)>1)	{
		/* old version entry */
		sprintf(temp, "#:%s:rna:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.rna);
	}
	if(Lenstr(data.macke.date)>1)	{
		sprintf(temp, "#:%s:date:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.date);
	}
	if(Lenstr(data.macke.acs)>1)	{
		sprintf(temp, "#:%s:acs:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.acs);
	}
	else	if(Lenstr(data.macke.nbk)>1)	{
		/* old version entry */
		sprintf(temp, "#:%s:acs:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.nbk);
	}
	if(Lenstr(data.macke.author)>1)	{
		sprintf(temp, "#:%s:auth:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.author);
	}
	if(Lenstr(data.macke.journal)>1)	{
		sprintf(temp, "#:%s:jour:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.journal);
	}
	if(Lenstr(data.macke.title)>1)	{
		sprintf(temp, "#:%s:title:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.title);
	}
	if(Lenstr(data.macke.who)>1)	{
		sprintf(temp, "#:%s:who:", data.macke.seqabbr);
		macke_print_line_78(fp, temp, data.macke.who);
	}

	/* print out remarks, wrap around if more than 78 columns */
	for(indi=0; indi<data.macke.numofrem; indi++)
	{
	/* Check if it is general comment or GenBank entry */
	/* if general comment, macke_in_one_line return(1). */
		if(macke_in_one_line(data.macke.remarks[indi]))
		{
			sprintf(temp,"#:%s:rem:", data.macke.seqabbr);
			macke_print_line_78
				(fp, temp, data.macke.remarks[indi]);
			continue;
		}

		/* if GenBank entry comments */
		macke_print_keyword_rem(indi, fp);

	}	/* for each remark */
}
/* ----------------------------------------------------------------
*	Function macke_print_keyword_rem().
*		Print out keyworded remark line in Macke file with
*			wrap around functionality.
*		(Those keywords are defined in GenBank COMMENTS by
*			RDP group)
*/
void
macke_print_keyword_rem(index, fp)
int	index;
FILE	*fp;
{
	int	indj, indk, indl, lineno, len, maxc;

	lineno = 0;
	len = Lenstr(data.macke.remarks[index])-1;
	for(indj=0; indj<len; indj+=(indk+1)) {
		indk= maxc =MACKEMAXCHAR-7
			-Lenstr(data.macke.seqabbr);
		if(lineno!=0) indk = maxc = maxc - 3;
		if((Lenstr(data.macke.remarks[index]+indj)-1)
		> maxc) {
			/* Search the last word */
			for(indk=maxc-1; indk>=0&&
			!last_word(data.macke.remarks
			[index][indk+indj]); indk--) ;

			if(lineno==0)	{
				fprintf(fp,"#:%s:rem:",
					data.macke.seqabbr);
				lineno++;
			} else
				fprintf(fp,"#:%s:rem::  ",
					data.macke.seqabbr);

			for(indl=0; indl<indk; indl++)
				fprintf(fp,"%c",
				data.macke.remarks[index][indj+indl]);

			if(data.macke.remarks[index][indj+indk]!=' ')
				fprintf(fp,"%c",
				data.macke.remarks[index][indj+indk]);
			fprintf(fp, "\n");

		} else if(lineno==0)
			fprintf(fp,"#:%s:rem:%s", data.macke.seqabbr,
				data.macke.remarks[index]+indj);
		       else
			fprintf(fp,"#:%s:rem::  %s",
				data.macke.seqabbr,
				data.macke.remarks[index]+indj);
	}	/* for every MACKEMAXCHAR columns */
}
/* ---------------------------------------------------------------
*	Function macke_print_line_78().
*		print a macke line and wrap around line after
*			78(MACKEMAXCHAR) column.
*/
void
macke_print_line_78(fp, line1, line2)
FILE	*fp;
char	*line1, *line2;
{
	int	len, indi, indj, indk, ibuf;

	for(indi=0, len=Lenstr(line2); indi<len; indi+=indj)	{

		indj=78-Lenstr(line1);

		if((Lenstr(line2+indi))>indj)	{

			ibuf = indj;

			for(; indj>0&&!last_word(line2[indi+indj]);
			indj--) ;

			if(indj==0) indj=ibuf;
			else if(line2[indi+indj+1]==' ') indj++;

			fprintf(fp, "%s", line1);

			for(indk=0; indk<indj; indk++)
				fprintf(fp, "%c", line2[indi+indk]);

			/* print out the last char if it is not blank */
			if(line2[indi+indj]==' ')
				indj++;

			fprintf(fp, "\n");

		} else fprintf(fp, "%s%s", line1, line2+indi);

	}	/* for loop */
}
/* -----------------------------------------------------------
*	Function macke_key_word().
*		Find the key in Macke line.
*/
int
macke_key_word(line, index, key, length)
char	*line;
int	index;
char	*key;
int	length;
{
	int	indi;

	if(line==NULL)	{ key[0]='\0'; return(index);	}

	/* skip white space */
	index = Skip_white_space(line, index);

	for(indi=index; (indi-index)<(length-1)&&line[indi]!=':'
	&&line[indi]!='\n'&&line[indi]!='\0'; indi++)
		key[indi-index]=line[indi];

	key[indi-index]='\0';

	return(indi+1);
}
/* ------------------------------------------------------------
 *	Function macke_in_one_line().
 *		Check if string should be in one line.
 */
int macke_in_one_line(string)
     char *string;
{
	char keyword[TOKENNUM];
	int	 iskey;

	macke_key_word(string, 0, keyword, TOKENNUM);
	iskey=0;
	if(Cmpstr(keyword, "KEYWORDS")==EQ)
		iskey=1;
	else if(Cmpstr(keyword, "GenBank ACCESSION")==EQ)
		iskey=1;
	else if(Cmpstr(keyword, "auth")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "title")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "jour")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "standard")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "Source of strain")==EQ)
		iskey=1;
	else if(Cmpstr(keyword, "Former name")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "Alternate name")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "Common name")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "Host organism")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "RDP ID")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "Sequencing methods")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "3' end complete")==EQ)
		iskey = 1;
	else if(Cmpstr(keyword, "5' end complete")==EQ)
		iskey = 1;

	/* is-key then could be more than one line */
	/* otherwise, must be in one line */
	if(iskey) return(0);
	else return(1);
}
/* --------------------------------------------------------------
*	Function macke_out2().
*		Output Macke format sequences data.
*/
void macke_out2(fp)
     FILE *fp;
{
    int  indj, indk;
    char temp[LINENUM];

    if (data.seq_length > MACKELIMIT) {
        sprintf(temp, "Lenght of sequence data is %d over AE2's limit %d.\n",
                data.seq_length, MACKELIMIT);
        warning(145, temp);
    }

    for (indk=indj=0; indk<data.seq_length; indk++)
    {
        if(indj==0) fprintf(fp,"%s%6d ", data.macke.seqabbr, indk);

        fputc(data.sequence[indk], fp);

        indj++;
        if(indj==50) { indj=0; fprintf(fp, "\n"); }

    } /* every line */

    if(indj!=0) fprintf(fp, "\n");
    /* every sequence */
}
