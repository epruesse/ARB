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
#include "menudefs.h"
#include "defines.h"

/*
Copyright (c) 1989, University of Illinois board of trustees.  All rights
reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/

InitEditSplit(oldview, newview, pos)
Xv_Window oldview, newview;
int   pos;
{
	Xv_Window view, win;
	extern Frame frame;
	extern  NA_Alignment *DataSet;
	extern Canvas EditCan;
	extern int SCALE;
	Scrollbar hsc,vsc;
	int j;

	if(DataSet == NULL || EditCan == NULL)
		return ;

	for(j=0;j<xv_get(EditCan,OPENWIN_NVIEWS);j++)
	{
		view = (Xv_window)xv_get(EditCan,OPENWIN_NTH_VIEW,j);

		hsc = (Scrollbar)xv_get(EditCan,
		OPENWIN_HORIZONTAL_SCROLLBAR,view);

		vsc = (Scrollbar)xv_get(EditCan,
		OPENWIN_VERTICAL_SCROLLBAR,view);

		xv_set(hsc,
		SCROLLBAR_VIEW_START,0,
		SCROLLBAR_OBJECT_LENGTH,((NA_Alignment*)DataSet)->
		maxlen,0);

		xv_set(vsc,
		SCROLLBAR_VIEW_START,0,
		SCROLLBAR_OBJECT_LENGTH,((NA_Alignment*)DataSet)->
		numelements,0);

		if (view == newview)
		{
/*
*	Get the paint window associated, and set it up the same as in
*	BasicDisplay:
*/

			(void)xv_set(xv_get(view,CANVAS_VIEW_PAINT_WINDOW),
			WIN_EVENT_PROC,NAEvents,
			WIN_CONSUME_EVENTS, WIN_MOUSE_BUTTONS,
			LOC_DRAG, LOC_WINENTER, WIN_ASCII_EVENTS,
			WIN_META_EVENTS, 0,
			0);

			notify_interpose_event_func(
			xv_get(hsc,SCROLLBAR_NOTIFY_CLIENT),
			EditCanScroll,NOTIFY_SAFE);

                        xv_set(hsc,
                        SCROLLBAR_OBJECT_LENGTH,((NA_Alignment*)DataSet)->
                        maxlen,SCROLLBAR_VIEW_START,0,
                        0);

                        xv_set(vsc,
                        SCROLLBAR_OBJECT_LENGTH,((NA_Alignment*)DataSet)->
                        numelements,0);
		}
	}
	RepaintAll(FALSE);
	return;
}


Notify_value EditCanScroll(client,event,arg,type)
Notify_client  client;
Event   *event;
Notify_arg      arg;
Notify_event_type       type;
{
	extern  NA_Alignment *DataSet;
	extern Canvas EditCan;
	extern Panel_item left_foot,right_foot;
	extern int DisplayAttr,SCALE;

	Notify_client  parent;
	Drawable draw;
	GC gc;
	Display *dpy;
	Xv_xrectlist area;

	Xv_window win,view;
	Scrollbar hsc,vsc;
	extern Frame frame;
	int lastx,currentx,deltax,j;
	int lasty,currenty,deltay;
	int dx,dy;
	char buffer[80];

	hsc=(Scrollbar)xv_get(EditCan,OPENWIN_HORIZONTAL_SCROLLBAR, client);
	vsc=(Scrollbar)xv_get(EditCan,OPENWIN_VERTICAL_SCROLLBAR, client);
/*
	test for hsc && vsc attempts to fix warnings at split
*/

	if(event_id(event) == SCROLLBAR_REQUEST && hsc && vsc)
	{
		win=(Xv_window)xv_get(client,
		CANVAS_VIEW_PAINT_WINDOW);

		dx=((NA_DisplayData*)(((NA_Alignment*)DataSet)->
		na_ddata))-> font_dx;

		dy=((NA_DisplayData*)(((NA_Alignment*)DataSet)->
		na_ddata))-> font_dy;

		lastx=(int)xv_get(hsc,
		SCROLLBAR_LAST_VIEW_START);

		currentx=(int)xv_get(hsc,SCROLLBAR_VIEW_START)/SCALE;
		deltax=(int)xv_get(hsc,SCROLLBAR_VIEW_LENGTH);

		lasty=(int)xv_get(vsc,
		SCROLLBAR_LAST_VIEW_START);

		currenty=(int)xv_get(vsc,SCROLLBAR_VIEW_START);
		deltay=(int)xv_get(vsc,SCROLLBAR_VIEW_LENGTH);

		area.count=1;
		area.rect_array[0].x=0;
		area.rect_array[0].y=0;
		area.rect_array[0].width=(short)(deltax*dx);
		area.rect_array[0].height=(short)(deltay*dy);

		RepaintNACan(EditCan,win,xv_get(client,
		XV_DISPLAY),
		(Window)xv_get(win,XV_XID),&area);

		sprintf(buffer,"Columns %d - %d shown",currentx,
		currentx+deltax*SCALE);
		if(DisplayAttr & VSCROLL_LOCK)
		{
			DisplayAttr &= (unsigned int)(255 - VSCROLL_LOCK);
			for(j=0;j<xv_get(EditCan,OPENWIN_NVIEWS);j++)
			{
				view = xv_get(EditCan,OPENWIN_NTH_VIEW,j);
				if(view != client)
				{
				  if(view)
				  {
				    vsc=(Scrollbar)xv_get(EditCan,
				    OPENWIN_VERTICAL_SCROLLBAR, view);
				    if(vsc)
				  	xv_set(vsc,SCROLLBAR_VIEW_START,
					currenty, SCROLLBAR_VIEW_LENGTH,deltay,
					0);
				  }
				}
			}
			DisplayAttr |= VSCROLL_LOCK;
		}
		xv_set(frame,FRAME_RIGHT_FOOTER,buffer,0);
		xv_set(right_foot,PANEL_LABEL_STRING,buffer,0);
		return(XV_OK);
	}

/*
	test for hsc && vsc attempts to fix warnings at split
*/
	else if ((event_action(event) == ACTION_SPLIT_HORIZONTAL ||
	event_action(event) == ACTION_SPLIT_VERTICAL ) &&
	hsc && vsc)
	{
		xv_set(hsc,SCROLLBAR_VIEW_START,0,0);
		xv_set(vsc,SCROLLBAR_VIEW_START,0,0);
		return(notify_next_event_func(client,event,arg,type));
	}
	else
		return(notify_next_event_func(client,event,arg,type));
}



