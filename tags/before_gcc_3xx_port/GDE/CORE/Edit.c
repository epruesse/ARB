#include <string.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include <xview/window.h>
#include <xview/notice.h>
#include "menudefs.h"
#include "defines.h"

/*
Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.
*/
	extern int repeat_cnt,EditDir,BlockInput,SCALE;
	extern Frame frame;
	extern Panel_item left_foot,right_foot;
	extern EditMode;
	extern NA_Alignment *DataSet,*Clipboard;
	extern Canvas EditCan;
	extern DisplayAttr;

NAEvents(win,event,arg)
Xv_window win;
Event *event;
Notify_arg arg;
{
	NA_DisplayData *ddata;
	NA_Alignment *aln;
	NA_Sequence *this_seq;
	Display *dpy;
	GC gc;
	Xv_window view;
	NA_Base c,this_base;
	Scrollbar hsc,vsc;
	char *buf;

	Window xwin;
	int i,j,k,x,y,cursorx,cursory,protection_violation,success=FALSE;
	int startx,endx,starty,endy,test_offscreen = FALSE,eventid;

	if(DataSet == NULL)
		return;

	for(j=0;j<xv_get(EditCan,OPENWIN_NVIEWS);j++)
	{
		view = (Xv_window)xv_get(EditCan,OPENWIN_NTH_VIEW,j,0);
		if(win == (Xv_window)(xv_get(view,CANVAS_VIEW_PAINT_WINDOW)))
			j = xv_get(EditCan,OPENWIN_NVIEWS);
	}

	aln = (NA_Alignment*)DataSet;
	ddata = (NA_DisplayData*)(aln->na_ddata);
	if(ddata == NULL)
		return;

	eventid = event_id(event);
	dpy = (Display *)xv_get(EditCan, XV_DISPLAY);
	xwin = (Window)xv_get(win,XV_XID);
	gc = DefaultGC(dpy,DefaultScreen(dpy));
	cursorx = ddata->cursor_x;
	cursory = ddata->cursor_y;

	if (eventid == WIN_RESIZE)
	{
		if(EditCan)
		{
			hsc=(Scrollbar)xv_get(EditCan,
			    OPENWIN_HORIZONTAL_SCROLLBAR, view);
			vsc=(Scrollbar)xv_get(EditCan,
			    OPENWIN_VERTICAL_SCROLLBAR,view);
			if(hsc)
				xv_set(hsc,SCROLLBAR_VIEW_START,0,0);
			if(vsc)
				xv_set(vsc,SCROLLBAR_VIEW_START,0,0);
		}
	}
/*
*	Highly interdependent with AUTO_SHRINK attribute
*
*	The following an attempt to remove Warning messages on
*	split screen and loading of new data.
*/
	if((event_is_down(event) || event_is_button(event)) && EditCan)
	{
		hsc=(Scrollbar)xv_get(EditCan,OPENWIN_HORIZONTAL_SCROLLBAR,
		    view);
		vsc=(Scrollbar)xv_get(EditCan,OPENWIN_VERTICAL_SCROLLBAR,view);
		if(hsc)
		{
			startx = (int)xv_get(hsc,SCROLLBAR_VIEW_START)/SCALE;
			startx = (int)xv_get(hsc,SCROLLBAR_VIEW_START);
			endx = startx + (int)xv_get(hsc,SCROLLBAR_VIEW_LENGTH)
			    *SCALE;
		}
		if(vsc)
		{
			starty = (int)xv_get(vsc,SCROLLBAR_VIEW_START);
			endy = starty + (int)xv_get(vsc,SCROLLBAR_VIEW_LENGTH);
		}
		if(!(hsc || vsc))
			return;
	}
	if (eventid == LOC_WINENTER ||
	    (event_action(event) == ACTION_TAKE_FOCUS))
		win_set_kbd_focus(win,xwin);
	else if(event_is_down(event) && event_action(event) == ACTION_COPY)
	{
		(void)EditCopy(win,event);
	}
	else if(event_is_down(event) && event_action(event) == ACTION_CUT)
	{
		(void)EditCut(win,event);
	}
	else if(event_is_down(event) && event_action(event) == ACTION_PASTE)
	{
		(void)EditPaste(win,event);
	}
	else if(event_is_button(event) && !BlockInput)
	{
		x=(event_x(event)/ddata->font_dx)*SCALE + startx;
		y=event_y(event)/ddata->font_dy + starty;

		y=MAX(0,MIN(y,aln->numelements - 1));
		x=MAX(0,MIN(x,aln->maxlen));
		if(event_is_down(event)&&(event_action(event)==ACTION_SELECT))
		{
			repeat_cnt = 0;
			UnsetNACursor(ddata,EditCan,win,xwin,dpy,gc);
			ddata->cursor_x = x;
			ddata->cursor_y = y;
			ResetPos(ddata);
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
		}
		if(event_is_up(event)&&(event_action(event)==ACTION_SELECT))
		{
			SubSelect(aln,event_shift_is_down(event),
			    ddata->cursor_x,ddata->cursor_y,x,y);
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
		}
		else if(event_is_up(event)
		    &&(event_action(event)==ACTION_ADJUST))
		{
			SubSelect(aln,TRUE,ddata->cursor_x,ddata->cursor_y,x,y);
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
		}
	}

	if(event_is_ascii(event) && event_is_down(event) &&
	    event_meta_is_down(event) && ((char)eventid == 'm' ||
	    (char)eventid == 'M') )
	{
		EditMode = (EditMode==CHECK)?INSERT:CHECK;
		SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
	}
	else if(event_is_ascii(event) && event_is_down(event) &&
	    event_meta_is_down(event) && (char)eventid == 'u' )
	{
		(void)Ungroup(NULL,NULL);
	}
	else if(event_is_ascii(event) && event_is_down(event) &&
	    event_meta_is_down(event) && (char)eventid == 'g' )
	{
		(void)Group(NULL,NULL);
	}
	else if(event_is_ascii(event) && event_is_down(event) &&
	    event_meta_is_down(event) && (char)eventid == 'i' )
	{
		(void)ModAttr(NULL,NULL);
	}
	else if(event_is_ascii(event) && event_is_down(event) &&
	    event_meta_is_down(event) && (char)eventid == 'p' )
	{
		(void)SetProtection(NULL,NULL);
	}
	else if(event_is_down(event) && event_is_ascii(event) &&
	    event_meta_is_down(event) && (char)eventid>'9')
		DoMeta((char)eventid);

	else if(event_is_down(event) && (event_is_ascii(event) ||
		(char)eventid==0x7 || (char)eventid==0x7f) && !BlockInput)
	{
		if(DisplayAttr & KEYCLICKS)
			Keyclick();
/*
*	De-select the text
*/
		SubSelect(aln,FALSE,0,0,0,0);

		if((char)eventid<='9' && (char)eventid>='0' && ddata->use_repeat &&
		    (aln->element[cursory].elementtype != MASK &&
		    aln->element[cursory].elementtype != TEXT ||
		    event_meta_is_down(event)))
		{
			repeat_cnt = repeat_cnt*10+eventid-'0';
			if(repeat_cnt > 100000000)
				repeat_cnt = 0;
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
		}
		else if((eventid ==0x7f || eventid == 0x8) && EditMode != CHECK)
		{
			int current_id = aln->element[cursory].groupid;
			protection_violation=FALSE;
			repeat_cnt = MAX(1,repeat_cnt);

			for(this_seq = &(aln->element[cursory]);
			    this_seq != NULL;
			    this_seq = this_seq->groupf)

				protection_violation |=
				    DeleteViolate(aln,this_seq,
				    repeat_cnt,cursorx);

			for(this_seq = &(aln->element[cursory]);

			    this_seq != NULL;this_seq = this_seq->groupb)
				protection_violation |=
				    DeleteViolate(aln,this_seq,repeat_cnt,cursorx);

			if(protection_violation == FALSE)
			{
				if(current_id == 0)
					success |= DeleteNA(aln,cursory,
					    repeat_cnt, cursorx);
				else
					for(j=0;j<aln->numelements;j++)
						if(aln->element[j].groupid
						    == current_id)
							success|=DeleteNA(aln,j,
							    repeat_cnt,cursorx);
				if(success)
					ddata->cursor_x -=repeat_cnt;
				test_offscreen = TRUE;
				NormalizeOffset(aln);
			}
			else
			{
				xv_set(frame,FRAME_RIGHT_FOOTER,
				    "Cannot delete",0);
				xv_set(right_foot,PANEL_LABEL_STRING, "Cannot delete",0);
				xv_set(right_foot,PANEL_LABEL_STRING, "Cannot delete",0);
				Beep();
			}
			repeat_cnt = 0;
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
		}

		/* --------------------------------------------------------- */
		/* Added by Scott Ferguson, Exxon Research & Engineering Co. */
		/* --------------------------------------------------------- */
		else if(EditMode == INSERT && (eventid == 11 || eventid == 12))
		{
			/*
				The FETCH key grabs the nearest repeat_cnt bases to the right
				or left and moves them to where the cursor is without shifting
				the other parts of the alignment

				eventid = 11 (CNTRL 'k') means Fetch from the right
				eventid = 12 (CNTRL 'l') means Fetch from the left
			*/

			repeat_cnt = MAX(1,repeat_cnt);
			this_seq=&(aln->element[cursory]);

			for(;this_seq != NULL;
			    this_seq = this_seq->groupf)
				if(this_seq != NULL)
					success = FetchNA(this_seq,eventid,
					    repeat_cnt, cursorx);

			UnsetNACursor(ddata,EditCan,win,xwin,dpy,gc);
			if (success)
				if (eventid == 12)
					ddata->cursor_x += repeat_cnt;
				else
					ddata->cursor_x -= repeat_cnt;

			test_offscreen = TRUE;
			repeat_cnt = 0;
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
		}
		/* ---------------END OF SEGMENT:--------------------------- */
		/* Added by Scott Ferguson, Exxon Research & Engineering Co. */
		/* --------------------------------------------------------- */

		else if(EditMode == INSERT)
		{
			repeat_cnt = MAX(1,repeat_cnt);
			this_seq=&(aln->element[cursory]);
			c = (char)eventid;
			/*
*	remap "space" to "-" if AA sequence
*/
			if(this_seq->elementtype == PROTEIN)
				if(c == ' ')
					c = '-';

			buf = Calloc(sizeof(NA_Base),repeat_cnt);
			for(j=0;j<repeat_cnt;j++)
				buf[j] = c;

			protection_violation = InsertViolate(aln,this_seq,buf,
			    cursorx,repeat_cnt);
			for(;this_seq->groupf != NULL;
			    this_seq = this_seq->groupf)
				protection_violation |=
				    InsertViolate(aln,this_seq,buf,cursorx,
				    repeat_cnt);

			for(this_seq = &(aln->element[cursory]);
			    this_seq->groupb != NULL;
			    this_seq = this_seq->groupb)
				protection_violation |=
				    InsertViolate(aln,this_seq,buf,cursorx,
				    repeat_cnt);

			if(protection_violation == FALSE)
			{
				for(;this_seq != NULL;
				    this_seq = this_seq->groupf)
					if(this_seq != NULL)
						success = InsertNA(this_seq,buf,
						    repeat_cnt, cursorx);

				UnsetNACursor(ddata,EditCan,win,xwin,dpy,gc);
				if(success)
					ddata->cursor_x +=(repeat_cnt * EditDir);

				test_offscreen = TRUE;
			}
			else
			{
				xv_set(frame,FRAME_RIGHT_FOOTER,
				    "Cannot insert",0);
				xv_set(right_foot,PANEL_LABEL_STRING, "Cannot delete",0);
				Beep();
			}

			repeat_cnt = 0;
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
			cfree(buf);
		}
		/*
*	Check mode
*/
		else
		{
			printf("Eventid = %d\n",eventid);
			c = toupper((char)eventid);
			this_base = getelem(&(aln->element[cursory]),cursorx);
			if(aln->element[cursory].tmatrix)
			{
				this_base=aln->element[cursory].
				    tmatrix[(int)this_base];
			}
			this_base = toupper(this_base);

			if(c==this_base)
			{
				UnsetNACursor(ddata,EditCan,win,xwin,dpy,gc);
				ddata->cursor_x++;
				test_offscreen = TRUE;
				ResetPos(ddata);
				SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
			}
			else
			{
				Beep();
			}
		}
	}
	else if(event_is_down(event)&& !BlockInput)
	{
		if (event_action(event) == ACTION_GO_COLUMN_BACKWARD)
		{
			if(DisplayAttr & KEYCLICKS)
				Keyclick();
			repeat_cnt = MAX(1,repeat_cnt);
			UnsetNACursor(ddata,EditCan,win,xwin,dpy,gc);
			ddata->cursor_y = MAX(0,ddata->cursor_y-repeat_cnt);
			test_offscreen = TRUE;
			repeat_cnt = 0;
			ResetPos(ddata);
			SubSelect(aln,FALSE,0,0,0,0);
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
		}
		else if (event_action(event) == ACTION_GO_COLUMN_FORWARD)
		{
			if(DisplayAttr & KEYCLICKS)
				Keyclick();
			repeat_cnt = MAX(1,repeat_cnt);
			UnsetNACursor(ddata,EditCan,win,xwin,dpy,gc);
			ddata->cursor_y = MAX(0,MIN(aln->numelements - 1,
			    ddata->cursor_y+repeat_cnt));
			repeat_cnt = 0;
			test_offscreen = TRUE;
			ResetPos(ddata);
			SubSelect(aln,FALSE,0,0,0,0);
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
		}
		else if (event_action(event) == ACTION_GO_CHAR_BACKWARD)
		{
			if(DisplayAttr & KEYCLICKS)
				Keyclick();
			repeat_cnt = MAX(SCALE,repeat_cnt);
			UnsetNACursor(ddata,EditCan,win,xwin,dpy,gc);
			ddata->cursor_x = MAX(0,ddata->cursor_x-repeat_cnt);
			test_offscreen = TRUE;
			repeat_cnt = 0;
			ResetPos(ddata);
			SubSelect(aln,FALSE,0,0,0,0);
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
		}
		else if (event_action(event) == ACTION_GO_CHAR_FORWARD)
		{
			if(DisplayAttr & KEYCLICKS)
				Keyclick();
			repeat_cnt = MAX(SCALE,repeat_cnt);
			UnsetNACursor(ddata,EditCan,win,xwin,dpy,gc);
			ddata->cursor_x = MAX(0,MIN(aln->maxlen,ddata->
			    cursor_x + repeat_cnt));
			test_offscreen = TRUE;
			repeat_cnt = 0;
			ResetPos(ddata);
			SubSelect(aln,FALSE,0,0,0,0);
			SetNACursor(ddata,EditCan,win,xwin,dpy,gc);
		}
	}
	if(((ddata->cursor_x<startx)||(ddata->cursor_x>endx-1))
	    && test_offscreen)
	{
		x = ddata->cursor_x-(endx-startx)/2;
		x = (MAX(0,MIN(x,aln->maxlen - (endx-startx))));
		(void)JumpTo(view,x,starty);
	}

	if(((ddata->cursor_y<starty)||(ddata->cursor_y>endy-1))
	    && test_offscreen)
	{
		y = ddata->cursor_y-(endy-starty)/2;
		y = (MAX(0,MIN(y,aln->numelements - (endy-starty))));
		(void)JumpTo(view,startx,y);
	}
	return;
}


