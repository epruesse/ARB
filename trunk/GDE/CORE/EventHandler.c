#include <stdio.h>
/* #include <malloc.h> */
#include <string.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/window.h>
#include <xview/textsw.h>
#include "menudefs.h"
#include "defines.h"


/*
HandleMenus():
	Callback routine for the menus.  Determine what function was called,
and perform the desired operation.

Copyright (c) 1989, University of Illinois board of trustees.  All rights
reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/
	extern GmenuItem *current_item;
	extern Gmenu menu[];
	extern Frame frame,pframe;
	extern Panel popup;
	extern int num_menus,BlockInput;


void HandleMenus(m,mi)
Menu m;
Menu_item mi;
{

	int i,j,/*k,*/curmenu,curitem;
	Gmenu *thismenu;
	GmenuItem *thisitem;
/* 	Panel choice; */
	char *label1;


	/*
*	Find menu, and menu item by searching menu[] and menu[].item[]
*	for the called menu item.
*/
	if(xv_get(pframe,WIN_SHOW))
	{
		/*
*	By returning after destroying the dialog box, a potential
*	problem with syncronization is avoided.  To demonstrate, compile
*	without the following "return", and click on a menu item several
*	times quickly.  The current solution is annoying in that if one
*	decides to change menu items without hitting <cancel>, they must
*	hit the menu button twice.
*/
		DONT(0,0);
		return;
	}

/*
*	Locate menu chosen...
*/
	BlockInput = TRUE;
	for(j=0;j<num_menus;j++)
		if(menu[j].X == m)
			curmenu = j;

	thismenu = &(menu[curmenu]);
	label1 = (char*)xv_get(mi,MENU_STRING);

/*
*	and menu item chosen.
*/
	for(j=0;j<menu[curmenu].numitems;j++)
		if(strcmp(label1,thismenu->item[j].label) == 0)
			curitem = j;

	thisitem = &(thismenu->item[curitem]);
	xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL,0);


/*
*	Create a temporary dialog popup, and set all of the calling
*	arguements by dialog box returned values.
*/

/*
*	For all needed arguments...
*/
	for(j=0;j<thisitem->numargs;j++)
	{
/*
*	Create a prompt for the argument
*/
		switch (thisitem->arg[j].type)
		{
		case SLIDER:
			thisitem->arg[j].X=xv_create(popup,PANEL_SLIDER,
			    PANEL_LAYOUT,PANEL_VERTICAL,
			    PANEL_LABEL_STRING,thisitem->arg[j].label,
			    PANEL_MIN_VALUE,thisitem->arg[j].min,
			    PANEL_MAX_VALUE,thisitem->arg[j].max,
			    PANEL_VALUE,thisitem->arg[j].value,
			    PANEL_NOTIFY_PROC, HandleMenuItem,
			    0);
			break;

		case TEXTFIELD:
			thisitem->arg[j].X = xv_create(popup,PANEL_TEXT,
			    PANEL_LAYOUT,PANEL_VERTICAL,
			    PANEL_VALUE_DISPLAY_LENGTH,32,
			    PANEL_LABEL_STRING,thisitem->arg[j].label,
			    PANEL_VALUE,thisitem->arg[j].textvalue,
			    PANEL_NOTIFY_LEVEL,PANEL_ALL,
			    PANEL_NOTIFY_PROC, HandleMenuItem,
			    0);
			break;

		case CHOOSER:
			thisitem->arg[j].X=xv_create(popup,
			    PANEL_CHOICE,
			    PANEL_LAYOUT,PANEL_HORIZONTAL,
			    PANEL_LABEL_STRING,thisitem->arg[j].label,
			    PANEL_NOTIFY_PROC, HandleMenuItem,
				PANEL_CHOICE_STRING,
			    0,thisitem->arg[j].choice[0].label,
			    0);
			for(i=1;i<thisitem->arg[j].numchoices;i++)
				xv_set(thisitem->arg[j].X,
				    PANEL_CHOICE_STRING, i,
				    thisitem->arg[j].choice[i].label,
				    0);
			xv_set(thisitem->arg[j].X,
			    PANEL_VALUE,thisitem->arg[j].value,
			    0);
			break;
		case CHOICE_LIST:
			thisitem->arg[j].X=xv_create(popup,
			    PANEL_LIST,
			    PANEL_LAYOUT, PANEL_VERTICAL,
			    PANEL_LABEL_STRING,thisitem->arg[j].label,
				PANEL_DISPLAY_ROWS,3,
			    PANEL_NOTIFY_PROC, HandleMenuItem,
				PANEL_LIST_STRING,
			    0,thisitem->arg[j].choice[0].label,
			    0);
			for(i=1;i<thisitem->arg[j].numchoices;i++)
				xv_set(thisitem->arg[j].X,
				    PANEL_LIST_STRING, i,
				    thisitem->arg[j].choice[i].label,
				    0);
			xv_set(thisitem->arg[j].X,
			    PANEL_VALUE,thisitem->arg[j].value,
			    0);
			break;
		case CHOICE_MENU:
			thisitem->arg[j].X=xv_create(popup,
			    PANEL_CHOICE_STACK,
			    PANEL_LAYOUT,PANEL_HORIZONTAL,
			    PANEL_LABEL_STRING,thisitem->arg[j].label,
			    PANEL_NOTIFY_PROC, HandleMenuItem,
				PANEL_CHOICE_STRING,
			    0,thisitem->arg[j].choice[0].label,
			    0);
			for(i=1;i<thisitem->arg[j].numchoices;i++)
				xv_set(thisitem->arg[j].X,
				    PANEL_CHOICE_STRING, i,
				    thisitem->arg[j].choice[i].label,
				    0);
			xv_set(thisitem->arg[j].X,
			    PANEL_VALUE,thisitem->arg[j].value,
			    0);
			break;


		default:
			break;
		};
	}

	xv_set(pframe,FRAME_LABEL,thisitem->label,
	    WIN_DESIRED_HEIGHT,1000,
	    WIN_DESIRED_WIDTH,1000,
/*
		I worry about this one, but a true dialog should not
		allow you to do anything other than respond to it.
*/

			WIN_GRAB_ALL_INPUT,TRUE,
	    0);

	current_item = thisitem;

