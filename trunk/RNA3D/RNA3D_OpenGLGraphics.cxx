#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_Renderer.hxx"
#include "RNA3D_Textures.hxx"
#include "RNA3D_StructureData.hxx"

#include <math.h>

// The following includes are needed to use AW_window_Motif 
// and access Application's colormap
#include "../WINDOW/aw_awar.hxx"
#include "../WINDOW/aw_Xm.hxx"
#include "../WINDOW/aw_click.hxx"
#include "../WINDOW/aw_size.hxx"
#include "../WINDOW/aw_print.hxx"
#include "../WINDOW/aw_window_Xm.hxx"
#include "../WINDOW/aw_commn.hxx"

using namespace std;

OpenGLGraphics::OpenGLGraphics(void)
    : displayGrid(false)
    , ApplicationBGColor(0, 0, 0)
{
}

OpenGLGraphics::~OpenGLGraphics(void){
}

// Sets the Background Color for the OpenGL Window
void OpenGLGraphics::SetOpenGLBackGroundColor() {

	unsigned long bgColor;
	XtVaGetValues( RNA3D->glw, XmNbackground, &bgColor, NULL );

    Widget w = RNA3D->gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area;

    XColor xcolor;
    xcolor.pixel = bgColor;
    Colormap colormap = DefaultColormapOfScreen( XtScreen( w ) );
    XQueryColor( XtDisplay( w ), colormap, &xcolor );

    float r, g, b; r = g = b = 0.0;
    r = (float) xcolor.red / 65535.0;
    g = (float) xcolor.green / 65535.0;
    b = (float) xcolor.blue / 65535.0;

    // set OpenGL Backgroud Color to the widget's backgroud     
    glClearColor(r, g, b, 1);

    // extern ColorRGBf ApplicationBGColor;
    ApplicationBGColor = ColorRGBf(r, g, b);
}

// Converts the GC into RGB values 
ColorRGBf OpenGLGraphics::ConvertGCtoRGB(int gc) {
    ColorRGBf clr = ColorRGBf(0,0,0);
    float r, g, b; r = g = b = 0.0;
   
    Widget w = RNA3D->gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area;

    AW_common *common = RNA3D->gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->common;
    register class AW_GC_Xm *gcm = AW_MAP_GC( gc );
 
    XGCValues xGCValues;
    XGetGCValues( XtDisplay( w ), gcm->gc, GCForeground, &xGCValues );
    unsigned long color = xGCValues.foreground;

    XColor xcolor;
    xcolor.pixel = color;

    Colormap colormap = DefaultColormapOfScreen( XtScreen( w ) );
	XQueryColor( XtDisplay( w ), colormap, &xcolor );
	
	r = (float) xcolor.red / 65535.0;
	g = (float) xcolor.green / 65535.0;
	b = (float) xcolor.blue / 65535.0;

    clr = ColorRGBf(r,g,b);
    return clr;
}

// Converts the GC into RGB values and sets the glColor
void OpenGLGraphics::SetColor(int gc) {
    ColorRGBf color = ConvertGCtoRGB(gc);
    glColor4f(color.red, color.green, color.blue, 1);
}

// Converts the GC into RGB values and returns them 
ColorRGBf OpenGLGraphics::GetColor(int gc) {
    ColorRGBf color = ConvertGCtoRGB(gc);
    return color;
}

void OpenGLGraphics::WinToScreenCoordinates(int x, int y, GLdouble *screenPos) {
    //project window coords to gl coord
    glLoadIdentity();
    GLdouble modelMatrix[16];
    glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);
    GLdouble projMatrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX,projMatrix);
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT,viewport);
    gluUnProject(x, y, 0,
                 modelMatrix,
                 projMatrix,
                 viewport,
                 //the next 3 parameters are the pointers to the final object coordinates.(double)
                 &screenPos[0], &screenPos[1], &screenPos[2]
                 );
}

void OpenGLGraphics::ScreenToWinCoordinates(int x, int y, GLdouble *winPos) {
    //project window coords to gl coord
    glLoadIdentity();
    GLdouble modelMatrix[16];
    glGetDoublev(GL_MODELVIEW_MATRIX,modelMatrix);
    GLdouble projMatrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX,projMatrix);
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT,viewport);
    gluProject(x, y, 0,
               modelMatrix,
               projMatrix,
               viewport,
               //the next 3 parameters are the pointers to the final object coordinates.(double)
               &winPos[0], &winPos[1], &winPos[2]
               );
}

void OpenGLGraphics::DrawCursor(int x, int y) {
    GLdouble screenPos[3];
    WinToScreenCoordinates(x, y, screenPos);
    glPushMatrix();
    glColor4f(1,0,0,1); 
    DrawCube((float)screenPos[0],(float)-screenPos[1],(float)screenPos[2], 0.01);
    glPopMatrix();
}

