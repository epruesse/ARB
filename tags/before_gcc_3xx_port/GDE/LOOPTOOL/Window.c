#include <stdio.h>
#include <errno.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/cursor.h>
#include <xview/scrollbar.h>
#include <xview/panel.h>
#include <string.h>
/* #include <malloc.h> */
#include <math.h>
#include "loop.h"
#include "globals.h"
#include <xview/cms.h>


Xv_singlecolor Default_Colors[16]= {
        {0,128,0},
        {255,192,0},
        {255,0,255},
        {225,0,0},
        {0,192,192},
        {0,192,0},
        {0,0,255},
        {128,0,255},
        {0,0,0},
        {36,36,36},
        {72,72,72},
        {109,109,109},
        {145,145,145},
        {182,182,182},
        {218,218,218},
        {255,255,255}
};

int COLOR;

/*Window.c
*	Written by Steven Smith, Copyright 1989, U of Illinois
*	Microbiology/CGPA.
*/

/*
*	WindowSetup: Set up windows, menus, dialogs etc.  No
*	parameters passed in, only globals passed out.  Current
*	palette set to 8 simple colors.
*/

WindowSetup()
{
	extern EditProc(),FileProc(),StyleProc(),EventHandler(),SetDepth();
	extern SetFile(),ShoCon(),xv_pf_open();
	extern void ReDraw();
	Cms colmap;

	Panel menubar,fdlg,pdlg;
	int i,j;

	unsigned char red[8],green[8],blue[8];

/*
*	Open font for screen operations.
*/
	page=xv_create(NULL,FRAME,
		FRAME_LABEL,    "LoopTool",
		WIN_INHERIT_COLORS,TRUE,
		0);

/*
*	Dialogs...
*/
	file_dialog=xv_create(page,FRAME,0);
	print_dialog=xv_create(page,FRAME,0);

        fdlg=xv_create(file_dialog,PANEL,
                XV_HEIGHT,150,
		XV_WIDTH, 200,
                PANEL_LAYOUT,PANEL_HORIZONTAL,
                0);


	xv_create(fdlg,PANEL_BUTTON,
                PANEL_LABEL_STRING,"Ok",
		PANEL_NOTIFY_PROC,SetFile,
		0);

        xv_create(fdlg,PANEL_TEXT,
		PANEL_LABEL_STRING,"Data File:",
		PANEL_VALUE,"",
                PANEL_NOTIFY_PROC,SetFile,
                0);


/*
*	Set menu items.
*/
	File=xv_create(NULL,MENU,
		MENU_NOTIFY_PROC,FileProc,
		MENU_STRINGS,"Save","Print","Quit",0,0);

	Edit=xv_create(NULL,MENU,
		MENU_NOTIFY_PROC,EditProc,
		MENU_STRINGS,"Clear constraint",
		"Show Constraints", "Distance Constraint",
		"Position Constraint",
		"Invert Helix","Stack Helix" ,0,0);

/*
	Style=xv_create(NULL,MENU,
		MENU_NOTIFY_PROC,StyleProc,
		MENU_STRINGS,"Font","Size","Grey level","Label",0,0);
*/

	menubar=xv_create(page,PANEL,
		XV_HEIGHT,50,
		0);

/*
*	Menu items.
*/
	xv_create(menubar,PANEL_BUTTON,
        	PANEL_LABEL_STRING,"File",
		PANEL_ITEM_MENU,File,
        	0);

	xv_create(menubar,PANEL_BUTTON,
        	PANEL_LABEL_STRING,"Edit",
		PANEL_ITEM_MENU,Edit,
        	0);

/*
	xv_create(menubar,PANEL_BUTTON,
        	PANEL_LABEL_STRING,"Style",
		PANEL_ITEM_MENU,Style,
        	0);
*/

	xv_create(menubar,PANEL_SLIDER,
        	PANEL_LABEL_STRING,"View Depth:",
        	PANEL_VALUE,100,
        	PANEL_MIN_VALUE,0,
        	PANEL_MAX_VALUE,100,
        	PANEL_SLIDER_WIDTH,60,
        	PANEL_SHOW_RANGE,TRUE,
        	PANEL_SHOW_VALUE,TRUE,
        	PANEL_NOTIFY_PROC,SetDepth,
        	0);

	window_fit_height(menubar);

/*
*	Pagecan is the canvas used for the primary display.
*/
	pagecan=xv_create(page,CANVAS,
		WIN_INHERIT_COLORS,TRUE,
		WIN_DYNAMIC_VISUAL,TRUE,
		WIN_BELOW,menubar,
/*
        	CANVAS_WIDTH,WINDOW_SIZE,
        	CANVAS_HEIGHT,WINDOW_SIZE,
*/
        	OPENWIN_AUTO_CLEAR,TRUE,
        	CANVAS_AUTO_SHRINK,TRUE,
        	CANVAS_AUTO_EXPAND,TRUE,
        	CANVAS_REPAINT_PROC,ReDraw,
	        0);

/*
	xv_create(pagecan,SCROLLBAR,
                SCROLLBAR_DIRECTION,SCROLLBAR_VERTICAL,
                SCROLLBAR_SPLITTABLE,TRUE,
                SCROLLBAR_OVERSCROLL,0,
                0);

	xv_create(pagecan,SCROLLBAR,
                SCROLLBAR_DIRECTION,SCROLLBAR_HORIZONTAL,
                SCROLLBAR_SPLITTABLE,TRUE,
                SCROLLBAR_OVERSCROLL,0,
                0);
*/

	xv_set(canvas_paint_window(pagecan),
        	WIN_EVENT_PROC,EventHandler,
                WIN_CONSUME_EVENTS,
                        WIN_MOUSE_BUTTONS,
			WIN_RESIZE,
                        LOC_DRAG,
                        LOC_WINENTER,
                        WIN_ASCII_EVENTS,
                        WIN_META_EVENTS,
                        0,
                0);

/*
* Set palette
*/

        if( xv_get(page,WIN_DEPTH)>4)
	{
		colmap = (Cms)xv_find(page,CMS,
                        CMS_NAME,"GDE Palette",
                        XV_AUTO_CREATE,FALSE,
                        0);


 	       if(colmap == NULL)
        	        colmap = (Cms)xv_create(NULL,CMS,
                        CMS_TYPE,XV_DYNAMIC_CMS,
                        CMS_SIZE,16,
                        CMS_COLORS,Default_Colors,
                        0);

 	       xv_set(canvas_paint_window(pagecan),
        	WIN_CMS_NAME,"GDE Palette",
 	        WIN_CMS, colmap,
       		WIN_FOREGROUND_COLOR,8,
 	        WIN_BACKGROUND_COLOR,15,
 	        WIN_INHERIT_COLORS,FALSE,
 	        0);
		COLOR = TRUE;
	}
	else
		COLOR = FALSE;

	window_fit_width(page);

	fdlg=xv_create(file_dialog,PANEL,
                XV_WIDTH,300,
                XV_HEIGHT,200,
                PANEL_LAYOUT,PANEL_HORIZONTAL,
                0);

	pdlg=xv_create(print_dialog,PANEL,
                XV_WIDTH,300,
                XV_HEIGHT,200,
                PANEL_LAYOUT,PANEL_HORIZONTAL,
                0);

        window_main_loop(page);
	return;
}