/*
*	Fit it, and show it
*/
	window_fit(popup);
	window_fit(pframe);
	if((thisitem->numargs >0) || (thisitem->help !=NULL))
		xv_set(pframe,WIN_SHOW,TRUE,0);
	else
		DO();
	return;
}


/*
HandleMenuItem():
	Callback routine for buttons etc. in the dialog box.  Store the
values returned from the dialog box so that they can be used for calling
the external function.

Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/

HandleMenuItem(item,event)
Panel_item item;
Event *event;
{
	int i,j,thisarg;
	extern GmenuItem *current_item;

	Panel_setting ps;
/*
*	Find which value was modified...
*/
	for(j=0;j<current_item->numargs;j++)
		if(item == current_item->arg[j].X)
			thisarg = j;
/*
*	and store the new value.
*/
	switch(current_item->arg[thisarg].type)
	{
	case CHOICE_LIST:
		for(j=0;j < (int)xv_get(item,PANEL_LIST_NROWS);j++)
		{
			if((int)xv_get(item, PANEL_LIST_SELECTED, j) )
				current_item->arg[thisarg].value = j;
		}
		break;
	case CHOICE_MENU:
		current_item->arg[thisarg].value =
		    (int)xv_get(item,PANEL_VALUE);
		break;
	case CHOOSER:
		current_item->arg[thisarg].value =
		    (int)xv_get(item,PANEL_VALUE);
		break;
	case SLIDER:
		current_item->arg[thisarg].value =
		    (int)xv_get(item,PANEL_VALUE);
		break;
	case TEXTFIELD:
		ps = panel_text_notify(item,event);
		strcpy(current_item->arg[thisarg].textvalue,
		    (char*)xv_get(item,PANEL_VALUE));
		return(ps);
		break;
	default:
		Error("[HandleMenuItem] Menu argument type invalid");
	}
	return;
}


/*
DO():
	Call external function.

Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.
*/

