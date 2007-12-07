/* genabnk and Macke converting program */

#include <stdio.h>
#include <stdlib.h>
#include "convert.h"
#include "global.h"

extern int	warning_out;

/* --------------------------------------------------------------
*	Function init_gm_data().
*	Initialize data structure of genbank and Macke formats.
*/
void
init_gm_data()	{
/* 	void	init_macke(), init_genbank(); */

	init_macke();
	init_genbank();
}
/* ----------------------------------------------------------
*	Function genbank_to_macke().
*	Convert from Genbank format to Macke format.
*/
void
genbank_to_macke(inf, outf)
     char	*inf, *outf;
{
	FILE	    *IFP, *ofp;
    FILE_BUFFER  ifp;
	char	     temp[TOKENNUM];
	int	         indi, total_num;

	if((IFP=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "CANNOT open input file %s, exit.\n", inf);
		error(0, temp);
	}
    ifp              = create_FILE_BUFFER(inf, IFP);
	if(Lenstr(outf) <= 0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "CANNOT open output file %s, exit.\n", outf);
		error(1, temp);
	}
	/* seq irelenvant header */
	init();
	init_gm_data();
	macke_out_header(ofp);

#ifdef log
	fprintf(stderr, "Start converting...\n");
#endif

	for(indi=0; indi<3; indi++)	{
		FILE_BUFFER_rewind(ifp);
		init_seq_data();
		while(genbank_in(ifp)!=EOF)	{
			data.numofseq++;
			if(gtom()) {
			/* convert from genbank form to macke form */
				switch(indi)	{
				case 0:
					/* output seq display format */
					macke_out0(ofp, GENBANK);
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
			} else error(7,
			"Conversion from genbank to macke fails, Exit");
			init_gm_data();
#ifdef log
	if((data.numofseq % 100)==0)
		fprintf(stderr, "%d sequences have been processed\n",
			data.numofseq);
#endif
		}
		total_num = data.numofseq;
		if(indi==0)	{
			fprintf(ofp, "#-\n");
			/* no warning message for next loop */
			warning_out = 0;
		}
	}	/* for each seq; loop */

	warning_out = 1;	/* resume warning messages */

#ifdef log
	fprintf(stderr,
		 "Total %d sequences have been processed\n", total_num);
#endif

}
/* --------------------------------------------------------------
*	Function gtom().
*		Convert from Genbank format to Macke format.
*/
int
gtom()	{
/* 	void	genbank_key_word(), error(), Freespace(), Cpystr(); */
/* 	void	Append(), gtom_remarks(), replace_entry(); */
/* 	char	*Catstr(); */
	char	temp[LONGTEXT], buffer[TOKENNUM];
	char	genus[TOKENNUM], species[TOKENNUM];
/* 	char	*genbank_date(), *today_date(); */
/* 	char	*genbank_get_strain(), *genbank_get_subspecies(); */
/* 	char	*genbank_get_atcc(); */
/* 	int	Lenstr(), num_of_remark(), Cmpstr(); */
/* 	int	len; */
/* 	int 	indj, indk, remnum; */

	/* copy seq abbr, assume every entry in gbk must end with \n\0 */
	/* no '\n' at the end of the string */
	genbank_key_word(data.gbk.locus, 0, temp, TOKENNUM);
	replace_entry(&(data.macke.seqabbr), temp);

	/* copy name and definition*/
	if(Lenstr(data.gbk.organism)>1)
		replace_entry(&(data.macke.name),data.gbk.organism);
	else	if(Lenstr(data.gbk.definition)>1)	{
		sscanf(data.gbk.definition, "%s %s", genus, species);
		if(species[Lenstr(species)-1]==';')
			species[Lenstr(species)-1] = '\0';
		sprintf(temp, "%s %s\n", genus, species);
		replace_entry(&(data.macke.name), temp);
	}

	/* copy cc name and number */
	if(Lenstr(data.gbk.comments.orginf.cc)>1)
		replace_entry(&(data.macke.atcc),
			data.gbk.comments.orginf.cc);

	/* copy rna(methods) */
	if(Lenstr(data.gbk.comments.seqinf.methods)>1)
		replace_entry(&(data.macke.rna),
			data.gbk.comments.seqinf.methods);

	/* copy date---DD-MMM-YYYY\n\0  */
	if(Lenstr(data.gbk.locus)<61) 	{
		data.macke.date = genbank_date(today_date());
		Append(&(data.macke.date), "\n");
	} else
		replace_entry(&(data.macke.date), data.gbk.locus+50);

	/* copy genbank entry  (gbkentry has higher priority than gbk.accession)*/
	if(Lenstr(data.gbk.comments.seqinf.gbkentry)>1)
		replace_entry(&(data.macke.acs),
			data.gbk.comments.seqinf.gbkentry);
	else {
		if(Lenstr(data.gbk.accession)>1
		&&Cmpstr(data.gbk.accession, "No information\n")!=EQ)	{
			sscanf(data.gbk.accession, "%s", buffer);
			Catstr(buffer, "\n");
		} else Cpystr(buffer, "\n");
		replace_entry(&(data.macke.acs), buffer);
	}

	/* copy the first reference from GenBank to Macke */
	if(data.gbk.numofref>0)	{
		if(Lenstr(data.gbk.reference[0].author)>1)
			replace_entry(&(data.macke.author),
				data.gbk.reference[0].author);

		if(Lenstr(data.gbk.reference[0].journal)>1)
			replace_entry(&(data.macke.journal),
				data.gbk.reference[0].journal);

		if(Lenstr(data.gbk.reference[0].title)>1)
			replace_entry(&(data.macke.title),
				data.gbk.reference[0].title);
	} /* the rest of references are put into remarks, rem:..... */

	gtom_remarks();

	/* adjust the strain, subspecies, and atcc information */
	Freespace(&(data.macke.strain));
	data.macke.strain = (char*)genbank_get_strain();
	Freespace(&(data.macke.subspecies));
	data.macke.subspecies = (char*)genbank_get_subspecies();
	if(Lenstr(data.macke.atcc)<=1)	{
		Freespace(&(data.macke.atcc));
		data.macke.atcc = (char*)genbank_get_atcc();
	}

	return(1);
}
/* --------------------------------------------------------------
* 	Function gtom_remarks().
*		Create Macke remarks.
*/
void
gtom_remarks()	{

	int	remnum, len;
	int	indi, indj;
	char	temp[LONGTEXT];
/* 	void	gtom_copy_remark(); */

	/* remarks in Macke format */
	remnum = num_of_remark();
	data.macke.remarks = (char**)calloc(1,(unsigned)(sizeof(char*)*remnum));
	remnum=0;

	/* REFERENCE the first reference */
	if(data.gbk.numofref>0)
		gtom_copy_remark(data.gbk.reference[0].ref, "ref:", &remnum);

	/* The rest of the REFERENCES */
	for(indi=1; indi<data.gbk.numofref; indi++)	{
		gtom_copy_remark(data.gbk.reference[indi].ref, "ref:", &remnum);
		gtom_copy_remark(data.gbk.reference[indi].author, "auth:", &remnum);
		gtom_copy_remark(data.gbk.reference[indi].journal, "jour:", &remnum);
		gtom_copy_remark(data.gbk.reference[indi].title, "title:", &remnum);
		gtom_copy_remark(data.gbk.reference[indi].standard, "standard:", &remnum);

	}	/* loop for copying other reference */

	/* copy keywords as remark */
	gtom_copy_remark(data.gbk.keywords, "KEYWORDS:", &remnum);

	/* copy accession as remark when genbank entry also exists. */
	gtom_copy_remark(data.gbk.accession, "GenBank ACCESSION:", &remnum);

	/* copy source of strain */
	gtom_copy_remark(data.gbk.comments.orginf.source, "Source of strain:", &remnum);

	/* copy former name */
	gtom_copy_remark(data.gbk.comments.orginf.formname, "Former name:", &remnum);

	/* copy alternate name */
	gtom_copy_remark(data.gbk.comments.orginf.nickname, "Alternate name:", &remnum);

	/* copy common name */
	gtom_copy_remark(data.gbk.comments.orginf.commname, "Common name:", &remnum);

	/* copy host organism */
	gtom_copy_remark(data.gbk.comments.orginf.hostorg, "Host organism:", &remnum);

	/* copy RDP ID */
	gtom_copy_remark(data.gbk.comments.seqinf.RDPid, "RDP ID:", &remnum);

	/* copy methods */
	gtom_copy_remark(data.gbk.comments.seqinf.methods, "Sequencing methods:", &remnum);

	/* copy 3' end complete */
	if(data.gbk.comments.seqinf.comp3!=' ')	{
		if(data.gbk.comments.seqinf.comp3=='y') data.macke.remarks[remnum++] = Dupstr("3' end complete:  Yes\n");
		else data.macke.remarks[remnum++]= Dupstr("3' end complete:  No\n");
	}

	/* copy 5' end complete */
	if(data.gbk.comments.seqinf.comp5!=' ')	{
		if(data.gbk.comments.seqinf.comp5=='y') data.macke.remarks[remnum++]= Dupstr("5' end complete:  Yes\n");
		else data.macke.remarks[remnum++]= Dupstr("5' end complete:  No\n");
	}

	/* other comments, not RDP DataBase specially defined */
	if(Lenstr(data.gbk.comments.others)>0)	{
		len = Lenstr(data.gbk.comments.others);
		for(indi=0, indj=0; indi<len; indi++)
		{
			temp[indj++] = data.gbk.comments.others[indi];
			if(data.gbk.comments.others[indi]=='\n'
			|| data.gbk.comments.others[indi]=='\0') {
				temp[indj] = '\0';
				data.macke.remarks[remnum++] =
					(char*)Dupstr(temp);

				indj=0;
			}	/* new remark line */
		}	/* for loop to find other remarks */
	}	/* other comments */

	/* done with the remarks copying */

	data.macke.numofrem = remnum;
}
/* --------------------------------------------------------------------
 *	Function gtom_copy_remark().
 *		If string length > 1 then copy string with key to remark.
 */
