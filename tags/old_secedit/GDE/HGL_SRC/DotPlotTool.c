/********************************
 *
 *  To compile:
 *
 *  cc -o DotPlotTool plot.c HGLfuncs.c Alloc.c ChooseFile.c
 *     -lxview -lolgx -lX11
 *
 *  Notes: Set canvas width and height to fit the max_width when
 *         loading a dataset.  Change the viewable size by changing
 *         the viewable_length of the scrollbars.
 *
 ********************************/

#ifndef _GLOBAL_DEFS_H
#include "global_defs.h"
#define _GLOBAL_DEFS_H
#endif

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/keysymdef.h>
#include <X11/keysym.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/scrollbar.h>
#include <xview/xv_xrect.h>
#include <xview/rect.h>
#include <xview/cms.h>
#include <sys/types.h>
#include <xview/icon.h>
#include <xview/svrimage.h>

#define POS2(array, i, j) (array[2*(i)+(j)])

#define WHITE   0
#define BLACK   8
#define RED     7

#define min_size     1
#define max_size     4
#define min_width    1
#define min_filter   1
#define max_filter   100
#define min_cutoff   -7
#define max_cutoff   17

#define mmscore      0  /* WHITE */
#define pmscore      9  /* GREY */
#define mscore       8  /* BLACK */
#define window_size  2
#define margin       50

Frame     frame=NULL, prop_subframe;
Panel     panel_1;  /* panel_1 width should change with the size of the
		     * dataset.
		     */
Panel     width_panel=NULL; /* There is a limitation on the max size of a
                             * canvas.  When loading a large dataset,
			     * max_width may have to be reduced to fit into
			     * the canvas.  So, width_panel has to be saved
			     * to allow changes.
			     */
Canvas    canvas;
Xv_Window paint_win;
Display   *display;
GC        gc;
Window    xwin;
Cms       cms;
Scrollbar h_scrollbar, v_scrollbar;
int Xaxis, Yaxis;

int size    = 2;
int max_width, width;
int cutoff = 1;
int filter  = 1;
int direction = 0;  /* both / and \. */
             /*  1: \ or direct
		-1: / or reversed */
char footnote[256], heading[128];

int drawarea_min_y, drawarea_max_y;
unsigned long *colors;
char clear_mark = 'F', compd = 'N';
Sequence *tSeq;

int tmp_direction, tmp_size,tmp_width,tmp_filter, tmp_cutoff;
char tmp_compd;
int mark_x=0;  /* record the x,y of the paint window, */
int mark_y=0;  /* not the canvas or the view window.  */
int set_offset;/* The offset of the sequences being displayed. */
int AAmatr[256][256];

unsigned short plot_bits[] = {
#include "plot.icon"
};

