#include <stdio.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/defaults.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/window.h>
#include <xview/icon.h>
#include <pixrect/pixrect.h>
/* #include <malloc.h> */
#include "menudefs.h"
#include "defines.h"

/*
FreeNASeq():
Destroy a nucleic acid sequence structure, and free its memory usage.

Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/


FreeNASeq(seq)
NA_Sequence *seq;
{
	if(seq->sequence)
		Cfree(seq->sequence);
	if(seq->mask)
		Cfree(seq->mask);
	if(seq->cmask)
		Cfree(seq->cmask);
	if(seq->baggage)
		Cfree(seq->baggage);
	if(seq->comments)
		Cfree(seq->comments);

	if(seq->groupf != NULL && seq->groupb != NULL)
	{
		((NA_Sequence*)(seq->groupf))->groupb = seq->groupb;
		((NA_Sequence*)(seq->groupb))->groupf = seq->groupf;
	}
	return;
}


FreeNAAln(aln)
NA_Alignment*aln;
{
	Cfree(aln->id);
	Cfree(aln->description);
	Cfree(aln->authority);
	Cfree(aln->cmask);
	Cfree(aln->mask);
	if(aln->na_ddata != NULL)
	{
		((NA_DisplayData *)(aln->na_ddata))->aln = NULL;
		FreeNADD(aln->na_ddata);
	}
	Cfree(aln);

	return;
}


FreeNADD(nadd)
NA_DisplayData *nadd;
{
	Cfree(nadd->jumptbl);
	Cfree(nadd);

	return;
}