void JumpTo(view,x,y)
Xv_window view;
int x,y;
{
	extern  NA_Alignment *DataSet;
	extern Canvas EditCan;
	Scrollbar hsc,vsc;
	int j,dx,dy;
	Xv_xrectlist area;
	Xv_window win;

	hsc = (Scrollbar)xv_get(EditCan,OPENWIN_HORIZONTAL_SCROLLBAR,view);
	vsc = (Scrollbar)xv_get(EditCan,OPENWIN_VERTICAL_SCROLLBAR,view);
	win = (Xv_window)xv_get(view,CANVAS_VIEW_PAINT_WINDOW);

	(void)xv_set(hsc,SCROLLBAR_VIEW_START,MIN(x,
	xv_get(hsc,SCROLLBAR_OBJECT_LENGTH)-xv_get(hsc,SCROLLBAR_VIEW_LENGTH)),
	0);
	(void)xv_set(vsc,SCROLLBAR_VIEW_START,MIN(y,
	xv_get(vsc,SCROLLBAR_OBJECT_LENGTH)-xv_get(vsc,SCROLLBAR_VIEW_LENGTH)),
	0);
	(void)xv_set(hsc,SCROLLBAR_OBJECT_LENGTH,DataSet->maxlen,0);
	(void)xv_set(vsc,SCROLLBAR_OBJECT_LENGTH,DataSet->numelements,0);

	return;
}


RepaintAll(Names)
int Names;
{
	extern  NA_Alignment *DataSet;
	extern Canvas EditCan,EditNameCan;
	Xv_xrectlist area;
	Xv_window win,view;
	Scrollbar hsc,vsc;
	extern int SCALE;
	extern Frame frame;
	int lastx,currentx,deltax;
	int lasty,currenty,deltay;
	int dx,dy,j;
	char buffer[80];

	if(DataSet == NULL)
		return;

	if((NA_DisplayData*)(((NA_Alignment*)DataSet)->na_ddata == NULL))
		return;

	for(j=0;j<xv_get(EditCan,OPENWIN_NVIEWS);j++)
	{
		view = (Xv_window)xv_get(EditCan,OPENWIN_NTH_VIEW,j);
		win = (Xv_window)xv_get(view,CANVAS_VIEW_PAINT_WINDOW);
		dx = ((NA_DisplayData*)(((NA_Alignment*)DataSet)->na_ddata))->
		font_dx ;
		dy = ((NA_DisplayData*)(((NA_Alignment*)DataSet)->na_ddata))->
		font_dy;
		hsc=(Scrollbar)xv_get(EditCan,OPENWIN_HORIZONTAL_SCROLLBAR,
		view);
		vsc=(Scrollbar)xv_get(EditCan,OPENWIN_VERTICAL_SCROLLBAR, view);

		lastx = (int)xv_get(hsc,SCROLLBAR_LAST_VIEW_START);
		currentx = (int)xv_get(hsc,SCROLLBAR_VIEW_START);
		deltax = (int)xv_get(hsc,SCROLLBAR_VIEW_LENGTH);

		lasty = (int)xv_get(vsc,SCROLLBAR_LAST_VIEW_START);
		currenty = (int)xv_get(vsc,SCROLLBAR_VIEW_START);
		deltay = (int)xv_get(vsc,SCROLLBAR_VIEW_LENGTH);

		area.count = 1;
		area.rect_array[0].x = 0;
		area.rect_array[0].y = 0;
		area.rect_array[0].width = (short)(deltax*dx);
		area.rect_array[0].height = (short)(deltay*dy);

		RepaintNACan(EditCan,win,xv_get(view, XV_DISPLAY),
		(Window)xv_get(win,XV_XID),&area);

		sprintf(buffer,"%d - %d",currentx/SCALE,
			currentx/SCALE+deltax*SCALE);
		xv_set(frame,FRAME_RIGHT_FOOTER,buffer,0);
	}
	if(Names)
		DrawNANames(xv_get(view, XV_DISPLAY),
			(Window)xv_get(xv_get(EditNameCan,
			CANVAS_NTH_PAINT_WINDOW,0), XV_XID));
	return;
}

DestroySplit(view)
Xv_window view;
{}