main(argc, argv)
     int argc;
     char *argv[];
{
    int LoadHGLData();
    extern exit_proc();
    extern size_proc();
    extern width_proc();
    extern footer_proc();
    extern canvas_repaint_proc();
    extern Frame load_file();
    extern Load();
    extern show_prop_frame();
    extern dir_proc();
    extern compd_proc();
    extern ok_proc();
    extern filter_proc();
    extern cutoff_proc();
    extern ToDisplay();
    int i, layer, ii, jj, kk, ll;
    char GetBase();
    Icon  icon;
    Server_image  image;

    static Xv_singlecolor cms_colors[] = {
      {255, 255, 255}, /* WHITE */
    {128, 0, 255},     /* purple */
    {0, 0, 128},       /* navy blue */
    {0, 0, 255 },      /* blue */
    {0, 255, 0},       /* green */
    {255, 255, 0},     /* yellow */
    {255, 165, 0},     /* orange */
    {225, 0, 0},       /* RED */
    {0, 0, 0},         /* black  */
    {192, 192, 192},   /* GREY */
    {240, 240, 240},
    {210, 210, 210},
    {180, 180, 180},
    {150, 150, 150},
    {120, 120, 120},
    {90, 90, 90},
    {60,60,60},
    {30, 30, 30},
    {0, 0, 0}
  };

    FILE *fp;
    Rect *rect;
    Panel panel;
    int pam;
    static char aa[] = {"arndcqeghilkmfpstwyvbzx*"};
    static pam120[24][24] = {
       /*a  r  n  d  c  q  e  g  h  i  l  k  m  f  p  s  t  w  y  v  b  z  x  *  */
/*a*/	 3,-3,-1, 0,-3,-1, 0, 1,-3,-1,-3,-2,-2,-4, 1, 1, 1,-7,-4, 0, 1, 0,-1,-8,
/*r*/	-3, 6,-1,-3,-4, 1,-3,-4, 1,-2,-4, 2,-1,-5,-1,-1,-2, 1,-5,-3,-1, 0,-2,-8,
/*n*/	-1,-1, 4, 2,-5, 0, 1, 0, 2,-2,-4, 1,-3,-4,-2, 1, 0,-4,-2,-3, 4, 1,-1,-8,
/*d*/	 0,-3, 2, 5,-7, 1, 3, 0, 0,-3,-5,-1,-4,-7,-3, 0,-1,-8,-5,-3, 5, 3,-2,-8,
/*c*/	-3,-4,-5,-7, 9,-7,-7,-4,-4,-3,-7,-7,-6,-6,-4, 0,-3,-8,-1,-3,-4,-6,-4,-8,
/*q*/	-1, 1, 0, 1,-7, 6, 2,-3, 3,-3,-2, 0,-1,-6, 0,-2,-2,-6,-5,-3, 1, 5,-1,-8,
/*e*/	 0,-3, 1, 3,-7, 2, 5,-1,-1,-3,-4,-1,-3,-7,-2,-1,-2,-8,-5,-3, 3, 5,-1,-8,
/*g*/	 1,-4, 0, 0,-4,-3,-1, 5,-4,-4,-5,-3,-4,-5,-2, 1,-1,-8,-6,-2, 1,-1,-2,-8,
/*h*/	-3, 1, 2, 0,-4, 3,-1,-4, 7,-4,-3,-2,-4,-3,-1,-2,-3,-3,-1,-3, 2, 2,-2,-8,
/*i*/	-1,-2,-2,-3,-3,-3,-3,-4,-4, 6, 1,-3, 1, 0,-3,-2, 0,-6,-2, 3,-2,-2,-1,-8,
/*l*/	-3,-4,-4,-5,-7,-2,-4,-5,-3, 1, 5,-4, 3, 0,-3,-4,-3,-3,-2, 1,-3,-2,-2,-8,
/*k*/	-2, 2, 1,-1,-7, 0,-1,-3,-2,-3,-4, 5, 0,-7,-2,-1,-1,-5,-5,-4, 1, 0,-2,-8,
/*m*/	-2,-1,-3,-4,-6,-1,-3,-4,-4, 1, 3, 0, 8,-1,-3,-2,-1,-6,-4, 1,-3,-1,-2,-8,
/*f*/	-4,-5,-4,-7,-6,-6,-7,-5,-3, 0, 0,-7,-1, 8,-5,-3,-4,-1, 4,-3,-4,-5,-3,-8,
/*p*/	 1,-1,-2,-3,-4, 0,-2,-2,-1,-3,-3,-2,-3,-5, 6, 1,-1,-7,-6,-2,-1, 0,-2,-8,
/*s*/	 1,-1, 1, 0, 0,-2,-1, 1,-2,-2,-4,-1,-2,-3, 1, 3, 2,-2,-3,-2, 1, 0,-1,-8,
/*t*/	 1,-2, 0,-1,-3,-2,-2,-1,-3, 0,-3,-1,-1,-4,-1, 2, 4,-6,-3, 0, 1,-1,-1,-8,
/*w*/	-7, 1,-4,-8,-8,-6,-8,-8,-3,-6,-3,-5,-6,-1,-7,-2,-6,12,-2,-8,-5,-6,-5,-8,
/*y*/	-4,-5,-2,-5,-1,-5,-5,-6,-1,-2,-2,-5,-4, 4,-6,-3,-3,-2, 8,-3,-2,-4,-3,-8,
/*v*/	 0,-3,-3,-3,-3,-3,-3,-2,-3, 3, 1,-4, 1,-3,-2,-2, 0,-8,-3, 5,-2,-2,-1,-8,
/*b*/	 1,-1, 4, 5,-4, 1, 3, 1, 2,-2,-3, 1,-3,-4,-1, 1, 1,-5,-2,-2, 6, 4,-1,-8,
/*z*/	 0, 0, 1, 3,-6, 5, 5,-1, 2,-2,-2, 0,-1,-5, 0, 0,-1,-6,-4,-2, 4, 6, 0,-8,
/*x*/	-1,-2,-1,-2,-4,-1,-1,-2,-2,-1,-2,-2,-2,-3,-2,-1,-1,-5,-3,-1,-1, 0,-2,-8,
/***/	-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8, 1};

    static pam250[24][24] = {
   /*a  r  n  d  c  q  e  g  h  i  l  k  m  f  p  s  t  w  y  v  b  z  x  *  */
     2,-2, 0, 0,-2, 0, 0, 1,-1,-1,-2,-1,-1,-3, 1, 1, 1,-6,-3, 0, 2, 1, 0,-8,
    -2, 6, 0,-1,-4, 1,-1,-3, 2,-2,-3, 3, 0,-4, 0, 0,-1, 2,-4,-2, 1, 2,-1,-8,
     0, 0, 2, 2,-4, 1, 1, 0, 2,-2,-3, 1,-2,-3, 0, 1, 0,-4,-2,-2, 4, 3, 0,-8,
     0,-1, 2, 4,-5, 2, 3, 1, 1,-2,-4, 0,-3,-6,-1, 0, 0,-7,-4,-2, 5, 4,-1,-8,
    -2,-4,-4,-5,12,-5,-5,-3,-3,-2,-6,-5,-5,-4,-3, 0,-2,-8, 0,-2,-3,-4,-3,-8,
     0, 1, 1, 2,-5, 4, 2,-1, 3,-2,-2, 1,-1,-5, 0,-1,-1,-5,-4,-2, 3, 5,-1,-8,
     0,-1, 1, 3,-5, 2, 4, 0, 1,-2,-3, 0,-2,-5,-1, 0, 0,-7,-4,-2, 4, 5,-1,-8,
     1,-3, 0, 1,-3,-1, 0, 5,-2,-3,-4,-2,-3,-5, 0, 1, 0,-7,-5,-1, 2, 1,-1,-8,
    -1, 2, 2, 1,-3, 3, 1,-2, 6,-2,-2, 0,-2,-2, 0,-1,-1,-3, 0,-2, 3, 3,-1,-8,
    -1,-2,-2,-2,-2,-2,-2,-3,-2, 5, 2,-2, 2, 1,-2,-1, 0,-5,-1, 4,-1,-1,-1,-8,
    -2,-3,-3,-4,-6,-2,-3,-4,-2, 2, 6,-3, 4, 2,-3,-3,-2,-2,-1, 2,-2,-1,-1,-8,
    -1, 3, 1, 0,-5, 1, 0,-2, 0,-2,-3, 5, 0,-5,-1, 0, 0,-3,-4,-2, 2, 2,-1,-8,
    -1, 0,-2,-3,-5,-1,-2,-3,-2, 2, 4, 0, 6, 0,-2,-2,-1,-4,-2, 2,-1, 0,-1,-8,
    -3,-4,-3,-6,-4,-5,-5,-5,-2, 1, 2,-5, 0, 9,-5,-3,-3, 0, 7,-1,-3,-4,-2,-8,
     1, 0, 0,-1,-3, 0,-1, 0, 0,-2,-3,-1,-2,-5, 6, 1, 0,-6,-5,-1, 1, 1,-1,-8,
     1, 0, 1, 0, 0,-1, 0, 1,-1,-1,-3, 0,-2,-3, 1, 2, 1,-2,-3,-1, 2, 1, 0,-8,
     1,-1, 0, 0,-2,-1, 0, 0,-1, 0,-2, 0,-1,-3, 0, 1, 3,-5,-3, 0, 2, 1, 0,-8,
    -6, 2,-4,-7,-8,-5,-7,-7,-3,-5,-2,-3,-4, 0,-6,-2,-5,17, 0,-6,-4,-4,-4,-8,
    -3,-4,-2,-4, 0,-4,-4,-5, 0,-1,-1,-4,-2, 7,-5,-3,-3, 0,10,-2,-2,-3,-2,-8,
     0,-2,-2,-2,-2,-2,-2,-1,-2, 4, 2,-2, 2,-1,-1,-1, 0,-6,-2, 4, 0, 0,-1,-8,
     2, 1, 4, 5,-3, 3, 4, 2, 3,-1,-2, 2,-1,-3, 1, 2, 2,-4,-2, 0, 6, 5,-1,-8,
     1, 2, 3, 4,-4, 5, 5, 1, 3,-1,-1, 2, 0,-4, 1, 1, 1,-4,-3, 0, 5, 6, 0,-8,
     0,-1, 0,-1,-3,-1,-1,-1,-1,-1,-1,-1,-1,-2,-1, 0, 0,-4,-2,-1,-1, 0,-1,-8,
    -8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8, 1};

    /* malloc_debug(2); */

    if(argc == 1)
    {
	fprintf(stderr, "An input file is required.\n");
	fprintf(stderr, "Usage: %s filename [pam120|pam250(If AA seqs)]\n",
		argv[0]);
	exit(0);
    }

    set_offset = LoadHGLData(argv[1]);

    /* setup comparison AAmatrix. */

    for(ii = 0; ii < 256; ii++)
      for(jj = 0; jj < 256; jj++)
	AAmatr[ii][jj] = -100;

    pam = 250;
    if(argc > 2)
    {
	if(strcmp(argv[2], "pam120") == 0)
	  pam = 120;
	else if(strcmp(argv[2], "pam250") != 0)
	{
	    fprintf(stderr, "Incorrect pam name: %s\n", argv[2]);
	    exit(1);
	}
    }

    for(ii = 0; ii < strlen(aa); ii++)
      for(jj = 0; jj < strlen(aa); jj++)
      {
	  AAmatr[aa[ii]][aa[jj]] =
	    AAmatr[aa[ii]-32][aa[jj]-32] =
	      AAmatr[aa[ii]][aa[jj]-32] =
		AAmatr[aa[ii]-32][aa[jj]] =
		  (pam==250)?pam250[ii][jj]:pam120[ii][jj];
      }

    tmp_cutoff = cutoff;
    tmp_width = width;
    tmp_direction = direction;
    tmp_size = size;
    tmp_filter = filter;
    tmp_compd = compd;

    xv_init(XV_INIT_ARGS, argc, argv, NULL);

    frame = xv_create(XV_NULL, FRAME,
		      FRAME_LABEL, heading,
		      FRAME_SHOW_FOOTER, TRUE,
		      FRAME_LEFT_FOOTER, "Mouse Location:",
		      FRAME_RIGHT_FOOTER, footnote,
		      NULL);

    panel_1 = (Panel) xv_create(frame, PANEL,
				NULL);

    (void) xv_create(panel_1,PANEL_BUTTON,
		     PANEL_LABEL_STRING, "Properties...",
		     PANEL_NOTIFY_PROC, show_prop_frame,
		     NULL);

    (void) xv_create(panel_1,PANEL_BUTTON,
		     PANEL_LABEL_STRING, "Load",
		     PANEL_NOTIFY_PROC, Load,
		     NULL);

    (void) xv_create(panel_1,PANEL_BUTTON,
		     PANEL_LABEL_STRING, "Exit",
		     PANEL_NOTIFY_PROC, exit_proc,
		     NULL);

    window_fit_height(panel_1);

    canvas = (Canvas) xv_create(frame, CANVAS,
				CANVAS_X_PAINT_WINDOW, TRUE,
				CANVAS_REPAINT_PROC,   canvas_repaint_proc,
				CANVAS_AUTO_CLEAR,     FALSE,
				CANVAS_AUTO_SHRINK,    FALSE,
				CANVAS_AUTO_EXPAND,    FALSE,
				CANVAS_WIDTH, Xaxis*max_width+margin,
				CANVAS_HEIGHT,Yaxis*max_width+margin,
				XV_WIDTH, MIN(600, Xaxis*width)+margin,
				XV_HEIGHT,MIN(400, Yaxis*width)+margin,
				WIN_BELOW,             panel_1,
				CANVAS_RETAINED,       FALSE,
				NULL);
/*
    printf("Canvas_WIDTH=%d  _HEIGHT=%d\n",
	   (int)xv_get(canvas, CANVAS_WIDTH),
	   (int)xv_get(canvas, CANVAS_HEIGHT));
*/
    paint_win = (Xv_Window) canvas_paint_window(canvas);
    xv_set(paint_win,
	   WIN_BIT_GRAVITY, ForgetGravity,
           WIN_CONSUME_EVENTS,
	      MS_LEFT,
	      NULL,
	   WIN_EVENT_PROC,  footer_proc,
	   NULL);

    h_scrollbar = (Scrollbar) xv_create(canvas, SCROLLBAR,
		SCROLLBAR_DIRECTION, SCROLLBAR_HORIZONTAL,
		SCROLLBAR_OBJECT_LENGTH, Xaxis*width+margin,
		SCROLLBAR_VIEW_START, 0,
		NULL);

    v_scrollbar = (Scrollbar) xv_create(canvas, SCROLLBAR,
		SCROLLBAR_DIRECTION, SCROLLBAR_VERTICAL,
		SCROLLBAR_OBJECT_LENGTH, Yaxis*width+margin,
		SCROLLBAR_VIEW_START, 0,
		NULL);

    cms = xv_create(NULL, CMS,
		    CMS_SIZE,  19,
		    CMS_TYPE,  XV_DYNAMIC_CMS,
		    CMS_COLORS, cms_colors,
		    NULL);

    xv_set(canvas, WIN_CMS, cms,
	   WIN_INHERIT_COLORS,    FALSE,
	   WIN_BACKGROUND_COLOR,  WHITE,
	   NULL);

    window_fit(canvas);

    prop_subframe = (Frame)xv_create(frame, FRAME_CMD,
				     FRAME_LABEL, "Properties",
				     NULL);

    panel = (Panel)xv_get(prop_subframe, FRAME_CMD_PANEL);
    (void) xv_set(panel, PANEL_LAYOUT, PANEL_VERTICAL,NULL);

    (void) xv_create(panel, PANEL_SLIDER,
		     PANEL_LABEL_STRING, "Dot Size:         ",
		     PANEL_VALUE,     size,
		     PANEL_MIN_VALUE, min_size,
		     PANEL_MAX_VALUE, max_size,
		     PANEL_SLIDER_WIDTH, 100,
		     PANEL_TICKS, 4,
		     PANEL_NOTIFY_PROC, size_proc,
		     NULL);

    width_panel = xv_create(panel, PANEL_SLIDER,
		     PANEL_LABEL_STRING, "Dot Width:        ",
		     PANEL_VALUE,     width,
		     PANEL_MIN_VALUE, min_width,
		     PANEL_MAX_VALUE, max_width,
		     PANEL_SLIDER_WIDTH, 100,
		     PANEL_TICKS, 5,
		     PANEL_NOTIFY_PROC, width_proc,
		     NULL);

    (void) xv_create(panel, PANEL_SLIDER,
		     PANEL_LABEL_STRING, "Min Match Length: ",
		     PANEL_VALUE,     filter,
		     PANEL_MIN_VALUE, min_filter,
		     PANEL_MAX_VALUE, max_filter,
		     PANEL_SLIDER_WIDTH, 100,
		     PANEL_TICKS, 5,
		     PANEL_NOTIFY_PROC, filter_proc,
		     NULL);

    (void)xv_create(panel, PANEL_CHOICE,
		    PANEL_LABEL_STRING, "Match Direction",
		    PANEL_CHOICE_STRINGS,
		    "Direct",
		    "Reversed",
		    "Both",
		    NULL,
		    PANEL_NOTIFY_PROC, dir_proc,
		    PANEL_VALUE, 0,
		    NULL);

    (void)xv_create(panel, PANEL_CHOICE,
		    PANEL_LABEL_STRING, "Complemented(for NN only):",
		    PANEL_CHOICE_STRINGS, "No", "Yes", NULL,
		    PANEL_NOTIFY_PROC, compd_proc,
		    PANEL_VALUE, 0,
		    NULL);

    (void) xv_create(panel, PANEL_SLIDER,
		     PANEL_LABEL_STRING, "Cutoff PAM250 score(for AA only):",
		     PANEL_VALUE,     cutoff,
		     PANEL_MIN_VALUE, min_cutoff,
		     PANEL_MAX_VALUE, max_cutoff,
		     PANEL_SLIDER_WIDTH, 100,
		     PANEL_TICKS, 5,
		     PANEL_NOTIFY_PROC, cutoff_proc,
		     NULL);

    (void)xv_create(panel, PANEL_BUTTON,
		    PANEL_LABEL_STRING, "OK",
		    PANEL_NOTIFY_PROC, ok_proc,
		    NULL);

    window_fit(panel);
    window_fit(prop_subframe);

    colors = (unsigned long *)xv_get(canvas, WIN_X_COLOR_INDICES);

    display = (Display *)xv_get(paint_win, XV_DISPLAY);
    xwin = (Window)xv_get(paint_win, XV_XID);
    gc = DefaultGC(display, DefaultScreen(display));

    xv_set(panel_1,XV_WIDTH,MIN(600, Xaxis*width)+margin,NULL);
    window_fit(frame);

    rect = (Rect *)xv_get(canvas,CANVAS_VIEWABLE_RECT,paint_win);
    XClearArea(display, xwin,
	       rect->r_left, rect->r_top,
	       rect->r_width, rect->r_height,
	       0);

    image = (Server_image)xv_create(NULL, SERVER_IMAGE,
				    XV_WIDTH,  64,
				    XV_HEIGHT, 64,
				    SERVER_IMAGE_BITS, plot_bits,
				    NULL);

    icon = (Icon)xv_create(frame, ICON,
			   ICON_IMAGE,  image,
/*			   ICON_TRANSPARENT, TRUE,*/
			   WIN_FOREGROUND_COLOR, WHITE,
			   WIN_BACKGROUND_COLOR, BLACK,
	   XV_LABEL, "DotPlotTool",
			   NULL);

    xv_set(frame, FRAME_ICON, icon, NULL);

    xv_main_loop(frame);
}



