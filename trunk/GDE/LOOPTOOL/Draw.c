#include <stdio.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/cursor.h>
#include <xview/panel.h>
#include <string.h>
/* #include <malloc.h> */
#include "loop.h"
#include "globals.h"

DrawPair(base1,base2,pw)
Base base1,base2;
Pixwin *pw;
{
	char a,b;
	int i,j,x,y;
	double x1,y1,x2,y2;

	a=base1.nuc | 32;
	b=base2.nuc | 32;
	x1=base1.x+(double)CWID/xscale*.5;
	y1=base1.y-(double)CHT/yscale*.5;
	x2=base2.x+(double)CWID/xscale*.5;
	y2=base2.y-(double)CHT/yscale*.5;

	if(	a=='u' && b=='a' ||
		a=='t' && b=='a' ||
		a=='a' && b=='u' ||
		a=='a' && b=='t' ||
		a=='c' && b=='g' ||
		a=='g' && b=='c'
	)
	{
		line(x2+0.35*(x1-x2),y2+0.35*(y1-y2),
		x2+0.65*(x1-x2),y2+0.65*(y1-y2),6,pw);
	}
	else if(	a=='u' && b=='g' ||
			a=='t' && b=='g' ||
			a=='g' && b=='u' ||
			a=='g' && b=='t'
	)
	{
		x=(int)((x1+x2)*.5*xscale+xoffset);
		y=(int)((y1+y2)*.5*yscale+yoffset);
		pw_vector(pw,x-1,y-1,x+1,y-1,(PIX_SRC),1);
		pw_vector(pw,x-1,y,x+1,y,(PIX_SRC),1);
		pw_vector(pw,x-1,y+1,x+1,y+1,(PIX_SRC),1);
	}
	else if(        a=='a' && b=='g' ||
                        a=='u' && b=='u' ||
                        a=='u' && b=='u'	)
	{
		x=(int)((x1+x2)*.5*xscale+xoffset);
                y=(int)((y1+y2)*.5*yscale+yoffset);
                pw_vector(pw,x-1,y-1,x+1,y-1,(PIX_SRC),1);
                pw_vector(pw,x-1,y-1,x-1,y+1,(PIX_SRC),1);
                pw_vector(pw,x+1,y-1,x+1,y+1,(PIX_SRC),1);
                pw_vector(pw,x-1,y+1,x+1,y+1,(PIX_SRC),1);
	}
	return;

}

line(x1,y1,x2,y2,color,pw)
double x1,y1,x2,y2;
int color;
Pixwin *pw;
{
	 pw_vector(pw,(int)(x1* xscale +xoffset),
		(int)(y1* yscale + yoffset),
		(int)(x2* xscale +xoffset),
		(int)(y2* yscale + yoffset),
                (PIX_SRC | (color<<5)),color );
	return;
}


gputc(x,y,c,pw)
double x,y;
char c;
Pixwin *pw;
{
	pw_char(pw,(int)(x*xscale+xoffset),
		(int)(y*yscale+yoffset),
		PIX_SRC,NULL,c);
	return;
}


