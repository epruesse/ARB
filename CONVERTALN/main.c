/* ------------------------------------------------------------	*/
/*							    	*/
/*	Format Conversion Program.				*/
/*								*/
/*		Woese Lab., Dept. of Microbiology, UIUC		*/
/*								*/
/* ------------------------------------------------------------ */
/* if using Dec. Unix, <sys/time.h> doesn't not exist */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "convert.h"
#include "global.h"

/* ---------------------------------------------------------------
 *	Main program:
 *		File conversion;  convert from one file format to
 *		the other.  eg.  Genbank<-->Macke format.
 */
int main(argc, argv)
     int	argc;
     char **argv;
{
	int	intype, outtype;
/* 	int	Lenstr(), file_exist(); */
/* 	void	error(), genbank_to_macke(), macke_to_genbank(); */
/* 	void	Getstr(), change_file_suffix(); */
/* 	char	*Dupstr(), *Reallocspace(); */
/* 	char	error_msg[LINENUM]; */
	char	temp[LINENUM];
	char	choice[LINENUM];

	intype = outtype = UNKNOWN;
	if(argc < 3 || argv[1][0] != '-')	{
		fprintf(stderr,
                "---------------------------------------------------------------\n");
		fprintf(stderr,
                "\n  convert_aln - an alignment and file converter written by\n");
		fprintf(stderr,
                "  WenMin Kuan for the RDP database project.  Please\n");
		fprintf(stderr,
                "  report errors or deficiencies to kuan@phylo.life.uiuc.edu\n");
		fprintf(stderr,
                "\n  Command line usage:\n\n");
		fprintf(stderr,
                "  $ convert_aln -infmt input_file -outfmt output_file, where	\n");
		fprintf(stderr,
                "  infmt can be either GenBank, EMBL, AE2, SwissProt or ALMA\n");
		fprintf(stderr,
                "  outfmt GenBank, EMBL, AE2, PAUP, PHYLIP, GCG, Printable or ALMA\n\n");
		fprintf(stderr,
                "---------------------------------------------------------------\n\n");
        fprintf(stderr, "Select input format (<CR> means default)\n\n");
		fprintf(stderr, "  (1)	GenBank [default]\n");
		fprintf(stderr, "  (2)	EMBL\n");
		fprintf(stderr, "  (3)	AE2\n");
		fprintf(stderr, "  (4)	SwissProt\n");
		fprintf(stderr, "  (5)	ALMA\n");
		fprintf(stderr, "  (6)	Quit\n");
		fprintf(stderr, "  ? ");
		Getstr(choice, LINENUM);
		switch(choice[0])	{
            case '1':	intype = GENBANK;
				break;
            case '2':	intype = EMBL;
				break;
            case '3':	intype = MACKE;
				break;
            case '4':	intype = PROTEIN;
				break;
            case '5':	intype = ALMA;
				break;
            case '6':	exit(0);
            case '\0':	intype = GENBANK;
				break;	/* default selection */
            default :	error(16, "Unknown input file format, EXIT.");
		}
		argv=(char**)calloc(1,sizeof(char*)*5);

		fprintf(stderr, "\nInput file name? ");
		Getstr(temp, LINENUM);
		argv[2] = Dupstr(temp);
		if(!file_exist(temp))
			error(77, "Input file not found, EXIT.");

		/* output file information */
        fprintf(stderr, "\nSelect output format (<CR> means default)\n\n");
		fprintf(stderr, "  (1)	GenBank\n");
		fprintf(stderr, "  (2)	EMBL\n");
		fprintf(stderr, "  (3)	AE2 [default]\n");
		fprintf(stderr, "  (4)	PAUP\n");
		fprintf(stderr, "  (5)	PHYLIP\n");
		fprintf(stderr, "  (A)	PHYLIP2 (insert stdin in first line)\n");
		fprintf(stderr, "  (6)	GCG\n");
		fprintf(stderr, "  (7)	Printable\n");
		fprintf(stderr, "  (8)	ALMA\n");
		fprintf(stderr, "  (9)	Quit\n");
		fprintf(stderr, "  ? ");
		Getstr(choice, LINENUM);
		switch(choice[0])	{
            case '1':	outtype = GENBANK;
				break;
            case '2':	outtype = EMBL;
				break;
            case '3':	outtype = MACKE;
				break;
            case '4':	outtype = PAUP;
				break;
            case '5':	outtype = PHYLIP;
				break;
            case 'A':	outtype = PHYLIP2;
				break;
            case '6':	outtype = GCG;
				break;
            case '7':	outtype = PRINTABLE;
				break;
            case '8':	outtype = ALMA;
				break;
            case '9':	exit(0);
            case '\0':	outtype = MACKE;
				break;
            default	:	error(66, "Unknown output file format, EXIT.");
		}
		change_file_suffix(argv[2], temp, outtype);
		if(outtype!=GCG)	{
			fprintf(stderr, "\nOutput file name [%s]? ", temp);
			Getstr(temp, LINENUM);
			if(Lenstr(temp)==0)
				change_file_suffix(argv[2], temp, outtype);
		}
		argv[4] = Dupstr(temp);
		argc = 5;
	}	else	{ /* processing command line */
		/* input file */
		if(argv[1][1]=='G'||argv[1][1]=='g')
			intype=GENBANK;
		else if(argv[1][1]=='E'||argv[1][1]=='e')
			intype=EMBL;
		else if((argv[1][1]=='A'||argv[1][1]=='a')
                &&(argv[1][2]=='E'|| argv[1][2]=='e'))
			intype=MACKE;
		else if(argv[1][1]=='S'||argv[1][1]=='s')
			intype = PROTEIN;
		else if((argv[1][1]=='A'||argv[1][1]=='a')
                &&(argv[1][2]=='L'||argv[1][2]=='l'))
			intype = ALMA;
		else error(67, "UNKNOWN input file type, EXIT.");

		/* output file */
		if((argv[3][1]=='G'||argv[3][1]=='g')
           &&(argv[3][2]=='E'||argv[3][2]=='e'))
			outtype = GENBANK;
		else if((argv[3][1]=='E'||argv[3][1]=='e'))
			outtype = EMBL;
		else if((argv[3][1]=='A'||argv[3][1]=='a')
                &&(argv[3][2]=='E'||argv[3][2]=='e'))
			outtype = MACKE;
		else if((argv[3][1]=='P'||argv[3][1]=='p')
                &&(argv[3][2]=='a'||argv[3][2]=='A'))
			outtype = PAUP;
		else if(!strncasecmp(argv[3]+1,"ph",2)){
		    if (!strcasecmp(argv[3]+1,"phylip2")){
                outtype = PHYLIP2;
		    }else{
                outtype = PHYLIP;
		    }
		}else if((argv[3][1]=='G'||argv[3][1]=='g')
                 &&(argv[3][2]=='C'||argv[3][2]=='c'))
			outtype = GCG;
		else if((argv[3][1]=='P'||argv[3][1]=='p')
                &&(argv[3][2]=='R'||argv[3][2]=='r'))
			outtype = PRINTABLE;
		else if((argv[3][1]=='A'||argv[3][1]=='a')
                &&(argv[3][2]=='L'||argv[3][2]=='l'))
			outtype = ALMA;
		else error(68, "UNKNOWN output file file, EXIT.");
	}	/* command line */

	if(argc==4)	{               /* default output file */
        /*         argv            = (char**)Reallocspace(argv, sizeof(char*)*5); */
        const char **argv_new = calloc(sizeof(char*), 5);
        memcpy(argv_new, argv, sizeof(char*)*4);
		argv_new[4]     = "";
        argv            = (char**)argv_new;
	}

#ifdef log
	if(outtype!=GCG)
		fprintf(stderr, "\n\nConvert file %s to file %s.\n", argv[2],
                argv[4]);
	else fprintf(stderr, "\n\nConvert file %s to GCG files\n", argv[2]);
#endif

	/* check if output file exists and filename's validation */
	if(file_exist(argv[4]))	{
        sprintf(temp, "Output file %s exists, will be overwritten.", argv[4]);
		warning(151, temp);
	}

	/* file format transfer... */
	convert(argv[2], argv[4], intype, outtype);
	return 0;

}
/* ---------------------------------------------------------------
 *	Function file_type
 *		According to the first line in the file to decide
 *		the file type.  File type could be Genbank, Macke,...
 */
