#include "GlobalHeader.hxx"
#include "Main.hxx"
//#include "KeyBoard.hxx"
#include "Mouse.hxx"
#include "Menu.hxx"

//=======================  Functions =======================================
void KeyBoardFunction(unsigned char key, int x, int y);
void SpecialKey(int key, int x, int y);
void MouseMain(int btn, int state, int x, int y);
void MenuFunction(int value);
void ReshapeMain(GLint width, GLint height);
void MakeMenu(void);
void RotateStructure(float x, float y);
void rotate(int x, int y);
void IdleFunction(void);
void DisplayMain(void);
void Init(void);
void CalculateRotationMatrix();
//============================================================================

// //Global Class declarations 
// OpenGLGraphics *cGraphics  = new OpenGLGraphics();
// Structure3D    *cStructure = new Structure3D();
// Texture2D      *cTexture   = new Texture2D();
// GLRenderer     *cRenderer  = new GLRenderer();
// CCamera         cCamera;

int main(int argc,char** argv) {
    glutInit (&argc, argv);
    Init();                         // initialization of window and associated functions
    glutMainLoop ();                // event processing loop
    return 0;
}

Vector3 sCenter;

void Init(void){
    // glut initializatin commands
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);

    //window related commands
    glutInitWindowPosition (20, 20);
    glutInitWindowSize (SCREEN_WIDTH, SCREEN_HEIGHT);
    cGraphics->screenXmax = SCREEN_WIDTH; cGraphics->screenYmax = SCREEN_HEIGHT;
    glutCreateWindow ((char*)TITLE);
    
    {
        GLenum err = glewInit();
        if (GLEW_OK != err) {
            /* problem: glewInit failed, something is seriously wrong */
            fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        }
        fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
    }

    // function associated commands - event processing
    glutDisplayFunc(DisplayMain);  // display function
    glutReshapeFunc(ReshapeMain);  // redisplay / resize
    glutIdleFunc(IdleFunction);    // idle function

    //    glutPassiveMotionFunc(RotateScene);
    glutMotionFunc(rotate); // mouse movements when one or more buttons pressed
    glutMouseFunc(MouseMain); // mouse function
    glutKeyboardFunc(KeyBoardFunction); //keyboard function
    glutIgnoreKeyRepeat(1);  // key repeat
    glutSpecialFunc(SpecialKey);

    // openGL initialization 
    cTexture->LoadGLTextures();              // Load The Texture(s) 
    glEnable(GL_TEXTURE_2D);			    // Enable Texture Mapping

    glClearDepth(1.0);				        // Enables Clearing Of The Depth Buffer
    glDepthFunc(GL_LESS);			         // The Type Of Depth Test To Do
    glEnable(GL_DEPTH_TEST);		    	 // Enables Depth Testing
    glShadeModel(GL_FLAT);			 

    glPointSize(1.0);
    if (!glIsEnabled(GL_POINT_SMOOTH)) {
        glEnable(GL_POINT_SMOOTH);
    }

    glLineWidth(1.0);
    if(!glIsEnabled(GL_LINE_SMOOTH)) {
        glEnable(GL_LINE_SMOOTH);
    }

    MakeMenu(); // menu function

    cStructure->ReadCoOrdinateFile(&sCen);         // Reading Structure information
    cStructure->PrepareStructureSkeleton();    // Preparing structure skeleton with just coordinates  

    cStructure->GetSecondaryStructureInfo();
    //   cStructure->PrepareSecondaryStructureInfo()
    cStructure->Combine2Dand3DstructureInfo();
    cStructure->BuildSecondaryStructureMask();
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	// Inititialization of camera position
	cCamera.PositionCamera(0,0,-2,                         // Position 
                           0,0,0,
                           0, 1, 0 );                                                      //upVector

}

void DisplayMain(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    cGraphics->SetBackGroundColor(BACKGROUND_CLR);

    glLoadIdentity();   // Reset the matrix

    gluLookAt(Viewer.x, Viewer.y, Viewer.z,
              Center.x, Center.y, Center.z,
              Up.x,     Up.y,     Up.z);

    glScalef(scale,scale,scale);

    if(rotateMolecule || autoRotate) CalculateRotationMatrix();

    glPushMatrix();
    glMultMatrixf(rotation_matrix); 

    glTranslatef(-sCen.x, -sCen.y, -sCen.z);

    {  // Actual Drawing Routine
        if(dispHelices) {
            char buf[100];
            glColor4fv(DEFAULT);
            sprintf(buf, "Helices Displayed (HELIX N0.) = %d - %d", cRenderer->StartHelix, cRenderer->EndHelix);
            cGraphics->PrintString(sCen.x,sCen.y,sCen.z,buf,GLUT_BITMAP_8_BY_13);
        }

        glPushMatrix();
        cRenderer->BeginTexturizer();
        if (dispBases)         cRenderer->TexturizeStructure(MODE, cTexture, cGraphics);
        if (dispNonHelixBases) cRenderer->TexturizeStructure(HELIX_MASK, cTexture, cGraphics);
        cRenderer->EndTexturizer();
        glPopMatrix();

        glPushMatrix();
        if (dispHelices)       cRenderer->DisplayHelices();
        if (dispHelixBackbone) cRenderer->DisplayHelixBackBone();
        if (dispPositions)     cRenderer->DisplayPositions();
        if (dispHelixNrs)      cRenderer->DisplayHelixNumbers();
        glPopMatrix();

        glPushMatrix();
        glLineWidth(cRenderer->ObjectSize/2);
        glColor4fv(BLUE);
        glBegin(GL_LINE_STRIP);
        glCallList(STRUCTURE_SEARCH);
        glEnd();
        glPopMatrix();

        glPushMatrix();
        if(iStructurize){
            glCallList(STRUCTURE_BACKBONE_CLR);
        }
        else if (dispBackBone) {
            glColor4fv(DEFAULT);
            glCallList(STRUCTURE_BACKBONE);
        }
        glPopMatrix();

        if (showCursor) {
            POINT click;
            cCamera.GetCursorPos(&click);
            cGraphics->DrawCursor(click.x, click.y);
        }
    }
    glPopMatrix();

    glutSwapBuffers();
}