/*
*	ReDraw:  Redrawing procedure.  Two primary states are handled.
*	If modified is false, then the bases are laid using their current
*	coordinates.  If modified is true, then the structure is recalculated.
*/

void ReDraw(canvas,pw,repaint_area)
	Canvas canvas;
	Pixwin *pw;
	Rectlist *repaint_area;
{
	extern DrawPair();
	Rect rect;
	int nucpos,i,size,ccw,imax;
	LoopStak lstack[10000];
	extern PromptSeq(),BuildLoopStk(),Translate();
	extern ReadSeqSet(),gprint(),Arc();
	errno = 0;

/*
*	if no file was defined...
*/
	if(infile==NULL)
	{
		PromptFile();
		ReadSeqSet(&dataset);
	}
/*
*	if no sequence was picked...
*/

	if (seqnum== -1)
	{
		PromptSeq(infile,&dataset,&seqnum);
		Translate(&dataset,seqnum,&baselist,&seqlen);
		if(tempfile)
			ImposeTemp();
	}
/*
*	if the constraints have been modified, then recalculate.
*/
	if(modified)
	{
/*
*	set all base positions to an unknown state.
*/
		modified=FALSE;
		for(nucpos=1;nucpos<seqlen-1;nucpos++)
			baselist[nucpos].known=FALSE;

/*
*	except for the first and last.
*/
		baselist[0].known=TRUE;
		baselist[seqlen-1].known=TRUE;
		if(baselist[0].posnum>0)
			PosConFix(baselist[0]);

/*
*	for all bases...
*/
		for(nucpos=0;nucpos<seqlen-1;nucpos++)
		{
			redo=FALSE;
/*
*	If the current base is unknown,back up and flag that it was
*	not known.
*/
			for(;baselist[nucpos].known==FALSE;nucpos--)
			{
				redo=TRUE;
			}

/*
*	If the next base is not known, then begin a loop.
*/
			if(baselist[nucpos+1].known==FALSE)
			{
				size=BuildLoopStk(lstack,nucpos,baselist,&ccw,
				&imax);
/*
*	If the loop is more than two bases, then lay the arc out.
*/
				if(size>2)
					Arc(lstack,size,baselist,ccw,imax);
			}
		}
/*
*	 Find min and max positions.
*/

		xmax = -99999.99;
		ymax = -99999.99;
		xmin = 99999.99;
		ymin = 99999.99;

		for(nucpos=0;nucpos<seqlen;nucpos++)
		{
			xmax=Max(xmax,baselist[nucpos].x);
			xmin=Min(xmin,baselist[nucpos].x);
			ymax=Max(ymax,baselist[nucpos].y);
			ymin=Min(ymin,baselist[nucpos].y);
		}
/*
*       Scale all points so that ther fall on the page.
*/
		WINDOW_SIZE = Min(xv_get(canvas_paint_window(pagecan),XV_HEIGHT),
		xv_get(canvas_paint_window(pagecan),XV_WIDTH));
		xscale=(double)(xv_get(canvas_paint_window(pagecan),XV_WIDTH)-30) / (xmax-xmin);
		yscale=(double)(xv_get(canvas_paint_window(pagecan),XV_HEIGHT)-30) / (ymax-ymin);

		if(xscale<yscale)
			yscale=xscale;
		else
			xscale=yscale;

		xoffset=(-xmin)*xscale+20;
		yoffset=(-ymin)*yscale+20;

/*
*	Clear the page..
*/
		if(COLOR)
			pw_writebackground(canvas_paint_window(pagecan),0,0,
			xv_get(canvas_paint_window(pagecan),XV_WIDTH),
			xv_get(canvas_paint_window(pagecan),XV_HEIGHT),
			PIX_SRC | (15 <<5));
		else
			pw_writebackground(canvas_paint_window(pagecan),0,0,
			xv_get(canvas_paint_window(pagecan),XV_WIDTH),
			xv_get(canvas_paint_window(pagecan),XV_HEIGHT),
			PIX_SRC );
	}
	WINDOW_SIZE = Min(xv_get(canvas_paint_window(pagecan),XV_HEIGHT),
	xv_get(canvas_paint_window(pagecan),XV_WIDTH));
	rect_construct(&rect,0,0,xv_get(canvas_paint_window(pagecan),XV_WIDTH),xv_get(canvas_paint_window(pagecan),XV_HEIGHT));
/*
*	Write all bases and symbols on the page.
*/
	for(nucpos=0;nucpos<seqlen;nucpos++)
	{
		if(baselist[nucpos].depth <ddepth)
		{
			gprint(baselist[nucpos],pw);
			if(baselist[nucpos].pair > nucpos)
				DrawPair(baselist[nucpos],
				baselist[baselist[nucpos].pair],pw);
			if(baselist[nucpos].label != NULL)
				PlaceLabel(baselist[nucpos].label,nucpos,pw);
		}

	}
	if(sho_con)
		ShoCon();
	return;
}