int
file_type(filename)
     char	*filename;
{
/* 	char	line[LINENUM]; */
	char	token[LINENUM], temp[LINENUM];
/* 	int	Cmpstr(); */
	FILE	*fp;
/* 	void	error(); */

	if((fp=fopen(filename, "r"))==NULL)	{
		sprintf(temp, "Cannot open file: %s. Exit.\n", filename);
		error(5, temp);
	}
	/* rewind(fp); */
	fscanf(fp, "%s", token);
	if(Cmpstr(token, "LOCUS")==EQ)
		return(GENBANK);
	else if(Cmpstr(token, "#-")==EQ)
		return(MACKE);
	else if(Cmpstr(token, "ID")==EQ)
		return(PROTEIN);
	else if(Cmpstr(token, "#NEXUS")==EQ)
		return(PAUP);
	else if(isnum(token))
		return(PHYLIP);
	else return(UNKNOWN);
}
/* -----------------------------------------------------------------
 *	Function isnum().
 *		Return TRUE if the string is an integer number.
 */
int
isnum(string)
     char	*string;
{
	int	indi, length, flag;

	for(indi=0, length=Lenstr(string), flag=1; flag&&indi<length; indi++)
		if(!isdigit(string[indi]))	flag = 0;

	return(flag);
}
/* ------------------------------------------------------------------
 *	Function file_exist().
 *		Check if file is already existed and also check file
 *			name is valid or not.
 */
int
file_exist(file_name)
     char	*file_name;
{
/* 	int	Lenstr(); */
/* 	void	error(); */
	char	temp[TOKENNUM];

	if(Lenstr(file_name)<=0)	{
		sprintf(temp, "ilegal file name: %s, EXIT.", file_name);
		error(152, temp);
	}
	return((fopen(file_name, "r")!=NULL));
}
/* -------------------------------------------------------------------
 *	Function change_file_suffix().
 *		Define the default file name by changing suffix.
 */
void
change_file_suffix(old_file, file_name, type)
     char	*old_file, *file_name;
     int	type;
{
	int	indi, indj;
/* 	char	*Catstr(); */
/* 	void	Cpystr(); */

	for(indi=Lenstr(old_file)-1;indi>=0&&old_file[indi]!='.'; indi--)
        if(indi==0)
            Cpystr(file_name, old_file);
        else {
            for(indj=0; indj<(indi-1); indj++)
                file_name[indj]=old_file[indj];
            file_name[indj]='\0';
        }
	switch(type)	{
        case GENBANK:	Catstr(file_name, ".GB");
			break;
        case MACKE:	Catstr(file_name, ".aln");
			break;
        case PAUP:	Catstr(file_name, ".PAUP");
			break;
        case PHYLIP:	Catstr(file_name, ".PHY");
			break;
        case EMBL:	Catstr(file_name, ".EMBL");
			break;
        case PRINTABLE: Catstr(file_name, ".prt");
			break;
        case ALMA: 	Catstr(file_name, ".ALMA");
			break;
        default:	Catstr(file_name, ".???");
	}
}
