#include "RNA3D_GlobalHeader.hxx"

#include <GL/aw_window_ogl.hxx>
#include <aw_window_Xm_interface.hxx>

#include "RNA3D_Global.hxx"
#include "RNA3D_Graphics.hxx"
#include "RNA3D_OpenGLEngine.hxx"
#include "RNA3D_Interface.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_StructureData.hxx"

#include <string>
#include <iostream>

#include <X11/keysym.h>

#include <aw_preset.hxx>
#include <aw_root.hxx>
#include <aw_msg.hxx>
#include <aw_awars.hxx>


using namespace std;

static AW_window_menu_modes_opengl *awm;
static XtAppContext                 appContext;
static XtWorkProcId                 workId   = 0;

// ========= SpinMolecule(XtPointer ...) : The Actual WorkProc Routine ============//
// => Sets the Rotation speed to 0.05 : default speed in Rotation Mode.
// => Calls RefreshOpenGLDisplay() : to calculate new rotate matrix
//    and then, redraws the entire scene.
// => Returns False : so that the work proc remains registered and
//    the animation will continue.
// ===============================================================================//

static Boolean SpinMolecule(XtPointer /* clientData */) {
    RNA3D->ROTATION_SPEED = 0.05;
    RefreshOpenGLDisplay();
    return false; // leave work proc active
}

// ========== RotateMoleculeStateChanged(void)==================================//
// => if bAutoRotate, removes the WorkProc routine and sets bAutoRotate to false.
// => if not, adds the WorkProc routine [SpinMolecule()] and sets bAutoRotate to true.
// bAutoRotate is used in recalculation of Rotation Matrix
// in RenderOpenGLScene() ==> defined in RNA3D_OpenGLEngine.cxx.
// ===============================================================================//

static void RotateMoleculeStateChanged(AW_root *awr) {
    bool rotate = awr->awar(AWAR_3D_MOL_ROTATE)->read_int();
    if (rotate != RNA3D->bAutoRotate) {
        if (RNA3D->bAutoRotate) {
            XtRemoveWorkProc(workId);
            RNA3D->bAutoRotate = false;
        }
        else {
            workId = XtAppAddWorkProc(appContext, SpinMolecule, NULL);
            RNA3D->bAutoRotate = true;
        }
    }
}

// ============ RotateMoleculeStateChanged_cb(AW_root *awr)===========================//
// Callback bound to Rotatation Awar (AWAR_3D_MOL_ROTATE)
// Is called when the
// 1. Rotation Awar is changed.
// 2. Also when the Spacebar is pressed in KeyBoardEventHandler().
// ===============================================================================//

static void RotateMoleculeStateChanged_cb(AW_root *awr) {
    MapDisplayParameters(awr);
    RotateMoleculeStateChanged(awr);
    RefreshOpenGLDisplay();
}

static void ResizeOpenGLWindow(Widget /* w */, XtPointer /* client_data */, XEvent *event, char* /* x */) {
    XConfigureEvent *evt;
    evt = (XConfigureEvent*) event;

    if (RNA3D->OpenGLEngineState == NOT_CREATED) {
        return;
    }

    ReshapeOpenGLWindow((GLint) evt->width, (GLint) evt->height);

    RefreshOpenGLDisplay();
}

static void KeyReleaseEventHandler(Widget /* w */, XtPointer /* client_data */, XEvent * /* event */, char* /* x */) {
    RefreshOpenGLDisplay();
}

static void KeyPressEventHandler(Widget /* w */, XtPointer /* client_data */, XEvent *event, char* /* x */) {
    char   buffer[1];
    KeySym keysym;

    // Converting keycode to keysym
    XLookupString((XKeyEvent *) event, buffer, 1, &keysym, NULL);

    switch (keysym) {
        case XK_space:
            RNA3D->iRotateMolecule = !RNA3D->iRotateMolecule;
            RNA3D->root->awar(AWAR_3D_MOL_ROTATE)->write_int(RNA3D->iRotateMolecule);
            break;
        case XK_Escape:
            RNA3D->bDisplayComments = !RNA3D->bDisplayComments;
            break;
        case XK_Tab:
            RNA3D->bDisplayMask = !RNA3D->bDisplayMask;
            break;
        case XK_Up:
            RNA3D->Center.y -= 0.1;
            break;
        case XK_Down:
            RNA3D->Center.y += 0.1;
            break;
        case XK_Left:
            RNA3D->Center.x -= 0.1;
            break;
        case XK_Right:
            RNA3D->Center.x += 0.1;
            break;
    }

    RefreshOpenGLDisplay();
}

