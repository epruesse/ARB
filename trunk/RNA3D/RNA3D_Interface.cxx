#include "RNA3D_GlobalHeader.hxx"

// The following includes are needed to use AW_window_Motif 

#include "../WINDOW/aw_awar.hxx"
#include "../WINDOW/aw_Xm.hxx"
#include "../WINDOW/aw_click.hxx"
#include "../WINDOW/aw_size.hxx"
#include "../WINDOW/aw_print.hxx"
#include "../WINDOW/aw_window_Xm.hxx"

#include "RNA3D_Global.hxx"
#include "RNA3D_Graphics.hxx"
#include "RNA3D_OpenGLEngine.hxx"
#include "RNA3D_Interface.hxx"

using namespace std;

AWT_canvas *gl_Canvas;
AW_window_menu_modes_opengl *awm;
AW_root *gAwRoot;

extern GBDATA *gb_main;
Widget OpenGLParentWidget;

XtAppContext appContext;
XtWorkProcId workId   = 0;
GLboolean    Spinning = GL_FALSE;

//========= SpinMolecule(XtPointer ...) : The Actual WorkProc Routine ============//
// => Sets the Rotation speed to 0.05 : default speed in Rotation Mode.
// => Calls RefreshOpenGLDisplay() : to calculate new rotate matrix
//    and then, redraws the entire scene.
// => Returns False : so that the work proc remains registered and  
//    the animation will continue.
//===============================================================================//

Boolean SpinMolecule(XtPointer clientData) {
    AWUSE(clientData);
    extern float ROTATION_SPEED;
    ROTATION_SPEED = 0.05;
    RefreshOpenGLDisplay();
    return false; /* leave work proc active */
}

//========== RotateMoleculeStateChanged(void)==================================//
// => if Spinning, removes the WorkProc routine and sets Spinning & bAutoRotate to false.
// => if not, adds the WorkProc routine [SpinMolecule()] and sets Spinning & bAutoRotate to true.
// bAutoRotate is used in recalculation of Rotation Matrix 
// in RenderOpenGLScene() ==> defined in RNA3D_OpenGLEngine.cxx.
//===============================================================================//

static void RotateMoleculeStateChanged(void) {

    extern bool bAutoRotate;

    if(Spinning) {
        XtRemoveWorkProc(workId);
        Spinning = GL_FALSE; 
        bAutoRotate = false;
    } else {
        workId = XtAppAddWorkProc(appContext, SpinMolecule, NULL);
        Spinning = GL_TRUE; 
        bAutoRotate = true;
    }
}

//============ RotateMoleculeStateChanged_cb(AW_root *awr)===========================//
// Callback bound to Rotatation Awar (AWAR_3D_MOL_ROTATE)
// Is called when the 
// 1. Rotation Awar is changed.
// 2. Also when the Spacebar is pressed in KeyBoardEventHandler().
//===============================================================================//

static void RotateMoleculeStateChanged_cb(AW_root *awr) {
    MapDisplayParameters(awr);
    RotateMoleculeStateChanged();
    RefreshOpenGLDisplay();
}

void ResizeOpenGLWindow( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	XConfigureEvent *evt;
	evt = (XConfigureEvent*) event;
	
	extern int OpenGLEngineState;  // it is globally defined in GEN_OpenGL.cxx
	
	if ( OpenGLEngineState == NOT_CREATED ) {
		return;
	}
	
	ReshapeOpenGLWindow( (GLint) evt->width, (GLint) evt->height );

    RefreshOpenGLDisplay();
}

void KeyBoardEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	XKeyEvent *evt;
    evt = (XKeyEvent*) event;

    char   buffer[1];
    KeySym keysym;
    int    count;

    // Converting keycode to keysym
    count = XLookupString((XKeyEvent *) event, buffer, 1, &keysym, NULL);

    extern float fClipNear;

    switch(keysym) {
    case XK_space:
        extern int iRotateMolecule;
        iRotateMolecule = !iRotateMolecule;
        gAwRoot->awar(AWAR_3D_MOL_ROTATE)->write_int(iRotateMolecule);
        break;
    case XK_Escape:
        exit(0); 
        break;
    case XK_Up:
        fClipNear += 0.10f;cout<<fClipNear<<endl;
        break;
    case XK_Down:
        fClipNear -= 0.10f;cout<<fClipNear<<endl;
        break;
    }

    RefreshOpenGLDisplay();
}

void ButtonReleaseEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	XButtonEvent *xr;
	xr = (XButtonEvent*) event;

    extern bool bRotateMolecule;

    switch(xr->button) {

    case LEFT_BUTTON:
        bRotateMolecule = false;   
        break;

    case MIDDLE_BUTTON:
        break;

    case RIGHT_BUTTON:
        break;
    }

    RefreshOpenGLDisplay();
}

void ButtonPressEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x ) {

	XButtonEvent *xp;
	xp = (XButtonEvent*) event;
	extern GLfloat saved_x, saved_y;
    extern bool bRotateMolecule;
    extern float scale;

    switch(xp->button) {

    case LEFT_BUTTON:
        bRotateMolecule = true;   
        saved_x = xp->x;
        saved_y = xp->y;
        break;

    case MIDDLE_BUTTON:
        gl_Canvas->set_mode(AWT_MODE_NONE);
        break;

    case RIGHT_BUTTON:
        break;

    case WHEEL_UP:
        scale += ZOOM_FACTOR;
        break;

    case WHEEL_DOWN:
        scale -= ZOOM_FACTOR;
        break;
	}
    
    RefreshOpenGLDisplay();
}

void MouseMoveEventHandler( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	
	XMotionEvent *xp;
	xp = (XMotionEvent*) event;
	
    extern bool bAutoRotate;
    if (!bAutoRotate) {
        extern float ROTATION_SPEED;
        ROTATION_SPEED = 0.5;
    }

    extern bool bRotateMolecule;
    if(bRotateMolecule) {
        // ComputeRotationXY : computes new rotation X and Y based on the mouse movement
        ComputeRotationXY(xp->x, xp->y);
    }

    RefreshOpenGLDisplay();
}

void ExposeOpenGLWindow( Widget w, XtPointer client_data, XEvent *event, char* x ) {
	extern int  OpenGLEngineState;
    static bool ok = false;

	if ( OpenGLEngineState == NOT_CREATED ) {
		extern GBDATA* OpenGL_gb_main;
		OpenGL_gb_main = gb_main;
		
		InitializeOpenGLWindow( w );

		XExposeEvent *evt;
		evt = (XExposeEvent*) event;

        try {
            InitializeOpenGLEngine( (GLint) evt->height, (GLint) evt->height );
            ReshapeOpenGLWindow( (GLint) evt->width, (GLint) evt->height );
            ok =  true;
        }
        catch (string& err) {
            //errors catched here should close the RNA3D window again (or prevent it from opening)
            aw_message(GBS_global_string("Error in RNA3D: %s", err.c_str()));
            awm->hide();
        }
    }

    if (ok) RefreshOpenGLDisplay();
}

void RefreshOpenGLDisplay() {
	extern int OpenGLEngineState;
	if ( OpenGLEngineState == CREATED ) {
		extern Widget _glw;
        RenderOpenGLScene(_glw);
    }
}

void SetOpenGLBackGroundColor() {
	extern Widget _glw;
	unsigned long bgColor;
	XtVaGetValues( _glw, XmNbackground, &bgColor, NULL );

    extern AWT_canvas *gl_Canvas;
    Widget w = gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area;

    XColor xcolor;
    xcolor.pixel = bgColor;
    Colormap colormap = DefaultColormapOfScreen( XtScreen( w ) );
    XQueryColor( XtDisplay( w ), colormap, &xcolor );

    float r, g, b; r = g = b = 0.0;
    r = (float) xcolor.red / 65535.0;
    g = (float) xcolor.green / 65535.0;
    b = (float) xcolor.blue / 65535.0;

    // set OpenGL Backgroud Color to the widget's backgroud     
    glClearColor(r, g, b, 0);
}

