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
#include "RNA3D_Global.hxx"

float fAspectRatio;
float fViewAngle = 90.0;
float fClipNear  = 0.01;
float fClipFar   = 100000;

//bool alpha_Size_Supported is defined in WINDOWS/AW_window.cxx 

int OpenGLEngineState = -1;
GBDATA *OpenGL_gb_main;

Widget _glw;

static Display *dpy;
static GLXContext glx_context;

static int attributes[]               = { GLX_RGBA, GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4, GLX_ALPHA_SIZE, 4, GLX_DOUBLEBUFFER, None };
static int attributes_Without_Alpha[] = { GLX_RGBA, GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4, GLX_DOUBLEBUFFER, None };
static int attributes_Single_Buffer[] = { GLX_RGBA, GLX_RED_SIZE, 4, GLX_GREEN_SIZE, 4, GLX_BLUE_SIZE, 4, None };

static GLfloat rotation_matrix[16];
float ROTATION_SPEED = 0.5;
static GLfloat rot_x = 0.0, rot_y = 0.0;
GLfloat saved_x, saved_y;

Vector3 Viewer = Vector3(0.0, 0.0, -2);
Vector3 Center = Vector3(0.0, 0.0, 0.0);
Vector3 Up     = Vector3(0.0, 1.0, 0.0);

bool rotateMolecule = false;
bool autoRotate     = false;
bool enableZoom     = true;
float scale = 1.0;

#define ZOOM 0.5

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
    cout<<"Initializing OpenGLEngine : "<<width<<" x "<<height<<endl;
    
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

void DrawCube(float x, float y, float z, float radius);

void PrintString(float x, float y, float z, char *s) {
    glRasterPos3d(x, y, z);
    for (unsigned int i = 0; i < strlen(s); i++) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, s[i]);
    }
}

void drawObjects(){

    DrawCube(-0.5,-0.5,-0.5,1);

    PrintString(0,0,0,"Center");
    PrintString(-0.5,0,0,"Center");
}

void rotate(int x, int y) {
    GLfloat dx, dy;
    dx = saved_x - x; 
    dy = saved_y - y;
    if (enableZoom) {
        if (dy > 0) scale += ZOOM;
        else        scale -= ZOOM;
        if (scale<0.005) scale = 0.005;
    }
    else {
        rot_y = (GLfloat)(x - saved_x) * ROTATION_SPEED;
        rot_x = (GLfloat)(y - saved_y) * ROTATION_SPEED;
    }
    saved_x = x;
    saved_y = y;

}

void CalculateRotationMatrix(){
    static int initialized = 0;
    GLfloat new_rotation_matrix[16];

    /* calculate new rotation matrix */
    glPushMatrix();
    glLoadIdentity();
    glRotatef(-rot_x, 1.0, 0.0, 0.0);
    glRotatef(rot_y, 0.0, 1.0, 0.0);
    glGetFloatv(GL_MODELVIEW_MATRIX, new_rotation_matrix);
    glPopMatrix();
    
    /* calculate total rotation */
    glPushMatrix();
    glLoadIdentity();
    glMultMatrixf(new_rotation_matrix);
    if (initialized) {
      glMultMatrixf(rotation_matrix);
    }

    glGetFloatv(GL_MODELVIEW_MATRIX, rotation_matrix);
    initialized = 1;
    glPopMatrix();
}

void RenderOpenGLScene(Widget w){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 	glClearColor(0,0,0,0);
    glLoadIdentity();

    gluLookAt(Viewer.x, Viewer.y, Viewer.z,
              Center.x, Center.y, Center.z,
              Up.x,     Up.y,     Up.z);

    glScalef(scale,scale,scale);

    if(rotateMolecule || autoRotate)  CalculateRotationMatrix();

    glPushMatrix();
    glMultMatrixf(rotation_matrix); 
	drawObjects();
    glPopMatrix();

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

void DrawCube(float x, float y, float z, float radius)
{
	// Here we create 6 QUADS (Rectangles) to form a cube
	// With the passed in radius, we determine the width and height of the cube

	glBegin(GL_QUADS);		
    glColor4f(1,1,1,1);		
		// These vertices create the Back Side
		glVertex3f(x, y, z);
		glVertex3f(x, y + radius, z);
		glVertex3f(x + radius, y + radius, z); 
		glVertex3f(x + radius, y, z);
    glColor4f(1,0,0,1);		
		// These vertices create the Front Side
		glVertex3f(x, y, z + radius);
		glVertex3f(x, y + radius, z + radius);
		glVertex3f(x + radius, y + radius, z + radius); 
		glVertex3f(x + radius, y, z + radius);
    glColor4f(0,1,1,1);		
		// These vertices create the Bottom Face
		glVertex3f(x, y, z);
		glVertex3f(x, y, z + radius);
		glVertex3f(x + radius, y, z + radius); 
		glVertex3f(x + radius, y, z);
    glColor4f(1,1,0,1);		
		// These vertices create the Top Face
		glVertex3f(x, y + radius, z);
		glVertex3f(x, y + radius, z + radius);
		glVertex3f(x + radius, y + radius, z + radius); 
		glVertex3f(x + radius, y + radius, z);
    glColor4f(0,0,1,1);		
		// These vertices create the Left Face
		glVertex3f(x, y, z);
		glVertex3f(x, y, z + radius);
		glVertex3f(x, y + radius, z + radius); 
		glVertex3f(x, y + radius, z);
    glColor4f(1,0,1,1);		
		// These vertices create the Right Face
		glVertex3f(x + radius, y, z);
		glVertex3f(x + radius, y, z + radius);
		glVertex3f(x + radius, y + radius, z + radius); 
		glVertex3f(x + radius, y + radius, z);

	glEnd();
}