int LoadHGLData(name_str)
char *name_str;
{
    int iSeq, ii, jj, kk, YY, tmp, cnt_match;
    FILE *fp;

    if(strcmp(name_str, "") == 0)
      return -1;

    if((fp = fopen(name_str, "r")) == NULL)
    {
	fprintf(stderr, "Can't open file %s.\n", name_str);
	exit(1);
    }

    iSeq = 0;
    tSeq = (Sequence *)Calloc(2, sizeof(Sequence));

    while(iSeq<2 && (ReadRecord(fp, &(tSeq[iSeq]))) != -1)
    {
	SeqNormal(&(tSeq[iSeq++]));
    };
    if(iSeq == 1)
    {
	CopyRecord(&(tSeq[1]), &(tSeq[0]));
	iSeq++;
    }

    fclose(fp);

    Xaxis = strlen(tSeq[0].c_elem);
    Yaxis = strlen(tSeq[1].c_elem);
    max_width = MIN(6, MAX(1, MIN(28000/Xaxis, 28000/Yaxis)));
    width = MAX(1, max_width/2);
    if(width_panel != NULL)
      xv_set(width_panel, PANEL_MAX_VALUE, max_width, NULL);

    sprintf(heading, "PLOT : %s", name_str);
    sprintf(footnote, "X-axis: %s   Y-axis: %s ",
	    (tSeq[0].name[0]!='\0')?tSeq[0].name:tSeq[0].sequence_ID,
	    (tSeq[1].name[0]!='\0')?tSeq[1].name:tSeq[1].sequence_ID);

    if(frame != NULL)
    {
	XClearWindow(display, xwin);

	(void)xv_set(canvas,
		     XV_WIDTH, MIN(600, Xaxis*width)+margin,
		     XV_HEIGHT,MIN(400, Yaxis*width)+margin,
		     CANVAS_WIDTH, Xaxis*max_width+margin,
		     CANVAS_HEIGHT,Yaxis*max_width+margin,
		     NULL);

	xv_set(panel_1,XV_WIDTH,(int)xv_get(canvas,XV_WIDTH),NULL);
	window_fit(frame);

	xv_set(frame,FRAME_RIGHT_FOOTER, footnote, NULL);

	(void)xv_set(h_scrollbar,
		     SCROLLBAR_OBJECT_LENGTH, Xaxis*width+margin,
		     SCROLLBAR_VIEW_START, 0,
		     NULL);

	(void)xv_set(v_scrollbar,
		     SCROLLBAR_OBJECT_LENGTH, Yaxis*width+margin,
		     SCROLLBAR_VIEW_START, 0,
		     NULL);

	mark_x = mark_y = 0;
	canvas_repaint_proc(canvas,paint_win,display, xwin, NULL);
    }
}