static void RefreshCanvas(AW_root *awr) {
    MapDisplayParameters(awr);
    RefreshOpenGLDisplay();
}

/*---------------------------- Creating WINDOWS ------------------------------ */
static void AddCallBacks(AW_root *awr) {
    // adding callbacks to the awars to refresh the display if recieved any changes

    // General Molecule Display  Section
    awr->awar(AWAR_3D_MOL_BACKBONE)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_MOL_COLORIZE)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_MOL_SIZE)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_MOL_DISP_POS)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_MOL_ROTATE)->add_callback(RotateMoleculeStateChanged_cb);
    awr->awar(AWAR_3D_MOL_POS_INTERVAL)->add_callback(DisplayPostionsIntervalChanged_CB);
    awr->awar(AWAR_3D_CURSOR_POSITION)->add_callback(RefreshCanvas);
    awr->awar(AWAR_CURSOR_POSITION)->add_callback(CursorPositionChanged_CB);

    // Display Base Section
    awr->awar(AWAR_3D_DISPLAY_BASES)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_DISPLAY_SIZE)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_BASES_MODE)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_BASES_HELIX)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_BASES_UNPAIRED_HELIX)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_BASES_NON_HELIX)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_SHAPES_HELIX)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_SHAPES_UNPAIRED_HELIX)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_SHAPES_NON_HELIX)->add_callback(RefreshCanvas);

    // Display Helix Section
    awr->awar(AWAR_3D_DISPLAY_HELIX)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_HELIX_BACKBONE)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_HELIX_MIDPOINT)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_HELIX_FROM)->add_callback(DisplayHelixNrsChanged_CB);
    awr->awar(AWAR_3D_HELIX_TO)->add_callback(DisplayHelixNrsChanged_CB);
    awr->awar(AWAR_3D_HELIX_NUMBER)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_HELIX_SIZE)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_DISPLAY_TERTIARY_INTRACTIONS)->add_callback(RefreshCanvas);

    // Mapping Sequence Data Section
    awr->awar(AWAR_SPECIES_NAME)->add_callback(MapSelectedSpeciesChanged_CB);
    awr->awar(AWAR_3D_MAP_ENABLE)->add_callback(RefreshCanvas);

    awr->awar(AWAR_3D_MAP_SAI)->add_callback(RefreshCanvas);
    awr->awar(AWAR_SAI_GLOBAL)->add_callback(MapSaiToEcoliTemplateChanged_CB);
    awr->awar(AWAR_3D_MAP_SEARCH_STRINGS)->add_callback(RefreshCanvas);

    awr->awar(AWAR_3D_MAP_SPECIES)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_MAP_SPECIES_DISP_BASE)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_MAP_SPECIES_DISP_POS)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_MAP_SPECIES_DISP_DELETIONS)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_MAP_SPECIES_DISP_MISSING)->add_callback(RefreshCanvas);
}

static void RefreshMappingDisplay(AW_window *aw) {
    // Refreshes the SAI Display if and when ...
    // 1.Changes made to SAI related settings in EDIT4 and not updated automatically 
    // 2.Colors related to SAI Display changed in RNA3D Application

    MapSaiToEcoliTemplateChanged_CB(gAwRoot);

    // Refreshes the Search Strings Display if 
    // Colors related to Search Strings changed in RNA3D Application 
    // and not updated automatically
    MapSearchStringsToEcoliTemplateChanged_CB(gAwRoot);
}