void EventHandler(window,event,arg)
        Xv_window window;
        Event *event;
        caddr_t arg;
{
	double x,y;
	int nuc,i,start,endd;
	extern void ReDraw();
	if(event_id(event) == WIN_RESIZE)
	{
		modified = TRUE;
		ReDraw(pagecan,canvas_paint_window(pagecan),NULL);
	}
	x = (double)(event_x(event)-xoffset);
	x /= xscale;
	y = (double)(event_y(event)-yoffset);
	y /= yscale;
	x = x-(double)CWID/xscale*.5;
	y = y+(double)CHT/yscale*.5;

	if(event_id(event) == BUT(1) && event_is_down(event))
	{
		nuc=FindNuc(x,y);


		if(nuc== -1)
		{
			if(select_state == 3)
			{
				start=Min(nuc_sel1,nuc_sel2);
                                endd=Max(nuc_sel1,nuc_sel2);
                                for(i=start; i<=endd;i++)
                                        UnHiLite(i);
			}
			else
			{
				UnHiLite(nuc_sel1);
				UnHiLite(nuc_sel2);
			}
			nuc_sel1= -1;
			nuc_sel2= -1;
			select_state=0;
			return;
		}
		switch(select_state)
		{
			case 0:
				nuc_sel1=nuc;
				HiLite(nuc);
				select_state=1;
				break;
			case 1:
				nuc_sel2=nuc;
				HiLite(nuc);
				select_state=2;
				break;
			case 2:
				UnHiLite(nuc_sel1);
				UnHiLite(nuc_sel2);
				select_state=0;
                                nuc_sel1= -1;
                                nuc_sel2= -1;
				break;
			case 3:
				start=Min(nuc_sel1,nuc_sel2);
				endd=Max(nuc_sel1,nuc_sel2);
				for(i=start; i<=endd;i++)
					UnHiLite(i);
				nuc_sel1= -1;
				nuc_sel2= -1;
				select_state=0;
				break;
			default:
				select_state=0;
				break;
		};
	}
	else if(event_id(event) == BUT(1) && event_is_up(event))
	{
		nuc=FindNuc(x,y);
		if(nuc== -1 && select_state != 2)
		{
			if(select_state == 3)
			{
				start=Min(nuc_sel1,nuc_sel2);
                                endd=Max(nuc_sel1,nuc_sel2);
                                for(i=start; i<=endd;i++)
					UnHiLite(i);
			}
			else
			{
				UnHiLite(nuc_sel1);
				UnHiLite(nuc_sel2);
			}
			nuc_sel1= -1;
			nuc_sel2= -1;
			select_state=0;
			return;
		}
		if(nuc!=nuc_sel1)
		switch(select_state)
		{
			case 1:
				nuc_sel2=nuc;
				start=Min(nuc_sel1,nuc_sel2);
				endd=Max(nuc_sel1,nuc_sel2);
				for(i=start;i<=endd;i++)
                                        HiLite(i);
				select_state=3;
				break;
			case 2:
				if(IsLoop(nuc_sel1,nuc_sel2))
					switch (constraint_state)
					{
					    case POSITIONAL:
						SetPos(nuc_sel1,nuc_sel2,x,y);
						break;

					    case ANGULAR:
						SetAng(nuc_sel1,nuc_sel2,x,y);
						break;

					    case DISTANCE:
						SetDist(nuc_sel1,nuc_sel2,x,y);
						break;
					}
				else
				{
					fprintf(stderr,"%c",7);
					fflush(stderr);
				}

				UnHiLite(nuc_sel1);
                                UnHiLite(nuc_sel2);
                                select_state=0;
                                nuc_sel1= -1;
                                nuc_sel2= -1;
				select_state=0;
                                break;
			default:
				break;
		};
	}
	if(modified)
                ReDraw(pagecan,canvas_paint_window(pagecan),NULL);
	return;
}

HiLite(nuc)
int nuc;
{
	if(nuc == -1) return;
	baselist[nuc].attr |= HILITE;
	if(baselist[nuc].depth<ddepth)
		gprint(baselist[nuc],canvas_paint_window(pagecan));
	return;
}

UnHiLite(nuc)
int nuc;
{
	if(nuc == -1) return;
	baselist[nuc].attr &= ~HILITE;
	if(baselist[nuc].depth<ddepth)
			gprint(baselist[nuc],canvas_paint_window(pagecan));
	return;
}

FindNuc(x,y)
double x,y;
{
	double sqrt();
	int i,best=0;
	double d,dmin=999.999;

	for(i=0;i<seqlen;i++)
	{
		d=distance(x,y,baselist[i].x,baselist[i].y);
		if (d<dmin && baselist[i].depth<ddepth)
		{
			dmin=d;
			best=i;
		}
	}
	if(dmin>3.0) best= -1;
	return(best);
}


SetDist(nuc1,nuc2,x,y)
int nuc1,nuc2;
double x,y;
{
	int i,n1,n2;
	double d,sqrt();

	n1=Min(nuc1,nuc2);
	n2=Max(nuc1,nuc2);

	if(n1==nuc1)
	{
		d = distance(x,y,baselist[n1].x,baselist[n1].y);
	}
	else
	{
		d = distance(x,y,baselist[n2].x,baselist[n2].y);
	}

	baselist[n1].dforw.pair=n2;
	baselist[n1].dforw.dist=d;

	baselist[n2].dback.pair=n1;
	baselist[n2].dback.dist= -d;
	modified = TRUE;
}




SetPos(nuc1,nuc2,x,y)
int nuc1,nuc2;
double x,y;
{
	int i,nuc,pair;
	double dx,dy,sqrt();

	dx = x-baselist[nuc1].x;
	dy = y-baselist[nuc1].y;

	nuc=nuc1;
	pair=nuc2;

	if(baselist[nuc].posnum < 0)
	{
		baselist[nuc].posnum = 0;
		baselist[nuc].pos = (Pcon*)calloc(sizeof(Pcon),10);
	}
	if(baselist[nuc].posnum < 10)
	{
		baselist[nuc].pos[baselist[nuc].posnum].pair=pair;
		baselist[nuc].pos[baselist[nuc].posnum].dx=dx;
		baselist[nuc].pos[baselist[nuc].posnum++].dy=dy;
		modified = TRUE;
	}
	else
	{
		fprintf(stderr,"%d Only 10 positional constraints per base\n"
		,nuc);
	}
	return;
}




