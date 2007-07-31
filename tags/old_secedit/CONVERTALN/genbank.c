/* -------------- genbank related subroutines ----------------- */

#include <stdio.h>
#include <ctype.h>
#include "convert.h"
#include "global.h"
#include <assert.h>

#define NOPERIOD	0
#define PERIOD		1

extern int	warning_out;
/* ------------------------------------------------------------
 *	Function init_genbank().
 *	Initialize genbank entry.
 */
void init_genbank()	{

/* 	void	Freespace(); */
/* 	char	*Dupstr(); */
	int	indi;

/* initialize genbank format */

	Freespace(&(data.gbk.locus));
	Freespace(&(data.gbk.definition));
	Freespace(&(data.gbk.accession));
	Freespace(&(data.gbk.keywords));
	Freespace(&(data.gbk.source));
	Freespace(&(data.gbk.organism));
	for(indi=0; indi<data.gbk.numofref; indi++)	{
		Freespace(&(data.gbk.reference[indi].ref));
		Freespace(&(data.gbk.reference[indi].author));
		Freespace(&(data.gbk.reference[indi].title));
		Freespace(&(data.gbk.reference[indi].journal));
		Freespace(&(data.gbk.reference[indi].standard));
	}
	Freespace((char**)&(data.gbk.reference));
	Freespace(&(data.gbk.comments.orginf.source));
	Freespace(&(data.gbk.comments.orginf.cc));
	Freespace(&(data.gbk.comments.orginf.formname));
	Freespace(&(data.gbk.comments.orginf.nickname));
	Freespace(&(data.gbk.comments.orginf.commname));
	Freespace(&(data.gbk.comments.orginf.hostorg));
	Freespace(&(data.gbk.comments.seqinf.RDPid));
	Freespace(&(data.gbk.comments.seqinf.gbkentry));
	Freespace(&(data.gbk.comments.seqinf.methods));
	Freespace(&(data.gbk.comments.others));
	data.gbk.locus=Dupstr("\n");
	data.gbk.definition=Dupstr("\n");
	data.gbk.accession=Dupstr("\n");
	data.gbk.keywords=Dupstr("\n");
	data.gbk.source=Dupstr("\n");
	data.gbk.organism=Dupstr("\n");
	data.gbk.numofref=0;
	data.gbk.comments.orginf.exist=0;
	data.gbk.comments.orginf.source=Dupstr("\n");
	data.gbk.comments.orginf.cc=Dupstr("\n");
	data.gbk.comments.orginf.formname=Dupstr("\n");
	data.gbk.comments.orginf.nickname=Dupstr("\n");
	data.gbk.comments.orginf.commname=Dupstr("\n");
	data.gbk.comments.orginf.hostorg=Dupstr("\n");
	data.gbk.comments.seqinf.exist=0;
	data.gbk.comments.seqinf.RDPid=Dupstr("\n");
	data.gbk.comments.seqinf.gbkentry=Dupstr("\n");
	data.gbk.comments.seqinf.methods=Dupstr("\n");
	data.gbk.comments.others=NULL;
	data.gbk.comments.seqinf.comp5=' ';
	data.gbk.comments.seqinf.comp3=' ';
}
/* -----------------------------------------------------------
 *	Function genbank_in().
 *		Read in one genbank entry.
 */
