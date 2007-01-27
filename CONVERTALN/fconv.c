/* ------------- File format converting subroutine ------------- */

#include <stdio.h>
#include <stdlib.h>
#include "convert.h"
#include "global.h"

/* -------------------------------------------------------------
 *	Function convert().
 *		For given input file and  type, convert the file
 *			to desired out type and save the result in
 *			the out file.
 */
void convert(inf, outf, intype, outype)
     char *inf, *outf;
     int   intype, outype;
{
/* 	void	genbank_to_macke(), genbank_to_genbank(); */
/* 	void	genbank_to_embl(); */
/* 	void	embl_to_macke(), embl_to_genbank(); */
/* 	void	embl_to_embl(); */
/* 	void	macke_to_genbank(), macke_to_embl(); */
/* 	void	to_gcg(), to_paup(), to_phylip(), to_printable(); */
/* 	void	alma_to_macke(), alma_to_genbank(); */
/* 	void	embl_to_alma(), macke_to_alma(), genbank_to_alma(); */
/* 	int	Cmpstr(); */
	int	dd;		/* copy stdin to outfile after first line */
	dd = 0;
	if (outype == PHYLIP2){
	    outype = PHYLIP;
	    dd = 1;
	}

	if(Cmpstr(inf, outf)==EQ)
		error(45, "Input file and output file must be different file.\n");
	       if(intype==GENBANK&&outype==MACKE)	{
		genbank_to_macke(inf, outf);
	} else if(intype==GENBANK&&outype==GENBANK)	{
		genbank_to_genbank(inf, outf);
	} else if(intype==GENBANK&&outype==PAUP)	{
		to_paup(inf, outf, GENBANK);
	} else if(intype==GENBANK&&outype==PHYLIP)	{
		to_phylip(inf, outf, GENBANK,dd);
	} else if(intype==GENBANK&&outype==PROTEIN)	{
		error(69, "Sorry, cannot convert from GENBANK to SWISSPROT, Exit");
	} else if(intype==GENBANK&&outype==EMBL)	{
		genbank_to_embl(inf, outf);
	} else if(intype==GENBANK&&outype==PRINTABLE)	{
		to_printable(inf, outf, GENBANK);
	} else if(intype==GENBANK&&outype==ALMA)	{
		genbank_to_alma(inf, outf);
	} else if(intype==MACKE&&outype==GENBANK)	{
		macke_to_genbank(inf, outf);
	} else if(intype==MACKE&&outype==PAUP)		{
		to_paup(inf, outf, MACKE);
	} else if(intype==MACKE&&outype==PHYLIP)	{
		to_phylip(inf, outf, MACKE,dd);
	} else if(intype==MACKE&&outype==PROTEIN)	{
		macke_to_embl(inf, outf);
	} else if(intype==MACKE&&outype==EMBL)	{
		macke_to_embl(inf, outf);
	} else if(intype==MACKE&&outype==PRINTABLE)	{
		to_printable(inf, outf, MACKE);
	} else if(intype==MACKE&&outype==ALMA)	{
		macke_to_alma(inf, outf);
	} else if(intype==PAUP&&outype==GENBANK)	{
		error(6, "Sorry, cannot convert from Paup to GENBANK, Exit.\n");
	} else if(intype==PAUP&&outype==MACKE)		{
		error(81, "Sorry, cannot convert from Paup to AE2, Exit.\n");
	} else if(intype==PAUP&&outype==PHYLIP)		{
		error(8, "Sorry, cannot convert from Paup to Phylip, Exit.\n");
	} else if(intype==PAUP&&outype==PROTEIN)		{
		error(71, "Sorry, cannot convert from Paup to SWISSPROT, Exit.");
	} else if(intype==PAUP&&outype==EMBL)		{
		error(78, "Sorry, cannot convert from Paup to SWISSPROT, Exit.");
	} else if(intype==PHYLIP&&outype==GENBANK)	{
		error(85, "Sorry, cannot convert from Phylip to GenBank, Exit.\n");
	} else if(intype==PHYLIP&&outype==MACKE)	{
		error(88, "Sorry, cannot convert from Phylip to AE2, Exit.\n");
	} else if(intype==PHYLIP&&outype==PAUP)		{
		error(89, "Sorry, cannot convert from Phylip to Paup, Exit.\n");
	} else if(intype==PHYLIP&&outype==PROTEIN)		{
		error(72, "Sorry, cannot convert from Phylip to SWISSPROT, Exit.\n");
	} else if(intype==PHYLIP&&outype==EMBL)		{
		error(79, "Sorry, cannot convert from Phylip to SWISSPROT, Exit.\n");
	} else if(intype==PROTEIN&&outype==MACKE)	{
		embl_to_macke(inf, outf, PROTEIN);
	} else if(intype==PROTEIN&&outype==GENBANK)	{
		error(78, "GenBank doesn't maintain protein data.");
		/* embl_to_genbank(inf, outf, PROTEIN); */
		/* protein_to_genbank(inf, outf);  */
	} else if(intype==PROTEIN&&outype==PAUP)	{
		to_paup(inf, outf, PROTEIN);
	} else if(intype==PROTEIN&&outype==PHYLIP)	{
		to_phylip(inf, outf, PROTEIN,dd);
	} else if(intype==PROTEIN&&outype==EMBL)	{
		error(58, "EMBL doesn't maintain protein data.");
		/* embl_to_embl(inf, outf, PROTEIN); */
	} else if(intype==PROTEIN&&outype==PRINTABLE)	{
		to_printable(inf, outf, PROTEIN);
	} else if(intype==EMBL&&outype==EMBL)	{
		embl_to_embl(inf, outf, EMBL);
	} else if(intype==EMBL&&outype==GENBANK)	{
		embl_to_genbank(inf, outf, EMBL);
	} else if(intype==EMBL&&outype==MACKE)	{
		embl_to_macke(inf, outf, EMBL);
	} else if(intype==EMBL&&outype==PROTEIN)	{
		embl_to_embl(inf, outf, EMBL);
	} else if(intype==EMBL&&outype==PAUP)	{
		to_paup(inf, outf, EMBL);
	} else if(intype==EMBL&&outype==PHYLIP)	{
		to_phylip(inf, outf, EMBL,dd);
	} else if(intype==EMBL&&outype==PRINTABLE)	{
		to_printable(inf, outf, EMBL);
	} else if(intype==EMBL&&outype==ALMA)	{
		embl_to_alma(inf, outf);
	} else if(intype==ALMA&&outype==MACKE)	{
		alma_to_macke(inf, outf);
	} else if(intype==ALMA&&outype==GENBANK)	{
		alma_to_genbank(inf, outf);
	} else if(intype==ALMA&&outype==PAUP)	{
		to_paup(inf, outf, ALMA);
	} else if(intype==ALMA&&outype==PHYLIP)	{
		to_phylip(inf, outf, ALMA,dd);
	} else if(intype==ALMA&&outype==PRINTABLE)	{
		to_printable(inf, outf, ALMA);
	} else if(outype==GCG)	{
		to_gcg(intype, inf);
	} else	error(90, "Unidentified input type or output type, Exit\n");
}