DrawDot(display, xwin, gc, x, y, size)
Display *display;
Window xwin;
GC gc;
int x, y, size;
{
    int ii, jj;

    switch(size)
    {
      case 1:
	XDrawPoint(display, xwin, gc, x, y);
	break;
      case 2:
	XDrawLine(display, xwin, gc, x,y+1, x+2,y+1);
	XDrawLine(display, xwin, gc, x+1,y, x+1,y+2);
	break;
      case 3:
	XDrawLine(display, xwin, gc, x,y+1, x+3,y+1);
	XDrawLine(display, xwin, gc, x,y+2, x+3,y+2);
	XDrawLine(display, xwin, gc, x+1,y, x+1,y+3);
	XDrawLine(display, xwin, gc, x+2,y, x+2,y+3);
	break;
      case 4:
	XDrawLine(display, xwin, gc, x,y+1, x+4,y+1);
	XDrawLine(display, xwin, gc, x,y+2, x+4,y+2);
	XDrawLine(display, xwin, gc, x,y+3, x+4,y+3);
	XDrawLine(display, xwin, gc, x+1,y, x+1,y+4);
	XDrawLine(display, xwin, gc, x+2,y, x+2,y+4);
	XDrawLine(display, xwin, gc, x+3,y, x+3,y+4);
	break;
      default:
	fprintf(stderr,
		"Dot size %d is not implemented. 2 is used.\n",size);
	XDrawLine(display, xwin, gc, x,y+1, x+2,y+1);
	XDrawLine(display, xwin, gc, x+1,y, x+1,y+2);
	break;
    }
}