SetAng(nuc1,nuc2,x,y)
int nuc1,nuc2;
double x,y;
{
	int i,j;
	double dist,theta;
	double sqrt(),sin(),cos(),atan2();

	dist = distance(baselist[nuc1].x,baselist[nuc1].y,
	baselist[nuc2].x,baselist[nuc2].y);

	theta = atan2(y-baselist[nuc1].y,x-baselist[nuc1].x);

	x=baselist[nuc1].x + dist*cos(theta);
	y=baselist[nuc1].y + dist*sin(theta);

	SetPos(nuc1,nuc2,x,y);

	return;
}


PosConFix(base)
Base base;
{
	int i;

	for(i=0;i<base.posnum;i++)
	{
		baselist[base.pos[i].pair].x = base.x+base.pos[i].dx;
		baselist[base.pos[i].pair].y = base.y+base.pos[i].dy;
		baselist[base.pos[i].pair].known = TRUE;
		if(baselist[base.pos[i].pair].posnum >0)
			PosConFix(baselist[base.pos[i].pair]);
	}
	return;
}


UnTangle(n1,n2)
int n1,n2;
{
	int j,pair,nhelix=0,helx[25];
	double dist,sqrt(),atan2(),sin(),cos(),theta,dtheta,theta1;
	double dist2,dx,dy;
	Base *b1,*b2,*b3,*b4;


	b1 = &(baselist[n1]);
	b2 = &(baselist[n2]);
	for(j=n1+1;j < n2;)
	{
		pair=baselist[j].pair;
		if(pair>j)
		{
			helx[nhelix++]=j;
			UnTangle(j,pair);
			j=pair;
		}
		else
			j++;
	}
	if(nhelix < 2) return;

	for(j=0;j<nhelix;j++)
	{
		b3 = &(baselist[helx[j]]);
		b4 = &(baselist[b3->pair]);

		dist = distance(b2->x,b2->y,b4->x,b4->y);
		dist2 = distance(b3->x,b3->y,b2->x,b2->y);
		if(j<(nhelix/2))
		{
			b4->dforw.dist = dist2;
			b4->dforw.pair = n2;
		}
		else
		{
			if(b3->pair != n2)
			{
				b3->dforw.dist = dist;
				b3->dforw.pair = n2;
			}
			else
			{
				b1->dforw.dist = dist2;
				b1->dforw.pair = helx[j];
			}
		}

	}
	return;
}

RemoveCon(nuc1,nuc2)
int nuc1,nuc2;
{
	int i,j;
	Base *b1;

	b1 = &(baselist[nuc1==(-1) ? nuc2:nuc1]);
	switch(select_state)
	{
		case 1:
			b1->dforw.pair = -1;
			b1->dforw.dist=0.0;
			b1->posnum = -1;
			cfree(b1->pos);
			break;

		case 2:
			break;

		case 3:
			for(j=nuc1;j<=nuc2;j++)
			{
				b1 = &(baselist[j]);
				b1->dforw.pair = -1;
                        	b1->dforw.dist = 0.0;
                        	b1->posnum = -1;
                        	cfree(b1->pos);
			}
			break;

		default:
			break;

	};
	modified = TRUE;
	return;
}


StackHel(nuc1,nuc2)
int nuc1,nuc2;
{
	int j,k,branches=0,other_end,start,end;
	Base *base,*next;
	double dist1,dist2,sqrt(),fabs();

	start=Min(nuc_sel1,nuc_sel2);
	end=Max(nuc_sel1,nuc_sel2);
	for(j=start;j<=end;j++)
	{
		base = &baselist[j];
		if(base -> pair > j )
		{
			branches=0;
			for(k=j+1;k!= base->pair;)
			{
				if(baselist[k].pair <k)
					k++;
				else
				{
					branches++;
					other_end=k;
					k=baselist[k].pair;
				}
			}

			if(branches == 1)
			{
				next = &baselist[other_end];
				dist1=distance(base->x,base->y,
				next->x,next->y);

				next = &(baselist[next->pair]);
				base = &(baselist[base->pair]);
				dist2=distance(base->x,base->y,
                                       next->x,next->y);

				if(fabs(dist1-dist2)>.001)
				{
					if(dist1>dist2)
						dist1=dist2;

					if(dist1 < BASE_TO_BASE_DIST * 1.5)
						dist1*= 1.5;

					next->dforw.pair = baselist[j].pair;
					next->dforw.dist = dist1;
					next = &baselist[other_end];
					base = &baselist[j];
					base->dforw.pair = other_end;
					base->dforw.dist = dist1;
					modified = TRUE;

				}
			}
			base = &baselist[j];
		}
	}
	return;
}


