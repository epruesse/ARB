/*
Copyright (c) 1989-1990, University of Illinois board of trustees.  All
rights reserved.  Written by Michael Maciukenas at the Center for Prokaryote
Genome Analysis.  Design and implementation guidance by Steven Smith, Carl
Woese.
*/
/* File picker by Mike Maciukenas
** Allows the user to search up and down the directory tree, and choose a
**  file.
** "Open" descends down into a directory, or chooses a file (depending **  on what is selected).  The user may also press return after choosing
**  a file or directory, to do the same thing.
** "Up Dir" ascends to the parent directory.
** "Cancel" cancels the operation.
** The user may also type a directory into the "Directory:" field.  When the
**  user presses return (or tab, or newline), the contents of the new directory
**  will be shown.
*/
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#define OWTOOLKIT_WARNING_DISABLED
#include <xview/textsw.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/scrollbar.h>
#include <xview/rectlist.h>
#include <xview/notice.h>
#include <xview/font.h>
#include <sys/stat.h>


#define GBUFSIZ 1024 /* buffer size, remove when adding to Steve's code */

#define FL_VIEW_H 15 /* # of files to show in one page, originally */


/* structure for a linked list that allows sorting of filenames */
typedef struct namedata {char *FileN;		/* file name */
			 int type;		/* flag: 1 if directory '/'
						**       2 if executable '*'
						**       3 if symbolic link '@'
						**       4 if socket '='
						**       0 if normal */
			 struct namedata *Next; /* next in list */
			} NameData;

Frame fl_getframe = XV_NULL; /* frame, is set to XV_NULL by free_mem(),
			     ** load_file() checks this to see if it should
			     ** destroy an existing frame */
Scrollbar fl_scroll; /* the scrollbar for the file list canvas */
Canvas fl_FileList; /* the file list canvas */
Panel_item fl_DirText; /* the text item that displays the directory */
Panel fl_Getpanel; /* the panel, contains buttons, and DirText */
GC fl_gc; /* gc to use for drawing file names, just the default GC with
	  ** the frame's font copied in. */
int fl_current_picked, fl_current_len; /* the current item picked in the file
				       ** list, and the current number of items
				       ** in the file list */
int fl_cell_h, fl_width, fl_ascent; /* the height of the font, the width of the
				    ** canvas, and the default ascent of the
				    ** font, all used for drawing into the file
				    ** list canvas */
Xv_opaque data;



NameData *fl_start; /* the root node for the linked list of filenames */