PromptSeq(infile,dset,seqnum)
FILE *infile;
DataSet *dset;
int *seqnum;
{
	*seqnum=0;
}


/*
*	BuildLoopStack:  Build a stack of bases that make a subloop of
*	the structure.  Unknown bases are pushed on the stack until a known
*	base is encountered.  Base to base distances are also pushed upon
*	the stack for the arc subroutine.
*/
BuildLoopStk(stk,nuc,blist,ccw,imax)
LoopStak stk[];
int nuc,*imax;
Base *blist;
int *ccw;
{
	int stkp,current,cw_cnt=0,ccw_cnt=0;
	double dist,dmax=0.0;
	Base *here,*there;

/*
*	Push the first base on...
*/
	stk[stkp=0].nucnum=nuc;
	stk[stkp++].dist=0.0;


	current=NextBase(nuc,blist,&dist);
	for(;blist[current].known == FALSE;
		current=NextBase(current,blist,&dist))
	{
		if(dist>dmax)
		{
			dmax=dist;
			*imax=stkp;
		}
		if(blist[current].dir == CCW) ccw_cnt++;
		else
			cw_cnt++;
		stk[stkp].nucnum=current;
		stk[stkp++].dist=dist;
	}
	if(dist>dmax)
	{
		dmax=dist;
		*imax=stkp;
	}
	stk[stkp].nucnum=current;
        stk[stkp++].dist=dist;
	here = &(blist[nuc]);
	there = &(blist[current]);
	stk[0].dist = distance(here->x,here->y,there->x,there->y);
/*
*	Is this a clockwise loop, or a counter clockwise loop.
*/
	*ccw = ((ccw_cnt >= cw_cnt)? TRUE:FALSE);
	return(stkp);
}



