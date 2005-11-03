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

/*
Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.
*       Global comments window
*/

Textsw comments_tsw;
NA_Sequence *this_elem;

	extern char FileName[];


Open(mnu,mnuitm)
Menu mnu;
Menu_item mnuitm;
{
	extern Frame frame,pframe;
	extern Panel popup;
	/*

	if(pframe)
		xv_destroy_safe(pframe);

        pframe = xv_create(frame,FRAME_CMD,
		FRAME_CMD_PUSHPIN_IN,TRUE,
                FRAME_DONE_PROC,FrameDone,
		FRAME_SHOW_RESIZE_CORNER,FALSE,
                WIN_DESIRED_HEIGHT,100,
                WIN_DESIRED_WIDTH,300,
		FRAME_LABEL,"Open...",
                XV_X,300,
                XV_Y,150,
                WIN_SHOW,FALSE,
                0);

	popup = xv_find(pframe,PANEL,
       		PANEL_LAYOUT,PANEL_HORIZONTAL,
                0);
	popup = xv_get(pframe,FRAME_CMD_PANEL);
	popup = xv_get(pframe,FRAME_CMD_PANEL);

	(void)xv_create (popup,PANEL_BUTTON,
        	PANEL_LABEL_STRING,"OK",
		PANEL_NOTIFY_PROC,OpenFileName,
        	0);

	(void)xv_create (popup,PANEL_BUTTON,
        	PANEL_LABEL_STRING,"Cancel",
        	PANEL_NOTIFY_PROC,DONT,
        	0);

	(void)xv_set(popup,
	        PANEL_LAYOUT,PANEL_VERTICAL,
		0);

	(void)xv_create(popup,PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH,20,
		PANEL_LABEL_STRING,"File name?",
		PANEL_NOTIFY_LEVEL,PANEL_ALL,
		PANEL_VALUE,FileName,
		PANEL_NOTIFY_PROC,SetFilename,
		0);

	window_fit(popup);
	window_fit(pframe);

	(void)xv_set(pframe,XV_SHOW,TRUE,0);
*/
	(void)load_file(frame,300,150,NULL);
	return(XV_OK);
}


SaveAs(mnu,mnuitm)
Menu mnu;
Menu_item mnuitm;
{
	extern Frame frame,pframe;
	extern Panel popup;
	extern char FileName[];
	extern NA_Alignment *DataSet;
	NA_Alignment *aln;

	if(pframe)
		xv_destroy_safe(pframe);

	if(DataSet == NULL)
		return(XV_OK);

	aln = (NA_Alignment*)DataSet;

	pframe = xv_create(frame,FRAME_CMD,
		FRAME_CMD_PUSHPIN_IN,TRUE,
	    FRAME_DONE_PROC,FrameDone,
	    FRAME_SHOW_RESIZE_CORNER,FALSE,
	    WIN_DESIRED_HEIGHT,100,
	    WIN_DESIRED_WIDTH,300,
	    FRAME_LABEL,"Save alignment as...",
	    XV_X,300,
	    XV_Y,150,
	    WIN_SHOW,FALSE,
	    0);

/*
	popup = xv_find(pframe,PANEL,
	    PANEL_LAYOUT,PANEL_HORIZONTAL,
	    0);
*/

	popup = xv_get(pframe,FRAME_CMD_PANEL);
	(void)xv_create (popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"OK",
	    PANEL_NOTIFY_PROC,SaveAsFileName,
	    0);

	(void)xv_create (popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"Cancel",
	    PANEL_NOTIFY_PROC,DONT,
	    0);

	(void)xv_set(popup,
	    PANEL_LAYOUT,PANEL_VERTICAL,
	    0);

	(void)xv_create(popup,PANEL_CHOICE,
	    PANEL_LAYOUT,PANEL_HORIZONTAL,
	    PANEL_NOTIFY_PROC,SaveFormat,
	    PANEL_LABEL_STRING,"Format:",
	    PANEL_CHOICE_STRING,0,"Genbank",
	    PANEL_CHOICE_STRING,1,"Flat file",
	    PANEL_CHOICE_STRING,2,"GDE",
	    PANEL_VALUE,aln->format == GENBANK?0:
	    (aln->format == GDE)?2:1,
	    0);

	(void)xv_create(popup,PANEL_TEXT,
	    PANEL_VALUE_DISPLAY_LENGTH,20,
	    PANEL_LABEL_STRING,"File name?",
	    PANEL_NOTIFY_PROC,SetFilename,
	    PANEL_NOTIFY_LEVEL,PANEL_ALL,
	    PANEL_VALUE,FileName,
	    0);

	window_fit(popup);
	window_fit(pframe);

	(void)xv_set(pframe,XV_SHOW,TRUE,0);

	return(XV_OK);
}


SaveFormat(item,event)
Panel_item item;
Event *event;
{
	extern NA_Alignment *DataSet;
	NA_Alignment *aln;
	int format;

	if(DataSet == NULL)
		return(XV_OK);

	format = xv_get(item,PANEL_VALUE);
	DataSet->format = (format == 0)?
	    GENBANK:(format == 1)? NA_FLAT:GDE;
	return(XV_OK);
}


SaveAsFileName(item,event)
Panel_item item;
Event *event;
{
	extern NA_Alignment *DataSet;

	char *file;
	int j;

	file = FileName;

	DONT(0,0);
	if(DataSet == NULL)
		return(XV_OK);

	switch( ((NA_Alignment*)DataSet)->format )
	{
	case GENBANK:
		WriteGen(DataSet,file,ALL,FALSE);
		break;
	case NA_FLAT:
		WriteNA_Flat(DataSet,file,ALL,FALSE);
		break;
	case GDE:
		WriteGDE(DataSet,file,ALL,FALSE);
		break;
	default:
		fprintf(stderr,"Unknown file type for write\n");
		break;
	}
	return(XV_OK);
}

	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet;


act_on_it_lf(filename,data)
char filename[];
Xv_opaque data;
{
	Xv_window view;
	Scrollbar hscroll,vscroll;
	int j;

	if(filename == NULL)
		return(XV_OK);

	LoadData(filename);

	for(j=0;j<xv_get(EditCan,OPENWIN_NVIEWS);j++)
	{
		view = (Xv_window)xv_get(EditCan,OPENWIN_NTH_VIEW,j,0);

		hscroll = (Scrollbar)xv_get(EditCan,
		    OPENWIN_HORIZONTAL_SCROLLBAR,view);
		vscroll = (Scrollbar)xv_get(EditCan,
		    OPENWIN_VERTICAL_SCROLLBAR,view);

		if(hscroll)
			xv_destroy_safe(hscroll);
		if(vscroll)
			xv_destroy_safe(vscroll);
	}

	if(EditCan)
		xv_destroy_safe(EditCan);

	if(EditNameCan)
		xv_destroy_safe(EditNameCan);

	BasicDisplay(DataSet);

	if(DataSet != NULL)
		((NA_Alignment*)DataSet)->na_ddata = (char*)SetNADData
		    ((NA_Alignment*)DataSet,EditCan,EditNameCan);
	return(XV_OK);
}

act_on_it_sf(){
}

OpenFileName(item,event)
Panel_item item;
Event *event;
{
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet;
	Xv_window view;

	char *file;
	int j;
	NA_DisplayData *ddata;
	Scrollbar hscroll,vscroll;

	/*
*	major kluge in progress, if event is NULL, then item is
*	really a pointer to the name of a file to be read in
*/

	if(event != NULL)
		file = FileName;
	else
		file = (char*)item;

	DONT(0,0);
	LoadData(file);

	for(j=0;j<xv_get(EditCan,OPENWIN_NVIEWS);j++)
	{
		view = (Xv_window)xv_get(EditCan,OPENWIN_NTH_VIEW,j,0);

		hscroll = (Scrollbar)xv_get(EditCan,
		    OPENWIN_HORIZONTAL_SCROLLBAR,view);
		vscroll = (Scrollbar)xv_get(EditCan,
		    OPENWIN_VERTICAL_SCROLLBAR,view);

		xv_destroy(hscroll);
		xv_destroy(vscroll);
	}

	xv_destroy_safe(EditCan);
	xv_destroy_safe(EditNameCan);

	BasicDisplay(DataSet);

	if(DataSet != NULL)
		((NA_Alignment*)DataSet)->na_ddata = (char*)SetNADData
		    ((NA_Alignment*)DataSet,EditCan,EditNameCan);
	return(XV_OK);
}