Frame load_file(Parentframe, x, y, passdata)
/* pick a file for loading. */
Frame Parentframe;
int x, y;
Xv_opaque passdata;
{

  /* callback procedures */
  int fl_open_btn_lf(), fl_up_dir_btn(), lf_cancel_btn();
  void fl_show_list_lf();
  void fl_list_select_lf();
  Panel_setting fl_dir_typed();
  /* interposed destroy function */
  Notify_value fl_free_mem();

  char dirname[GBUFSIZ];
  Display *display;
  Xv_screen screen;
  int screen_no;
  Xv_Font font;
  XFontStruct *font_data;

  data=passdata;

  /* create the frame */
  fl_getframe = xv_create(Parentframe, FRAME_CMD,
			FRAME_CMD_PUSHPIN_IN,TRUE,
			FRAME_LABEL, "Choose File",
			FRAME_SHOW_RESIZE_CORNER, FALSE,
			XV_X, x,
			XV_Y, y,
			NULL);
  notify_interpose_destroy_func(fl_getframe, fl_free_mem);

  /* get font characteristics */
  font = xv_get(fl_getframe, XV_FONT);
  fl_cell_h = xv_get(font, FONT_DEFAULT_CHAR_HEIGHT);
  fl_width = 50*xv_get(font, FONT_DEFAULT_CHAR_WIDTH);
  font_data = (XFontStruct *)xv_get(font, FONT_INFO);
  fl_ascent = font_data->ascent;

  /* create the panel and panel buttons */
/*
  fl_Getpanel = xv_create(fl_getframe, PANEL,
			NULL);
*/
  fl_Getpanel = xv_get(fl_getframe, FRAME_CMD_PANEL);
  (void) xv_create(fl_Getpanel, PANEL_BUTTON,
		PANEL_LABEL_STRING, "Open",
		PANEL_NOTIFY_PROC, fl_open_btn_lf,
		NULL);
  (void) xv_create(fl_Getpanel, PANEL_BUTTON,
		PANEL_LABEL_STRING, "Up Dir",
		PANEL_NOTIFY_PROC, fl_up_dir_btn,
		NULL);
  (void) xv_create(fl_Getpanel, PANEL_BUTTON,
		PANEL_LABEL_STRING, "Cancel",
		PANEL_NOTIFY_PROC, lf_cancel_btn,
		NULL);
  /* create the "Directory:" field, initialized to the current working dir */
  getcwd(dirname, GBUFSIZ);
  fl_DirText = xv_create(fl_Getpanel, PANEL_TEXT,
		PANEL_LABEL_STRING,"Directory:",
		XV_X, xv_col(fl_Getpanel, 0),
		XV_Y, xv_row(fl_Getpanel, 1),
		PANEL_VALUE_STORED_LENGTH, GBUFSIZ,
		PANEL_VALUE_DISPLAY_LENGTH, 30,
		PANEL_VALUE, dirname,
		PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
		PANEL_NOTIFY_STRING, "\n\r\t",
		PANEL_NOTIFY_PROC, fl_dir_typed,
		NULL);

  window_fit(fl_Getpanel);

  /* create the file list canvas, below the above panel */
  fl_FileList = xv_create(fl_getframe, CANVAS,
		XV_X, 0,
		WIN_BELOW, fl_Getpanel,
		XV_WIDTH, fl_width,
		XV_HEIGHT, FL_VIEW_H*fl_cell_h+7,
		CANVAS_REPAINT_PROC, fl_show_list_lf,
		CANVAS_AUTO_EXPAND, FALSE,
		CANVAS_AUTO_SHRINK, FALSE,
		CANVAS_WIDTH, fl_width,
		CANVAS_HEIGHT, fl_cell_h,
		CANVAS_RETAINED, FALSE,
		OPENWIN_AUTO_CLEAR, FALSE,
		NULL);
  fl_scroll = xv_create(fl_FileList, SCROLLBAR,
		SCROLLBAR_DIRECTION, SCROLLBAR_VERTICAL,
		SCROLLBAR_PIXELS_PER_UNIT, fl_cell_h,
		SCROLLBAR_VIEW_LENGTH, fl_view_h(),
		SCROLLBAR_PAGE_LENGTH, fl_view_h(),
		NULL);
  xv_set(canvas_paint_window(fl_FileList),
	WIN_EVENT_PROC, fl_list_select_lf,
	WIN_CONSUME_EVENTS, WIN_MOUSE_BUTTONS, LOC_DRAG, WIN_ASCII_EVENTS, NULL,
	NULL);
  xv_set(fl_Getpanel, XV_WIDTH, xv_get(fl_FileList, XV_WIDTH), NULL);

  /* set up the gc for drawing into the file list */
  display = (Display *)xv_get(fl_getframe, XV_DISPLAY);
  screen = (Xv_screen)xv_get(fl_getframe, XV_SCREEN);
  screen_no = (int)xv_get(screen, SCREEN_NUMBER);
  fl_gc = XCreateGC(display, RootWindow(display, screen_no),
		0, NULL);
  XCopyGC(display, DefaultGC(display, DefaultScreen(display)),
	0xFFFFFFFF, fl_gc);
  XSetFont(display, fl_gc, xv_get(font, XV_XID));
/*
*	Added S.Smith 2/5/91
*/
  XSetForeground(display,fl_gc,BlackPixel(display,DefaultScreen(display)));
  XSetBackground(display,fl_gc,WhitePixel(display,DefaultScreen(display)));


  /* set up the extra trailing node for the linked list, makes insertion
  ** into the list easier */
  fl_start = (NameData *)calloc(1,1+sizeof(NameData));
  fl_start->FileN = (char *)NULL;
  fl_start->Next = NULL;

  /* make the list, showing files in the application`s current directory
  */
  (void) fl_make_list();

  window_fit(fl_getframe);
  xv_set(fl_getframe, XV_SHOW, TRUE, NULL);
  return(fl_getframe);
}