/*
Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Steven Smith at the Center for Prokaryote Genome
Analysis.  Design and implementation guidance by Dr. Gary Olsen and Dr.
Carl Woese.

Copyright (c) 1990,1991,1992 Steven Smith at the Harvard Genome Laboratory.
All rights reserved.

*/


InsertViolate(aln,seq,insert,cursor_x,len)
NA_Alignment *aln;
NA_Sequence *seq;
NA_Base *insert;
int cursor_x,len;
{
	int i,j,prot,violated = FALSE;
	prot = seq->protect;
	if(seq->rmatrix)
		for(i=0;i<len;i++)
			insert[i] = seq->rmatrix[insert[i]];


	if((prot & PROT_BASE_CHANGES)==0)
	{
		if(seq->elementtype == DNA || seq->elementtype == RNA)
		{
/*
*		if character is not '-' or 'N' then
*		protection is violated.
*/
			for(j=0;j<len;j++)
				if( (insert[j] & 15) && ((insert[j]&15)!=15))
					violated = TRUE;
		}

		else if(seq->elementtype == PROTEIN)
		{
			for(j=0;j<len;j++)
				if((insert[j] != '-') && (insert[j] !='~')
				    && (insert[j]|32) != 'x')
					violated = TRUE;
		}

		else if (seq->elementtype == MASK)
		{
			for(j=0;j<len;j++)
				if(insert[j] |= '0')
					violated = TRUE;
		}
	}

	if((prot & PROT_WHITE_SPACE)==0)
	{
		if(seq->elementtype == DNA || seq->elementtype == RNA)
		{
			/*
*		if character is '-' then
*		protection is violated.
*/
			for(j=0;j<len;j++)
				if((insert[j]&15)==0)
					violated = TRUE;
		}
		else if(seq->elementtype == PROTEIN)
		{
			for(j=0;j<len;j++)
				if((insert[j])=='-' || insert[j] == '~' ||
				    insert[j] == ' ')
					violated = TRUE;
		}

	}

	if((prot & PROT_GREY_SPACE)==0)
	{

		if(seq->elementtype == DNA || seq->elementtype == RNA)
		{
			for(j=0;j<len;j++)
				if((insert[j] & 15)==15)
					violated = TRUE;
		}

		else if(seq->elementtype == PROTEIN)
		{
			for(j=0;j<len;j++)
				if((insert[j]|32) == 'x')
					violated = TRUE;
		}
	}
	if(seq->tmatrix)
		for(i=0;i<len;i++)
			insert[i] = seq->tmatrix[insert[i]];

	return(violated);
}