char genbank_in(fp)
     FILE_BUFFER fp;
{
    char	    line[LINENUM], key[TOKENNUM] /*, temp[LINENUM]*/;
    const char *eof;
    char	    eoen;
    /*     char	*Fgetline(); */
    /*     char	*genbank_one_entry_in(); */
    /*     char	*genbank_source(), *genbank_reference(); */
    /*     char	*genbank_comments(), *genbank_origin(); */
    /*     char	*genbank_skip_unidentified(); */
    /*     void	genbank_key_word(), warning(), error(); */
    /*     void	Append_char(); */
    /*     void	genbank_verify_accession(), genbank_verify_keywords(); */
    /*     int	Lenstr(); */
    /*     static	int	count = 0; */

    eoen=' ';
    /* end-of-entry, set to be 'y' after '//' is read */
    for(eof=Fgetline(line, LINENUM, fp);eof!=NULL&&eoen!='y'; ) {

        if(Lenstr(line)<=1) {
            eof=Fgetline(line, LINENUM, fp);
            continue;	/* empty line, skip */
        }

        genbank_key_word(line, 0, key, TOKENNUM);

        eoen='n';

        if((Cmpstr(key, "LOCUS"))==EQ)	{

            eof = genbank_one_entry_in(&data.gbk.locus, line, fp);
            if(Lenstr(data.gbk.locus)<61) warning(14, "LOCUS data might be incomplete");

        } else if((Cmpstr(key, "DEFINITION"))==EQ)	{

            eof = genbank_one_entry_in(&data.gbk.definition, line, fp);

            /* correct missing '.' at the end */
            Append_char(&(data.gbk.definition), '.');

        } else if((Cmpstr(key, "ACCESSION"))==EQ)	{

            eof = genbank_one_entry_in(&data.gbk.accession, line, fp);

            genbank_verify_accession();

        } else if((Cmpstr(key, "KEYWORDS"))==EQ)	{

            eof = genbank_one_entry_in(&data.gbk.keywords, line, fp);
            genbank_verify_keywords();

        } else if((Cmpstr(key, "SOURCE"))==EQ)		{
            eof = genbank_source(line, fp);
            /* correct missing '.' at the end */
            Append_char(&(data.gbk.source), '.');
            Append_char(&(data.gbk.organism), '.');
        } else if((Cmpstr(key, "REFERENCE"))==EQ)	{
            eof = genbank_reference(line, fp);
        } else if((Cmpstr(key, "COMMENTS"))==EQ)	{
            eof = genbank_comments(line, fp);
        } else if((Cmpstr(key, "COMMENT"))==EQ)	{
            eof = genbank_comments(line, fp);
        } else if((Cmpstr(key, "ORIGIN"))==EQ)		{
            eof = genbank_origin(line, fp);
            eoen = 'y';
        } else {	/* unidentified key word */
            eof = genbank_skip_unidentified(line, fp, 2);
        }
        /* except "ORIGIN", at the end of all the other cases,
           * a new line has already read in, so no further read
           * is necessary */
    }	/* for loop to read an entry line by line */

    if(eoen=='n') {
        warning(86, "Reach EOF before sequence data is read.");
        return(EOF);
    }
    if(eof==NULL)	return(EOF);
    else	return(EOF+1);

}
/* -------------------------------------------------------------
 *	Function genbank_key_word().
 *		Get the key_word from line beginning at index.
 */
void genbank_key_word(line, index, key, length)
     char *line;
     int   index;
     char *key;
     int   length;
{
    int	indi, indj;

    if(line==NULL)	{	key[0]='\0'; return;	}

    for(indi=index, indj=0;
	(index-indi)<length&&line[indi]!=' '&&line[indi]!='\t'
	    &&line[indi]!='\n'&&line[indi]!='\0'&&indi<12;
	indi++, indj++)
	key[indj] = line[indi];

    key[indj] = '\0';
}
/* -------------------------------------------------------------
 *	Function genbank_comment_subkey_word().
 *		Get the subkey_word in comment lines beginning
 *			at index.
 */
int genbank_comment_subkey_word(line, index, key, length)
     char *line;
     int   index;
     char *key;
     int   length;
{
	int	indi, indj;

	if(line==NULL)	{ key[0]='\0'; return(index);	}

	for(indi = index, indj=0;
        (index-indi)<length && line[indi] != ':' && line[indi]!='\t' && line[indi]!='\n' && line[indi]!='\0' && line[indi]!='(';
        indi++, indj++)
    {
		key[indj] = line[indi];
    }

	if(line[indi]==':') key[indj++] = ':';
	key[indj] = '\0';

	return(indi+1);
}
/* ------------------------------------------------------------
 *	Function genbank_chcek_blanks().
 *		Check if there is (numb) of blanks at beginning
 *			of line.
 */
int genbank_check_blanks(line, numb)
     char *line;
     int   numb;
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
 *	Function genbank_continue_line().
 *		if there are (numb) of blanks at the beginning
 *			of line, it is a continue line of the
 *			current command.
 */
char *genbank_continue_line(string, line, numb, fp)
     char **string, *line;
     int	numb;	            /* number of blanks needed to define a continue line */
     FILE_BUFFER  fp;
{
	int	ind;
/* 	int	Lenstr(); */
/* 	int	genbank_check_blanks(), Skip_white_space(); */
	char	*eof, temp[LINENUM];
/* 	char	*Fgetline(); */
/* 	void	Cpystr(), Append_rp_eoln(); */

	/* check continue lines */
	for(eof=Fgetline(line, LINENUM, fp);
	eof!=NULL&&(genbank_check_blanks(line, numb)
	||line[0]=='\n'); eof=Fgetline(line, LINENUM, fp)) {

		if(line[0]=='\n') continue; /* empty line is allowed */
		/* remove end-of-line, if there is any */
		ind=Skip_white_space(line, 0);
		Cpystr(temp, (line+ind));
		Append_rp_eoln(string, temp);

	}	/* end of continue line checking */

	return(eof);
}
/* ------------------------------------------------------------
 *	Function genbank_one_entry_in().
 *		Read in genbank one entry lines.
 */
char *genbank_one_entry_in(datastring, line, fp)
     char **datastring, *line;
     FILE_BUFFER  fp;
{
	int	index;
/* 	int	Skip_white_space(), Lenstr(); */
	char	*eof;
/* 	char	*genbank_continue_line(), *Dupstr(); */
/* 	void	error(), Freespace(); */

	index = Skip_white_space(line, 12);
	Freespace(datastring);
	*datastring = Dupstr(line+index);
	eof = (char*)genbank_continue_line(datastring, line, 12, fp);

	return(eof);
}
/* ------------------------------------------------------------
 *	Function genbank_one_comment_entry().
 *		Read in one genbank sub-entry in comments lines.
 */