canvas_repaint_proc(canvas,pw,display, xwin, xrects )
     Canvas canvas;
     Xv_Window pw;
     Display *display;
     Window xwin;
     Xv_xrectlist *xrects;
{
    int y, iSeq, iSeg, i, cnt, head, tail;
    XGCValues gc_val;
    Rect *rect;
    int drawarea_min_x = INT_MAX;
    int drawarea_max_x = 0;
    int ii, jj, prev_color = -99, tmp_color;

    if(xrects == NULL)
    {
	if(clear_mark == 'F')
	{
	    rect=(Rect *)xv_get(canvas,CANVAS_VIEWABLE_RECT,paint_win);
	    drawarea_min_y = rect->r_top;
	    drawarea_max_y = drawarea_min_y + rect->r_height;
	}

	rect=(Rect *)xv_get(canvas,CANVAS_VIEWABLE_RECT,paint_win);
	drawarea_min_x = MAX(0, rect->r_left -1);
	drawarea_max_x = rect->r_left + rect->r_width;

	/*
	printf("min_x=%d  max_x=%d  min_y=%d  max_y=%d\n",
	       drawarea_min_x,drawarea_max_x,
	       drawarea_min_y,drawarea_max_y);
	*/

	for(ii=MAX(drawarea_min_y/width, 0);
	    ii<MIN(drawarea_max_y/width+1, Yaxis);
	    ii++)
	{
	    for(jj = MAX(drawarea_min_x/width, 0);
		jj<MIN(drawarea_max_x/width+1, Xaxis);
		jj++)
	    {
		if((tmp_color=ToDisplay(tSeq, jj, ii)) == WHITE)
		  continue;

		if(tmp_color != prev_color)
		{
		    XSetForeground(display,gc,colors[tmp_color]);
		    prev_color = tmp_color;
		}
		DrawDot(display, xwin, gc,
			jj * width,
			ii * width,
			size);
	    }
	}
    }
    else
    {
	for(cnt=xrects->count-1; cnt>=0; cnt--)
	{
	    drawarea_min_y = xrects->rect_array[cnt].y;
	    drawarea_max_y = xrects->rect_array[cnt].y +
	      xrects->rect_array[cnt].height;

	    drawarea_min_x = xrects->rect_array[cnt].x;
	    drawarea_max_x = xrects->rect_array[cnt].x +
	      xrects->rect_array[cnt].width;

	    /*
	    printf("else: min_x=%d  max_x=%d  min_y=%d  max_y=%d\n",
		   drawarea_min_x,drawarea_max_x,
		   drawarea_min_y,drawarea_max_y);
	    */

	    for(ii=MAX(drawarea_min_y/width, 0);
		ii<MIN(drawarea_max_y/width+1, Yaxis);
		ii++)
	    {
		for(jj = MAX(drawarea_min_x/width, 0);
		jj<MIN(drawarea_max_x/width+1, Xaxis);
		    jj++)
		{

		    if((tmp_color=ToDisplay(tSeq, jj, ii)) == WHITE)
		      continue;

		    if(tmp_color != prev_color)
		    {
			XSetForeground(display,gc,colors[tmp_color]);
			prev_color = tmp_color;
		    }
		    DrawDot(display, xwin, gc,
			    jj * width,
			    ii * width,
			    size);
		}
	    }
	}
    }

    /*  Draw marker if (mark_x, mark_y) is not the one
     *  just being deleted.
     */
    if(clear_mark == 'F')
    {
	XSetForeground(display, gc, colors[RED]);
	cross(size, mark_x, mark_y);
    }
    else
      clear_mark = 'F';

    return XV_OK;
}



