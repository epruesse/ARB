/* #include <malloc.h> */
#include <X11/X.h>
#include <X11/Xlib.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/scrollbar.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <xview/xv_xrect.h>
#include <xview/cms.h>
#include <xview/notice.h>
#include "menudefs.h"
#include "defines.h"

	extern Panel menubar;
	extern Canvas EditNameCan;
	extern Frame frame;
	extern Canvas EditCan;
	extern NA_Alignment *DataSet;
	extern Xv_singlecolor Default_Colors[];
	extern int DisplayType;
	extern Gmenu menu[];
	extern int num_menus;

Panel menubar = 0;

/*
BasicDisplay():
Set up menus and primary display.

Copyright (c) 1989, University of Illinois board of trustees.  All rights
reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.
*/
	extern Frame frame;

int QuitGDE()
{
	extern Frame    frame;
	if (notice_prompt(frame, NULL, NOTICE_MESSAGE_STRINGS,
			  "Are you sure you want to Quit?", NULL,
			  NOTICE_BUTTON, "Confirm", 1,
			  NOTICE_BUTTON, "Cancel", 2,
			  0) == 1) {
		xv_destroy_safe(frame);
		exit(0);
	} else
		return (XV_OK);
}

int QuitGDE_update(Panel_item item, Event *event)
{
	Updata_Arbdb(item,event);
	return QuitGDE();
}

Panel BasicDisplay(DataSet)
NA_Alignment *DataSet;
{
/* 	int i,j,k; */
	int j;

	if (!menubar)
	{
		menubar = xv_create(frame,PANEL,
		    0);
		/*
*	For all menus defined in the .GDEmenu file, create a corresponding
*	menu on the menu bar, and tie its XView object to the internal
*	menu structure.
*/
		for(j=0;j<num_menus ;j++)
		{
			menu[j].button=xv_create(menubar,PANEL_BUTTON,
			    PANEL_LABEL_STRING,menu[j].label,
			    PANEL_ITEM_MENU, menu[j].X,
			    0);
		}
	}

	/*
*	Determine which type of display should be generated based on the
*	current view of the data set.
*/
	MakeNAADisplay();
	return menubar;
}


/*
GenMenu():
	Generate the menus described in the .GDEmenu file.  Link menu items
to their corresponding XView objects.

Copyright (c) 1989, University of Illinois board of trustees.  All rights
reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.
*/

