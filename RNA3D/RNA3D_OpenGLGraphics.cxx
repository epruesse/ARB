#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_Renderer.hxx"
#include "RNA3D_Textures.hxx"
#include "RNA3D_StructureData.hxx"

#include <math.h>


using namespace std;

OpenGLGraphics::OpenGLGraphics(void){
    displayGrid = false;
}

OpenGLGraphics::~OpenGLGraphics(void){
}

void OpenGLGraphics::SetBackGroundColor(int color){
    glClearColor(0,0,0,1);
}

void OpenGLGraphics::SetColor(int gc) {
    float r, g, b;
    extern AW_window_menu_modes_opengl *awm;
    const char *ERROR = awm->GC_to_RGB_float(awm->get_device(AW_MIDDLE_AREA), gc, r, g, b);
    if (ERROR) {
        cout<<"Error retrieving RGB values for GC #"<<gc<<" : "<<ERROR
            <<endl<<"Setting the color to default white"<<endl;
        glColor4f(1,1,1,1);
    }
    else {
        glColor4f(r,g,b,1);
    }
}

ColorRGBf OpenGLGraphics::GetColor(int gc) {
    ColorRGBf clr = ColorRGBf(0,0,0);
    float r, g, b;

    extern AW_window_menu_modes_opengl *awm;
    const char *ERROR = awm->GC_to_RGB_float(awm->get_device(AW_MIDDLE_AREA), gc, r, g, b);
    if (ERROR) {
        cout<<"Error retrieving RGB values for GC #"<<gc<<" : "<<ERROR<<endl;
        return clr;
    }
    else {
        clr = ColorRGBf(r,g,b);
        return clr;
    }
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
    glRasterPos3d(x, y, z);
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
    PrintString(x + len, y, z, "X",GLUT_BITMAP_8_BY_13);
    glColor4f(0,0,1,1); // Y axis
    PrintString(x, y + len, z, "Y",GLUT_BITMAP_8_BY_13);
    glColor4f(0,1,0,1);// Z axis
    PrintString(x, y, z + len, "Z",GLUT_BITMAP_8_BY_13);

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