int fl_open_btn_lf(item, event)
/* callback procedure for the open button.  If it's a directory, switch to
** the new directory, otherwise return the filename
*/
Panel_item item;
Event *event;
{
  int i, end;
  char namebuf[GBUFSIZ], thestr[GBUFSIZ];
  NameData *current;

  if(fl_current_picked != -1) /* then an item is selected.  Work with it */
   {
    /* find item in list */
    current = fl_start;
    for(i=0; i<fl_current_picked; i++)
      current = current->Next;
    strcpy(namebuf, current->FileN);
    if(current->type == 1) /* then it's a directory, so switch to it */
     {
      if(fl_checkdir(namebuf))
       {
        chdir(namebuf);
        (void) fl_make_list();
	fl_set_dirtext(fl_DirText);
        return XV_OK;
       }
     }
    else /* it's a file name, so return it */
     {
      if(fl_checkdir(xv_get(fl_DirText, PANEL_VALUE))) /* then valid dir */
       {
	if(current->type != 0) /* then it's not a regular file, so strip off
			       ** the extra type character: *, =, /, or @ */
	 namebuf[strlen(namebuf)-1]='\0';
	/* create the file string (with full directory path) */
	getcwd(thestr, GBUFSIZ);
	if(thestr[strlen(thestr)-1] != '/')
	  strcat(thestr, "/");
	strcat(thestr, namebuf);
	act_on_it_lf(thestr, data); /* give filename to application */
	xv_destroy_safe(fl_getframe);
	return XV_OK;
       }
      else
      { /* invalid directory, so show notice*/
        int result;
        Panel panel = (Panel)xv_get(fl_FileList, PANEL_PARENT_PANEL);

        result = notice_prompt(panel, NULL,
                NOTICE_MESSAGE_STRINGS, "Invalid Directory specified.", NULL,
                NOTICE_FOCUS_XY, event_x(event), event_y(event),
                NOTICE_BUTTON_YES, "Change Directory",
                NULL);
        }
     }
   }
}

int fl_up_dir_btn(item, event)
/* go up one directory */
Panel_item item;
Event *event;
{
 char dirname[GBUFSIZ];

 /* pretty simple, just go up, show it, and change the "Directory:" field */
 (void) chdir("..");
 (void) fl_make_list();
 fl_set_dirtext(fl_DirText);
 return XV_OK;
}

Panel_setting fl_dir_typed(item, event)
/* handle when user types return, newline, or tab in the "Directory:" field.
** if it's a valid directory, it moves to it, otherwise, display a notice
*/
Panel_item item;
Event *event;
{
 int error;
 char dirname[GBUFSIZ];

 switch (event_action(event))
  {
   case '\n':
   case '\r':
   case '\t':
    {
     if(fl_checkdir(xv_get(fl_DirText, PANEL_VALUE)))
      { /* valid directory, chdir to it and show it */
       chdir(xv_get(fl_DirText, PANEL_VALUE));
       fl_make_list();
       fl_set_dirtext(fl_DirText);
      }
     else
      { /* invalid directory, so show notice */
        int result;
        Panel panel = (Panel)xv_get(fl_FileList, PANEL_PARENT_PANEL);

        result = notice_prompt(panel, NULL,
                NOTICE_MESSAGE_STRINGS, "Invalid Directory specified.", NULL,
                NOTICE_FOCUS_XY, event_x(event), event_y(event),
                NOTICE_BUTTON_YES, "Change Directory",
                NULL);
      }
     return PANEL_NONE;
    };
   /* if it wasn't \n, \t, or \r, pass event on to standard
   ** panel_text handler
   */
   default:
    return(panel_text_notify(item, event));
  }
}

