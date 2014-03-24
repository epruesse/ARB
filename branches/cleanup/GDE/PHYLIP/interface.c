/* stderr */
/* Interface
   version 3.5c. (c) Copyright 1992-2000 by the University of Washington.
   Written by Sean T. Lamont and Michal Palczewski.
   For use with the Macintosh version of the Phylogeny Inference Package,
   Permission is granted to copy and use this program provided no fee is
   charged for it and provided that this copyright notice is not removed.

   Functions you need to know how to use:
   macsetup(char *name):  initializes the interface, brings up a window of
                          the name of the argument, for I/O.
   textmode(); hides the graphics window
   gfxmode(); shows the graphics window.
 */

#include <sioux.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "draw.h"
#include "interface.h"
#define MAX(a,b) (a) > (b) ? (a) : (b)

#define TEXT 0
#define GFX 1

extern winactiontype winaction;

extern long winheight;
extern long winwidth;
Rect rect = { 0, 0, 16000, 16000 }; /* a nice big rect very convenient */

/* These are all external variables from other files that this program needs
to access in order to draw and resize */
#define boolean char
extern long          strpbottom,strptop,strpwide,strpdeep,strpdiv,hpresolution;
extern boolean       dotmatrix,empty,preview,previewing,pictbold,pictitalic,
                     pictshadow,pictoutline;
extern double        expand,xcorner,xnow,xsize,xscale,xunitspercm,
                     ycorner,ynow,ysize,yscale,yunitspercm,labelrotation,
                     labelheight,xmargin,ymargin,pagex,pagey,paperx,papery,
                     hpmargin,vpmargin;
extern long          filesize;
extern growth        grows;
extern enum {yes,no} penchange,oldpenchange;
extern FILE          *plotfile;
extern plottertype   plotter,oldplotter,previewer;
extern striptype     stripe;
extern char             resopts;
 
extern double oldx, oldy;

extern boolean didloadmetric;
extern long   nmoves,oldpictint,pagecount;
extern double labelline,linewidth,oldxhigh,oldxlow,oldyhigh,oldylow,
       raylinewidth,treeline,oldxsize,oldysize,oldxunitspercm,
       oldyunitspercm,oldxcorner,oldycorner,oldxmargin,oldymargin,
       oldhpmargin,oldvpmargin,clipx0,clipx1,clipy0,clipy1,userxsize,userysize;
extern long rootmatrix[51][51];
extern long  HiMode,GraphDriver,GraphMode,LoMode,bytewrite;


void handlemouse(WindowPtr win,EventRecord ev);

/* Global variables used in many functions*/
int mode = TEXT;
int quitmac;
WindowPtr gfx_window;
ControlHandle plot_button;
ControlHandle change_button;
ControlHandle quit_button;
ControlHandle about_button;
Rect gfxBounds = { 50, 10, 400, 260 };        /* position and size of gfx_window */
Rect resizeBounds = {100+MAC_OFFSET,340,16000,16000}; /* how much the window can be resized*/

RGBColor background; 
RGBColor foreground;

/* saved parameters needed to make a call to makebox*/
mpreviewparams macpreviewparms;

/* initialize general stuff*/
void
macsetup (char *tname, char *gname)
{

  Str255 buf1, buf2;                        
  Str255 title = "";        
  Rect plot_rec={10,10,30,110}; 
  Rect change_rec={10,120,60,220};
  Rect quit_rec={40,10,60,110};
  Rect about_rec={10,230,30,330};
  unsigned char* plot=(unsigned char*)"\4Plot";
  unsigned char* change=(unsigned char*)"\7Change\rParameters";
  unsigned char* quit=(unsigned char*)"\4Quit";
  unsigned char* about=(unsigned char*)"\5About";
  change[0]=0x11;

  background.red=0xcc00; /* #ccffff phylip color */
  background.green=0xffff;
  background.blue=0xffff;
  
#undef fontsize
  SIOUXSettings.fontsize= log(qd.screenBits.bounds.right);
  SIOUXSettings.autocloseonquit = true;
 
  putchar('\n'); /* initialize sioux and let sioux initialize the toolbox and menus*/
  
  
  strcpy ((char *) buf1 + 1, tname);
  strcpy ((char *) buf2 + 1, gname);
  buf1[0] = strlen (tname);
  buf2[0] = strlen (gname);
  
  
  gfxBounds.bottom=qd.screenBits.bounds.bottom*.7;
  gfxBounds.right=MAX((gfxBounds.bottom-MAC_OFFSET)*.7,340);
  winheight=gfxBounds.bottom-gfxBounds.top-MAC_OFFSET;
  winwidth=gfxBounds.right-gfxBounds.left;
  
  
  gfx_window = NewCWindow (0L, &gfxBounds, buf2, false, documentProc,
                                                 (WindowPtr) - 1L, true, 0);
  plot_button = NewControl(gfx_window,&plot_rec,plot,1,0,0,1,pushButProc,0);
  change_button = NewControl(gfx_window,&change_rec,change,1,0,0,1,pushButProc,0);
  quit_button = NewControl(gfx_window,&quit_rec,quit,1,0,0,1,pushButProc,0);
  about_button = NewControl(gfx_window,&about_rec,about,1,0,0,1,pushButProc,0);
  
  foreground.red=0x0000;  /* black foreground */
  foreground.green=0x0000;
  foreground.blue=0x0000;
  
}