InsertNA(seq,insert,len,pos)
/*
*	return Success
*/
NA_Sequence *seq;
NA_Base *insert;
int len,pos;
{
	int i,j,snum,x = pos+100;
	int curlen,maxlen,offset;
	extern NA_Alignment *DataSet;
	extern Canvas EditCan;
	NA_DisplayData *NAdd;
	NA_Alignment *aln = (NA_Alignment*)DataSet;
	Xv_window win;
	Window xwin;
	Display *dpy;
	GC gc;

	if(seq->rmatrix)
		for(i=0;i<len;i++)
			insert[i] = seq->rmatrix[insert[i]];

	dpy = (Display *)xv_get(EditCan, XV_DISPLAY);
	gc = DefaultGC(dpy,DefaultScreen(dpy));
	NAdd =(NA_DisplayData*)aln->na_ddata;
	offset = seq->offset;
	curlen = seq->seqlen;
	maxlen = seq->seqmaxlen;

	if(seq->elementtype == MASK)
		for(i=0;i<len;i++)
			if(insert[i] < '0' || insert[i] > '9')
				insert[i] = '0';


	/*
*	The current snum (sequence number) should be passed into this
*	routine.  This means that the index into the alignment needs
*	to be included in the "sequence id." The following is a lookup
*	for the snum (slow).
*/

	for(j = 0;j < aln->numelements;j++)
		if(&(aln->element[j]) == seq)
		{
			snum = j;
			j = aln->numelements;
		}

	if(pos > seq->seqlen+seq->offset)
	{
		xv_set(frame,FRAME_RIGHT_FOOTER, "Cannot insert beyond end",0);
		xv_set(right_foot,PANEL_LABEL_STRING, "Cannot insert beyond end",0);
		Beep();
		if(seq->tmatrix)
			for(i=0;i<len;i++)
				insert[i] = seq->tmatrix[insert[i]];
		return(FALSE);
	}

	if(seq->seqlen+len>=seq->seqmaxlen-1)
	{
		if(seq->sequence)
		{
			seq->sequence = (NA_Base*)
			    Realloc(seq->sequence,(seq->seqlen+len+100)*sizeof(NA_Base));
			seq->seqmaxlen = seq->seqlen+len+100;
		}
		else
		{
			seq->sequence = (NA_Base*)
			    Calloc(sizeof(NA_Base),(seq->seqlen+len+100));
			seq->seqmaxlen = seq->seqlen+len+100;
		}
		if(seq->cmask)
		{
			seq->cmask = (int*)Realloc(seq->cmask,
			    seq->seqlen*sizeof(int));
		}
	}

	/*
*	This forces space to be allocated upstream, and thus prevents
*	memory thrashing.  Not a wonderful fix, but it will do for now...
*/
	if(pos<seq->offset)
		putelem(seq,pos,'\0');
	for(j=0;j<=seq->seqlen+seq->offset - pos+1;j++)
	{
		putelem(seq,seq->seqlen+len+seq->offset - j,
		    getelem(seq,seq->offset+seq->seqlen - j));
		if(seq->cmask)
			putcmask(seq,seq->seqlen+len+seq->offset-j,
			    getcmask(seq,seq->seqlen+seq->offset - j));
	}

	for(j=0;j<len;j++)
	{
		putelem(seq,pos+j,insert[j]);
		if(seq->cmask)
			putcmask(seq,pos+j,8);
	}

	seq->seqlen = seq->seqlen + len;
	aln->maxlen = MAX(aln->maxlen,seq->seqlen+1);

	RedrawAllNAViews(snum,pos);

	if(seq->tmatrix)
		for(i=0;i<len;i++)
			insert[i] = seq->tmatrix[insert[i]];

	return(TRUE);
}

