#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_OpenGLEngine.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_StructureData.hxx"
#include "RNA3D_Textures.hxx"
#include "RNA3D_Renderer.hxx"
#include "RNA3D_Graphics.hxx"
#include "RNA3D_Interface.hxx"
//#include "RNA3D_Main.hxx"

OpenGLGraphics *cGraphics  = new OpenGLGraphics();
Structure3D    *cStructure = new Structure3D();
Texture2D      *cTexture   = new Texture2D();
GLRenderer     *cRenderer  = new GLRenderer();

float fAspectRatio;
float fViewAngle = 90.0;
float fClipNear  = 0.01;
float fClipFar   = 100000;

int OpenGLEngineState = -1;
GBDATA *OpenGL_gb_main;

Widget _glw;

static Display *dpy;
static GLXContext glx_context;

static int DoubleBuffer[] = { GLX_RGBA, 
                              GLX_DEPTH_SIZE, 12, 
                              GLX_STENCIL_SIZE, 1,  
                              GLX_RED_SIZE, 4, 
                              GLX_GREEN_SIZE, 4, 
                              GLX_BLUE_SIZE, 4, 
                              GLX_DOUBLEBUFFER, 
                              None };
static int SingleBuffer[] = { GLX_RGBA, 
                              GLX_DEPTH_SIZE, 12, 
                              GLX_STENCIL_SIZE, 1,
                              GLX_RED_SIZE, 4, 
                              GLX_GREEN_SIZE, 4, 
                              GLX_BLUE_SIZE, 4, 
                              None };

static GLfloat rotation_matrix[16];
float ROTATION_SPEED = 0.5;
static GLfloat rot_x = 0.0, rot_y = 0.0;
GLfloat saved_x, saved_y;

Vector3 Viewer = Vector3(0.0, 0.0, -2);
Vector3 Center = Vector3(0.0, 0.0, 0.0);
Vector3 Up     = Vector3(0.0, 1.0, 0.0);

bool bRotateMolecule = false;
bool bAutoRotate     = false;
bool bPointSpritesSupported = false;
float scale = 0.01;
int iScreenWidth, iScreenHeight;
int iRotateMolecule; // Used for Rotation of Molecule

static bool bMapSpDispListCreated     = false;
static bool bCursorPosDispListCreated = false;
static bool bHelixNrDispListCreated   = false;
bool bMapSaiDispListCreated           = false;
bool bMapSearchStringsDispListCreated = false;

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
	iScreenWidth  = width; 
    iScreenHeight = height;

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
    cout<<"RNA3D: Initializing OpenGLEngine : "<<width<<" x "<<height<<endl;

    saved_x = saved_y = 2.0f;
    ComputeRotationXY(1,1);
    
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

    glDepthFunc(GL_LEQUAL);		     // The Type Of Depth Test To Do
    glEnable(GL_DEPTH_TEST);	   	 // Enables Depth Testing
    glClearDepth(1.0);
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