/* Find the location of the mouse and print the
 * results in the footer of the frame.
 */

footer_proc ( paint_win, event, arg )
     Xv_Window paint_win;
     Event *event;
     Notify_arg arg;
{
    char buf[125];
    int save_line, i, ii, iSeq, iSeg;
    int  return_len=10;
    char return_str[10];
    KeySym keysym;
    XEvent *xevent;
    int save_mark_x = mark_x;
    int save_mark_y = mark_y;
    char need_to_paint = 'F';
    char loc_name[32];
    int loc_col = INT_MAX, ii1, ii2;
    char cc1, cc2;
    Rect *rect;

    if (event_id(event) == LOC_WINENTER)
    {
	win_set_kbd_focus(canvas, xwin);
    }
    else if(event_id(event) == LOC_WINEXIT)
    {
	win_refuse_kbd_focus(canvas);
    }

/*    else if(event_id(event) == LOC_MOVE) */
    if (event_is_down(event) &&
	event_action(event)==ACTION_SELECT)
    {
	mark_x = event_x(event);
	mark_y = event_y(event);
	need_to_paint = 'T';

	if((ii1 = mark_x/width) >= Xaxis)
	{
	    ii1 = Xaxis;
	    cc1 = ' ';
	}
	else
	  cc1 = tSeq[0].c_elem[ii1];

	if((ii2 = mark_y/width) >= Yaxis)
	{
	    ii2 = Yaxis;
	    cc2 = ' ';
	}
	else
	  cc2 = tSeq[1].c_elem[ii2];

	sprintf(buf,
		"Mouse location: X=(%c, %d)   Y=(%c, %d)  ",
		cc1, ii1, cc2, ii2);

	xv_set(frame,FRAME_LEFT_FOOTER, buf, NULL);
    }

    if(need_to_paint == 'T')
    {
	/**
	 ** clear the old mark.
	 **/

	XSetForeground(display, gc, colors[WHITE]);
	cross(size, save_mark_x, save_mark_y);

	drawarea_min_y = save_mark_y - 10;
	drawarea_max_y = save_mark_y + 15;

	clear_mark = 'T';

	canvas_repaint_proc(canvas,paint_win,display, xwin, NULL);

	/**
	 ** put a new mark.
	 **/

	XSetForeground(display, gc, colors[RED]);
	cross(size, mark_x, mark_y);
    }
    return XV_OK;
}