static AW_window *CreateDisplayBases_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;

    aws->init( aw_root, "DISPLAY_BASES", "RNA3D : Display BASES");
    aws->load_xfig("RNA3D_DisplayBases.fig");

    aws->callback( AW_POPUP_HELP,(AW_CL)"rna3d_displayOptions.hlp");
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP","#help.xpm");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE","#closeText.xpm");

    {  // Display Bases Section
        aws->at("dispBases");
        aws->create_toggle(AWAR_3D_DISPLAY_BASES);

        aws->at("helix");
        aws->create_toggle(AWAR_3D_BASES_HELIX);
        aws->at("unpairHelix");
        aws->create_toggle(AWAR_3D_BASES_UNPAIRED_HELIX);
        aws->at("nonHelix");
        aws->create_toggle(AWAR_3D_BASES_NON_HELIX);

        aws->at("shapesSize");
        aws->create_input_field(AWAR_3D_DISPLAY_SIZE, 5);

        aws->at("basesMode");
        aws->create_toggle_field(AWAR_3D_BASES_MODE,0);
        aws->insert_toggle("CHARACTERS", "C", 0);
        aws->insert_toggle("SHAPES", "S", 1);
        aws->update_toggle_field();

        aws->at("spHelix");
        aws->create_toggle_field(AWAR_3D_SHAPES_HELIX,1);
        aws->insert_toggle("#circle.bitmap",    "C", 0);
        aws->insert_toggle("#diamond.bitmap",   "D", 1);
        aws->insert_toggle("#polygon.bitmap",   "P", 2);
        aws->insert_toggle("#star.bitmap",      "S", 3);
        aws->insert_toggle("#rectangle.bitmap", "R", 4);
        aws->update_toggle_field();

        aws->at("spUnpairedHelix");
        aws->create_toggle_field(AWAR_3D_SHAPES_UNPAIRED_HELIX,1);
        aws->insert_toggle("#circle.bitmap",    "C", 0);
        aws->insert_toggle("#diamond.bitmap",   "D", 1);
        aws->insert_toggle("#polygon.bitmap",   "P", 2);
        aws->insert_toggle("#star.bitmap",      "S", 3);
        aws->insert_toggle("#rectangle.bitmap", "R", 4);
        aws->update_toggle_field();

        aws->at("spNonHelix");
        aws->create_toggle_field(AWAR_3D_SHAPES_NON_HELIX,1);
        aws->insert_toggle("#circle.bitmap",    "C", 0);
        aws->insert_toggle("#diamond.bitmap",   "D", 1);
        aws->insert_toggle("#polygon.bitmap",   "P", 2);
        aws->insert_toggle("#star.bitmap",      "S", 3);
        aws->insert_toggle("#rectangle.bitmap", "R", 4);
        aws->update_toggle_field();
   }

    aws->show();

    return (AW_window *)aws;
}

static AW_window *CreateDisplayHelices_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;

    aws->init( aw_root, "DISPLAY_HELICES", "RNA3D : Display HELICES");
    aws->load_xfig("RNA3D_DisplayHelices.fig");

    aws->callback( AW_POPUP_HELP,(AW_CL)"rna3d_displayOptions.hlp");
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP","#help.xpm");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE","#closeText.xpm");

    {  // Display Helices Section
        aws->at("dispHelix");
        aws->create_toggle(AWAR_3D_DISPLAY_HELIX);

        aws->at("backbone");
        aws->create_toggle(AWAR_3D_HELIX_BACKBONE);
        aws->at("midHelix");
        aws->create_toggle(AWAR_3D_HELIX_MIDPOINT);
        aws->at("helixNr");
        aws->create_toggle(AWAR_3D_HELIX_NUMBER);
        aws->at("from");
        aws->create_input_field(AWAR_3D_HELIX_FROM, 5);
        aws->at("to");
        aws->create_input_field(AWAR_3D_HELIX_TO, 5);
        aws->at("helixSize");
        aws->create_input_field(AWAR_3D_HELIX_SIZE, 5);

        aws->at("dispTI");
        aws->create_toggle(AWAR_3D_DISPLAY_TERTIARY_INTRACTIONS);
   }
    aws->show();
    return (AW_window *)aws;
}