char *genbank_one_comment_entry(datastring, line, start_index, fp)
     char **datastring, *line;
     int	start_index;
     FILE_BUFFER  fp;
{
	int	index;
/* 	int	Skip_white_space(), Lenstr(); */
	char	*eof;
/* 	char *genbank_continue_line(), *Dupstr(); */
/* 	void	error(), Freespace(); */

	index = Skip_white_space(line, start_index);
	Freespace(datastring);
	*datastring = Dupstr(line+index);
	eof = (char*)genbank_continue_line (datastring, line, 20, fp);
	return(eof);
}
/* --------------------------------------------------------------
 *	Function genbank_source()
 *		Read in genbank SOURCE lines and also ORGANISM
 *			lines.
 */
char *genbank_source(line, fp)
     char *line;
     FILE_BUFFER fp;
{
 	int     index;
/*  	int     Skip_white_space(); */
	char    *eof;
/* 	char    *genbank_continue_line(), *Dupstr(); */
/* 	char    *genbank_one_entry_in(); */
	char    *dummy, key[TOKENNUM];
/* 	void    Freespace(), genbank_key_word(); */

  	eof = genbank_one_entry_in(&(data.gbk.source), line, fp);
  	genbank_key_word(line, 2, key, TOKENNUM);
  	if(Cmpstr(key, "ORGANISM")==EQ) {
  	index = Skip_white_space(line, 12);
  	data.gbk.organism = Dupstr(line+index);
  	dummy = (char*)Dupstr("\n");
  	eof = (char*)genbank_continue_line(&(dummy), line, 12, fp);
  	Freespace(&dummy);
  	}
   	return(eof);
}
/* --------------------------------------------------------------
 *	Function genbank_reference().
 *		Read in genbank REFERENCE lines.
 */
char *genbank_reference(line, fp)
     char *line;
     FILE_BUFFER fp;
{
#define AUTH	0
#define TIT	1
#define JOUR	2
/* 	void	genbank_key_word(), error(), warning(); */
/* 	void	Freespace(), init_reference(); */
/* 	void	Append_char(); */
	char	*eof, key[TOKENNUM];
/* 	char	*Dupstr(), *genbank_skip_unidentified(); */
	char	temp[LINENUM];
/* 	char	*Reallocspace(), *genbank_one_entry_in(); */
	int	/*index, indi,*/ refnum;
/* 	int	Cmpstr(), Skip_white_space(); */
	int     acount=0, tcount=0, jcount=0, scount=0;

	sscanf(line+12, "%d", &refnum);
	if(refnum <= data.gbk.numofref)	{
		sprintf(temp,
			"Might redefine reference %d", refnum);
		warning(17, temp);
		eof = genbank_skip_unidentified(line, fp, 12);
	} else	{
		data.gbk.numofref = refnum;
		data.gbk.reference = (Reference*)Reallocspace ((char*)data.gbk.reference, (unsigned) (sizeof(Reference)*(data.gbk.numofref)));
		/* initialize the buffer */
		init_reference(&(data.gbk.reference[refnum-1]), ALL);
		eof = genbank_one_entry_in (&(data.gbk.reference[refnum-1].ref),line, fp);
	}
	/* find the reference listings */
	for( ;eof!=NULL&&line[0]==' '&&line[1]==' '; )
	{
		/* find the key word */
		genbank_key_word(line, 2, key, TOKENNUM);
		/* skip white space */
		if((Cmpstr(key, "AUTHORS"))==EQ)	{
			eof = genbank_one_entry_in(
				&(data.gbk.reference[refnum-1].author),
				line, fp);

			/* add '.' if missing at the end */
				Append_char
			(&(data.gbk.reference[refnum-1].author),'.');

			if(acount==0) acount=1;
			else	{

		sprintf(temp, "AUTHORS of REFERENCE %d is redefined"
			, refnum);
				warning(10, temp);
			}

		} else if((Cmpstr(key, "TITLE"))==EQ)	{
			eof = genbank_one_entry_in(
				&(data.gbk.reference[refnum-1].title),
				line, fp);
			if(tcount==0) tcount=1;
			else	{

		sprintf(temp, "TITLE of REFERENCE %d is redefined"
			, refnum);

				warning(11, temp);
			}
		} else if((Cmpstr(key, "JOURNAL"))==EQ)	{

			eof = genbank_one_entry_in(
			&(data.gbk.reference[refnum-1].journal),
				line, fp);

			if(jcount==0) jcount=1;
			else	{

				sprintf(temp,
			"JOURNAL of REFERENCE %d is redefined", refnum);

				warning(12, temp);
			}
		} else if((Cmpstr(key, "STANDARD"))==EQ)	{

			eof = genbank_one_entry_in(
			&(data.gbk.reference[refnum-1].standard),
				line, fp);

			if(scount==0) scount=1;
			else	{

				sprintf(temp,
		"STANDARD of REFERENCE %d is redefined", refnum);

				warning(13, temp);
			}
		} else {

		sprintf(temp,
		"Unidentified REFERENCE subkeyword: %s#", key);

			warning(18,temp);
			eof = genbank_skip_unidentified(line, fp, 12);
		}
	}	/* for loop */
	return(eof);
}
/* --------------------------------------------------------------
 *	Function genbank_comments().
 *		Read in genbank COMMENTS lines.
 */