void gtom_copy_remark(string, key, remnum)
     char *string;
     const char *key;
     int  *remnum;
{
/* 	int	Lenstr(); */
/* 	char	*Dupstr(); */
/* 	void	Append(); */

	/* copy host organism */
	if(Lenstr(string)>1)	{
		data.macke.remarks[(*remnum)]
			=(char*)Dupstr(key);
		Append(&(data.macke.remarks[(*remnum)]), string);
		(*remnum)++;
	}
}
/* --------------------------------------------------------------------
*	Function genbank_get_strain().
*		Get strain from DEFINITION, COMMENT or SOURCE line in
*		Genbank data file.
*/
char *genbank_get_strain()	{

	int	indj, indk;
/* 	int	find_pattern(), Lenstr(); */
/* 	int	Skip_white_space(), Reach_white_space(); */
	char	strain[LONGTEXT], temp[LONGTEXT], buffer[LONGTEXT];
/* 	void	get_string(), warning(), Cpystr(); */

	strain[0]='\0';
	/* get strain */
	if(Lenstr(data.gbk.comments.others)>1)	{
		if((indj=find_pattern(data.gbk.comments.others,
		"*source:"))>=0)	{
			if((indk=find_pattern(
			(data.gbk.comments.others+indj),
			"strain="))>=0)	{
				/* skip blank spaces */
				indj=Skip_white_space(
					data.gbk.comments.others,
					(indj+indk+7));
				/* get strain */
				get_string(data.gbk.comments.others,
				temp, indj);
				Cpystr(strain, temp); /* copy new strain */
			}	/* get strain */
		}	/* find source: line in comment */
	}	/* look for strain on comments */

	if(Lenstr(data.gbk.definition)>1)	{
		if((indj=find_pattern(data.gbk.definition, "str. "))>=0
		||(indj=find_pattern(data.gbk.definition, "strain "))>=0) {
			/* skip the key word */
			indj=Reach_white_space(data.gbk.definition, indj);
			/* skip blank spaces */
			indj=Skip_white_space(data.gbk.definition, indj);
			/* get strain */
			get_string(data.gbk.definition, temp, indj);
			if(Lenstr(strain)>1)	{
				if(Cmpstr(temp, strain)!=EQ){
					sprintf(buffer,
			"Inconsistent strain definition in DEFINITION: %s and %s",
					temp, strain);
					warning(91, buffer);
				} /* check consistency of duplicated def */
			} else	Cpystr(strain, temp); /* get strain */
		}	/* find strain in definition */
	}	/* if there is definition line */

	if(Lenstr(data.gbk.source)>1)	{
		if((indj=find_pattern(data.gbk.source, "str. "))>=0
		||(indj=find_pattern(data.gbk.source, "strain "))>=0){
			/* skip the key word */
			indj=Reach_white_space(data.gbk.source, indj);
			/* skip blank spaces */
			indj=Skip_white_space(data.gbk.source, indj);
			/* get strain */
			get_string(data.gbk.source, temp, indj);
			if(Lenstr(strain)>1)	{
				if(Cmpstr(temp, strain)!=EQ) {
				sprintf(buffer,
		"Inconsistent strain definition in SOURCE: %s and %s",
				temp, strain);
					warning(92, buffer);
				}
			} else	Cpystr(strain, temp);
			/* check consistency of duplicated def */
		}	/* find strain */
	}	/* look for strain in SOURCE line */

	return(Dupstr(strain));
}
/* --------------------------------------------------------------------
*	Function genbank_get_subspecies().
*		Get subspecies information from SOURCE, DEFENITION, or
*		COMMENT line of Genabnk data file.
*/
char
*genbank_get_subspecies()	{

	int	indj, indk;
/* 	int	find_pattern(), Lenstr(); */
/* 	int	Skip_white_space(), Reach_white_space(); */
	char	subspecies[LONGTEXT], temp[LONGTEXT], buffer[LONGTEXT];
/* 	char	*Dupstr(); */
/* 	void	get_string(), warning(), Cpystr(); */
/* 	void	correct_subspecies(); */

	subspecies[0]='\0';
	/* get subspecies */
	if(Lenstr(data.gbk.definition)>1)	{
		if((indj=find_pattern(data.gbk.definition, "subsp. "))>=0) {
			/* skip the key word */
			indj=Reach_white_space(data.gbk.definition, indj);
			/* skip blank spaces */
			indj=Skip_white_space(data.gbk.definition, indj);
			/* get subspecies */
			get_string(data.gbk.definition, temp, indj);
			correct_subspecies(temp);
			Cpystr(subspecies, temp);
		}
	}
	if(Lenstr(data.gbk.comments.others)>1)	{
		if((indj=find_pattern(data.gbk.comments.others,
		"*source:"))>=0)	{
			if((indk=find_pattern((data.gbk.comments.others+indj),
			"sub-species="))>=0
			||(indk=find_pattern((data.gbk.comments.others+indj),
			"subspecies="))>=0
			||(indk=find_pattern((data.gbk.comments.others+indj),
			"subsp.="))>=0)	{
				/* skip the key word */
				for(indj+=indk;
				data.gbk.comments.others[indj]!='='; indj++);
				indj++;
				/* skip blank spaces */
				indj=Skip_white_space(data.gbk.comments.others, indj);
				/* get subspecies */
				get_string(data.gbk.comments.others, temp,
					indj);
				if(Lenstr(subspecies)>1){
					if(Cmpstr(temp, subspecies)!=EQ){
						sprintf(buffer,
		"Inconsistent subspecies definition in COMMENTS *source: %s and %s",
						temp, subspecies);
						warning(20, buffer);
					}
				} else	Cpystr(subspecies, temp);
			}	/* get subspecies */
		}	/* find *source: line in comment */
	}	/* look for subspecies on comments */

	if(Lenstr(data.gbk.source)>1)	{
		if((indj=find_pattern(data.gbk.source, "subsp. "))>=0
		||(indj=find_pattern(data.gbk.source, "subspecies "))>=0
		||(indj=find_pattern(data.gbk.source, "sub-species "))>=0) {
			/* skip the key word */
			indj=Reach_white_space(data.gbk.source, indj);
			/* skip blank spaces */
			indj=Skip_white_space(data.gbk.source, indj);
			/* get subspecies */
			get_string(data.gbk.source, temp, indj);
			correct_subspecies(temp);
			if(Lenstr(subspecies)>1) {
				if(Cmpstr(temp, subspecies)!=EQ){
					sprintf(buffer,
		"Inconsistent subspecies definition in SOURCE: %s and %s",
					temp, subspecies);
					warning(21, buffer);
				}
			} else	Cpystr(subspecies, temp);
			/* check consistency of duplicated def */
		}	/* find subspecies */
	}	/* look for subspecies in SOURCE line */

	return(Dupstr(subspecies));
}
/* ---------------------------------------------------------------
*	Function correct_subspecies().
*		Remove the strain information in subspecies which is
*		sometime mistakenly written into it.
*/
void
correct_subspecies(subspecies)
char	*subspecies;
{
	int	indj;

	if((indj=find_pattern(subspecies, "str\n"))>=0
	||(indj=find_pattern(subspecies, "str."))>=0
	||(indj=find_pattern(subspecies, "strain"))>=0) {
		subspecies[indj-1]='\n';
		subspecies[indj]='\0';
	}
}
/* --------------------------------------------------------------------
*	Function genbank_get_atcc().
*		Get atcc from SOURCE line in Genbank data file.
*/
char
*genbank_get_atcc()	{

/* 	int	Lenstr(); */
	char	temp[LONGTEXT];
	char	*atcc;


	atcc = NULL;
	/* get culture collection # */
	if(Lenstr(data.gbk.source)>1)
		atcc = get_atcc(data.gbk.source);
	if(Lenstr(atcc)<=1&&Lenstr(data.macke.strain)>1)	{
		/* add () to macke strain to be processed correctly */
		sprintf(temp, "(%s)", data.macke.strain);
		atcc = get_atcc(temp);
	}
	return(atcc);
}
/* ------------------------------------------------------------------- */
/*	Function get_atcc().
*/
char
*get_atcc(source)
char	*source;
{

	static int	cc_num=16;
	static const char	*CC[16] = {"ATCC", "CCM", "CDC", "CIP", "CNCTC",
			"DSM", "EPA", "JCM", "NADC", "NCDO", "NCTC", "NRCC",
			"NRRL", "PCC", "USDA", "VPI"};
/* 	int	indk; */
	int	indi, indj, index;
	int	length;
/* 	int	find_pattern(), Lenstr(); */
/* 	int	paren_string(), Skip_white_space(), Reach_white_space(); */
	char	buffer[LONGTEXT], temp[LONGTEXT], pstring[LONGTEXT];
	char	atcc[LONGTEXT];
/* 	char	*Catstr(), *Dupstr(); */
/* 	void	get_atcc_string(); */

	atcc[0]='\0';
	for(indi=0; indi<cc_num; indi++)	{
		index=0;
		while((index=paren_string(source, pstring, index))>0) {
			if((indj=find_pattern(pstring, CC[indi]))>=0){
				/* skip the key word */
				indj += Lenstr(CC[indi]);
				/* skip blank spaces */
				indj=Skip_white_space(pstring, indj);
				/* get strain */
				get_atcc_string(pstring, buffer, indj);
				sprintf(temp, "%s %s", CC[indi], buffer);
				length=Lenstr(atcc);
				if(length>0)	{
					atcc[length]= '\0';
					Catstr(atcc, ", ");
				}
				Catstr(atcc, temp);
			}	/* find atcc */
		}	/* while loop */
	}	/* for loop */
	/* append eoln to the atcc string */
	length = Lenstr(atcc);
    if (data.macke.atcc) {
        data.macke.atcc[length] = '\0';
    }
	Catstr(atcc, "\n");
	return(Dupstr(atcc));
}
/* ----------------------------------------------------------------- */
/*	Function paren_string()
*/
int
paren_string(string, pstring, index)
char	*string, *pstring;
int	index;
{
	int	pcount=0, len, indi;

	for(indi=0, len=Lenstr(string); index<len; index++) {
		if(pcount>=1)	pstring[indi++]=string[index];
		if(string[index]=='(') pcount++;
		if(string[index]==')') pcount--;
	}
	if(indi==0)	return(-1);
	pstring[--indi]='\0';
	return(index);
}
/* ----------------------------------------------------------------
*	Function num_of_remark().
*		Count num of remarks needed in order to alloc spaces.
*/
int
num_of_remark()	{

	int	remnum, /*indi, */indj, length;

	remnum = 0;
	/* count references to be put into remarks */
	if(data.gbk.numofref>0&&Lenstr(data.gbk.reference[0].ref)>1)
		remnum++;
	for(indj=1; indj<data.gbk.numofref; indj++)	{
		if(Lenstr(data.gbk.reference[indj].ref)>1)
			remnum++;
		if(Lenstr(data.gbk.reference[indj].journal)>1)
			remnum++;
		if(Lenstr(data.gbk.reference[indj].author)>1)
			remnum++;
		if(Lenstr(data.gbk.reference[indj].title)>1)
			remnum++;
		if(Lenstr(data.gbk.reference[indj].standard)>1)
			remnum++;
	}	/* loop for copying other reference */
	/* count the other keyword in GenBank format to be put into remarks */
	if(Lenstr(data.gbk.keywords)>1)
		remnum++;
	if(Lenstr(data.gbk.accession)>1)
		remnum++;
	if(Lenstr(data.gbk.comments.orginf.source)>1)	/* Source of strain */
		remnum++;
	if(Lenstr(data.gbk.comments.orginf.formname)>1)
		remnum++;
	if(Lenstr(data.gbk.comments.orginf.nickname)>1)	 /* Alternate name */
		remnum++;
	if(Lenstr(data.gbk.comments.orginf.commname)>1)
		remnum++;
	if(Lenstr(data.gbk.comments.orginf.hostorg)>1)	/* host organism */
		remnum++;
	if(Lenstr(data.gbk.comments.seqinf.RDPid)>1)
		remnum++;
	if(Lenstr(data.gbk.comments.seqinf.methods)>1)
		remnum++;
	if(data.gbk.comments.seqinf.comp3!=' ')
		remnum++;
	if(data.gbk.comments.seqinf.comp5!=' ')
		remnum++;
	/* counting other than specific keyword comments */
	if(Lenstr(data.gbk.comments.others)>0)	{
		length = Lenstr(data.gbk.comments.others);
		for(indj=0; indj<length; indj++)
		{
			if(data.gbk.comments.others[indj]=='\n'
			|| data.gbk.comments.others[indj]=='\0') {
				remnum++;
			}	/* new remark line */
		}	/* for loop to find other remarks */
	}	/* other comments */
	return(remnum);
}
/* -----------------------------------------------------------------
*	Function macke_to_genbank().
*		Convert from macke format to genbank format.
*/
void
macke_to_genbank(inf, outf)
     char	*inf, *outf;
{
	FILE	    *IFP1, *IFP2, *IFP3, *ofp;
    FILE_BUFFER  ifp1, ifp2, ifp3;
	char	     temp[TOKENNUM];

	if((IFP1=fopen(inf, "r"))==NULL||
       (IFP2=fopen(inf, "r"))==NULL||
       (IFP3=fopen(inf, "r"))==NULL)	{
		sprintf(temp, "Cannot open input file %s\n", inf);
		error(19, temp);
	}

    ifp1 = create_FILE_BUFFER(inf, IFP1);
    ifp2 = create_FILE_BUFFER(inf, IFP2);
    ifp3 = create_FILE_BUFFER(inf, IFP3);

	if(Lenstr(outf)<=0)	ofp = stdout;
	else if((ofp=fopen(outf, "w"))==NULL)	{
		sprintf(temp, "Cannot open output file %s\n", outf);
		error(44, temp);
	}
	init();
	init_seq_data();
	init_gm_data();

#ifdef log
	fprintf(stderr, "Start converting...\n");
#endif

	while(macke_in(ifp1, ifp2, ifp3)!=EOF)	{
		data.numofseq++;
		if(mtog()) genbank_out(ofp);
		else error(15, "Conversion from macke to genbank fails, Exit");
		init_gm_data();

#ifdef log
		if((data.numofseq % 50)==0)
			fprintf(stderr,
				"%d sequences are converted...\n",
				data.numofseq);
#endif

	}

#ifdef log
	fprintf(stderr,
	"Total %d sequences have been processed\n", data.numofseq);
#endif

}
/* ----------------------------------------------------------------
*	Function mtog().
*		Convert Macke format to Genbank format.
*/
int
mtog() {
	int	indi;
/* 	int	len, Lenstr(), Cmpstr(); */
	char	temp[LONGTEXT];
/* 	char	*today_date(); */
/* 	char	*Dupstr(), *Reallocspace(), *macke_copyrem(); */
/* 	char	*genbank_date(); */
/* 	void	Freespace(), Cpystr(), Append(), Append_char(); */
/* 	void	mtog_genbank_def_and_source(); */
/* 	void	mtog_decode_ref_and_remarks(); */
/* 	void	init_reference(), replace_entry(), warning(); */

	Cpystr(temp, data.macke.seqabbr);

	for(indi=Lenstr(temp); indi<13; temp[indi++] = ' ') ;

	if(Lenstr(data.macke.date)>1)

		sprintf((temp+10),
			"%7d bp    RNA             RNA       %s\n",
			data.seq_length, genbank_date(data.macke.date));

	else sprintf((temp+10),
			"%7d bp    RNA             RNA       %s\n",
			data.seq_length, genbank_date(today_date()));

	replace_entry(&(data.gbk.locus), temp);

	/* GenBank ORGANISM */
	if(Lenstr(data.macke.name)>1)	{
		replace_entry(&(data.gbk.organism), data.macke.name);

		/* append a '.' at the end */
		Append_char(&(data.gbk.organism), '.');
	}

	if(Lenstr(data.macke.rna)>1)	{
		data.gbk.comments.seqinf.exist = 1;
		replace_entry(&(data.gbk.comments.seqinf.methods),
			data.macke.rna);
	}
	if(Lenstr(data.macke.acs)>1)	{
/* #### not converted to accession but to comment gbkentry only, temporarily
		Freespace(&(data.gbk.accession));
		data.gbk.accession = Dupstr(data.macke.acs);
*/
		data.gbk.comments.seqinf.exist = 1;
		replace_entry(&(data.gbk.comments.seqinf.gbkentry),
			data.macke.acs);
	}	else if(Lenstr(data.macke.nbk)>1)	{
/* #### not converted to accession but to comment gbkentry only, temp
		Freespace(&(data.gbk.accession));
		data.gbk.accession = Dupstr(data.macke.nbk);
*/
		data.gbk.comments.seqinf.exist = 1;
		replace_entry(&(data.gbk.comments.seqinf.gbkentry),
			data.macke.nbk);
	}
	if(Lenstr(data.macke.atcc)>1)	{
		data.gbk.comments.orginf.exist = 1;
		replace_entry(&(data.gbk.comments.orginf.cc),
			data.macke.atcc);
	}
	mtog_decode_ref_and_remarks();
	/* final conversion of cc */
	if(Lenstr(data.gbk.comments.orginf.cc)<=1&&Lenstr(data.macke.atcc)>1){
		replace_entry(&(data.gbk.comments.orginf.cc),
			data.macke.atcc);
	}

	/* define GenBank DEFINITION, after GenBank KEYWORD is defined. */
	mtog_genbank_def_and_source();

	return(1);
}
/* ---------------------------------------------------------------
*	Function mtog_decode_remarks().
*		Decode remarks of Macke to GenBank format.
*/
void
mtog_decode_ref_and_remarks()	{

	int	indi, indj;
	int	acount, tcount, jcount, rcount, scount;
	char	key[TOKENNUM], temp[LONGTEXT];
/* 	char	*macke_copyrem(), *Reallocspace(), *Dupstr(); */
/* 	void	Append_char(), Append(), Cpystr(); */
/* 	void	mtog_copy_remark(), init_reference(); */

	data.gbk.numofref=acount=tcount=jcount=rcount=scount=0;

	if(Lenstr(data.macke.author)>1)	{
		if((acount+1)>data.gbk.numofref)	{
			/* new reference */
			data.gbk.reference = (Reference*)Reallocspace(data.gbk.reference, sizeof(Reference)*(acount+1));
			data.gbk.numofref = acount+1;
		init_reference(&(data.gbk.reference[acount]), AUTHOR);
		} else acount = data.gbk.numofref - 1;
		data.gbk.reference[acount++].author
			= (char*)Dupstr(data.macke.author);
	}
	if(Lenstr(data.macke.journal)>1)	{
		if((jcount+1)>data.gbk.numofref)	{
			data.gbk.reference =
			(Reference*)Reallocspace(data.gbk.reference,
			sizeof(Reference)*(jcount+1));
			data.gbk.numofref = jcount+1;
		init_reference(&(data.gbk.reference[jcount]), JOURNAL);
		} else jcount = data.gbk.numofref - 1;
		data.gbk.reference[jcount++].journal
			= (char*)Dupstr(data.macke.journal);
	}
	if(Lenstr(data.macke.title)>1)	{
		if((tcount+1)>data.gbk.numofref)	{
			data.gbk.reference =
			(Reference*)Reallocspace(data.gbk.reference,
			sizeof(Reference)*(tcount+1));
			data.gbk.numofref = tcount+1;
		init_reference(&(data.gbk.reference[tcount]), TITLE);
		} else tcount = data.gbk.numofref - 1;
		data.gbk.reference[tcount++].title
			= (char*)Dupstr(data.macke.title);
	}
	for(indi=0; indi<data.macke.numofrem; indi++)	{
		indj = macke_key_word(data.macke.remarks[indi],
			0, key, TOKENNUM);
		if(Cmpstr(key, "KEYWORDS")==EQ)	{
			mtog_copy_remark(&(data.gbk.keywords),
				&indi, indj);

			/* append a '.' at the end */
			Append_char(&(data.gbk.keywords), '.');

		} else if(Cmpstr(key, "GenBank ACCESSION")==EQ)	{
			mtog_copy_remark(&(data.gbk.accession),
				&indi, indj);

		} else if(Cmpstr(key, "ref")==EQ)	{
			if((rcount+1)>data.gbk.numofref)	{
				/* new reference */
				data.gbk.reference = (Reference*)
				Reallocspace(data.gbk.reference,
				sizeof(Reference)*(rcount+1));
				data.gbk.numofref = rcount+1;
			init_reference(&(data.gbk.reference[rcount]),
					REF);
			} else rcount = data.gbk.numofref - 1;
			data.gbk.reference[rcount++].ref
			= macke_copyrem(data.macke.remarks, &indi,
					data.macke.numofrem, indj);
		} else if(Cmpstr(key, "auth")==EQ)	{
			if((acount+1)>data.gbk.numofref)	{
				/* new reference */
				data.gbk.reference = (Reference*)
				Reallocspace(data.gbk.reference,
				sizeof(Reference)*(acount+1));
				data.gbk.numofref = acount+1;
			init_reference(&(data.gbk.reference[acount]),
					AUTHOR);
			} else acount = data.gbk.numofref - 1;
			data.gbk.reference[acount++].author
			= macke_copyrem(data.macke.remarks, &indi,
					data.macke.numofrem, indj);
		} else if(Cmpstr(key, "title")==EQ)	{
			if((tcount+1)>data.gbk.numofref)	{
				data.gbk.reference = (Reference*)
				Reallocspace(data.gbk.reference,
				sizeof(Reference)*(tcount+1));
				data.gbk.numofref = tcount+1;
			init_reference(&(data.gbk.reference[tcount]),
					TITLE);
			} else tcount = data.gbk.numofref - 1;
			data.gbk.reference[tcount++].title
			= macke_copyrem(data.macke.remarks, &indi,
					data.macke.numofrem, indj);
		} else if(Cmpstr(key, "jour")==EQ) {
			if((jcount+1)>data.gbk.numofref)	{
				data.gbk.reference = (Reference*)
				Reallocspace(data.gbk.reference,
				sizeof(Reference)*(jcount+1));
				data.gbk.numofref = jcount+1;
			init_reference(&(data.gbk.reference[jcount]),
					JOURNAL);
			} else jcount = data.gbk.numofref - 1;
			data.gbk.reference[jcount++].journal
			= macke_copyrem(data.macke.remarks, &indi,
					data.macke.numofrem, indj);
		} else if(Cmpstr(key, "standard")==EQ) {
			if((scount+1)>data.gbk.numofref)	{
				data.gbk.reference = (Reference*)
				Reallocspace(data.gbk.reference,
				sizeof(Reference)*(scount+1));
				data.gbk.numofref = scount+1;
			init_reference(&(data.gbk.reference[scount]),
					STANDARD);
			} else scount = data.gbk.numofref - 1;
			data.gbk.reference[scount++].standard
			= macke_copyrem(data.macke.remarks, &indi,
					data.macke.numofrem, indj);

		} else if(Cmpstr(key, "Source of strain")==EQ)	{

			data.gbk.comments.orginf.exist = 1;
			mtog_copy_remark(
			&(data.gbk.comments.orginf.source),
				&indi, indj);

		} else if(Cmpstr(key, "Former name")==EQ)	{

			data.gbk.comments.orginf.exist = 1;
			mtog_copy_remark(
			&(data.gbk.comments.orginf.formname),
				&indi, indj);

		} else if(Cmpstr(key, "Alternate name")==EQ) {

			data.gbk.comments.orginf.exist = 1;
			mtog_copy_remark(
			&(data.gbk.comments.orginf.nickname),
				&indi, indj);

		} else if(Cmpstr(key, "Common name")==EQ) {

			data.gbk.comments.orginf.exist = 1;
			mtog_copy_remark(
			&(data.gbk.comments.orginf.commname),
				&indi, indj);

		} else if(Cmpstr(key, "Host organism")==EQ) {

			data.gbk.comments.orginf.exist = 1;
			mtog_copy_remark(
			&(data.gbk.comments.orginf.hostorg),
				&indi, indj);

		} else if(Cmpstr(key, "RDP ID")==EQ) {

			data.gbk.comments.seqinf.exist = 1;
			mtog_copy_remark(
			&(data.gbk.comments.seqinf.RDPid),
				&indi, indj);

		} else if(Cmpstr(key, "Sequencing methods")==EQ) {

			data.gbk.comments.seqinf.exist = 1;
			mtog_copy_remark(
			&(data.gbk.comments.seqinf.methods),
				&indi, indj);

		} else if(Cmpstr(key, "3' end complete")==EQ) {

			data.gbk.comments.seqinf.exist = 1;
		sscanf(data.macke.remarks[indi]+indj, "%s", key);
			if(Cmpstr(key, "Yes")==EQ)
				data.gbk.comments.seqinf.comp3 = 'y';
			else	data.gbk.comments.seqinf.comp3 = 'n';

		} else if(Cmpstr(key, "5' end complete")==EQ) {

			data.gbk.comments.seqinf.exist = 1;
		sscanf(data.macke.remarks[indi]+indj, "%s", key);

			if(Cmpstr(key, "Yes")==EQ)
				data.gbk.comments.seqinf.comp5
					= 'y';
			else	data.gbk.comments.seqinf.comp5
					= 'n';

		} else {	/* other comments */
			Cpystr(temp, data.macke.remarks[indi]);
			if(data.gbk.comments.others==NULL)
				data.gbk.comments.others
					= (char*)Dupstr(temp);
			else Append(&(data.gbk.comments.others), temp);
		}
	}	/* for each rem */
}
/* ---------------------------------------------------------------
*	Function mtog_copy_remark().
*		Convert one remark back to GenBank format.
*/
void
mtog_copy_remark(string, indi, indj)
char	**string;
int	*indi, indj;
{
/* 	void	Freespace(); */
/* 	char	*macke_copyrem(); */

	Freespace(string);
	(*string) = (char*)macke_copyrem(data.macke.remarks, indi,
			data.macke.numofrem, indj);
}
/* ------------------------------------------------------------------
 *	Function macke_copyrem().
 *		uncode rem lines.
 */