NextBase(cur,blist,dist)
int cur;
Base blist[];
double *dist;
{
	double Spacing();
	Base here;
	int next,pair,forw;

	here=blist[cur];
	pair=here.pair;
	forw=here.dforw.pair;

	if(redo == TRUE)
	{
		next = ++cur;
		redo = FALSE;
		*dist = BASE_TO_BASE_DIST;
	}
	else if(pair > cur && forw > cur)
	{
		if(blist[pair].known && blist[forw].known)
		{
			next= ++cur;
			*dist=BASE_TO_BASE_DIST;
		}
		else if(here.known == FALSE)
		{
			if(pair>forw)
			{
				next = pair;
				*dist=Spacing(here.nuc,blist[next].nuc);
			}
			else
			{
				next = forw;
				*dist = here.dforw.dist;
			}
		}
		else
		{
			if(pair<=forw)
                        {
                                next = pair;
                                *dist=Spacing(here.nuc,blist[next].nuc);
                        }
                        else
                        {
                                next = forw;
                                *dist = here.dforw.dist;
                        }
                }
	}

	else if(forw != -1)
	{
		next=here.dforw.pair;
		*dist=here.dforw.dist;
	}

	else if(pair > cur && here.known == FALSE)
	{
		next=pair;
		*dist=Spacing(here.nuc,blist[next].nuc);
	}

	else
	{
		next= ++cur;
		*dist=BASE_TO_BASE_DIST;
	}

	return(next);
}