void IdleFunction(void) {
    if(autoRotate) {
        ROTATION_SPEED = 0.05;
        glutPostRedisplay();
    }
    else {
        ROTATION_SPEED = 0.5;
    }
}

void ReshapeMain(GLint width, GLint height) {
    glViewport(0, 0, width, height);  // Drawing Boundries

    glMatrixMode(GL_PROJECTION);  // Select The Projection Matrix
    glLoadIdentity();             // Reset The Projection Matrix

    gluPerspective(FOV_ANGLE, (float)width / height, CLIP_NEAR, CLIP_FAR);

    glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
    glLoadIdentity();           // Reset The Modelview Matrix
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

    glutPostRedisplay();
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


void KeyBoardFunction(unsigned char key, int x, int y){
    switch(key) {
    case '=' : if ((cRenderer->StartHelix > 0) && (cRenderer->StartHelix < 50))  cRenderer->StartHelix++; break;
    case '-' : if ((cRenderer->StartHelix > 1) && (cRenderer->StartHelix <= 50)) cRenderer->StartHelix--; break;
    case '_' : if ((cRenderer->EndHelix > 1)   && (cRenderer->EndHelix <= 50))   cRenderer->EndHelix--;   break;
    case '+' : if ((cRenderer->EndHelix > 0)   && (cRenderer->EndHelix < 50))    cRenderer->EndHelix++;   break;
    case 27  : exit (0);                 break;
    case 'g' : glShadeModel (GL_SMOOTH); break;
    case 'f' : glShadeModel (GL_FLAT);   break;
    case 'q' : glPolygonMode(GL_FRONT_AND_BACK,GL_POINT); break;
    case 'w' : glPolygonMode(GL_FRONT_AND_BACK,GL_LINE); break;
    case 'e' : glPolygonMode(GL_BACK,GL_FILL);  break;
    case 'T' : cRenderer->HelixWidth += 0.5; if(cRenderer->HelixWidth>10)  cRenderer->HelixWidth = 10; break;
    case 't' : cRenderer->HelixWidth -= 0.5; if(cRenderer->HelixWidth<0.5) cRenderer->HelixWidth = 0.5; break;
    case 'O' : cRenderer->ObjectSize += 0.5; break;
    case 'o' : cRenderer->ObjectSize -= 0.5; break;
    case 'r' : autoRotate = !autoRotate;  break;
    case '1' : iStructurize      = (!iStructurize)?      TRUE : FALSE; break;
    case '2' : dispBackBone      = (!dispBackBone)?      TRUE : FALSE; break;
    case '3' : dispBases         = (!dispBases)?         TRUE : FALSE; break;
    case '4' : dispHelixBackbone = (!dispHelixBackbone)? TRUE : FALSE; break;
    case '5' : dispHelices       = (!dispHelices)?       TRUE : FALSE; break;
    case '6' : dispHelixNrs      = (!dispHelixNrs)?      TRUE : FALSE; break;
    case '7' : dispNonHelixBases = (!dispNonHelixBases)? TRUE : FALSE; break;
    case '8' : dispPositions     = (!dispPositions)?     TRUE : FALSE; break;
    case '9' : showCursor        = (!showCursor)?        TRUE : FALSE; break;
    case 'H' : MODE = BASES_HELIX;          break;
    case 'N' : MODE = BASES_NON_HELIX;      break;
    case 'U' : MODE = BASES_UNPAIRED_HELIX; break;
    case 'L' : MODE = BASES_LOOP;           break;
    case 'b' : MODE = BASES;                break;
    case 'B' : MODE = BASES_COLOR;          break;
    }
}

void SpecialKey(int key, int x, int y) {
    Vector3 vView = cCamera.View();
    switch (key) {
    case GLUT_KEY_LEFT:
        // We want to rotate around the Y axis so we pass in a positive Y speed
        cCamera.RotateAroundPoint(vView, kSpeed, 0, 1, 0);	
        //        rotY += 0.5;
        break;

    case GLUT_KEY_RIGHT:
		// Use a negative Y speed to rotate around the Y axis
        cCamera.RotateAroundPoint(vView, -kSpeed, 0, 1, 0);	
        //        rotY -= 0.5;
        break;

    case GLUT_KEY_UP:
        //        cCamera.RotateView(Angle, 1, 0, 0);
        cCamera.RotateAroundPoint(vView, kSpeed, 1, 0, 0);	
        //        rotX += 0.5; 
        break;

    case GLUT_KEY_DOWN:
        //        cCamera.RotateView(-Angle, 1, 0, 0);
        cCamera.RotateAroundPoint(vView, -kSpeed, 1, 0, 0);	
        //        rotX -= 0.5;
        break;

    case GLUT_KEY_PAGE_UP:
        cCamera.MoveCamera(zoomFactor);
        break;

    case GLUT_KEY_PAGE_DOWN:
        cCamera.MoveCamera(-zoomFactor);
        break;
    }
    glutPostRedisplay();
}