void GenMenu(type)
int type;
{
/* 	int i,j,k; */
	int curmenu,curitem/*,curarg,curinput,curoutput,curchoice*/;
	extern Gmenu menu[];
	extern Frame frame;
	extern int num_menus;
	Gmenu *thismenu;
	GmenuItem *thisitem;
	/*
*	For all menus...
*/
	for(curmenu = 0;curmenu<num_menus;curmenu++)
	{
		thismenu = &(menu[curmenu]);
		thismenu->X = xv_create(0,MENU,0);
		if(strcmp(thismenu->label,"File")==0)
		{
			if (DataSet && DataSet->gb_main) {
				xv_set(thismenu->X,
				       MENU_ITEM,
				       MENU_STRING, "Update Sequences in ARB",
				       MENU_NOTIFY_PROC, Updata_Arbdb,
				       0,
				       MENU_ITEM,
				       MENU_STRING, "Properties...",
				       MENU_NOTIFY_PROC, ChangeDisplay,
				       0,
				       MENU_ITEM,
				       MENU_STRING, "Protections...<meta p>",
				       MENU_NOTIFY_PROC, SetProtection,
				       0,
				       MENU_ITEM,
				       MENU_STRING, "Get info... <meta i>",
				       MENU_NOTIFY_PROC, ModAttr,
				       0,
				       0);
			} else {
				xv_set(thismenu->X,
				       MENU_ITEM,
				       MENU_STRING, "Open...",
				       MENU_NOTIFY_PROC, Open,
				       0,
				       MENU_ITEM,
				       MENU_STRING, "Save as...",
				       MENU_NOTIFY_PROC, SaveAs,
				       0,
				       MENU_ITEM,
				       MENU_STRING, "Properties...",
				       MENU_NOTIFY_PROC, ChangeDisplay,
				       0,
				       MENU_ITEM,
				       MENU_STRING, "Protections...<meta p>",
				       MENU_NOTIFY_PROC, SetProtection,
				       0,
				       MENU_ITEM,
				       MENU_STRING, "Get info... <meta i>",
				       MENU_NOTIFY_PROC, ModAttr,
				       0,
				       0);
			}
		}
		else if(strcmp(thismenu->label,"Edit")==0)
		{
			xv_set(thismenu->X,
			    MENU_ITEM,
			    MENU_STRING,"Select All",
			    MENU_NOTIFY_PROC,SelectAll,
			    0,
			    MENU_ITEM,
			    MENU_STRING,"Select by name...",
			    MENU_NOTIFY_PROC,SelectBy,
			    0,
			    MENU_ITEM,
			    MENU_STRING,"Cut",
			    MENU_NOTIFY_PROC,EditCut,
			    0,
			    MENU_ITEM,
			    MENU_STRING,"Copy",
			    MENU_NOTIFY_PROC,EditCopy,
			    0,
			    MENU_ITEM,
			    MENU_STRING,"Paste",
			    MENU_NOTIFY_PROC,EditPaste,
			    0,
			    MENU_ITEM,
			    MENU_STRING,"Group  <meta g>",
			    MENU_NOTIFY_PROC,Group,
			    0,
			    MENU_ITEM,
			    MENU_STRING,"Ungroup <meta u>",
			    MENU_NOTIFY_PROC,Ungroup,
			    0,
			    MENU_ITEM,
			    MENU_STRING,"Compress",
			    MENU_NOTIFY_PROC,CompressAlign,
			    0,
			    MENU_ITEM,
			    MENU_STRING,"Reverse",
			    MENU_NOTIFY_PROC,RevSeqs,
			    0,
			    MENU_ITEM,
			    MENU_STRING,"Change case",
			    MENU_NOTIFY_PROC,CaseChange,
			    0,
			    0);
		}
		else if(strcmp(thismenu->label,"DNA/RNA")==0)
		{

			xv_set(thismenu->X,
			    MENU_ITEM,
			    MENU_STRING,"Complement",
			    MENU_NOTIFY_PROC,CompSeqs,
			    0,
			    0);
		}

		/*
*	For all menu items of the current menu...
*/
		for(curitem = 0;curitem<thismenu->numitems;curitem++)
		{
			thisitem = &(thismenu->item[curitem]);
			xv_set(thismenu->X,
			    MENU_ITEM,
			    MENU_STRING,thismenu->item[curitem].label,
			    MENU_NOTIFY_PROC,HandleMenus,
			    0,
			    0);
		}
		/*
*	Make the menu "pin"able
*/
		xv_set(thismenu->X,
		    MENU_GEN_PIN_WINDOW,frame,thismenu->label,
		    0);
	}
	if (DataSet && DataSet->gb_main) {
		xv_set(menu[0].X,
		    MENU_ITEM,
		    MENU_STRING,"Quit ( and Update Sequences in ARB )",
		    MENU_NOTIFY_PROC,QuitGDE_update,
		    0,
		    MENU_ITEM,
		    MENU_STRING,"Quit ( Without Sequence Changes)",
		    MENU_NOTIFY_PROC,QuitGDE,
		    0,
		    0);
	}else{
		xv_set(menu[0].X,
		    MENU_ITEM,
		    MENU_STRING,"Quit",
		    MENU_NOTIFY_PROC,QuitGDE,
		    0,
		    0);
	}
	return;
}


/*
MakeNAADisplay():
	Set up the generic display rectangle to be a DNA/RNA display.

Copyright (c) 1989, University of Illinois board of trustees.  All rights
reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.
*/