/*
Copyright (c) 1989, University of Illinois board of trustees.  All rights
reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.
*/

	extern Frame pframe,frame;
	extern Panel popup;
	extern NA_Alignment *DataSet;
	extern EditMode,EditDir,DisplayAttr;
	extern int SCALE;

ChangeDisplay(item,event)
Panel_item item;
Event *event;
{
	NA_DisplayData *na_dd;
	int color,font_size;
	GC gc;
	Display *dpy;
	Xv_font font;

	if(DataSet == NULL)
	{
		Warning("Must load a dataset first");
		return(XV_OK);
	}

	na_dd = (NA_DisplayData*)(((NA_Alignment*)DataSet)->na_ddata);
	if(na_dd == NULL)
	{
		Warning("Must load a dataset first");
		return(XV_OK);
	}
	switch(na_dd->color_type)
	{
	case COLOR_MONO:
		color = 0;
		break;
	case COLOR_LOOKUP:
		color = 1;
		break;
	case COLOR_ALN_MASK:
		color = 2;
		break;
	case COLOR_SEQ_MASK:
		color = 3;
		break;
	case COLOR_STRAND:
		color = 4;
		break;
	default:
		break;
	}

	xv_destroy_safe(pframe);

	pframe = xv_create(frame,FRAME_CMD,
		FRAME_CMD_PUSHPIN_IN,TRUE,
	    FRAME_DONE_PROC,FrameDone,
	    FRAME_SHOW_RESIZE_CORNER,FALSE,
	    FRAME_LABEL,"Properties",
	    XV_X,300,
	    XV_Y,150,
	    WIN_SHOW,FALSE,
	    0);

/*
	popup = xv_find(pframe,PANEL,
	    PANEL_LAYOUT,PANEL_HORIZONTAL,
	    0);
*/

	popup = xv_get(pframe,FRAME_CMD_PANEL);
	(void)xv_create (popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"OK",
	    PANEL_NOTIFY_PROC,ChDisplayDone,
	    0);

	(void)xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL,0);

	(void)xv_create(popup,PANEL_CHOICE_STACK,
	    PANEL_LAYOUT,PANEL_VERTICAL,
	    PANEL_LABEL_STRING,"Color type",
	    PANEL_NOTIFY_PROC,ChColor,
	    PANEL_CHOICE_STRING,0,"Monochrome",
	    PANEL_CHOICE_STRING,1,"Character->color",
	    PANEL_CHOICE_STRING,2,"Alignment color mask",
	    PANEL_CHOICE_STRING,3,"Sequence color mask",
	    PANEL_CHOICE_STRING,4,"Strand->color",
	    PANEL_CHOOSE_NONE,FALSE,
	    PANEL_VALUE,color,
	    0);

	(void)xv_set(popup,PANEL_LAYOUT,PANEL_HORIZONTAL,0);
	dpy = (Display *)xv_get(EditCan, XV_DISPLAY);
	gc = DefaultGC(dpy,DefaultScreen(dpy));

	font = xv_get(frame,XV_FONT);
	switch(xv_get(font,FONT_SCALE))
	{
	case WIN_SCALE_EXTRALARGE:
		font_size = 0;
		break;
	case WIN_SCALE_LARGE:
		font_size = 1;
		break;
	case WIN_SCALE_MEDIUM:
		font_size = 2;
		break;
	case WIN_SCALE_SMALL:
		font_size = 3;
		break;
	default:
		font_size = 2;
		break;
	}

	(void)xv_create(popup,PANEL_CHOICE_STACK,
	    PANEL_LAYOUT,PANEL_VERTICAL,
	    PANEL_NOTIFY_PROC,ChFontSize,
	    PANEL_LABEL_STRING,"Font Size",
	    PANEL_CHOICE_STRING,0,"Extra large",
	    PANEL_CHOICE_STRING,1,"Large",
	    PANEL_CHOICE_STRING,2,"Medium",
	    PANEL_CHOICE_STRING,3,"Small",
	    PANEL_CHOOSE_NONE,FALSE,
	    PANEL_VALUE,font_size,
	    0);

	(void)xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL,0);

	(void)xv_create(popup,PANEL_CHOICE,
	    PANEL_LAYOUT,PANEL_HORIZONTAL,
	    PANEL_NOTIFY_PROC,ChEditMode,
	    PANEL_LABEL_STRING,"Editing mode",
	    PANEL_CHOICE_STRING,0,"Insert",
	    PANEL_CHOICE_STRING,1,"Check",
	    PANEL_VALUE,EditMode,
	    0);

	(void)xv_create(popup,PANEL_CHECK_BOX,
	    PANEL_LAYOUT,PANEL_VERTICAL,
	    PANEL_CHOICE_STRINGS,
	    "Inverted","Lock vertical scroll","Key clicks","Message panel",
	    0,
	    PANEL_NOTIFY_PROC,ChDisAttr,
	    PANEL_VALUE,DisplayAttr,
	    0);

	(void)xv_set(popup,PANEL_LAYOUT,PANEL_HORIZONTAL,0);

	(void)xv_create(popup,PANEL_CHOICE,
	    PANEL_LAYOUT,PANEL_VERTICAL,
	    PANEL_NOTIFY_PROC,ChEditDir,
	    PANEL_LABEL_STRING,"Insertion",
	    PANEL_CHOICE_STRING,0,"Right of cursor",
	    PANEL_CHOICE_STRING,1,"Left of cursor",
	    PANEL_VALUE,EditDir,
	    0);

	(void)xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL,0);
	(void)xv_create(popup,PANEL_SLIDER,
	    PANEL_LABEL_STRING,"Scale:",
	    PANEL_MIN_VALUE,1,
	    PANEL_MAX_VALUE,20,
	    PANEL_VALUE,SCALE,
	    PANEL_NOTIFY_PROC,SetScale,
	    0);


	window_fit(popup);
	window_fit(pframe);

	(void)xv_set(pframe,XV_SHOW,TRUE,0);

	return(XV_OK);
}

SetScale(item,event)
Panel_item item;
Event *event;
{
	extern int SCALE;
	SCALE = xv_get(item,PANEL_VALUE);
	return (XV_OK);
}


ChColor(item,event)
Panel_item item;
Event *event;
{
	int i,j;
	NA_DisplayData *ddata;

	if(DataSet == NULL)
		return(XV_OK);
	ddata = (NA_DisplayData*)((NA_Alignment*)DataSet) ->na_ddata;

	switch(xv_get(item,PANEL_VALUE))
	{
	case 0:
		ddata->color_type = COLOR_MONO;
		break;
	case 1:
		ddata->color_type = COLOR_LOOKUP;
		break;
	case 2:
		ddata->color_type = COLOR_ALN_MASK;
		break;
	case 3:
		ddata->color_type = COLOR_SEQ_MASK;
		break;
	case 4:
		ddata->color_type = COLOR_STRAND;
		break;
	default:
		break;
	}
	return(XV_OK);
}