/* ------------------- COMMON SUBROUTINES ----------------------- */

/* --------------------------------------------------------------
*	Function init().
*	Initialize data structure at the very beginning of running
*		any program.
*/
void
init()	{

    /* initialize macke format */
	data.macke.seqabbr                 = NULL;
	data.macke.name                    = NULL;
	data.macke.atcc                    = NULL;
	data.macke.rna                     = NULL;
	data.macke.date                    = NULL;
	data.macke.nbk                     = NULL;
	data.macke.acs                     = NULL;
	data.macke.who                     = NULL;
	data.macke.remarks                 = NULL;
	data.macke.numofrem                = 0;
	data.macke.rna_or_dna              = 'd';
	data.macke.journal                 = NULL;
	data.macke.title                   = NULL;
	data.macke.author                  = NULL;
	data.macke.strain                  = NULL;
	data.macke.subspecies              = NULL;
    /* initialize gnebank format */
	data.gbk.locus                     = NULL;
	data.gbk.definition                = NULL;
	data.gbk.accession                 = NULL;
	data.gbk.keywords                  = NULL;
	data.gbk.source                    = NULL;
	data.gbk.organism                  = NULL;
	data.gbk.numofref                  = 0;
	data.gbk.reference                 = NULL;
	data.gbk.comments.orginf.exist     = 0;
	data.gbk.comments.orginf.source    = NULL;
	data.gbk.comments.orginf.cc        = NULL;
	data.gbk.comments.orginf.formname  = NULL;
	data.gbk.comments.orginf.nickname  = NULL;
	data.gbk.comments.orginf.commname  = NULL;
	data.gbk.comments.orginf.hostorg   = NULL;
	data.gbk.comments.seqinf.exist     = 0;
	data.gbk.comments.seqinf.RDPid     = NULL;
	data.gbk.comments.seqinf.gbkentry  = NULL;
	data.gbk.comments.seqinf.methods   = NULL;
	data.gbk.comments.seqinf.comp5     = ' ';
	data.gbk.comments.seqinf.comp3     = ' ';
	data.gbk.comments.others           = NULL;
    /* initialize paup format */
	data.paup.ntax                     = 0;
	data.paup.equate                   = "~=.|><";
	data.paup.gap                      = '-';
    /* initial phylip data */
    /* initial embl data */
	data.embl.id                       = NULL;
	data.embl.dateu                    = NULL;
	data.embl.datec                    = NULL;
	data.embl.description              = NULL;
	data.embl.os                       = NULL;
	data.embl.accession                = NULL;
	data.embl.keywords                 = NULL;
	data.embl.dr                       = NULL;
	data.embl.numofref                 = 0;
	data.embl.reference                = NULL;
	data.embl.comments.orginf.exist    = 0;
	data.embl.comments.orginf.source   = NULL;
	data.embl.comments.orginf.cc       = NULL;
	data.embl.comments.orginf.formname = NULL;
	data.embl.comments.orginf.nickname = NULL;
	data.embl.comments.orginf.commname = NULL;
	data.embl.comments.orginf.hostorg  = NULL;
	data.embl.comments.seqinf.exist    = 0;
	data.embl.comments.seqinf.RDPid    = NULL;
	data.embl.comments.seqinf.gbkentry = NULL;
	data.embl.comments.seqinf.methods  = NULL;
	data.embl.comments.seqinf.comp5    = ' ';
	data.embl.comments.seqinf.comp3    = ' ';
	data.embl.comments.others          = NULL;
    /* initial alma data */
	data.alma.id                       = NULL;
	data.alma.filename                 = NULL;
	data.alma.format                   = UNKNOWN;
	data.alma.defgap                   = '-';
	data.alma.num_of_sequence          = 0;
	data.alma.sequence                 = NULL;
    /* inital NBRF data format */
	data.nbrf.id                       = NULL;
	data.nbrf.description              = NULL;
    /* initial sequence data */
	data.numofseq                      = 0;
	data.seq_length                    = 0;
	data.max                           = INITSEQ;
	data.sequence                      = (char*)calloc(1,(unsigned)(sizeof(char)*INITSEQ+1));

	data.ids       = NULL;
	data.seqs      = NULL;
	data.lengths   = NULL;
    data.allocated = 0;
}
/* --------------------------------------------------------------
 *	Function init_seq_data().
 *		Init. seq. data.
*/
void
init_seq_data()	{
	data.numofseq = 0;
	data.seq_length = 0;
}
