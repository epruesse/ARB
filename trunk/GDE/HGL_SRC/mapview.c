
/********************************
 *
 *  To compile:
 *
 *  cc -o mapview mapview.c HGLfuncs.c Alloc.c ChooseFile.c
 *     -lxview -lolgx -lX11
 *
 ********************************/

#define POS2(array, i, j) (array[2*(i)+(j)])

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
#include <sys/time.h>
#include <sys/stat.h>

#ifndef _GLOBAL_DEFS_H
#include "global_defs.h"
#define _GLOBAL_DEFS_H
#endif

#define min_pbl        1
#define max_pbl        15
#define min_scale      1
#define max_scale      1000
#define min_lwidth     1
#define max_lwidth     5
#define margin         100

#define WHITE   15
#define BLACK   8
#define RED     3
#define MASK_S2 5
#define MASK_S1 7
#define MASK_S0 10

Frame     frame, prop_subframe;
Canvas    canvas;
Xv_Window paint_win;
Display   *display;
GC        gc;
Window    xwin;
Cms       cms;
Scrollbar h_scrollbar;
Panel_item scale_slider, pbl_slider, lwidth_slider, color_chooser;
Menu color_menu;

int pbl       = 10;
int scale     = 2;
int lwidth    = 2;
int line_space= 12;
char FROM_RESIZE = 'F';

int max_dots  = 1000;
/* This value will be dynamically assigned.
 * It is used to decide the paint window size.
 */

int drawarea_min_y, drawarea_max_y;
unsigned long *colors;
char clear_mark = 'F';

int mark_x=0;  /* record the x,y of the paint window, */
int mark_y=0;  /* not the canvas or the view window.  */
int set_offset;/* The offset of the sequences being displayed. */
int save_for_mark_y;

/* wrapping up. */
int canvas_h, line_p_page;

typedef struct
{
    char name[32];
    int direction, strandedness, size, max_size;
    int orig_strand, orig_direction;
    int *dots;  /* Dynamic 2D array. */
} INFO;

INFO *info;
int info_size;

/* synchronization. */
char Lsync_fname[128];
char Lsync_YesNo = 'N';  /*Location synchronization Yes or No. */
char Csync_fname[128];
char Csync_YesNo = 'N';  /*Color synchronization Yes or No. */

