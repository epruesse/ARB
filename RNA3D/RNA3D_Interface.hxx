#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <stdio.h>

AW_window *createRNA3DMainWindow(AW_root *awr);

void resize( Widget w, XtPointer client_data, XEvent *event, char* x );
void expose( Widget w, XtPointer client_data, XEvent *event, char* x );
void init( Widget w, XtPointer client_data, XtPointer call_data );
void keyboard( Widget w, XtPointer client_data, XEvent *event, char* x );
void button_press_event_handler( Widget w, XtPointer client_data, XEvent *event, char* x );
void button_release_event_handler( Widget w, XtPointer client_data, XEvent *event, char* x );
void mouse_move_event_handler( Widget w, XtPointer client_data, XEvent *event, char* x );
void refreshOpenGLDisplay();