/*------------------------------------------------------------------------*/
/*	   Added by Scott Ferguson, Exxon Research & Engineering Co.    */
/*		    In support of the "Fetch" key operation	     */
/*------------------------------------------------------------------------*/
/* dir = CNTRL 'k'(11), fetch left; dir = CNTRL 'l'(12), fetch right */
FetchNA(seq,dir,len,pos)
/*
*	return Success
*/
NA_Sequence *seq;
unsigned int dir;
int len,pos;
{
	int i,j,snum,x = pos+100;
	int curlen,maxlen,offset;
	extern NA_Alignment *DataSet;
	extern Canvas EditCan;
	NA_DisplayData *NAdd;
	NA_Alignment *aln = (NA_Alignment*)DataSet;
	Xv_window win;
	Window xwin;
	Display *dpy;
	GC gc;
	NA_Base *scratch, tgap;
	int incr, nearest, *cscratch, tcmask;

	dpy = (Display *)xv_get(EditCan, XV_DISPLAY);
	gc = DefaultGC(dpy,DefaultScreen(dpy));
	NAdd =(NA_DisplayData*)aln->na_ddata;
	offset = seq->offset;
	curlen = seq->seqlen;
	maxlen = seq->seqmaxlen;

	/*
*	The current snum (sequence number) should be passed into this
*	routine.  This means that the index into the alignment needs
*	to be included in the "sequence id." The following is a lookup
*	for the snum (slow).
*/

	for(j = 0;j < aln->numelements;j++)
		if(&(aln->element[j]) == seq)
		{
			snum = j;
			j = aln->numelements;
		}

	scratch = (NA_Base *) Calloc(sizeof(NA_Base),len);
	cscratch = (int *) Calloc(sizeof(int),len);

	if (dir == 12)
	{
		incr = 1;
		if (pos < offset) nearest = offset-pos;
		else nearest = 0;
		while (pos+nearest < seq->seqlen+seq->offset)
		{
			if (isagap(seq,pos+nearest)) nearest++;
			else break;
		}
		if (pos+nearest+len > seq->seqlen+seq->offset)
		{
			xv_set(frame,FRAME_RIGHT_FOOTER, "Cannot fetch beyond end",0);
			xv_set(right_foot,PANEL_LABEL_STRING, "Cannot fetch beyond end",0);
			Beep();
			return(FALSE);
		}
	}
	else
	{
		if (pos >= offset + curlen)
		{
			xv_set(frame,FRAME_RIGHT_FOOTER, "Cannot fetch past end",0);
			xv_set(right_foot,PANEL_LABEL_STRING, "Cannot fetch past end",0);
			Beep();
			return(FALSE);
		}
		incr = -1;
		nearest = 0;
		while (pos+nearest > seq->offset) {
			if (isagap(seq,pos+nearest)) nearest--;
			else break;
		}
		if (pos+nearest-len+1 < seq->offset)
		{
			xv_set(frame,FRAME_RIGHT_FOOTER, "Cannot fetch beyond beginning",0);
			xv_set(right_foot,PANEL_LABEL_STRING, "Cannot fetch beyond beginning",0);
			Beep();
			return(FALSE);
		}
	}

	if (nearest == 0)
	{
		xv_set(frame,FRAME_RIGHT_FOOTER, "Base is not a Gap",0);
		xv_set(right_foot,PANEL_LABEL_STRING, "Base is not a Gap",0);
		Beep();
		return(FALSE); /* this means we're not sitting on a gap */
	}

	tgap = getelem(seq,pos);
	if (seq->cmask)
		tcmask = getcmask(seq,pos);
	for(j=0;j<len;j++)
	{
		scratch[j] = getelem(seq,pos+j*incr+nearest);
		if (seq->cmask)
			cscratch[j] = getcmask(seq,pos+j*incr+nearest);
	}

	for (j=0;j<len;j++)
	{
		putelem(seq,pos+j*incr+nearest,tgap);
		if (seq->cmask)
			putcmask(seq,pos+j*incr+nearest,tcmask);
	}

	for (j=0;j<len;j++)
	{
		putelem(seq,pos+j*incr,scratch[j]);
		if(seq->cmask)
			putcmask(seq,pos+j*incr,cscratch[j]);
	}

	nearest = 0;
	while (isagap(seq,seq->offset+nearest)) nearest++;
	if (nearest != 0) {
		for (j=0;j<seq->seqlen-nearest;j++) {
			putelem(seq,seq->offset+j,getelem(seq,seq->offset+nearest+j));
			if(seq->cmask)
				putcmask(seq,seq->offset+j,
					getcmask(seq,seq->offset+nearest+j));
		}
		for (j=0;j<nearest;j++) {
			putelem(seq,seq->offset+seq->seqlen-nearest+j,tgap);
			if(seq->cmask)
				putcmask(seq,seq->offset+j,tcmask);
		}
		seq->seqlen -= nearest;
		seq->offset += nearest;
	}

	if (dir == 12)
		RedrawAllNAViews(snum,MIN(pos,pos+nearest+(len-1)*incr));
	else
		RedrawAllNAViews(snum,0);
	xv_set(frame,FRAME_RIGHT_FOOTER, "",0);
	xv_set(right_foot,PANEL_LABEL_STRING, "",0);
	free(cscratch);
	free(scratch);
	return(TRUE);
}
/*------------------------------------------------------------------------*/
/*		 End of lines added by S. R. Ferguson srfergu@erenj.com */
/*		    In support of the "Fetch" key operation	     */
/*------------------------------------------------------------------------*/