void OpenGLGraphics::PrintString(float x, float y, float z, char *s, void *font) {
    glRasterPos3f(x, y, z);
    for (unsigned int i = 0; i < strlen(s); i++) {
        glutBitmapCharacter(font, s[i]);
    }
}

void OpenGLGraphics::PrintCharacter(float x, float y, float z, char c, void *font) {
    glRasterPos3d(x, y, z);
    glutBitmapCharacter(font, c);
}

void OpenGLGraphics::DrawSphere(float radius, float x, float y, float z){
    GLUquadricObj *newObj = gluNewQuadric();
    gluQuadricDrawStyle(newObj, GLU_FILL);
    gluQuadricNormals(newObj, GLU_SMOOTH);
    glVertex3f(x,y,z);
    gluSphere(newObj,radius, 10, 10);
    gluDeleteQuadric(newObj);
}

void OpenGLGraphics::DrawCircle(float radius,float x, float y){
    glNewList(CIRCLE_LIST, GL_COMPILE);

    glBegin(GL_POLYGON);
    float degInRad, cosine, sine;
    for (int i=0; i < 360; i++) {
        degInRad = i * DEG2RAD;
        cosine   = cos(degInRad)*radius;
        sine     = sin(degInRad)*radius;
        glVertex2f(x+cosine, y+sine);
    }
    glEnd();

    glEndList();
}

void  OpenGLGraphics::DrawCube(float x, float y, float z, float radius)
{
	// Here we create 6 QUADS (Rectangles) to form a cube
	// With the passed in radius, we determine the width and height of the cube

	glBegin(GL_QUADS);		
		
		// These vertices create the Back Side
		glVertex3f(x, y, z);
		glVertex3f(x, y + radius, z);
		glVertex3f(x + radius, y + radius, z); 
		glVertex3f(x + radius, y, z);

		// These vertices create the Front Side
		glVertex3f(x, y, z + radius);
		glVertex3f(x, y + radius, z + radius);
		glVertex3f(x + radius, y + radius, z + radius); 
		glVertex3f(x + radius, y, z + radius);

		// These vertices create the Bottom Face
		glVertex3f(x, y, z);
		glVertex3f(x, y, z + radius);
		glVertex3f(x + radius, y, z + radius); 
		glVertex3f(x + radius, y, z);
			
		// These vertices create the Top Face
		glVertex3f(x, y + radius, z);
		glVertex3f(x, y + radius, z + radius);
		glVertex3f(x + radius, y + radius, z + radius); 
		glVertex3f(x + radius, y + radius, z);

		// These vertices create the Left Face
		glVertex3f(x, y, z);
		glVertex3f(x, y, z + radius);
		glVertex3f(x, y + radius, z + radius); 
		glVertex3f(x, y + radius, z);

		// These vertices create the Right Face
		glVertex3f(x + radius, y, z);
		glVertex3f(x + radius, y, z + radius);
		glVertex3f(x + radius, y + radius, z + radius); 
		glVertex3f(x + radius, y + radius, z);

	glEnd();
}

void OpenGLGraphics::DrawAxis(float x, float y, float z, float len){
    // This function draws the axis at the defined position on the screen

    glLineWidth(5.0);

    glBegin(GL_LINES);
    glColor4f(1,0,0,1);  // X axis
      glVertex3f(x, y, z);
      glVertex3f(x + len, y, z);

      glColor4f(0,0,1,1); // Y axis
      glVertex3f(x, y, z);
      glVertex3f(x, y + len, z);

      glColor4f(0,1,0,1);// Z axis
      glVertex3f(x, y, z);
      glVertex3f(x, y, z + len);
    glEnd();

    glColor4f(1,0,0,1);  // X axis
    //    PrintString(x + len, y, z, "X",GLUT_BITMAP_8_BY_13);
    glColor4f(0,0,1,1); // Y axis
    // PrintString(x, y + len, z, "Y",GLUT_BITMAP_8_BY_13);
    glColor4f(0,1,0,1);// Z axis
    // PrintString(x, y, z + len, "Z",GLUT_BITMAP_8_BY_13);

    glLineWidth(1.0);
}

void OpenGLGraphics::Draw3DSGrid() {
	// This function was added to give a better feeling of moving around.
	// A black background just doesn't give it to ya :)  We just draw 100
	// green lines vertical and horizontal along the X and Z axis.

	// Turn the lines GREEN
	glColor3ub(0, 255, 0);

	// Draw a 1x1 grid along the X and Z axis'
	for(float i = -50; i <= 50; i += 1)
	{
		// Start drawing some lines
		glBegin(GL_LINES);

			// Do the horizontal lines (along the X)
			glVertex3f(-50, 0, i);
			glVertex3f(50, 0, i);

			// Do the vertical lines (along the Z)
			glVertex3f(i, 0, -50);
			glVertex3f(i, 0, 50);

		// Stop drawing lines
		glEnd();
	}
}
