// COPYRIGHT (C) 2004 - 2005 KAI BADER <BADERK@IN.TUM.DE>,
// DEPARTMENT OF MICROBIOLOGY (TECHNICAL UNIVERSITY MUNICH)
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "arb_interface.hxx"
#include "main_dialog.hxx"
#include "analyze_window.hxx"
#include "config_dialog.hxx"
#include "msgbox.hxx"


/****************************************************************************
*  MAIN DIALOG - CONSTRUCTOR
****************************************************************************/
mainDialog::mainDialog(Widget p)
    : MDialog(p, NULL)
{
    // SHOW DIALOG
    createShell("");

    // CALL CREATE WINDOW FUNCTION
    createWindow();

    // SET WINDOW WIDTH
    XtVaSetValues(m_shell,
        XmNwidth, 330,
        NULL);

    // REALIZE WINDOW
    realizeShell();

    setWindowName("PGT - Main Selector");

    // UPDATE THE LIST ENTRIES
    updateListEntries();

    // ADD ARB AWAR CALLBACKS
    add_mainDialog_callback(AWAR_SPECIES_NAME,    static_main_ARB_callback, this);
    add_mainDialog_callback(AWAR_EXPERIMENT_NAME, static_main_ARB_callback, this);
    add_mainDialog_callback(AWAR_PROTEOM_NAME,    static_main_ARB_callback, this);
    add_mainDialog_callback(AWAR_PROTEIN_NAME,    static_main_ARB_callback, this);
    
    // add_species_callback(static_main_ARB_callback, this);
    // add_experiment_callback(static_main_ARB_callback, this);
    // add_proteom_callback(static_main_ARB_callback, this);
    // add_protein_callback(static_main_ARB_callback, this);
}


/****************************************************************************
*  MAIN DIALOG - CREATE WINDOW
****************************************************************************/
void mainDialog::createWindow()
{
    // CREATE TOP LEVEL WIDGET
    m_top= XtVaCreateManagedWidget("top",
        xmRowColumnWidgetClass, m_shell,
        XmNorientation, XmVERTICAL,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        NULL);

    // CREATE TOOLBAR
    createToolbar();

    // CREATE MAIN AREA (WORKING AREA)
    createMainArea();

}

/****************************************************************************
*  MAIN DIALOG - CREATE TOOLBAR
****************************************************************************/
void mainDialog::createToolbar()
{
    // CREATE MANAGER WIDGET
    Widget manager= XtVaCreateManagedWidget("toolbar",
        xmRowColumnWidgetClass, m_top,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        XmNorientation, XmHORIZONTAL,
        NULL);

    // GET FOREGROUND AND BACKGROUND PIXEL COLORS
    Pixel fg, bg;
    XtVaGetValues(manager, XmNforeground, &fg, XmNbackground, &bg, NULL);

    // OPEN AND IMPORT XPM IMAGES (BUTTON LOGOS)
    Pixmap analyze_xpm, exit_xpm, import_xpm, visualize_xpm, info_xpm, config_xpm, pgtinfo_xpm;
    //
    Screen *s     = XtScreen(manager);
    analyze_xpm   = PGT_LoadPixmap("analyze.xpm", s, fg, bg);
    exit_xpm      = PGT_LoadPixmap("exit.xpm", s, fg, bg);
    import_xpm    = PGT_LoadPixmap("import.xpm", s, fg, bg);
    visualize_xpm = PGT_LoadPixmap("visualize.xpm", s, fg, bg);
    info_xpm      = PGT_LoadPixmap("proteininfo.xpm", s, fg, bg);
    config_xpm    = PGT_LoadPixmap("config.xpm", s, fg, bg);
    pgtinfo_xpm   = PGT_LoadPixmap("info.xpm", s, fg, bg);

    // CREATE BUTTON: IMPORT
    Widget importButton= XtVaCreateManagedWidget("importbtn",
        xmPushButtonWidgetClass, manager,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, import_xpm,
        NULL);
    XtAddCallback(importButton, XmNactivateCallback, staticOpenImportCallback, this);

    // CREATE BUTTON: VISUALIZE
    Widget imageButton= XtVaCreateManagedWidget("visualizebtn",
        xmPushButtonWidgetClass, manager,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, visualize_xpm,
        NULL);
    XtAddCallback(imageButton, XmNactivateCallback, staticOpenImageCallback, this);

    // CREATE BUTTON: ANALYZE
    Widget analyzeButton= XtVaCreateManagedWidget("analyzebtn",
        xmPushButtonWidgetClass, manager,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, analyze_xpm,
        NULL);
    XtAddCallback(analyzeButton, XmNactivateCallback, staticOpenAnalyzeCallback, this);

    // CREATE BUTTON: INFO
    Widget infoButton= XtVaCreateManagedWidget("infobtn",
        xmPushButtonWidgetClass, manager,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, info_xpm,
        NULL);
    XtAddCallback(infoButton, XmNactivateCallback, staticInfoCallback, this);

    // CREATE BUTTON: CONFIG
    Widget configButton= XtVaCreateManagedWidget("configbtn",
        xmPushButtonWidgetClass, manager,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, config_xpm,
        NULL);
    XtAddCallback(configButton, XmNactivateCallback, staticConfigCallback, this);

    // CREATE BUTTON: PGT INFO
    Widget pgtinfobutton= XtVaCreateManagedWidget("pgtinfobtn",
        xmPushButtonWidgetClass, manager,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, pgtinfo_xpm,
        NULL);
    XtAddCallback(pgtinfobutton, XmNactivateCallback, staticPGTInfoCallback, this);

    // CREATE BUTTON: EXIT
    Widget exitbutton= XtVaCreateManagedWidget("exitbtn",
        xmPushButtonWidgetClass, manager,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, exit_xpm,
        NULL);
    XtAddCallback(exitbutton, XmNactivateCallback, staticExitCallback, this);

    // CREATE HORIZONTAL SEPARATOR WIDGET
    XtVaCreateManagedWidget("separator",
        xmSeparatorWidgetClass, m_top,
        XmNorientation, XmHORIZONTAL,
        NULL);
}


