/* #include <malloc.h> */
#include <X11/X.h>
#include <X11/Xlib.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
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


DrawNAColor(can,NAdd,xwin,left,top,indx,lpos,rpos,dpy,gc,mode,inverted)
Canvas can;
NA_DisplayData *NAdd;
Window xwin;
Display *dpy;
int left,top,indx,lpos,rpos,mode,inverted;
GC gc;
{
	char buffer[MAX_NA_DISPLAY_WIDTH],map_chr;
	register int j,i,next_color,invrt = FALSE;
	int unselected_inverted = FALSE,dir;
	int pmin,wid,x,y,*tmat,fdx,fdy,pmax,first,used;
	register int seqposindx;
	register unsigned long *pixels;
	extern int SCALE, DisplayAttr;
	extern Pixmap grey_pm[];

	int *color_mask,*colors,color,start_col,offset;
	int maxlen = 0,global_offset = 0;
	NA_Base base;
	NA_Sequence *elem;
	NA_Alignment *aln = NAdd->aln;
	int scrn = DefaultScreen(dpy);
	int dithered = FALSE;

	colors = aln->element[indx].col_lut;
/*
*	Just in case no characters need to be drawn...
*/
	next_color = 13;
	XSetForeground(dpy,gc,BlackPixel(dpy,scrn));
	XSetBackground(dpy,gc,WhitePixel(dpy,scrn));


	pixels = (unsigned long*)xv_get(can,WIN_X_COLOR_INDICES);
	fdx = NAdd -> font_dx;
	fdy = NAdd -> font_dy;
	y=fdy * (indx+1-top);

	elem = &(NAdd->aln->element[indx]);
	tmat = aln->element[indx].tmatrix;
#ifdef HGL
	dir = OrigDir(elem);
#else
	dir = (elem->attr & IS_PRIMARY)?1:
	(elem->attr & IS_SECONDARY)?-1:0;
#endif

	map_chr = (dir == 0)?'+':(dir == -1)?'<':'>';

	wid = (rpos-lpos)/SCALE+1;

	seqposindx=lpos;
	pmax = MIN(lpos+wid*SCALE,aln->element[indx].seqlen+
	    aln->element[indx].offset);
/*
        pmax = lpos+wid*SCALE;
*/
	if(aln->element[indx].elementtype == TEXT)
		mode = COLOR_MONO;

	if((inverted && ((DisplayAttr & INVERTED)==0)) ||
	    ((inverted==FALSE) && (DisplayAttr & INVERTED)))
		unselected_inverted = TRUE;
	else
		unselected_inverted = FALSE;
	if(NAdd->num_colors <16 && mode != COLOR_MONO )
		dithered = TRUE;

	if(mode == COLOR_SEQ_MASK)
	{
		color_mask = elem->cmask;
		if(color_mask == NULL)
			mode = COLOR_LOOKUP;
	}
	if(mode == COLOR_ALN_MASK)
	{
		color_mask = NAdd->aln->cmask;
		global_offset = aln->cmask_offset - aln->rel_offset;
		maxlen = aln->maxlen;
		if(color_mask == NULL)
			mode = COLOR_LOOKUP;
	}
	if(mode == COLOR_LOOKUP && colors == NULL)
		mode = COLOR_MONO;

	color = 9999;
	for(j=0;seqposindx<pmax;j++,seqposindx+=SCALE)
	{
		base = getelem(elem,seqposindx);
		offset = elem->offset;

		switch(mode)
		{
		case COLOR_SEQ_MASK:
			next_color = ((seqposindx >= offset) &&
			    (seqposindx<offset+elem->seqlen))?
			    color_mask[seqposindx-offset]:13;
			break;
		case COLOR_LOOKUP:
			next_color = colors[base];
			break;
		case COLOR_ALN_MASK:
			next_color = ((seqposindx >= global_offset) &&
				 (seqposindx-global_offset < aln->cmask_len))?
			    color_mask[seqposindx-global_offset]:13;
			break;
		case COLOR_STRAND:
			if(((tmat?tmat[base]:base) == '-') ||
			    ((tmat?tmat[base]:base) == '~'))
				next_color = 13;
#ifdef HGL
			else if(elem->attr & IS_ORIG_PRIMARY)
				next_color = 3;
			else if(elem->attr & IS_ORIG_SECONDARY)
				next_color = 6;
#else
			else if(elem->attr & IS_PRIMARY)
				next_color = 3;
			else if(elem->attr & IS_SECONDARY)
				next_color = 6;
#endif
			else
				next_color = NAdd->black;
			break;
		case COLOR_MONO:
		default:
			next_color = NAdd->black;
			break;
		}

		if(elem->subselected)
			if(aln->selection_mask[seqposindx] == '1')
				next_color = 1000 + NAdd->black;
/*
	Adding 1000 to a color signals that it is selected/inverted
*/
		if( next_color == color)
		{
			buffer[j] = tmat?
			    tmat[base]:base;
/*
*	If in map view, set character to '>' '<' '+' or ' '
*/
			if(SCALE > 1)
			{
				if(buffer[j] != '-' && buffer[j] != '~')
					buffer[j] = map_chr;
				else
					buffer[j] = ' ';
			}
		}

		else if (color == 9999)
		{
			buffer[j] = tmat? tmat[base]:base;
/*
*       If in map view, set character to '>' '<' '+' or ' '
*/
			if(SCALE > 1)
			{
				if(buffer[j] != '-' && buffer[j] != '~')
					buffer[j] = map_chr;
				else
					buffer[j] = ' ';
			}
			color = next_color;
			start_col = (seqposindx - left)/SCALE * fdx;
		}
		else
		{
			if(color > 999)
			{
				invrt = (unselected_inverted)?FALSE:TRUE;
				color -= 1000;
			}
			else
				invrt = unselected_inverted;
			if(invrt)
			{
				if(dithered)
				{
					XSetStipple(dpy,gc,grey_pm[15-color]);
				}
				else
				{
					XSetBackground(dpy,gc,pixels[color]);
					XSetForeground(dpy,gc,WhitePixel(dpy,scrn));
				}
			}
			else
			{
				if(dithered)
				{
					XSetStipple(dpy,gc,grey_pm[color]);
				}
				else
				{
					XSetForeground(dpy,gc,pixels[color]);
					XSetBackground(dpy,gc,WhitePixel(dpy,scrn));
				}
			}
			if(dithered == FALSE)
				XDrawImageString(dpy,xwin,gc,
				    start_col, y, buffer,j);
			else
			{
				XSetFillStyle(dpy,gc,FillOpaqueStippled);
				XFillRectangle(dpy,xwin,gc,start_col,y-fdy,
				    j * fdx,fdy);
				XSetFillStyle(dpy,gc,FillSolid);
				XDrawString(dpy,xwin,gc,
				    start_col, y, buffer,j);
			}

			wid -= j;
			j=0;
			buffer[j] = tmat?
			    tmat[base]:base;
/*
*	If in map view, set character to '>' '<' '+' or ' '
*/
			if(SCALE > 1)
			{
				if(buffer[j] != '-' && buffer[j] != '~')
					buffer[j] = map_chr;
				else
					buffer[j] = ' ';
			}
			color = next_color;
			start_col = (seqposindx - left)/SCALE * fdx;
		}
	}

	if(color == 9999)
		color = 13;

	if(color > 999)
	{
		invrt = (unselected_inverted)?FALSE:TRUE;
		color -= 1000;
	}
	else
		invrt = unselected_inverted;
	if(invrt)
	{
		if(dithered)
			XSetStipple(dpy,gc,grey_pm[15-color]);
		else
		{
			XSetBackground(dpy,gc,pixels[color]);
			XSetForeground(dpy,gc,WhitePixel(dpy,scrn));
		}
	}
	else
	{
		if(dithered)
			XSetStipple(dpy,gc,grey_pm[color]);
		else
		{
			XSetForeground(dpy,gc,pixels[color]);
			XSetBackground(dpy,gc,WhitePixel(dpy,scrn));
		}
	}

	if(dithered == FALSE)
		XDrawImageString(dpy,xwin,gc, start_col,y, buffer,j);
	else
	{
		XSetFillStyle(dpy,gc,FillOpaqueStippled);
		XFillRectangle(dpy,xwin,gc,start_col,y-fdy,
		    j * fdx,fdy);
		XSetFillStyle(dpy,gc,FillSolid);
		XDrawString(dpy,xwin,gc, start_col,y, buffer,j);
	}

	wid -= j;
	start_col = (seqposindx - left)/SCALE * fdx;
	for(j=0;j<wid;j++)
	{
		if(SCALE == 1 && elem->elementtype != TEXT)
			buffer[j] = '~';
		else
			buffer[j] = ' ';
	}
	invrt = unselected_inverted;

	buffer[j] = '\0';
	switch(mode)
	{
	case COLOR_MONO:
		color = NAdd ->black;
		break;
	case COLOR_SEQ_MASK:
	case COLOR_ALN_MASK:
	case COLOR_LOOKUP:
	default:
		color = 13;
		break;
	}
	if(invrt)
	{
		if(dithered)
			XSetStipple(dpy,gc,grey_pm[15-color]);
		else
		{
			XSetBackground(dpy,gc,pixels[color]);
			XSetForeground(dpy,gc,WhitePixel(dpy,scrn));
		}
	}
	else
	{
		if(dithered)
			XSetStipple(dpy,gc,grey_pm[color]);
		else
		{
			XSetForeground(dpy,gc,pixels[color]);
			XSetBackground(dpy,gc,WhitePixel(dpy,scrn));
		}
	}

	if(dithered == FALSE)
		XDrawImageString(dpy,xwin,gc,start_col,y, buffer,j);
	else
	{
		XSetFillStyle(dpy,gc,FillOpaqueStippled);
		XFillRectangle(dpy,xwin,gc,start_col,y-fdy, j*fdx,fdy);
		XSetFillStyle(dpy,gc,FillSolid);
		XDrawString(dpy,xwin,gc,start_col,y, buffer,j);
	}

	return;
}
