// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$


#include "image_dialog.hxx"
#include "selection_dialog.hxx"
#include "config_dialog.hxx"
#include "help_dialog.hxx"
#include "math.h"
#include <X11/cursorfont.h>

#define LEFT_MOUSE_BUTTON   1
#define MIDDLE_MOUSE_BUTTON 2
#define RIGHT_MOUSE_BUTTON  3

#define PROTEIN_MARK   1
#define PROTEIN_UNMARK 2
#define PROTEIN_INVERT 3

// MAXIMUM DISTANCE BETWEEN SPOT CENTER AND
// MOUSE CLICK DURING SELECTION
#define MAX_SELECTION_DISTANCE 20

// LOWER LIMIT FOR A GENERATED IMAGE
#define MIN_IMAGE_SIZE 50


// CALLBACK WRAPPER FUNCTIONS (STATIC)
static void staticARBdataButtonCallback(Widget, XtPointer, XtPointer);
static void staticTIFFnameButtonCallback(Widget, XtPointer, XtPointer);
static void staticImageSpeciesCallback(Widget, XtPointer, XtPointer);
static void staticImageExperimentCallback(Widget, XtPointer, XtPointer);
static void staticImageProteomeCallback(Widget, XtPointer, XtPointer);
static void staticImageFileDialogCloseCallback(Widget, XtPointer, XtPointer);
static void staticImageFileDialogCallback(Widget, XtPointer, XtPointer);
static void staticImageRedrawCallback(Widget, XtPointer, XtPointer);
static void staticCrosshairButtonCallback(Widget, XtPointer, XtPointer);
static void staticTextButtonCallback(Widget, XtPointer, XtPointer);
static void staticTextOnlyButtonCallback(Widget, XtPointer, XtPointer);
static void staticCircleButtonCallback(Widget, XtPointer, XtPointer);
static void staticMarkedOnlyButtonCallback(Widget, XtPointer, XtPointer);
static void staticMarkAllButtonCallback(Widget, XtPointer, XtPointer);
static void staticMarkInvertButtonCallback(Widget, XtPointer, XtPointer);
static void staticMarkNoneButtonCallback(Widget, XtPointer, XtPointer);
static void staticImageEventCallback(Widget, XtPointer, XtPointer);
static void staticLockToggleButtonCallback(Widget, XtPointer, XtPointer);
static void static_ARB_protein_callback(GBDATA *, imageDialog *id, GB_CB_TYPE);
static void static_ARB_gene_callback(GBDATA *, imageDialog *id, GB_CB_TYPE);
static void static_PGT_config_callback(GBDATA *, imageDialog *id, GB_CB_TYPE);
static void staticUpdateGeneButtonCallback(Widget, XtPointer, XtPointer);
static void staticSpots2GenesButtonCallback(Widget, XtPointer, XtPointer);
static void staticGenes2SpotsButtonCallback(Widget, XtPointer, XtPointer);
static void staticHelpDialogCallback(Widget, XtPointer, XtPointer);
static void staticMarkWithInfoButtonCallback(Widget, XtPointer, XtPointer);

// #warning hi kai, obige prototypen waren im header, werden aber nur lokal verwendet
// --> Stimmt, danke für den Hinweis. :-)

// prinzipielle Anmerkung: Du definierst erst den Prototyp, dann folgt der Aufruf und am Schluss
// kommt die Funktiondefinition.
// Besser ist folgende Definitionsreihenfolge : Erst die Funktion dann der Aufruf. Prototyp ist dadurch ueberfluessig!
// Beispiel :

// static int complicated();

// static int simple() {
//     return 2;
// }

// void aufrufe() {
//     simple();
//     complicated();
// }

// static int complicated() {
//     return 7;
// }



/****************************************************************************
 *  IMAGE DIALOG - CONSTRUCTOR
 ****************************************************************************/
imageDialog::imageDialog(Widget p, MDialog *d)
    : MDialog(p, d)
{
    // PREDEFINE VARIABLES
    m_hasFileDialog= false;
    m_hasTIFFdata= false;
    m_hasImagedata= false;
    m_hasARBdata= false;
    m_lockVisualization= false;
    m_updateGene= false;
    m_filename= NULL;
    m_ximage= NULL;
    m_image= NULL;
    m_width= 0;
    m_height= 0;
    m_numSpots= 0;
    m_numMarkedSpots= 0;
    m_numSelectedSpots= 0;
    m_selectedSpotName= NULL;

    // CHECK THE PGT AWARS (SET DEFAULT IF NEEDED)
    checkCreateAWARS();

    m_species= get_species_AWAR();
    m_experiment= get_experiment_AWAR();
    m_proteome= get_proteom_AWAR();

    // DEBUG -- PRESET -- DEBUG -- PRESET
    m_x_container= "x_coordinate";
    m_y_container= "y_coordinate";
    m_id_container= "name";
    m_vol_container= "volume";
    m_area_container= "area";
    m_avg_container= "avg";
    // DEBUG -- PRESET -- DEBUG -- PRESET

    m_crosshairFlag= false;
    m_circleFlag= false;
    m_labelFlag= false;
    m_linkedOnlyFlag= false;
    m_textOnlyFlag= false;
    m_markedOnlyFlag= false;

    // CREATE WINDOW SHELL
    createShell("");

    // CREATE MAIN WINDOW WIDGETS
    createWindow();

    // SET WINDOW WIDTH
    XtVaSetValues(m_shell,
        XmNwidth, 800,
        XmNheight, 600,
        NULL);

    // REALIZE SHELL & WIDGETS
    realizeShell();

    // SET WINDOW LABEL
    setWindowName("PGT - Image View");

    // UPDATE ARB SELECTION ENTRIES
    updateARBText();

    // CROSSHAIR CURSOR FOR DRAWING AREA
    Cursor crosshair_cursor;
    crosshair_cursor= XCreateFontCursor(XtDisplay(m_drawingArea), XC_crosshair);
    XDefineCursor(XtDisplay(m_drawingArea), XtWindow(m_drawingArea), crosshair_cursor);

    // GET GLOBAL SETTINGS (SAVED IN THE ARB DATABASE)
    getSettings();

    // ADD ARB AWAR CALLBACKS

    add_imageDialog_callback(AWAR_PROTEIN_NAME,   static_ARB_protein_callback, this);
    add_imageDialog_callback(AWAR_GENE_NAME,      static_ARB_gene_callback,    this);
    add_imageDialog_callback(AWAR_CONFIG_CHANGED, static_PGT_config_callback,  this);

    // add_protein_callback(static_ARB_protein_callback, this);
    // add_gene_callback(static_ARB_gene_callback, this);
    // add_config_callback(static_PGT_config_callback, this);
}