void MakeNAADisplay()
{
	extern Panel menubar;
	extern Canvas EditNameCan;
	extern Frame frame;
	extern Canvas EditCan;
/* 	extern NA_Alignment *DataSet; */
	extern Xv_singlecolor Default_Colors[];
	Scrollbar hscroll,vscroll;

	GC gc;
	Cms colmap;
	XGCValues gcv;
	Display *dpy;
	Xv_font font;
	int j,fnt_siz,fnt_style,depth;

	extern unsigned char *greys[];
	extern Pixmap grey_pm[];
	/*
*	The window will be scrollable in both X and Y
*/
	xv_set(menubar,WIN_FIT_HEIGHT,0,0);
	/*
*	set up a window for the organism names on the left side of
*	the screen.
*/


	EditNameCan = xv_create(frame,CANVAS,
	    WIN_BELOW,menubar,
	    WIN_WIDTH,150,
	    CANVAS_AUTO_EXPAND,TRUE,
	    CANVAS_AUTO_SHRINK,TRUE,
	    CANVAS_RETAINED,FALSE,
	    CANVAS_X_PAINT_WINDOW,TRUE,
	    OPENWIN_ADJUST_FOR_HORIZONTAL_SCROLLBAR,TRUE,
	    CANVAS_AUTO_CLEAR,FALSE,
	    CANVAS_REPAINT_PROC,DummyRepaint,
	    CANVAS_MIN_PAINT_WIDTH,150,
	    WIN_INHERIT_COLORS,TRUE,
	    0);

	(void)xv_set(canvas_paint_window(EditNameCan),
	    WIN_EVENT_PROC,NANameEvents,
	    WIN_CONSUME_EVENTS,
	    WIN_MOUSE_BUTTONS,
	    /*
				LOC_DRAG,
*/
	LOC_WINENTER,
	    WIN_ASCII_EVENTS,
	    WIN_META_EVENTS,
	    0,
	    0);


	/*
*	Set up a window to hold the NA sequences.
*/

	EditCan=xv_create(frame,CANVAS,
	    WIN_RIGHT_OF,EditNameCan,
	    CANVAS_AUTO_SHRINK,TRUE,
	    CANVAS_AUTO_EXPAND,TRUE,
/*
	    CANVAS_CMS_REPAINT,TRUE,
*/
	    CANVAS_X_PAINT_WINDOW,TRUE,
	    CANVAS_AUTO_CLEAR,FALSE,
	    CANVAS_RETAINED,FALSE,
	    CANVAS_MIN_PAINT_WIDTH,150,
	    OPENWIN_SPLIT,
	    OPENWIN_SPLIT_INIT_PROC,InitEditSplit,
	    OPENWIN_SPLIT_DESTROY_PROC,DestroySplit,
	    NULL,
	    WIN_INHERIT_COLORS,FALSE,
	    WIN_BELOW,menubar,
		CANVAS_REPAINT_PROC,RepaintNACan,
	    0);

/*
*	This causes resize events to occur even if the screen shrinks
*	in size.
*/
	xv_set(canvas_paint_window(EditCan),
	    WIN_BIT_GRAVITY,ForgetGravity,
	    0);


	hscroll = xv_create(EditCan,SCROLLBAR,
	    SCROLLBAR_DIRECTION,SCROLLBAR_HORIZONTAL,
	    SCROLLBAR_SPLITTABLE,TRUE,
	    SCROLLBAR_OVERSCROLL,0,
	    0);

	vscroll = xv_create(EditCan,SCROLLBAR,
	    SCROLLBAR_DIRECTION,SCROLLBAR_VERTICAL,
	    SCROLLBAR_SPLITTABLE,FALSE,
	    SCROLLBAR_OVERSCROLL,0,
	    0);

	notify_interpose_event_func(
	    xv_get(hscroll,SCROLLBAR_NOTIFY_CLIENT),
	    EditCanScroll,NOTIFY_SAFE);

	dpy = (Display *)xv_get(EditNameCan, XV_DISPLAY);

	gc = DefaultGC(dpy,DefaultScreen(dpy));
	depth = xv_get(frame,WIN_DEPTH);
	if(depth>3)
	{
		colmap = (Cms)xv_find(frame,CMS,
		    CMS_NAME,"GDE Palette",
		    XV_AUTO_CREATE,FALSE,
		    0);


		if(colmap == 0)
			colmap = (Cms)xv_create(0,CMS,
			    CMS_TYPE,XV_STATIC_CMS,
			    CMS_SIZE,16,
			    CMS_COLORS,Default_Colors,
			    0);

		xv_set(EditCan,
		    WIN_CMS_NAME,"GDE Palette",
		    WIN_CMS, colmap,
		    WIN_FOREGROUND_COLOR,8,
		    WIN_BACKGROUND_COLOR,15,
		    WIN_INHERIT_COLORS,FALSE,
		    0);
	}

	(void)xv_set(canvas_paint_window(EditCan),
	    WIN_EVENT_PROC,NAEvents,
	    WIN_CONSUME_EVENTS,
	    WIN_MOUSE_BUTTONS,
	    LOC_WINENTER,
	    WIN_ASCII_EVENTS,
	    WIN_META_EVENTS,
	    0,
	    0);

	font = (Xv_font)xv_get(frame,XV_FONT);
	fnt_siz = (int)xv_get(font,FONT_SIZE);
	fnt_style = (int)xv_get(font,FONT_STYLE);
	font = (Xv_font)xv_find(frame,FONT,
	    FONT_FAMILY,FONT_FAMILY_DEFAULT_FIXEDWIDTH,
	    FONT_STYLE,fnt_style,
	    FONT_SIZE,fnt_siz,
	    0);

	xv_set(frame,XV_FONT,font,0);

	gcv.font = (Font)xv_get(font,XV_XID);

	if(gcv.font != 0)
		XChangeGC(dpy,gc,GCFont,&gcv);

	for(j=0;j<16;j++)
	{
		grey_pm[j] = XCreatePixmapFromBitmapData(dpy,
		    DefaultRootWindow(dpy), greys[j], grey_width,
			grey_height, 1, 0, 1);
	}

	return;
}


