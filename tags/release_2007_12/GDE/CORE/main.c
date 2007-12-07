#include <stdio.h>
#include <unistd.h>
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
#include "globals.h"

/*
Main()

Copyright (c) 1989, University of Illinois board of trustees.  All rights
reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.


Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/

Gmenu menu[100];
int num_menus = 0,repeat_cnt = 0;
Frame frame,pframe,infoframe;
Panel popup,infopanel;
Panel_item left_foot,right_foot;
Canvas EditCan,EditNameCan;
int DisplayType;
GmenuItem *current_item;
NA_Alignment *DataSet = NULL;
NA_Alignment *Clipboard = NULL;
char **TextClip;
int TextClipSize = 0,TextClipLength = 0;

/*
*	Icon structure (pixmap dependent)
*/


static unsigned short GDEicon[258]={
#include "icon_gde.bitmap"
};

mpr_static(iconpr,64,64,1,GDEicon);

int main(argc,argv)
     int    argc;
     char **argv;
{

	Icon tool_icon;			/* obvious	*/
	extern char FileName[],current_dir[];

	int type = GENBANK;		/* default file type */
	DataSet = NULL;
	Clipboard = (NA_Alignment*)Calloc(1,sizeof(NA_Alignment));
	DisplayType = NASEQ_ALIGN;	/* default data type */
	Clipboard->maxnumelements = 5;
	Clipboard->element =(NA_Sequence*)Calloc(Clipboard->maxnumelements,
	    sizeof(NA_Sequence));

/*
*	Connect to server, and set up initial XView data types
*	that are common to ALL display types
*/
	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc,argv,0);


	/*
*	Main frame (primary window);
*/

	frame = xv_create(0,FRAME,
	    FRAME_NO_CONFIRM,FALSE,
	    FRAME_LABEL,    "Genetic Data Environment 2.2",
	    FRAME_INHERIT_COLORS,TRUE,
	    XV_WIDTH,700,
	    XV_HEIGHT,500,
	    FRAME_SHOW_FOOTER,TRUE,
	    0);

	/*
*	Popup frame (dialog box window), and default settings in
*	the dialog box.  These are changed to fit each individual
*	command's needs in EventHandler().
*/
	infoframe = xv_create(frame,FRAME_CMD,
	    FRAME_LABEL,"Messages",
	    WIN_DESIRED_HEIGHT,100,
	    WIN_DESIRED_WIDTH,300,
	    FRAME_SHOW_RESIZE_CORNER,TRUE,
	    FRAME_INHERIT_COLORS,TRUE,
	    FRAME_CLOSED,FALSE,
	    WIN_SHOW,FALSE,
	    0);

	pframe = xv_create(frame,FRAME_CMD,
		FRAME_CMD_PUSHPIN_IN,TRUE,
	    FRAME_DONE_PROC,FrameDone,
	    XV_HEIGHT,100,
	    XV_WIDTH,300,
	    FRAME_SHOW_RESIZE_CORNER,FALSE,
	    FRAME_CLOSED,FALSE,
	    XV_X,300,
	    XV_Y,150,
	    WIN_SHOW,FALSE,
	    0);

	infopanel = xv_get(infoframe,FRAME_CMD_PANEL);
	xv_set(infopanel, PANEL_LAYOUT,PANEL_VERTICAL,
	    XV_WIDTH,300,
	    XV_HEIGHT,50,
	    0);

	left_foot = xv_create(infopanel,PANEL_MESSAGE,0);
	right_foot = xv_create(infopanel,PANEL_MESSAGE,0);

	window_fit(infoframe);

/*
	popup = xv_create(pframe,PANEL,
	    PANEL_LAYOUT,PANEL_HORIZONTAL,
	    0);
*/
	popup = xv_get(pframe,FRAME_CMD_PANEL);

	xv_create(popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"HELP",
	    PANEL_NOTIFY_PROC,HELP,
	    0);

	xv_create(popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"OK",
	    PANEL_NOTIFY_PROC,DO,
	    0);

	xv_create(popup,PANEL_BUTTON,
	    PANEL_LABEL_STRING,"Cancel",
	    PANEL_NOTIFY_PROC,DONT,
	    0);

/*
*	Keep original directory where program was started
*/
	(void)getcwd(current_dir,GBUFSIZ);

	ParseMenu();
	if(argc>1)	LoadData(argv[1]);
	GenMenu(type);

	/*
*	Set up the basics of the displays, and off to the main loop.
*/
	BasicDisplay(DataSet);

	if(DataSet != NULL)
		((NA_Alignment*)DataSet)->na_ddata = (char*)SetNADData
		    ((NA_Alignment*)DataSet,EditCan,EditNameCan);

	tool_icon = xv_create(0,ICON,
	    ICON_IMAGE,&iconpr,
	    ICON_LABEL,strlen(FileName)>0?FileName:"GDE",
	    0);

	xv_set(frame,
	    FRAME_ICON,tool_icon,
	    0);

	window_main_loop(frame);
	exit(0);
}

