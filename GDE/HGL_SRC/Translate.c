#include <stdio.h>
/* #include <malloc.h> */
#include <string.h>
#include "global_defs.h"

char vert_mito[512][4] =
{
"AAA","Lys", "AAC","Asn", "AAG","Lys", "AAT","Asn", "ACA","Thr",
"ACC","Thr", "ACG","Thr", "ACT","Thr", "AGA","Ter", "AGC","Ser",
"AGG","Ter", "AGT","Ser", "ATA","Met", "ATC","Ile", "ATG","Met",
"ATT","Ile", "CAA","Gln", "CAC","His", "CAG","Gln", "CAT","His",
"CCA","Pro", "CCC","Pro", "CCG","Pro", "CCT","Pro", "CGA","Arg",
"CGC","Arg", "CGG","Arg", "CGT","Arg", "CTA","Leu", "CTC","Leu",
"CTG","Leu", "CTT","Leu", "GAA","Glu", "GAC","Asp", "GAG","Glu",
"GAT","Asp", "GCA","Ala", "GCC","Ala", "GCG","Ala", "GCT","Ala",
"GGA","Gly", "GGC","Gly", "GGG","Gly", "GGT","Gly", "GTA","Val",
"GTC","Val", "GTG","Val", "GTT","Val", "TAA","Ter", "TAC","Tyr",
"TAG","Ter", "TAT","Tyr", "TCA","Ser", "TCC","Ser", "TCG","Ser",
"TCT","Ser", "TGA","Trp", "TGC","Cys", "TGG","Trp", "TGT","Cys",
"TTA","Leu", "TTC","Phe", "TTG","Leu", "TTT","Phe"
 },
mycoplasma[512][4] =
{
"AAA","Lys", "AAC","Asn", "AAG","Lys", "AAT","Asn", "ACA","Thr",
"ACC","Thr", "ACG","Thr", "ACT","Thr", "AGA","Arg", "AGC","Ser",
"AGG","Arg", "AGT","Ser", "ATA","Ile", "ATC","Ile", "ATG","Met",
"ATT","Ile", "CAA","Gln", "CAC","His", "CAG","Gln", "CAT","His",
"CCA","Pro", "CCC","Pro", "CCG","Pro", "CCT","Pro", "CGA","Arg",
"CGC","Arg", "CGG","Arg", "CGT","Arg", "CTA","Leu", "CTC","Leu",
"CTG","Leu", "CTT","Leu", "GAA","Glu", "GAC","Asp", "GAG","Glu",
"GAT","Asp", "GCA","Ala", "GCC","Ala", "GCG","Ala", "GCT","Ala",
"GGA","Gly", "GGC","Gly", "GGG","Gly", "GGT","Gly", "GTA","Val",
"GTC","Val", "GTG","Val", "GTT","Val", "TAA","Ter", "TAC","Tyr",
"TAG","Ter", "TAT","Tyr", "TCA","Ser", "TCC","Ser", "TCG","Ser",
"TCT","Ser", "TGA","Trp", "TGC","Cys", "TGG","Trp", "TGT","Cys",
"TTA","Leu", "TTC","Phe", "TTG","Leu", "TTT","Phe" },
universal[512][4] =
{
"AAA","Lys", "AAC","Asn", "AAG","Lys", "AAT","Asn", "ACA","Thr",
"ACC","Thr", "ACG","Thr", "ACT","Thr", "AGA","Arg", "AGC","Ser",
"AGG","Arg", "AGT","Ser", "ATA","Ile", "ATC","Ile", "ATG","Met",
"ATT","Ile", "CAA","Gln", "CAC","His", "CAG","Gln", "CAT","His",
"CCA","Pro", "CCC","Pro", "CCG","Pro", "CCT","Pro", "CGA","Arg",
"CGC","Arg", "CGG","Arg", "CGT","Arg", "CTA","Leu", "CTC","Leu",
"CTG","Leu", "CTT","Leu", "GAA","Glu", "GAC","Asp", "GAG","Glu",
"GAT","Asp", "GCA","Ala", "GCC","Ala", "GCG","Ala", "GCT","Ala",
"GGA","Gly", "GGC","Gly", "GGG","Gly", "GGT","Gly", "GTA","Val",
"GTC","Val", "GTG","Val", "GTT","Val", "TAA","Ter", "TAC","Tyr",
"TAG","Ter", "TAT","Tyr", "TCA","Ser", "TCC","Ser", "TCG","Ser",
"TCT","Ser", "TGA","Ter", "TGC","Cys", "TGG","Trp", "TGT","Cys",
"TTA","Leu", "TTC","Phe", "TTG","Leu", "TTT","Phe" },
yeast[512][4] =
{
"AAA","Lys", "AAC","Asn", "AAG","Lys", "AAT","Asn", "ACA","Thr",
"ACC","Thr", "ACG","Thr", "ACT","Thr", "AGA","Arg", "AGC","Ser",
"AGG","Arg", "AGT","Ser", "ATA","Met", "ATC","Ile", "ATG","Met",
"ATT","Ile", "CAA","Gln", "CAC","His", "CAG","Gln", "CAT","His",
"CCA","Pro", "CCC","Pro", "CCG","Pro", "CCT","Pro", "CGA","Arg",
"CGC","Arg", "CGG","Arg", "CGT","Arg", "CTA","Thr", "CTC","Thr",
"CTG","Thr", "CTT","Thr", "GAA","Glu", "GAC","Asp", "GAG","Glu",
"GAT","Asp", "GCA","Ala", "GCC","Ala", "GCG","Ala", "GCT","Ala",
"GGA","Gly", "GGC","Gly", "GGG","Gly", "GGT","Gly", "GTA","Val",
"GTC","Val", "GTG","Val", "GTT","Val", "TAA","Ter", "TAC","Tyr",
"TAG","Ter", "TAT","Tyr", "TCA","Ser", "TCC","Ser", "TCG","Ser",
"TCT","Ser", "TGA","Trp", "TGC","Cys", "TGG","Trp", "TGT","Cys",
"TTA","Leu", "TTC","Phe", "TTG","Leu", "TTT","Phe"
};


