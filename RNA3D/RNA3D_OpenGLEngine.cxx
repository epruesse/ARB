#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_OpenGLEngine.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_StructureData.hxx"
#include "RNA3D_Textures.hxx"
#include "RNA3D_Renderer.hxx"
#include "RNA3D_Graphics.hxx"
#include "RNA3D_Interface.hxx"

OpenGLGraphics *cGraphics  = new OpenGLGraphics();
Structure3D    *cStructure = new Structure3D();
Texture2D      *cTexture   = new Texture2D();
GLRenderer *cRenderer = new GLRenderer();

Vector3 sCen;

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
float scale = 0.01;

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

    // Prepare the structure Data 

    cStructure->ReadCoOrdinateFile(&sCen);    // Reading Structure information
    cStructure->PrepareStructureSkeleton();    // Preparing structure skeleton with just coordinates  
    cStructure->GetSecondaryStructureInfo();
    cStructure->Combine2Dand3DstructureInfo();
    cStructure->BuildSecondaryStructureMask();

    // Generate Textures
    cTexture->LoadGLTextures();  // Load The Texture(s) 

    // Generate Display Lists  

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

    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

    ReshapeOpenGLWindow(width,height);

    CalculateRotationMatrix();
}

void PrintString(float x, float y, float z, char *s) {
    glRasterPos3d(x, y, z);
    for (unsigned int i = 0; i < strlen(s); i++) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, s[i]);
    }
}

void RotateMolecule(int x, int y) {
    GLfloat dx, dy;
    dx = saved_x - x; 
    dy = saved_y - y;
    rot_y = (GLfloat)(x - saved_x) * ROTATION_SPEED;
    rot_x = (GLfloat)(y - saved_y) * ROTATION_SPEED;
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

void DrawStructure(){
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
}


void ConvertGCtoRGB(AW_window *aww){
    //    for (int gc = int(RNA3D_GC_BACKGROUND); gc <= RNA3D_GC_MAX;  ++gc) {

    int gc = int(RNA3D_GC_BACKGROUND);
    float r, g, b;
    const char *error = aww->GC_to_RGB_float(aww->get_device(AW_MIDDLE_AREA), gc, r, g, b);
    if (error) {
        printf("Error retrieving RGB values for GC #%i: %s\n", gc, error);
        glClearColor(0,0,0,0);
    }
    else {
        printf("GC #%i RGB values: r=%.1f g=%.1f b=%.1f\n", gc, r, g, b);
        //        glClearColor(r,g,b,0);
    }
        //    }
}

void RenderOpenGLScene(Widget w){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    SetOpenGLBackGroundColor();  // setting the BackGround Color of the OpenGL Scene
    glLoadIdentity();

    gluLookAt(Viewer.x, Viewer.y, Viewer.z,
              Center.x, Center.y, Center.z,
              Up.x,     Up.y,     Up.z);

    glScalef(scale,scale,scale);

    if(rotateMolecule || autoRotate)  CalculateRotationMatrix();

    glPushMatrix();
    glMultMatrixf(rotation_matrix); 

    glTranslatef(-sCen.x, -sCen.y, -sCen.z);

    DrawStructure();
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