int lf_cancel_btn(item, event)
/* handle the cancel button.  Just destroys the frame and returns
*/
Panel_item item;
Event *event;
{

  act_on_it_lf(NULL);
  xv_destroy_safe(fl_getframe);
  return XV_OK;
}

fl_readln(file, buf)
FILE *file;
char *buf;
{
	int ic;
	int i = 0;

	while (((ic=getc(file)) != EOF) && ((char)ic != '\n'))
	    buf[i++]= (char)ic;
	buf[i] = '\0';
}

int fl_make_list()
/* Creates a list of files, out of the current working directory.  It then
** tells the file list canvas to refresh itself.  The list sits attached to
** fl_start, for reading by the show_list() routine.
*/
{
  FILE *dirp;					/* for directory data */
  int i, list_len, cur_pos;
  char dirname[GBUFSIZ], tempbuf[GBUFSIZ];
  NameData *current, *temp;			/* structures for reading
						** and sorting file names */
  int notdone;
  struct stat statbuf;				/* for checking if a file
						** name is a directory    */
  int pid = getpid();				/* for creation of temp
						** file for directory list */
  char tmpcmd[GBUFSIZ];				/* for holding ls command */
  char tmpname[GBUFSIZ];			/* for holding file names */


  getcwd(dirname, GBUFSIZ);
  sprintf(tmpcmd, "cd %s;ls -F > /usr/tmp/.svlffil%d", dirname, pid);
  sprintf(tmpname, "/usr/tmp/.svlffil%d", pid);
  system(tmpcmd);
  dirp = fopen(tmpname, "r");
  if (dirp == NULL)   /* just a check to make sure */
   {
    fprintf(stderr, "fl_make_list was passed bad directory name\n");
    return(-1);
   }
  else
   {
    /* free up the old list, to build a new one */
    for(current = fl_start; current->FileN != (char *)NULL; i++)
     {
      temp = current;
      current = current->Next;
      free(temp->FileN);
      free(temp);
     };
    /* set up the linked list for sorting */
    fl_start = (NameData *)calloc(1, sizeof(NameData)+1);
    fl_start->FileN = (char *)NULL;
    fl_start->Next = NULL;
    /* read through the directory entries */
    list_len = 0;
    for(fl_readln(dirp, tempbuf); tempbuf[0] != '\0'; fl_readln(dirp, tempbuf))
     {
	/* don't include "." and ".." in the list */
	if((strcmp(tempbuf,"./")!=0)&&
	   (strcmp(tempbuf,"../")!=0))
	  {
	   /* find the right spot in the list to insert the new name */
	   current = fl_start;
	   notdone = 1;
	   while(notdone)
	     if(current->FileN == NULL)
		notdone = 0;
	     else if(strcmp(tempbuf, current->FileN)>0)
	       current = current->Next;
	     else
		notdone = 0;
	   /* insert the new name */
	   temp = (NameData *)calloc(1, sizeof(NameData)+1);
	   temp->FileN = current->FileN;
	   temp->type = current->type;
	   temp->Next = current->Next;
	   ++list_len;
	   current->Next = temp;
	   /* set flag for file type */
           switch(tempbuf[strlen(tempbuf)-1])
            {
             case '/': /* directory */
              {
               current->type = 1;
               break;
              }
             case '@': /* symbolic link */
              {
               current->type = 3;
               break;
              }
             case '=': /* socket */
              {
               current->type = 4;
               break;
              }
             case '*': /* executable */
              {
               current->type = 2;
               break;
              }
             default:
	      {
               current->type = 0;
	       break;
	      }
         }
	   current->FileN = (char *)calloc(1, 1+strlen(tempbuf));
	   strcpy(current->FileN,tempbuf);
	  };
     }
    fclose(dirp);
    sprintf(tmpcmd, "rm %s", tmpname);
    system(tmpcmd);

    /* adjust the Canvas size, and refresh it */
    fl_current_len = list_len;
    cur_pos = xv_get(fl_scroll, SCROLLBAR_VIEW_START);
    xv_set(fl_FileList, CANVAS_HEIGHT,
		(list_len+fl_view_h()+1)*fl_cell_h,
		NULL);
    /* scrollbars bomb with zero-length objects */
    if(list_len == 0) ++list_len;
    /* reset scrollbar */
    xv_set(fl_scroll, SCROLLBAR_VIEW_START, 0,
		SCROLLBAR_OBJECT_LENGTH, list_len,
		NULL);
    /* refresh canvas */
    wmgr_refreshwindow(canvas_paint_window(fl_FileList));
    fl_current_picked = -1;
    return(0);
   }
}