main(argc, argv)
     int argc;
     char *argv[];
{
    Panel panel;
    int LoadHGLData();
    extern exit_proc();
    extern scale_proc();
    extern pbl_proc();
    extern footer_proc();
    extern canvas_repaint_proc();
    extern canvas_resize_proc();
    extern Frame load_file();
    extern Load();
    extern Lsync_proc();
/*    extern Csync_proc();*/
    extern show_prop_frame();
    extern lwidth_proc();
    extern l_arrow(), r_arrow();
    int i;

    static Xv_singlecolor cms_colors[] = {
        {0,128,0},
        {255,192,0},
        {255,0,255},
        {225,0,0},
        {0,192,192},
        {0,192,0},   /* green  */
        {0,0,255},
        {128,0,255}, /* purple */
        {0,0,0},
        {36,36,36},
        {72,72,72},
        {109,109,109},
        {145,145,145},
        {182,182,182},
        {218,218,218},
        {255,255,255}
    };

    char filename[128];
    FILE *fp;
    Rect *rect;

    /* malloc_debug(2); */

    for(i = 1; i<argc; i++)
    {
	if(strcmp(argv[i], "-npp") == 0 && i+1 < argc)
	{
	    i++;
	    scale = atoi(argv[i]);
	    if(scale>max_scale || scale<min_scale)
	    {
		fprintf(stderr,"Invalid -npp value: %s\n", argv[i]);
		exit(1);
	    }
	}
	else if(strcmp(argv[i], "-pbl") == 0 && i+1 < argc)
	{
	    i++;
	    pbl = atoi(argv[i]);
	    if(pbl>max_pbl || pbl<min_pbl)
	    {
		fprintf(stderr,"Invalid -pbl value: %s\n", argv[i]);
		exit(1);
	    }
	}
	else if(strcmp(argv[i], "-help") == 0)
	{
	    fprintf(stderr, "\n%s\n  %s\n\t%s\n\t%s\n\t%s\n\n",
		    "Usage:",
		    "mapview [-npp npp-value] [-pbl pbl-value] [file-name]",
		    "where:",
		    "npp-value: Integer.  Number of bases per pixel.",
		    "pbl-value: Integer.  Number of pixels between lines");
	    exit(0);
	}
	else
	{
	    strcpy(filename, argv[i]);
	}
    }
    strcpy(Lsync_fname, filename);
    strcat(Lsync_fname, ".Lsync");
/*    strcpy(Csync_fname, filename);
    strcat(Csync_fname, ".Csync");
*/
    xv_init(XV_INIT_ARGS, argc, argv, NULL);

    frame = xv_create(XV_NULL, FRAME,
     FRAME_LABEL, "",
     FRAME_SHOW_FOOTER, TRUE,
     FRAME_LEFT_FOOTER,
     "Mouse location: Line =         Position =         Column =",
     FRAME_RIGHT_FOOTER, "Sequence:     ",
     NULL);

    panel = (Panel) xv_create(frame, PANEL, NULL);

    (void) xv_create(panel,PANEL_BUTTON,
		     PANEL_LABEL_STRING, "Properties...",
		     PANEL_NOTIFY_PROC, show_prop_frame,
		     NULL);

    (void) xv_create(panel,PANEL_BUTTON,
		     PANEL_LABEL_STRING, "Load",
		     PANEL_NOTIFY_PROC, Load,
		     NULL);

    (void) xv_create(panel,PANEL_BUTTON,
		     PANEL_LABEL_STRING, "Exit",
		     PANEL_NOTIFY_PROC, exit_proc,
		     NULL);

    window_fit_height(panel);

    cms = xv_create(NULL, CMS,
		    CMS_SIZE,  16,
		    CMS_TYPE,  XV_DYNAMIC_CMS,
		    CMS_COLORS, cms_colors,
		    NULL);

    canvas = (Canvas) xv_create(frame, CANVAS,
				CANVAS_RETAINED,       FALSE,
				CANVAS_X_PAINT_WINDOW, TRUE,
				CANVAS_REPAINT_PROC,   canvas_repaint_proc,
				CANVAS_RESIZE_PROC,    canvas_resize_proc,
				CANVAS_AUTO_CLEAR,     FALSE,
				CANVAS_AUTO_SHRINK,    TRUE,
				CANVAS_AUTO_EXPAND,    TRUE,
				WIN_INHERIT_COLORS,    FALSE,
				WIN_BELOW,             panel,
				NULL);

    h_scrollbar = (Scrollbar) xv_create(canvas, SCROLLBAR,
		SCROLLBAR_DIRECTION, SCROLLBAR_HORIZONTAL,
		/* SCROLLBAR_OVERSCROLL,0, */
		SCROLLBAR_OBJECT_LENGTH, max_dots+margin,
		NULL);

    xv_set(canvas,
	   WIN_CMS,                cms,
	   WIN_BACKGROUND_COLOR,   WHITE,
	   NULL);

    xv_set(canvas_paint_window(canvas),
	   WIN_BIT_GRAVITY, ForgetGravity,
           WIN_CONSUME_EVENTS,
	      MS_LEFT,
	      WIN_RIGHT_KEYS,
	      WIN_ASCII_EVENTS,
	      LOC_WINENTER,
	      LOC_WINEXIT,
	      NULL,
	   WIN_EVENT_PROC,  footer_proc,
	   NULL);

    window_fit(canvas);

    prop_subframe = (Frame)xv_create(frame, FRAME_CMD,
				     FRAME_LABEL, "Properties",
				     NULL);

    panel = (Panel)xv_get(prop_subframe, FRAME_CMD_PANEL);
    (void) xv_set(panel, PANEL_LAYOUT, PANEL_VERTICAL,NULL);

    scale_slider = (Panel_item) xv_create(panel, PANEL_SLIDER,
		  PANEL_LABEL_STRING, "Nucleotides Per Pixel",
		  PANEL_VALUE,     scale,
		  PANEL_MIN_VALUE, min_scale,
		  PANEL_MAX_VALUE, max_scale,
		  PANEL_SLIDER_WIDTH, 100,
		  PANEL_TICKS, 5,
		  PANEL_NOTIFY_PROC, scale_proc,
		  NULL);

    pbl_slider = (Panel_item) xv_create(panel, PANEL_SLIDER,
		PANEL_LABEL_STRING, "Pixel Between Lines  ",
		PANEL_VALUE,     pbl,
		PANEL_MIN_VALUE, min_pbl,
		PANEL_MAX_VALUE, max_pbl,
		PANEL_SLIDER_WIDTH, 100,
		PANEL_TICKS, 5,
		PANEL_NOTIFY_PROC, pbl_proc,
		NULL);

    lwidth_slider = (Panel_item) xv_create(panel, PANEL_SLIDER,
		PANEL_LABEL_STRING, "Line Thickness         ",
		PANEL_VALUE,     lwidth,
		PANEL_MIN_VALUE, min_lwidth,
		PANEL_MAX_VALUE, max_lwidth,
		PANEL_SLIDER_WIDTH, 100,
		PANEL_TICKS, 5,
		PANEL_NOTIFY_PROC, lwidth_proc,
		NULL);

    /******
    color_menu = (Menu)xv_create(NULL, MENU,
				 MENU_NOTIFY_PROC, cl_menu_proc,
				 MENU_STRINGS, "Monochrome",
				 "Strandedness",
				 "Alignment color mask",
				 "Sequence color mask",
				 NULL,
				 NULL);

    color_chooser = (Panel_item)xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, "Color Type",
                PANEL_ITEM_MENU, color_menu,
		XV_KEY_DATA, FILE_NAME, filename,
		NULL);

    (void) xv_create(panel,PANEL_CHOICE,
		     PANEL_LABEL_STRING,   "Synchronization",
		     PANEL_CHOICE_STRINGS, "No", "Yes", NULL,
		     PANEL_NOTIFY_PROC,     Lsync_proc,
		     PANEL_VALUE,           0,
		     NULL);
    *******/

    window_fit(panel);
    window_fit(prop_subframe);

    colors = (unsigned long *)xv_get(canvas, WIN_X_COLOR_INDICES);

    paint_win = (Xv_Window) canvas_paint_window(canvas);
    display = (Display *)xv_get(paint_win, XV_DISPLAY);
    xwin = (Window)xv_get(paint_win, XV_XID);
    gc = DefaultGC(display, DefaultScreen(display));

    window_fit(frame);

    if (argc>1)
    {
	xv_set(frame,FRAME_LABEL, filename, NULL);

	rect = (Rect *)xv_get(canvas,CANVAS_VIEWABLE_RECT,paint_win);
	XClearArea(display, xwin,
		   rect->r_left, rect->r_top,
		   rect->r_width, rect->r_height,
		   0);

	if((set_offset = LoadHGLData(filename)) == -1)
	{
	    exit(1);
	}
    }

/*    line_p_page = (canvas_h -line_space + pbl/2) / line_space; */

    xv_main_loop(frame);
}