ChFontSize(item,event)
Panel_item item;
Event *event;
{
	int i,j,fnt_style;
	GC gc;
	Display *dpy;
	XGCValues gcv;
	Xv_font font;
	font = xv_get(frame,XV_FONT);
	fnt_style = (int)xv_get(font,FONT_STYLE);

	dpy = (Display *)xv_get(EditCan, XV_DISPLAY);
	gc = DefaultGC(dpy,DefaultScreen(dpy));

	switch(xv_get(item,PANEL_VALUE))
	{
	case 0:
		font = (Xv_font)xv_find(frame,FONT,
		    FONT_FAMILY,FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		    FONT_STYLE,fnt_style,
		    FONT_SCALE,WIN_SCALE_EXTRALARGE,
		    0);
		break;
	case 1:
		font = (Xv_font)xv_find(frame,FONT,
		    FONT_FAMILY,FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		    FONT_STYLE,fnt_style,
		    FONT_SCALE,WIN_SCALE_LARGE,
		    0);
		break;
	case 2:
		font = (Xv_font)xv_find(frame,FONT,
		    FONT_FAMILY,FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		    FONT_STYLE,fnt_style,
		    FONT_SCALE,WIN_SCALE_MEDIUM,
		    0);
		break;
	case 3:
		font = (Xv_font)xv_find(frame,FONT,
		    FONT_FAMILY,FONT_FAMILY_DEFAULT_FIXEDWIDTH,
		    FONT_STYLE,fnt_style,
		    FONT_SCALE,WIN_SCALE_SMALL,
		    0);
		break;
	default:
		break;
	}
	(void)xv_set(frame,XV_FONT,font,0);
	gcv.font = (Font)xv_get(font,XV_XID);

	if(gcv.font != NULL)
		XChangeGC(dpy,gc,GCFont,&gcv);

	(void)SetNADData(DataSet,EditCan,EditNameCan);
	return(XV_OK);
}


ChDisplayDone()
{
	extern Frame frame;

	DONT(0,0);

	RepaintAll(FALSE);
	return(XV_OK);
}

SetProtection(item,event)
Panel_item item;
Event *event;
{
	int j;
	unsigned int current_prot;
	NA_Alignment *aln;
	int mismatch_prot = FALSE,num_selected = 0;

	extern Frame pframe,frame;
	extern Panel popup;
	extern NA_Alignment *DataSet;


	if(DataSet == NULL)
		return(XV_OK);

	aln = (NA_Alignment*)DataSet;
	if(aln->numelements == 0)
		return(XV_OK);

	for(j=0;j<aln->numelements;j++)
		if(aln->element[j].selected)
		{
			current_prot = aln->element[j].protect;
			num_selected++;
		}

	for(j=0;j<aln->numelements;j++)
		if(aln->element[j].selected && aln->element[j].protect
		    != current_prot)
		{
			current_prot=0;
			mismatch_prot = TRUE;
		}

	xv_destroy_safe(pframe);
	pframe = xv_create(frame,FRAME_CMD,
		FRAME_CMD_PUSHPIN_IN,TRUE,
	    FRAME_DONE_PROC,FrameDone,
	    FRAME_SHOW_RESIZE_CORNER,FALSE,
	    WIN_DESIRED_HEIGHT,100,
	    WIN_DESIRED_WIDTH,300,
	    XV_X,300,
	    XV_Y,150,
	    XV_SHOW,FALSE,
	    0);


/*
	popup = xv_find(pframe,PANEL,
	    PANEL_LAYOUT,PANEL_VERTICAL,
	    0);
*/
	popup = xv_get(pframe,FRAME_CMD_PANEL);

	(void)xv_create(popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"Done",
	    PANEL_NOTIFY_PROC,DONT,
	    0);
	if(mismatch_prot)
		(void)xv_create(popup,PANEL_MESSAGE,
		    PANEL_LABEL_STRING,
		    "Warning: Current protections differ",NULL);
	if(num_selected == 0)
		(void)xv_create(popup,PANEL_MESSAGE,
		    PANEL_LABEL_STRING,"Warning: No sequences selected",
		    NULL);


	(void)xv_create(popup,PANEL_CHECK_BOX,
	    PANEL_LAYOUT,PANEL_VERTICAL,
	    PANEL_CHOOSE_ONE,FALSE,
	    PANEL_LABEL_STRING,"Allowed modifications:",
	    PANEL_CHOICE_STRINGS,
	    "unambiguous characters",
	    "ambiguous characters",
	    "alignment gaps",
	    "translations",
	    NULL,
	    PANEL_NOTIFY_PROC,Prot,
	    PANEL_VALUE,current_prot,
	    0);
	window_fit(popup);
	window_fit(pframe);
	(void)xv_set(pframe,XV_SHOW,TRUE,
	    FRAME_LABEL,"Set Protections",0);

	return(XV_OK);
}


Prot(item,event)
Panel_item item;
Event *event;
{
	int j;
	unsigned int current_prot;
	NA_Alignment *aln;
	extern NA_Alignment *DataSet;


	if(DataSet == NULL)
		return(XV_OK);

	aln = (NA_Alignment*)DataSet;
	if(aln->numelements == 0)
		return(XV_OK);

	current_prot = xv_get(item,PANEL_VALUE);
	for(j=0;j<aln->numelements;j++)
		if(aln->element[j].selected)
			aln->element[j].protect = current_prot;

	return(XV_OK);
}


SelectAll(item,event)
Panel_item item;
Event *event;
{
	int i;
	extern NA_Alignment *DataSet;
	Display *dpy;
	NA_Alignment *aln = (NA_Alignment*)DataSet;

	if(DataSet == NULL)
		return(XV_OK);

	for(i=0;i<aln->numelements;i++)
		aln->element[i].selected = TRUE;

	dpy = (Display*)xv_get(EditNameCan, XV_DISPLAY);
	DrawNANames(dpy,xv_get(canvas_paint_window(EditNameCan),XV_XID));
	return(XV_OK);
}

SelectBy(item,event)
Panel_item item;
Event *event;
{
	int i;
	extern NA_Alignment *DataSet;
	Display *dpy;
	NA_Alignment *aln = (NA_Alignment*)DataSet;
	xv_destroy_safe(pframe);

	pframe = xv_create(frame,FRAME_CMD,
		FRAME_CMD_PUSHPIN_IN,TRUE,
	    FRAME_DONE_PROC,FrameDone,
	    FRAME_SHOW_RESIZE_CORNER,FALSE,
	    FRAME_LABEL,"Select sequences by name",
	    WIN_DESIRED_HEIGHT,100,
	    WIN_DESIRED_WIDTH,300,
	    XV_X,300,
	    XV_Y,150,
	    WIN_SHOW,FALSE,
	    0);

/*
	popup = xv_find(pframe,PANEL,
	    PANEL_LAYOUT,PANEL_HORIZONTAL,
	    0);
*/
	popup = xv_get(pframe,FRAME_CMD_PANEL);
	(void)xv_create (popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"Done",
	    PANEL_NOTIFY_PROC,DONT,
	    0);

	(void)xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL,
	    0);

	(void)xv_create(popup,PANEL_TEXT,
	    PANEL_VALUE_DISPLAY_LENGTH,20,
	    PANEL_LABEL_STRING,"Search for?",
	    PANEL_NOTIFY_PROC,SelectByName,
	    0);

	window_fit(popup);
	window_fit(pframe);

	(void)xv_set(pframe,XV_SHOW,TRUE,0);

	return(XV_OK);
}

SelectByName(item,event)
Panel_item item;
Event *event;
{
	extern NA_Alignment *DataSet;
	extern Canvas EditCan,EditNameCan;
	char search[80];
	Display *dpy;
	Xv_window view;
	int i,lastselected;

	if(DataSet == NULL)
		return(XV_OK);

	strncpy(search,(char*)(xv_get(item,PANEL_VALUE)),79);

	for(i=0;i<DataSet->numelements;i++)
		if(Find(DataSet->element[i].short_name,search))
		{
			DataSet->element[i].selected = TRUE;
			lastselected = i;
		}

	dpy = (Display*)xv_get(EditNameCan, XV_DISPLAY);
	DrawNANames(dpy,xv_get(canvas_paint_window(EditNameCan),XV_XID));
	view = (Xv_window)xv_get(EditCan,OPENWIN_NTH_VIEW,0);

	OPENWIN_EACH_VIEW(EditCan,view)
		JumpTo(view, 0,lastselected);
	OPENWIN_END_EACH;

	(void)xv_set(item,PANEL_VALUE,"",0);

	return(XV_OK);

}


