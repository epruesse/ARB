#include <stdio.h>
/* #include <malloc.h> */
#include <string.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/scrollbar.h>
#include <xview/panel.h>
#include <xview/window.h>
#include <xview/notice.h>
#include <xview/textsw.h>
#include "menudefs.h"
#include "defines.h"
    extern Canvas EditCan,EditNameCan;
    extern Panel_item left_foot,right_foot;
	extern Frame frame;
	extern NA_Alignment *DataSet,*Clipboard;

/*

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.
*/
EditCut(item,event)
Panel_item item;
Event *event;
{
	char buffer[80];

	int j,numselected=0,numshifted = 0;

	if(TestSelection() == SELECT_REGION)
		return(EditSubCut(item,event));

	for(j=0;j<Clipboard->numelements;j++)
	{
		FreeNASeq(&Clipboard->element[j]);
		InitNASeq(&(Clipboard->element[j]),TEXT);
	}
	Clipboard->numelements = 0;

	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].selected)
		{
			if(numselected >= Clipboard->maxnumelements-1)
			{
				Clipboard->maxnumelements += 10;
				Clipboard->element = (NA_Sequence*)Realloc
				(Clipboard->element,
				Clipboard->maxnumelements*sizeof(NA_Sequence));
			}
			Clipboard->element[(Clipboard->numelements)] =
			DataSet->element[j];
/*
*	Map sequences back into their global positions, as we will
*	normailze the alignment after they are copied out.
*/
			Clipboard->element[(Clipboard->numelements)++].offset+=
			DataSet->rel_offset;

			numselected++;
		}

	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].selected)
			numshifted++;
		else
			DataSet->element[j-numshifted] =
			DataSet->element[j];

	DataSet->numelements -= numshifted;

	NormalizeOffset(DataSet);

	SetNADData(DataSet,EditCan,EditNameCan);

	Regroup(DataSet);

	RepaintAll(TRUE);
	sprintf(buffer,"%d sequence in Sequence Clipboard",numselected);
	xv_set(frame,FRAME_RIGHT_FOOTER,buffer,0);
	xv_set(right_foot,PANEL_LABEL_STRING,buffer,0);

	return(XV_OK);
}

EditCopy(item,event)
Panel_item item;
Event *event;
{
	extern Frame frame;
    extern Canvas EditCan,EditNameCan;
    extern NA_Alignment *DataSet,*Clipboard;
	char buffer[80];

	int i,j,numselected=0,numshifted = 0,this;

	if(TestSelection() == SELECT_REGION)
		return(EditSubCopy(item,event));

	for(j=0;j<Clipboard->numelements;j++)
	{
		FreeNASeq(&Clipboard->element[j]);
		InitNASeq(&(Clipboard->element[j]),TEXT);
	}
	Clipboard->numelements = 0;

	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].selected)
		{
			this = Clipboard->numelements;
			if(numselected >= Clipboard->maxnumelements-1)
			{
				Clipboard->maxnumelements += 10;
				Clipboard->element = (NA_Sequence*)Realloc
				(Clipboard->element,
				Clipboard->maxnumelements*sizeof(NA_Sequence));
				InitNASeq(&(Clipboard->element
				[this]),
				DataSet->element[j].elementtype);
			}
			Clipboard->element[this] = DataSet->element[j];
/*
*	Handle comments
*/
			if(DataSet->element[j].comments)
			{
				Clipboard->element[this].comments = (char*)
				strdup(DataSet->element[j].comments);
				Clipboard->element[this].comments_maxlen =
				Clipboard->element[this].comments_len;
			}
/*
*	And baggage
*/
			if(DataSet->element[j].baggage)
			{
				Clipboard->element[this].baggage = (char*)
				strdup(DataSet->element[j].baggage);
				Clipboard->element[this].baggage_maxlen =
				Clipboard->element[this].baggage_len;
			}

			Clipboard->element[this].cmask = NULL;
			Clipboard->element[this].sequence
			= (NA_Base*)Calloc(DataSet->element[j].seqmaxlen,
			sizeof(NA_Base));
			for(i=0;i<DataSet->element[j].seqlen;i++)
				Clipboard->element[Clipboard->numelements].
				sequence[i] = DataSet->element[j].sequence[i];

/*
				putelem(&(Clipboard->element[Clipboard->
				numelements]),i,
				getelem(&(DataSet->element[j]),i));
*/
/*
*	Map sequences back into their global positions, as we will
*	normailze the alignment after they are copied out.
*/
			Clipboard->element[(Clipboard->numelements)].offset +=
			DataSet->rel_offset;

			(Clipboard->numelements)++;
			numselected++;
		}
	sprintf(buffer,"%d sequence in Clipboard",numselected);
	xv_set(frame,FRAME_RIGHT_FOOTER,buffer,0);
	xv_set(right_foot,PANEL_LABEL_STRING,buffer,0);

	return(XV_OK);
}

