#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/notice.h>
#include <stdio.h>
#include "loop.h"
#include "globals.h"

StyleProc(item,event)
Panel_item item;
Event *event;
{
}
    extern int COLOR;


EditProc(menu,menu_item)
Menu menu;
Menu_item menu_item;
{
    extern void ReDraw();
    int start,endd,i;
	char choice[80];

	strncpy(choice,xv_get(menu_item,MENU_STRING),79);

       if(strcmp(choice,"Clear constraint") == 0)
	{
            start=Min(nuc_sel1,nuc_sel2);
            endd=Max(nuc_sel1,nuc_sel2);
            RemoveCon(start,endd);
	}

       else if(strcmp(choice,"Show Constraints") == 0)
	{
            if(sho_con == TRUE)
                sho_con = FALSE;
            else
                sho_con = TRUE;

	    WINDOW_SIZE = Min(xv_get(canvas_paint_window(pagecan),XV_HEIGHT),
            xv_get(canvas_paint_window(pagecan),XV_WIDTH));
/*
*       Clear the page..
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
                        PIX_SRC);
            ReDraw(pagecan,canvas_paint_window(pagecan),NULL);
	}
       else if(strcmp(choice,"Distance Constraint") == 0)
            constraint_state = DISTANCE;
       else if(strcmp(choice,"Position Constraint") == 0)
            constraint_state = POSITIONAL;
       else if(strcmp(choice,"Angular Constraint") == 0)
            constraint_state = ANGULAR;
       else if(strcmp(choice,"Invert Helix") == 0)
	{
            if(select_state == 3)
            {
                start=Min(nuc_sel1,nuc_sel2);
                endd=Max(nuc_sel1,nuc_sel2);
                for(i=start+1;i<endd;i++)
                    baselist[i].dir *= -1;
                modified=TRUE;
            }
	}
       else if(strcmp(choice,"Stack Helix") == 0)
	{
            if(select_state == 3)
            {
                start=Min(nuc_sel1,nuc_sel2);
                endd=Max(nuc_sel1,nuc_sel2);
                StackHel(start,endd);
            }
	}

    if(modified)
        ReDraw(pagecan,canvas_paint_window(pagecan),NULL);
    return;
}


FileProc(menu,menu_item)
Menu menu;
Menu_item menu_item;
{
    int j,fsize;
    double xscale,yscale,xoffset,yoffset;    /* defined locally */
    double dx,dy,x,y,xc,yc;
    Base b1,b2;
    extern SaveTemp();
    FILE *lpr_file;
    char choice[80];

	strncpy(choice,xv_get(menu_item,MENU_STRING),79);
	if(strcmp(choice,"Save") == 0)
		SaveTemp();
	else if(strcmp(choice,"Print") == 0)
	{
            xscale=(double)(PRINT_WID) / (xmax-xmin);
            yscale=(double)(PRINT_HT) / (ymax-ymin);
            if(xscale<yscale)
                    yscale=xscale;
            else
                    xscale=yscale;
            xoffset=(-xmin)*xscale+20;
            yoffset=(-ymin)*yscale+20;

            fsize=baselist[0].size;
            lpr_file=fopen("loopout","w");


            fprintf(lpr_file,"%%!\ngsave\n.1 setlinewidth\n");
            fprintf(lpr_file,
            "/Courier findfont %d scalefont setfont\n",fsize);
            fprintf(lpr_file, "/p { moveto \n");
            fprintf(lpr_file, "show stroke} def\n");
            fprintf(lpr_file,"\n");

            for(j=0;j<seqlen;j++)
            {
                b1 = baselist[j];
                x = b1.x * xscale +xoffset;
                y = (double)PRINT_HT - b1.y * yscale - yoffset;
                xc = x - ( (double)fsize * 72.0/300.0);
                yc = y - ( (double)fsize * 72.0/300.0);

                if(b1.size != fsize)
                {
                    fsize = b1.size;
                    fprintf(lpr_file,
                    "/Courier findfont %d scalefont setfont\n"
                    ,fsize);
                }

                fprintf(lpr_file,"(%c)  %6.1f    %6.1f p\n",
                b1.nuc, xc, yc);
		if(b1.label != NULL)
		{
                    fprintf(lpr_file,
                    "/Courier findfont %d scalefont setfont\n"
                    ,fsize* 3/4);

		    fprintf(lpr_file,"%6.1f %6.1f moveto (%s)  show stroke\n",
		    b1.label->x*xscale + xoffset,PRINT_HT - (b1.label->y*yscale
		    + yoffset),b1.label->text);

                    fprintf(lpr_file,
                    "/Courier findfont %d scalefont setfont\n"
		    ,fsize);
		}

                if(b1.pair >j)
                {
                    char a,b;
                    b2 = baselist[baselist[j].pair];

                        dx = (b2.x - b1.x)*xscale;
                        dy = (b1.y - b2.y)*yscale;

                    a = b1.nuc | 32;
                    b = b2.nuc | 32;
                    if(
                    a == 'c' && b == 'g' ||
                    a == 'g' && b == 'c' ||
                    a == 'a' && (b == 'u' || b == 't') ||
                    (a == 'u' || a == 't') && b == 'a'
                    )
                    {
                        fprintf(lpr_file,
                        "%6.1f %6.1f %6.1f %6.1f moveto lineto\n",
                        x+.35*dx,y+.35*dy,x+.65*dx,y+.65*dy);
                    }
                    else if(
                    a == 'g' && b == 'u' ||
                    a == 'u' && b == 'g'
		    )
			fprintf(lpr_file,
                        "newpath\n%6.1f %6.1f %6.1f 0 360 arc fill\n"
			,x+.5*dx,y+.5*dy,xscale*.2);
		    else
			fprintf(lpr_file,
                        "%6.1f %6.1f %6.1f 0 360 arc\n"
                        ,x+.5*dx,y+.5*dy,xscale*.2);
                }
            }
            fprintf(lpr_file,"showpage\ngrestore\n");
            fclose(lpr_file);
	    system("lpr loopout");
		(void) notice_prompt( page, NULL,
				NOTICE_MESSAGE_STRINGS,
					"A PostScript copy has been sent to the default",
					"printer, and a copy of the PostScript file",
					"has been saved in a file `loopout'.",
					NULL,
				NOTICE_BUTTON, "OK", 1,
				NULL);

	}
	else if(strcmp(choice,"Quit") == 0)
		exit(0);
    	return;
}


SetDepth(item, value, event)
Panel_item item;
int value;
Event *event;
{
    ddepth=value*Mxdepth/90;
    WINDOW_SIZE = Min(xv_get(canvas_paint_window(pagecan),XV_HEIGHT),
    xv_get(canvas_paint_window(pagecan),XV_WIDTH));
/*
*       Clear the page..
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
                        PIX_SRC);
    ReDraw(pagecan,canvas_paint_window(pagecan),NULL);
    return;
}

PrintPair(j,lpr,xoffset,yoffset,xscale,yscale,x,y)
int j;
double xoffset,yoffset,xscale,yscale;
FILE *lpr;
{
    Base b1,b2;
    double dx,dy;
    char a,b;

    b1 = baselist[j];
    b2 = baselist[b1.pair];

    a = b1.nuc | 32;
    b = b2.nuc | 32;
    if(
    a == 'c' && b == 'g' ||
    a == 'g' && b == 'c' ||
    a == 'a' && (b == 'u' || b == 't') ||
    (a == 'u' || a == 't') && b == 'a'
    )
    {
        fprintf(stderr,"%d  ",j);
        dx = (b2.x - b1.x)*xscale;
        dy = (b1.y - b2.y)*yscale;

        b2 = baselist[baselist[j].pair];
        dx = (b2.x - b1.x)*xscale;
        dy = (b1.y - b2.y)*yscale;
        fprintf(lpr,
        "%6.1f %6.1f %6.1f %6.1f moveto lineto\n",
        x+.35*dx,y+.35*dy,x+.65*dx,y+.65*dy);
    }
    return;
}