/*
SetNADData()
Fills in the display data structure for an initial monochrome display.
All settings are simple defaults, and will need to be modified externally
if otherwise.  This routine passes back a new NA_DisplayData structure, which
can be destroyed after use with a call to cfree().

Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.
*/

NA_DisplayData *SetNADData(aln,Can,NamCan)
NA_Alignment *aln;
Canvas Can,NamCan;
{
	NA_DisplayData *ddata;
	Scrollbar hscroll,vscroll;
	Xv_window view;
	int j;

	extern Frame frame;
/* 	extern int Default_Color_LKUP[]; */
/* 	extern DisplayAttr; */

	int reset_all;

	if(aln->na_ddata == NULL)
	{
		ddata = (NA_DisplayData*)Calloc(1,sizeof(NA_DisplayData));
		reset_all = TRUE;
	}
	else
	{
		ddata =(NA_DisplayData*)(aln->na_ddata);
		reset_all = FALSE;
	}

	ddata -> font = (Xv_font)xv_get(frame,XV_FONT);
	ddata -> font_dx = xv_get(ddata->font,FONT_DEFAULT_CHAR_WIDTH);
	ddata -> font_dy = xv_get(ddata->font,FONT_DEFAULT_CHAR_HEIGHT);
	if(reset_all)
	{
		ddata -> wid = 0;
		ddata -> ht = 0;
		ddata -> position = 0;
		ddata -> depth = xv_get(frame,WIN_DEPTH);
		if(ddata -> depth >= 4)
		{
			ddata -> color_type = COLOR_LOOKUP;
			ddata -> num_colors = 16;
			ddata -> white = 15;
			ddata -> black = 8;
		}
		else
		{
			ddata -> color_type = COLOR_MONO;;
			ddata -> num_colors = 2;
			ddata -> white = 0;
			ddata -> black = 1;
		}
		ddata -> jtsize = 0;
		ddata -> aln = aln;
		ddata -> seq_x = xv_get(Can,XV_XID);
		ddata -> nam_x = xv_get(NamCan, XV_XID);
		ddata -> use_repeat = TRUE;
	}
	ddata -> seq_can = Can;
	ddata -> nam_can = NamCan;

	for(j=0;j<xv_get(Can,OPENWIN_NVIEWS);j++)
	{
		view = (Xv_window)xv_get(Can,OPENWIN_NTH_VIEW,j,0);

		hscroll = (Scrollbar)xv_get(Can,
		    OPENWIN_HORIZONTAL_SCROLLBAR,view);
		vscroll = (Scrollbar)xv_get(Can,
		    OPENWIN_VERTICAL_SCROLLBAR,view);

		if(hscroll && vscroll)
		{
			xv_set(hscroll,SCROLLBAR_PIXELS_PER_UNIT,
			    ddata->font_dx,0);
			xv_set(vscroll,SCROLLBAR_PIXELS_PER_UNIT,
			    ddata->font_dy,0);
		}

		/*
*	Set the length and height of the alignment
*/
		xv_set(hscroll,SCROLLBAR_OBJECT_LENGTH,aln->maxlen,0);
		xv_set(vscroll,SCROLLBAR_OBJECT_LENGTH,aln->numelements,0);

		scrollbar_paint(vscroll);
		scrollbar_paint(hscroll);
	}

	if(aln->numelements !=0)
	{
		xv_set(Can,
		    WIN_HEIGHT,MIN(MAX_STARTUP_CANVAS_HEIGHT,
		    ddata->font_dy * (aln->numelements+2)),
		    CANVAS_RETAINED,FALSE,
		    0);

		xv_set(NamCan,
		    WIN_HEIGHT,MIN(MAX_STARTUP_CANVAS_HEIGHT,
		    ddata->font_dy * (aln->numelements+2)),
		    0);
	}

	(void)window_fit(NamCan);
	(void)window_fit(Can);
	(void)window_fit(frame);

	return (ddata);
}