EditPaste(item,event)
Panel_item item;
Event *event;
{

	extern Frame frame;
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet,*Clipboard;
	extern TextClipSize;
	int j,last = -1;

	if(TextClipSize != 0)
		{
		if(Clipboard->numelements == 0)
		return(EditSubPaste(item,event));
		else if(notice_prompt(frame,NULL,NOTICE_MESSAGE_STRINGS,
				"You have data in both clipboards, do you",
		"wish to paste from the...",
		NULL,
		NOTICE_BUTTON,"Sequence clipboard",1,
		NOTICE_BUTTON,"Text clipboard",2,
		0) == 2)
		return(EditSubPaste(item,event));
		}


	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].selected)
			last = j;

	if(DataSet->maxnumelements <= DataSet->numelements+
	Clipboard->numelements)
	{
		DataSet->maxnumelements+=Clipboard->numelements;
		DataSet->element =(NA_Sequence*)Realloc(DataSet->element,
		DataSet->maxnumelements*sizeof(NA_Sequence));
	}

	for(j=DataSet->numelements-1;j>=last+1;j--)
	DataSet->element[j+Clipboard->numelements] =
	DataSet->element[j];

	for(j=0;j<Clipboard->numelements;j++)
	{
		DataSet->element[last+1+j] = Clipboard->element[j];
/*
*	be sure to bring them back into alignment with the rest
*/
		DataSet->element[last+1+j].offset -= DataSet->rel_offset;
	}
	DataSet->numelements += Clipboard->numelements;

/*
	for(j=0;j<Clipboard->numelements;j++)
	{
		FreeNASeq(Clipboard->element[j]);
		InitNASeq(&(Clipboard->element[j]),TEXT);
	}
*/
	Clipboard->numelements = 0;

	NormalizeOffset(DataSet);

	SetNADData(DataSet,EditCan,EditNameCan);

	Regroup(DataSet);

	RepaintAll(TRUE);
	xv_set(frame,FRAME_RIGHT_FOOTER,"Clipboard empty",0);
	xv_set(right_foot,PANEL_LABEL_STRING,"Clipboard empty",0);
	return(XV_OK);
}


Regroup(alignment)
NA_Alignment *alignment;
{

	int j,group,last;

	for(j=0;j<alignment->numelements;j++)
	{
		alignment->element[j].groupf = NULL;
		alignment->element[j].groupb = NULL;
	}

	for(group = 1;group <= alignment->numgroups;group++)
	{
		last = -1;
		for(j=0;j<alignment->numelements;j++)
			if(alignment->element[j].groupid == group)
			{
				if(last != -1)
				{
					alignment->element[j].groupb =
					&(alignment->element[last]);
					alignment->element[last].groupf =
					&(alignment->element[j]);
				}
				last = j;
			}
	}
	return;
}