/* event loop for the preview window */
void
eventloop ()
{
  int status=1;
  quitmac=0;

  while (status > 0 && quitmac == 0)
        {
          status = handleevent ();
          if (status <= 0 || quitmac)
                textmode();

        }
}


/* event handler */
int
handleevent ()
{
  EventRecord ev;
  WindowPtr win;
  int SIOUXDidEvent,where, ok;
  ok = GetNextEvent (everyEvent, &ev);
  if (!ok) return 1;
  where = FindWindow (ev.where, &win);
  if (win != gfx_window || where  == inMenuBar) {
          SIOUXDidEvent = SIOUXHandleOneEvent(&ev);
          if (SIOUXDidEvent) return 1;
  }
  if (win != gfx_window) return 1;
  
  if ((ev.what == keyDown) &&
          (ev.modifiers & cmdKey) && 
          (toupper ((char) (ev.message & charCodeMask)) == 'W'))
        return 0;
  else if (ev.what == activateEvt) {
                InvalRect(&rect);
        }
  else if (ev.what == updateEvt && win == gfx_window )
        paint_gfx_window();
  else if (ev.what == mouseDown && where == inContent) 
    handlemouse(win,ev);
  else if (ev.what == mouseDown && where == inSysWindow)
        SystemClick (&ev, win);
  else if (ev.what == mouseDown && where == inDrag)
        DragWindow (win, ev.where, &rect);
  else if (ev.what == mouseDown && where == inGrow) 
          resize_gfx_window(ev);
  else if (ev.what == mouseDown && where == inGoAway)
        if ( TrackGoAway( win, ev.where ) ) {
                winaction = changeparms;
                return 0;        
        }
  if (ev.what == mouseDown ) {
          SelectWindow(win);
  }

  return 1;
}

/*Handle mouse down event */
void handlemouse(WindowPtr win,EventRecord ev) {
        Point mouse;
        ControlHandle control;
        int part;
        
        
        mouse=ev.where;
        GlobalToLocal(&mouse);
        part=FindControl(mouse,win,&control);
        if (part != 10) return;
        TrackControl(control,mouse,NULL);
        if (control == plot_button) {
                quitmac=1;
                winaction=plotnow;
        } else if (control == quit_button){
                quitmac=1;
            winaction=quitnow;
        } else if (control == change_button) {
                winaction = changeparms;
                quitmac=1;
        }
}

/* Draw a string to the graphics window */
void
putstring (string)
         char *string;
{
  unsigned char buf[256];
  strncpy ((char *) buf + 1, string, 253);        
  buf[0] = strlen (string);
  DrawString (buf);
}

/* go into text mode */
void
textmode ()
{
  HideWindow (gfx_window);
  mode = TEXT;
}

/* go into graphics mode */
void
gfxmode ()
{
  InitCursor();
  SetPort (gfx_window);
  ShowWindow (gfx_window);
  SelectWindow (gfx_window);
  
  mode = GFX;
}


/*call this function to paint the graphics window*/
void paint_gfx_window () {
        
        BeginUpdate(gfx_window);
        RGBBackColor(&background);
        RGBForeColor(&foreground);
          EraseRect(&rect);
  
          PenSize(1,1);        
          makebox(macpreviewparms.fn,
                 macpreviewparms.xo,
                  macpreviewparms.yo,
                  macpreviewparms.scale,
                 macpreviewparms.nt);
    PenSize(linewidth/5,linewidth/5);
          plottree(macpreviewparms.root, macpreviewparms.root);
          plotlabels(macpreviewparms.fn);
          UpdateControls(gfx_window,gfx_window->visRgn);
        EndUpdate(gfx_window);
          
          penchange = oldpenchange;
          xsize = oldxsize;
          ysize = oldysize;
          xunitspercm = oldxunitspercm;
          yunitspercm = oldyunitspercm;
          xscale = xunitspercm;
          yscale = yunitspercm;
          plotter = oldplotter;
          xcorner = oldxcorner;
        ycorner = oldycorner;
        xmargin = oldxmargin;
        ymargin = oldymargin;
          hpmargin = oldhpmargin;
          vpmargin = oldvpmargin;
        
}

/* resize the graphics window*/
void resize_gfx_window(EventRecord ev) {
        long windowsize;
          windowsize=GrowWindow(gfx_window,ev.where,&resizeBounds);
          if (windowsize != 0 ) {
                  SizeWindow(gfx_window,LoWord(windowsize),HiWord(windowsize),TRUE);
                 winheight=HiWord(windowsize)-MAC_OFFSET;
                 winwidth=LoWord(windowsize);
                 InvalRect(&rect);
         }
}
