#define WINDOW_WIDTH   950
#define WINDOW_HEIGHT  650

#define ZOOM_FACTOR    0.0005f     // scaling factor in z-axis (ZOOM)

enum {
    LEFT_BUTTON = 1,
    MIDDLE_BUTTON,
    RIGHT_BUTTON,
    WHEEL_UP,
    WHEEL_DOWN
};

void ResizeOpenGLWindow( Widget w, XtPointer client_data, XEvent *event, char* x );
void ExposeOpenGLWindow( Widget w, XtPointer client_data, XEvent *event, char* x );
void KeyBoardEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x );
void ButtonPressEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x );
void ButtonReleaseEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x );
void MouseMoveEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x );
void RefreshOpenGLDisplay();

class AW_root;
class AW_window;
AW_window *CreateRNA3DMainWindow(AW_root *awr); // Creates RNA3D Application Main Window
