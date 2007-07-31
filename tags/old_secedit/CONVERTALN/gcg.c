#include <stdio.h>
#include "convert.h"
#include "global.h"

/* ------------------------------------------------------------------
*	Function to_gcg().
*		Convert from whatever to GCG format.
*/
void
to_gcg(intype, inf)
     int	intype;
     char *inf;
{
	FILE        *IFP1, *IFP2, *IFP3, *ofp;
    FILE_BUFFER  ifp1 = 0, ifp2 = 0, ifp3 = 0;
	char         temp[TOKENNUM], *eof, line[LINENUM], key[TOKENNUM];
	char         line1[LINENUM], line2[LINENUM], line3[LINENUM], name[LINENUM];
	char        *eof1, *eof2, *eof3;
	char         outf[TOKENNUM];
	int	         seqdata;

	if(intype==MACKE)	{
		if((IFP1=fopen(inf, "r"))==NULL||(IFP2=fopen(inf, "r"))==NULL ||(IFP3=fopen(inf, "r"))==NULL)	{
			sprintf(temp, "Cannot open input file %s\n", inf);
			error(38, temp);
		}
        ifp1 = create_FILE_BUFFER(inf, IFP1);
        ifp2 = create_FILE_BUFFER(inf, IFP2);
        ifp3 = create_FILE_BUFFER(inf, IFP3);
	}
    else {
        if((IFP1=fopen(inf, "r")) == NULL)	{
            sprintf(temp, "CANNOT open input file %s, exit.\n", inf);
            error(37, temp);
        }
        ifp1 = create_FILE_BUFFER(inf, IFP1);
	}
	if(intype==MACKE)	{
		/* skip to #=; where seq. first appears */
		for(eof1=Fgetline(line1, LINENUM, ifp1); eof1!=NULL&&(line1[0]!='#'||line1[1]!='=') ;
		eof1=Fgetline(line1, LINENUM, ifp1)) ;
		/* skip to #:; where the seq. information is */
		for(eof2=Fgetline(line2, LINENUM, ifp2); eof2!=NULL&&(line2[0]!='#'||line2[1]!=':');
		eof2=Fgetline(line2, LINENUM, ifp2)) ;
		/* skip to where seq. data starts */
		for(eof3=Fgetline(line3,LINENUM, ifp3); eof3!=NULL&&line3[0]=='#';
		eof3=Fgetline(line3, LINENUM, ifp3)) ;
		/* for each seq. print out one gcg file. */
		for(; eof1!=NULL&&(line1[0]=='#'&&line1[1]=='=');
		eof1=Fgetline(line1, LINENUM, ifp1)) 	{
			macke_abbrev(line1, key, 2);
			Cpystr(temp, key);
			gcg_output_filename(temp, outf);
			if((ofp=fopen(outf, "w"))==NULL) {
				sprintf(temp, "CANNOT open file %s, exit.\n", outf);
				error(39, temp);
			}
			for(macke_abbrev(line2, name, 2);
			eof2!=NULL&&line2[0]=='#'&&line2[1]==':'&&Cmpstr(name, key)==EQ;
			eof2=Fgetline(line2, LINENUM, ifp2), macke_abbrev(line2, name, 2))
				gcg_doc_out(line2, ofp);
			eof3 = macke_origin(key, line3, ifp3);
			gcg_seq_out(ofp, key);
			fclose(ofp);
			init_seq_data();
		}
	} else	{
		seqdata=0;	/* flag of doc or data */
		eof = Fgetline(line, LINENUM, ifp1);
		while(eof!=NULL)	{
			if(intype==GENBANK)	{
				genbank_key_word(line, 0, temp, TOKENNUM);
				if((Cmpstr(temp, "LOCUS"))==EQ)	{
					genbank_key_word(line+12, 0, key, TOKENNUM);
					gcg_output_filename(key, outf);
					if((ofp=fopen(outf, "w"))==NULL) {
						sprintf(temp, "CANNOT open file %s, exit.\n", outf);
						error(40, temp);
					}
				} else if(Cmpstr(temp, "ORIGIN")==EQ)	{
					gcg_doc_out(line, ofp);
					seqdata = 1;
					eof = genbank_origin(line, ifp1);
				}
			} else { /* EMBL or SwissProt */
				if(Lenstr(line)>2&&line[0]=='I'&&line[1]=='D')
				{
					embl_key_word(line, 5, key, TOKENNUM);
					gcg_output_filename(key, outf);
					if((ofp=fopen(outf, "w"))==NULL) {
						sprintf(temp, "CANNOT open file %s, exit.\n", outf);
						error(41, temp);
					}
				} else if(Lenstr(line)>1&&line[0]=='S'
					&&line[1]=='Q')	{
					gcg_doc_out(line, ofp);
					seqdata = 1;
					eof = embl_origin(line, ifp1);
				}
			}
			if(seqdata)	{
				gcg_seq_out(ofp, key);
				init_seq_data();
				seqdata=0;
				fclose(ofp);
			} else {
				gcg_doc_out(line, ofp);
				eof = Fgetline(line, LINENUM, ifp1);
			}
		}
	}
}
/* ----------------------------------------------------------------
*	Function gcg_seq_out().
*		Output sequence data in gcg format.
*/
void
gcg_seq_out(ofp, key)
FILE	*ofp;
char	*key;
{
    /* 	int	indi, indj, indk; */
/* 	char	*today_date(), *gcg_date(); */
/* 	void	gcg_out_origin(); */

	fprintf(ofp, "\n%s  Length: %d  %s  Type: N  Check: %d  ..\n\n", key,
		gcg_seq_length(), gcg_date(today_date()),
		checksum(data.sequence, data.seq_length));
	gcg_out_origin(ofp);
}
/* --------------------------------------------------------------------
*	Function gcg_doc_out().
*		Output non-sequence data(document) of gcg format.
*/
void
gcg_doc_out(line, ofp)
char	*line;
FILE	*ofp;
{
	int	indi, len;
	int	previous_is_dot;

	for(indi=0, len=Lenstr(line), previous_is_dot=0; indi<len; indi++) {
		if(previous_is_dot)	{
			if(line[indi]=='.') fprintf(ofp, " ");
			else previous_is_dot=0;
		}
		fprintf(ofp, "%c", line[indi]);
		if(line[indi]=='.') previous_is_dot=1;
	}
}
/* -----------------------------------------------------------------
*	Function checksum().
*		Calculate checksum for GCG format.
*/
int
checksum(string, numofstr)
char	*string;
int	numofstr;
{
	int	cksum=0, indi, count=0, charnum;

	for(indi=0; indi<numofstr; indi++)	{
		if(string[indi]=='.'||string[indi]=='-'||string[indi]=='~')
			continue;
		count++;
		if(string[indi]>='a'&&string[indi]<='z')
			charnum = string[indi]-'a'+'A';
		else charnum = string[indi];
		cksum = ((cksum+count*charnum) % 10000);
		if(count==57) count=0;
	}
	return(cksum);
}
/* --------------------------------------------------------------------
*	Fcuntion gcg_out_origin().
*		Output sequence data in gcg format.
*/
void
gcg_out_origin(fp)
FILE	*fp;
{

	int	indi, indj, indk;

	for(indi=0, indj=0, indk=1; indi<data.seq_length; indi++)	{
		if(data.sequence[indi]=='.'||data.sequence[indi]=='~'
			||data.sequence[indi]=='-') continue;
		if((indk % 50)==1) fprintf(fp, "%8d  ", indk);
		fprintf(fp, "%c", data.sequence[indi]);
		indj++;
		if(indj==10) { fprintf(fp, " "); indj=0; }
		if((indk % 50)==0) fprintf(fp, "\n\n");
		indk++;
	}
	if((indk % 50)!=1) fprintf(fp, " \n");
}
/* --------------------------------------------------------------
*	Function gcg_output_filename().
*		Get gcg output filename, convert all '.' to '_' and
*			append ".RDP" as suffix.
*/
void
gcg_output_filename(prefix, name)
char	*prefix, *name;
{
	int	indi, len;

	for(indi=0, len=Lenstr(prefix); indi<len; indi++)
		if(prefix[indi]=='.') prefix[indi]='_';
	sprintf(name, "%s.RDP", prefix);
}
/* ------------------------------------------------------------------
*	Function gcg_seq_length().
*		Calculate sequence length without gap.
*/
int
gcg_seq_length()	{

	int	indi, len;

	for(indi=0, len=data.seq_length; indi<data.seq_length; indi++)
		if(data.sequence[indi]=='.'||data.sequence[indi]=='-'
		||data.sequence[indi]=='~')	len--;

	return(len);
}