/****************************************************************************
*  MAIN DIALOG - CREATE MAIN AREA
****************************************************************************/
void mainDialog::createMainArea()
{
    // CREATE STRINGS
    XmString str_01= PGT_XmStringCreateLocalized("Selected Species");
    XmString str_02= PGT_XmStringCreateLocalized("Selected Experiment");
    XmString str_03= PGT_XmStringCreateLocalized("Selected Proteome");
    XmString str_04= PGT_XmStringCreateLocalized("Selected Protein");

    // CREATE TOP LEVEL WIDGET
    m_selection_area= XtVaCreateManagedWidget("top",
        xmFormWidgetClass, m_top,
        XmNorientation, XmVERTICAL,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        NULL);

    // CREATE SPECIES LABEL
    Widget species_label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_selection_area,
        XmNlabelString, str_01,
        XmNwidth, 200,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE SPECIES TEXT FIELD
    m_speciesText= XtVaCreateManagedWidget("speciesText",
        xmTextWidgetClass, m_selection_area,
        XmNeditable, false,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, species_label,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE EXPERIMENT LABEL
    Widget experiment_label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_selection_area,
        XmNlabelString, str_02,
        XmNwidth, 200,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_speciesText,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE EXPERIMENT TEXT FIELD
    m_experimentText= XtVaCreateManagedWidget("experimentText",
        xmTextWidgetClass, m_selection_area,
        XmNeditable, false,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, experiment_label,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE PROTEOME LABEL
    Widget proteome_label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_selection_area,
        XmNlabelString, str_03,
        XmNwidth, 200,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_experimentText,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE PROTEOME TEXT FIELD
    m_proteomeText= XtVaCreateManagedWidget("proteomeText",
        xmTextWidgetClass, m_selection_area,
        XmNeditable, false,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, proteome_label,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE PROTEIN LABEL
    Widget protein_label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_selection_area,
        XmNlabelString, str_04,
        XmNwidth, 200,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_proteomeText,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE PROTEIN TEXT FIELD
    m_proteinText= XtVaCreateManagedWidget("proteinText",
        xmTextWidgetClass, m_selection_area,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, protein_label,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // FREE CREATED STRINGS
    XmStringFree(str_01);
    XmStringFree(str_02);
    XmStringFree(str_03);
    XmStringFree(str_04);
}


/****************************************************************************
*  MAIN DIALOG - UPDATE THE LIST ENTRIES
****************************************************************************/
void mainDialog::updateListEntries()
{
    // GET AWAR CONTENT
    char *species_AWAR=    get_species_AWAR();
    char *experiment_AWAR= get_experiment_AWAR();
    char *proteome_AWAR=   get_proteom_AWAR();
    char *protein_AWAR=    get_protein_AWAR();

    // SET DEFAULT CONTENT IF AWAR IS EMPTY
    if(strlen(species_AWAR) == 0) species_AWAR= strdup("no selected species");
    if(strlen(experiment_AWAR) == 0) experiment_AWAR= strdup("no selected experiment");
    if(strlen(proteome_AWAR) == 0) proteome_AWAR= strdup("no selected proteome");
    if(strlen(protein_AWAR) == 0) protein_AWAR= strdup("no selected protein");

    // SET TEXT FIELD ENTRIES
    XtVaSetValues(m_speciesText, XmNvalue, species_AWAR, NULL);
    XtVaSetValues(m_experimentText, XmNvalue, experiment_AWAR, NULL);
    XtVaSetValues(m_proteomeText, XmNvalue, proteome_AWAR, NULL);
    XtVaSetValues(m_proteinText, XmNvalue, protein_AWAR, NULL);

    free(protein_AWAR);
    free(proteome_AWAR);
    free(experiment_AWAR);
    free(species_AWAR);
}


/****************************************************************************
*  ARB AWAR CALLBACK - PROTEIN ENTRY HAS CHANGED
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void static_main_ARB_callback(GBDATA *, mainDialog *mD, GB_CB_TYPE)
{
     // // GET POINTER OF THE ORIGINAL CALLER
    // mainDialog *mD= (mainDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    mD->ARB_callback();
}


/****************************************************************************
*  ARB AWAR CALLBACK - PROTEIN ENTRY HAS CHANGED
****************************************************************************/
void mainDialog::ARB_callback()
{
    updateListEntries();
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - OPEN IMPORT WINDOW
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticOpenImportCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    mainDialog *mD= (mainDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    mD->openImportCallback(widget, callData);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - OPEN IMPORT WINDOW
****************************************************************************/
void mainDialog::openImportCallback(Widget, XtPointer)
{
    // OPEN A NEW IMPORT DIALOG, GIVE OUT SHELL WIDGET AS PARENT WIDGET
    m_importDialog= new importDialog(m_shell, this);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - SHOW (TIFF)IMAGE VIEW WINDOW
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticOpenImageCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    mainDialog *mD= (mainDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    mD->openImageCallback(widget, callData);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - SHOW (TIFF)IMAGE VIEW WINDOW
****************************************************************************/
void mainDialog::openImageCallback(Widget, XtPointer)
{
    new imageDialog(m_shell, this);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - SHOW ANALYZE WINDOW
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticOpenAnalyzeCallback(Widget widget, XtPointer clientData, XtPointer callData)
{// Department of Microbiology (Technical University Munich)

    // GET POINTER OF THE ORIGINAL CALLER
    mainDialog *mD= (mainDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    mD->openAnalyzeCallback(widget, callData);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - SHOW ANALYZE WINDOW
****************************************************************************/
void mainDialog::openAnalyzeCallback(Widget, XtPointer)
{
//     new analyzeWindow(m_shell, this);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - CONFIG CALL
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticConfigCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    mainDialog *mD= (mainDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    mD->configCallback(widget, callData);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - CONFIG CALL
****************************************************************************/
void mainDialog::configCallback(Widget, XtPointer)
{
    new configDialog(this->m_shell, this);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - INFO CALL
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticInfoCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    mainDialog *mD= (mainDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    mD->infoCallback(widget, callData);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - INFO CALL
****************************************************************************/
void mainDialog::infoCallback(Widget, XtPointer)
{
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - EXIT CALL
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticExitCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    mainDialog *mD= (mainDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    mD->exitCallback(widget, callData);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - EXIT CALL
****************************************************************************/
void mainDialog::exitCallback(Widget widget, XtPointer)
{
    int answer= OkCancelDialog(widget, "Exit PGT", "Do you really want to exit?");
    if (answer == 1)
        closeDialog();

//     closeDialog();
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - PGT INFO CALL
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticPGTInfoCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    mainDialog *mD= (mainDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    mD->PGTinfoCallback(widget, callData);
}


/****************************************************************************
*  MAIN DIALOG CALLBACK - PGT INFO CALL
****************************************************************************/
void mainDialog::PGTinfoCallback(Widget widget, XtPointer)
{
    // PGT INFORMATION STRING:
    const char *pgtinfo=
        "PGT - Proteome and Genome Toolkit\n"
        "Version 0.2.0 (beta stage)\n\n"
        "(c) Copyright 2004-2005 Kai Bader <baderk@in.tum.de>,\n"
        "Department of Microbiology (Technical University Munich)\n\n"
        "The software is provided \"as is\", without warranty of any kind, express or\n"
        "implied, including but not limited to the warranties of merchantability,\n"
        "fitness for a particular purpose and noninfringement. In no event shall\n"
        "the author be liable for any claim, damages or other liability, whether\n"
        "in an action of contract, tort or otherwise, arising from, out of or in\n"
        "connection with the software or the use or other dealings in the software.\n\n"
        "You have the right to use this version of PGT for free. All interested\n"
        "parties may redistribute and modify PGT as long as all copies are accompanied\n"
        "by this license information and all copyright notices remain intact. Parties\n"
        "redistributing PGT must do so on a non-profit basis, charging only for cost\n"
        "of media or distribution.\n";

    // SHOW PGT INFO MESSAGEBOX
    ShowMessageBox(widget, "PGT Information", pgtinfo);
}
