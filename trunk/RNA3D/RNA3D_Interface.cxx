#include "RNA3D_GlobalHeader.hxx"

// The following includes are needed to use AW_window_Motif 
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>

// rest of conflict resolving (left here for testing) : 
// #include <aw_awars.hxx>
// #include <aw_preset.hxx>
// #include <awt_canvas.hxx>
// #include <awt.hxx>
// #include <aw_root.hxx>

// #include <GL/glew.h>
// #include <GL/GLwMDrawA.h>

// #include <X11/keysym.h>
// #include <X11/Xlib.h>
// #include <X11/Xutil.h>
// #include <Xm/Xm.h>
// #include <Xm/Protocols.h>
// #include <Xm/MwmUtil.h>
// #include <Xm/MainW.h>

#warning including ../something means hacking - will be removed when yadhu implemented the general idle handler
#include "../WINDOW/aw_at.hxx"
#include "../WINDOW/aw_awar.hxx"
#include "../WINDOW/aw_xfig.hxx"
#include "../WINDOW/aw_xfigfont.hxx"
#include "../WINDOW/aw_Xm.hxx"
#include "../WINDOW/aw_click.hxx"
#include "../WINDOW/aw_size.hxx"
#include "../WINDOW/aw_print.hxx"
#include "../WINDOW/aw_window_Xm.hxx"
#include "../WINDOW/aw_xkey.hxx"

#include "RNA3D_Global.hxx"
#include "RNA3D_Graphics.hxx"
#include "RNA3D_OpenGLEngine.hxx"
#include "RNA3D_Interface.hxx"

using namespace std;

AWT_canvas *gl_Canvas;
AW_window_menu_modes_opengl *awm;

extern GBDATA *gb_main;
Widget OpenGLParentWidget;

XtAppContext appContext;
XtWorkProcId workId   = 0;
GLboolean    Spinning = GL_FALSE;

Boolean SpinMolecule(XtPointer clientData) {
    extern float ROTATION_SPEED;
    ROTATION_SPEED = 0.05;
    RefreshOpenGLDisplay();
    return false; /* leave work proc active */
}

void MapStateChanged(XEvent * event) {
    switch (event->type) {
    case MapNotify:
        if (Spinning && workId != 0) 
            workId = XtAppAddWorkProc(appContext, SpinMolecule, NULL);
        break;
    case UnmapNotify:
        if (Spinning) 
            XtRemoveWorkProc(workId);
        break;
    }
}

void ResizeOpenGLWindow( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	XConfigureEvent *evt;
	evt = (XConfigureEvent*) event;
	
	extern int OpenGLEngineState;  // because it is globally defined in GEN_OpenGL.cxx
	
	if ( OpenGLEngineState == NOT_CREATED ) {
		return;
	}
	
	ReshapeOpenGLWindow( (GLint) evt->width, (GLint) evt->height );

	MapStateChanged(event);

    RefreshOpenGLDisplay();
}


void KeyBoardEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	XKeyEvent *evt;
    evt = (XKeyEvent*) event;

    char   buffer[1];
    KeySym keysym;
  	int    count;
    extern bool autoRotate;

    // Converting keycode to keysym
    count = XLookupString((XKeyEvent *) event, buffer, 1, &keysym, NULL);

    switch(keysym) {
    case XK_space:
        if(Spinning) {
            XtRemoveWorkProc(workId);
            Spinning = GL_FALSE; autoRotate = false;
        } else {
            workId = XtAppAddWorkProc(appContext, SpinMolecule, NULL);
            Spinning = GL_TRUE; autoRotate = true;
        }
        break;
    case XK_Escape:
        exit(0); 
        break;
    }

    RefreshOpenGLDisplay();
}

void ButtonReleaseEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	XButtonEvent *xr;
	xr = (XButtonEvent*) event;

    extern bool rotateMolecule;
    extern bool enableZoom;

    switch(xr->button) {
    case LEFT_BUTTON:
        rotateMolecule = false;   
        break;
    case MIDDLE_BUTTON:
        break;
    case RIGHT_BUTTON:
        cout<<"Right Button REleased !!!"<<endl;
        break;
    }

    RefreshOpenGLDisplay();
}


void ButtonPressEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x ) {

	XButtonEvent *xp;
	xp = (XButtonEvent*) event;
	extern GLfloat saved_x, saved_y;
    extern bool rotateMolecule;
    extern float scale;

    switch(xp->button) {
    case LEFT_BUTTON:
        rotateMolecule = true;   
        saved_x = xp->x;
        saved_y = xp->y;
        break;
    case MIDDLE_BUTTON:
        gl_Canvas->set_mode(AWT_MODE_NONE);
        cout<<"Middle Button pressed !!!"<<endl;
        break;
    case RIGHT_BUTTON:
        cout<<"Right Button PRESSed !!!"<<endl;
        break;
    case WHEEL_UP:
        scale += ZOOM_FACTOR;
        cout<<"Wheel Up !!!"<<endl;
        break;
    case WHEEL_DOWN:
        scale -= ZOOM_FACTOR;
        cout<<"Wheel Down !!!"<<endl;
        break;
	}
    
    RefreshOpenGLDisplay();
}

void MouseMoveEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	
	XMotionEvent *xp;
	xp = (XMotionEvent*) event;
	
	if ( xp->state & Button3Mask ) { //Rechte Maustaste
	}
	else if ( xp->state & Button3Mask ) {
		/* do nothing*/
	}
    extern float ROTATION_SPEED;
    extern bool autoRotate;
    if (!autoRotate)  ROTATION_SPEED = 0.5;
    extern bool rotateMolecule;
    if(rotateMolecule) {
        RotateMolecule(xp->x, xp->y);
    }

    RefreshOpenGLDisplay();
}

void ExposeOpenGLWindow( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	
	extern int OpenGLEngineState;  
	
	if ( OpenGLEngineState == NOT_CREATED ) {
		extern GBDATA* OpenGL_gb_main;
		OpenGL_gb_main = gb_main;
		
		InitializeOpenGLWindow( w );
		
		XExposeEvent *evt;
		evt = (XExposeEvent*) event;
		
		InitializeOpenGLEngine( (GLint) evt->height, (GLint) evt->height );
		
		ReshapeOpenGLWindow( (GLint) evt->width, (GLint) evt->height );
	}

    RefreshOpenGLDisplay();
}

void RefreshOpenGLDisplay() {
	extern int OpenGLEngineState;
	if ( OpenGLEngineState == CREATED ) {
		extern Widget _glw;
        RenderOpenGLScene(_glw);
    }
}

void SetOpenGLBackGroundColor() {
	extern Widget _glw;
	unsigned long bgColor;
	XtVaGetValues( _glw, XmNbackground, &bgColor, NULL );

    extern AWT_canvas *gl_Canvas;
    Widget w = gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area;

    XColor xcolor;
    xcolor.pixel = bgColor;
    Colormap colormap = DefaultColormapOfScreen( XtScreen( w ) );
    XQueryColor( XtDisplay( w ), colormap, &xcolor );

    float r, g, b; r = g = b = 0.0;
    r = (float) xcolor.red / 65535.0;
    g = (float) xcolor.green / 65535.0;
    b = (float) xcolor.blue / 65535.0;

    // set OpenGL Backgroud Color to the widget's backgroud     
    glClearColor(r, g, b, 0);
}

/*---------------------------- Creating WINDOWS ------------------------------ */
static void AddCallBacks(AW_root *awr, AWT_canvas *ntw) {
    // adding callbacks to the awars to refresh the display if recieved any changes
}

static AW_window *CreateSelectSAI_window(AW_root *aw_root){
    // window to select SAI from the existing SAIs in the database
    static AW_window_simple *aws = 0;
    if(aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "SELECT_SAI", "SELECT SAI");
    aws->load_xfig("selectSAI.fig");

    aws->at("selection");
    aws->callback((AW_CB0)AW_POPDOWN);
    awt_create_selection_list_on_extendeds(gb_main,(AW_window *)aws,AWAR_SAI_SELECTED);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->window_fit();
    return (AW_window *)aws;
}