Group(item,event)
Panel_item item;
Event *event;
{
	int j,old_groups = FALSE,result;
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet;
	Display *dpy;
	NA_Alignment *aln;
	NA_Sequence *temp = NULL,*element;

	if(DataSet == NULL)
		return(XV_OK);
	aln = (NA_Alignment*)DataSet;
	if(aln == NULL)
		return(XV_OK);
	element = aln->element;

	for(j=0;j<aln->numelements;j++)
		if((element[j].groupid !=0 ) && element[j].selected)
			old_groups = TRUE;

	if(old_groups)
	{
		result = notice_prompt(frame,NULL,
		    NOTICE_MESSAGE_STRINGS,
		    "Groups already exist.  Do you wish to",
		    "Merge these groups, create a new group,",
		    "or cancel?",
		    NULL,
		    NOTICE_BUTTON,"Merge groups",1,
		    NOTICE_BUTTON,"Create new group",2,
		    NOTICE_BUTTON,"Cancel",3,
		    0);
		switch(result)
		{
		case 3:
			break;
		case 2:
			for(j=0;j<aln->numelements;j++)
				if(element[j].selected)
					RemoveFromGroup(&(element[j]));
			for(j=0;j<aln->numelements;j++)
			{
				if(element[j].selected)
				{
					element[j].groupid =
					    aln->numgroups+1;
					element[j].groupb = temp;
					if(temp != NULL)
						temp->groupf =
						    &(element[j]);
					temp = &(element[j]);
				}
			}
			if(temp != NULL)
				temp->groupf = NULL;
			if(temp != NULL)
				if(temp->groupb !=NULL)
				{
					aln->numgroups++;
					AdjustGroups(aln);
					DrawNANames(xv_get(EditNameCan,
					    XV_DISPLAY),xv_get
					    (canvas_paint_window(
					    EditNameCan),XV_XID));
				}
			break;
		case 1:
			temp = NULL;
			for(j=0;j<aln->numelements;j++)
			{
				if(element[j].selected)
				{
					if(temp != NULL)
						MergeGroups(temp,&(element[j]));
					temp = &(element[j]);
				}
			}
			AdjustGroups(aln);
			DrawNANames(xv_get(EditNameCan,XV_DISPLAY),
			    xv_get(canvas_paint_window(EditNameCan),
			    XV_XID));
			break;
		}
	}
	else
	{
		temp = NULL;
		for(j=0;j<aln->numelements;j++)
		{
			if(element[j].selected)
			{
				element[j].groupid = aln->numgroups+1;
				element[j].groupb = temp;
				if(temp != NULL)
					temp->groupf = &(element[j]);
				temp = &(element[j]);
			}
		}
		if(temp != NULL)
		{
			temp->groupf = NULL;
			if(temp->groupb !=NULL)
			{
				aln->numgroups++;
				DrawNANames(xv_get(EditNameCan, XV_DISPLAY),
				    xv_get(canvas_paint_window(EditNameCan),
				    XV_XID));
			}
		}
	}
	return(XV_OK);
}

RemoveFromGroup(element)
NA_Sequence *element;
{
	if(element == NULL)
		return(XV_OK);

	if(element->groupb)
		((NA_Sequence*)(element->groupb))->groupf = element->groupf;

	if(element->groupf)
		((NA_Sequence*)(element->groupf))->groupb = element->groupb;

	element->groupf = NULL;
	element->groupb = NULL;
	element->groupid = 0;

	return(XV_OK);
}


AdjustGroups(aln)
NA_Alignment *aln;
{
	int i,j,c,done=FALSE;

#ifdef HGL
	return;
#else
	for(c=0;c<200 && !done;c++)
	{
		for(j=1;j<=aln->numgroups;j++)
		{
			done = FALSE;
			for(i=0;i<aln->numelements;i++)
			{
				if(aln->element[i].groupid == j)
				{
					if(aln->element[i].groupf!=NULL ||
					    aln->element[i].groupb!=NULL)
						done = TRUE;
					else
						aln->element[i].groupid = 0;
				}
			}
			if(done == FALSE)
			{
				for(i=0;i<aln->numelements;i++)
					if(aln->element[i].groupid ==
					    aln->numgroups)
						aln->element[i].groupid = j;
				aln->numgroups--;
			}
		}
		if(aln->numgroups == 0)
			done = TRUE;
	}
	return;
#endif
}

Ungroup(item,event)
Panel_item item;
Event *event;
{
	int j;
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet;
	Display *dpy = (Display *)xv_get(EditNameCan, XV_DISPLAY);
	NA_Alignment *aln;
	NA_Sequence *temp = NULL,*element;

	if(DataSet == NULL)
		return(XV_OK);
	aln = (NA_Alignment*)DataSet;
	if(aln == NULL)
		return(XV_OK);
	element = aln->element;

	for(j=0;j<aln->numelements;j++)
		if(element[j].selected && element[j].groupid != 0)
			RemoveFromGroup(&(element[j]));
	AdjustGroups(aln);
	DrawNANames(dpy,xv_get(canvas_paint_window(EditNameCan),XV_XID));
	return(XV_OK);
}


MergeGroups(el1,el2)
NA_Sequence *el1,*el2;
{
	int i,j,newid;
	NA_Sequence *last,*first,*temp;
	newid = MAX(el1->groupid,el2->groupid);
	if( el1->groupid == el2->groupid && el1->groupid != 0) return;
	last = el1;

	for(;last->groupf != NULL;) last = last->groupf;
	first = el1;

	for(;first->groupb != NULL;) first = first->groupb;
	for(;el2->groupf != NULL;) el2 = el2->groupf;

	el2->groupf = first;
	first->groupb = el2;

	el2->groupid = newid;
	for(;last != NULL; last=last->groupb) last->groupid = newid;
	return;
}


New()
{
	extern NA_Alignment *DataSet;
}