/****************************************************************************
*  IMAGE DIALOG - CREATE WINDOW
****************************************************************************/
void imageDialog::createWindow()
{
    // CREATE TOP LEVEL WIDGET
    m_top= XtVaCreateManagedWidget("top",
        xmFormWidgetClass, m_shell,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        NULL);

    // CREATE TOP TOOLBAR WIDGET
    m_topToolbar= XtVaCreateManagedWidget("topToolbar",
        xmFormWidgetClass, m_top,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE HORIZONTAL SEPARATOR
    Widget separator= XtVaCreateManagedWidget("separator",
        xmSeparatorWidgetClass, m_top,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_topToolbar,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNorientation, XmHORIZONTAL,
        NULL);

    // CREATE LEFT TOOLBAR WIDGET
    m_leftToolbar= XtVaCreateManagedWidget("topToolbar",
        xmRowColumnWidgetClass, m_top,
        XmNorientation, XmVERTICAL,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, separator,
        XmNleftAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);

    // CREATE A STATUS LABEL AREA
    m_statusLabel= XtVaCreateManagedWidget("statusLabel",
        // xmTextWidgetClass, m_top,
        xmLabelWidgetClass, m_top,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, m_leftToolbar,
        XmNrightAttachment, XmATTACH_FORM,
        XmNheight, 20,
        XmNborderWidth, 1,
        XtVaTypedArg, XmNborderColor, XmRString, "gray", strlen("gray"),
        XmNlabelString, PGT_XmStringCreateLocalized("Ok..."),
        XmNalignment, XmALIGNMENT_BEGINNING,
        NULL);

    // CREATE A SCROLL WIDGET - CONTAINS THE DRAWING AREA
    Widget scroll= XtVaCreateManagedWidget("scroll",
        xmScrolledWindowWidgetClass, m_top,
        XmNscrollingPolicy, XmAUTOMATIC,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_topToolbar,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, m_leftToolbar,
        XmNrightAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_WIDGET,
        XmNbottomWidget, m_statusLabel,
        NULL);

    // DRAWING AREA WIDGET - IMAGE IS DISPLAYED IN HERE
    m_drawingArea= XtVaCreateManagedWidget("area",
        xmDrawingAreaWidgetClass, scroll,
        XmNwidth, 60,  // DEFAULT
        XmNheight, 60, // DEFAULT
        XtVaTypedArg, XmNbackground,
        XmRString, "white", 6,
        NULL);

    // CALLBACK FOR DRAWING AREA: IF XIMAGE NEEDS REDRAW
    XtAddCallback(m_drawingArea, XmNexposeCallback, staticImageRedrawCallback, this);

    // CALLBACK FOR DRAWING AREA: IF A MOUSE (KEYBOARD) EVENT OCCURED
    XtAddCallback(m_drawingArea, XmNinputCallback, staticImageEventCallback, this);

    // FILL TOOLBARS WITH WIDGETS
    createTopToolbar();
    createLeftToolbar();
}


/****************************************************************************
*  IMAGE DIALOG - CREATE TOP TOOLBAR
****************************************************************************/
void imageDialog::createTopToolbar()
{
    // CREATE PROTEOME DATA LABEL
    Widget label01= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_topToolbar,
        XmNlabelString, PGT_XmStringCreateLocalized("Proteome Data:"),
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNheight, 30,
        XmNwidth, 100,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    // CREATE TIFF IMAGE LABEL
    Widget label02= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_topToolbar,
        XmNlabelString, PGT_XmStringCreateLocalized("TIFF Image:"),
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNheight, 30,
        XmNwidth, 100,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label01,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    Widget ARBdataButton= XtVaCreateManagedWidget("ARBdataButton",
        xmPushButtonWidgetClass, m_topToolbar,
        XmNlabelString, PGT_XmStringCreateLocalized("..."),
        XmNwidth, 40,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
//         XmNleftAttachment, XmATTACH_WIDGET,
//         XmNleftWidget, m_ARBdata,
        NULL);
    XtAddCallback(ARBdataButton, XmNactivateCallback, staticARBdataButtonCallback, this);

    Widget TIFFnameButton= XtVaCreateManagedWidget("TIFFnameButton",
        xmPushButtonWidgetClass, m_topToolbar,
        XmNlabelString, PGT_XmStringCreateLocalized("..."),
        XmNwidth, 40,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, ARBdataButton,
        XmNrightAttachment, XmATTACH_FORM,
//         XmNleftAttachment, XmATTACH_WIDGET,
//         XmNleftWidget, m_TIFFname,
        NULL);
    XtAddCallback(TIFFnameButton, XmNactivateCallback, staticTIFFnameButtonCallback, this);

    m_ARBdata= XtVaCreateManagedWidget("ARBentries",
        xmTextWidgetClass, m_topToolbar,
        XmNheight, 30,
        XmNeditable, false,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, label01,
        XmNrightAttachment, XmATTACH_WIDGET,
        XmNrightWidget, ARBdataButton,
        NULL);

    m_TIFFname= XtVaCreateManagedWidget("TIFFname",
        xmTextWidgetClass, m_topToolbar,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_ARBdata,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, label02,
        XmNrightAttachment, XmATTACH_WIDGET,
        XmNrightWidget, TIFFnameButton,
        NULL);

    m_LockToggleButton= XtVaCreateManagedWidget("Lock visualized data",
        xmToggleButtonGadgetClass, m_topToolbar,
        XmNset, false,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_TIFFname,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_LockToggleButton, XmNvalueChangedCallback, staticLockToggleButtonCallback, this);

    m_UpdateGeneButton= XtVaCreateManagedWidget("Link to gene map",
        xmToggleButtonGadgetClass, m_topToolbar,
        XmNset, false,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_TIFFname,
        XmNrightAttachment, XmATTACH_WIDGET,
        XmNrightWidget, m_LockToggleButton,
        NULL);
    XtAddCallback(m_UpdateGeneButton, XmNvalueChangedCallback, staticUpdateGeneButtonCallback, this);

    Widget label03= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_topToolbar,
        XmNlabelString, PGT_XmStringCreateLocalized("Mouse Buttons:"),
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNheight, 30,
        XmNwidth, 100,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label02,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_topToolbar,
        XmNlabelString, PGT_XmStringCreateLocalized("left button = select protein  --  right button = (un)mark protein"),
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_TIFFname,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, label03,
        XmNrightAttachment, XmATTACH_WIDGET,
        XmNrightWidget, m_UpdateGeneButton,
        NULL);
}


/****************************************************************************
*  IMAGE DIALOG - CREATE LEFT TOOLBAR
****************************************************************************/
void imageDialog::createLeftToolbar()
{
    // GET FOREGROUND AND BACKGROUND PIXEL COLORS
    Pixel fg, bg;
    XtVaGetValues(m_leftToolbar, XmNforeground, &fg, XmNbackground, &bg, NULL);

    // USED PIXMAPS (BUTTON LOGOS)
    Pixmap circle22_xpm, cross22_xpm, text22_xpm, markonly22_xpm, onlyid22_xpm,
           markall22_xpm, marknone22_xpm, arb2mark_xpm, mark2arb_xpm, help_xpm,
           markid22_xpm, markinvert22_xpm;

    // OPEN THE PIXMAP FILES
    Screen *s        = XtScreen(m_leftToolbar);
    circle22_xpm=     PGT_LoadPixmap("circle22.xpm", s, fg, bg);
    cross22_xpm=      PGT_LoadPixmap("cross22.xpm", s, fg, bg);
    text22_xpm=       PGT_LoadPixmap("text22.xpm", s, fg, bg);
    markonly22_xpm=   PGT_LoadPixmap("markonly22.xpm", s, fg, bg);
    onlyid22_xpm=     PGT_LoadPixmap("onlyid22.xpm", s, fg, bg);
    markall22_xpm=    PGT_LoadPixmap("markall22.xpm", s, fg, bg);
    markinvert22_xpm= PGT_LoadPixmap("markinvert22.xpm", s, fg, bg);
    marknone22_xpm=   PGT_LoadPixmap("marknone22.xpm", s, fg, bg);
    markid22_xpm=     PGT_LoadPixmap("markid22.xpm", s, fg, bg);
    arb2mark_xpm=     PGT_LoadPixmap("arb2mark22.xpm", s, fg, bg);
    mark2arb_xpm=     PGT_LoadPixmap("mark2arb22.xpm", s, fg, bg);
    help_xpm=         PGT_LoadPixmap("help22.xpm", s, fg, bg);

    // SHOW CROSSHAIR BUTTON
    Widget spotsButton= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, cross22_xpm,
        NULL);
    XtAddCallback(spotsButton, XmNactivateCallback, staticCrosshairButtonCallback, this);

    // SHOW CIRCLE BUTTON
    Widget circleButton= XtVaCreateManagedWidget("circlebtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, circle22_xpm,
        NULL);
    XtAddCallback(circleButton, XmNactivateCallback, staticCircleButtonCallback, this);

    // SHOW TEXT BUTTON
    Widget textButton= XtVaCreateManagedWidget("textbtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, text22_xpm,
        NULL);
    XtAddCallback(textButton, XmNactivateCallback, staticTextButtonCallback, this);

    // CREATE HORIZONTAL SEPARATOR
    XtVaCreateManagedWidget("separator",
        xmSeparatorWidgetClass, m_leftToolbar,
        XmNorientation, XmHORIZONTAL,
        NULL);

    // SHOW ONLY SPOTS WITH TEXT BUTTON
    Widget textOnlyButton= XtVaCreateManagedWidget("textonlybtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, onlyid22_xpm,
        NULL);
    XtAddCallback(textOnlyButton, XmNactivateCallback, staticTextOnlyButtonCallback, this);

    // SHOW ONLY SPOTS WITH TEXT BUTTON
    Widget markedOnlyButton= XtVaCreateManagedWidget("markedonlybtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, markonly22_xpm,
        NULL);
    XtAddCallback(markedOnlyButton, XmNactivateCallback, staticMarkedOnlyButtonCallback, this);

    // CREATE HORIZONTAL SEPARATOR
    XtVaCreateManagedWidget("separator",
        xmSeparatorWidgetClass, m_leftToolbar,
        XmNorientation, XmHORIZONTAL,
        NULL);

    // MARK ALL PROTEINS BUTTON
    Widget markAllButton= XtVaCreateManagedWidget("markallbtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, markall22_xpm,
        NULL);
    XtAddCallback(markAllButton, XmNactivateCallback, staticMarkAllButtonCallback, this);

    // INVERT PROTEIN MARKER BUTTON
    Widget markInvertButton= XtVaCreateManagedWidget("markinvertbtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, markinvert22_xpm,
        NULL);
    XtAddCallback(markInvertButton, XmNactivateCallback, staticMarkInvertButtonCallback, this);

    // UNMARK ALL PROTEINS BUTTON
    Widget markNoneButton= XtVaCreateManagedWidget("marknonebtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, marknone22_xpm,
        NULL);
    XtAddCallback(markNoneButton, XmNactivateCallback, staticMarkNoneButtonCallback, this);

    // TRANSFER MARKER FROM SPOTS TO GENES BUTTON
    Widget markSpots2GenesButton= XtVaCreateManagedWidget("markspot2genesbtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, mark2arb_xpm,
        NULL);
    XtAddCallback(markSpots2GenesButton, XmNactivateCallback, staticSpots2GenesButtonCallback, this);

    // TRANSFER MARKER FROM SPOTS TO GENES BUTTON
    Widget markGenes2SpotsButton= XtVaCreateManagedWidget("markspot2genebtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, arb2mark_xpm,
        NULL);
    XtAddCallback(markGenes2SpotsButton, XmNactivateCallback, staticGenes2SpotsButtonCallback, this);

    // MARK ALL SPOTS WITH INFO
    Widget markSpotsWithInfoButton= XtVaCreateManagedWidget("markspotswithinfobtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, markid22_xpm,
        NULL);
    XtAddCallback(markSpotsWithInfoButton, XmNactivateCallback, staticMarkWithInfoButtonCallback, this);

    // HELP BUTTON
    Widget helpButton= XtVaCreateManagedWidget("helpbtn",
        xmPushButtonWidgetClass, m_leftToolbar,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, help_xpm,
        NULL);
    XtAddCallback(helpButton, XmNactivateCallback, staticHelpDialogCallback, this);
}


/****************************************************************************
*  CALLBACK - ARB DATA BUTTON CLICKED...
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticARBdataButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *sD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    sD->ARBdataButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - ARB DATA BUTTON CLICKED...
****************************************************************************/
void imageDialog::ARBdataButtonCallback(Widget, XtPointer)
{
    selectionDialog *sD= new selectionDialog(m_shell, this, SELECTION_DIALOG_READ);

    // SET SPECIES, EXPERIMENT AND PROTEOME CALLBACK
    sD->setSpeciesCallback(staticImageSpeciesCallback);
    sD->setExperimentCallback(staticImageExperimentCallback);
    sD->setProteomeCallback(staticImageProteomeCallback);
}