DO()
{
	extern Frame pframe,frame;
	extern Panel popup;
	extern NA_Alignment *DataSet;
	extern char current_dir[];
	extern int OVERWRITE;

	int i,j,k,flag,select_mode;
	static int fileindx = 0;
	char *Action,buffer[GBUFSIZ],temp[80];


/*
*	Remove dialog.....
*/
	flag = FALSE;
	for(j=0;j<current_item->numinputs;j++)
		if(current_item->input[j].format != STATUS_FILE)
			flag = TRUE;

	if(flag && DataSet)
		select_mode = TestSelection();

/*
	Make sure that we are still in a writeable directory
*/
	(void)chdir(current_dir);
	for(j=0;j<current_item->numinputs;j++)
	{
		sprintf(buffer,"gde%d_%d",getpid(),fileindx++);
		current_item->input[j].name = String(buffer);
		switch(current_item->input[j].format)
		{
		case COLORMASK:
			WriteCMask(DataSet,buffer,select_mode,
			    current_item->input[j].maskable);
			break;
		case GENBANK:
			WriteGen(DataSet,buffer,select_mode,
			    current_item->input[j].maskable);
			break;
		case NA_FLAT:
			WriteNA_Flat(DataSet,buffer,select_mode,
			    current_item->input[j].maskable);
			break;
		case STATUS_FILE:
			WriteStatus(DataSet,buffer,select_mode);
			break;
		case GDE:
			WriteGDE(DataSet,buffer,select_mode,
			    current_item->input[j].maskable);
			break;
		default:
			break;
		}
	}

	for(j=0;j<current_item->numoutputs;j++)
	{
		sprintf(buffer,"gde%d_%d",getpid(),fileindx++);
		current_item->output[j].name = String(buffer);
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
	    WIN_SHOW,FALSE,
	    0);

/*
*	Reset dialog for next call...
*/

	popup = xv_get(pframe,FRAME_CMD_PANEL);

	xv_create (popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"HELP",
	    PANEL_NOTIFY_PROC,HELP,
	    0);

	xv_create (popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"OK",
	    PANEL_NOTIFY_PROC,DO,
	    0);

	xv_create(popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"Cancel",
	    PANEL_NOTIFY_PROC,DONT,
	    0);

	/*
*	Create the command line for external the function call
*/
	Action = (char*)strdup(current_item->method);
	if(Action == NULL)
		Error("DO(): Error in duplicating method string");
	for(j=0;j<current_item->numargs;j++)
		Action = ReplaceArgs(Action,current_item->arg[j]);

	for(j=0;j<current_item->numinputs;j++)
		Action = ReplaceFile(Action,current_item->input[j]);

	for(j=0;j<current_item->numoutputs;j++)
		Action = ReplaceFile(Action,current_item->output[j]);



	/*
*	call and go...
*/

	xv_set(pframe,FRAME_BUSY,TRUE,0);
	xv_set(frame,FRAME_BUSY,TRUE,0);
	system(Action);
	cfree(Action);
	xv_set(pframe,FRAME_BUSY,FALSE,0);
	xv_set(frame,FRAME_BUSY,FALSE,0);
	BlockInput = FALSE;

	for(j=0;j<current_item->numoutputs;j++)
	{
		if(current_item->output[j].overwrite)
		{
			if(current_item->output[j].format == GDE)
				OVERWRITE = TRUE;
			else
				Warning("Overwrite mode only available for GDE format");
		}
		switch(current_item->output[j].format)
		{
/*
*     The LoadData routine must be reworked so that
*     OpenFileName uses it, and so I can remove the
*     major kluge in OpenFileName().
*/
		case GENBANK:
		case NA_FLAT:
		case GDE:
			OpenFileName(current_item->output[j].name,NULL);
			break;
		case COLORMASK:
			ReadCMask(current_item->output[j].name);
			break;
		case STATUS_FILE:
			ReadStatus(current_item->output[j].name);
			break;
		default:
			break;
		}
		OVERWRITE = FALSE;
	}
	for(j=0;j<current_item->numoutputs;j++)
	{
		if(!current_item->output[j].save)
		{
			sprintf(buffer,"/bin/rm -f %s",
			    current_item->output[j].name);
			system(buffer);
		}
	}

	for(j=0;j<current_item->numinputs;j++)
	{
		if(!current_item->input[j].save)
		{
			sprintf(buffer,"/bin/rm -f %s",
			    current_item->input[j].name);
			system(buffer);
		}
	}
	return;
}