ModAttr(mnu,mnuitm)
Menu mnu;
Menu_item mnuitm;
{
	extern NA_Alignment *DataSet;
	extern Frame frame,pframe;
	extern Panel popup;
	extern BlockInput;
	extern Notify_value;
	int cur_type = 0,direction = 0,j,sel_count;
	extern Textsw comments_tsw;
	Textsw baggage_tsw;
	char temp[80];
	NA_Alignment *aln = (NA_Alignment*)DataSet;

	if(DataSet == NULL)
		return(XV_OK);

	if(aln->na_ddata == NULL)
		return(XV_OK);

	for(j=0,sel_count = 0;j<aln->numelements;j++)
		if(aln->element[j].selected)
		{
			this_elem = &(aln->element[j]);
			sel_count++;
		}

	if(sel_count == 0)
	{
		Warning("Must select sequence(s) first");
		return(XV_OK);
	}
	if(this_elem->elementtype == RNA) cur_type = 0;
	if(this_elem->elementtype == DNA) cur_type = 1;
	if(this_elem->elementtype == TEXT) cur_type = 2;
	if(this_elem->elementtype == MASK) cur_type = 3;
	if(this_elem->elementtype == PROTEIN) cur_type = 4;

	xv_destroy_safe(pframe);
	pframe = xv_create(frame,FRAME_CMD,
		FRAME_CMD_PUSHPIN_IN,TRUE,
	    FRAME_DONE_PROC,FrameDone,
	    FRAME_SHOW_RESIZE_CORNER,FALSE,
	    FRAME_LABEL,"Sequence Information",
	    WIN_DESIRED_HEIGHT,100,
	    WIN_DESIRED_WIDTH,300,
	    XV_X,300,
	    XV_Y,150,
	    WIN_SHOW,FALSE,
	    0);

/*
	popup = xv_find(pframe,PANEL,
	    PANEL_LAYOUT,PANEL_HORIZONTAL,
	    0);
*/
	popup = xv_get(pframe,FRAME_CMD_PANEL);

	(void)xv_create (popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"OK",
	    PANEL_NOTIFY_PROC,ModAttrDone,
	    0);

	(void)xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL, 0);

	if(sel_count == 1)
		(void)xv_create(popup,PANEL_TEXT,
		    PANEL_VALUE_DISPLAY_LENGTH,20,
		    PANEL_LABEL_STRING,"Short name",
		    PANEL_VALUE,this_elem->short_name,
		    PANEL_NOTIFY_PROC,ChAttr,
		    PANEL_NOTIFY_LEVEL,PANEL_ALL,
		    PANEL_ITEM_X_GAP,5,
		    PANEL_ITEM_Y_GAP,3,
		    0);

	(void)xv_create(popup,PANEL_CHOICE_STACK,
	    PANEL_NOTIFY_PROC,ChAttrType,
	    PANEL_LABEL_STRING,"Type:",
	    PANEL_CHOICE_STRINGS,
	    "RNA",
	    "DNA",
	    "TEXT",
	    "MASK",
	    "PROTEIN",
	    0,
	    PANEL_VALUE,cur_type,
	    PANEL_ITEM_X_GAP,5,
	    PANEL_ITEM_Y_GAP,3,
	    0);

	if(sel_count == 1)
		(void)xv_set(popup,PANEL_LAYOUT,PANEL_HORIZONTAL, 0);
	else
		(void)xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL, 0);


	(void)xv_create(popup,PANEL_CHOICE_STACK,
	    PANEL_NOTIFY_PROC,ChAttrType,
	    PANEL_LABEL_STRING,"Strand",
	    PANEL_CHOICE_STRINGS,
	    "Primary",
	    "Secondary",
	    "Undefined",
	    0,
	    PANEL_VALUE,(this_elem->attr & IS_SECONDARY)?1:
	    (this_elem->attr & IS_PRIMARY)?0:2,
	    PANEL_ITEM_X_GAP,5,
	    PANEL_ITEM_Y_GAP,3,
	    0);

	if(sel_count == 1)
		(void)xv_set(popup,PANEL_LAYOUT,PANEL_HORIZONTAL, 0);
	else
		(void)xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL, 0);


	(void)xv_create(popup,PANEL_CHOICE_STACK,
	    PANEL_LAYOUT,PANEL_HORIZONTAL,
	    PANEL_NOTIFY_PROC,ChAttrType,
	    PANEL_LABEL_STRING,"Direction",
	    PANEL_CHOICE_STRINGS,
	    "5' to 3'",
	    "3' to 5'",
	    "Undefined",
	    0,
	    PANEL_VALUE,(this_elem->attr & IS_3_TO_5)?1:
	    (this_elem->attr & IS_5_TO_3)?0:2,
	    PANEL_ITEM_X_GAP,5,
	    PANEL_ITEM_Y_GAP,3,
	    0);


	(void)xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL, 0);

	if(sel_count == 1)
		(void)xv_create(popup,PANEL_TEXT,
		    PANEL_VALUE_DISPLAY_LENGTH,40,
		    PANEL_LABEL_STRING,"Full name ",
		    PANEL_VALUE,this_elem->seq_name,
		    PANEL_NOTIFY_PROC,ChAttr,
		    PANEL_NOTIFY_LEVEL,PANEL_ALL,
		    PANEL_ITEM_X_GAP,5,
		    PANEL_ITEM_Y_GAP,3,
		    0);

	if(sel_count == 1)
		(void)xv_create(popup,PANEL_TEXT,
		    PANEL_VALUE_DISPLAY_LENGTH,40,
		    PANEL_LABEL_STRING,"ID Number   ",
		    PANEL_VALUE,this_elem->id,
		    PANEL_NOTIFY_PROC,ChAttr,
		    PANEL_NOTIFY_LEVEL,PANEL_ALL,
		    PANEL_ITEM_X_GAP,5,
		    PANEL_ITEM_Y_GAP,3,
		    0);

	if(sel_count == 1)
		(void)xv_create(popup,PANEL_TEXT,
		    PANEL_VALUE_DISPLAY_LENGTH,40,
		    PANEL_LABEL_STRING,"Description",
		    PANEL_VALUE,this_elem->description,
		    PANEL_NOTIFY_PROC,ChAttr,
		    PANEL_NOTIFY_LEVEL,PANEL_ALL,
		    PANEL_ITEM_X_GAP,5,
		    PANEL_ITEM_Y_GAP,3,
		    0);

#ifdef HGL
	if(sel_count == 1)
		(void)xv_create(popup,PANEL_TEXT,
		    PANEL_VALUE_DISPLAY_LENGTH,40,
		    PANEL_LABEL_STRING,"Membrane  ",
		    PANEL_VALUE,this_elem->membrane,
		    PANEL_NOTIFY_PROC,ChAttr,
		    PANEL_NOTIFY_LEVEL,PANEL_ALL,
		    PANEL_ITEM_X_GAP,5,
		    PANEL_ITEM_Y_GAP,3,
		    0);
#endif

	if(sel_count == 1)
		(void)xv_create(popup,PANEL_TEXT,
		    PANEL_VALUE_DISPLAY_LENGTH,40,
		    PANEL_LABEL_STRING,"Author    ",
		    PANEL_VALUE,this_elem->authority,
		    PANEL_NOTIFY_PROC,ChAttr,
		    PANEL_NOTIFY_LEVEL,PANEL_ALL,
		    PANEL_ITEM_X_GAP,5,
		    PANEL_ITEM_Y_GAP,3,
		    0);

#ifdef HGL
	if(sel_count == 1)
		(void)xv_create(popup,PANEL_TEXT,
		    PANEL_VALUE_DISPLAY_LENGTH,40,
		    PANEL_LABEL_STRING,"Barcode   ",
		    PANEL_VALUE,this_elem->barcode,
		    PANEL_NOTIFY_PROC,ChAttr,
		    PANEL_NOTIFY_LEVEL,PANEL_ALL,
		    PANEL_ITEM_X_GAP,5,
		    PANEL_ITEM_Y_GAP,3,
		    0);
#endif

	direction = OrigDir(this_elem);
	if(sel_count == 1)
	{
#ifdef HGL
		sprintf(temp,"Created on %d/%d/%d %d:%d:%d (%s %s) %s",
		    this_elem->t_stamp.origin.mm,
		    this_elem->t_stamp.origin.dd,
		    this_elem->t_stamp.origin.yy,
		    this_elem->t_stamp.origin.hr,
		    this_elem->t_stamp.origin.mn,
		    this_elem->t_stamp.origin.sc,
		    (this_elem->attr & IS_ORIG_PRIMARY)?"Primary":
		    (this_elem->attr & IS_ORIG_SECONDARY)?"Secondary":"Strand ?",
		    (direction == 1)?"-->":
		    (direction == -1)?"<--":"<-?->",
			this_elem->attr & IS_CIRCULAR? "Circular":"");
#else
		sprintf(temp,"Created on %d/%d/%d %d:%d:%d (%s) %s",
		    this_elem->t_stamp.origin.mm,
		    this_elem->t_stamp.origin.dd,
		    this_elem->t_stamp.origin.yy,
		    this_elem->t_stamp.origin.hr,
		    this_elem->t_stamp.origin.mn,
		    this_elem->t_stamp.origin.sc,
		    (direction == 1)?"-->":
		    (direction == -1)?"<--":"<-?->",
			this_elem->attr & IS_CIRCULAR? "Circular":"");
#endif

		    xv_create(popup,PANEL_MESSAGE,
		    PANEL_LABEL_STRING,temp,
		    PANEL_ITEM_X_GAP,5,
		    PANEL_ITEM_Y_GAP,3,
		    0);

	}
	(void)xv_set(popup,PANEL_LAYOUT,PANEL_HORIZONTAL, 0);

	if(sel_count == 1)
		(void)xv_create(popup,PANEL_MESSAGE,
		    PANEL_LABEL_STRING,
		    "   Comments:",
		    0);

	(void)xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL, 0);
	if(sel_count == 1)
		window_fit_height(popup);
	else
		window_fit(popup);

	if(sel_count == 1)
	{
		comments_tsw = xv_create(pframe,TEXTSW,
		    WIN_INHERIT_COLORS,TRUE,
		    WIN_BELOW,popup,
		    XV_X,0,
		    XV_HEIGHT,(this_elem->baggage)?90:180,
		    TEXTSW_CONTENTS,this_elem->comments?
		    this_elem->comments:"",
		    TEXTSW_READ_ONLY,FALSE,
		    0);

		window_fit(comments_tsw);
		if(this_elem->baggage)
		{
			baggage_tsw = xv_create(pframe,TEXTSW,
			    WIN_INHERIT_COLORS,TRUE,
			    WIN_BELOW,comments_tsw,
			    XV_X,0, XV_HEIGHT,90,
			    TEXTSW_CONTENTS,this_elem->baggage?
			    this_elem->baggage:"",
			    TEXTSW_READ_ONLY,TRUE,
			    0);
			window_fit(baggage_tsw);
		}
		window_fit(pframe);

		notify_interpose_destroy_func(comments_tsw,SaveComments);
	}
	window_fit(pframe);

	(void)xv_set(pframe,XV_SHOW,TRUE,0);
	BlockInput = TRUE;
	return(XV_OK);
}