char *macke_copyrem(strings, index, maxline, pointer)
     char **strings;
     int   *index, maxline, pointer;
{
	char	*string;
	int	indi;
/* 	int	len, length, macke_rem_continue_line(); */
/* 	char	*Catstr(), *Dupstr(); */
/* 	void	Append_rp_eoln(); */

	string = (char*)Dupstr(strings[(*index)]+pointer);
	for(indi=(*index)+1; indi<maxline
	&&macke_rem_continue_line(strings, indi); indi++)
		Append_rp_eoln(&string, strings[indi]+3);
	(*index) = indi-1;
	return(string);
}
/* ------------------------------------------------------------------
*	Function mtog_genbank_def_and_source().
*		Define GenBank DEFINITION and SOURCE lines the way RDP
*			group likes.
*/
void
mtog_genbank_def_and_source()	{

/* 	int	Lenstr(); */
/* 	void	Freespace(), Append(), Append_rm_eoln(), Append_char(); */
/* 	void	replace_entry(), warning(); */

	if(Lenstr(data.macke.name)>1)
		replace_entry(&(data.gbk.definition),
			data.macke.name);

	if(Lenstr(data.macke.subspecies)>1)	{

		if(Lenstr(data.gbk.definition)<=1)	{
			warning(22, "Genus and Species not defined");
			Append_rm_eoln(&(data.gbk.definition), "subsp. ");
		} else Append_rm_eoln(&(data.gbk.definition), " subsp. ");

		Append(&(data.gbk.definition), data.macke.subspecies);
	}

	if(Lenstr(data.macke.strain)>1)	{

		if(Lenstr(data.gbk.definition)<=1)	{
			warning(23,
			"Genus and Species and Subspecies not defined");
			Append_rm_eoln(&(data.gbk.definition), "str. ");
		} else Append_rm_eoln(&(data.gbk.definition), " str. ");

		Append(&(data.gbk.definition), data.macke.strain);
	}

	/* create SOURCE line, temp. */
	if(Lenstr(data.gbk.definition)>1)	{
		replace_entry(&(data.gbk.source), data.gbk.definition);
		Append_char(&(data.gbk.source), '.');
	}

	/* append keyword to definition, if there is keyword. */
	if(Lenstr(data.gbk.keywords)>1)	{

		if(Lenstr(data.gbk.definition)>1)
			Append_rm_eoln(&(data.gbk.definition),
				"; \n");

		/* Here keywords must be ended by a '.' already */
		Append_rm_eoln(&(data.gbk.definition),
			data.gbk.keywords);

	} else Append_rm_eoln((&data.gbk.definition), ".\n");

}
/* -------------------------------------------------------------------
*	Function get_string().
*		Get the rest of the string untill reaching certain
*			terminators, such as ';', ',', '.',...
*		Always append "\n" at the end of the returned string.
*/
void
get_string(line, temp, index)
char	*line, *temp;
int	index;
{
	int	indk, len, paren_num;
/* 	int	not_ending_mark(); */

	len = Lenstr(line);
	paren_num=0;
	for(indk=0; index<len; index++, indk++)	{
		temp[indk]=line[index];
		if(temp[indk]=='(') paren_num++;
		if(temp[indk]==')')
			if(paren_num==0)	break;
			else	paren_num--;
		else if(temp[indk]=='\n'||(paren_num==0
			&&temp[indk]==';'))	break;
	}
	if(indk>1 && !(not_ending_mark(temp[indk-1]))) indk--;
	temp[indk++]='\n';
	temp[indk]='\0';
}
/* -------------------------------------------------------------------
*	Function get_atcc_string().
*		Get the rest of the string untill reaching certain
*			terminators, such as ';', ',', '.',...
*/
void
get_atcc_string(line, temp, index)
char	*line, *temp;
int	index;
{
	int	indk, len, paren_num;

	len = Lenstr(line);
	paren_num=0;
	for(indk=0; index<len; index++, indk++){
		temp[indk]=line[index];
		if(temp[indk]=='(') paren_num++;
		if(temp[indk]==')')
			if(paren_num==0)	break;
			else	paren_num--;
		else if(paren_num==0&&(temp[indk]==';'||temp[indk]=='.'
			||temp[indk]==','||temp[indk]=='/'||temp[indk]=='\n'))
				break;
	}
	temp[indk]='\0';
}