int DummyRepaint(can,win,dpy,xwin,area)
     Canvas        can;
     Xv_window     win;
     Display      *dpy;
     Window        xwin;
     Xv_xrectlist *area;
{
	DrawNANames(dpy,xwin);
	return XV_OK;
}

int DrawNANames(dpy,xwin)
     Display *dpy;
     Window   xwin;
{
	extern NA_Alignment *DataSet;
	extern Canvas EditCan,EditNameCan;
	NA_DisplayData *NAdd;
	NA_Alignment *aln;
	NA_Sequence *element;
	int maxseq,minseq,maxnoseq,/*i,*/j;
	unsigned long *pixels;
	char buffer[GBUFSIZ];
	int scrn = DefaultScreen(dpy);
	GC gc;

	aln = DataSet;
	if(DataSet == NULL)
		return XV_OK;
	NAdd = (NA_DisplayData*)(DataSet)->na_ddata;
	gc = DefaultGC(dpy,DefaultScreen(dpy));

	pixels = (unsigned long*)xv_get(EditCan,WIN_X_COLOR_INDICES);
	XSetBackground(dpy,gc,WhitePixel(dpy,scrn));
	XSetForeground(dpy,gc,BlackPixel(dpy,scrn));

	minseq = NAdd->top_seq;
	maxseq = minseq + NAdd->ht;
	maxseq = MIN(maxseq+1,aln->numelements);

	for(j=minseq;j<maxseq;j++)
	{
		element = &(aln->element[j]);
		if(element->groupid != 0)
			sprintf(buffer,"%d %s                          ",
			    element->groupid,element->short_name);
		else
			sprintf(buffer,"%s                              ",
			    element->short_name);

		if(aln->element[j].selected)
		{
			XSetForeground(dpy,gc,WhitePixel(dpy,scrn));
			XSetBackground(dpy,gc,BlackPixel(dpy,scrn));
		}

		XDrawImageString(dpy,xwin,gc,5,
		    NAdd->font_dy*(j-minseq+1),buffer,40);

		if(aln->element[j].selected)
		{
			XSetForeground(dpy,gc,BlackPixel(dpy,scrn));
			XSetBackground(dpy,gc,WhitePixel(dpy,scrn));
		}
	}
	maxnoseq = xv_get(EditNameCan,XV_HEIGHT)/NAdd->font_dy;
	for(j=maxseq;j<maxnoseq;j++)
		XDrawImageString(dpy,xwin,gc,5,
		    NAdd->font_dy*(j-minseq+1),
		    "                                ",40);
	return XV_OK;
}