Notify_value SaveComments(client,status)
Notify_client client;
Destroy_status status;
{
	int j,numselected = 0,lastselected = 0;
	extern NA_Alignment *DataSet;

	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].selected)
		{
			numselected ++;
			lastselected = j;
		}

	if(numselected == 1)
	{
		Cfree(DataSet->element[lastselected].comments);

		DataSet->element[lastselected].comments =
		    Calloc(xv_get(client,TEXTSW_LENGTH)+1,sizeof(char));

		DataSet->element[lastselected].comments_len =
		    strlen(DataSet->element[lastselected].comments);

		DataSet->element[lastselected].comments_maxlen =
		    xv_get(client,TEXTSW_LENGTH);

		(void)xv_get(client,TEXTSW_CONTENTS,0,
		    DataSet->element[lastselected].comments,
		    xv_get(client,TEXTSW_LENGTH));

		DataSet->element[lastselected].comments[
		xv_get(client,TEXTSW_LENGTH)] = '\0';

	}
	return(notify_next_destroy_func(client,status));
}



ChAttr(item,event)
Panel_item item;
Event *event;
{
	int j;
	NA_Sequence *this_element;
	NA_Alignment *aln;
	Panel_setting ps;

	if(DataSet == NULL)
		return;

	aln = (NA_Alignment*)DataSet;

	for(j=0;j<aln->numelements;j++)
		if(aln->element[j].selected)
			this_element = &(aln->element[j]);

	ps = panel_text_notify(item,event);

	if(Find(xv_get(item,PANEL_LABEL_STRING),"Short name"))
	{
		strncpy(this_element->short_name,xv_get(item,PANEL_VALUE),31);
		for(j=0;j<strlen(this_element->short_name);j++)
			if(this_element->short_name[j] == ' ')
				this_element->short_name[j] = '_';
	}

	else if(Find(xv_get(item,PANEL_LABEL_STRING),"Full name "))
	{
		strncpy(this_element->seq_name,xv_get(item,PANEL_VALUE),79);
	}

	else if(Find(xv_get(item,PANEL_LABEL_STRING),"Description"))
	{
		strncpy(this_element->description,xv_get(item,PANEL_VALUE),79);
	}

	else if(Find(xv_get(item,PANEL_LABEL_STRING),"Author    "))
	{
		strncpy(this_element->authority,xv_get(item,PANEL_VALUE),79);
	}

	else if(Find(xv_get(item,PANEL_LABEL_STRING),"ID Number   "))
	{
		strncpy(this_element->id,xv_get(item,PANEL_VALUE),79);
	}

	else if(Find(xv_get(item,PANEL_LABEL_STRING),"Membrane  "))
	{
		strncpy(this_element->membrane,xv_get(item,PANEL_VALUE),79);
	}

	else if(Find(xv_get(item,PANEL_LABEL_STRING),"Contig   "))
	{
		strncpy(this_element->contig,xv_get(item,PANEL_VALUE),79);
	}

	else if(Find(xv_get(item,PANEL_LABEL_STRING),"Barcode   "))
	{
		strncpy(this_element->barcode,xv_get(item,PANEL_VALUE),79);
	}


	return(ps);
}

ModAttrDone()
{
	FILE *file;
	extern Textsw comments_tsw;
	int j,maxlen = 20,numselected = 0;
	char c,*tempstring;

	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].selected)
		{
			this_elem = &(DataSet->element[j]);
			numselected++;
		}

	if(numselected == 1)
	{
		if(this_elem->comments)
			maxlen = strlen(this_elem->comments)+10;

		tempstring =(char*)Calloc(maxlen,sizeof(char));
		textsw_store_file(comments_tsw,"/tmp/gde_tmp",300,100);

		file = fopen("/tmp/gde_tmp","r");
		if(file == NULL)
		{
			Warning("Comments could not be saved");
			return XV_OK;
		}

		for(j=0;(c=getc(file))!=EOF;j++)
		{
			if(j==maxlen-1)
			{
				maxlen *=2;
				tempstring =(char *)Realloc(tempstring,maxlen);
			}
			tempstring[j] = c;
		}
		tempstring[j] = '\0';

		fclose(file);

		unlink("/tmp/gde_tmp");

		if(this_elem->comments)
			Cfree(this_elem->comments);

		this_elem->comments = tempstring;
		this_elem->comments_len = j;
		StripSpecial(this_elem->comments);

	}
	DONT(0,0);
	RepaintAll(TRUE);
}


ChEditMode(item,event)
Panel_item item;
Event *event;
{
	extern EditMode;
	EditMode = xv_get(item,PANEL_VALUE);

	return(XV_OK);
}


ChEditDir(item,event)
Panel_item item;
Event *event;
{
	extern EditDir;
	EditDir = xv_get(item,PANEL_VALUE);

	return(XV_OK);
}

ChDisAttr(item,event)
Panel_item item;
Event *event;
{
	extern DisplayAttr;
	extern Frame infoframe;

	DisplayAttr = xv_get(item,PANEL_VALUE);
	(void)SetNADData(DataSet,EditCan,EditNameCan);
	if(DisplayAttr & GDE_MESSAGE_PANEL)
		(void)xv_set(infoframe,XV_SHOW,TRUE,0);
	else
		(void)xv_set(infoframe,XV_SHOW,FALSE,0);
	return(XV_OK);
}


ChAttrType(item,event)
Panel_item item;
Event *event;
{
	int j,current_insert = 0,new_type,type;
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet;
	extern int Default_DNA_Trans[],Default_NA_RTrans[],Default_RNA_Trans[];
	extern int Default_NA_RTrans[],Default_PROColor_LKUP[],Default_NAColor_LKUP[];
	NA_Alignment *aln;
	NA_Sequence *temp = NULL,*element;

	if(DataSet == NULL)
		return;
	aln = (NA_Alignment*)DataSet;
	if(aln == NULL)
		return;
	element = aln->element;


	if(strcmp(xv_get(item,PANEL_LABEL_STRING),"Type:") == 0)
	{
		new_type = xv_get(item,PANEL_VALUE);
		type = (new_type == 0)?RNA:
		    (new_type == 1)?DNA:
		    (new_type == 2)?TEXT:
		    (new_type == 3)?MASK:
		    PROTEIN;


		for(j=0;j<aln->numelements;j++)
			if(element[j].selected)
			{
				if((element[j].protect & PROT_TRANSLATION) == 0)
				{
					Warning("Protect violation");
					(void)xv_set(item,PANEL_VALUE,element[j].elementtype
					    ,0);
					return(XV_ERROR);
				}
				if(element[j].elementtype == DNA ||
				    element[j].elementtype == RNA)
					switch(new_type)
					{
					case 1:
						element[j].tmatrix =Default_DNA_Trans;
						element[j].elementtype = type;
						element[j].col_lut=Default_NAColor_LKUP;
						break;
					case 0:
						element[j].tmatrix =Default_RNA_Trans;
						element[j].elementtype = type;
						element[j].col_lut=Default_NAColor_LKUP;
						break;
					case 4:
					case 2:
					case 3:
					default:
						/*
					(void)xv_set(item,PANEL_VALUE,old_type,0);
*/
						break;
					}
				else if (element[j].elementtype == PROTEIN)
					switch(new_type)
					{
					case 0:
					case 1:
						(void)xv_set(item,PANEL_VALUE,
						    element[j].elementtype ,0);
						break;
					case 4:
						(void)xv_set(item,PANEL_VALUE,
						    element[j].elementtype ,0);
						break;
					case 2:
					case 3:
						element[j].elementtype = type;
						element[j].col_lut=Default_PROColor_LKUP;
					default:
						break;
					}
				else if (element[j].elementtype == TEXT)
					switch(new_type)
					{
					case 0:
					case 1:
						(void)xv_set(item,PANEL_VALUE,
						    element[j].elementtype ,0);
						break;
					case 4:
						element[j].elementtype=type;
						element[j].col_lut=Default_PROColor_LKUP;
						break;
					case 3:
					case 2:
						element[j].elementtype=type;
						element[j].col_lut=NULL;
						break;
					default:
						break;
					}
				else if (element[j].elementtype == MASK)
					switch(new_type)
					{
					case 0:
					case 1:
						(void)xv_set(item,PANEL_VALUE,
						    element[j].elementtype ,0);
						break;
					case 4:
						element[j].elementtype=type;
						element[j].col_lut=Default_PROColor_LKUP;
						break;
					case 3:
					case 2:
						element[j].elementtype = type;
						element[j].col_lut = NULL;
						break;
					default:
						break;
					}

			}
	}
	else if(strcmp(xv_get(item,PANEL_LABEL_STRING),"Direction") == 0)
	{
		for(j=0;j<aln->numelements;j++)
			if(element[j].selected)
				switch(xv_get(item,PANEL_VALUE))
				{
				case 0:
					element[j].attr |= IS_5_TO_3;
					element[j].attr &= (0xffff - IS_3_TO_5);
					break;
				case 1:
					element[j].attr |= IS_3_TO_5;
					element[j].attr &= (0xffff-IS_5_TO_3);
					break;
				default:
					element[j].attr &= (0xffff-IS_5_TO_3);
					element[j].attr &= (0xffff-IS_3_TO_5);
					break;
				}
	}
	else if(strcmp(xv_get(item,PANEL_LABEL_STRING),"Strand") == 0)
	{
		for(j=0;j<aln->numelements;j++)
			if(element[j].selected)
				switch(xv_get(item,PANEL_VALUE))
				{
				case 0:
					element[j].attr |= IS_PRIMARY;
					element[j].attr &= (0xffff - IS_SECONDARY);
					break;
				case 1:
					element[j].attr |= IS_SECONDARY;
					element[j].attr &= (0xffff - IS_PRIMARY);
					break;
				default:
					element[j].attr &= (0xffff - IS_PRIMARY);
					element[j].attr &= (0xffff - IS_SECONDARY);
					break;
				}
	}
	return(XV_OK);
}