Arc(stk,siz,blist,ccw,imax)
LoopStak stk[];
int siz,imax;
Base *blist;
int ccw;
{
	double x1,y1,x2,y2,xc,yc,a,d,c,dx,dy;
	double dist,dtry,temp,temp1,temp2;
	double th[1000];
	double rho,phi,theta2,theta,dsqrd;
	int sw_flag=FALSE;
	double r,rmax,rmin,slen=0.0;
	register i,j,upflag = TRUE;

	x1 = blist[stk[0].nucnum].x;
	y1 = blist[stk[0].nucnum].y;
	x2 = blist[stk[siz-1].nucnum].x;
	y2 = blist[stk[siz-1].nucnum].y;

	dist=distance(x1,y1,x2,y2);
	dsqrd = sqr(dist);

	rmin=0.0;
	for(i=1;i<siz;i++)
		slen+=stk[i].dist;
	if(slen <= dist)
	{
		dx=(x2-x1)/(double)(siz-1);
		dy=(y2-y1)/(double)(siz-1);
		for(i=1;i<siz-1;i++)
		{
			blist[stk[i].nucnum].x=x1+dx*(double)i;
			blist[stk[i].nucnum].y=y1+dy*(double)i;
			blist[stk[i].nucnum].known=TRUE;
			if(blist[stk[i].nucnum].posnum != -1)
				PosConFix(blist[stk[i].nucnum]);
		}
	}
	else
	{
		if(stk[imax].dist > dist)
		{
			temp=dist;
			dist=stk[imax].dist;
			stk[imax].dist = temp;
			sw_flag = TRUE;
			dsqrd = dist*dist;
		}
		rmax=4.118252;
		rmin=0.0;
/*
		for(i=1;i<siz;i++)
			rmin=Max(rmin,stk[i].dist * .5);
*/
		r = 0.0;
		for(;fabs((rmax + rmin)*.5 - r)>.0001;)
		{
			r=(rmin+rmax) * .5;
			c=1.0/r;
			theta=0.0;
			for(i=1;i<siz;i++)
			{
				temp = stk[i].dist*.5*c;
				if(temp>1.0)
					theta = TWO_PI *100.0;
				else
				{
					th[i]=2.0*asin(temp);
					theta+=th[i];
				}
			}
			if(theta > TWO_PI)
			{
				rmin=r;
				if(upflag)
                                	rmax = r*2.0;
                        }
			else
			{
				temp1=r-r*cos(theta);
				temp2=r*sin(theta);
				dtry=sqr(temp1)+sqr(temp2);

				if(dtry-dsqrd>.01)
				{
					rmax=r;
					upflag = FALSE;
				}
				else if(dtry-dsqrd< -.01)
				{
					rmin=r;
					if(upflag)
						rmax = r*2.0;
				}
				else
				{
					rmin=r;
					rmax=r;
				}
				if(r>10000.0)
				{
                                        rmin=r;
                                        rmax=r;
                                }
			}
		}
		if(sw_flag == TRUE)
		{
			dist=stk[imax].dist;
			temp=th[imax];
			th[imax] = (TWO_PI-theta);
			theta = (TWO_PI-temp);
		}
		rho=atan2(y2-y1,x2-x1);

		a=sqrt(fabs(r*r-dist*dist*.25));
		if(theta<PI_val && sw_flag == FALSE)
			a = -a;

		if (ccw)
			phi=rho-PI_o2  + PI_val;
		else
			phi=rho-PI_o2;


		xc=(x1+x2)*.5+a*cos(phi);
		yc=(y1+y2)*.5+a*sin(phi);

		if(ccw)
			theta2=phi+theta*.5;
		else
			theta2=phi-theta*.5;


		for(i=1;i<siz-1;i++)
		{
			if(ccw)
				theta2-=th[i];
			else
				theta2+=th[i];

			blist[stk[i].nucnum].x=xc+r*cos(theta2);
			blist[stk[i].nucnum].y=yc+r*sin(theta2);
			blist[stk[i].nucnum].known=TRUE;
			if(blist[stk[i].nucnum].posnum != -1)
				PosConFix(blist[stk[i].nucnum]);
		}
	}
}

PromptFile(){}

gprint(base,pw)
Base base;
Pixwin *pw;
{
	int xp,yp;
	int color;
	char dummy[132];


	xp=(int)(base.x * xscale +xoffset);
	yp=(int)(base.y * yscale + yoffset);

	if(base.attr & HILITE)
	{
		pw_char(pw,xp,yp,PIX_NOT(PIX_SRC),NULL,base.nuc);
	}
	else
	{
		switch(base.nuc | 32)
		{
			case 'a':
				color=3;
				break;
			case 'g':
				color = 8;
				break;
			case 'c':
				color = 6;
				break;
			case 'u':
				color = 5;
				break;
			case 't':
				color = 5;
				break;
			default:
				color = 12;
				break;
		};
		if(COLOR)
			pw_char(pw,xp,yp,PIX_SRC | (color <<5),NULL,base.nuc);
		else
			pw_char(pw,xp,yp,PIX_SRC ,NULL,base.nuc);
	}
}

double Spacing(a,b)
char a,b;
{
	a |= 32;
	b |= 32;

	if( a=='a' && b=='u' || a=='u' && b=='a' ||
	a=='a' && b=='t' || a=='t' && b=='a')
		return (1.8);

	if( a=='g' && b=='c' || a=='c' && b=='g')
		return (1.8);

	if( a=='g' && b=='u' || a=='u' && b=='g' ||
	a=='g' && b=='t' || a=='t' && b=='g')
		return (1.8);

	return(2.4);

}

SetFile(){ return 0;}