static AW_window *CreateDisplayOptions_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;

    aws->init( aw_root, "GENERAL_DISPLAY", "RNA3D : General Display ");
    aws->load_xfig("RNA3D_DisplayOptions.fig");

    aws->callback( AW_POPUP_HELP,(AW_CL)"rna3d_displayOptions.hlp");
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP","#help.xpm");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE","#closeText.xpm");

    {  // Display Molecule Section
        aws->at("backbone");
        aws->create_toggle(AWAR_3D_MOL_BACKBONE);
        aws->at("color");
        aws->create_toggle(AWAR_3D_MOL_COLORIZE);
        aws->at("dispPos");
        aws->create_toggle(AWAR_3D_MOL_DISP_POS);
        aws->at("rot");
        aws->create_toggle(AWAR_3D_MOL_ROTATE);
        aws->at("pos");
        aws->create_input_field(AWAR_3D_MOL_POS_INTERVAL, 2);
        aws->at("molSize");
        aws->create_input_field(AWAR_3D_MOL_SIZE, 5);
        aws->at("cp");
        aws->create_toggle(AWAR_3D_CURSOR_POSITION);
   }
    aws->show();
    return (AW_window *)aws;
}


static AW_window *CreateMapSequenceData_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;

    aws->init( aw_root, "MAP_SPECIES", "RNA3D : Map Sequence Data ");
    aws->load_xfig("RNA3D_SeqMapping.fig");

    aws->callback( AW_POPUP_HELP,(AW_CL)"rna3d_displayOptions.hlp");
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP","#help.xpm");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE","#closeText.xpm");

    {  // Display Map Current Species Section
        aws->at("en");
        aws->create_toggle(AWAR_3D_MAP_ENABLE);
        
        aws->at("src");
        aws->create_toggle(AWAR_3D_MAP_SEARCH_STRINGS);

        aws->at("sai");
        aws->create_toggle(AWAR_3D_MAP_SAI);

        aws->callback(RefreshMappingDisplay);
        aws->at("ref");
        aws->button_length(0);
        aws->create_button("REFRESH","#refresh.xpm");

        aws->at("sp");
        aws->create_toggle(AWAR_3D_MAP_SPECIES);
        aws->at("base");
        aws->create_toggle(AWAR_3D_MAP_SPECIES_DISP_BASE);
        aws->at("pos");
        aws->create_toggle(AWAR_3D_MAP_SPECIES_DISP_POS);
        aws->at("del");
        aws->create_toggle(AWAR_3D_MAP_SPECIES_DISP_DELETIONS);
        aws->at("mis");
        aws->create_toggle(AWAR_3D_MAP_SPECIES_DISP_MISSING);
   }
    aws->show();
    return (AW_window *)aws;
}