SwapElement(aln,e1,e2)
NA_Alignment *aln;
int e1,e2;
{
	/*
*	Warning,  The following code may not be compatable with other
*	C compilers.  The elements may need to be explicitly copied.
*/
	NA_Sequence temp;
	register i;

	for(i=0;i<aln->numelements;i++)
	{
		if(aln->element[i].groupf == &(aln->element[e1]))
			aln->element[i].groupf = &(aln->element[e2]);
		else if(aln->element[i].groupf == &(aln->element[e2]))
			aln->element[i].groupf = &(aln->element[e1]);
		if(aln->element[i].groupb == &(aln->element[e1]))
			aln->element[i].groupb = &(aln->element[e2]);
		else if(aln->element[i].groupb == &(aln->element[e2]))
			aln->element[i].groupb = &(aln->element[e1]);
	}


	temp = aln->element[e1];
	aln->element[e1] = aln->element[e2];
	aln->element[e2] = temp;
	return;
}

CompressAlign(item,event)
Panel_item item;
Event *event;
{
	int j,k,offset,pos = 0;
	int max_wid = -999999;
	int min_wid = 999999;
	int min_offset = 99999999;
	int any_selected = FALSE;
	int compress_all;

	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet;
	NA_Base *rev_seq;
	char *temp,*mask,*temp_c;
	Display *dpy;
	NA_Sequence *element;

	if(DataSet == NULL)
		return;

	element = DataSet->element;

	switch(notice_prompt(frame,NULL,
	    NOTICE_MESSAGE_STRINGS,"Removing extra gaps, Do you want to:",NULL,
	    NOTICE_BUTTON,"Preserve alignment",1,
	    NOTICE_BUTTON,"Remove all dashes",2,
	    NOTICE_BUTTON,"Cancel",3,0))
	{
	case 1:
		compress_all=FALSE;
		break;
	case 2:
		compress_all=TRUE;
		break;
	case 3:
	default:
		return(XV_OK);
	}

	any_selected = FALSE;
	for(j=0;j<DataSet->numelements;j++)
		if(element[j].selected)
		{
			max_wid = MAX(max_wid,element[j].offset + element[j].seqlen);
			min_wid = MIN(min_wid,element[j].offset);
			any_selected = TRUE;
		}
	if(any_selected == FALSE)
		return(XV_OK);

	mask = Calloc(max_wid - min_wid,sizeof(char));
	temp = Calloc(max_wid - min_wid,sizeof(char));
	temp_c = Calloc(max_wid - min_wid,sizeof(int));

	for(j=min_wid;j<max_wid;j++)
	{
		mask[j-min_wid] = '0';
		for(k=0;k<DataSet->numelements;k++)
			if(element[k].selected)
			{
				if(j>=element[k].offset && j< element[k].offset +
				    element[k].seqlen)
				{
					switch (element[k].elementtype)
					{
					case DNA:
					case RNA:
						if((getelem(&(element[k]),j) & 15) != 0)
							mask[j-min_wid] = '1';
						break;
					case PROTEIN:
						if(getelem(&(element[k]),j) != '-')
							mask[j-min_wid] = '1';
						break;
					default:
						break;
					}
				}
			}
	}

	for(j=0;j<DataSet->numelements;j++)
		if(element[j].selected)
			if(element[j].protect & PROT_WHITE_SPACE == 0)
			{
				Warning("Some sequences are protected");
				return(XV_OK);
			}

	if(compress_all)
	{
		for(j=0;j<DataSet->numelements;j++)
			if(element[j].selected)
			{
				this_elem = &(element[j]);
				offset = this_elem->offset;
				pos = 0;
				for(k=0; k<this_elem->seqlen;k++)
				{
					if(this_elem->tmatrix && (this_elem->
					    sequence[k]& 15)!='\0')
						temp[pos++] = this_elem->sequence[k];

					else if((this_elem->tmatrix == NULL) &&
					    (this_elem->sequence[k] != '-'))
						temp[pos++] = this_elem->sequence[k];
				}
				this_elem->seqlen = pos;
				for(k=0;k<pos;k++)
				{
					this_elem->sequence[k] = temp[k];
				}
				min_offset = MIN(min_offset,offset);
			}
	}
	else
	{
		/*
*	Use the mask to remove all positions where the mask is set to '0'
*/
		for(j=0;j<DataSet->numelements;j++)
			if(element[j].selected)
			{
				this_elem = &(element[j]);
				offset = this_elem->offset;
				pos = 0;

				for(k=offset; k<offset+this_elem->seqlen;k++)
				{
					if(mask[k-min_wid] == '1')
					{
						temp[(pos++)] = this_elem->sequence[k-offset];
					}
				}

				this_elem->seqlen = pos;
				for(k=0;k<pos;k++)
				{
					this_elem->sequence[k] = temp[k];
				}
				min_offset = MIN(min_offset,offset);
			}
	}

	for(j=0;j<DataSet->numelements;j++)
	{
		if(element[j].selected)
		{
			if(compress_all)
				element[j].offset =  -(DataSet->rel_offset);
			else
				element[j].offset -= min_offset;
		}
	}
	NormalizeOffset(DataSet);

	DataSet->maxlen = 0;

	for(j=0;j<DataSet->numelements;j++)
		DataSet->maxlen = MAX(DataSet->maxlen,element[j].seqlen+
		    element[j].offset);

	Cfree(mask);
	Cfree(temp);
	Cfree(temp_c);

	RepaintAll(FALSE);
	return(XV_OK);
}