char three_to_one[23][5] = {
"AlaA", "ArgR", "AsnN", "AspD",
"AsxB", "CysC", "GlnQ", "GluE",
"GlxZ", "GlyG", "HisH", "IleI",
"LeuL", "LysK", "MetM", "PheF",
"ProP", "SerS", "ThrT", "TrpW",
"TyrY", "ValV", "Ter*"
};



main(ac,av)
int ac;
char **av;
{
	int Success = TRUE,cursize,tbl,i,j,k,maxsize = 10,frame=1,
		min_frame = 0,ltrs=0, sep=0, tmp_num_frame, print_comp=FALSE;
	Sequence temp,*seqs;
	FILE *file;
	char number[5],tr_tbl[512][4];
	extern char *Realloc();
	extern Translate_NA_AA();


	if(ac == 1)
	{
	fprintf(stderr,
			"Options:[-tbl codon_table] [-frame #]      [-min_frame #]  [-3]          [-sep]         GDEfile\n");
	fprintf(stderr,"       1=universal         1=first frame    Shortest AA     Three letter  don't seperate\n");
	fprintf(stderr,"       2=mycoplasma        2=second frame   sequence        codes         groups\n");
	fprintf(stderr,"       3=yeast             3=third frame    to translate\n");
	fprintf(stderr,"       4=Vert. mito.       6=All six\n");
	exit(0);
	}
	for(j=1;j<ac-1;j++)
	{
		if(strcmp(av[j],"-tbl")==0)
		{
			sscanf(av[++j],"%d",&tbl);
		}
		if(strcmp(av[j],"-frame")==0)
		{
			sscanf(av[++j],"%d",&frame);
		}
		if(strcmp(av[j],"-min_frame")==0)
		{
			sscanf(av[++j],"%d",&min_frame);
		}
		if(strcmp(av[j],"-3")==0)
		{
			ltrs = 1;
		}
		if(strcmp(av[j],"-sep")==0)
		{
			sep = 1;
		}
	}
	file = fopen(av[ac-1],"r");

	if(file == NULL)
		exit(-1);

	seqs = (Sequence*)Calloc(maxsize,sizeof(Sequence));
	for(cursize = 0;Success == TRUE;cursize++)
	{
		Success = ReadRecord(file,&(seqs[cursize]));
		if(cursize == maxsize-1)
		{
			maxsize *= 2;
			seqs = (Sequence*)Realloc(seqs, maxsize*sizeof(Sequence));
		}
		if(Success==TRUE)
		{
			seqs[cursize].group_number = 99999;
		}
	}
	cursize--;

	tmp_num_frame = frame;    /*added by jckelley, 7/2/92 */
	for(j=0;j<cursize;j++)
	{
		frame = tmp_num_frame;  /*added by jckelley, 7/2/92 */
		if(frame == 6)
			for(i=0;i<2;i++)
			{
				for(k=0;k<3;k++)
				{
					SeqCopy(&(seqs[j]),&temp);
					if(i==0)
					sprintf(number,"_%d",k+1);
					else
					sprintf(number,"_comp%d",k+1);
					strncat(temp.name,number,32);
					switch (tbl)
					{
					case 3:
						Translate_NA_AA(&temp,yeast,k,min_frame,ltrs,sep);
						break;
					case 4:
						Translate_NA_AA(&temp,vert_mito,k,min_frame,ltrs,sep);
						break;
					case 2:
						Translate_NA_AA(&temp,mycoplasma,k,min_frame,ltrs,sep);
						break;
					case 1:
					default:
						Translate_NA_AA(&temp,universal,k,min_frame,ltrs,sep);
						break;
					}
/*					WriteRecord(stdout,&temp,NULL,0); */
				}
				SeqRev(&(seqs[j]),seqs[j].offset,seqs[j].seqlen+
				    seqs[j].offset-1);
				SeqComp(&(seqs[j]));
			}
		else
		{
			frame = (frame-1)%3;
			SeqCopy(&(seqs[j]),&temp);
			sprintf(number,"_%d",frame+1);
			strncat(temp.name,number,32);
			switch(tbl)
			{
			case 3:
				Translate_NA_AA(&temp,yeast,frame,min_frame,ltrs,sep);
				break;
			case 4:
				Translate_NA_AA(&temp,vert_mito,frame,min_frame,ltrs,sep);
				break;
			case 2:
				Translate_NA_AA(&temp,mycoplasma,frame,min_frame,ltrs,sep);
				break;
			case 1:
			default:
				Translate_NA_AA(&temp,universal,frame,min_frame,ltrs,sep);
				break;
			}
		}
	}
	exit(0);
}