void ComputeRotationXY(int x, int y) {
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
    cRenderer->iBackBone       = root->awar(AWAR_3D_MOL_BACKBONE)->read_int();
    cRenderer->iColorise       = root->awar(AWAR_3D_MOL_COLORIZE)->read_int();
    cRenderer->fSkeletonSize   = root->awar(AWAR_3D_MOL_SIZE)->read_float();
    cRenderer->iDispPos        = root->awar(AWAR_3D_MOL_DISP_POS)->read_int();
    cStructure->iInterval      = root->awar(AWAR_3D_MOL_POS_INTERVAL)->read_int();
    cRenderer->iDispCursorPos  = root->awar(AWAR_3D_CURSOR_POSITION)->read_int();
    iRotateMolecule            = root->awar(AWAR_3D_MOL_ROTATE)->read_int();

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
    cRenderer->iDispTerInt    = root->awar(AWAR_3D_DISPLAY_TERTIARY_INTRACTIONS)->read_int();

    // Mapping Sequence Data Section
    cStructure->iMapSAI        = root->awar(AWAR_3D_MAP_SAI)->read_int();
    cStructure->iMapSearch     = root->awar(AWAR_3D_MAP_SEARCH_STRINGS)->read_int();
    cStructure->iMapEnable     = root->awar(AWAR_3D_MAP_ENABLE)->read_int();
    cRenderer->iMapSpecies     = root->awar(AWAR_3D_MAP_SPECIES)->read_int();
    cRenderer->iMapSpeciesBase = root->awar(AWAR_3D_MAP_SPECIES_DISP_BASE)->read_int();
    cRenderer->iMapSpeciesPos  = root->awar(AWAR_3D_MAP_SPECIES_DISP_POS)->read_int();
    cRenderer->iMapSpeciesDels = root->awar(AWAR_3D_MAP_SPECIES_DISP_DELETIONS)->read_int();
    cRenderer->iMapSpeciesMiss = root->awar(AWAR_3D_MAP_SPECIES_DISP_MISSING)->read_int();

    { // Validation of Helix Numbers entered by the User
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

    // Generation of DisplayLists for displaying Helix Numbers
    if (!bHelixNrDispListCreated && 
        (cRenderer->iHelixNrs || cRenderer->iHelixMidPoint)) 
        {
            cStructure->GenerateHelixNrDispList(cRenderer->iStartHelix, cRenderer->iEndHelix);
            bHelixNrDispListCreated = true;
        }

    // Validation of Base Position display
    if(cStructure->iInterval < 1) {
        cout<<"WARNING: Invalid POSITION Interval!! Setting it to Default Value (25)."<<endl;
        root->awar(AWAR_3D_MOL_POS_INTERVAL)->write_int(25);
    }

    if (cStructure->iMapEnable) 
        {
            if (cStructure->iMapSearch) {
                if (!bMapSearchStringsDispListCreated) {
                    cStructure->MapSearchStringsToEcoliTemplate(root);
                }
            }
            if (cStructure->iMapSAI) {
                if (!bMapSaiDispListCreated) {
                    cStructure->MapSaiToEcoliTemplate(root);
                }
            }
            if(!bMapSpDispListCreated) {
                cStructure->MapCurrentSpeciesToEcoliTemplate(root);
                bMapSpDispListCreated = true;
            }
        }
}
    

void DisplayPostionsIntervalChanged_CB(AW_root *awr) {
    MapDisplayParameters(awr);
    glDeleteLists(STRUCTURE_POS,2);
    cStructure->ComputeBasePositions();
    RefreshOpenGLDisplay();
}

void MapSelectedSpeciesChanged_CB(AW_root *awr) {

    // If Selected Species (in Primary Editor) changed and 
    // the MapSpecies display lists created then,
    //  1. Delete the display lists; 
    //  2. Recalculate the Display lists for current species;
    //  3. Map it to EColi Template;

    if (cStructure->iMapEnable && 
        cRenderer->iMapSpecies && 
        bMapSpDispListCreated) 
        {
            glDeleteLists(MAP_SPECIES_BASE_DIFFERENCE,9);
            cStructure->MapCurrentSpeciesToEcoliTemplate(awr);
        }

    // If selected species changed then regenerate the SearchStrings DisplayList
    MapSearchStringsToEcoliTemplateChanged_CB(awr);

    RefreshOpenGLDisplay();
}

void MapSaiToEcoliTemplateChanged_CB(AW_root *awr) {
    //if SAI changed in EDIT4 then diplay lists should be recalculated

    if (cStructure->iMapEnable  &&
        cStructure->iMapSAI     &&
        bMapSaiDispListCreated ) 
        {   
            bMapSaiDispListCreated = false;
            glDeleteLists(MAP_SAI_TO_STRUCTURE,1);
            cStructure->MapSaiToEcoliTemplate(awr);
        }

    RefreshOpenGLDisplay();
}

void MapSearchStringsToEcoliTemplateChanged_CB(AW_root *awr) {

    // If selected species changed then regenerate the 
    // SearchStrings DisplayList

    if (cStructure->iMapEnable  &&
        cStructure->iMapSearch  &&
        bMapSearchStringsDispListCreated) 
        {
            bMapSearchStringsDispListCreated = false;
            glDeleteLists(MAP_SEARCH_STRINGS_TO_STRUCTURE,2);
            cStructure->MapSearchStringsToEcoliTemplate(awr);
        }
}

void CursorPositionChanged_CB(AW_root *awr) {
    extern bool bEColiRefInitialised;
    if(bEColiRefInitialised) {
        long iCursorPos = awr->awar(AWAR_CURSOR_POSITION)->read_int();
        long EColiPos, dummy;
        cStructure->EColiRef->abs_2_rel(iCursorPos, EColiPos, dummy);

        if (!bCursorPosDispListCreated) {
            cStructure->GenerateCursorPositionDispList(EColiPos);
            bMapSpDispListCreated = true;
        }
        else {
            glDeleteLists(ECOLI_CURSOR_POSITION,1);
            cStructure->GenerateCursorPositionDispList(EColiPos);
        }
        RefreshOpenGLDisplay();
    }
}

void DisplayHelixNrsChanged_CB(AW_root *awr) {
    MapDisplayParameters(awr);
    
    int iStart = cRenderer->iStartHelix;
    int iEnd   = cRenderer->iEndHelix;
    if (!bHelixNrDispListCreated) {
        cStructure->GenerateHelixNrDispList(iStart, iEnd);
        bHelixNrDispListCreated = true;
    }
    else {
        glDeleteLists(HELIX_NUMBERS, 2);
        cStructure->GenerateHelixNrDispList(iStart, iEnd);
    }
    RefreshOpenGLDisplay();
}

void DrawStructure(){

    // Drawing Molecule Skeleton
    cRenderer->DisplayMolecule(cStructure);

    // Mapping Helices to The molecule
    cRenderer->DoHelixMapping();

    // Texture Mapping
    cRenderer->BeginTexturizer();
    cRenderer->TexturizeStructure(cTexture);
    cRenderer->EndTexturizer();
}

void RenderOpenGLScene(Widget w){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 

    // setting the BackGround Color of the OpenGL Scene
    SetOpenGLBackGroundColor();  

    glLoadIdentity();

    gluLookAt(Viewer.x, Viewer.y, Viewer.z,
              Center.x, Center.y, Center.z,
              Up.x,     Up.y,     Up.z);

    {// Displaying Molecule Name
        cRenderer->DisplayMoleculeName(iScreenWidth, iScreenHeight);
    }

    glScalef(scale,scale,scale);

    if(bRotateMolecule || bAutoRotate) {
        CalculateRotationMatrix();
    }

    glMultMatrixf(rotation_matrix); 

    glTranslatef(-cStructure->strCen->x, -cStructure->strCen->y, -cStructure->strCen->z);

    DrawStructure();

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
        vi = glXChooseVisual(dpy, DefaultScreen( dpy ), DoubleBuffer);            
			
		if (!vi) {
			fprintf(stderr, "try to get a single buffered visual\n");
			vi = glXChooseVisual(dpy, DefaultScreen(dpy), SingleBuffer);
			if (!vi)
				fprintf(stderr, "could not get visual\n");
		}
        else { 
            printf("RNA3D: Double Buffered Visual Supported !\n"); 
			glx_context = glXCreateContext(dpy, vi, NULL, GL_TRUE);
			if (!glXIsDirect(dpy, glx_context))
				fprintf(stderr, "direct rendering not supported\n");
			else
				printf("RNA3D: Direct rendering supported\n");
			
			GLwDrawingAreaMakeCurrent(w, glx_context);

			_glw = w;

			OpenGLEngineState = CREATED;
		}
	}
}