RevSeqs(item,event)
Panel_item item;
Event *event;
{
	int j,i,slen,current_insert = 0,offset,*rev_mask;
	int min_range = 9999999,max_range = -9999999;
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet;
	NA_Base *rev_seq;
	Display *dpy;
	NA_Alignment *aln;
	NA_Sequence *element;

	if(DataSet == NULL)
		return;
	aln = (NA_Alignment*)DataSet;

	if(aln == NULL)
		return;
	element = aln->element;


	for(j=0;j<aln->numelements;j++)
		if(element[j].selected)
		{
			if(element[j].offset<min_range)
				min_range = element[j].offset;

			if(element[j].offset+element[j].seqlen>max_range)
				max_range = element[j].offset+element[j].seqlen;
		}

	for(j=0;j<aln->numelements;j++)
		if(element[j].selected && (element[j].protect &
		    PROT_TRANSLATION))
		{
			slen = element[j].seqlen;
			offset = element[j].offset;
			rev_seq =(NA_Base*)Calloc(element[j].seqmaxlen,
			    sizeof(NA_Base));
			for(i=0;i<slen;i++)
			{
				rev_seq[i] =
				    (NA_Base)getelem(&(element[j]),
				    slen+offset - i - 1);
			}
			Cfree(element[j].sequence);
			element[j].sequence =(NA_Base*)rev_seq;
			if(element[j].cmask)
			{
				/*
*	Warning, you will also need to change the offset
*	or else you will get a large chunk of sequence
*	with indels
*/
				rev_mask = (int*)Calloc(element[j].seqmaxlen,
				    sizeof(int));

				slen = element[j].seqlen;
				for(i=0;i<slen;i++)
				{
					rev_mask[i] =
					    element[j].cmask[slen+offset - i -1];
				}
				Cfree(element[j].cmask);
				element[j].cmask = rev_mask;
			}

			element[j].offset = min_range + (max_range -
			    (element[j].offset+ element[j].seqlen));

			if(element[j].attr & IS_5_TO_3)
			{
				element[j].attr |= IS_3_TO_5;
				element[j].attr &= (0xffff - IS_5_TO_3);
			}
			else if(element[j].attr & IS_3_TO_5)
			{
				element[j].attr |= IS_5_TO_3;
				element[j].attr &= (0xffff - IS_3_TO_5);
			}
		}
	RepaintAll(FALSE);
	return;
}

CompSeqs(item,event)
Panel_item item;
Event *event;
{
	int j,i,slen,offset,current_insert = 0;
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet;
	unsigned char temp,temp2;
	unsigned register k;
	Display *dpy;
	NA_Alignment *aln;
	NA_Sequence *element;

	if(DataSet == NULL)
		return;

	aln = (NA_Alignment*)DataSet;
	if(aln == NULL)
		return;
	element = aln->element;

	for(j=0;j<aln->numelements;j++)
		if(element[j].selected && (element[j].protect &
		    PROT_TRANSLATION))
		{
			char c;
			switch(element[j].elementtype)
			{
			case DNA:
			case RNA:
				slen = element[j].seqlen;
				offset = element[j].offset;
				for(i=0;i<slen;i++)
				{
					/*
*	Diddle the four lsb's
*/
					temp = '\0';
					temp2 = (unsigned char)getelem(&(element[j]),
					    i+offset);
					for(k=0;k<4;k++)
					{
						temp<<=1;
						temp|=((unsigned char)(temp2)>>k)&
						    (unsigned char)1;
					}
					putelem(&(element[j]),i+offset,(NA_Base)temp|
					    ((unsigned char)240 & temp2));
				}
				break;
			case MASK:
				slen = element[j].seqlen;
				offset = element[j].offset;
				for(i=0;i<slen;i++)
				{
					c = getelem(&(element[j]),i+offset);

					putelem(&(element[j]),i+offset,
					    (c=='1')?'0':
					    (c=='0')?'1':
					    (NA_Base)c);
				}
				break;
			case PROTEIN:
			case TEXT:
			default:
				break;
			}
			if(element[j].attr & IS_PRIMARY)
			{
				element[j].attr |= IS_SECONDARY;
				element[j].attr &= (0xffff - IS_PRIMARY);
			}
			else if(element[j].attr & IS_SECONDARY)
			{
				element[j].attr |= IS_PRIMARY;
				element[j].attr &= (0xffff - IS_SECONDARY);
			}

			if(element[j].attr & IS_5_TO_3)
			{
				element[j].attr |= IS_3_TO_5;
				element[j].attr &= (0xffff - IS_5_TO_3);
			}
			else if(element[j].attr & IS_3_TO_5)
			{
				element[j].attr |= IS_5_TO_3;
				element[j].attr &= (0xffff - IS_3_TO_5);
			}
		}
	RepaintAll(FALSE);

	return;
}


SetFilename(item,event)
Panel_item item;
Event *event;
{
	Panel_setting ps;
	ps = panel_text_notify(item,event);
	strncpy(FileName,(char*)(xv_get(item,PANEL_VALUE)),79);

	return(ps);
}

CaseChange(item,event)
Panel_item item;
Event *event;
{
	extern Canvas EditCan,EditNameCan;
	extern NA_Alignment *DataSet;
	NA_Alignment *aln;
	NA_Sequence *element;
	NA_DisplayData *ddata;
	NA_Base base;
	int *rmat,*tmat,i,j,pos=0,subselected = FALSE;

	if(DataSet == NULL)
		return;

	aln = (NA_Alignment*)DataSet;

	if(aln == NULL)
		return;

	element = aln->element;
	ddata = (NA_DisplayData*)(aln->na_ddata);
	tmat = element[ddata->cursor_y].tmatrix;
	rmat = element[ddata->cursor_y].rmatrix;


	for(j=0;j<aln->numelements;j++)
		if(aln->element[j].subselected && DataSet->selection_mask != NULL)
		{
			subselected = TRUE;
			j = aln->numelements;
		}

	if(subselected)
	{
		for(j=0;j<aln->numelements;j++)
			if(aln->element[j].subselected)
				for(i=0;i<aln->element[j].seqlen;i++)
					if(aln->selection_mask[i+aln->element[j].offset -
					aln->rel_offset] == '1')
					{
						pos = i+aln->element[j].offset;
						base = (char)getelem(&(element[j]), pos);
						switch(element[j].elementtype)
						{
						case DNA:
						case RNA:
							base = tmat[base];
							base = (base & 32)? (base & 223):
							    (base | 32);
							base = rmat[base];
							putelem(&(element[j]), pos, base);
							break;
						case TEXT:
						case PROTEIN:
							base = (base & 32)? (base & 223): (base | 32);
							putelem(&(element[j]), pos,base);
							break;
						case MASK:
						default:
							base = (base == '0')?
							    '1':(base == '1')? '0':base;
							putelem(&(element[j]), pos,base);
							break;
						}

					}
		/*
*	Repaint the screen, names not needed
*/
		RepaintAll(FALSE);
	}
	else
	{
		base = (char)getelem(&(element[ddata->cursor_y]),ddata->cursor_x);
		switch(element[ddata->cursor_y].elementtype)
		{
		case DNA:
		case RNA:
			base = tmat[base];
			base = (base & 32)? (base & 223): (base | 32);
			base = rmat[base];
			putelem(&(element[ddata->cursor_y]),ddata->cursor_x,
			    base);
			break;
		case TEXT:
		case PROTEIN:
			base = (base & 32)? (base & 223): (base | 32);
			putelem(&(element[ddata->cursor_y]),
			    ddata->cursor_x,base);
			break;
		case MASK:
		default:
			base = (base == '0')? '1':(base == '1')? '0':base;
			putelem(&(element[ddata->cursor_y]),
			    ddata->cursor_x,base);
			break;
		}
	}
	return XV_OK;
}

OrigDir(seq)
NA_Sequence *seq;
{
	int test;
	test = seq->attr;

#ifdef HGL
	if(test & IS_ORIG_PRIMARY)
		if(test & IS_ORIG_5_TO_3)
			return(1);
		else if(test & IS_ORIG_3_TO_5)
			return(-1);

	if(test & IS_ORIG_SECONDARY)
		if(test & IS_ORIG_5_TO_3)
			return(-1);
		else if(test & IS_ORIG_3_TO_5)
			return(1);
#else
	if(test & IS_PRIMARY)
		return(1);
	else if(test & IS_SECONDARY)
		return(-1);
#endif


	return(0);
}
