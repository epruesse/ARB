#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <Xm/Xm.h>
#include <stdio.h>

AW_window *createRNA3DMainWindow(AW_root *awr);

void ResizeOpenGLWindow( Widget w, XtPointer client_data, XEvent *event, char* x );
void ExposeOpenGLWindow( Widget w, XtPointer client_data, XEvent *event, char* x );
void KeyBoardEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x );
void ButtonPressEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x );
void ButtonReleaseEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x );
void MouseMoveEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x );
void refreshOpenGLDisplay();

enum {
    LEFT_BUTTON = 1,
    MIDDLE_BUTTON,
    RIGHT_BUTTON,
    WHEEL_UP,
    WHEEL_DOWN
};
