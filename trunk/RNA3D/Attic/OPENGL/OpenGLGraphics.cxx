#include "GlobalHeader.hxx"
#include "OpenGLGraphics.hxx"
#include "Globals.hxx"

char HEXA_DECI_VALUE[] = {"000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9FA0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBFC0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF"};

OpenGLGraphics::OpenGLGraphics(void){
    displayGrid = false;
}

OpenGLGraphics::~OpenGLGraphics(void){
}

void OpenGLGraphics::SetBackGroundColor(int color){
    glClearColor(0,0,0,1);
}

void OpenGLGraphics::SetColor(char base) {
    switch (base) {
    case 'A': glColor4fv(BLUE);    break;
    case 'G': glColor4fv(GREEN);   break;
    case 'U': glColor4fv(WHITE);   break;
    case 'C': glColor4fv(RED);     break;
    default : glColor4fv(BLACK); break;
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

void OpenGLGraphics::DrawCursor(int x, int y) {
    GLdouble screenPos[3];
    WinToScreenCoordinates(x, y, screenPos);
    glPushMatrix();
    glColor4fv(RED); 
    DrawCube((float)screenPos[0],(float)-screenPos[1],(float)screenPos[2], 0.01);
    glPopMatrix();
}

void OpenGLGraphics::DrawPoints(float x, float y, float z, char base) {
    SetColor(base);     // Set color
    glVertex3f(x,y,z);   // Draw Points
}

void OpenGLGraphics::PrintString(float x, float y, float z, char *s, void *font) {
    glRasterPos3d(x, y, z);
    for (unsigned int i = 0; i < strlen(s); i++) {
        glutBitmapCharacter(font, s[i]);
    }
}

void OpenGLGraphics::HexaDecimalToRGB(char *color, float *RGB){
    float R, G, B; R=G=B=1.0;
    if(strlen(color)<6) {
        cout<<"Invalid Color \""<<color<<"\" ! Setting default White !"<<endl; 
        RGB[0] = R; RGB[1] = G; RGB[2] = B;
    }
    else {
        int r,g,b; r=g=b=FALSE;
        char HEXA[3]; HEXA[2] = '\0';
        char hexa_R[3]; hexa_R[0] = color[0]; hexa_R[1] = color[1]; hexa_R[2] ='\0';
        char hexa_G[3]; hexa_G[0] = color[2]; hexa_G[1] = color[3]; hexa_G[2] ='\0';
        char hexa_B[3]; hexa_B[0] = color[4]; hexa_B[1] = color[5]; hexa_B[2] ='\0';

        for(unsigned int i = 0, k = 0; i<strlen(HEXA_DECI_VALUE); i += 2, k++) {
            HEXA[0] = HEXA_DECI_VALUE[i];
            HEXA[1] = HEXA_DECI_VALUE[i+1];
            if(strcmp(hexa_R, HEXA) == 0) { R = (float) k/255; r = TRUE;}
            if(strcmp(hexa_G, HEXA) == 0) { G = (float) k/255; g = TRUE;}
            if(strcmp(hexa_B, HEXA) == 0) { B = (float) k/255; b = TRUE;}
            if(r && g && b) break;
        }
        RGB[0] = R; RGB[1] = G; RGB[2] = B;
    }
}

void  OpenGLGraphics::ClickedPos(float x, float y, float z){
//     mouseX = x;
//     mouseY = y;
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
      glColor4fv(RED);  // X axis
      glVertex3f(x, y, z);
      glVertex3f(x + len, y, z);

      glColor4fv(BLUE); // Y axis
      glVertex3f(x, y, z);
      glVertex3f(x, y + len, z);

      glColor4fv(GREEN);// Z axis
      glVertex3f(x, y, z);
      glVertex3f(x, y, z + len);
    glEnd();

    glColor4fv(RED);  // X axis
    PrintString(x + len, y, z, "X",GLUT_BITMAP_8_BY_13);
    glColor4fv(BLUE); // Y axis
    PrintString(x, y + len, z, "Y",GLUT_BITMAP_8_BY_13);
    glColor4fv(GREEN);// Z axis
    PrintString(x, y, z + len, "Z",GLUT_BITMAP_8_BY_13);

    glLineWidth(1.0);
    glColor4fv(DEFAULT);
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