fl_set_dirtext(fl_DirText)
/* sets the "Directory:" field according to the current directory
** fl_DirText is the Xview pointer to the fl_DirText Panel Item
*/
Panel_item fl_DirText;
{
 char dirbuf[GBUFSIZ];

 getcwd(dirbuf, GBUFSIZ);
 xv_set(fl_DirText, PANEL_VALUE, dirbuf, NULL);

}

int fl_checkdir(dirname)
/* check if a directory can be opened.  directory can be specified by
** full root name or by current name.  returns true if it can be opened.
*/
char *dirname;
{
 DIR *dirp;

 dirp = opendir(dirname);
 if(dirp == NULL)		/* not available, user cannot enter */
  return(0);
 else
  {
   closedir(dirp);		/* must close it */
   return(1);
  }
}

void fl_show_list_lf(canvas, paint_window, repaint_area)
/* repaint procedure for the file list canvas.  Repaints all file names in
** the damaged area */
Canvas canvas;
Xv_Window paint_window;
Rectlist *repaint_area;
{
  NameData *current;
  int i;
  int start_draw, end_draw;
  Display *dpy;
  Window xwin;


  /* make sure AUTO_CLEAR is off, this routine will do it itself */
  while(xv_get(fl_FileList, OPENWIN_AUTO_CLEAR)!=FALSE)
   {
    fprintf(stderr, "lf:found bug--OPENWIN_AUTO_CLEAR still TRUE");
    xv_set(fl_FileList, OPENWIN_AUTO_CLEAR, FALSE, NULL);
   }
  /* make sure RETAINED is off, this routine will repaint itself */
  while(xv_get(fl_FileList, CANVAS_RETAINED)!=FALSE)
   {
    fprintf(stderr, "lf:found bug--CANVAS_RETAINED still TRUE");
    xv_set(fl_FileList, CANVAS_RETAINED, FALSE, NULL);
   }
  /* get display and window */
  dpy = (Display *)xv_get(paint_window, XV_DISPLAY);
  xwin = (Window)xv_get(paint_window, XV_XID);

  /* clear the area given us by Xview, for simplicity, we clear the
  ** smallest rectangle that encloses all of the destroyed areas, the
  ** rl_bound rectangle */
  XClearArea(dpy, xwin,
    repaint_area->rl_bound.r_left,
    repaint_area->rl_bound.r_top,
    repaint_area->rl_bound.r_width,
    repaint_area->rl_bound.r_height,
    0);
  /* the next 3 lines calculate which file names must be drawn, by where the
  ** top and bottom of the rl_bound rectangle lie */
  start_draw = repaint_area->rl_bound.r_top;
  end_draw = (repaint_area->rl_bound.r_height + start_draw - 1) / fl_cell_h;
  start_draw = (start_draw - 1) / fl_cell_h;

  /* find the first element to draw in the list */
  current = fl_start;
  for(i = 0; (i<start_draw) && (current->Next != NULL); i++)
    current = current->Next;
  /* now start drawing them */
  for(; (i<=end_draw) && (current->Next != NULL); i++)
   {
    XDrawString(dpy, xwin, fl_gc, 5, i*fl_cell_h+fl_ascent, current->FileN,
		strlen(current->FileN));
    /* add a box if we are drawing the currently picked one */
    if(i==fl_current_picked)
     {
	XDrawRectangle(dpy, xwin, fl_gc,
		2, i*fl_cell_h,
		xv_get(canvas, XV_WIDTH)-11-xv_get(fl_scroll, XV_WIDTH),
		fl_cell_h);
	}
    current = current->Next;
   }
}

