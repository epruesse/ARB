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
GLRenderer     *cRenderer  = new GLRenderer();

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
bool bPointSpritesSupported = false;
float scale = 0.01;

using namespace std;

void ShowVendorInformation(){
	const GLubyte *vendor = NULL;  
	vendor = glGetString(GL_VENDOR);   cout<<"Vendor  : "<<vendor<<endl;
	vendor = glGetString(GL_RENDERER); cout<<"Rederer : "<<vendor<<endl;
	vendor = glGetString(GL_VERSION);  cout<<"Version : "<<vendor<<endl;
}

void initExtensions() {
    // check mandatory for extensions
	char missingExtensions[500]="";
	if (!GLEW_VERSION_1_2) {
        strcat(missingExtensions,"\nOpenGL Version 1.2");	
    }
	if (strlen(missingExtensions) > 0) {
		printf("ERROR: Some needed extensions are not present:%s\n",missingExtensions);
		char dummy;		scanf( "%c",&dummy);		exit(-1);
	} else {		
#ifdef DEBUG
			printf("DEBUG: All mandatory extensions seem to be ok.\n");
#endif // DEBUG
	}

	// the following code checks if point sprites could be used and activates them if possible
	missingExtensions[0] = 0;
	if (!GLEW_EXT_point_parameters) strcat(missingExtensions,"\nGL_EXT_point_parameters");
	if (!GLEW_ARB_point_sprite)		strcat(missingExtensions,"\nGL_ARB_point_sprite");	
	if (strlen(missingExtensions) > 0) {
		printf("Some extra extensions are not present:%s\n",missingExtensions);
		printf("Molecule Display: Quality of Rendering is LOW!!\n");
		bPointSpritesSupported = false;
	} else {		
		#ifdef _DEBUG
			printf("DEBUG: All extra extensions seem to be ok as well.\n");
		#endif // _DEBUG
		bPointSpritesSupported = true ;
	}
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

    initExtensions();

    // Prepare the structure Data  and Generate Display Lists  

    cStructure->ReadCoOrdinateFile();    // Reading Structure information
    cStructure->GetSecondaryStructureInfo();  // Getting Secondary Structure Information
    cStructure->Combine2Dand3DstructureInfo(); // Combining Secondary Structure data with 3D Coordinates

    //    cStructure->PrepareStructureSkeleton();    // Preparing structure skeleton with just coordinates  
    cStructure->GenerateDisplayLists(); // Generating Display Lists for Rendering

    // Generate Textures
    cTexture->LoadGLTextures();  // Load The Texture(s) 
    glEnable(GL_TEXTURE_2D);	// Enable Texture Mapping

    glClearDepth(1.0);			     // Enables Clearing Of The Depth Buffer
    glDepthFunc(GL_LESS);		     // The Type Of Depth Test To Do
    glEnable(GL_DEPTH_TEST);	   	 // Enables Depth Testing
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

void MapDisplayParameters(AW_root *root){
    // General Molecule Display Section
    cRenderer->iBackBone     = root->awar(AWAR_3D_MOL_BACKBONE)->read_int();
    cRenderer->iColorise     = root->awar(AWAR_3D_MOL_COLORIZE)->read_int();
    cRenderer->fSkeletonSize = root->awar(AWAR_3D_MOL_SIZE)->read_float();

    // Display Bases Section
    cRenderer->iDisplayBases     = root->awar(AWAR_3D_DISPLAY_BASES)->read_int();
    cRenderer->ObjectSize        = root->awar(AWAR_3D_DISPLAY_SIZE)->read_float();
    cRenderer->iBaseMode         = root->awar(AWAR_3D_BASES_MODE)->read_int();
    cRenderer->iBaseHelix        = root->awar(AWAR_3D_BASES_HELIX)->read_int();
    cRenderer->iBaseUnpairHelix  = root->awar(AWAR_3D_BASES_UNPAIRED_HELIX)->read_int();
    cRenderer->iBaseNonHelix     = root->awar(AWAR_3D_BASES_NON_HELIX)->read_int();
    cRenderer->iShapeHelix       = root->awar(AWAR_3D_SHAPES_HELIX)->read_int();
    cRenderer->iShapeUnpairHelix = root->awar(AWAR_3D_SHAPES_UNPAIRED_HELIX)->read_int();
    cRenderer->iShapeNonHelix    = root->awar(AWAR_3D_SHAPES_NON_HELIX)->read_int();

    //Display Helices Section
    cRenderer->iDisplayHelix  = root->awar(AWAR_3D_DISPLAY_HELIX)->read_int();
    cRenderer->iHelixMidPoint = root->awar(AWAR_3D_HELIX_MIDPOINT)->read_int();
    cRenderer->iHelixNrs      = root->awar(AWAR_3D_HELIX_NUMBER)->read_int();
    cRenderer->fHelixSize     = root->awar(AWAR_3D_HELIX_SIZE)->read_float();
    cRenderer->iHelixBackBone = root->awar(AWAR_3D_HELIX_BACKBONE)->read_int();
    cRenderer->iStartHelix    = root->awar(AWAR_3D_HELIX_FROM)->read_int();
    cRenderer->iEndHelix      = root->awar(AWAR_3D_HELIX_TO)->read_int();

    if (cRenderer->iStartHelix < 1 ||  cRenderer->iStartHelix > 50 ) {
        cout<<"Invalid Helix NUMBER !!"<<endl;
        root->awar(AWAR_3D_HELIX_FROM)->write_int(1);
    }
    if (cRenderer->iEndHelix < 1 ||  cRenderer->iEndHelix > 50 ) {
        cout<<"Invalid Helix NUMBER !!"<<endl;
        root->awar(AWAR_3D_HELIX_TO)->write_int(50);
    }

    if(cRenderer->iStartHelix > cRenderer->iEndHelix) {
        root->awar(AWAR_3D_HELIX_FROM)->write_int(cRenderer->iEndHelix - 1);
    } 
    else if(cRenderer->iEndHelix < cRenderer->iStartHelix) {
        root->awar(AWAR_3D_HELIX_TO)->write_int(cRenderer->iStartHelix + 1);
    }
}

void DrawStructure(){

    glPushMatrix();
    cRenderer->DoHelixMapping();
    glPopMatrix();

    glPushMatrix();
    cRenderer->DisplayMolecule();
    glPopMatrix();

    // Texture Mapping
    if(bPointSpritesSupported){
        cRenderer->BeginTexturizer();
        cRenderer->TexturizeStructure(cTexture);
        cRenderer->EndTexturizer();
    }

    if(!bPointSpritesSupported)
        {
            cRenderer->BeginTexturizer();
            
            glBindTexture(GL_TEXTURE_2D, cTexture->texture[STAR]); 
            glColor4f(0,0,1,1);
            glCallList(STRUCTURE_BACKBONE_POINTS);
            cRenderer->EndTexturizer();
        }
	
//     glPushMatrix();
//     glLineWidth(cRenderer->ObjectSize/2);
//     glColor4fv(BLUE);
//     glBegin(GL_LINE_STRIP);
//     glCallList(STRUCTURE_SEARCH);
//     glEnd();
//     glPopMatrix();

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

    glTranslatef(-cStructure->strCen->x, -cStructure->strCen->y, -cStructure->strCen->z);

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