EditSubCut(item,event)
Panel_item item;
Event *event;
{
	extern Frame frame;
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet,*Clipboard;
	extern char **TextClip;
	extern int TextClipSize,TextClipLength;
	int blank_space = 0;

	NA_Sequence *this_elem;
	char buffer[80];

	int j,i,k,numselected=0,numshifted = 0,columns=0;

/*
*	Check how many columns selected
*/

	if(DataSet->selection_mask == NULL)
		return;

	for(j=0;j<DataSet->maxlen;j++)
		if(DataSet->selection_mask[j] == '1')
			columns++;

	if(columns == 0)
		return;
/*
*	Free old Text clipboard
*/
	if(SubCutViolate())
	{
		Warning("Cut violates current protections");
		return XV_OK;
	}

	for(j=0;j<TextClipSize;j++)
		Cfree(TextClip[j]);
	TextClipSize = 0;

	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].subselected)
			TextClipSize++;

	if(TextClipSize == 0)
		return;

	TextClip = (char**)Calloc(TextClipSize,sizeof(char*));

	for(j=0;j<TextClipSize;j++)
		TextClip[j] = (char *)Calloc(columns,sizeof(char));


	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].subselected)
		{
			this_elem = &(DataSet->element[j]);
/*
*	Need to check protections
*/

			for(i=0,blank_space = 0;i<this_elem->offset;i++)
				if(DataSet->selection_mask[i] == '1')
					TextClip[numselected][blank_space++] =
						(char)getelem(this_elem,i);


			for(i=0,k=0;i<this_elem->seqlen;i++)
			{
				if(DataSet->selection_mask[i+this_elem->offset]
				== '1')
				{
					if(this_elem->tmatrix)
					TextClip[numselected][k++] =
						this_elem->tmatrix[
						(char)getelem(this_elem,
						i+this_elem->offset)];
					else
					TextClip[numselected][k++] =
						(char)getelem(this_elem,
						i+this_elem->offset);
				}

				if((k!=0) && (i<this_elem->seqlen-1))
				{
					this_elem->sequence[1+i-k] =
					this_elem->sequence[1+i];
					if(this_elem->cmask)
					{
						this_elem->cmask[1+i-k] =
						this_elem->cmask[1+i];
					}
				}
			}

			numselected++;
			this_elem->seqlen -= k;
			this_elem->offset -= blank_space;
/*
*		This might cause problems later on if the selection mask is
*		not cleaned...Make sure you test for subselected, not just
*		for a non-0 mask.
*/
			this_elem->subselected = FALSE;
		}

	TextClipLength = columns;
	NormalizeOffset(DataSet);

	SetNADData(DataSet,EditCan,EditNameCan);

	Regroup(DataSet);

	RepaintAll(TRUE);

	sprintf(buffer,"%d bytes in Text Clipboard",numselected*columns);
	xv_set(frame,FRAME_RIGHT_FOOTER,buffer,0);
	xv_set(right_foot,PANEL_LABEL_STRING,buffer,0);

	return(XV_OK);
}



SubCutViolate()
{
	int i,j;
	NA_Sequence *this_elem;
	char base,tbase;
	int GAP=FALSE,UNAMB=FALSE,AMB=FALSE,prot;

	for(j=0;j<DataSet->numelements;j++)
	{
		this_elem = &(DataSet->element[j]);
		prot = this_elem->protect;
		if((this_elem->subselected)&& (this_elem->elementtype!=TEXT))
		{
			for(i=0;i<DataSet->maxlen;i++)
			    if(DataSet->selection_mask[i] == '1')
			    {
				base = (char)getelem(this_elem,i);
				if(this_elem->tmatrix)
					tbase = (this_elem->tmatrix[base])|32;
				switch(this_elem->elementtype)
				{
					case DNA:
					case RNA:
						if((base&15) == 0)
							GAP=TRUE;
						else if(tbase == 'n')
							AMB=TRUE;
						else
							UNAMB = TRUE;
						break;
					case PROTEIN:
						if(base == '-' ||
						   base == '~' ||
						   base == ' ')
							GAP=TRUE;
						else if(base == 'X' ||
							base == 'x')
							AMB=TRUE;
						else
							UNAMB = TRUE;
						break;
					case MASK:
						if(base == '0')
							GAP = TRUE;
						else
							UNAMB = TRUE;
						break;
					default:
						break;
				}
				if(((prot & PROT_WHITE_SPACE)==0) && GAP)
					return(TRUE);
				else if(((prot & PROT_GREY_SPACE)==0) && AMB)
					return(TRUE);
				else if(((prot & PROT_BASE_CHANGES)==0) &&
				UNAMB)
					return(TRUE);
			    }
		}
	}
	return FALSE;
}



EditSubPaste(item,event)
Panel_item item;
Event *event;
{
	extern Frame frame;
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet,*Clipboard;
	extern char **TextClip;
	extern int TextClipSize,TextClipLength;
	int *temp_cmask,cursorx,cursory;

	NA_Sequence *this_elem;
	char buffer[80];

	int j,i,k,violate = FALSE;

	if(DataSet->selection_mask == NULL)
		return(XV_OK);

	if(TextClipSize == 0 || TextClipLength==0)
		return(XV_OK);

	cursorx = ((NA_DisplayData*)(DataSet->na_ddata))->cursor_x;
	cursory = ((NA_DisplayData*)(DataSet->na_ddata))->cursor_y;

	if(cursory + TextClipSize > DataSet->numelements)
	{
		Warning("Can't paste a block there.");
		return(XV_OK);
	}

	for(j=0;j<TextClipSize;j++)
	{
		this_elem = &(DataSet->element[j+cursory]);
		violate |= InsertViolate(DataSet,this_elem,
			TextClip[j],cursorx,TextClipLength);
	}
	if(violate == FALSE)
	{
		for(j=0;j<TextClipSize;j++)
		{
			this_elem = &(DataSet->element[j+cursory]);
			InsertNA(this_elem,TextClip[j],TextClipLength,cursorx);
		}
	}

	RepaintAll(TRUE);
	sprintf(buffer,"%d bytes in Text Clipboard",TextClipLength *
			TextClipSize);
	xv_set(frame,FRAME_RIGHT_FOOTER,buffer,0);
	xv_set(right_foot,PANEL_LABEL_STRING,buffer,0);
	return(XV_OK);
}