static void ButtonReleaseEventHandler(Widget /* w */, XtPointer /* client_data */, XEvent *event, char* /* x */) {
    XButtonEvent *xr;
    xr = (XButtonEvent*) event;

    switch (xr->button)
    {
        case LEFT_BUTTON:
            RNA3D->bRotateMolecule = false;
            break;

        case MIDDLE_BUTTON:
            break;

        case RIGHT_BUTTON:
            break;
    }

    RefreshOpenGLDisplay();
}

static void ButtonPressEventHandler(Widget /* w */, XtPointer /* client_data */, XEvent *event, char* /* x */) {

    XButtonEvent *xp;
    xp = (XButtonEvent*) event;

    switch (xp->button)
    {
        case LEFT_BUTTON:
            RNA3D->bRotateMolecule = true;
            RNA3D->saved_x = xp->x;
            RNA3D->saved_y = xp->y;
            break;

        case MIDDLE_BUTTON:
            RNA3D->gl_Canvas->set_mode(AWT_MODE_NONE);
            break;

        case RIGHT_BUTTON:
            break;

        case WHEEL_UP:
            RNA3D->scale += ZOOM_FACTOR;
            break;

        case WHEEL_DOWN:
            RNA3D->scale -= ZOOM_FACTOR;
            break;
    }

    RefreshOpenGLDisplay();
}

static void MouseMoveEventHandler(Widget /* w */, XtPointer /* client_data */, XEvent *event, char* /* x */) {

    XMotionEvent *xp;
    xp = (XMotionEvent*) event;

    if (!RNA3D->bAutoRotate) {
        RNA3D->ROTATION_SPEED = 0.5;
    }

    if (RNA3D->bRotateMolecule) {
        // ComputeRotationXY : computes new rotation X and Y based on the mouse movement
        ComputeRotationXY(xp->x, xp->y);
    }

    RefreshOpenGLDisplay();
}

static void ExposeOpenGLWindow(Widget  w, XtPointer /* client_data */, XEvent *event, char* /* x */) {
    static bool ok = false;

    if (RNA3D->OpenGLEngineState == NOT_CREATED) {
        InitializeOpenGLWindow(w);

        XExposeEvent *evt;
        evt = (XExposeEvent*) event;

        try {
            InitializeOpenGLEngine((GLint) evt->height, (GLint) evt->height);
            ReshapeOpenGLWindow((GLint) evt->width, (GLint) evt->height);
            ok =  true;
        }
        catch (string& err) {
            // errors catched here should close the RNA3D window again (or prevent it from opening)
            aw_message(GBS_global_string("Error in RNA3D: %s", err.c_str()));
            awm->hide();
        }
    }

    if (ok) RefreshOpenGLDisplay();
}

void RefreshOpenGLDisplay() {

    if (RNA3D->OpenGLEngineState == CREATED) {
        RenderOpenGLScene(RNA3D->glw);
    }
}

static void RefreshCanvas(AW_root *awr) {
    MapDisplayParameters(awr);
    RefreshOpenGLDisplay();
}

static void SynchronizeColorsWithEditor(AW_window *aww) {
    // overwrites color settings with those from EDIT4

    AW_copy_GCs(aww->get_root(), "ARB_EDIT4", "RNA3D", false,
                "User1",   "User2",   "Probe",
                "Primerl", "Primerr", "Primerg",
                "Sigl",    "Sigr",    "Sigg",
                "RANGE_0", "RANGE_1", "RANGE_2",
                "RANGE_3", "RANGE_4", "RANGE_5",
                "RANGE_6", "RANGE_7", "RANGE_8",
                "RANGE_9",
                NULL);
}

static void Change3DMolecule_CB(AW_root *awr) {
    cout<<"Rendering new 3D molecule.... please wait..."<<endl;

    RNA3D->cStructure->LSU_molID = awr->awar(AWAR_3D_23S_RRNA_MOL)->read_int();

    RNA3D->cStructure->DeleteOldMoleculeData();        // Deleting the old molecule data
    RNA3D->cStructure->ReadCoOrdinateFile();           // Reading Structure information
    RNA3D->cStructure->GetSecondaryStructureInfo();    // Getting Secondary Structure Information
    RNA3D->cStructure->Combine2Dand3DstructureInfo();  // Combining Secondary Structure data with 3D Coordinates
    RNA3D->cStructure->GenerateDisplayLists();         // Generating Display Lists for Rendering

    // recalculate the mapping information
    awr->awar(AWAR_SPECIES_NAME)->touch();

    // recalculate helix numbering
    awr->awar(AWAR_3D_HELIX_FROM)->touch();

    // render new structure in OpenGL window
    RefreshCanvas(awr);
}