Load(item,event)
     Panel_item item;
     Event *event;
{
    (void)load_file(frame,300,150,NULL);
    return XV_OK;
}


size_proc(item, i_sz, event)
     Panel_item item;
     int i_sz;
     Event *event;
{
    tmp_size = i_sz;
    return XV_OK;
}


width_proc(item, i_wd, event)
     Panel_item item;
     int i_wd;
     Event *event;
{
    tmp_width = i_wd;
    return XV_OK;
}


filter_proc(item, i_fl, event)
     Panel_item item;
     int i_fl;
     Event *event;
{
    tmp_filter = i_fl;
    return XV_OK;
}


cutoff_proc(item, i_ct, event)
     Panel_item item;
     int i_ct;
     Event *event;
{
    tmp_cutoff = i_ct;
    return XV_OK;
}


exit_proc(item, event)
     Panel_item item;
     Event *event;
{
    if (event_action(event) == ACTION_SELECT)
    {
	xv_destroy_safe(frame);
	return(XV_OK);
    }
    else
      return(XV_ERROR);
}


show_prop_frame(item, event)
Frame item;
Event *event;
{
    xv_set(prop_subframe, XV_SHOW, TRUE, NULL);
}


cross(size, loc_x, loc_y)
     int size, loc_x, loc_y;
{
    int ii;

    if(size == 1)
    {
	XDrawLine(display,xwin,gc, loc_x-2,loc_y,  loc_x+2,loc_y);
	XDrawLine(display,xwin,gc, loc_x,  loc_y-2,loc_x,  loc_y+2);
    }
    else if(size == 2)
    {
	for(ii=0; ii<2; ii++)
	{
	    XDrawLine(display,xwin,gc,loc_x-2,loc_y+ii,loc_x+3,loc_y+ii);
	    XDrawLine(display,xwin,gc,loc_x+ii,loc_y-2,loc_x+ii,loc_y+3);
	}
    }
    else
    {
	for(ii= -1; ii<2; ii++)
	{
	    XDrawLine(display,xwin,gc,loc_x-4,loc_y+ii,loc_x+4,loc_y+ii);
	    XDrawLine(display,xwin,gc,loc_x+ii,loc_y-4,loc_x+ii,loc_y+4);
	}
    }
}


int ScoreMatch(a,b)
     char a,b;
{
    int i, intc;
    char c;
    static int tmatr[16] = {'-','a','c','m','g','r','s','v',
                              't','w','y','h','k','d','b','n'};

    static int matr[128] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x00,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x01,0xe,0x02,0x0d,0,0,0x04,0x0b,0,0,0x0c,0,0x03,0x0f,0,0x05,0,0x05,0x06,
        0x08,0x08,0x07,0x09,0x00,0xa,0,0,0,0,0,0,0,0x01,0x0e,0x02,0x0d,0,0,0x04,
        0x0b,0,0,0x0c,0,0x03,0x0f,0,0x05,0,0x05,0x06,0x08,0x08,0x07,0x09,0x00,0x0a,
        0,0,0,0,0x00,0
      };

    if(a == '-' || b == '-') return(mscore);

    if(matr[a] == matr[b]) return(mscore);

    intc = matr[a] & matr[b];

    if((intc==matr[a]) || (intc==matr[b])) return(pmscore);

    return(mmscore);
}