DeleteNA(aln,seqnum,len,offset)
NA_Alignment *aln;
int seqnum,len,offset;
{
	int i,j,seqlen = aln->element[seqnum].seqlen+
	aln->element[seqnum].offset;

	NA_Sequence *seq =&(aln->element[seqnum]);
	if(offset > seq->offset+seq->seqlen)
	{
		xv_set(frame,FRAME_RIGHT_FOOTER, "Cannot delete beyond end",0);
		xv_set(right_foot,PANEL_LABEL_STRING, "Cannot delete beyond end",0);
		Beep();
		return(FALSE);
	}

	if (len>offset)
	{
		xv_set(frame,FRAME_RIGHT_FOOTER, "Cannot delete beyond end",0);
		xv_set(right_foot,PANEL_LABEL_STRING, "Cannot delete beyond end",0);
		Beep();
		return(FALSE);
	}
	for(j=offset-len;j<seqlen-len;j++)
	{
		putelem(seq,j,getelem(seq,j+len));
		if(aln->element[seqnum].cmask)
			putcmask(seq,j-seq->offset,getcmask(seq,j+seq->offset));
	}
	aln->element[seqnum].seqlen = aln->element[seqnum].seqlen-len;
	RedrawAllNAViews(seqnum,offset-len);
	return(TRUE);
}