static void Change3DMolecule(AW_window *aww, int molID) {
    // changes the displayed 3D structure in the case of 23S rRNA
    aww->get_root()->awar(AWAR_3D_23S_RRNA_MOL)->write_int(molID);
}

static void DisplayMoleculeMask(AW_root * /* awr */) {
    RNA3D->bDisplayMask = !RNA3D->bDisplayMask;
    RefreshOpenGLDisplay();
}


// ---------------------------- Creating WINDOWS ------------------------------
static void AddCallBacks(AW_root *awr) {
    // adding callbacks to the awars to refresh the display if received any changes

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
    awr->awar(AWAR_3D_MAP_SPECIES_DISP_INSERTIONS)->add_callback(RefreshCanvas);
    awr->awar(AWAR_3D_MAP_SPECIES_DISP_INSERTIONS_INFO)->add_callback(RefreshCanvas);

    awr->awar(AWAR_3D_DISPLAY_MASK)->add_callback(DisplayMoleculeMask);
    awr->awar(AWAR_3D_23S_RRNA_MOL)->add_callback(Change3DMolecule_CB);
}

static void InitCallBacks(AW_root *awr) {
    // init callbacks
    RotateMoleculeStateChanged_cb(awr);
}

static void RefreshMappingDisplay(AW_window * /* aw */) {
    // Refreshes the SAI Display if and when ...
    // 1.Changes made to SAI related settings in EDIT4 and not updated automatically
    // 2.Colors related to SAI Display changed in RNA3D Application
    MapSaiToEcoliTemplateChanged_CB(RNA3D->root);

    // Refreshes the Search Strings Display if
    // Colors related to Search Strings changed in RNA3D Application
    // and not updated automatically
    MapSearchStringsToEcoliTemplateChanged_CB(RNA3D->root);

    // Resetting the Molecule Transformations
    // 1.Reset the Molecule view to Center of the viewer (default view).
    // 2.Zoom the Molecule to fit to window (default zoom).
    RNA3D->Center = Vector3(0.0, 0.0, 0.0);
    RNA3D->scale = 0.01;

    RefreshCanvas(RNA3D->root);
}

static void insertShapeSelection(AW_window *aws, const char *at, const char *awar_name) {
    aws->at(at);
    aws->create_toggle_field(awar_name, 1);
    aws->insert_toggle("#circle.xpm",    "C", 0);
    aws->insert_toggle("#diamond.xpm",   "D", 1);
    aws->insert_toggle("#polygon.xpm",   "P", 2);
    aws->insert_toggle("#star.xpm",      "S", 3);
    aws->insert_toggle("#rectangle.xpm", "R", 4);
    aws->update_toggle_field();
}

static AW_window *CreateDisplayBases_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;

    aws->init(aw_root, "DISPLAY_BASES", "RNA3D : Display BASES");
    aws->load_xfig("RNA3D_DisplayBases.fig");

    aws->callback(makeHelpCallback("rna3d_dispBases.hlp"));
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP", "#help.xpm");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE", "#closeText.xpm");

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
        aws->create_toggle_field(AWAR_3D_BASES_MODE, 0);
        aws->insert_toggle("CHARACTERS", "C", 0);
        aws->insert_toggle("SHAPES", "S", 1);
        aws->update_toggle_field();

        insertShapeSelection(aws, "spHelix", AWAR_3D_SHAPES_HELIX);
        insertShapeSelection(aws, "spUnpairedHelix", AWAR_3D_SHAPES_UNPAIRED_HELIX);
        insertShapeSelection(aws, "spNonHelix", AWAR_3D_SHAPES_NON_HELIX);
    }

    aws->show();

    return (AW_window *)aws;
}