/*
ReplaceArgs():
	Replace all command line arguements with the appropriate values
stored for the chosen menu item.

Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/

char *ReplaceFile(Action,file)
char *Action;
GfileFormat file;
{
	char *symbol,*method,*temp;
	int i,j,newlen;
	symbol = file.symbol;
	method = file.name;

	for(; (i=Find2(Action,symbol)) != -1;)
	{
		newlen = strlen(Action)-strlen(symbol) + strlen(method)+1;
		temp = calloc(newlen,1);
		if (temp == NULL)
			Error("ReplaceFile():Error in calloc");
		strncat(temp,Action,i);
		strncat(temp,method,strlen(method));
		strcat( temp,&(Action[i+strlen(symbol)]) );
		cfree(Action);
		Action = temp;
	}
	return(Action);
}





char *ReplaceArgs(Action,arg)
char *Action;
GmenuItemArg arg;
{
	/*
*	The basic idea is to replace all of the symbols in the method
*	string with the values picked in the dialog box.  The method
*	is the general command line structure.  All arguements have three
*	parts, a label, a method, and a value.  The method never changes, and
*	is used to represent '-flag's for a given function.  Values are the
*	associated arguements that some flags require.  All symbols that
*	require argvalue replacement should have a '$' infront of the symbol
*	name in the itemmethod definition.  All symbols without the '$' will
*	be replaced by their argmethod.  There is currently no way to do a label
*	replacement, as the label is considered to be for use in the dialog
*	box only.  An example command line replacement would be:
*
*		itemmethod=> 		"lpr arg1 $arg1 $arg2"
*
*		arglabel arg1=>		"To printer?"
*		argmethod arg1=>	"-P"
*		argvalue arg1=>		"lw"
*
*		arglabel arg2=>		"File name?"
*		argvalue arg2=>		"foobar"
*		argmethod arg2=>	""
*
*	final command line:
*
*		lpr -P lw foobar
*
*	At this point, the chooser dialog type only supports the arglabel and
*	argmethod field.  So if an argument is of type chooser, and
*	its symbol is "this", then "$this" has no real meaning in the
*	itemmethod definition.  Its format in the .GDEmenu file is slighty
*	different as well.  A choice from a chooser field looks like:
*
*		argchoice:Argument_label:Argument_method
*
*
*/
	char *symbol,*method,*textvalue,buf1[GBUFSIZ],buf2[GBUFSIZ],*temp;
	int i,j,newlen,type;
	symbol = arg.symbol;
	method = arg.method;
	textvalue = arg.textvalue;
	type = arg.type;
	if(type == SLIDER)
	{
		textvalue = buf2;
		sprintf(buf2,"%d",arg.value);
	}
	else if((type == CHOOSER) || (type == CHOICE_MENU) || (type == CHOICE_LIST))
	{
		method = arg.choice[arg.value].method;
		textvalue = arg.choice[arg.value].method;
	}

	if(textvalue == NULL)
		textvalue="";

	if(method == NULL)
		method="";

	if(symbol == NULL)
		symbol="";

	for(; (i=Find2(Action,symbol)) != -1;)
	{
		if(i>0 && Action[i-1] =='$' )
		{
			newlen = strlen(Action)-strlen(symbol)
			    +strlen(textvalue);
			temp = calloc(newlen,1);
			if (temp == NULL)
				Error("ReplaceArgs():Error in calloc");
			strncat(temp,Action,i-1);
			strncat(temp,textvalue,strlen(textvalue));
			strcat( temp,&(Action[i+strlen(symbol)]) );
			cfree(Action);
			Action = temp;
		}
		else
		{
			newlen = strlen(Action)-strlen(symbol)
			    +strlen(method)+1;
			temp = calloc(newlen,1);
			if (temp == NULL)
				Error("ReplaceArgs():Error in calloc");
			strncat(temp,Action,i);
			strncat(temp,method,strlen(method));
			strcat( temp,&(Action[i+strlen(symbol)]) );
			cfree(Action);
			Action = temp;
		}
	}
	return(Action);
}




/*
DONT(0,0):
	Dont execute the command associated with the current dialog box.
This function corresponds to the <cancel> button on the dialog box.

Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/

DONT(item,event)
Panel_item item;
Event *event;
{
	extern Frame pframe,frame;
	extern Panel popup;
	extern int BlockInput;
	int i,j,k;

	/*
*	Reset the dialog box, andf remove it.
*/
	xv_destroy_safe(pframe);
	pframe = xv_create(frame,FRAME_CMD,
		FRAME_CMD_PUSHPIN_IN,TRUE,
	    FRAME_DONE_PROC,FrameDone,
	    FRAME_SHOW_RESIZE_CORNER,FALSE,
	    WIN_DESIRED_HEIGHT,100,
	    WIN_DESIRED_WIDTH,300,
	    XV_X,300,
	    XV_Y,150,
	    WIN_SHOW,FALSE,
	    0);

	popup = xv_get(pframe,FRAME_CMD_PANEL);