AW_window *CreateRNA3DMainWindow(AW_root *awr){
    // Main Window - Canvas on which the actual painting is done
    GB_transaction dummy(gb_main);

    awr->awar_int(AWAR_3D_SAI_SELECTED, 0, AW_ROOT_DEFAULT); 

    extern AW_window_menu_modes_opengl *awm;
    awm = new AW_window_menu_modes_opengl();
    awm->init(awr,"RNA3D", "RNA3D: 3D Structure of Ribosomal RNA", WINDOW_WIDTH, WINDOW_HEIGHT);

    AW_gc_manager aw_gc_manager;
    RNA3D_Graphics *rna3DGraphics = new RNA3D_Graphics(awr,gb_main);

    extern AWT_canvas *gl_Canvas;
    gl_Canvas = new AWT_canvas(gb_main,awm, rna3DGraphics, aw_gc_manager,AWAR_SPECIES_NAME);

    gl_Canvas->recalc_size();
    gl_Canvas->refresh();
    gl_Canvas->set_mode(AWT_MODE_NONE); 

    awm->create_menu( 0, "File", "F", 0,  AWM_ALL );
    awm->insert_menu_topic( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);

    {
        awm->at(11,2);
        awm->auto_space(2,0);
        awm->shadow_width(1);

        int cur_x, cur_y, start_x, first_line_y, second_line_y, third_line_y;
        awm->get_at_position( &start_x,&first_line_y);
        awm->button_length(0);
        awm->callback( (AW_CB0)AW_POPDOWN );
        awm->create_button("Quit", "#quitText.xpm");
    
        awm->get_at_position( &cur_x,&cur_y );
        awm->callback(AW_POPUP,(AW_CL)AW_create_gc_window,(AW_CL)aw_gc_manager);
        awm->button_length(0);
        awm->create_button("setColors", "#colors.xpm");

        awm->get_at_position( &cur_x,&cur_y );
        awm->callback(AW_POPUP,(AW_CL)CreateDisplayBases_window,(AW_CL)0);
        awm->button_length(0);
        awm->create_button("displayBases", "#basesText.xpm");

        awm->get_at_position( &cur_x,&cur_y );
        awm->at(cur_x-10, cur_y);
        awm->create_toggle(AWAR_3D_DISPLAY_BASES, "#uncheck.xpm", "#check.xpm");

        awm->get_at_position( &cur_x,&cur_y );
        awm->callback(AW_POPUP,(AW_CL)CreateDisplayHelices_window,(AW_CL)0);
        awm->button_length(0);
        awm->create_button("displayHelix", "#helixText.xpm");

        awm->get_at_position( &cur_x,&cur_y );
        awm->at(cur_x-10, cur_y);
        awm->create_toggle(AWAR_3D_DISPLAY_HELIX, "#uncheck.xpm", "#check.xpm");

        awm->get_at_position( &cur_x,&cur_y );
        awm->callback(AW_POPUP,(AW_CL)CreateDisplayOptions_window,(AW_CL)0);
        awm->button_length(0);
        awm->create_button("displayMolecule", "#molText.xpm");

        awm->get_at_position( &cur_x,&cur_y );
        awm->callback(AW_POPUP,(AW_CL)CreateMapSequenceData_window,(AW_CL)0);
        awm->button_length(12);
        awm->create_button("mapSpecies", "#mapping.xpm");

        awm->get_at_position( &cur_x,&cur_y );
        awm->at(cur_x-10, cur_y);
        awm->create_toggle(AWAR_3D_MAP_ENABLE, "#uncheck.xpm", "#check.xpm");

        awm->get_at_position( &cur_x,&cur_y );
        awm->callback(RefreshMappingDisplay);
        awm->button_length(0);
        awm->create_button("REFRESH","#refresh.xpm");

        awm->get_at_position( &cur_x,&cur_y );
        awm->callback( AW_POPUP_HELP,(AW_CL)"rna3d_displayOptions.hlp");
        awm->button_length(0);
        awm->create_button("help", "#helpText.xpm");
    }

    AddCallBacks(awr);
    gAwRoot = awr;

    appContext = awr->prvt->context;

    extern int OpenGLEngineState; // defined globally in OpenGLEngine.cxx
    OpenGLEngineState = NOT_CREATED;
		
    /** Add event handlers */
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       StructureNotifyMask , 0, (XtEventHandler) ResizeOpenGLWindow, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       ExposureMask, 0, (XtEventHandler) ExposeOpenGLWindow, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       KeyPressMask, 0, (XtEventHandler) KeyBoardEventHandler, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       ButtonPressMask, 0, (XtEventHandler) ButtonPressEventHandler, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       ButtonReleaseMask, 0, (XtEventHandler) ButtonReleaseEventHandler, (XtPointer) 0 );
		
    XtAddEventHandler( gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area,
                       PointerMotionMask, 0, (XtEventHandler) MouseMoveEventHandler, (XtPointer) 0 );

    extern Widget OpenGLParentWidget;
    OpenGLParentWidget = awm->p_w->areas[ AW_MIDDLE_AREA ]->area;

#ifdef DEBUG
    cout<<"Openglwindow created!!"<<endl;
#endif

    return awm;
}