static AW_window *CreateDisplayHelices_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;

    aws->init(aw_root, "DISPLAY_HELICES", "RNA3D : Display HELICES");
    aws->load_xfig("RNA3D_DisplayHelices.fig");

    aws->callback(makeHelpCallback("rna3d_dispHelices.hlp"));
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP", "#help.xpm");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE", "#closeText.xpm");

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
        {
            const char *helixRange = 0;

            arb_assert(RNA3D->cStructure);
            int rnaType = RNA3D->cStructure->FindTypeOfRNA();

            switch (rnaType) {
                case LSU_23S: helixRange = "[1-101]"; break;
                case SSU_16S: helixRange = "[1-50]";  break;
                case LSU_5S:  helixRange = "[1-5]";   break;
            }
            aws->at("rangeLabel");
            aws->create_autosize_button(0, helixRange);
        }

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

    aws->init(aw_root, "GENERAL_DISPLAY", "RNA3D : General Display ");
    aws->load_xfig("RNA3D_DisplayOptions.fig");

    aws->callback(makeHelpCallback("rna3d_dispMolecule.hlp"));
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP", "#help.xpm");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE", "#closeText.xpm");

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

    aws->init(aw_root, "MAP_SPECIES", "RNA3D : Map Sequence Data ");
    aws->load_xfig("RNA3D_SeqMapping.fig");

    aws->callback(makeHelpCallback("rna3d_mapSeqData.hlp"));
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP", "#help.xpm");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE", "#closeText.xpm");

    aws->callback(SynchronizeColorsWithEditor);
    aws->at("sync");
    aws->button_length(35);
    aws->create_button("SYNC", "SYNCHRONIZE COLORS WITH EDITOR");

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
        aws->create_button("REFRESH", "#refresh.xpm");

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
        aws->at("ins");
        aws->create_toggle(AWAR_3D_MAP_SPECIES_DISP_INSERTIONS);
        aws->at("bs");
        aws->create_toggle(AWAR_3D_MAP_SPECIES_DISP_INSERTIONS_INFO);
    }
    aws->show();
    return (AW_window *)aws;
}

static AW_window *CreateChangeMolecule_window(AW_root *aw_root) {
    static AW_window_simple *aws = 0;
    if (aws) return (AW_window *)aws;

    aws = new AW_window_simple;

    aws->init(aw_root, "CHANGE_MOLECULE", "RNA3D : Change 3D Molecule");
    aws->load_xfig("RNA3D_ChangeMolecule.fig");

    aws->callback(makeHelpCallback("rna3d_changeMolecule.hlp"));
    aws->at("help");
    aws->button_length(0);
    aws->create_button("HELP", "#help.xpm");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->button_length(0);
    aws->create_button("CLOSE", "#closeText.xpm");

    aws->callback(makeWindowCallback(Change3DMolecule, 1));
    aws->at("1pnu");
    aws->button_length(73);
    aws->create_button(0, "1PNU: 8.7 A^ Vila-Sanjurjo et al. Proc.Nat.Acad.Sci.(2003) 100, 8682.");

    aws->callback(makeWindowCallback(Change3DMolecule, 2));
    aws->at("1vor");
    aws->button_length(73);
    aws->create_button(0, "1VOR: 11.5 A^ Vila-Sanjurjo et al. Nat.Struct.Mol.Biol.(2004) 11, 1054.");

    aws->callback(makeWindowCallback(Change3DMolecule, 3));
    aws->at("1c2w");
    aws->button_length(73);
    aws->create_button(0, "1C2W: 7.5 A^ Mueller et al. J.Mol.Biol.(2000) 298, 35-59.", 0, "white");

    aws->show();
    return (AW_window *)aws;
}

static AW_window *CreateRNA3DGcWindow(AW_root *awr, AW_gc_manager gcman) {
    return AW_create_gc_window_named(awr, gcman, "RNA3D_PROPS_GC", "RNA3D colors and fonts");
}