const char *genbank_comments(line, fp)
     char *line;
     FILE_BUFFER fp;
{
	int	index, indi, ptr/*, genbank_check_blanks()*/;
/* 	int	Lenstr(), Skip_white_space(); */
/* 	int	Cmpstr(), genbank_comment_subkey_word(); */
	const char	*eof;
/* 	char	*Fgetline(), *Dupstr(); */
	char	key[TOKENNUM];
/* 	char	*genbank_one_comment_entry(); */
/* 	void	Freespace(), Append(); */

	if(Lenstr(line)<=12)	{
		if((eof = Fgetline(line, LINENUM, fp))==NULL)
			return(eof);
	}
/* make up data to match the logic reasoning for next statment */
	for(indi=0; indi<12; line[indi++]=' ') ; eof = "NONNULL";

	for( ;eof!=NULL&&(genbank_check_blanks(line, 12) ||line[0]=='\n'); ) {
		if(line[0]=='\n') {	/* skip empty line */
			eof=Fgetline(line, LINENUM, fp);
			continue;
		}

		ptr = index = 12;

		index = Skip_white_space(line, index);
#if defined(DEBUG)
        if (index >= TOKENNUM) {
            printf("big index %i after Skip_white_space\n", index);
        }
#endif /* DEBUG */
		index = genbank_comment_subkey_word (line, index, key, TOKENNUM);
#if defined(DEBUG)
        if (index >= TOKENNUM) {
            printf("big index %i after genbank_comment_subkey_word\n", index);
        }
#endif /* DEBUG */

		if(Cmpstr(key, "Source of strain:")==EQ) {
			eof = genbank_one_comment_entry (&(data.gbk.comments.orginf.source), line, index, fp);

		} else if(Cmpstr(key, "Culture collection:")==EQ) {

			eof = genbank_one_comment_entry (&(data.gbk.comments.orginf.cc), line, index, fp);

		} else if(Cmpstr(key, "Former name:")==EQ) {

			eof = genbank_one_comment_entry (&(data.gbk.comments.orginf.formname), line, index, fp);

		} else if(Cmpstr(key, "Alternate name:")==EQ) {

			eof = genbank_one_comment_entry (&(data.gbk.comments.orginf.nickname), line, index, fp);

		} else if(Cmpstr(key, "Common name:")==EQ) {

			eof = genbank_one_comment_entry (&(data.gbk.comments.orginf.commname), line, index, fp);

		} else if(Cmpstr(key, "Host organism:")==EQ) {

			eof = genbank_one_comment_entry (&(data.gbk.comments.orginf.hostorg), line, index, fp);

		} else if(Cmpstr(key, "RDP ID:")==EQ) {
			eof = genbank_one_comment_entry (&(data.gbk.comments.seqinf.RDPid), line, index, fp);

		} else if(Cmpstr(key,
                         "Corresponding GenBank entry:")==EQ) {

			eof = genbank_one_comment_entry (&(data.gbk.comments.seqinf.gbkentry), line, index, fp);

		} else if(Cmpstr(key, "Sequencing methods:")==EQ) {

			eof = genbank_one_comment_entry (&(data.gbk.comments.seqinf.methods), line, index, fp);

		} else if(Cmpstr(key, "5' end complete:")==EQ) {
			sscanf(line+index, "%s", key);
			if(key[0]=='Y') data.gbk.comments.seqinf.comp5 = 'y';
			else data.gbk.comments.seqinf.comp5 = 'n';
			eof=Fgetline(line, LINENUM, fp);
		} else if(Cmpstr(key, "3' end complete:")==EQ) {
			sscanf(line+index, "%s", key);
			if(key[0]=='Y') data.gbk.comments.seqinf.comp3 = 'y';
			else data.gbk.comments.seqinf.comp3 = 'n';
			eof=Fgetline(line, LINENUM, fp);
		} else if(Cmpstr(key,
                         "Sequence information ")==EQ) {
            /* do nothing */
			data.gbk.comments.seqinf.exist = 1;
			eof=Fgetline(line, LINENUM, fp);
		} else if(Cmpstr(key, "Organism information")==EQ) {
            /* do nothing */
			data.gbk.comments.orginf.exist = 1;
			eof=Fgetline(line, LINENUM, fp);
		} else {	/* other comments */

            assert(ptr == 12);
			if(data.gbk.comments.others == NULL) {
				data.gbk.comments.others =(char*)Dupstr(line+ptr);
			}
            else {
                Append(&(data.gbk.comments.others), line+ptr);
            }

			eof=Fgetline(line, LINENUM, fp);
		}
	}	/* for loop */

	return(eof);
}
/* --------------------------------------------------------------
*	Function genbank_origin().
*		Read in genbank sequence data.
*/
char
*genbank_origin(line, fp)
     char	*line;
     FILE_BUFFER	fp;
{
	char	*eof;
	int	index;
/* 	void	warning(); */

	data.seq_length                = 0;
    data.sequence[data.seq_length] = '\0'; // needed if sequence data is empty

	/* read in whole sequence data */
	for(eof=Fgetline(line, LINENUM, fp);
        eof != NULL&&line[0]!='/'&&line[1]!='/';
        eof  = Fgetline(line, LINENUM, fp))
	{
		/* empty line, skip */
		if(Lenstr(line)<=1) continue;
		for(index=9; line[index]!='\n'&&line[index]!='\0';
            index++)
		{
			if(line[index]!=' ' && data.seq_length>=data.max) {
				data.max += 100;

				data.sequence = (char*)Reallocspace(data.sequence,
                                                    (unsigned)(sizeof(char)*data.max));
			}
			if(line[index]!=' ') data.sequence[data.seq_length++] = line[index];
		}
        if(data.seq_length>=data.max) {
            data.max += 100;

            data.sequence = (char*)Reallocspace(data.sequence,
                                                (unsigned)(sizeof(char)*data.max));
        }
		data.sequence[data.seq_length] = '\0';
	}

	return(eof);
}
/* ---------------------------------------------------------------
 *	Function genbank_skip_unidentified().
 *		Skip the lines of unidentified keyword.
 */