DeleteViolate(aln,this_seq,len,offset)
NA_Alignment *aln;
NA_Sequence* this_seq;
int len,offset;
{
	int i,j,prot,violated = FALSE;
	prot = this_seq->protect;

	if((prot & PROT_BASE_CHANGES)==0)
	{
		if(this_seq->elementtype == DNA ||
		    this_seq->elementtype == RNA)
		{
			for(j=offset-len;j<offset;j++)
				if((getelem(this_seq,j) & 15)!=0 &&
				    (getelem(this_seq,j)&15)!=15)
					violated = TRUE;
		}
		else if(this_seq->elementtype == PROTEIN)
		{
			for(j=offset-len;j<offset;j++)
				if(getelem(this_seq,j)!= '-' &&
				    getelem(this_seq,j)!= '~' &&
				    getelem(this_seq,j)!= ' ' &&
				    (getelem(this_seq,j)|32) != 'x')
					violated = TRUE;
		}
		else if(this_seq->elementtype == MASK)
			for(j=offset-len;j<offset;j++)
				if(this_seq->sequence[j] != '0')
					violated = TRUE;
	}

	if((prot & PROT_WHITE_SPACE)==0)
	{
		if(this_seq->elementtype == DNA ||
		    this_seq->elementtype == RNA)
		{
			for(j=offset-len;j<offset;j++)
				if(getelem(this_seq,j) & 15)
					violated = TRUE;
		}

		else if(this_seq->elementtype == PROTEIN)
		{
			for(j=offset-len;j<offset;j++)
				if(getelem(this_seq,j) =='-' ||
				    getelem(this_seq,j) =='~' ||
				    getelem(this_seq,j) ==' ')
					violated = TRUE;
		}
	}

	if((prot & PROT_GREY_SPACE)==0)
	{
		if(this_seq->elementtype == DNA ||
		    this_seq->elementtype == RNA)
		{
			for(j=offset-len;j<offset;j++)
				if((getelem(this_seq,j) &15)==15)
					violated = TRUE;
		}
		else if(this_seq->elementtype == PROTEIN)
		{
			for(j=offset-len;j<offset;j++)
				if((getelem(this_seq,j)|32) == 'x')
					violated = TRUE;
		}
	}

	/*
*	Test to see if we really can delete this much
*	or from this column position.

	if ((len>offset) || (offset > this_seq->seqlen + this_seq->offset))
		violated = TRUE;
*/
	if(len>offset)
		violated = TRUE;

	return(violated);
}