AW_window *CreateRNA3DMainWindow(AW_root *awr){
    // Main Window - Canvas on which the actual painting is done
    GB_transaction dummy(gb_main);

    awr->awar_int(AWAR_SAI_SELECTED, 0, AW_ROOT_DEFAULT); 
    int width = 600, height = 400;

    extern AW_window_menu_modes_opengl *awm;
    awm = new AW_window_menu_modes_opengl();
    awm->init(awr,"RNA3D", "RNA3D: 3D Structure of Ribosomal RNA",width,height);

    AW_gc_manager aw_gc_manager;
    RNA3D_Graphics *rna3DGraphics = new RNA3D_Graphics(awr,gb_main);

    extern AWT_canvas *gl_Canvas;
    gl_Canvas = new AWT_canvas(gb_main,awm, rna3DGraphics, aw_gc_manager,AWAR_SPECIES_NAME);

    gl_Canvas->recalc_size();
    gl_Canvas->refresh();
    gl_Canvas->set_mode(AWT_MODE_NONE); 

    awm->insert_help_topic("rna3D_help_how", "How to Visualize 3D structure of rRNA ?", "H", "rna3DHelp.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"rna3DHelp.hlp", 0);
    awm->create_menu( 0, "File", "F", 0,  AWM_ALL );
    awm->insert_menu_topic( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);
    awm->create_menu( 0, "Properties", "P", 0,  AWM_ALL );
    awm->insert_menu_topic( "selectSAI", "Select SAI", "S","selectSai.hlp", AWM_ALL,AW_POPUP, (AW_CL)CreateSelectSAI_window, (AW_CL)0);
    awm->insert_menu_topic( "SetColors", "Change Colors ", "c","setColors.hlp", AWM_ALL,AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager);

    {
        awm->at(11,2);
        awm->auto_space(5,0);
        awm->shadow_width(1);

        int cur_x, cur_y, start_x, first_line_y, second_line_y, third_line_y;
        awm->get_at_position( &start_x,&first_line_y);
        awm->button_length(10);
        awm->callback( (AW_CB0)AW_POPDOWN );
        awm->create_button("Close", "Close");
    
        awm->get_at_position( &cur_x,&cur_y );
        awm->callback(AW_POPUP,(AW_CL)CreateSelectSAI_window,(AW_CL)0);
        awm->button_length(25);
        awm->create_button("SELECT_SAI", AWAR_SAI_SELECTED);

        awm->at_newline();
        awm->get_at_position( &start_x,&second_line_y);
        awm->button_length(15);
        awm->callback( (AW_CB0)AW_POPDOWN );
        awm->create_button("sai", "Select SAI");
    }

    AddCallBacks(awr, gl_Canvas);

    appContext = awr->prvt->context;

    extern int OpenGLEngineState; // defined globally in OpenGLEngine.cxx
    OpenGLEngineState = NOT_CREATED;
		
    /** Add event handlers */
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       StructureNotifyMask , 0, (XtEventHandler) ResizeOpenGLWindow, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       ExposureMask, 0, (XtEventHandler) ExposeOpenGLWindow, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       KeyPressMask, 0, (XtEventHandler) KeyBoardEventHandler, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       ButtonPressMask, 0, (XtEventHandler) ButtonPressEventHandler, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       ButtonReleaseMask, 0, (XtEventHandler) ButtonReleaseEventHandler, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       PointerMotionMask, 0, (XtEventHandler) MouseMoveEventHandler, (XtPointer) 0 );

//     XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
//                       VisibilityChangeMask, 0, (XtEventHandler) RotateMolecule, (XtPointer) 0 );
		
    extern Widget OpenGLParentWidget;
    OpenGLParentWidget = awm->p_w->areas[ AW_MIDDLE_AREA ]->area;

#ifdef DEBUG
    cout<<"Openglwindow created!!"<<endl;
#endif

    return awm;
}