char
*genbank_skip_unidentified(line, fp, blank_num)
     char	     *line;
     FILE_BUFFER  fp;
     int	blank_num;
{
	char	*eof;
    /* 	int	genbank_check_blanks(); */

	for(eof=Fgetline(line, LINENUM, fp);
	eof!=NULL&&genbank_check_blanks(line, blank_num);
	eof=Fgetline(line, LINENUM, fp))	;

	return(eof);
}
/* ---------------------------------------------------------------
*	Function genbank_verify_accession().
*		Verify accession information.
*/
void
genbank_verify_accession()
{
	int	  indi, /*index,*/ len, count, remainder;
	char  temp[LONGTEXT];
/* 	char *Reallocspace(); */
/* 	void  warning(); */

	if(Cmpstr(data.gbk.accession, "No information\n")==EQ) return;
	len=Lenstr(data.gbk.accession);
	if((len % 7)!=0)	{
		if(warning_out)
			fprintf(stderr,
			"\nACCESSION: %s", data.gbk.accession);
		warning(136,
	"Each accession number should be a six-character identifier.");
	}
	for(indi=count=0; indi<len-1; indi++)	{
		remainder=indi % 7;
		switch(remainder)	{
		case	0:
			count++;
			if(count>9){
				if(warning_out) fprintf(stderr,
				"\nACCESSION: %s", data.gbk.accession);
				warning(137,
	"No more than 9 accession numbers are allowed in ACCESSION line.");
				data.gbk.accession[indi-1]='\n';
				data.gbk.accession[indi]='\0';
				data.gbk.accession = (char*)Reallocspace
					(data.gbk.accession,
					(unsigned)(sizeof(char)*indi));
				return;
			}
			if(!isalpha(data.gbk.accession[indi]))	{
				sprintf(temp,
		"The %d(th) accession number must start with a letter.",
					count);
				warning(138, temp);
			}
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			if(!isdigit(data.gbk.accession[indi]))	{
				sprintf(temp,
"The last 5 characters of the %d(th) accession number should be all digits.",
					count);
				warning(140, temp);
			}
			break;
		case 6:
			if((indi!=(len-1)&&data.gbk.accession[indi]!=' ')
			||(indi==(len-1)&&data.gbk.accession[indi]!='\n'))
			{
				if(warning_out) fprintf(stderr,
				"\nACCESSION: %s", data.gbk.accession);
				warning(139,
			"Accesssion numbers should be separated by a space.");
				data.gbk.accession[indi]=' ';
			}
			break;
		default: ;
		}
	}	/* check every char of ACCESSION line. */
}
/* ------------------------------------------------------------------
*	Function genbank_verify_keywords().
*		Verify keywords.
*/
void
genbank_verify_keywords()	{

	int	indi, count, len;
/* 	void	Append_char(), warning(); */

	/* correct missing '.' at the end */
	Append_char(&(data.gbk.keywords), '.');

	for(indi=count=0, len=Lenstr(data.gbk.keywords); indi<len; indi++)
		if(data.gbk.keywords[indi]=='.') count++;

	if(count!=1)	{
		if(warning_out) fprintf(stderr,
			"\nKEYWORDS: %s", data.gbk.keywords);
		warning(141,
	"No more than one period is allowed in KEYWORDS line.");
	}
}
/* ---------------------------------------------------------------
*	Function genbank_in_locus().
*		Read in next genbank locus and sequence only.
*		For use of converting to simple format(read in only simple
*		information instead of whole records).
*/
char
genbank_in_locus(fp)
     FILE_BUFFER fp;
{
	char	line[LINENUM], key[TOKENNUM];
	char	*eof, eoen;
/* 	char	*genbank_one_entry_in(), *genbank_origin(); */
/* 	void	genbank_key_word(), warning(), error(); */

	eoen=' '; /* end-of-entry, set to be 'y' after '//' is read */
	for(eof=Fgetline(line, LINENUM, fp); eof!=NULL&&eoen!='y'; ) {
		genbank_key_word(line, 0, key, TOKENNUM);
		if((Cmpstr(key, "ORIGIN"))==EQ)		{
			eof = genbank_origin(line, fp);
			eoen = 'y';
		} else if((Cmpstr(key, "LOCUS"))==EQ)	{
			eof = genbank_one_entry_in(&data.gbk.locus,
				line, fp);
		} else eof=Fgetline(line, LINENUM, fp);
	}	/* for loop to read an entry line by line */

	if(eoen=='n')
		error(9, "Reach EOF before one entry is read, Exit");

	if(eof==NULL)	return(EOF);
	else	return(EOF+1);

}
/* ---------------------------------------------------------------
*	Function genbank_out().
*		Output in a genbank format.
*/
void
genbank_out(fp)
FILE	*fp;
{
/* 	void	genbank_print_lines(), genbank_out_origin(); */
/* 	void	genbank_print_comment(), count_base(); */
/* 	char	temp[LONGTEXT]; */
/* 	void	genbank_out_one_entry(), genbank_out_one_comment(); */
/* 	int	Lenstr(), deterninator(); */
	int	indi, /*indj, indk,*/ length;
	int	base_a, base_t, base_g, base_c, base_other;

	/* Assume the last char of each field is '\n' */
	genbank_out_one_entry(fp, data.gbk.locus, "LOCUS       ", SEPNODEFINED, "", NOPERIOD);
	genbank_out_one_entry(fp, data.gbk.definition, "DEFINITION  ", SEPNODEFINED, "", PERIOD);
	genbank_out_one_entry(fp, data.gbk.accession, "ACCESSION   ", SEPNODEFINED, "", NOPERIOD);
	genbank_out_one_entry(fp, data.gbk.keywords, "KEYWORDS    ", SEPDEFINED, ";", PERIOD);

	if(Lenstr(data.gbk.source)>1) {
		fprintf(fp, "SOURCE      ");
		genbank_print_lines(fp, data.gbk.source, SEPNODEFINED, "");
		if(Lenstr(data.gbk.organism)>1) {
			fprintf(fp, "  ORGANISM  ");
			genbank_print_lines(fp, data.gbk.organism, SEPNODEFINED, "");
		} else	fprintf(fp, "  ORGANISM  No information.\n");
	} else if(Lenstr(data.gbk.organism)>1) {

	fprintf(fp, "SOURCE      No information.\n  ORGANISM  ");
			genbank_print_lines(fp, data.gbk.organism,
				SEPNODEFINED, "");
		} else	fprintf(fp,
	"SOURCE      No information.\n  ORGANISM  No information.\n");

	if(data.gbk.numofref>0) {
		for(indi=0; indi<data.gbk.numofref; indi++)	{

			if(Lenstr(data.gbk.reference[indi].ref)>1) {
				fprintf(fp, "REFERENCE   ");
				genbank_print_lines(fp,
					data.gbk.reference[indi].ref,
					SEPNODEFINED, "");
			} else fprintf(fp,
				"REFERENCE   %d\n", indi+1);

			genbank_out_one_entry(fp,
				data.gbk.reference[indi].author,
				"  AUTHORS   ", SEPDEFINED, " ",
				NOPERIOD);

			if(Lenstr(data.gbk.reference[indi].title)>1)
			{
				fprintf(fp, "  TITLE     ");
				genbank_print_lines(fp,
				data.gbk.reference[indi].title,
					SEPNODEFINED, "");
			}

			genbank_out_one_entry(fp,
				data.gbk.reference[indi].journal,
				"  JOURNAL   ", SEPNODEFINED, "",
				NOPERIOD);

			genbank_out_one_entry(fp,
				data.gbk.reference[indi].standard,
				"  STANDARD  ", SEPNODEFINED, "",
				NOPERIOD);

		}	/* subkey loop */
	} else	{
		fprintf(fp, "REFERENCE   1\n");
		fprintf(fp, "  AUTHORS   No information\n");
		fprintf(fp, "  JOURNAL   No information\n");
		fprintf(fp, "  TITLE     No information\n");
		fprintf(fp, "  STANDARD  No information\n");
	}

	if(data.gbk.comments.orginf.exist==1||
	data.gbk.comments.seqinf.exist == 1 ||
	Lenstr(data.gbk.comments.others)>0)
	{
		fprintf(fp, "COMMENTS    ");

		if(data.gbk.comments.orginf.exist==1)	{
			fprintf(fp, "Organism information\n");

		genbank_out_one_comment(fp, data.gbk.comments.orginf.source,
			"Source of strain: ",
			COMMSKINDENT, COMMCNINDENT);

		genbank_out_one_comment(fp,
			data.gbk.comments.orginf.cc,
			"Culture collection: ",
			COMMSKINDENT, COMMCNINDENT);

		genbank_out_one_comment(fp,
			data.gbk.comments.orginf.formname,
			"Former name: ",
			COMMSKINDENT, COMMCNINDENT);

		genbank_out_one_comment(fp,
			data.gbk.comments.orginf.nickname,
			"Alternate name: ",
			COMMSKINDENT, COMMCNINDENT);

		genbank_out_one_comment(fp,
			data.gbk.comments.orginf.commname,
			"Common name: ",
			COMMSKINDENT, COMMCNINDENT);

		genbank_out_one_comment(fp,
			data.gbk.comments.orginf.hostorg,
			"Host organism: ",
			COMMSKINDENT, COMMCNINDENT);

		if(data.gbk.comments.seqinf.exist == 1 ||
		Lenstr(data.gbk.comments.others)>0)
			fprintf(fp, "            ");
		}	/* organism information */

		if(data.gbk.comments.seqinf.exist==1) {

			fprintf(fp,
			"Sequence information (bases 1 to %d)\n",
				data.seq_length);
		}

		genbank_out_one_comment(fp,
			data.gbk.comments.seqinf.RDPid,
			"RDP ID: ",
			COMMSKINDENT, COMMCNINDENT);

		genbank_out_one_comment(fp,
			data.gbk.comments.seqinf.gbkentry,
			"Corresponding GenBank entry: ",
			COMMSKINDENT, COMMCNINDENT);

		genbank_out_one_comment(fp,
			data.gbk.comments.seqinf.methods,
			"Sequencing methods: ",
			COMMSKINDENT, COMMCNINDENT);

		if(data.gbk.comments.seqinf.comp5=='n')
			fprintf(fp,
			"              5' end complete: No\n");

		else if(data.gbk.comments.seqinf.comp5=='y')
			fprintf(fp,
			"              5' end complete: Yes\n");

		if(data.gbk.comments.seqinf.comp3=='n')
			fprintf(fp,
			"              3' end complete: No\n");

		else if(data.gbk.comments.seqinf.comp3=='y')
			fprintf(fp,
			"             3' end complete: Yes\n");

		/* print 12 spaces of the first line */
		if(Lenstr(data.gbk.comments.others)>0)
			fprintf(fp, "            ");

		if(Lenstr(data.gbk.comments.others)>0) {
			length = Lenstr(data.gbk.comments.others);
			for(indi=0; indi<length; indi++)
			{
				fprintf(fp, "%c",
				data.gbk.comments.others[indi]);

		/* if another line, print 12 spaces first */
			if(data.gbk.comments.others[indi]=='\n'
			&&data.gbk.comments.others[indi+1]!='\0')

				fprintf(fp, "            ");

			}
		}	/* other comments */
	}	/* comment */

	count_base(&base_a, &base_t, &base_g, &base_c, &base_other);

	/* don't write 0 others in this base line */
	if(base_other>0)
		fprintf(fp,
		"BASE COUNT  %6d a %6d c %6d g %6d t %6d others\n",
			base_a, base_c, base_g, base_t, base_other);
	else fprintf(fp, "BASE COUNT  %6d a %6d c %6d g %6d t\n",
		base_a, base_c, base_g, base_t);

	genbank_out_origin(fp);
}
/* ------------------------------------------------------------
 *	Function genbank_out_one_entry().
 *		Print out key and string if string length > 1
 *		otherwise print key and "No information" w/wo
 *		period at the end depending on flag period.
 */