RedrawAllNAViews(seqnum,start)
int seqnum,start;
{

	extern NA_Alignment *DataSet;
	extern Canvas EditCan;
	extern int SCALE;
	NA_DisplayData *ddata;
	NA_Alignment *aln;
	NA_Sequence *this_seq;
	Scrollbar hsc,vsc;
	Display *dpy;
	GC gc;
	Xv_window win,view;
	Window xwin;
	int hstart,vstart,hend,j;

	if(DataSet == NULL)
		return;

	aln = (NA_Alignment*)DataSet;
	ddata = (NA_DisplayData*)(aln->na_ddata);
	if(ddata == NULL)
		return;

	dpy = (Display *)xv_get(EditCan, XV_DISPLAY);
	gc = DefaultGC(dpy,DefaultScreen(dpy));

	for(j=0;j<(int)xv_get(EditCan,OPENWIN_NVIEWS);j++)
	{
		view = (Xv_window)xv_get(EditCan,OPENWIN_NTH_VIEW,j,0);
		win = xv_get(view,CANVAS_VIEW_PAINT_WINDOW);
		xwin = (Window)xv_get(win,XV_XID);
		hsc = (Scrollbar)xv_get(EditCan,OPENWIN_HORIZONTAL_SCROLLBAR,
		    view);
		vsc = (Scrollbar)xv_get(EditCan,OPENWIN_VERTICAL_SCROLLBAR,
		    view);
		hstart = xv_get(hsc,SCROLLBAR_VIEW_START);
		vstart = xv_get(vsc,SCROLLBAR_VIEW_START);
		hend = hstart + xv_get(hsc,SCROLLBAR_VIEW_LENGTH) * SCALE;
		if(start < hend)
			DrawNAColor(EditCan,ddata,xwin,hstart, vstart,seqnum,
			    MAX(start,hstart),hend, dpy,gc,ddata->color_type,FALSE);
	}
	return;
}


ResetPos(ddata)
NA_DisplayData *ddata;
{
	NA_Base *seq;
	int j,total = 0,maxpos;
	NA_Sequence *elem;

	if(ddata == NULL)
		return;
	elem = &(ddata->aln->element[ddata->cursor_y]);
	if(elem->sequence == NULL)
		return;

	maxpos = MAX(0,MIN(ddata->cursor_x,elem->seqlen+elem->offset));

	switch (elem->elementtype)
	{
	case DNA:
	case RNA:
		for(j=elem->offset;j<=maxpos;j++)
			if((getelem(elem,j) & 15)
			    && !(getelem(elem,j) & 128))
				total++;
		break;
	case PROTEIN:
		for(j=0;j<=maxpos;j++)
			if((getelem(elem,j) != ' ') &&
			    (getelem(elem,j) != '-') &&
			    (getelem(elem,j) != '~'))
				total++;
		break;
	case MASK:
	case TEXT:
	default:
		total = ddata->cursor_y;
		break;
	}
	ddata->position = total;
	return;
}

Beep()
{
#ifdef SUN4
	FILE *audio;
	int j;
	audio = fopen("/dev/audio","w");
	if (audio != NULL)
		for(j=0;j<20;j++)
			fprintf(audio,"zzzzz     ");
	fclose(audio);
#else
	fprintf(stderr,"%c",7);
	fflush(stderr);
#endif
	return;
}

Keyclick()
{
#ifdef SUN4
	FILE *audio;
	int j;
	audio = fopen("/dev/audio","w");
	if (audio != NULL)
		for(j=0;j<10;j++)
			fprintf(audio,"zzzzzzzzzz	  ");
	fclose(audio);
#else
	fprintf(stderr,"%c",7);
	fflush(stderr);
#endif
	return;
}

putelem(a,b,c)
NA_Sequence *a;
int b;
NA_Base c;
{
	int j,newsize;
	NA_Base *temp;

	if(b>=(a->offset+a->seqmaxlen))
		Warning("Putelem:insert beyond end of sequence space ignored");
	else if(b >= (a->offset))
		a->sequence[b-(a->offset)] = c;
	else
	{
		temp =(NA_Base*)Calloc(a->seqmaxlen+a->offset-b,
		    sizeof(NA_Base));
		switch (a->elementtype)
		{
			/*
*	Pad out with gap characters fron the point of insertion to the offset
*/
		case MASK:
			for(j=b;j<a->offset;j++)
				temp[j-b]='0';
			break;
		case DNA:
		case RNA:
			for(j=b;j<a->offset;j++)
				temp[j-b]='\0';
			break;
		case PROTEIN:
			for(j=b;j<a->offset;j++)
				temp[j-b]='-';
			break;
		case TEXT:
		default:
			for(j=b;j<a->offset;j++)
				temp[j-b]=' ';
			break;
		}

		for(j=0;j<a->seqmaxlen;j++)
			temp[j+a->offset-b] = a->sequence[j];
		Cfree(a->sequence);
		a->sequence = temp;
		a->seqlen += (a->offset - b);
		a->seqmaxlen +=(a->offset - b);
		a->offset = b;
		a->sequence[0] = c;
	}
	return;
}