/*
	popup = xv_create(pframe,PANEL,
	    PANEL_LAYOUT,PANEL_HORIZONTAL,
	    0);
*/

	xv_create (popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"HELP",
	    PANEL_NOTIFY_PROC,HELP,
	    0);

	xv_create (popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"OK",
	    PANEL_NOTIFY_PROC,DO,
	    0);

	xv_create(popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"Cancel",
	    PANEL_NOTIFY_PROC,DONT,
	    0);

	BlockInput = FALSE;

	return;
}

FrameDone(this_frame)
Frame this_frame;
{
	extern Frame pframe;
	if(this_frame == pframe)
		DONT(0,0);

	return(XV_OK);
}
NANameEvents(win,event,arg)
Xv_window win;
Event *event;
Notify_arg arg;
{
	extern int EditMode;
	extern NA_Alignment *DataSet;
	NA_DisplayData *ddata;
	NA_Alignment *aln;
	NA_Sequence *this_seq;
	extern int first_select,BlockInput;
	int i,j,x,y,redraw = FALSE;

	if(DataSet == NULL || BlockInput)
		return;

	aln = (NA_Alignment*)DataSet;
	ddata = (NA_DisplayData*)(aln->na_ddata);

	if(ddata == NULL)
		return;

	x=event_x(event)/ddata->font_dx;
	y=event_y(event)/ddata->font_dy +
	    ddata->top_seq;

	y=MIN(y,aln->numelements - 1);
	y=MAX(y,0);

	this_seq = &(aln->element[y]);

	if (event_id(event) == LOC_WINENTER)
		win_set_kbd_focus(win,(Window)xv_get(win,XV_XID));
	else if(event_is_down(event) && event_is_ascii(event) &&
	    event_meta_is_down(event))
		DoMeta(event_id(event));

	else if(!event_is_up(event))
	{
		switch (event_action(event))
		{
		case ACTION_SELECT:
			if(!event_shift_is_down(event))
			{
				for(j=0;j<aln->numelements;j++)
					aln->element[j].
					    selected = FALSE;
				redraw = TRUE;
			}

			if(x<=strlen(this_seq->short_name))
			{
				redraw = TRUE;
				first_select = y;
			}
			else
				first_select = -1;
			break;
		default:
			break;
		}
	}
	else if(first_select != -1)
		switch (event_action(event))
		{
		case ACTION_SELECT:
			if(!event_shift_is_down(event))
			{
				for(j=0;j<aln->numelements;j++)
					aln->element[j].selected
					    = FALSE;
			}
			if(x<=strlen(this_seq->short_name))
			{
				for(j=MIN(first_select,y);
				    j<=MAX(first_select,y);j++)
					aln->element[j].selected =
					    aln->element[j].selected ?
					    FALSE:TRUE;
				redraw = TRUE;
			}
			break;
		default:
			break;
		}

	if(redraw)
		DrawNANames(xv_get(win,XV_DISPLAY),xv_get(win,XV_XID));
	return;
}


DoMeta(Code)
int Code;
{

	int k,j;
	extern int num_menus;
	extern Gmenu menu[];

	for(j=0;j<num_menus;j++)
		for(k=0;k<menu[j].numitems;k++)
		{
			if(((menu[j].item[k].meta|32) ==(char)(Code|32)) &&
			    (char)Code !='\0')
			{
				HandleMeta(j,k);
				k=menu[j].numitems;
				j=num_menus;
			}
		}
	return;
}


/*
HandleMeta():
	Callback routine for the menus.  Determine what function was called,
and perform the desired operation.

*/