/***
 *
 *  LoadHGLData() reads HGL format data from fp_name, stores
 *  the information in a global struct array 'info[]'.  Return minumn
 *  offset if successful, -1 if anything is wrong.
 *
 ***/

int
  LoadHGLData(name_str)
char *name_str;
{
    Sequence *tSeq;
    int iSeq, iSeg;
    int dot_cnt;
    int min_offset = INT_MAX;
    FILE *fp_fname;
    static int info_max_size = 0;

    if(strcmp(name_str, "") == 0)
      return -1;

    if((fp_fname = fopen(name_str,"r")) == NULL)
    {
	fprintf(stderr,"File not found: %s\n", name_str);
	exit(1);
    }

    max_dots = 1000;
    mark_x = mark_y = 0;

    iSeq = 0;
    tSeq = (Sequence *)Calloc(1, sizeof(Sequence));

    if(info_max_size == 0)
    {
	info_max_size = 256;
	info = (INFO *)Calloc(info_max_size, sizeof(INFO));
    }

    while(ReadRecord(fp_fname, tSeq) != -1)
    {
	SeqNormal(tSeq);

	if(iSeq == info_max_size)
	{
	    info_max_size *= 2;
	    info = (INFO *)Realloc(info, sizeof(INFO)*info_max_size);
	}
	strcpy(info[iSeq].name, tSeq->name);

	info[iSeq].direction = tSeq->direction;
	info[iSeq].strandedness = tSeq->strandedness;
	info[iSeq].orig_strand = tSeq->orig_strand;
	info[iSeq].orig_direction = tSeq->orig_direction;
	if(tSeq->orig_strand == 2)
	  info[iSeq].orig_direction *= -1;

	min_offset = MIN(min_offset, tSeq->offset);
	max_dots = MAX(max_dots, tSeq->offset+tSeq->seqlen);

	if(info[iSeq].max_size == 0)
	{
	    info[iSeq].max_size = 8;
	    info[iSeq].dots=(int *)Calloc(info[iSeq].max_size*2,
					  sizeof(int));
	}

	iSeg = 0;
	POS2(info[iSeq].dots, iSeg, 0) = tSeq->offset;

	dot_cnt = 0;
	while(dot_cnt < tSeq->seqlen)
	{
	    if(tSeq->c_elem[dot_cnt] != '-' &&
	       (tSeq->c_elem[dot_cnt+1] == '-' ||
		dot_cnt+1 == tSeq->seqlen))
	    {
		POS2(info[iSeq].dots, iSeg++, 1) = tSeq->offset+dot_cnt;
		if(iSeg == info[iSeq].max_size)
		{
		    info[iSeq].max_size *= 2;
		    info[iSeq].dots =
		      (int *)Realloc(info[iSeq].dots,
				     sizeof(int)*2*info[iSeq].max_size);
		}
	    }
	    else if(tSeq->c_elem[dot_cnt] == '-' &&
		    tSeq->c_elem[dot_cnt+1] != '-')
	    {
		POS2(info[iSeq].dots, iSeg, 0) = tSeq->offset + dot_cnt+1;
	    }
	    dot_cnt++;
	}

	info[iSeq].size = iSeg;
	iSeq++;
    }
    info_size = iSeq;

    fclose(fp_fname);
    FreeRecord(&tSeq);

    if (min_offset != 0)
    {
	max_dots -= min_offset;
	for(iSeq = 0; iSeq<info_size; iSeq++)
	  for ( iSeg=0; iSeg<info[iSeq].size; iSeg++)
	  {
	      POS2(info[iSeq].dots, iSeg, 0) -= min_offset;
	      POS2(info[iSeq].dots, iSeg, 1) -= min_offset;
	  }
    }

    XClearWindow(display, xwin);
    (void)xv_set(canvas_paint_window(canvas),
		 XV_WIDTH,
		 MAX((int)xv_get(canvas,XV_WIDTH),max_dots+margin),
		 NULL);
    (void) xv_set(h_scrollbar,
		  SCROLLBAR_OBJECT_LENGTH,
		       MAX((int)xv_get(canvas, XV_WIDTH),
			   max_dots/scale+margin),

		  SCROLLBAR_VIEW_START, 0,
		  NULL);

    /*    mallocmap();*/
    return(min_offset);
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

    if(FROM_RESIZE == 'T')
    {
	(void)xv_set(h_scrollbar,
		     SCROLLBAR_OBJECT_LENGTH,
		     MAX((int)xv_get(canvas,XV_WIDTH),
			 max_dots/scale+margin),
/**  XVIEW3 of DEC? :
  This version of xview
  1. calls resize_proc() when a scrollbar is clicked.
  2. automatically sets SCROLLBAR_VIEW_START to 0 when a resize_proc()
     is called.
  Therefore, the following action can't be used.

		     SCROLLBAR_VIEW_START,
		     MAX(0, MIN(mark_x/scale - 100,
				max_dots/scale+margin-
				(int)xv_get(canvas, XV_WIDTH))),
**/
		     NULL);

	(void)xv_set(paint_win,  XV_WIDTH,
		     MAX((int)xv_get(canvas,XV_WIDTH),
			 max_dots/scale+margin),
		     NULL);

	FROM_RESIZE = 'F';
    }

    if (clear_mark == 'F')
    {
	/* Draw the whole viewable canvas.
	 */

	rect=(Rect *)xv_get(canvas,CANVAS_VIEWABLE_RECT,paint_win);
	drawarea_min_y = rect->r_top;
	drawarea_max_y = drawarea_min_y + rect->r_height;
    }
    /* else: this is to redraw the cleared area.
     * drawarea_min_y and drawarea_max_y have been set by
     * the caller procedure, so don't reset they.
     */

    /*
     *  Set drawarea_min_x, drawarea_max_x.
     */

    rect=(Rect *)xv_get(canvas,CANVAS_VIEWABLE_RECT,paint_win);
    drawarea_min_x = rect->r_left -1;
    drawarea_max_x = rect->r_left + rect->r_width;
    canvas_h = rect->r_height;

    y = 0;

    for(iSeq = 0; iSeq<info_size; iSeq++)
    {
	y = (y + 2*line_space - pbl/2 > canvas_h) ?
	  line_space : y + line_space;

	/*
	if(info[iSeq].orig_strand == 1)
	  XSetForeground(display,gc,colors[MASK_S1]);
	else if(info[iSeq].orig_strand == 2 )
	  XSetForeground(display,gc,colors[MASK_S2]);
	else
	  */
	XSetForeground(display,gc,colors[MASK_S0]);

	/*if(info[iSeq].direction == -1 || info[iSeq].direction == 2 )*/
	if(info[iSeq].orig_direction == -1)
	{
	    l_arrow(lwidth, info[iSeq].orig_strand,
		    POS2(info[iSeq].dots,0,0)/scale,y);
	}
	else if( info[iSeq].orig_direction == 1 )
	{
	    r_arrow(lwidth, info[iSeq].orig_strand,
		    POS2(info[iSeq].dots,
			 info[iSeq].size -1, 1)/scale,y);
	}

	for(iSeg=0; iSeg<info[iSeq].size; iSeg++)
	{
	    head = POS2(info[iSeq].dots,iSeg,0)/scale;
	    tail = POS2(info[iSeq].dots,iSeg,1)/scale;

	    /****
	    fprintf(stderr, "min_x=%d  max_x=%d  min_y=%d  max_y=%d  ",
		    drawarea_min_x, drawarea_max_x,
		    drawarea_min_y, drawarea_max_y);

	    fprintf(stderr, "head=%d  tail=%d  y=%d\n",
		    head, tail, y);
	    ****/

	    if(head <= drawarea_max_x && tail >= drawarea_min_x &&
	       drawarea_min_y <= y && y <= drawarea_max_y)
	    {
		for(i=0; i<lwidth; i++)
		{

		    XDrawLine(display, xwin, gc,
			      POS2(info[iSeq].dots,iSeg,0)/scale,y+i,
			      POS2(info[iSeq].dots,iSeg,1)/scale,y+i);
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
	cross(lwidth, mark_x/scale, mark_y);
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
    static int print_line = 1;
    int save_line, position, i, ii, iSeq, iSeg;
    int  return_len=10;
    char return_str[10];
    KeySym keysym;
    XEvent *xevent;
    int save_mark_x = mark_x;
    int save_mark_y = mark_y;
    char need_to_paint = 'F';
    static time_t last_Lsync_time = -1;
    struct stat stbuf;
    char loc_name[32];
    int loc_col = INT_MAX;
    FILE *Lsync_fp;
    char line[256];
    Rect *rect;

    stat(Lsync_fname, &stbuf);

    if (event_id(event) == LOC_WINENTER)
    {
	win_set_kbd_focus(canvas, xwin);
	/* win_set_kbd_focus(paint_win, xwin); */
	if(Lsync_YesNo == 'Y')
	{
	    stat(Lsync_fname, &stbuf);
	    if(stbuf.st_mtime > last_Lsync_time)
	    {
		Lsync_fp = fopen(Lsync_fname, "r");
		loc_name[0] = '\0';
		while(fgets(line, 256, Lsync_fp) != NULL)
		{
		    if(strncmp(line, "Col:", 4) == 0)
		      loc_col = atoi(line+4);
		    else if(strncmp(line, "SeqID:", 6) == 0)
		    {
			strcpy(loc_name, line+6);
			loc_name[strlen(loc_name)-1] = '\0';
		    }
		}
		fclose(Lsync_fp);

		if(loc_name[0] == ' ' || loc_col == INT_MAX)
		{
		    fprintf(stderr, "Bad status file.\n");
		}
		else
		{
		    iSeq = 0;
		    while(iSeq<info_size &&
			  strcmp(info[iSeq].name, loc_name) != 0)
		    {
			iSeq++;
		    }

		    if(iSeq == info_size)
		    {
			fprintf(stderr, "Sequence name for location-sync not found.\n");
		    }
		    else
		    {
			mark_x = loc_col;
			mark_y = (iSeq + 1) * line_space;
			need_to_paint = 'T';

			rect = (Rect *)
			  xv_get(canvas,CANVAS_VIEWABLE_RECT,paint_win);

			(void)xv_set(h_scrollbar,
			    SCROLLBAR_VIEW_START,
			    MAX(0,MIN(mark_x/scale-rect->r_width/2,
				      (int)xv_get(h_scrollbar,
						  SCROLLBAR_OBJECT_LENGTH)
				      - (int)xv_get(canvas, XV_WIDTH))),
				     NULL);
			last_Lsync_time = stbuf.st_mtime;
		    }
		}
	    }
	}
    }
    else if(event_id(event) == LOC_WINEXIT)
    {
	win_refuse_kbd_focus(canvas);
	/* win_refuse_kbd_focus(paint_win); */
	if(Lsync_YesNo == 'Y')
	{
	    /* update the status file. */
	    ii = 0;
	    while(ii<20 && (Lsync_fp = fopen(Lsync_fname, "r+")) == NULL)
	    {
		ii++;
	    }
	    if(ii == 20)
	    {
		fprintf(stderr, "Can't open status file for updating: %s\n",
			Lsync_fname);
	    }
	    else
	    {
		char *temp_file;
		int file_maxlen = 256, file_len = 0;

		temp_file = (char *)Calloc(file_maxlen, 1);
		while(fgets(line, 256, Lsync_fp) != NULL)
		{
		    if(strncmp(line, "Col:", 4) == 0)
		    {
			sprintf(line, "Col:%d\n", mark_x / scale);
		    }
		    else if(strncmp(line, "SeqID:", 6) == 0)
		    {
			sprintf(line, "SeqID:%s\n",
				info[print_line-1].name);
		    }

		    file_len += strlen(line);
		    if(file_len+1 >=file_maxlen)
		    {
			file_maxlen *= 2;
			temp_file = (char *)Realloc(temp_file, file_maxlen);
		    }
		    strcat(temp_file , line);
		}
		fseek(Lsync_fp, 0L, 0);
		fprintf(Lsync_fp, "%s", temp_file);
		Cfree(temp_file);
	    }
	    fclose(Lsync_fp);
	}
    }

    if (event_is_down(event))
    {
	if(event_action(event)==ACTION_SELECT)
	{
	    mark_x = event_x(event)*scale + 0.5*scale;
	    mark_y = event_y(event);
	    need_to_paint = 'T';
	}
	else if(event_is_key_right(event) ||
		event_is_ascii(event))
	{
	    int return_int;

	    return_int =  XLookupString(event->ie_xevent,/*not used */
					return_str, /*not used*/
					return_len, /*not used*/
					&keysym,NULL );

	    switch(keysym)
	    {
	      case XK_F31: /* center, key 5 on the right keyboard. */
		rect = (Rect *)
		  xv_get(canvas,CANVAS_VIEWABLE_RECT,paint_win);

		(void)xv_set(h_scrollbar,
			     SCROLLBAR_VIEW_START,
			     MAX(0, MIN(mark_x/scale - rect->r_width/2,
			     (int)xv_get(h_scrollbar, SCROLLBAR_OBJECT_LENGTH)
					- (int)xv_get(canvas, XV_WIDTH))),
			     NULL);
		break;
	      case XK_F27:
		/* the home key. */
		(void)xv_set(h_scrollbar, SCROLLBAR_VIEW_START,0, NULL);
		break;
	      case XK_Left:
		if(mark_x - scale >1) mark_x -= scale;
		need_to_paint = 'T';
		break;
	      case XK_Right:
		mark_x += scale;
		need_to_paint = 'T';
		break;
	      case XK_Up:
		if (mark_y>= 2*line_space - pbl/2)
		  mark_y -= line_space;
		else
		  mark_y = line_p_page*line_space + lwidth;
		need_to_paint = 'T';
		break;
	      case XK_Down:
		if((mark_y - 2*line_space+pbl/2)/line_space +1 <line_p_page -1)
		  mark_y += line_space;
		else
		  mark_y = line_space;
		need_to_paint = 'T';
		break;
	      default: ;
	    }
	}
    }

    if(need_to_paint == 'T')
    {
	/**
	 ** clear the old mark.
	 **/

	XSetForeground(display, gc, colors[WHITE]);
	cross(lwidth, save_mark_x/scale, save_mark_y);

	drawarea_min_y = save_mark_y - 10;
	drawarea_max_y = save_mark_y + 10;

	clear_mark = 'T';

	canvas_repaint_proc(canvas,paint_win,display, xwin, NULL);

	/**
	 ** put a new mark.
	 **/

	XSetForeground(display, gc, colors[RED]);
	cross(lwidth, mark_x/scale, mark_y);

	/**
	 ** Update the frame footer information.
	 **/

	position = INT_MIN;

	if (mark_y < 2*line_space - pbl/2)
	  save_line = 0;
	else
	  save_line= (mark_y - 2*line_space + pbl/2)/line_space +1;

	iSeq = save_line;
	while(iSeq < info_size)
	{
	    int accu_pos = 0;

	    iSeg = 0;
	    while(iSeg < info[iSeq].size)
	    {
		if(mark_x > POS2(info[iSeq].dots,iSeg,1))
		{
		    accu_pos += POS2(info[iSeq].dots,iSeg,1) -
		      POS2(info[iSeq].dots, iSeg ,0)+1;
		    iSeg++;
		}
		else if(mark_x>= POS2(info[iSeq].dots,iSeg,0))
		{
		    if(position != INT_MIN)
		    {
			/* ambiguous location. *** Set Foreground?? */
			sprintf(buf,
				"Mouse location: Line = ?   Position = ?   Column = %d",
				set_offset + mark_x + 1 );
			xv_set(frame,FRAME_LEFT_FOOTER, buf, NULL);
			xv_set(frame,FRAME_RIGHT_FOOTER,"Sequence: ",NULL);
			return XV_OK;
		    }
		    position = accu_pos + mark_x - POS2(info[iSeq].dots, iSeg, 0) + 1;
		    print_line = iSeq+1;
		    break;
		}
		else
		{
		    break;
		}
	    }
	    iSeq += line_p_page;
	}

	if(position != INT_MIN)
	{
	    sprintf(buf,
		    "Mouse location: Line = %d   Position = %d   Column = %d",
		    print_line, position, set_offset + mark_x + 1 );
	    xv_set(frame,FRAME_LEFT_FOOTER, buf, NULL);
	    sprintf(buf,"Sequence: %s\n",info[print_line-1].name);
	    save_for_mark_y = print_line;
	}
	else
	{
	    sprintf(buf,
		    "Mouse location: Line =    Position =    Column = %d",
		    set_offset+mark_x+1);
	    xv_set(frame,FRAME_LEFT_FOOTER, buf, NULL);
	    sprintf(buf,"Sequence: \n");
	}

	xv_set(frame,FRAME_RIGHT_FOOTER, buf, NULL);
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



canvas_resize_proc(canvas, canvas_width, canvas_height)
Canvas canvas;
int canvas_width, canvas_height;
{
    int tt;
    FROM_RESIZE = 'T';
    canvas_h = canvas_height;
    line_p_page = (canvas_h -line_space + pbl/2) / line_space;

    tt = save_for_mark_y % line_p_page;
    if(tt == 0)
      tt = line_p_page;

    mark_y = tt*line_space;
}



scale_proc(item, i_scale, event)
     Panel_item item;
     int i_scale;
     Event *event;
{
    scale = i_scale;

    (void)xv_set(h_scrollbar,
	 SCROLLBAR_OBJECT_LENGTH, MAX((int)xv_get(canvas, XV_WIDTH),
				      max_dots/scale+margin),
	 SCROLLBAR_VIEW_START, MAX(0, MIN(mark_x/scale - 100,
					  max_dots/scale+margin -
					  (int)xv_get(canvas, XV_WIDTH))),
	 NULL);

    (void)xv_set(paint_win,  XV_WIDTH,
		 MAX((int)xv_get(canvas,XV_WIDTH),max_dots/scale+margin),
		 NULL);

    XClearWindow(display, xwin);

    canvas_repaint_proc(canvas,paint_win,display, xwin, NULL);
    return XV_OK;
}



pbl_proc(item, i_pbl, event)
     Panel_item item;
     int i_pbl;
     Event *event;
{
    int tt;

    pbl = i_pbl;
    line_space = pbl + lwidth;
    line_p_page = (canvas_h -line_space + pbl/2) / line_space;

    tt = save_for_mark_y % line_p_page;
    if(tt == 0)
      tt = line_p_page;

    mark_y = tt*line_space;

    XClearWindow(display, xwin);

    canvas_repaint_proc(canvas,paint_win,display, xwin, NULL);
    return XV_OK;
}



lwidth_proc(item, i_lw, event)
     Panel_item item;
     int i_lw;
     Event *event;
{
    int tt;

    lwidth = i_lw;
    line_space = pbl + lwidth;
    line_p_page = (canvas_h -line_space + pbl/2) / line_space;

    tt = save_for_mark_y % line_p_page;
    if(tt == 0)
      tt = line_p_page;

    mark_y = tt*line_space;

    XClearWindow(display, xwin);

    canvas_repaint_proc(canvas,paint_win,display, xwin, NULL);
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



Lsync_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
    if(value == 0)
      Lsync_YesNo = 'N';
    else if(value == 1)
      Lsync_YesNo = 'Y';
    else
      fprintf(stderr, "Lsync_proc: value= %d\n", value);
}


show_prop_frame(item, event)
Frame item;
Event *event;
{
    xv_set(prop_subframe, XV_SHOW, TRUE, NULL);
}



r_arrow(size, strand, loc_x, loc_y)
int size, strand, loc_x, loc_y;
{
    int ii;

    if(size == 1)
    {
	if(strand < 2)
	  XDrawLine(display,xwin,gc,loc_x, loc_y,loc_x-2,loc_y-2);
	if(strand == 0 || strand == 2)
	  XDrawLine(display,xwin,gc,loc_x, loc_y,loc_x-2,loc_y+2);
    }

    /* draw the wings. */
    for(ii=1; ii <= size/2+1; ii++)
    {
	if(strand < 2)
	  XDrawLine(display,xwin,gc,
		    loc_x-ii, loc_y-ii, loc_x-ii, loc_y-1);
	if(strand == 0 || strand == 2)
	  XDrawLine(display,xwin,gc,
		    loc_x-ii, loc_y+size-1+ii, loc_x-ii,
		    loc_y+size); /* actually, loc_y+size-1+1.*/
    }

    /* draw the tip. */
    for(ii = 1; strand == 0 && ii < (size+1)/2; ii++)
    {
	XDrawLine(display,xwin,gc,
		  loc_x+ii, loc_y+ii, loc_x+ii, loc_y+size-1-ii);
    }

    for(ii=1; strand==1 && ii < size; ii++)
    {
	XDrawLine(display,xwin,gc,
		  loc_x+ii, loc_y+ii, loc_x+ii, loc_y+size-1);
    }

    for(ii=1; strand==2 && ii < size; ii++)
    {
	XDrawLine(display,xwin,gc,
		  loc_x+ii, loc_y, loc_x+ii, loc_y+size-1-ii);
    }
}



l_arrow(size, strand, loc_x, loc_y)
int size, strand, loc_x, loc_y;
{
    int ii;

    if(size == 1)
    {
	if(strand < 2)
	  XDrawLine(display,xwin,gc,loc_x, loc_y,loc_x+2,loc_y-2);
	if(strand == 0 || strand == 2)
	  XDrawLine(display,xwin,gc,loc_x, loc_y,loc_x+2,loc_y+2);
    }

    /* draw the wings. */
    for(ii=1; ii <= size/2+1; ii++)
    {
	if(strand < 2)
	  XDrawLine(display,xwin,gc,
		    loc_x+ii, loc_y-ii, loc_x+ii, loc_y-1);
	if(strand == 0 || strand == 2)
	  XDrawLine(display,xwin,gc,
		    loc_x+ii, loc_y+size-1+ii, loc_x+ii,
		    loc_y+size); /* actually, loc_y+size-1+1.*/
    }

    /* draw the tip. */
    for(ii = 1; strand == 0 && ii < (size+1)/2; ii++)
    {
	XDrawLine(display,xwin,gc,
		  loc_x-ii, loc_y+ii, loc_x-ii, loc_y+size-1-ii);
    }

    for(ii=1; strand==1 && ii < size; ii++)
    {
	XDrawLine(display,xwin,gc,
		  loc_x-ii, loc_y+ii, loc_x-ii, loc_y+size-1);
    }

    for(ii=1; strand==2 && ii < size; ii++)
    {
	XDrawLine(display,xwin,gc,
		  loc_x-ii, loc_y, loc_x-ii, loc_y+size-1-ii);
    }
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