int ToDisplay(tSeq, x, y)
Sequence *tSeq; /* 0 and 1 sequences. */
int x, y;       /* axes. */
                /* return the color to draw. */
{
    int ii, jj, kk, cnt;
    char this_score;

    if(strcmp(tSeq[0].type, "PROTEIN") == 0 &&
       strcmp(tSeq[1].type, "PROTEIN") == 0)
    {
	if((this_score=AAmatr[tSeq[0].c_elem[x]][tSeq[1].c_elem[y]])
	   < cutoff)
	  return WHITE;

	if(direction == 0 || direction == 1)
	{
	    cnt = 1;
	    for(kk = 1;
		cnt < filter &&
		kk < filter &&
		x - kk >= 0 && y - kk >= 0 &&
		AAmatr[tSeq[0].c_elem[x-kk]][tSeq[1].c_elem[y-kk]] >= cutoff;
		kk++, cnt++);

	    for(kk = 1;
		cnt < filter &&
		kk < filter &&
		x + kk < Xaxis && y + kk < Yaxis &&
		AAmatr[tSeq[0].c_elem[x+kk]][tSeq[1].c_elem[y+kk]] >= cutoff;
		kk++, cnt++);

	    if(cnt >= filter)
	      return ((this_score+9)/4+1 +11);

	    if(direction == 1)
	      return WHITE;
	}

	if(direction == 0 || direction == -1)
	{
	    cnt = 1;
	    for(kk = 1;
		cnt < filter &&
		kk < filter &&
		y + kk < Yaxis && x - kk >= 0 &&
		AAmatr[tSeq[0].c_elem[x-kk]][tSeq[1].c_elem[y+kk]] >= cutoff;
		kk++, cnt++);

	    for(kk = 1;
		cnt < filter &&
		kk < filter &&
		y - kk >= 0 && x + kk < Xaxis &&
		AAmatr[tSeq[0].c_elem[x+kk]][tSeq[1].c_elem[y-kk]] >= cutoff;
		kk++, cnt++);

	    if(cnt >= filter)
	      return ((this_score+9)/4+1 +11); /* +11 to use grey scale.*/
	    return WHITE;
	}
    }
    else if((strcmp(tSeq[0].type, "DNA") == 0 || strcmp(tSeq[0].type, "RNA") == 0 || strcmp(tSeq[0].type, "TEXT") == 0) &&
	    (strcmp(tSeq[1].type, "DNA") == 0 || strcmp(tSeq[1].type, "RNA") == 0 || strcmp(tSeq[1].type, "TEXT") == 0))
    {
	if((this_score=ScoreMatch(tSeq[0].c_elem[x],GetBase(tSeq[1],y)))
	   == mmscore)
	  return WHITE;

	if(direction == 0 || direction == 1)
	{
	    cnt = 1;
	    for(kk = 1;
		cnt < filter &&
		kk < filter &&
		x - kk >= 0 && y - kk >= 0 &&
		ScoreMatch(tSeq[0].c_elem[x-kk],GetBase(tSeq[1],y-kk))
		!= mmscore;
		kk++, cnt++);

	    for(kk = 1;
		cnt < filter &&
		kk < filter &&
		x + kk < Xaxis && y + kk < Yaxis &&
		ScoreMatch(tSeq[0].c_elem[x+kk],GetBase(tSeq[1],y+kk))
		!= mmscore;
		kk++, cnt++);

	    if(cnt >= filter)
	      return this_score;

	    if(direction == 1)
	      return WHITE;
	}

	if(direction == 0 || direction == -1)
	{
	    cnt = 1;
	    for(kk = 1;
		cnt < filter &&
		kk < filter &&
		y + kk < Yaxis && x - kk >= 0 &&
		ScoreMatch(tSeq[0].c_elem[x-kk],GetBase(tSeq[1],y+kk))
		!= mmscore;
		kk++, cnt++);

	    for(kk = 1;
		cnt < filter &&
		kk < filter &&
		y - kk >= 0 && x + kk < Xaxis &&
		ScoreMatch(tSeq[0].c_elem[x+kk],GetBase(tSeq[1],y-kk))
		!= mmscore;
		kk++, cnt++);

	    if(cnt >= filter)
	      return this_score;
	    return WHITE;
	}
    }
    else
    {
	fprintf(stderr, "%cCan't plot sequences with types %s vs. %s.\n",
		7, tSeq[0].type, tSeq[1].type);
	exit(1);
    }
}


dir_proc(item, ii, event)
Panel_item item;
int ii;
Event *event;
{
    if(ii == 0)
      tmp_direction = 1;
    else if(ii == 1)
      tmp_direction = -1;
    else
      tmp_direction = 0;

    return XV_OK;
}



compd_proc(item, ii, event)
Panel_item item;
int ii;
Event *event;
{
    if(ii == 0)
      tmp_compd = 'N';
    else
      tmp_compd = 'Y';

    return XV_OK;
}


ok_proc(item, event)
Panel_item item;
Event *event;
{
    direction = tmp_direction;
    compd  = tmp_compd;
    size   = tmp_size;
    filter = tmp_filter;
    cutoff = tmp_cutoff;

    XClearWindow(display, xwin);

    if(tmp_width != width)
    {
	mark_y = mark_y/width * tmp_width;
	mark_x = mark_x/width * tmp_width;
	width = tmp_width;

	(void)xv_set(h_scrollbar,
		     SCROLLBAR_OBJECT_LENGTH, Xaxis*width+margin,
		     SCROLLBAR_VIEW_START,
		     MIN(MAX(0,mark_x - 100),
			 MAX(0,Xaxis*width+margin-(int)xv_get(canvas,XV_WIDTH))),
		     NULL);

	(void)xv_set(v_scrollbar,
		     SCROLLBAR_OBJECT_LENGTH, Yaxis*width+margin,
		     SCROLLBAR_VIEW_START,
		     MIN(MAX(0,mark_y - 100),
			 MAX(0,Yaxis*width+margin-(int)xv_get(canvas, XV_HEIGHT))),
		     NULL);
    }

    canvas_repaint_proc(canvas,paint_win,display, xwin, NULL);
    return XV_OK;
}


char BaseComp(c)
char c;
{
        unsigned char in,out, case_bit;
        static int tmatr[16] = {'-','a','c','m','g','r','s','v',
                        't','w','y','h','k','d','b','n'};

        static int matr[128] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0x00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0x01,0x0e,0x02,0x0d,0,0,0x04,0x0b,0,
	0,0x0c,0,0x03,0x0f,0,0x05,0,0x05,0x06,0x08,0x08,0x07,0x09,
	0x00,0x0a,0,0,0,0,0,0,0,0x01,0x0e,0x02,0x0d,0,0,0x04,0x0b,
	0,0,0x0c,0,0x03,0x0f,0,0x05,0,0x05,0x06,0x08,0x08,0x07,
	0x09,0x00,0x0a,0,0,0,0,0x00,0
        };

/*
*       Save Case bit...
*/
	case_bit = c & 32;
	out = 0;
	in = matr[c];
	if(in&1)
	  out|=8;
	if(in&2)
	  out|=4;
	if(in&4)
	  out|=2;
	if(in&8)
	  out|=1;

	c = tmatr[out] | case_bit;

        return c;
}


char GetBase(tSeq, y)
Sequence tSeq;
int y;
{
    if(compd == 'N')
      return tSeq.c_elem[y];
    if(compd == 'Y')
      return BaseComp(tSeq.c_elem[y]);
}