int RepaintNACan(can,win,dpy,xwin,area)
     Canvas        can;
     Xv_window     win;
     Display      *dpy;
     Window        xwin;
     Xv_xrectlist *area;
{
	extern NA_Alignment *DataSet;
	extern Canvas EditCan,EditNameCan;
	extern int SCALE;
	Scrollbar hscroll,vscroll;
	NA_DisplayData *NAdd;
	Xv_window view;
	int maxseq,minseq,i,j,lpos,rpos,nviews;
	int start,end,top,bottom;
	GC gc;

	int scrn = DefaultScreen(dpy);
	gc = DefaultGC(dpy,scrn);

	if(DataSet == 0 || can == 0)
        return XV_OK;

	NAdd = (NA_DisplayData*)(DataSet)->na_ddata;
	if(NAdd == NULL)
        return XV_OK;
	for(;xv_get(can,CANVAS_RETAINED)==TRUE;)
		xv_set(can,CANVAS_RETAINED,FALSE,0);

	XSetForeground(dpy,gc,BlackPixel(dpy,scrn));
	XSetBackground(dpy,gc,WhitePixel(dpy,scrn));

	nviews = (int)xv_get(EditCan,OPENWIN_NVIEWS);
	for(j=0;j<nviews;j++)
	{
		view = (Xv_window)xv_get(EditCan,OPENWIN_NTH_VIEW,j);
		if(view)
			if(xv_get(view,CANVAS_VIEW_PAINT_WINDOW) == win)
				j=nviews;
	}

	/*
	added to remove warnings on split screen
*/
	hscroll = (Scrollbar)xv_get(EditCan,OPENWIN_HORIZONTAL_SCROLLBAR,view);
	vscroll = (Scrollbar)xv_get(EditCan,OPENWIN_VERTICAL_SCROLLBAR,view);

	if(vscroll)
	{
		xv_set(vscroll,SCROLLBAR_OBJECT_LENGTH,
		    (DataSet)-> numelements,0);
		minseq = (int)xv_get(vscroll,SCROLLBAR_VIEW_START);
		maxseq = (int)xv_get(vscroll,SCROLLBAR_VIEW_LENGTH);

		if( NAdd->top_seq != minseq || NAdd->ht != maxseq)
		{
			NAdd->top_seq = minseq;
			NAdd->ht = maxseq;
			DrawNANames(dpy,xv_get(canvas_paint_window(EditNameCan),
			    XV_XID));
		}

		maxseq += minseq;
		maxseq = MIN(maxseq+1,DataSet->numelements);

		top =(int)xv_get(vscroll,SCROLLBAR_VIEW_START);
		bottom = top +(int)xv_get(vscroll,SCROLLBAR_VIEW_LENGTH);
		for(;bottom-top>MAX_NA_DISPLAY_HEIGHT;)
		{
			top =(int)xv_get(vscroll,SCROLLBAR_VIEW_START);
			bottom= top +(int)xv_get(vscroll,SCROLLBAR_VIEW_LENGTH);
		}
	}

	if(hscroll)
	{
		xv_set(hscroll,SCROLLBAR_OBJECT_LENGTH, (DataSet)->maxlen,0);
		start =(int)xv_get(hscroll,SCROLLBAR_VIEW_START);
		end = start +(int)xv_get(hscroll,SCROLLBAR_VIEW_LENGTH);
		for(;end-start>MAX_NA_DISPLAY_WIDTH;)
		{
			start =(int)xv_get(hscroll,SCROLLBAR_VIEW_START);
			end = start +(int)xv_get(hscroll,SCROLLBAR_VIEW_LENGTH);
		}
	}


	for(i=0;(i<area->count) && hscroll && vscroll;i++)
	{
		lpos = start+((int)area->rect_array[i].x/NAdd->font_dx)*SCALE;
		rpos = (((int)area->rect_array[i].width/NAdd->font_dx)*SCALE +
		    lpos);

/*
		rpos = MIN(NAdd->aln->maxlen,rpos + 1);
*/
		rpos += 1;

		minseq = top+(int)area->rect_array[i].y/NAdd->font_dy;
		maxseq = (int)area->rect_array[i].height/NAdd->font_dy+minseq;
		maxseq = MIN(DataSet->numelements-1,maxseq+1);

		/*
		for(;rpos-lpos>MAX_NA_DISPLAY_WIDTH;)
        	{
                	lpos =(int)xv_get(hscroll,SCROLLBAR_VIEW_START)/SCALE;
                	rpos = lpos+(int)xv_get(hscroll,SCROLLBAR_VIEW_LENGTH)*SCALE;
        	}
*/

		for(j=minseq;j<=maxseq;j++)
			DrawNAColor(can,NAdd,xwin,start,top,j,lpos,rpos,dpy,gc,
			    NAdd->color_type,FALSE);
	}
	SetNACursor(NAdd,can,win,xwin,dpy,gc);
	(void)window_fit(EditCan);
	(void)window_fit(EditNameCan);
	(void)window_fit(frame);
	return XV_OK;
}

	extern NA_Alignment *DataSet;

