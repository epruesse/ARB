// ================================================================ //
//                                                                  //
//   File      : RNA3D_Interface.hxx                                //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef RNA3D_INTERFACE_HXX
#define RNA3D_INTERFACE_HXX

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

void KeyBoardEventHandler(Widget w, XtPointer client_data, XEvent *event, char* x);
void RefreshOpenGLDisplay();

class  AW_root;
class  AW_window;
class  GBDATA;
struct ED4_plugin_host;

AW_window *CreateRNA3DMainWindow(AW_root *awr, GBDATA *gb_main, ED4_plugin_host& host);

#else
#error RNA3D_Interface.hxx included twice
#endif // RNA3D_INTERFACE_HXX
