#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <iostream>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <awt_canvas.hxx>
#include <awt.hxx>
#include <aw_root.hxx>

#include <GL/glew.h>
#include <GL/GLwMDrawA.h>
//#include <GL/glx.h>

#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#include <Xm/MwmUtil.h>
#include <Xm/MainW.h>

#define _AW_COMMON_INCLUDED
#include <aw_root.hxx>
#include <aw_device.hxx>
//#include "../WINDOW/aw_commn.hxx"
#include "../WINDOW/aw_at.hxx"
#include <aw_window.hxx>
#include "../WINDOW/aw_awar.hxx"
#include "../WINDOW/aw_xfig.hxx"
#include "../WINDOW/aw_xfigfont.hxx"
#include "../WINDOW/aw_Xm.hxx"
#include "../WINDOW/aw_click.hxx"
#include "../WINDOW/aw_size.hxx"
#include "../WINDOW/aw_print.hxx"
#include "../WINDOW/aw_window_Xm.hxx"
#include "../WINDOW/aw_xkey.hxx"

#include "RNA3D_Graphics.hxx"
#include "RNA3D_OpenGLEngine.hxx"
#include "RNA3D_Interface.hxx"

using namespace std;

extern GBDATA *gb_main;
Widget OpenGLParentWidget;

void resize( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	XConfigureEvent *evt;
	evt = (XConfigureEvent*) event;
	
	extern int OpenGLEngineState;  // because it is globally defined in GEN_OpenGL.cxx
	
	if ( OpenGLEngineState == NOT_CREATED ) {
		return;
	}
	
	ReshapeOpenGLWindow( (GLint) evt->width, (GLint) evt->height );
	
    //	gmw->refresh();
}


void keyboard( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	XKeyEvent *evt;
	evt = (XKeyEvent*) event;
	
	//printf( "Key = %d\n", evt->keycode );
	
	if ( evt->keycode == 100 ) { /** Left */
        //		opengl_move_x += step;
	}
	else if ( evt->keycode == 102 ) { /** Right */
        //		opengl_move_x -= step;
	}
	else if ( evt->keycode == 98 ) { /** Up */
        //		opengl_move_y -= step;
	}
	else if ( evt->keycode == 104 ) { /** Down */
        //		opengl_move_y += step;
	}
	else if ( evt->keycode == 65 ) { /** Space */
        //		toggle_information_quad = !toggle_information_quad;
	}
	else if ( evt->keycode == 97 ) { /** Home */
    }
    cout<<"keyboard event occured"<<endl;
    //    refreshOpenGLDisplay();
    //    gl_Canvas->refresh();
}

void button_release_event_handler( Widget w, XtPointer client_data, XEvent *event, char* x ) {
}

/// Deals with ButtonPress events.
void button_press_event_handler( Widget w, XtPointer client_data, XEvent *event, char* x ) {

	XButtonEvent *xp;
	xp = (XButtonEvent*) event;
	
	if ( xp->button == 1 ) { // left
        //		if ( gl_Canvas->mode == AWT_MODE_MOD ) {
        //		}
    //		else if ( gl_Canvas->mode == AWT_MODE_ZOOM ) {
        //		}
	}
	else if ( xp->button == 3 ) { // right
	}
	else if ( xp->button == 4 ) { // scroll up ( mouse-wheel )
	}
	else if ( xp->button == 5 ) { // scroll down ( mouse-wheel )
	}

    cout<<"Mouse event occured"<<endl;

    refreshOpenGLDisplay();

    //    gl_Canvas->refresh();
}

void mouse_move_event_handler( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	
	XMotionEvent *xp;
	xp = (XMotionEvent*) event;
	
	if ( xp->state & Button3Mask ) { //Rechte Maustaste
	}
	else if ( xp->state & Button3Mask ) {
		/* do nothing*/
	}

    cout<<"Mouse move occured"<<endl;

    refreshOpenGLDisplay();
   	//gl_Canvas->refresh();
}

void expose( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	
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
	
    refreshOpenGLDisplay();
    //gl_Canvas->refresh();
}

void init( Widget w, XtPointer client_data, XtPointer call_data ) {
	InitializeOpenGLWindow( w );
    cout<<"OpenGl window initialized"<<endl;
}

void refreshOpenGLDisplay() {
	extern int OpenGLEngineState;
	if ( OpenGLEngineState == CREATED ) {
		extern Widget _glw;
        RenderOpenGLScene(_glw);
    }
}

/*---------------------------- Creating WINDOWS ------------------------------ */
static void refreshCanvas(AW_root *awr, AW_CL cl_ntw) {
    // repaints the canvas
    AWUSE(awr);
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    ntw->refresh();
}

static void addCallBacks(AW_root *awr, AWT_canvas *ntw) {
    // adding callbacks to the awars to refresh the display if recieved any changes
}

static AW_window *createSelectSAI_window(AW_root *aw_root){
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

AW_window *createRNA3DMainWindow(AW_root *awr){
    // Main Window - Canvas on which the actual painting is done
    GB_transaction dummy(gb_main);

    awr->awar_int(AWAR_SAI_SELECTED, 0, AW_ROOT_DEFAULT); 
    int width = 600, height = 400;

    AW_window_menu_modes_opengl *awm = new AW_window_menu_modes_opengl();
    awm->init(awr,"RNA3D", "RNA3D: 3D Structure of Ribosomal RNA",width,height);
    AW_gc_manager aw_gc_manager;
    RNA3D_Graphics *rna3DGraphics = new RNA3D_Graphics(awr,gb_main);

    AWT_canvas *gl_Canvas = (AWT_canvas *) new AWT_canvas(gb_main,awm, rna3DGraphics, aw_gc_manager,AWAR_SPECIES_NAME);

    gl_Canvas->recalc_size();
    gl_Canvas->refresh();
    gl_Canvas->set_mode(AWT_MODE_ZOOM); // Default-Mode

    awm->insert_help_topic("rna3D_help_how", "How to Visualize 3D structure of rRNA ?", "H", "rna3DHelp.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"rna3DHelp.hlp", 0);
    awm->create_menu( 0, "File", "F", 0,  AWM_ALL );
    awm->insert_menu_topic( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);
    awm->create_menu( 0, "Properties", "P", 0,  AWM_ALL );
    awm->insert_menu_topic( "selectSAI", "Select SAI", "S","selectSai.hlp", AWM_ALL,AW_POPUP, (AW_CL)createSelectSAI_window, (AW_CL)0);
    awm->insert_menu_topic( "SetColors", "Set Colors and Fonts", "t","setColors.hlp", AWM_ALL,AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager);

    addCallBacks(awr, gl_Canvas);

    extern int OpenGLEngineState; // defined globally in OpenGLEngine.cxx
    OpenGLEngineState = NOT_CREATED;
		
    /** Add event handlers */
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       StructureNotifyMask , 0, (XtEventHandler) resize, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       ExposureMask, 0, (XtEventHandler) expose, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       KeyPressMask, 0, (XtEventHandler) keyboard, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       ButtonPressMask, 0, (XtEventHandler) button_press_event_handler, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       ButtonReleaseMask, 0, (XtEventHandler) button_release_event_handler, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       PointerMotionMask, 0, (XtEventHandler) mouse_move_event_handler, (XtPointer) 0 );
		
    extern Widget OpenGLParentWidget;
    OpenGLParentWidget = awm->p_w->frame;
    cout<<"openglwindow created"<<endl;
    return awm;
}