void SetNACursor(NAdd,can,win,xwin,dpy,gc)
     NA_DisplayData *NAdd;
     Canvas          can;
     Xv_window       win;
     Window          xwin;
     Display        *dpy;
     GC              gc;
{
	extern int repeat_cnt,EditMode/*,SCALE*/;
	extern Panel_item left_foot/*,right_foot*/;
	extern Frame frame;

	Scrollbar hscroll,vscroll;
	NA_Sequence *this_elem;

	int xx,yy,j,dir=0,SubSel = FALSE;
	Xv_window view;

	char buffer[GBUFSIZ];
	int x = ((NA_DisplayData*)(DataSet)->
	    na_ddata)->cursor_x;
	int y = ((NA_DisplayData*)(DataSet)->
	    na_ddata)->cursor_y;
	int position = ((NA_DisplayData*)(DataSet)->
	    na_ddata)->position;

	this_elem = &(DataSet->element[y]);
	dir = OrigDir(this_elem);

	if(repeat_cnt > 0)
		sprintf(buffer,"[%s] pos:%d col:%d   %s %s (repeat:%d)",
		    EditMode==0?"Insert": "Check", position,((NA_DisplayData*)(
		    DataSet)->na_ddata)->cursor_x+1+DataSet->rel_offset,(DataSet)->
		    element[y].short_name,(dir == 1)?"	-->":
		    (dir == -1)?"	<--":" ",MAX(repeat_cnt,1));
	else
		sprintf(buffer,"[%s] pos:%d col:%d   %s %s",
		    EditMode==0?"Insert": "Check",position,((NA_DisplayData*)(
		    DataSet)->na_ddata)->cursor_x+1+DataSet->rel_offset,(DataSet)->
		    element[y].short_name,(dir == 1)?"	-->":
		    (dir == -1)?"	<--":" ");

	xv_set(frame,FRAME_LEFT_FOOTER,buffer,0);
	xv_set(left_foot,PANEL_LABEL_STRING,buffer,0);

	for(j=0;j<DataSet->numelements;j++)
		if(DataSet->element[j].subselected)
			SubSel = TRUE;

	for(j=0;j<xv_get(can,OPENWIN_NVIEWS) && !SubSel;j++)
	{
		view = xv_get(can,OPENWIN_NTH_VIEW,j);
		hscroll=(Scrollbar)xv_get(can,OPENWIN_HORIZONTAL_SCROLLBAR,view);
		vscroll=(Scrollbar)xv_get(can,OPENWIN_VERTICAL_SCROLLBAR,view);
		if(hscroll && vscroll)
		{
			yy = xv_get(vscroll,SCROLLBAR_VIEW_START);
			xx = xv_get(hscroll,SCROLLBAR_VIEW_START);
			xwin = (Window)xv_get
			    (xv_get(view,CANVAS_VIEW_PAINT_WINDOW), XV_XID);
			DrawNAColor(can,NAdd,xwin,xx,yy,y,x,x,dpy,gc,COLOR_MONO,
			    TRUE);
		}
	}
	return;
}


void UnsetNACursor(NAdd,can,win,xwin,dpy,gc)
     NA_DisplayData *NAdd;
     Canvas          can;
     Xv_window       win;
     Window          xwin;
     Display        *dpy;
     GC              gc;
{
/* 	NA_DisplayData *ddata; */
	NA_Alignment *aln;
	Scrollbar vscroll=0,hscroll=0;
	Xv_window view;
/* 	extern int SCALE; */
	int x,y,xx,yy,j;

	if(DataSet == NULL)
		return;

	aln = DataSet;
	x = ((NA_DisplayData*)(DataSet)->na_ddata)->cursor_x;
	y = ((NA_DisplayData*)(DataSet)->na_ddata)->cursor_y;

	for(j=0;j<xv_get(can,OPENWIN_NVIEWS);j++)
	{
		view = xv_get(can,OPENWIN_NTH_VIEW,j);
		hscroll=(Scrollbar)xv_get(can,
		    OPENWIN_HORIZONTAL_SCROLLBAR,view);
		vscroll=(Scrollbar)xv_get(can,
		    OPENWIN_VERTICAL_SCROLLBAR,view);

		yy = xv_get(vscroll,SCROLLBAR_VIEW_START);
		xx = xv_get(hscroll,SCROLLBAR_VIEW_START);
		xwin = (Window)xv_get(xv_get(view,CANVAS_VIEW_PAINT_WINDOW)
		    ,XV_XID);

		DrawNAColor(can,NAdd,xwin,xx,yy,y,x,x,dpy,gc,NAdd->color_type,
		    FALSE);
	}
	return;
}



int ResizeNACan(canvas,wd,ht)
     Canvas canvas;
     int    wd,ht;
{
	int dy;
	if(DataSet == NULL)
		return(XV_OK);
	if(DataSet->na_ddata == NULL)
		return(XV_OK);

	dy = (int)((NA_DisplayData*)(DataSet->na_ddata))->font_dy;
	if(ht > dy * (DataSet->numelements+2))
	{
		xv_set(canvas,XV_HEIGHT,dy * (DataSet->numelements+2),0);
	}
	return(XV_OK);
}