AW_window *CreateRNA3DMainWindow(AW_root *awr, GBDATA *gb_main, ED4_plugin_host& host) {
    // Main Window - Canvas on which the actual painting is done
    GB_transaction ta(gb_main);

    awr->awar_int(AWAR_3D_SAI_SELECTED, 0, AW_ROOT_DEFAULT);

    RNA3D_init_global_data(host);

    awm = new AW_window_menu_modes_opengl;
    awm->init(awr, "RNA3D", "RNA3D: 3D Structure of Ribosomal RNA", WINDOW_WIDTH, WINDOW_HEIGHT);

    RNA3D_Graphics *rna3DGraphics = new RNA3D_Graphics(awr, gb_main);

    Structure3D::gb_main = gb_main;

    RNA3D->gl_Canvas = new AWT_canvas(Structure3D::gb_main, awm, awm->get_window_id(), rna3DGraphics, AWAR_SPECIES_NAME);

    RNA3D->gl_Canvas->recalc_size_and_refresh();
    RNA3D->gl_Canvas->set_mode(AWT_MODE_NONE);

    awm->create_menu("File", "F", AWM_ALL);
    {
        arb_assert(RNA3D->cStructure);
        int rnaType = RNA3D->cStructure->FindTypeOfRNA();
        if (rnaType == LSU_23S) {
            awm->insert_menu_topic("changeMolecule", "Change Molecule", "M", "rna3d_changeMolecule.hlp", AWM_ALL, CreateChangeMolecule_window);
        }
    }
    awm->insert_menu_topic("close", "Close", "C", "quit.hlp", AWM_ALL, AW_POPDOWN);

    {
        awm->at(1, 2);
        awm->auto_space(2, 0);
        awm->shadow_width(1);

        int cur_x, cur_y, start_x, first_line_y, second_line_y;
        awm->get_at_position(&start_x, &first_line_y);
        awm->button_length(0);
        awm->callback(AW_POPDOWN);
        awm->create_button("Quit", "#quit.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->callback(RefreshMappingDisplay);
        awm->button_length(0);
        awm->create_button("REFRESH", "#refresh.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->button_length(0);
        awm->create_toggle(AWAR_3D_DISPLAY_MASK, "#unmask.xpm", "#mask.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->callback(makeCreateWindowCallback(CreateRNA3DGcWindow, RNA3D->gl_Canvas->gc_manager));
        awm->button_length(0);
        awm->create_button("setColors", "#colors.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->help_text("rna3d_dispBases.hlp");
        awm->callback(CreateDisplayBases_window);
        awm->button_length(0);
        awm->create_button("displayBases", "#basesText.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->at(cur_x-10, cur_y);
        awm->create_toggle(AWAR_3D_DISPLAY_BASES, "#uncheck.xpm", "#check.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->help_text("rna3d_dispHelices.hlp");
        awm->callback(CreateDisplayHelices_window);
        awm->button_length(0);
        awm->create_button("displayHelix", "#helixText.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->at(cur_x-10, cur_y);
        awm->create_toggle(AWAR_3D_DISPLAY_HELIX, "#uncheck.xpm", "#check.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->help_text("rna3d_dispMolecule.hlp");
        awm->callback(CreateDisplayOptions_window);
        awm->button_length(0);
        awm->create_button("displayMolecule", "#molText.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->help_text("rna3d_mapSeqData.hlp");
        awm->callback(CreateMapSequenceData_window);
        awm->button_length(0);
        awm->create_button("mapSpecies", "#mapping.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->at(cur_x-10, cur_y);
        awm->create_toggle(AWAR_3D_MAP_ENABLE, "#uncheck.xpm", "#check.xpm");

        awm->get_at_position(&cur_x, &cur_y);
        awm->callback(makeHelpCallback("rna3d_general.hlp"));
        awm->button_length(0);
        awm->create_button("help", "#helpText.xpm");

        awm->at_newline();
        awm->get_at_position(&cur_x, &second_line_y);
        awm->create_autosize_button(0, " Spacebar = auto rotate mode on/off | Mouse Left Button + Move = Rotates Molecule | Mouse Wheel = Zoom in/out");
    }

    AddCallBacks(awr);
    RNA3D->root = awr;

    appContext = AW_get_XtAppContext(awr);

    RNA3D->OpenGLEngineState = NOT_CREATED;

    // Add event handlers:
    Widget middle_area_widget = AW_get_AreaWidget(RNA3D->gl_Canvas->aww, AW_MIDDLE_AREA);

    XtAddEventHandler(middle_area_widget, StructureNotifyMask, 0, ResizeOpenGLWindow,        (XtPointer)0);
    XtAddEventHandler(middle_area_widget, ExposureMask,        0, ExposeOpenGLWindow,        (XtPointer)0);
    XtAddEventHandler(middle_area_widget, KeyPressMask,        0, KeyPressEventHandler,      (XtPointer)0);
    XtAddEventHandler(middle_area_widget, KeyReleaseMask,      0, KeyReleaseEventHandler,    (XtPointer)0);
    XtAddEventHandler(middle_area_widget, ButtonPressMask,     0, ButtonPressEventHandler,   (XtPointer)0);
    XtAddEventHandler(middle_area_widget, ButtonReleaseMask,   0, ButtonReleaseEventHandler, (XtPointer)0);
    XtAddEventHandler(middle_area_widget, PointerMotionMask,   0, MouseMoveEventHandler,     (XtPointer)0);

#ifdef DEBUG
    cout<<"RNA3D: OpenGL Window created!"<<endl;
#endif

    InitCallBacks(awr);
    
    return awm;
}