HandleMeta(curmenu,curitem)
int curitem,curmenu;
{
	extern Gmenu menu[];
	extern GmenuItem *current_item;
	extern Frame frame,pframe;
	extern Panel popup;
	extern int num_menus;

	int i,j,k;
	Gmenu *thismenu;
	GmenuItem *thisitem;
	Panel choice;
	char *label1;

	if(xv_get(pframe,WIN_SHOW))
	{
		DONT(0,0);
		return;
	}

	thismenu = &(menu[curmenu]);
	thisitem = &(thismenu->item[curitem]);
	xv_set(popup,PANEL_LAYOUT,PANEL_VERTICAL,0);

	for(j=0;j<thisitem->numargs;j++)
	{
		/*
*	Create a prompt for the argument
*/
		switch (thisitem->arg[j].type)
		{
		case SLIDER:
			thisitem->arg[j].X=xv_create(popup,PANEL_SLIDER,
			    PANEL_LAYOUT,PANEL_VERTICAL,
			    PANEL_LABEL_STRING,thisitem->arg[j].label,
			    PANEL_MIN_VALUE,thisitem->arg[j].min,
			    PANEL_MAX_VALUE,thisitem->arg[j].max,
			    PANEL_VALUE,thisitem->arg[j].value,
			    PANEL_NOTIFY_PROC, HandleMenuItem,
			    PANEL_TICKS,10,
			    0);
			break;

		case TEXTFIELD:
			thisitem->arg[j].X = xv_create(popup,PANEL_TEXT,
			    PANEL_LAYOUT,PANEL_VERTICAL,
			    PANEL_VALUE_DISPLAY_LENGTH,32,
			    PANEL_LABEL_STRING,thisitem->arg[j].label,
			    PANEL_VALUE,thisitem->arg[j].textvalue,
			    PANEL_NOTIFY_LEVEL,PANEL_ALL,
			    PANEL_NOTIFY_PROC, HandleMenuItem,
			    0);
			break;

		case CHOOSER:
			thisitem->arg[j].X=xv_create(popup,
			    PANEL_CHOICE,
			    PANEL_LAYOUT,PANEL_HORIZONTAL,
			    PANEL_LABEL_STRING,thisitem->arg[j].label,
			    PANEL_NOTIFY_PROC, HandleMenuItem,
			    PANEL_CHOICE_STRING,
			    0,thisitem->arg[j].choice[0].label,
			    0);
			for(i=1;i<thisitem->arg[j].numchoices;i++)
				xv_set(thisitem->arg[j].X,
				    PANEL_CHOICE_STRING, i,
				    thisitem->arg[j].choice[i].label,
				    0);
			xv_set(thisitem->arg[j].X,
			    PANEL_VALUE,thisitem->arg[j].value,
			    0);
			break;
		case CHOICE_MENU:
			thisitem->arg[j].X=xv_create(popup,
			    PANEL_CHOICE_STACK,
			    PANEL_LAYOUT,PANEL_VERTICAL,
			    PANEL_LABEL_STRING,thisitem->arg[j].label,
			    PANEL_NOTIFY_PROC, HandleMenuItem,
			    PANEL_CHOICE_STRING,
			    0,thisitem->arg[j].choice[0].label,
			    0);
			for(i=1;i<thisitem->arg[j].numchoices;i++)
				xv_set(thisitem->arg[j].X,
				    PANEL_CHOICE_STRING, i,
				    thisitem->arg[j].choice[i].label,
				    0);
			xv_set(thisitem->arg[j].X,
			    PANEL_VALUE,thisitem->arg[j].value,
			    0);
			break;


		default:
			break;
		};
	}

	xv_set(pframe,FRAME_LABEL,thisitem->label,
	    WIN_DESIRED_HEIGHT,1000,
	    WIN_DESIRED_WIDTH,1000,
	    WIN_GRAB_ALL_INPUT,TRUE,
	    0);

	current_item = thisitem;

	/*
*	Fit it, and show it
*/
	window_fit(popup);
	window_fit(pframe);
	if(thisitem->numargs >0)
		xv_set(pframe,WIN_SHOW,TRUE,0);
	else
		DO();
	return;
}

HELP(item,event)
Panel_item item;
Event *event;
{
	extern GmenuItem *current_item;
	extern Frame pframe;
	extern Panel popup;
	FILE *file;
	char help_file[1024];

	if(current_item->help == NULL)
	{
		Warning("Cannot open help file");
		return;
	}
	file = 0;
#ifdef USE_ARB
	sprintf(help_file,"%s/GDEHELP/%100s",getenv("ARBHOME"),help_file);
#else
	if((file == NULL) && (getenv("GDE_HELP_DIR") != NULL))
	{
		strncpy(help_file,getenv("GDE_HELP_DIR"),1023);
		strncat(help_file,"/",1023 - strlen(help_file));
		strncat(help_file,current_item->help,1023 - strlen(help_file));
	}
#endif
	file = fopen(help_file,"r");

	if(file == NULL)
	{
		Warning("Cannot find help file");
		return;
	}

	fclose(file);
	window_fit( xv_create(pframe,TEXTSW,
	    WIN_INHERIT_COLORS,TRUE,
	    WIN_BELOW,popup,
	    TEXTSW_READ_ONLY,TRUE,
	    XV_HEIGHT,180,
	    XV_WIDTH,80*8,
	    TEXTSW_FILE,help_file,
	    0)
	    );
	window_fit(pframe);
	xv_set(item,PANEL_INACTIVE,TRUE,0);

	return;
}