/****************************************************************************
*  CALLBACK - SELECTION DIALOG SPECIES CHANGED
****************************************************************************/
static void staticImageSpeciesCallback(Widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // GET SPECIES NAME FROM CALLDATA
    char *name= *((char **)callData);

    // SET NEW NAMES
    iD->setSpecies(name);
    iD->setExperiment("");
    iD->setProteome("");

    // UPDATE OUTPUT
    iD->updateARBText();
}


/****************************************************************************
*  CALLBACK - SELECTION DIALOG SPECIES CHANGED
****************************************************************************/
static void staticImageExperimentCallback(Widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // GET SPECIES NAME FROM CALLDATA
    char *name= *((char **)callData);

    // SET NEW NAMES
    iD->setExperiment(name);
    iD->setProteome("");

    // UPDATE OUTPUT
    iD->updateARBText();
}


/****************************************************************************
*  CALLBACK - SELECTION DIALOG SPECIES CHANGED
****************************************************************************/
static void staticImageProteomeCallback(Widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // GET SPECIES NAME FROM CALLDATA
    char *name= *((char **)callData);

    // SET NEW NAMES
    iD->setProteome(name);

    // UPDATE OUTPUT
    iD->updateARBText();
}


/****************************************************************************
*  CALLBACK - TIFF IMAGE NAME BUTTON CLICKED...
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticTIFFnameButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *sD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    sD->TIFFnameButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - TIFF IMAGE NAME BUTTON CLICKED...
****************************************************************************/
void imageDialog::TIFFnameButtonCallback(Widget, XtPointer)
{
    if (!m_hasFileDialog)
    {
        m_fileDialog= XmCreateFileSelectionDialog(m_shell, const_cast<char*>("importDialog"), NULL, 0);
        XtAddCallback(m_fileDialog, XmNokCallback, staticImageFileDialogCallback, this);
        XtAddCallback(m_fileDialog, XmNcancelCallback, staticImageFileDialogCloseCallback, this);
        XtAddCallback(m_fileDialog, XmNnoMatchCallback, staticImageFileDialogCloseCallback, this);
        XtSetSensitive(XmFileSelectionBoxGetChild(m_fileDialog, XmDIALOG_HELP_BUTTON), False);
        XtVaSetValues(m_fileDialog, XmNdialogTitle, PGT_XmStringCreateLocalized("Open proteome data file..."), NULL);
        m_hasFileDialog= true;
    }

    XtManageChild(m_fileDialog);
    XtPopup(XtParent(m_fileDialog), XtGrabNone);
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticImageFileDialogCloseCallback(Widget parent, XtPointer, XtPointer)
{
    // CLOSE FILE OPEN DIALOG
    XtUnmanageChild(parent);
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticImageFileDialogCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->imageFileDialogCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
****************************************************************************/
void imageDialog::imageFileDialogCallback(Widget widget, XtPointer callData)
{
    // FETCH SELECTED FILENAME
    XmFileSelectionBoxCallbackStruct *cbs;
    cbs= (XmFileSelectionBoxCallbackStruct *)callData;

    // CONVERT XMSTRING TO CHAR*
    char *str;
    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &str);

    // CLOSE FILE OPEN DIALOG
    XtUnmanageChild(widget);

    // WRITE STRING TO TEXT FIELD
    XtVaSetValues(m_TIFFname, XmNvalue, str, NULL);

    // SAVE THE PATH TO THE IMAGE IN THE ARB DATABASE
    set_ARB_image_path(str);

    // LOAD/UPDATE THE IMAGE
    updateImage();
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticCrosshairButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->crosshairButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
****************************************************************************/
void imageDialog::crosshairButtonCallback(Widget, XtPointer)
{
    // INVERT FLAG
    m_crosshairFlag= !m_crosshairFlag;

    // REDRAW IMAGE
    imageRedraw();
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticTextButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->textButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
****************************************************************************/
void imageDialog::textButtonCallback(Widget, XtPointer)
{
    // INVERT FLAG
    m_labelFlag= !m_labelFlag;

    // REDRAW IMAGE
    imageRedraw();
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticTextOnlyButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->textOnlyButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
****************************************************************************/
void imageDialog::textOnlyButtonCallback(Widget, XtPointer)
{
    // INVERT FLAG
    m_textOnlyFlag= !m_textOnlyFlag;

    // REDRAW IMAGE
    imageRedraw();
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticCircleButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->circleButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
****************************************************************************/
void imageDialog::circleButtonCallback(Widget, XtPointer)
{
    // INVERT FLAG
    m_circleFlag= !m_circleFlag;

    // REDRAW IMAGE
    imageRedraw();
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticMarkedOnlyButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->markedOnlyButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
****************************************************************************/
void imageDialog::markedOnlyButtonCallback(Widget, XtPointer)
{
    // INVERT FLAG
    m_markedOnlyFlag= !m_markedOnlyFlag;

    // REDRAW IMAGE
    imageRedraw();
}


/****************************************************************************
*  CALLBACK - MARK ALL BUTTON
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticMarkAllButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->markAllButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - MARK ALL BUTTON
****************************************************************************/
void imageDialog::markAllButtonCallback(Widget, XtPointer)
{
    // SET MARKER
    setAllMarker(PROTEIN_MARK);

    // CREATE A NEW SPOT LIST
    createSpotList();

    // REDRAW IMAGE
    imageRedraw();
}


/****************************************************************************
*  CALLBACK - INVERT MARK BUTTON
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticMarkInvertButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->markInvertButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - INVERT MARK BUTTON
****************************************************************************/
void imageDialog::markInvertButtonCallback(Widget, XtPointer)
{
    // SET MARKER
    setAllMarker(PROTEIN_INVERT);

    // CREATE A NEW SPOT LIST
    createSpotList();

    // REDRAW IMAGE
    imageRedraw();
}


/****************************************************************************
*  CALLBACK - MARK NONE BUTTON
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticMarkNoneButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->markNoneButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - MARK NONE BUTTON
****************************************************************************/
void imageDialog::markNoneButtonCallback(Widget, XtPointer)
{
    // SET MARKER
    setAllMarker(PROTEIN_UNMARK);

    // CREATE A NEW SPOT LIST
    createSpotList();

    // REDRAW IMAGE
    imageRedraw();
}


/****************************************************************************
*  (UN)MARK ALL BUTTONS
****************************************************************************/
bool imageDialog::setAllMarker(int state)
{
    GBDATA *gb_data, *gb_proteine_data, *gb_protein;
    int flag;

    // GET MAIN ARB GBDATA
    gb_data= get_gbData();
    if(!gb_data) return false;

    // FIND SELECTED PROTEIN DATA ENTRY
    gb_proteine_data= find_proteine_data(m_species, m_experiment, m_proteome);
    if(!gb_proteine_data) return false;

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // BROWSE ALL PROTEIN ENTRIES...
    gb_protein= GB_find(gb_proteine_data, "protein", 0, down_level);

    while(gb_protein)
    {
        // GET ACTUAL STATE
        flag= GB_read_flag(gb_protein);

        switch(state)
        {
            case PROTEIN_MARK:
                GB_write_flag(gb_protein, 1);
            break;
            case PROTEIN_UNMARK:
                GB_write_flag(gb_protein, 0);
            break;
            case PROTEIN_INVERT:
                GB_write_flag(gb_protein, !flag);
            break;
        }

        // FETCH NEXT PROTEIN FROM LIST
        gb_protein= GB_find(gb_protein, "protein", 0, this_level|search_next);
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();

    return true;
}


/****************************************************************************
*  CALLBACK - LOCK/UNLOCK ARB AWARS AND DATA UPDATE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticLockToggleButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->lockToggleButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - IMPORT IMAGE FILE NAME VALUE
****************************************************************************/
void imageDialog::lockToggleButtonCallback(Widget, XtPointer callData)
{
    XmToggleButtonCallbackStruct *cbs= (XmToggleButtonCallbackStruct *)callData;

    if(cbs->set) m_lockVisualization= true;
    else m_lockVisualization= false;

    // REDRAW IMAGE
    imageRedraw();
}


/****************************************************************************
*  IMAGE DIALOG - SET NEW SPECIES NAME
****************************************************************************/
void imageDialog::setSpecies(const char *species)
{
    // REMOVE OLD ENTRY
    // if(m_species) free(m_species);

    m_species= (char *)malloc((strlen(species) +1)*sizeof(char));
    strcpy(m_species, species);

    // SET ARB DATA AVAILABLE FLAG
    m_hasARBdata= false;

    // SEND DATA TO ARB AWAR (IF NOT LOCKED)
    if(!m_lockVisualization) set_species_AWAR(m_species);
}


/****************************************************************************
*  IMAGE DIALOG - SET NEW EXPERIMENT NAME
****************************************************************************/
void imageDialog::setExperiment(const char *experiment)
{
    // REMOVE OLD ENTRY
    // if(m_experiment) free(m_experiment);

    m_experiment= (char *)malloc((strlen(experiment) +1)*sizeof(char));
    strcpy(m_experiment, experiment);

    // SET ARB DATA AVAILABLE FLAG
    m_hasARBdata= false;

    // SEND DATA TO ARB AWAR (IF NOT LOCKED)
    if(!m_lockVisualization) set_experiment_AWAR(m_experiment);
}


/****************************************************************************
*  IMAGE DIALOG - SET NEW PROTEOME NAME
****************************************************************************/
void imageDialog::setProteome(const char *proteome)
{
    // REMOVE OLD ENTRY
    // if(m_proteome) free(m_proteome);

    m_proteome= (char *)malloc((strlen(proteome) +1)*sizeof(char));
    strcpy(m_proteome, proteome);

    // CREATE A NEW SPOT LIST
    createSpotList();

    // SET ARB DATA AVAILABLE FLAG
    m_hasARBdata= true;

    // DO WE HAVE AN AVAILABLE IMAGE? IF NOT, SIMULATE ONE...
    if(!m_hasTIFFdata && !m_hasImagedata) blankImage();

    // SEND DATA TO ARB AWAR (IF NOT LOCKED)
    if(!m_lockVisualization) set_proteom_AWAR(m_proteome);
}


/****************************************************************************
*  IMAGE DIALOG - UPDATE THE ARB TEXT WIDGET
****************************************************************************/
void imageDialog::updateARBText()
{
    // CREATE BUFFER
    char *buf= (char *)malloc(1024 * sizeof(char));
    const char *sp=   "no species";
    const char *exp=  "no experiment";
    const char *prot= "no proteome";

    // USE REAL STRINGS IF AVAILABLE
    if(m_species && (strlen(m_species))) sp= m_species;
    if(m_experiment && (strlen(m_experiment))) exp= m_experiment;
    if(m_proteome && (strlen(m_proteome))) prot= m_proteome;

    // FILL BUFFER STRING
    sprintf(buf, "[%s] -> [%s] -> [%s]", sp, exp, prot);

    // COPY STRING TO TEXT FIELD
    XtVaSetValues(m_ARBdata, XmNvalue, buf, NULL);

    if(m_species && m_experiment && m_proteome)
    {
        // UPDATE FLAG
        m_hasARBdata= true;

        // CREATE DESCRIPTIONS TABLE
        createDescriptions();

        // CREATE A NEW SPOT LIST
        createSpotList();

        // CHECK IF AN IMAGE IS AVAILABLE AND OPEN IT IF POSSIBLE
        char *path= get_ARB_image_path();
        if(path)
        {
            // WRITE STRING TO TEXT FIELD
            XtVaSetValues(m_TIFFname, XmNvalue, path, NULL);

            // LOAD/UPDATE THE IMAGE
            if(updateImage())
            {
                // updateImage() failed...

                // CLEAR ARB IMAGE PATH
                set_ARB_image_path("");

                // WRITE STRING TO TEXT FIELD
                XtVaSetValues(m_TIFFname, XmNvalue, "", NULL);
            }
        }
    }
    // FREE BUFFER
    free(buf);
}


/****************************************************************************
*  IMAGE DIALOG - GET THE PATH TO THE LAST USED IMAGE
****************************************************************************/
char *imageDialog::get_ARB_image_path()
{
    if(!m_hasARBdata) return NULL;

    char *path= NULL;

    GBDATA *gb_proteom= find_proteome(m_species, m_experiment, m_proteome);

    if(gb_proteom)
    {
        ARB_begin_transaction();

        GBDATA *gb_imagepath= GB_find(gb_proteom, "image_path", 0, down_level);

        if(gb_imagepath) path= GB_read_string(gb_imagepath);

        ARB_commit_transaction();
    }

    return path;
}


/****************************************************************************
*  IMAGE DIALOG - GET THE PATH TO THE LAST USED IMAGE
****************************************************************************/
void imageDialog::set_ARB_image_path(const char *path)
{
    if(!m_hasARBdata) return;

    GBDATA *gb_proteom, *gb_imagepath;

    gb_proteom= find_proteome(m_species, m_experiment, m_proteome);

    if(gb_proteom)
    {
        ARB_begin_transaction();

        if(!(gb_imagepath= GB_search(gb_proteom, "image_path", GB_FIND)))
            gb_imagepath= GB_create(gb_proteom, "image_path", GB_STRING);

        if(gb_imagepath) GB_write_string(gb_imagepath, path);

        ARB_commit_transaction();
    }

    return;
}


/****************************************************************************
*  IMAGE DIALOG - LOAD/UPDATE THE ARB AND IMAGE DATA
****************************************************************************/
int imageDialog::updateImage()
{
    // CREATE BUFFER
    char *buf= (char *)malloc(1024 * sizeof(char));
    char *filename;
    char *file;

    // SET ENVIRONMENT VARIABLES
    Display *display= XtDisplay(m_drawingArea);
    Window window= XtWindow(m_drawingArea);

    // GET SELECTED TIFF FILENAME
    filename= XmTextGetString(m_TIFFname);

    // FILE ALREADY OPENED?
    if(m_filename && (!strcmp(filename, m_filename))) return -1;

    // CREATE NEW IMAGE CLASS IF NECESSARY
    if(!m_image) m_image= new TIFFimage();

    // OPEN TIFF IMAGE FILE
    if(m_image->open(filename)) return -1;

    // GET DIMENSIONS
    m_width= m_image->width();
    m_height= m_image->height();

    // CREATE A PIXMAP
    m_pixmap = XCreatePixmap(display, window, m_width, m_height, XDefaultDepth(display, 0));

    // CREATE XIMAGE FROM IMAGE DATA
    m_ximage= m_image->createXImage(m_drawingArea);

    // TIFF DATA AVAILABLE
    m_hasTIFFdata= true;
    m_hasImagedata= true;

    // TRIGGER THE REDRAWING OF THE IMAGE
    imageRedraw();

    // STORE NEW FILENAME
    m_filename= filename;

    // ISOLATE FILENAME WITHOUT PATH (FOR USE IN WINDOW TITLE)
    file= strrchr(filename, '/');
    if(!file) file= filename; // IF NO '/' WAS FOUND USE WHOLE FILENAME
    else if(*(file+1)) file++; // REMOVE '/' CHAR FROM BEGINNING

    // SET WINDOW TITLE
    sprintf(buf, "PGT - Image View (%s)", file);
    setWindowName(buf);

    // FREE BUFFER
    free(buf);

    return 0;
}


/****************************************************************************
*  IMAGE DIALOG - CREATE A BLANK IMAGE
*  (THIS IS NEEDED FOR DATA WITHOUT A RELATED IMAGE FILE)
****************************************************************************/
int imageDialog::blankImage()
{
    // CREATE AND FILL A BLANK IMAGE BACKGROUND
    int retval= fillBlankImage();

    // TRIGGER THE REDRAWING OF THE IMAGE
    imageRedraw();

    return retval;
}


/****************************************************************************
*  IMAGE DIALOG - FETCH THE MAX. DIMENSIONS FOR A BLANK WINDOW
****************************************************************************/
int imageDialog::getSpotMaxDimensions()
{
    // RETURN, IF WE HAVE NO AVAILABLE DATA
    if(!m_hasARBdata)
    {
        m_width=  0;
        m_height= 0;

        return -1;
    }

    // CREATE AN ITERATOR
    list<SPOT>::iterator spot_it;

    float max_fx= 0, max_fy= 0;

    // TRAVERSE THE COMPLETE LIST
    for(spot_it= m_spotList.begin(); spot_it != m_spotList.end(); ++spot_it)
    {
        if((*spot_it).x + (*spot_it).diameter > max_fx)
            max_fx= (*spot_it).x + (*spot_it).diameter;

        if((*spot_it).y + (*spot_it).diameter > max_fy)
            max_fy= (*spot_it).y + (*spot_it).diameter;
    }

    m_width= (int)max_fx;
    m_height= (int)max_fy;

    return 0;
}


/****************************************************************************
*  IMAGE DIALOG - FETCH THE MAX. DIMENSIONS FOR A BLANK WINDOW
****************************************************************************/
int imageDialog::fillBlankImage()
{
    // SET ENVIRONMENT VARIABLES
    Display *display= XtDisplay(m_drawingArea);
    Window window= XtWindow(m_drawingArea);
    GC gc= XCreateGC(display, window, 0, 0);

    // GET SPOT MAX. DIMENSIONS (TO M_WIDTH, M_HEIGHT)
    getSpotMaxDimensions();

    // EXIT, IF SIZE IS TOO SMALL
    if((m_width < MIN_IMAGE_SIZE) && (m_width < MIN_IMAGE_SIZE))
        return -1;

    // CREATE A PIXMAP, IF NOT ALREADY DONE...
    if(!m_hasImagedata) m_pixmap = XCreatePixmap(display, window, m_width, m_height, XDefaultDepth(display, 0));

    // SET COLOR TO GRAY
    setColor(display, gc, 0xCC, 0xCC, 0xCC);

    // SOLID-FILL AREA WITH COLOR
    XFillRectangle(display, m_pixmap, gc, 0, 0,
        (unsigned int) m_width, (unsigned int) m_height) ;

    // SET COLOR TO DARKER GRAY
    setColor(display, gc, 0xBB, 0xBB, 0xBB);

    Pixmap Pattern;
    static char Pat[] =
        {0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
         0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF,
         0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00,
         0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00}; // 16x16 PIXMAP

    Pattern= XCreateBitmapFromData(display, window, Pat, 16, 16);

    XSetStipple(display, gc, Pattern);

    XSetFillStyle(display, gc, FillStippled);

    // PATTERN-FILL AREA
    XFillRectangle(display, m_pixmap, gc, 0, 0,
        (unsigned int) m_width, (unsigned int) m_height) ;

    // TIFF DATA AVAILABLE
    m_hasImagedata= true;

    return 0;

}


/****************************************************************************
*  IMAGE DIALOG - REDRAW XIMAGE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticImageRedrawCallback(Widget, XtPointer clientData, XtPointer)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->imageRedrawCallback();
}


/****************************************************************************
*  IMAGE DIALOG - REDRAW XIMAGE - CALLBACK
****************************************************************************/
void imageDialog::imageRedrawCallback()
{
    // RETURN, IF NO IMAGE DATA IS AVAILABLE
    if(!m_hasImagedata) return;

    // SET ENVIRONMENT VARIABLES
    Display *display= XtDisplay(m_drawingArea);
    Window window= XtWindow(m_drawingArea);
    GC gc= XCreateGC(display, window, 0, 0);

    // COPY PIXMAP TO DRAWING AREA
    XCopyArea(display, m_pixmap, window, gc, 0, 0, m_width, m_height, 0, 0);
}


/****************************************************************************
*  IMAGE DIALOG - REDRAW XIMAGE
****************************************************************************/
void imageDialog::imageRedraw()
{
    // RETURN, IF NO IMAGE DATA IS AVAILABLE
    if(!m_hasImagedata) return;

    // SET ENVIRONMENT VARIABLES
    Display *display= XtDisplay(m_drawingArea);
    Window window= XtWindow(m_drawingArea);
    GC gc= XCreateGC(display, window, 0, 0);

    // RESIZE DRAWING AREA
    XtVaSetValues(m_drawingArea,
        XmNwidth, m_width,
        XmNheight, m_height,
        NULL);

    // COPY XIMAGE (TIFF IMAGE) TO LOCAL PIXMAP
    if(m_hasTIFFdata) XPutImage(display, m_pixmap, gc, m_ximage, 0, 0, 0, 0, m_width, m_height);
    else if(m_hasImagedata) fillBlankImage();

    // ARB DATA AVAILABLE !?
    if(!m_hasARBdata) setText("[no ARB data available]", 10, 10);
    else drawSpots();

    // COPY PIXMAP TO DRAWING AREA
//     XPutImage(display, window, gc, m_ximage, 0, 0, 0, 0, m_width, m_height);
    XCopyArea(display, m_pixmap, window, gc, 0, 0, m_width, m_height, 0, 0);

    // UPDATE THE STATUS LABEL WHENEVER THE IMAGE IS REDRAWN
    updateStatusLabel();
}


/****************************************************************************
*  IMAGE DIALOG - REDRAW XIMAGE
****************************************************************************/
void imageDialog::setText(const char *text, int x, int y)
{
    // NO TEXT, IF POINTER = NULL
    if(!text) return;

    // SET ENVIRONMENT VARIABLES
    Display *display= XtDisplay(m_drawingArea);
    Window window= XtWindow(m_drawingArea);
    // GC gc= XCreateGC(display, m_pixmap, 0, 0);
    GC gc= XCreateGC(display, window, 0, 0);

    // SET TEXT COLOR
    setColor(display, gc, m_textColor.r, m_textColor.g, m_textColor.b);

    // GET FONT LIST
    XFontStruct *fontstruct= XLoadQueryFont(display, "-*-courier-*-r-*--10-*");
    XmFontList fontlist= XmFontListCreate(fontstruct, const_cast<char*>("charset1"));

    // SET FONT (A SMALL DIRTY WORKAROUND)
    XSetFont (display, gc, XLoadFont(display, "fixed"));

    XmStringDraw(display, m_pixmap, fontlist, PGT_XmStringCreateLocalized(text),
        gc, x, y, m_width, XmALIGNMENT_BEGINNING,
        XmSTRING_DIRECTION_L_TO_R, NULL);
}


/****************************************************************************
*  IMAGE DIALOG - REDRAW XIMAGE
****************************************************************************/
void imageDialog::setColor(Display *display, GC gc, int r, int g, int b)
{
    XColor xc;

    xc.red= r << 8; xc.green= g << 8; xc.blue= b << 8;
    xc.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(display, DefaultColormap(display, DefaultScreen(display)), &xc);
    XSetForeground(display, gc, xc.pixel);
}


/****************************************************************************
*  IMAGE DIALOG - REDRAW XIMAGE
****************************************************************************/
void imageDialog::drawCrosshair(int x, int y)
{
    // SET ENVIRONMENT VARIABLES
    Display *display= XtDisplay(m_drawingArea);
//     Window window= XtWindow(m_drawingArea);
    GC gc= XCreateGC(display, m_pixmap, 0, 0);

    // RED COLOR
    setColor(display, gc, m_crosshairColor.r, m_crosshairColor.g, m_crosshairColor.b);

    // DRAW THE LINES FOR A CROSSHAIR
    XDrawLine(display, m_pixmap, gc, x-2, y, x+2, y);
    XDrawLine(display, m_pixmap, gc, x, y-2, x, y+2);

    XFlush(display);
}


/****************************************************************************
*  IMAGE DIALOG - DRAW CIRCLE
****************************************************************************/
void imageDialog::drawCircle(int x, int y, int d, int r, int g, int b)
{
    // (X,Y) = POSITION
    // D     = DIAMETER
    // R,G,B = COLOR VALUE

    // SET ENVIRONMENT VARIABLES
    Display *display= XtDisplay(m_drawingArea);
    GC gc= XCreateGC(display, m_pixmap, 0, 0);

    // DRAW THE CIRCLE(S)
    setColor(display, gc, r, g, b);
    XFillArc(display, m_pixmap, gc, x-(d/2), y-(d/2), d, d, 0, 360*64);

    setColor(display, gc, 0, 0, 0);
    XDrawArc(display, m_pixmap, gc, x-(d/2), y-(d/2), d, d, 0, 360*64);

    XFlush(display);
}


/****************************************************************************
*  IMAGE DIALOG - FETCH COLOR & TEXT SETTINGS
****************************************************************************/
bool imageDialog::getSettings()
{
    // FETCH CONFIGS
    char *crosshairColor = get_CONFIG(CONFIG_PGT_COLOR_CROSSHAIR);
    char *unmarkedColor  = get_CONFIG(CONFIG_PGT_COLOR_UNMARKED);
    char *markedColor    = get_CONFIG(CONFIG_PGT_COLOR_MARKED);
    char *selectedColor  = get_CONFIG(CONFIG_PGT_COLOR_SELECTED);
    char *textColor      = get_CONFIG(CONFIG_PGT_COLOR_TEXT);

    if(crosshairColor)
    {
        if(strlen(crosshairColor) != 7) crosshairColor= strdup(DEFAULT_COLOR_CROSSHAIR);

        hex2rgb(&m_crosshairColor.r,
                &m_crosshairColor.g,
                &m_crosshairColor.b,
                crosshairColor);
    }


    if(unmarkedColor)
    {
        if(strlen(unmarkedColor) != 7) unmarkedColor= strdup(DEFAULT_COLOR_UNMARKED);

        hex2rgb(&m_unmarkedColor.r,
                &m_unmarkedColor.g,
                &m_unmarkedColor.b,
                unmarkedColor);
    }

    if(markedColor)
    {
        if(strlen(markedColor) != 7) markedColor= strdup(DEFAULT_COLOR_MARKED);

        hex2rgb(&m_markedColor.r,
                &m_markedColor.g,
                &m_markedColor.b,
                markedColor);
    }

    if(selectedColor)
    {
        if(strlen(selectedColor) != 7) selectedColor= strdup(DEFAULT_COLOR_SELECTED);

        hex2rgb(&m_selectedColor.r,
                &m_selectedColor.g,
                &m_selectedColor.b,
                selectedColor);
    }

    if(textColor)
    {
        if(strlen(selectedColor) != 7) textColor= strdup(DEFAULT_COLOR_SELECTED);

        hex2rgb(&m_textColor.r,
                &m_textColor.g,
                &m_textColor.b,
                textColor);
    }

    free(crosshairColor );
    free(unmarkedColor  );
    free(markedColor    );
    free(selectedColor  );
    free(textColor      );

    return true;
}


/****************************************************************************
*  IMAGE DIALOG - DISPLAY ARB DATA -> DRAW SPOTS
****************************************************************************/
void imageDialog::drawSpots()
{
    // SOME VARIABLES
    bool show_spot;
    int x, y, diameter;

    // CHECK IF TIFF- AND ARB-DATA IS AVAILABLE
    if(!m_hasImagedata && !m_hasARBdata) return;

    // LOCK CHANGES (AVOIDS DISTURBING CALLBACKS)
    m_changeInProgress= true;

    // CREATE AN ITERATOR
    list<SPOT>::iterator spot_it;

    // DO WE HAVE SPOT ENTRIES?
    if(m_spotList.size() == 0) return;


    // RESET SPOT COUNTER VALUES
    m_numSpots= 0;
    m_numMarkedSpots= 0;
    m_numSelectedSpots= 0;
    if(m_selectedSpotName)
    {
        free(m_selectedSpotName);
        m_selectedSpotName= NULL;
    }

    // ITERATE THROUGH THE SPOTS
    for(spot_it= m_spotList.begin(); spot_it != m_spotList.end(); ++spot_it)
    {
        // INCREASE SPOT COUNTER
        m_numSpots++;
        if((*spot_it).marked) m_numMarkedSpots++;
        if((*spot_it).selected)
        {
            if(!m_selectedSpotName) m_selectedSpotName= strdup((*spot_it).id);

            m_numSelectedSpots++;
        }

        // FIRST, LET US SHOW THE ACTUAL SPOT
        show_spot= true;

        // X,Y ARE USED OFTEN, SO EXTRACT THEM
        x= (int)(*spot_it).x;
        y= (int)(*spot_it).y;
        diameter= (int)(*spot_it).diameter;

        // SHOW ONLY SPOTS WHO HAVE AN IDENTIFIER?
        if(m_textOnlyFlag && (!(*spot_it).id || (*((*spot_it).id) == 0))) show_spot= false;

        // SHOW ONLY SPOTS WHICH ARE MARKED?
        if(m_markedOnlyFlag && !((*spot_it).marked)) show_spot= false;

        // ALWAYS SHOW THE SELECTED SPOT
        if((*spot_it).selected) show_spot= true;

        // SO, LET US DRAW THE SPOT (OR NOT)...
        if(show_spot)
        {
            // DRAW CIRCLE AT THE DEFINED POSITION
            if(m_circleFlag  && (x > 0))
            {
                // IF SELECTED, DRAW THICKER CIRCLE -> THICK BLACK BORDER
                if((*spot_it).selected)
                {
                    // FAT BLACK CIRCLE (BORDER)
                    drawCircle(x, y, diameter+2, 0, 0, 0);

                    // INNER COLORED CIRCLE
                    drawCircle(x, y, diameter, m_selectedColor.r, m_selectedColor.g, m_selectedColor.b);
                }

                // IF MARKED, USE DIFFERENT COLOR FOR SPOT
                else if((*spot_it).marked)
                    drawCircle(x, y, diameter, m_markedColor.r, m_markedColor.g, m_markedColor.b);

                // OTHERWISE JUST DRAW AN UNMARKED SPOT
                else drawCircle(x, y, diameter, m_unmarkedColor.r, m_unmarkedColor.g, m_unmarkedColor.b);
            }

            // SET CROSSHAIR AT THE DEFINED POSITION
            if(m_crosshairFlag  && (x > 0)) drawCrosshair(x, y);

            // ADD INFORMATIONAL TEXT TO THE (MARKED OR SELECTED) SPOT?
            if(m_labelFlag  && (x > 0) && ((*spot_it).selected || (*spot_it).marked))
                if((*spot_it).text) setText((*spot_it).text, x + (diameter/2) + 2, y);
        }
    }

    // UNLOCK CHANGES (PERMIT CALLBACKS)
    m_changeInProgress= false;
}


/****************************************************************************
*  CALLBACK - IMAGE EVENT CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticImageEventCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->imageEventCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - IMAGE EVENT CALLBACK
****************************************************************************/
void imageDialog::imageEventCallback(Widget, XtPointer callData)
{
    int x= 0, y= 0, button= 0;

    // FETCH CALLBACKSTRUCT + EVENT
    XmDrawingAreaCallbackStruct *cbs= (XmDrawingAreaCallbackStruct *)callData;
    XEvent *event= (XEvent *)cbs->event;

    if(event->xany.type == ButtonPress)
    {
        // GET COORDINATES
        x= event->xbutton.x;
        y= event->xbutton.y;

        // GET PRESSED BUTTON
        button= event->xbutton.button;

        // SELECT OR MARK BUTTON NEXT TO POS (X,Y)
        markSpotAtPos(button, x, y);

        // CREATE A NEW SPOT LIST
        createSpotList();

       // REDRAW IMAGE
        imageRedraw();
    }
}


/****************************************************************************
*  SELECT/MARK THE SPOT NEXT TO THE POS. X, Y
****************************************************************************/
void imageDialog::markSpotAtPos(int button, int x, int y)
{
    // LOCAL VARIABLES
    GBDATA *gb_data= NULL;
//     GBDATA *gb_proteome= NULL;
    GBDATA *gb_proteine_data= NULL;
    GBDATA *gb_protein= NULL;
    GBDATA *gb_nearest_protein= NULL;
    GBDATA *gb_protein_x= NULL;
    GBDATA *gb_protein_y= NULL;
    GBDATA *gb_protein_name= NULL;
    char *protein_name= NULL;
    float protein_x, protein_y;
    float delta, nearest_delta= 99999;
    float sqx, sqy;
    float fx= (float)x;
    float fy= (float)y;

    // GET MAIN ARB GBDATA
    gb_data= get_gbData();
    if(!gb_data) return;

    // FIND SELECTED PROTEOME/PROTEINE_DATA
    gb_proteine_data= find_proteine_data(m_species, m_experiment, m_proteome);
    if(!gb_proteine_data) return;

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // BROWSE ALL PROTEIN ENTRIES...
    gb_protein= GB_find(gb_proteine_data, "protein", 0, down_level);
    while(gb_protein)
    {
        // FETCH COORDINATE CONTAINER
        gb_protein_x= GB_find(gb_protein, m_x_container, 0, down_level);
        gb_protein_y= GB_find(gb_protein, m_y_container, 0, down_level);

        if(gb_protein_x && gb_protein_y)
        {
            // READ COORDINATES FROM THE CONTAINER
            protein_x= GB_read_float(gb_protein_x);
            protein_y= GB_read_float(gb_protein_y);

            // CALCULATE DIAGONAL LENGTH
            sqx= (fx - protein_x) * (fx - protein_x);
            sqy= (fy - protein_y) * (fy - protein_y);
            delta= sqrt(sqx + sqy);

            if(delta < nearest_delta)
            {
                gb_nearest_protein= gb_protein;
                nearest_delta= delta;

                gb_protein_name= GB_find(gb_protein, m_id_container, 0, down_level);
                protein_name= GB_read_string(gb_protein_name);
            }
        }

        // FETCH NEXT PROTEIN FROM LIST
        gb_protein= GB_find(gb_protein, "protein", 0, this_level|search_next);
    }

    // IF MOUSE CLICK IS TOO FAR AWAY FROM THE CENTER -> IGNORE
    if(nearest_delta > MAX_SELECTION_DISTANCE)
    {
        // CLOSE THE ARB TRANSACTION
        ARB_commit_transaction();

        // RETURN WITHOUT SELECTING/MARKING ANYTHING
        return;
    }

    // DO WE HAVE A 'NEAREST' PROTEIN SPOT?
    if(gb_nearest_protein)
    {
        // THE FOLLOWING ACTION DEPENDS ON THE PRESSED BUTTON:
        // LEFT  = SELECT SPOT/PROTEIN
        // RIGHT = MARK SPOT/PROTEIN
        if(button == LEFT_MOUSE_BUTTON)
        {
            // IF NOT LOCKED WRITE SELECTED PROTEIN NAME INTO THE AWAR
            if(protein_name) set_protein_AWAR(protein_name);
        }
        else if(button == RIGHT_MOUSE_BUTTON)
        {
            // INVERT MARKER (MARK/UNMARK PROTEIN)
            GB_write_flag(gb_nearest_protein, !GB_read_flag(gb_nearest_protein));
        }
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();
}


/****************************************************************************
*  ARB AWAR CALLBACK - PROTEIN ENTRY HAS CHANGED
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void static_ARB_protein_callback(GBDATA *, imageDialog *iD, GB_CB_TYPE)
{
     // // GET POINTER OF THE ORIGINAL CALLER
    // imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->ARB_protein_callback();
}


/****************************************************************************
*  ARB AWAR CALLBACK - PROTEIN ENTRY HAS CHANGED
****************************************************************************/
void imageDialog::ARB_protein_callback()
{
    if(!m_lockVisualization && !m_changeInProgress)
    {
        createSpotList();
        imageRedraw();
    }

    // IF ENABLED COPY IDENTIFIER TO SELECTED GENE
    if(m_updateGene) updateSelectedGene();
}


/****************************************************************************
*  ARB AWAR CALLBACK - GENE ENTRY HAS CHANGED
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void static_ARB_gene_callback(GBDATA *, imageDialog *iD, GB_CB_TYPE)
{
     // // GET POINTER OF THE ORIGINAL CALLER
    // imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->ARB_gene_callback();
}


/****************************************************************************
*  ARB AWAR CALLBACK - GENE ENTRY HAS CHANGED
****************************************************************************/
void imageDialog::ARB_gene_callback()
{
    char *awar_selected_gene, *config_gene_id, *name, *id;
    GBDATA *gb_genome, *gb_gene, *gb_gene_name, *gb_gene_id;

    if(!m_lockVisualization && !m_changeInProgress)
    {
        // FETCH GENE AWAR CONTENT & CONFIG
        awar_selected_gene= get_gene_AWAR();
        config_gene_id= get_CONFIG(CONFIG_PGT_ID_GENE);

        gb_genome= find_genome(m_species);
        if(!gb_genome) return;

        // INIT AN ARB TRANSACTION
        ARB_begin_transaction();

        gb_gene= GB_find(gb_genome, "gene", 0, down_level);

        while(gb_gene)
        {
            gb_gene_name= GB_find(gb_gene, "name", 0, down_level);

            if(gb_gene_name)
            {
                name= GB_read_as_string(gb_gene_name);

                if(name && (!strcmp(name, awar_selected_gene)))
                {
                    gb_gene_id= GB_find(gb_gene, config_gene_id, 0, down_level);

                    if(gb_gene_id)
                    {
                        id= GB_read_as_string(gb_gene_id);
                        set_protein_AWAR(id);
                        break;
                    }
                }
            }

            gb_gene= GB_find(gb_gene, "gene", 0, this_level|search_next);
        }

        // CLOSE THE ARB TRANSACTION
        ARB_commit_transaction();

        // UPDATE IMAGE
        createSpotList();
        imageRedraw();
    }

    // IF ENABLED COPY IDENTIFIER TO SELECTED GENE
    if(m_updateGene) updateSelectedGene();
}


/****************************************************************************
*  ARB AWAR CALLBACK - PGT CONFIG HAS CHANGED
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void static_PGT_config_callback(GBDATA *, imageDialog *iD, GB_CB_TYPE)
{
     // // GET POINTER OF THE ORIGINAL CALLER
    // imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->PGT_config_callback();
}


/****************************************************************************
*  ARB AWAR CALLBACK - PGT CONFIG HAS CHANGED
****************************************************************************/
void imageDialog::PGT_config_callback()
{
    // SETTINGS HAVE CHANGED - SO FETCH NEW SETTINGS...
    getSettings();

    // SPOT LIST MAY HAVE CHANGED ASWELL - CREATE A NEW ONE...
    createDescriptions();
    createSpotList();

    // UPDATE THE IMAGE (COLORS MAY HAVE CHANGED)
    imageRedraw();
}


/****************************************************************************
*  REACT TO A CHANGE OF THE SELECTED GENE: YES/NO
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticUpdateGeneButtonCallback(Widget, XtPointer clientData, XtPointer)
{
     // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->updateGeneButtonCallback();
}


/****************************************************************************
*  REACT TO A CHANGE OF THE SELECTED GENE: YES/NO
****************************************************************************/
void imageDialog::updateGeneButtonCallback()
{
    // INVERT FLAG
    m_updateGene= !m_updateGene;

    // IF ENABLED COPY IDENTIFIER TO SELECTED GENE
    if(m_updateGene) updateSelectedGene();

    // REDRAW IMAGE
    imageRedraw();
}


/****************************************************************************
*  CREATE GENE IDENTIFIER KEY (FOR QUICK HASH ACCESS)
****************************************************************************/
void imageDialog::genGeneKey(char *buf, char *spot_id, float x, float y)
{
    if(buf == NULL) return;

    unsigned int i_x= (unsigned int)x;
    unsigned int i_y= (unsigned int)y;

    if((spot_id != NULL) && (strlen(spot_id) > 0))
    {
        sprintf(buf, "%s_%d_%d", spot_id, i_x, i_y);
    }
    else
    {
        sprintf(buf, "0_%d_%d", i_x, i_y);
    }
}


/****************************************************************************
*  CREATE / UPDATE THE COMPLETE SPOT LIST
****************************************************************************/
bool imageDialog::createSpotList()
{
    // LOCAL SPOT
    SPOT spot;

    GBDATA *gb_data, *gb_proteine_data, *gb_protein, *gb_protein_id;
    GBDATA *gb_protein_x, *gb_protein_y, *gb_protein_area;

    // CREATE AN ITERATOR
    list<SPOT>::iterator spot_it;

    // GET MAIN ARB GBDATA
    gb_data= get_gbData();
    if(!gb_data) return false;

    // CREATE GENE KEY BUFFER
    char *keyBuf= (char *)malloc(1024 * sizeof(char));

    // FETCH PROTEIN AWAR CONTENT
    char *awar_selected_protein= get_protein_AWAR();
    char *awar_protein_id=       get_CONFIG(CONFIG_PGT_ID_PROTEIN);
    const char *awar_protein_x=        "x_coordinate"; // DEBUG - HARDCODED
    const char *awar_protein_y=        "y_coordinate"; // DEBUG - HARDCODED
    const char *awar_protein_area=     "area";         // DEBUG - HARDCODED

    // FIND SELECTED PROTEOME
    gb_proteine_data= find_proteine_data(m_species, m_experiment, m_proteome);
    if(!gb_proteine_data)
    {
        free(keyBuf);
        return false;
    }

    // FLUSH THE COMPLETE LIST
//     for(spot_it= m_spotList.begin(); spot_it != m_spotList.end(); ++spot_it)
//     {
//         if((*spot_it).text) free((*spot_it).text);
//         if((*spot_it).id) free((*spot_it).id);
//     }
    m_spotList.clear();

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // BROWSE ALL PROTEIN ENTRIES...
    gb_protein= GB_find(gb_proteine_data, "protein", 0, down_level);
    while(gb_protein)
    {
        // PREDEFINE SPOT
        spot.text=     NULL;
        spot.id=       NULL;
        spot.area=     0;
        spot.diameter= 8;
        spot.x=        0;
        spot.y=        0;
        spot.selected= false;
        spot.marked=   false;
        spot.gbdata=   gb_protein;

        // FETCH NECESSARY ENTRIES
        gb_protein_x=    GB_find(gb_protein, awar_protein_x, 0, down_level);
        gb_protein_y=    GB_find(gb_protein, awar_protein_y, 0, down_level);
        gb_protein_area= GB_find(gb_protein, awar_protein_area, 0, down_level);
        gb_protein_id=   GB_find(gb_protein, awar_protein_id, 0, down_level);

        // FETCH PROTEIN IDENTIFIER IF AVAILABLE
        if(gb_protein_id) spot.id= GB_read_string(gb_protein_id);

        // GET THE SELECTION CONDITION OF OUR SPOT
        if((strlen(spot.id) > 0) && !strcmp(spot.id, awar_selected_protein)) spot.selected= true;

        // GET PROTEIN MARKER CONDITION (MARKED/UNMARKED)
        spot.marked= (bool)GB_read_flag(gb_protein);

        // UPDATE OTHER VALUES (AS FAR AS AVAILABLE)
        if(gb_protein_x) spot.x= GB_read_float(gb_protein_x);
        if(gb_protein_y) spot.y= GB_read_float(gb_protein_y);
        if(gb_protein_area)
        {
            spot.area=     GB_read_int(gb_protein_area);
            spot.diameter= (int)sqrt((4 * spot.area)/(4 * 3.141592));
        }

//         if(spot.id && (strlen(spot.id) > 0))
//         {
            // CREATE KEY BUFFER
            genGeneKey(keyBuf, spot.id, spot.x, spot.y);

            // FETCH DESCRIPTOR FROM DESCRIPTOR LIST (HASH-ARRAY)
            char *descr= m_descriptorList[keyBuf];

            if(descr) spot.text= descr;
//         }

        // ADD SPOT TO LIST
        m_spotList.push_back(spot);

        // FETCH NEXT PROTEIN FROM LIST
        gb_protein= GB_find(gb_protein, "protein", 0, this_level|search_next);
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();

    // FREE KEY BUFFER
    free(keyBuf);

    return true;
}


/****************************************************************************
*  CREATE A LIST WITH PROTEIN/SPOT DESCRIPTIONS
****************************************************************************/
bool imageDialog::createDescriptions()
{
    char *token= NULL, *entry= NULL, *id= NULL;
    GBDATA *gb_proteine_data, *gb_protein, *gb_genome, *gb_gene;
    GBDATA *gb_protein_id, *gb_entry, *gb_gene_id, *gb_protein_x, *gb_protein_y;
    bool first_token;
    char *content;
    float x= 0, y= 0;

    // FIND SELECTED PROTEOME AND GENOME
    gb_proteine_data= find_proteine_data(m_species, m_experiment, m_proteome);
    if(!gb_proteine_data) return false;
    gb_genome= find_genome(m_species);
    if(!gb_genome) return false;

    // FETCH NECESSARY AWARS
    char *awar_protein_infos= get_CONFIG(CONFIG_PGT_INFO_PROTEIN);
    char *awar_gene_infos=    get_CONFIG(CONFIG_PGT_INFO_GENE);
    char *awar_protein_id=    get_CONFIG(CONFIG_PGT_ID_PROTEIN);
    char *awar_gene_id=       get_CONFIG(CONFIG_PGT_ID_GENE);
    char *awar_protein_x=     const_cast<char*>("x_coordinate"); // DEBUG - HARDCODED
    char *awar_protein_y=     const_cast<char*>("y_coordinate"); // DEBUG - HARDCODED
    if(!awar_protein_infos || !awar_gene_infos ||
       !awar_protein_id || !awar_gene_id) return false;

    // CREATE BUFFERS
    char *descriptor= (char *)malloc(1024 * sizeof(char));
    char *buf= (char *)malloc(1024 * sizeof(char));
    if(!descriptor || !buf) return false;

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // CREATE GENE IDENTIFIER AND GBDATA* HASH (AVOIDS RECURSIONS)
    gb_gene= GB_find(gb_genome, "gene", 0, down_level);
    while(gb_gene)
    {
        gb_gene_id= GB_find(gb_gene, awar_gene_id, 0, down_level);

        if(gb_gene_id)
        {
            content= GB_read_as_string(gb_gene_id);

            if(content && (strlen(content) > 0)) m_gene_GBDATA_map[content] = gb_gene;
        }

        gb_gene= GB_find(gb_gene, "gene", 0, this_level|search_next);
    }

    // BROWSE ALL PROTEIN ENTRIES...
    gb_protein= GB_find(gb_proteine_data, "protein", 0, down_level);
    while(gb_protein)
    {
        // CLEAN BUFFER
        *descriptor= 0;
        id= NULL;
        x= 0;
        y= 0;
        first_token= true;

        // FETCH PROTEIN IDENTIFIER
        gb_protein_id= GB_find(gb_protein, awar_protein_id, 0, down_level);
        gb_protein_x= GB_find(gb_protein, awar_protein_x, 0, down_level);
        gb_protein_y= GB_find(gb_protein, awar_protein_y, 0, down_level);

        // GET IDENTIFIER FIELD CONTENT
        if(gb_protein_id) id= GB_read_string(gb_protein_id);
        if(gb_protein_x) x= GB_read_float(gb_protein_x);
        if(gb_protein_y) y= GB_read_float(gb_protein_y);

//         // IF WE HAVE AN IDENTIFIER...
//         if((id && (strlen(id) > 0)) || ((x + y) != 0))
//         {
            strncpy(buf, awar_protein_infos, 1023);
            token= strtok(buf, ",; ");
            while(token)
            {
                gb_entry= GB_find(gb_protein, token, 0, down_level);

                // DO WE HAVE THE SELECTED CONTAINER?
                if(gb_entry)
                {
                    // FETCH WHATEVER IS IN THERE
                    entry= GB_read_as_string(gb_entry);

                    // APPEND THIS TO OUR STRING
                    if(entry && (strlen(entry) > 0))
                    {
                        // DO WE NEED A SEPARATOR?
                        if(first_token) first_token= false;
                        else strncat(descriptor, ";", 2);

                        // APPEND ENTRY TO DESCRIPTOR
                        strncat(descriptor, entry, 1023 - strlen(descriptor));
                    }
                }
                // FETCH NEXT TOKEN
                token= strtok(NULL, ",; ");
            }

            // FETCH GENE
            if(id)
                gb_gene= m_gene_GBDATA_map[id];
            else
                gb_gene= NULL;

            if(gb_gene)
            {
                strncpy(buf, awar_gene_infos, 1023);
                token= strtok(buf, ",; ");
                while(token)
                {
                    gb_entry= GB_find(gb_gene, token, 0, down_level);

                    // DO WE HAVE THE SELECTED CONTAINER?
                    if(gb_entry)
                    {
                        // FETCH WHATEVER IS IN THERE
                        entry= GB_read_as_string(gb_entry);

                        // APPEND THIS TO OUR STRING
                        if(entry && (strlen(entry) > 0))
                        {
                            // DO WE NEED A SEPARATOR?
                            if(first_token) first_token= false;
                            else strncat(descriptor, ";", 2);

                            // APPEND ENTRY TO DESCRIPTOR
                            strncat(descriptor, entry, 1023 - strlen(descriptor));
                        }
                    }
                    // FETCH NEXT TOKEN
                    token= strtok(NULL, ",; ");
                }
            }
//         }

        // CREATE KEY BUFFER
        genGeneKey(buf, id, x, y);

        // APPEND ENTRY TO LIST
        char *d_append= (char *)malloc((strlen(descriptor) + 1) * sizeof(char));
        char *d_id= (char *)malloc((strlen(buf) + 1) * sizeof(char));
        strcpy(d_append, descriptor);
        strcpy(d_id, buf);
        m_descriptorList[d_id] = d_append;

        // FETCH NEXT PROTEIN FROM LIST
        gb_protein= GB_find(gb_protein, "protein", 0, this_level|search_next);
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();

    free(descriptor);
    free(buf);

    return true;
}


/****************************************************************************
*  UPDATE THE SELECTED GENE AWAR (IF ENABLED)
****************************************************************************/
bool imageDialog::updateSelectedGene()
{
    char *awar_selected_protein, *awar_gene_id, *name= NULL;
    GBDATA *gb_genome, *gb_gene, *gb_name;

    // FETCH GENOME
    gb_genome= find_genome(m_species);
    if(!gb_genome) return false;

    // FETCH PROTEIN AWAR CONTENT
    awar_selected_protein= get_protein_AWAR();

    // FETCH GENE IDENTIFIER
    awar_gene_id= get_CONFIG(CONFIG_PGT_ID_GENE);

    // FETCH GENE
    gb_gene= m_gene_GBDATA_map[awar_selected_protein];

    if(gb_gene)
    {
        ARB_begin_transaction();

        gb_name= GB_find(gb_gene, "name", 0, down_level);
        name= GB_read_string(gb_name);

        ARB_commit_transaction();
    }


    if(name)
    {
        set_gene_AWAR(name);
        return true;
    }

    return false;
}


/****************************************************************************
*  TRANSFER MARKS:  SPOT-MARKER -> GENE-MARKER
****************************************************************************/
bool imageDialog::mark_Spots2Genes()
{
    list<SPOT>::iterator spot_it;
    GBDATA *gb_genome, *gb_gene, *gb_gene_id;
    char *content;

    gb_genome= find_genome(m_species);
    if(!gb_genome) return false;

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // TRAVERSE GENES
    gb_gene= GB_find(gb_genome, "gene", 0, down_level);
    while(gb_gene)
    {
        gb_gene_id= GB_find(gb_gene, "locus_tag" , 0, down_level);

        if(gb_gene_id)
        {
            content= GB_read_as_string(gb_gene_id);

            // TRAVERSE THE COMPLETE LIST
            for(spot_it= m_spotList.begin(); spot_it != m_spotList.end(); ++spot_it)
            {
                if(!strcmp((*spot_it).id, content))
                {
                    if((*spot_it).marked)
                        GB_write_flag(gb_gene, 1);
                    else
                        GB_write_flag(gb_gene, 0);
                }
            }
        }

        gb_gene= GB_find(gb_gene, "gene", 0, this_level|search_next);
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();

    return true;
}


/****************************************************************************
*  TRANSFER MARKS:  GENE-MARKER -> SPOT-MARKER
****************************************************************************/
bool imageDialog::mark_Genes2Spots()
{
    list<SPOT>::iterator spot_it;
    GBDATA *gb_genome, *gb_gene, *gb_gene_id;
    char *content;
    int flag;

    gb_genome= find_genome(m_species);
    if(!gb_genome) return false;

    // REMOVE ALL MARKER
    setAllMarker(PROTEIN_UNMARK);

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // TRAVERSE GENES
    gb_gene= GB_find(gb_genome, "gene", 0, down_level);
    while(gb_gene)
    {
        // GET ACTUAL STATE
        flag= GB_read_flag(gb_gene);

        // FETCH GENE ID
        gb_gene_id= GB_find(gb_gene, "locus_tag" , 0, down_level);

        if(gb_gene_id)
        {
            // FETCH THE GENE-ID AS STRING
            content= GB_read_as_string(gb_gene_id);

            // TRAVERSE THE COMPLETE LIST
            for(spot_it= m_spotList.begin(); spot_it != m_spotList.end(); ++spot_it)
            {
                if(!strcmp((*spot_it).id, content))
                    GB_write_flag((*spot_it).gbdata, flag);
            }
        }

        // FIND NEXT GENE
        gb_gene= GB_find(gb_gene, "gene", 0, this_level|search_next);
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();

    // CREATE A NEW SPOT LIST
    createSpotList();

    // REDRAW IMAGE
    imageRedraw();

    return true;
}


/****************************************************************************
*  COPY SPOT MARKS TO THE ARB GENE MAP
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticSpots2GenesButtonCallback(Widget, XtPointer clientData, XtPointer)
{
     // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->mark_Spots2Genes();
}


/****************************************************************************
*  COPY GENE MARKER TO SPOT VIEW
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticGenes2SpotsButtonCallback(Widget, XtPointer clientData, XtPointer)
{
     // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->mark_Genes2Spots();
}


/****************************************************************************
*  SHOW HELP DIALOG
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticHelpDialogCallback(Widget, XtPointer clientData, XtPointer)
{
    Widget shell= ((imageDialog*)clientData)->shellWidget();

    new helpDialog(shell, (imageDialog*)clientData);
}


/****************************************************************************
*  UPDATE THE STATUS BAR TEXT
****************************************************************************/
void imageDialog::updateStatusLabel()
{
    // CREATE BUFFER
    char *buf= (char *)malloc(1024 * sizeof(char));
    if(!buf) return;

    char *selected= const_cast<char*>("none");

    // FILL BUFFER STRING
    if(m_numSpots == 0)
    {
        sprintf(buf, "No spot data available");
    }
    else
    {
        if(m_selectedSpotName && (strlen(m_selectedSpotName) > 0)) selected= m_selectedSpotName;
        sprintf(buf, "Spots (total/marked/selected): %d / %d / %d  --  Selected Protein: %s  --  ",
            m_numSpots, m_numMarkedSpots, m_numSelectedSpots, selected);
    }

    // APPEND FLAGS...
    if(m_crosshairFlag) strcat(buf, " [CROSS-HAIR]");
    if(m_circleFlag) strcat(buf, " [CIRCLE]");
    if(m_labelFlag) strcat(buf, " [INFO]");
    if(m_linkedOnlyFlag) strcat(buf, " [LINKED ONLY]");
    if(m_textOnlyFlag) strcat(buf, " [ID ONLY]");
    if(m_markedOnlyFlag) strcat(buf, " [MARKED ONLY]");

    // WRITE STRING TO STATUS LABEL
    XtVaSetValues(m_statusLabel, XmNlabelString, PGT_XmStringCreateLocalized(buf), NULL);

    // FREE BUFFER
    free(buf);
}


/****************************************************************************
*  MARK ONLY SPOTS WITH INFO
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
static void staticMarkWithInfoButtonCallback(Widget, XtPointer clientData, XtPointer)
{
     // GET POINTER OF THE ORIGINAL CALLER
    imageDialog *iD= (imageDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->markWithInfo();
}


/****************************************************************************
*  MARK ONLY SPOTS WITH INFO
****************************************************************************/
void imageDialog::markWithInfo()
{
    // LOCAL VARIABLES
    GBDATA *gb_data, *gb_proteine_data, *gb_protein, *gb_protein_id;
    char *id= NULL;

    // GET MAIN ARB GBDATA
    gb_data= get_gbData();
    if(!gb_data) return;

    // FIND SELECTED PROTEOME
    gb_proteine_data= find_proteine_data(m_species, m_experiment, m_proteome);
    if(!gb_proteine_data) return;

    // FETCH PROTEIN ID AWAR
    char *awar_protein_id= get_CONFIG(CONFIG_PGT_ID_PROTEIN);

    // INIT AN ARB TRANSACTION
    ARB_begin_transaction();

    // BROWSE ALL PROTEIN ENTRIES...
    gb_protein= GB_find(gb_proteine_data, "protein", 0, down_level);
    while(gb_protein)
    {
        gb_protein_id=  GB_find(gb_protein, awar_protein_id, 0, down_level);

        // FETCH PROTEIN IDENTIFIER IF AVAILABLE
        if(gb_protein_id) id= GB_read_string(gb_protein_id);

        // MARK, IF WE HAVE AN IDENTIFIER
        if(strlen(id) > 0) GB_write_flag(gb_protein, 1);
        else GB_write_flag(gb_protein, 0);

        // FETCH NEXT PROTEIN FROM LIST
        gb_protein= GB_find(gb_protein, "protein", 0, this_level|search_next);
    }

    // CLOSE THE ARB TRANSACTION
    ARB_commit_transaction();

    // CREATE A NEW SPOT LIST
    createSpotList();

    // REDRAW IMAGE
    imageRedraw();
}