void genbank_out_one_entry(fp, string, key, flag, patterns, period)
     FILE	    *fp;
     char	    *string;
     const char	*key;
     int	     flag;
     const char	*patterns;
     int	     period;
{
/* 	int	Lenstr(); */
/* 	void	genbank_print_lines(); */

	if(Lenstr(string)>1) {
		fprintf(fp, "%s", key);
		genbank_print_lines(fp, string, flag, patterns);
	} else	if(period)
			fprintf(fp, "%sNo information.\n", key);
		else fprintf(fp, "%sNo information\n", key);
}
/* -------------------------------------------------------------
 *	Function genbank_out_one_comment().
 *		print out one genbank comment sub-keyword.
 */
void genbank_out_one_comment(fp, string, key, skindent, cnindent)
     FILE *fp;
     char *string;
     const char *key;
     int   skindent, cnindent;	/* subkeyword indent and  continue line indent */
{
/* 	int	Lenstr(); */
/* 	void	genbank_print_comment(); */

	if(Lenstr(string)>1)
		genbank_print_comment(fp, key, string, skindent, cnindent);
}
/* --------------------------------------------------------------
 *	Fucntion genbank_print_lines().
 *		Print one grnbank line, wrap around if over
 *			column 80.
 */
void genbank_print_lines(fp, string, flag, separators)
     FILE       *fp;
     char       *string;
     int         flag;
     const char *separators;
{
	int	first_time=1, indi, indj, indk /*,indl*/;
	int	ibuf, len;

	len = Lenstr(string)-1;
	/* indi: first char of the line */
	/* num of char, excluding the first char, of the line */
	for(indi=0; indi<len; indi+=(indj+1)) {
		indj=GBMAXCHAR;
		if((Lenstr(string+indi))>GBMAXCHAR) {

		/* search for proper termination of a line */

			ibuf = indj;

			for(;indj>0
                    &&((!flag&&!last_word(string[indj+indi]))
                       ||(flag&&!is_separator(string[indj+indi], separators)));
                indj--);

			if(indj==0) indj=ibuf;
			else if(string[indi+indj+1]==' ') indj++;

			/* print left margine */
			if(!first_time)
				fprintf(fp, "            ");
			else first_time = 0;

			for(indk=0; indk<indj; indk++)
				fprintf(fp, "%c", string[indi+indk]);

		/* leave out the last space, if there is any */
			if(string[indi+indj]!=' '&&string[indi+indj]!='\n')
				fprintf(fp, "%c", string[indi+indj]);
			fprintf(fp, "\n");

		} else if(first_time)
				fprintf(fp, "%s", string+indi);
			else fprintf(fp,
				"            %s", string+indi);
	}
}
/* --------------------------------------------------------------
 *	Fucntion genbank_print_comment().
 *		Print one grnbank line, wrap around if over
 *			column 80.
 */