putcmask(a,b,c)
NA_Sequence *a;
int b;
int c;
{
	int j,newsize;
	int *temp;
	if(b >= (a->offset) )
		a->cmask[b-(a->offset)] = c;
	return;
}


getelem(a,b)
NA_Sequence *a;
int b;
{
	if(a->seqlen == 0)
		return(-1);
	if(b<a->offset || (b>a->offset+a->seqlen))
		switch(a->elementtype)
		{
		case DNA:
		case RNA:
			return(0);
		case PROTEIN:
		case TEXT:
			return('~');
		case MASK:
			return('0');
		default:
			return('-');
		}
	else
		return(a->sequence[b-a->offset]);
}
/*------Added by Scott Ferguson, Exxon Research & Engineering Co. ---------*/
isagap(a,b)
NA_Sequence *a;
int b;
{
	int j,newsize;
	NA_Base *temp;

	if (b < a->offset) return(1);

	/*	Check to see if base at given position is a gap */
	switch (a->elementtype) {
		case MASK:
			if (a->sequence[b-a->offset] == '0') return(1);
			else return(0);
		case DNA:
		case RNA:
			if (a->sequence[b-a->offset] == '\0') return(1);
			else return(0);
		case PROTEIN:
			if (a->sequence[b-a->offset] == '-') return(1);
			else return(0);
		case TEXT:
		default:
			if (a->sequence[b-a->offset] == ' ') return(1);
			else return(0);
	}
}
/*-END:-Added by Scott Ferguson, Exxon Research & Engineering Co. ---------*/

SubSelect(aln,shift_down,x1,y1,x2,y2)
NA_Alignment *aln;
int shift_down,x1,y1,x2,y2;
{
	int j;
	NA_Sequence *next_elem;

	if(aln == NULL)
		return;

	if(!shift_down)
		for(j=0;j<aln->numelements;j++)
			if(aln->element[j].subselected == TRUE)
			{
				aln->element[j].subselected = FALSE;
				RedrawAllNAViews(j,aln->min_subselect);
			}

	if(x1==x2 && y1==y2)
	{
		if(!shift_down)
		{
			if(aln->selection_mask)
				for(j=0;j<aln->selection_mask_len;j++)
					aln->selection_mask[j] = '0';
			return;
		}
		else
			x1 = aln->min_subselect;
	}


	if(x1>x2)
	{
		j=x1;
		x1=x2;
		x2=j;
	}
	if(y1>y2)
	{
		j=y1;
		y1=y2;
		y2=j;
	}
	if(aln->maxlen > aln->selection_mask_len)
	{
		if(aln->selection_mask != NULL)
			Cfree(aln->selection_mask);
		aln->selection_mask = (char*)Calloc(aln->maxlen,sizeof(char));
		aln->selection_mask_len = aln->maxlen;
	}

	if(shift_down)
	{
		/*
*	Logical or select within the region
*/
		for(j=x1;j<=x2;j++)
			aln->selection_mask[j] = '1';

		/*
*	Logical or select across selected seqeunces
*/
		for(j=y1;j<=y2;j++)
		{
			aln->element[j].subselected = TRUE;
			/*
*	Impose groups...
*/
			for(next_elem= &(aln->element[j]);
			    next_elem!=NULL; next_elem=next_elem->groupf)
				next_elem->subselected = TRUE;

			for(next_elem= &(aln->element[j]);
			    next_elem!=NULL; next_elem=next_elem->groupb)
				next_elem->subselected = TRUE;
		}
		for(j=0;j<aln->numelements;j++)
			if(aln->element[j].subselected)
				RedrawAllNAViews(j,MIN(x1,aln->min_subselect));
	}
	else
	{
		for(j=0;j<aln->selection_mask_len;j++)
			if(j>x2 || j<x1)
				aln->selection_mask[j] = '0';
			else
				aln->selection_mask[j] = '1';

		for(j=0;j<aln->numelements;j++)
			aln->element[j].subselected = FALSE;

		for(j=0;j<aln->numelements;j++)
			if(j<=y2 && j>=y1)
			{
				aln->element[j].subselected = TRUE;
				/*
*	Impose groups...
*/
				for(next_elem= &(aln->element[j]);
				    next_elem!=NULL; next_elem=next_elem->groupf)
					next_elem->subselected = TRUE;

				for(next_elem= &(aln->element[j]);
				    next_elem!=NULL; next_elem=next_elem->groupb)
					next_elem->subselected = TRUE;
			}

		for(j=0;j<aln->numelements;j++)
			if(aln->element[j].subselected == TRUE)
				RedrawAllNAViews(j,MIN(x1,aln->min_subselect));
	}
	if(shift_down)
		aln->min_subselect = MIN(x1,aln->min_subselect);
	else
		aln->min_subselect = x1;
	return;
}
