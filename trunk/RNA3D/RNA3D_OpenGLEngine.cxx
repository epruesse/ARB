#include <cstdio>
#include <climits>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <awt_nds.hxx>
#include <aw_preset.hxx>
#include <awt.hxx>
#include <awt_canvas.hxx>
#include <awt_tree.hxx>
#include <awt_dtree.hxx>
#include <awt_tree_cb.hxx>

#include <GL/glew.h>
#include <GL/GLwMDrawA.h>
#include "RNA3D_OpenGLEngine.hxx"

float fAspectRatio;
float fViewAngle = 90.0;
float fClipNear  = 0.1;
float fClipFar   = 10000;

//bool alpha_Size_Supported is defined in WINDOWS/AW_window.cxx 

int OpenGLEngineState = -1;

GBDATA *OpenGL_gb_main;

Widget _glw;

static Display *dpy;
static GLXContext glx_context;

static int attributes[]               = { GLX_RGBA, GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4, GLX_ALPHA_SIZE, 4, GLX_DOUBLEBUFFER, None };
static int attributes_Without_Alpha[] = { GLX_RGBA, GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4, GLX_DOUBLEBUFFER, None };
static int attributes_Single_Buffer[] = { GLX_RGBA, GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4, None };

using namespace std;

void ShowVendorInformation(){
	const GLubyte *vendor = NULL;  
	vendor = glGetString(GL_VENDOR);   cout<<"Vendor  : "<<vendor<<endl;
	vendor = glGetString(GL_RENDERER); cout<<"Rederer : "<<vendor<<endl;
	vendor = glGetString(GL_VERSION);  cout<<"Version : "<<vendor<<endl;
}

void ReshapeOpenGLWindow( GLint width, GLint height ) {
	extern float fAspectRatio;
	fAspectRatio = (float) width / (float) height;
	
	glViewport( 0, 0, width, height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	gluPerspective( fViewAngle, fAspectRatio, fClipNear, fClipFar );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();
}

void InitializeOpenGLEngine(GLint width, GLint height ) {
    cout<<"Initializing OpenGLEngine : "<<width<<"x"<<height<<endl;
    
	//Get Information about Vendor & Version
	ShowVendorInformation();

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* problem: glewInit failed, something is seriously wrong */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    }
    fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    ReshapeOpenGLWindow(width,height);
}

void drawObjects(){

	glColor4f(0.1, 0.1, 0.6, 1);
	glBegin(GL_QUADS);
	glVertex3f(-0.6, 0.5, -1.0);
	glVertex3f(-0.6, -0.4, -1.0);
	glVertex3f(0.6, -0.4, -1.0);
	glVertex3f(0.6, 0.5, -1.0);
	glEnd();
}

void RenderOpenGLScene(Widget w){

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity ();
	gluPerspective(fViewAngle, fAspectRatio, fClipNear, fClipFar );
	glClearColor(0,0,0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	drawObjects();

	glFlush();
	glXWaitX();
	glXSwapBuffers ( XtDisplay( w ), XtWindow( w ) );
}

void InitializeOpenGLWindow( Widget w ) {
    extern int OpenGLEngineState;
	if (OpenGLEngineState == CREATED) return;
	
	Arg args[1];
	XVisualInfo *vi;
	extern Widget _glw;
	
	XtSetArg(args[0], GLwNvisualInfo, &vi);
	XtGetValues(w, args, 1);
	
	dpy = XtDisplay(w);
	if (!dpy) {
		fprintf(stderr, "could not open display\n");
	} 
    else {
		extern bool alpha_Size_Supported;
		if (alpha_Size_Supported)
			vi = glXChooseVisual(dpy, DefaultScreen( dpy ), attributes);
		else
			vi = glXChooseVisual(dpy, DefaultScreen( dpy ), attributes_Without_Alpha);
				
		if (!vi) {
			fprintf(stderr, "try to get a single buffered visual\n");
			vi = glXChooseVisual(dpy, DefaultScreen(dpy), attributes_Single_Buffer);
			if (!vi)
				fprintf(stderr, "could not get visual\n");
		}
        else {
			glx_context = glXCreateContext(dpy, vi, NULL, GL_TRUE);
			if (!glXIsDirect(dpy, glx_context))
				fprintf(stderr, "direct rendering not supported\n");
			else
				printf("direct rendering supported\n");
			
			GLwDrawingAreaMakeCurrent(w, glx_context);
			
			_glw = w;
			
			OpenGLEngineState = CREATED;
		}
	}
}