EditSubCopy(item,event)
Panel_item item;
Event *event;
{
	extern Frame frame;
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet,*Clipboard;
	extern char **TextClip;
	extern int TextClipSize,TextClipLength;
	int blank_space = 0;

	NA_Sequence *this_elem;
	char buffer[80];

	int j,i,k,numselected=0,numshifted = 0,columns=0;

/*
*	Check how many columns selected
*/

	if(DataSet->selection_mask == NULL)
		return;

	for(j=0;j<DataSet->maxlen;j++)
		if(DataSet->selection_mask[j] == '1')
			columns++;

	if(columns == 0)
		return;
/*
*	Free old Text clipboard
*/
	for(j=0;j<TextClipSize;j++)
		Cfree(TextClip[j]);
	TextClipSize = 0;

	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].subselected)
			TextClipSize++;

	if(TextClipSize == 0)
		return;

	TextClip = (char**)Calloc(TextClipSize,sizeof(char*));

	for(j=0;j<TextClipSize;j++)
		TextClip[j] = (char *)Calloc(columns,sizeof(char));


	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].subselected)
		{
			this_elem = &(DataSet->element[j]);
/*
*	Need to check protections
*/

			for(i=0,blank_space = 0;i<this_elem->offset;i++)
				if(DataSet->selection_mask[i] == '1')
					TextClip[numselected][blank_space++] =
						(char)getelem(this_elem,i);


			for(i=0,k=0;i<this_elem->seqlen;i++)
			{
				if(DataSet->selection_mask[i+this_elem->offset]
				== '1')
				{
					if(this_elem->tmatrix)
					TextClip[numselected][k++] =
						this_elem->tmatrix[
						(char)getelem(this_elem,
						i+this_elem->offset)];
					else
					TextClip[numselected][k++] =
						(char)getelem(this_elem,
						i+this_elem->offset);
				}

			}

			numselected++;
			this_elem->subselected = FALSE;
		}

	TextClipLength = columns;

	SetNADData(DataSet,EditCan,EditNameCan);

	RepaintAll(TRUE);

	sprintf(buffer,"%d bytes in Text Clipboard",numselected*columns);
	xv_set(frame,FRAME_RIGHT_FOOTER,buffer,0);
	xv_set(right_foot,PANEL_LABEL_STRING,buffer,0);

	return(XV_OK);
}

int TestSelection()
{
    int j,select_mode = 0,selected = 0,subselected = 0;
    extern NA_Alignment *DataSet;
	extern Frame frame;

    for(j=0;j<((NA_Alignment*)DataSet)->numelements;j++)
    {
	selected|=((NA_Alignment*)DataSet)->element[j].selected;
	subselected|=((NA_Alignment*)DataSet)->element[j].subselected;
    }

    if (!(selected || subselected))
	Warning("Warning, no sequences selected");

    if(selected && !subselected)
	select_mode = SELECTED;

    if(!selected && subselected)
	select_mode = SELECT_REGION;

    if(selected && subselected)
	select_mode = notice_prompt(frame,NULL,NOTICE_MESSAGE_STRINGS,
	"Do you want to use the...",NULL,
	NOTICE_BUTTON,"Selected sequences",SELECTED,
	NOTICE_BUTTON,"Selected regions",SELECT_REGION,
	NULL);
/*
	for(j=0;j<((NA_Alignment*)DataSet)->numelements;j++)
	{
		if(select_mode == SELECT_REGION)
			((NA_Alignment*)DataSet)->element[j].selected = FALSE;
		else
			((NA_Alignment*)DataSet)->element[j].subselected = FALSE;
	}
*/

    return(select_mode);
}