void genbank_print_comment(fp, key, string, offset, indent)
     FILE       *fp;
     char       *string;
     const char *key;
     int         offset, indent;
{
	int	first_time=1, indi, indj, indk, indl;
	int	len;

	len = Lenstr(string)-1;
	for(indi=0; indi<len; indi+=(indj+1)) {

		if(first_time)
			indj=GBMAXCHAR-offset-Lenstr(key)-1;
		else indj=GBMAXCHAR-offset-indent-1;

		fprintf(fp, "            ");

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
			for(;indj>=0&&!last_word(string[indj+indi]);
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
/* ---------------------------------------------------------------
*	Fcuntion genbank_out_origin().
*		Output sequence data in genbank format.
*/
void
genbank_out_origin(fp)
FILE	*fp;
{

	int	indi, indj, indk;

	fprintf(fp, "ORIGIN\n");

	for(indi=0, indj=0, indk=1; indi<data.seq_length; indi++)
	{
		if((indk % 60)==1) fprintf(fp, "   %6d ", indk);
		fprintf(fp, "%c", data.sequence[indi]);
		indj++;

		/* blank space follows every 10 bases, but not before '\n' */
		if((indk % 60)==0) { fprintf(fp, "\n"); indj=0; }
		else if(indj==10&&indi!=(data.seq_length-1))
			{ fprintf(fp, " "); indj=0; }
		indk++;
	}

	if((indk % 60)!=1) fprintf(fp, "\n");
	fprintf(fp, "//\n");
}
/* -----------------------------------------------------------
*	Function genbank_to_genbank().
*		Convert from genbank to genbank.
*/
void
genbank_to_genbank(inf, outf)
     char	*inf, *outf;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	char	     temp[TOKENNUM];

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp,
                "Cannot open input file %s, exit\n", inf);
		error(35, temp);
	}
    ifp = create_FILE_BUFFER(inf, IFP);
	if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp,
                "Cannot open output file %s, exit\n", outf);
		error(36, temp);
	}
	init();
	init_genbank();
	/* rewind(ifp); */
	while(genbank_in(ifp)!=EOF)	{
		data.numofseq++;
		genbank_out(ofp);
		init_genbank();
	}

#ifdef log
	fprintf(stderr,
	"Total %d sequences have been processed\n",
		data.numofseq);
#endif

	destroy_FILE_BUFFER(ifp); fclose(ofp);
}
/* -----------------------------------------------------------
*	Function init_reference().
*		Init. new reference record(init. value is "\n").
*/
void
init_reference(ref, flag)
Reference *ref;
int	flag;
{
/* 	char	*Dupstr(); */

	if(flag==REF)		ref->ref = Dupstr("\n");
	if(flag!=AUTHOR)	ref->author = Dupstr("\n");
	if(flag!=JOURNAL)	ref->journal = Dupstr("\n");
	if(flag!=TITLE)		ref->title = Dupstr("\n");
	if(flag!=STANDARD)	ref->standard = Dupstr("\n");
}