PlaceLabel(label,nucnum,pw)
Label *label;
int nucnum;
Pixwin *pw;
{
	Base *b;
	int j;
	double x,y,theta,xc,yc,sin(),cos(),atan2();

	b = &(baselist[nucnum]);
	xc = (double)(CWID*strlen(label->text))*.5 / xscale;
	yc = (double)(CHT) *.5 / yscale;

	if(label -> distflag ==FALSE)
	{
		printf("error.\n");
		x = b->x-xc+label->dx;
		y = b->y+yc+label->dy;
	}
	else
	{
		if(nucnum == 0)
			theta = atan2(b->y - baselist[1].y,
				b->x - baselist[1].x);
		else if(nucnum == seqlen-1)
			theta = atan2(baselist[seqlen-2].y - b->y,
				baselist[seqlen-2].x - b->x);
		else
		{
			theta = atan2(baselist[nucnum-1].y -
				baselist[nucnum+1].y,
				baselist[nucnum-1].x -
				baselist[nucnum+1].x);
			theta += (baselist[nucnum].dir == CW)?
				PI_o2 : -PI_o2;
		}

		x = b->x+label->dist*cos(theta)-xc;
		y = b->y+label->dist*sin(theta)-yc;
	}
	label -> x=x;
	label -> y=y;
	x = x*xscale+xoffset;
	y = y*yscale+yoffset;
	pw_text(pw,(int)x,(int)y,(PIX_SRC|PIX_DST),NULL,label->text);

	x = b->x+label->dist*cos(theta)-xc;
	y = b->y+label->dist*sin(theta)-yc;

/*
	line((x+.15*(b->x-x)),(y+.15*(b->y-y)),
		(x+.85*(b->x-x)),(y+.85*(b->y-y)),1,pw);
*/
	return;
}


IsLoop(n1,n2)
int n1,n2;
{
	int j, success = FALSE, last,step = 1;

	if(n1>n2)
	{
		j=n2;
		n2=n1;
		n1=j;
	}

	if(baselist[n1].pair != -1)
	{
		last = n1;
		j = baselist[n1].pair;
		for(;(j!=n1) && (success == FALSE);)
        	{
                	if(j==n2)
                        	success = TRUE;
                	else
                	{
                        	if(baselist[j].pair != -1 &&
                        	baselist[j].pair != last)
                        	{
                        	        last = j;
                        	        j = baselist[j].pair;
                        	}
                        	else
                        	{
                        	        last = j;
                                	j++;
                        	}
                	}
        	}
	}

	j = n1+1;
	for(;(j!=n1) && (success == FALSE);)
	{
		if(j==n2)
			success = TRUE;
		else
		{
			if(baselist[j].pair != -1 &&
			baselist[j].pair != last)
			{
				last = j;
				j = baselist[j].pair;
			}
			else
			{
				last = j;
				j++;
			}
		}
	}
	return(success);
}

ShoCon()
{
	int i,j;
	double x1,x2,y1,y2;
	Base b;

	for (i=0;i<seqlen;i++)
	{
		b=baselist[i];
		if(b.dforw.pair != -1 && b.depth < ddepth)
		{
			x1 = b.x+(double)CWID/xscale*.5;
			y1 = b.y-(double)CHT/yscale*.5;
			x2 = baselist[b.dforw.pair].x+(double)CWID/xscale*.5;
			y2 = baselist[b.dforw.pair].y-(double)CHT/yscale*.5;

			line(x2+0.05*(x1-x2),y2+0.05*(y1-y2),
                	x2+0.95*(x1-x2),y2+0.95*(y1-y2),3,canvas_paint_window(pagecan));
		}
		if(b.posnum > 0 && b.depth < ddepth)
		{
			x1 = b.x+(double)CWID/xscale*.5;
                        y1 = b.y-(double)CHT/yscale*.5;

			for(j=0;j<b.posnum;j++)
			{
                            x2 = baselist[b.pos[j].pair].x+
				(double)CWID/xscale*.5;

                            y2 = baselist[b.pos[j].pair].y-
				(double)CHT/yscale*.5;

                            line(x2+0.05*(x1-x2),y2+0.05*(y1-y2),
                            x2+0.95*(x1-x2),y2+0.95*(y1-y2),6,canvas_paint_window(pagecan));
			}
		}
	}
	return;
}