Translate_NA_AA(seq,table,r_frame,min_frame,letter_code,sep)
Sequence *seq;
char table[][4];
int r_frame,min_frame,letter_code, sep;
{
	extern char *Realloc(), ThreeToOne();
	int i,true_start,k,pos,fptr = 0,start = 0;
	int last_start = 0,codon_count = 0;
	char c,codon[4],*temp, *save_c_elem, save_name[80], strtmp[80];
	extern int Default_PROColor_LKUP[];
	int grp = 0;


	codon[3] = '\0';
	fptr = 0;

	/* save_name = malloc(80); */

	strncpy(save_name, seq->name, 80);
	temp=(char*)Calloc(seq->seqlen+1,sizeof(char));
	if (!temp) exit(-1);

	for(i=0;i<seq->seqlen;i++)
		temp[i] = '-';

	if(letter_code == 1)
	{
/*
*	Triple letter codes
*/
		strcpy(seq->type,"TEXT");
	}
	else
	{
/*
*	Single Letter Codes
*/
		strcpy(seq->type , "PROT");
	}

/*
*       Skip over r_frame valid characters (skip ' ','-','~')
*/
	for(true_start=0,i=0; i<r_frame&&true_start<
	    seq->seqlen;true_start++)
	{
		c=seq->c_elem[true_start];
		if(strchr(" -~",c) == NULL)
			i++;
	}
	for(pos=true_start;pos<seq->seqlen;pos++)
	{
		c=seq->c_elem[pos];
		if(strchr(" -~",c) == NULL)
		{
			c &= (255-32);	/*upper case*/
			if(c == 'U') c = 'T';
/*
*	We have a valid character...
*/
			if(fptr == 0)
				start = pos;

			codon[fptr++] = c;
			/*
*	Translate the codon...
*/
			if(fptr == 3)
			{
				/*
*		Place default code 'X' in case translation fails
*/
				temp[start] = 'X';
				for(i=0;i<512;i+=2)
					if(strcmp(codon,table[i]) == 0)
					{
						if(letter_code == 1)
						{
							temp[start] = table[i+1][0];
							temp[start+1] = table[i+1][1];
							temp[start+2] = table[i+1][2];
						}
						else
							temp[start] = ThreeToOne
							    (table[i+1]);
						i = 512;
					}
				fptr = 0;
/*
*	Check to see if it is a valid ORF
*/
				if((strncmp("Ter",&(temp[start]),3) == 0) ||
				   (temp[start] == '*'))
				{
/*
*	If the ORF is too small, clear it out...
*/
					if(codon_count < min_frame)
					{
/*
 * Should we seperate the groups out, or not?!?!
 *
 */
					  if (!sep)
					  {
/*
*	If reading from stop to stop, leave the elading stop codon
*/
					    if(temp[last_start]=='*' ||
					       strncmp("Ter",&(temp[last_start]),3) == 0)
					      for(i=last_start+3;i<start+3;i++)
						temp[i] = '-';
/*
*	Otherwise, we are at the first coding region, no stop codon...
*/
					    else
					      for(i=last_start;i<start+3;i++)
						temp[i] = '-';
					  }
					  else
					  {
					    for(i=0;i<seq->seqlen;i++)
					      temp[i] = '-';
					  }
					}
					else
					{
					  if (sep)
					  {
					    sprintf(strtmp, "_%d", grp++);
					    strncat(seq->name, strtmp, 20);
					    save_c_elem = seq->c_elem;
					    seq->c_elem = temp;
					    WriteRecord(stdout,seq,NULL,0);
					    strncpy(seq->name, save_name, 80);
					    seq->c_elem = save_c_elem;
					    for(i=0;i<seq->seqlen;i++)
					      temp[i] = '-';
					  }
					}
					codon_count = 0;
					last_start = start;
/*
 * Don't bother continuing if too close to end to meat min_frame requirements
 *
 */
					if ((pos + (3 * min_frame)) > seq->seqlen)
					  break;
				}
				else
					codon_count++;
			}
		}
	}
	Cfree(seq->c_elem);
	if (!sep)
	{
	  seq->c_elem = temp;
	  WriteRecord(stdout,seq,NULL,0);
	}
	Cfree(temp);
	return;
}


char ThreeToOne(s)
char s[];
{
	extern char three_to_one[][5];
	int j;

	for(j=0;j<23;j++)
		if(strncmp(s,three_to_one[j],3) == 0)
			return(three_to_one[j][3]);
	Warning("ThreeToOne, code not found");
	return('*');
}