void fl_list_select_lf(paint_window, event)
/* callback procedure for events that happen in the file list canvas.  Checks
** mouse button press or drag, and for when the user types return */
Xv_window paint_window;
Event *event;
{
  int picked, cur_pos;
  Window xwin = (Window)xv_get(paint_window, XV_XID);
  Display *dpy;

  dpy = (Display *)xv_get(paint_window, XV_DISPLAY);
  /* get the current position of the scrollbar for future reference */
  cur_pos = xv_get(fl_scroll, SCROLLBAR_VIEW_START);

	/* first, check for user picking a file name */
	if((event_action(event) == ACTION_SELECT)||
	   (event_action(event) == LOC_DRAG))
		{
		picked = (event_y(event) - 1)  / fl_cell_h;
		/* make sure the file picked is on screen.  if it is not,
		** we just ignore it.  this avoids wierd stuff, like being
		** able to pick files that aren't shown on screen */
		if((picked >= cur_pos)&&
		   (picked < cur_pos+fl_view_h())&&
		   (picked < fl_current_len))
		 {
		  /* efficiency:  ignore if it is already picked */
		  if(picked != fl_current_picked)
		   {
		    XSetFunction(dpy, fl_gc, GXclear);
		    XDrawRectangle(dpy, xwin, fl_gc,
			2, fl_current_picked*fl_cell_h,
			xv_get(fl_FileList, XV_WIDTH)-11-
				xv_get(fl_scroll, XV_WIDTH),
			fl_cell_h);
		    XSetFunction(dpy, fl_gc, GXcopy);
		    XDrawRectangle(dpy, xwin, fl_gc,
			2, picked*fl_cell_h,
			xv_get(fl_FileList, XV_WIDTH)-11-
				xv_get(fl_scroll, XV_WIDTH),
			fl_cell_h);
		     fl_current_picked = picked;
		    }
		 }
		}
	/* user may have pressed return, then just call the open button
	** callback procedure.  PANEL_FIRST_ITEM gets the pointer to the
	** open button itself, since it happens to be the first item on
	** the panel. fl_open_btn doesn't really use this parameter, but
	** just in case it ever does, we include it. */
	else if((event_is_ascii(event))&&(event_action(event) == '\r'))
	  fl_open_btn_lf(xv_get(fl_Getpanel, PANEL_FIRST_ITEM), event);
	else
	  return;
}
int fl_view_h()
/* returns the current height (in # of file names displayed) of the file list */
{
  return (((int)xv_get(fl_FileList, XV_HEIGHT))/fl_cell_h);
}

Notify_value
fl_free_mem(client, status)
/* clean up when the frame is destroyed.  Frees up the memory used in the
** linked list of file names, and sets the Frame variable (getframe) to null */
Notify_client client;
Destroy_status status;
{
 NameData *current, *temp;
 int i;

switch (status)
 {
  case DESTROY_CHECKING:
    return NOTIFY_DONE;
  case DESTROY_CLEANUP:
   {
    for(current = fl_start; current->FileN != (char *)NULL; i++)
     {
      temp = current;
      current = current->Next;
      free(temp->FileN);
      free(temp);
     };
    fl_getframe = XV_NULL;
    return notify_next_destroy_func(client, status);
   }
  default:
    return NOTIFY_DONE;
 }
}
