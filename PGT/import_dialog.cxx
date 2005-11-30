// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$


#include "import_dialog.hxx"
#include "selection_dialog.hxx"
#include "entry_sel_dialog.hxx"
#include "arb_interface.hxx"


// CALLBACK WRAPPER FUNCTIONS (STATIC)
void staticGetFilenameCallback(Widget, XtPointer, XtPointer);
void staticFileDialogCloseCallback(Widget parent, XtPointer, XtPointer);
void staticFileDialogCallback(Widget parent, XtPointer, XtPointer call_data);
void staticARBdestinationCallback(Widget, XtPointer, XtPointer);
void staticSpeciesChangedCallback(Widget, XtPointer, XtPointer);
void staticExperimentChangedCallback(Widget, XtPointer, XtPointer);
void staticProteomeChangedCallback(Widget, XtPointer, XtPointer);
void staticClearEntriesButtonCallback(Widget, XtPointer, XtPointer);
void staticOpenFileButtonCallback(Widget, XtPointer, XtPointer);
void staticColumnDownCallback(Widget, XtPointer, XtPointer);
void staticColumnUpCallback(Widget, XtPointer, XtPointer);
void staticHeaderChangedCallback(Widget, XtPointer, XtPointer);
void staticResetHeaderButtonCallback(Widget, XtPointer, XtPointer);
void staticImportDataCallback(Widget, XtPointer, XtPointer);
void staticChangedDatatypeCallback(Widget, XtPointer, XtPointer);
void staticSampleListEntryCallback(Widget, XtPointer, XtPointer);
void staticExternHeaderSelectionCallback(Widget, XtPointer, XtPointer);
void staticExternHeaderChangedCallback(Widget, XtPointer, XtPointer);


/****************************************************************************
*  MAIN DIALOG - CONSTRUCTOR
****************************************************************************/
importDialog::importDialog(MDialog *d)
    : MDialog(d)
{
    // PRESET CLASS VARIABLES
    m_species= get_species_AWAR();
    m_experiment= get_experiment_AWAR();
    m_proteome= get_proteom_AWAR();
    //
    m_table= NULL;
    m_xslt= NULL;

    m_hasFileDialog= false;
    m_hasSelectionDialog= false;
    m_hasTableData = false;

    // CREATE TYPE STRINGS
    m_str_int=    CreateDlgString("INT");
    m_str_float=  CreateDlgString("FLOAT");
    m_str_string= CreateDlgString("STRING");

    // SHOW DIALOG
    createShell("");

    // CALL CREATE WINDOW FUNCTION
    createWindow();

    // SET WINDOW WIDTH
    XtVaSetValues(m_shell,
        XmNwidth, 600,
        NULL);

    // REALIZE WINDOW
    realizeShell();

    // SET WINDOW LABEL
    setDialogTitle("PGT - Proteome Data Importer");
}


/****************************************************************************
*  MAIN DIALOG - DESTRUCTOR
****************************************************************************/
importDialog::~importDialog()
{
    // FREE ENTRIES
    if(m_species) free(m_species);
    if(m_experiment) free(m_experiment);
    if(m_proteome) free(m_proteome);
    if(m_table) free(m_table);
    if(m_xslt) free(m_xslt);
}


/****************************************************************************
*  MAIN DIALOG - CREATE WINDOW
****************************************************************************/
void importDialog::createWindow()
{
    // CREATE TOP LEVEL WIDGET
    m_top= XtVaCreateManagedWidget("top",
        xmRowColumnWidgetClass, m_shell,
        XmNorientation, XmVERTICAL,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        NULL);

    // CREATE SELECTION AREA
    createSelectionArea();

    // CREATE HORIZONTAL SEPARATOR WIDGET
    XtVaCreateManagedWidget("separator",
        xmSeparatorWidgetClass, m_top,
        XmNorientation, XmHORIZONTAL,
        NULL);

    // CREATE IMPORT AREA
    createImportArea();

    // FILL THE IMPORT TYPE COMBOBOX
    fillImportTypeCombobox();

    // UPDATE ARB ENTRY FIELD
    updateARBText();
}


/****************************************************************************
*  MAIN DIALOG - CREATE WINDOW
****************************************************************************/
void importDialog::createSelectionArea()
{
    // CREATE STRINGS
    XmString str_01= CreateDlgString("...");
    XmString str_02= CreateDlgString("Source File:");
    XmString str_03= CreateDlgString("ARB Destination:");
    XmString str_04= CreateDlgString("Import Filter:");
    XmString str_05= CreateDlgString("Clear Entries");
    XmString str_06= CreateDlgString("Open File");

    // CREATE LOCAL MANAGER WIDGET
    Widget manager= XtVaCreateManagedWidget("mainarea",
        xmFormWidgetClass, m_top,
        // XmNmarginHeight, 0,
        // XmNmarginWidth, 0,
        NULL);

    // CREATE FILENAME SELECTION BUTTON
    Widget fileButtonWidget= XtVaCreateManagedWidget("fileselectbutton",
        xmPushButtonWidgetClass, manager,
        XmNlabelString, str_01,
        XmNwidth, 40,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(fileButtonWidget, XmNactivateCallback, staticGetFilenameCallback, this);

    // CREATE DATASET SELECTION BUTTON
    Widget datasetButtonWidget= XtVaCreateManagedWidget("datasetbuttonwidget",
        xmPushButtonWidgetClass, manager,
        XmNlabelString, str_01,
        XmNwidth, 40,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, fileButtonWidget,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(datasetButtonWidget, XmNactivateCallback, staticARBdestinationCallback, this);

    // CREATE FIRST ROW LABEL
    Widget upperLabel= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, manager,
        XmNlabelString, str_02,
        XmNwidth, 110,
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    // CREATE SECOND ROW LABEL
    Widget lowerLabel= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, manager,
        XmNlabelString, str_03,
        XmNwidth, 110,
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, upperLabel,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    // CREATE THIRD ROW LABEL
    Widget typeLabel= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, manager,
        XmNlabelString, str_04,
        XmNwidth, 110,
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, lowerLabel,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    // CREATE FILENAME TEXT FIELD
    m_filenameWidget= XtVaCreateManagedWidget("filenamewidget",
        xmTextWidgetClass, manager,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_WIDGET,
        XmNrightWidget, fileButtonWidget,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, upperLabel,
        NULL);

    // CREATE ARB DATASET TEXT FIELD
    m_datasetWidget= XtVaCreateManagedWidget("arbdatasetwidget",
        xmTextWidgetClass, manager,
        XmNeditable, false,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_filenameWidget,
        XmNrightAttachment, XmATTACH_WIDGET,
        XmNrightWidget, datasetButtonWidget,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, lowerLabel,
        NULL);

    // CREATE COMBOBOX FOR FILE TYPE SELECTION
    m_fileTypeWidget= XtVaCreateManagedWidget("filetypecombobox",
        xmComboBoxWidgetClass, manager,
        XmNcomboBoxType, XmDROP_DOWN_COMBO_BOX,
        XmNmarginWidth, 1,
        XmNmarginHeight, 1,
        // XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_datasetWidget,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, typeLabel,
        NULL);

    // CREATE RESET BUTTON
    Widget resetButtonWidget= XtVaCreateManagedWidget("resetbutton",
        xmPushButtonWidgetClass, manager,
        XmNlabelString, str_05,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, datasetButtonWidget,
        // XmNbottomAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
//         XmNbottomAttachment, XmATTACH_FORM,
//         XmNrightAttachment, XmATTACH_WIDGET,
//         XmNrightWidget, openButtonWidget,
        XmNwidth, 100,
        XmNheight, 30,
        NULL);
    XtAddCallback(resetButtonWidget, XmNactivateCallback, staticClearEntriesButtonCallback, this);

    // CREATE IMPORT BUTTON
    Widget openButtonWidget= XtVaCreateManagedWidget("openbutton",
            xmPushButtonWidgetClass, manager,
            XmNlabelString, str_06,
            XmNtopAttachment, XmATTACH_WIDGET,
            XmNtopWidget, resetButtonWidget,
            XmNbottomAttachment, XmATTACH_FORM,
            XmNrightAttachment, XmATTACH_FORM,
//             XmNbottomAttachment, XmATTACH_FORM,
//             XmNrightAttachment, XmATTACH_WIDGET,
//             XmNrightWidget, openButtonWidget,
            XmNwidth, 100,
            XmNheight, 30,
            NULL);
    XtAddCallback(openButtonWidget, XmNactivateCallback, staticOpenFileButtonCallback, this);
}


/****************************************************************************
*  MAIN DIALOG - CREATE IMPORT AREA
****************************************************************************/
void importDialog::createImportArea()
{
    // CREATE STRINGS
    XmString str_01= CreateDlgString("Select Data Column:");
    XmString str_02= CreateDlgString("New ARB Container Type:");
    XmString str_03= CreateDlgString("Original Column Header:");
    XmString str_04= CreateDlgString("New ARB Container Name:");
    XmString str_05= CreateDlgString("ARB entry IDs");
    XmString str_06= CreateDlgString("Import -> ARB");
    XmString str_07= CreateDlgString("New Proteome Content:");
    XmString str_08= CreateDlgString("Reset Names");

    // CREATE LOCAL MANAGER WIDGET
    Widget manager= XtVaCreateManagedWidget("importarea",
        xmFormWidgetClass, m_top,
        // XmNmarginHeight, 0,
        // XmNmarginWidth, 0,
        NULL);

    // CREATE LABEL
    Widget typeLabel= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, manager,
        XmNlabelString, str_01,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_FORM,
        XmNwidth, 150,
        XmNheight, 30,
        NULL);

    // CREATE LABEL
    Widget columnTypeLabel= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, manager,
        XmNlabelString, str_02,
        XmNwidth, 150,
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, typeLabel,
        NULL);

    // CREATE LABEL
    Widget columnLabel= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, manager,
        XmNlabelString, str_03,
        XmNwidth, 150,
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, columnTypeLabel,
        NULL);

    // CREATE LABEL
    Widget containerLabel= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, manager,
        XmNlabelString, str_04,
        XmNwidth, 150,
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, columnLabel,
        NULL);

    // CREATE LEFT ARROW WIDGET
    Widget selector_l= XtVaCreateManagedWidget("arrowleft",
        xmArrowButtonGadgetClass, manager,
        XmNarrowDirection, XmARROW_LEFT,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, typeLabel,
        XmNwidth, 24,
        XmNheight, 30,
        NULL);
    XtAddCallback(selector_l, XmNactivateCallback, staticColumnDownCallback, this);

    // CREATE RIGHT ARROW WIDGET
    Widget selector_r= XtVaCreateManagedWidget("arrowright",
        xmArrowButtonGadgetClass, manager,
        XmNarrowDirection, XmARROW_RIGHT,
        XmNtopAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNwidth, 24,
        XmNheight, 30,
        NULL);
    XtAddCallback(selector_r, XmNactivateCallback, staticColumnUpCallback, this);

    // CREATE COLUMN SELECTOR TEXT FIELD
    m_selectorText= XtVaCreateManagedWidget("selectortext",
        xmTextWidgetClass, manager,
        XmNeditable, false,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, selector_l,
        XmNrightAttachment, XmATTACH_WIDGET,
        XmNrightWidget, selector_r,
        XmNheight, 30,
        NULL);

    // CREATE COLUMN TYPE COMBOBOX WIDGET
    m_fieldTypeWidget= XtVaCreateManagedWidget("fieldtypecombobox",
        xmComboBoxWidgetClass, manager,
        XmNcomboBoxType, XmDROP_DOWN_COMBO_BOX,
        XmNmarginWidth, 1,
        XmNmarginHeight, 1,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, columnTypeLabel,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_selectorText,
        NULL);
    XtAddCallback(m_fieldTypeWidget, XmNselectionCallback, staticChangedDatatypeCallback, this);

    // FILL THE COLUMN TYPE COMBOBOX
    fillARBTypeCombobox();

    // CREATE COLUMN HEADER TEXT FIELD
    m_columnHeaderWidget= XtVaCreateManagedWidget("columnHeaderText",
        xmTextWidgetClass, manager,
        XmNeditable, false,
        XmNheight, 30,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, columnLabel,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_fieldTypeWidget,
        NULL);

    // CREATE COLUMN HEADER TEXT FIELD
    m_containerHeaderWidget= XtVaCreateManagedWidget("containerHeaderText",
        xmTextWidgetClass, manager,
        XmNheight, 30,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, containerLabel,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_columnHeaderWidget,
        NULL);
    XtAddCallback(m_containerHeaderWidget, XmNactivateCallback, staticHeaderChangedCallback, this);

    // CREATE RESET NAMES BUTTON
    Widget namesButtonWidget= XtVaCreateManagedWidget("namesbutton",
        xmPushButtonWidgetClass, manager,
        XmNlabelString, str_08,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_containerHeaderWidget,
        XmNrightAttachment, XmATTACH_FORM,
        XmNwidth, 100,
        XmNheight, 30,
        NULL);
    XtAddCallback(namesButtonWidget, XmNactivateCallback, staticResetHeaderButtonCallback, this);

    // CREATE ARB NAMES BUTTON
    Widget ARBnamesButtonWidget= XtVaCreateManagedWidget("ARBnamesButton",
            xmPushButtonWidgetClass, manager,
            XmNlabelString, str_05,
            XmNtopAttachment, XmATTACH_WIDGET,
            XmNtopWidget, m_containerHeaderWidget,
            XmNrightAttachment, XmATTACH_WIDGET,
            XmNrightWidget, namesButtonWidget,
            XmNwidth, 100,
            XmNheight, 30,
            NULL);
    XtAddCallback(ARBnamesButtonWidget, XmNactivateCallback, staticExternHeaderSelectionCallback, this);

    // CREATE IMPORT BUTTON
    Widget importButtonWidget= XtVaCreateManagedWidget("importbutton",
            xmPushButtonWidgetClass, manager,
            XmNlabelString, str_06,
            XmNtopAttachment, XmATTACH_WIDGET,
            XmNtopWidget, namesButtonWidget,
            XmNrightAttachment, XmATTACH_FORM,
            XmNwidth, 100,
            XmNheight, 30,
            NULL);
    XtAddCallback(importButtonWidget, XmNactivateCallback, staticImportDataCallback, this);

    // CREATE LABEL
    XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, str_07,
        // XmNwidth, 160,
        // XmNheight, 30,
        NULL);

    // SAMPLE ARB CONTAINER ENTRIES LIST
    m_sampleList= XmCreateScrolledList(m_top, const_cast<char*>("sampleList"), NULL, 0);
    XtVaSetValues(m_sampleList,
        XmNmarginWidth, 1,
        XmNmarginHeight, 1,
        // XmNallowResize, true,
        XmNvisibleItemCount, 10,
        // XmNselectionPolicy, XmSINGLE_SELECT,
        NULL);
    XtManageChild(m_sampleList);
    XtAddCallback(m_sampleList, XmNbrowseSelectionCallback, staticSampleListEntryCallback, this);

//     // SAMPLE SCALE (FOR SELECTING THE SAMPLE ENTRY
//     m_sampleScale= XtVaCreateManagedWidget("sampleScale",
//         xmScaleWidgetClass, m_top,
//         XmNtitleString,   CreateDlgString("Entry"),
//         XmNorientation,    XmHORIZONTAL,
//         XmNmaximum,       10,
//         XmNdecimalPoints, 0,
//         XmNshowValue,     True,
//         // XmNwidth,         200,
//         // XmNheight,        100,
//         NULL);
}


/****************************************************************************
*  MAIN DIALOG - SET SPECIES NAME
****************************************************************************/
void importDialog::setSpecies(char *species)
{
    m_species= (char *)malloc((strlen(species) +1)*sizeof(char));
    strcpy(m_species, species);

    // set_species_AWAR(m_species);
}


/****************************************************************************
*  MAIN DIALOG - SET EXPERIMENT NAME
****************************************************************************/
void importDialog::setExperiment(char *experiment)
{
    m_experiment= (char *)malloc((strlen(experiment) +1)*sizeof(char));
    strcpy(m_experiment, experiment);

    // set_experiment_AWAR(m_experiment);
}


/****************************************************************************
*  MAIN DIALOG - SET PROTEOME NAME
****************************************************************************/
void importDialog::setProteome(char *proteome)
{
    m_proteome= (char *)malloc((strlen(proteome) +1)*sizeof(char));
    strcpy(m_proteome, proteome);

    // set_proteom_AWAR(m_proteome);
}


/****************************************************************************
*  MAIN DIALOG - GET FILENAME CALLBACK
****************************************************************************/
void importDialog::getFilenameCallback(Widget, XtPointer)
{
    if (!m_hasFileDialog)
    {
        m_fileDialog= XmCreateFileSelectionDialog(m_shell, const_cast<char*>("importDialog"), NULL, 0);
        XtAddCallback(m_fileDialog, XmNokCallback, staticFileDialogCallback, this);
        XtAddCallback(m_fileDialog, XmNcancelCallback, staticFileDialogCloseCallback, this);
        XtAddCallback(m_fileDialog, XmNnoMatchCallback, staticFileDialogCloseCallback, this);
        XtSetSensitive(XmFileSelectionBoxGetChild(m_fileDialog, XmDIALOG_HELP_BUTTON), False);

        XmString str_open= CreateDlgString("Open proteome data file...");
        XtVaSetValues(m_fileDialog, XmNdialogTitle, str_open, NULL);

        m_hasFileDialog= true;
    }

    XtManageChild(m_fileDialog);
    XtPopup(XtParent(m_fileDialog), XtGrabNone);
}


/****************************************************************************
*  MAIN DIALOG - CET FILENAME CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticGetFilenameCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->getFilenameCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - IMPORT FILE DIALOG VALUE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticFileDialogCloseCallback(Widget parent, XtPointer, XtPointer)
{
    // CLOSE FILE OPEN DIALOG
    XtUnmanageChild(parent);
}


/****************************************************************************
*  CALLBACK - IMPORT FILE DIALOG VALUE
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticFileDialogCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->fileDialogCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - IMPORT FILE DIALOG VALUE
****************************************************************************/
void importDialog::fileDialogCallback(Widget widget, XtPointer callData)
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
    XtVaSetValues(m_filenameWidget, XmNvalue, str, NULL);
}


/****************************************************************************
*  CALLBACK - SELECT ARB DESTINATION
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticARBdestinationCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->ARBdestinationCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - SELECT ARB DESTINATION
****************************************************************************/
void importDialog::ARBdestinationCallback(Widget, XtPointer)
{
    selectionDialog *sD= new selectionDialog(this, SELECTION_DIALOG_WRITE);

    // SET SPECIES CALLBACK
    sD->setSpeciesCallback(staticSpeciesChangedCallback);

    // SET EXPERIMENT CALLBACK
    sD->setExperimentCallback(staticExperimentChangedCallback);

    // SET PROTEOME CALLBACK
    sD->setProteomeCallback(staticProteomeChangedCallback);


    //     new selectionDialog(m_shell, &m_species, &m_experiment,
    //         &m_proteome, staticDataChangedCallback);
}


/****************************************************************************
*  CALLBACK - DATA CHANGED CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticSpeciesChangedCallback(Widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // GET SPECIES NAME FROM CALLDATA
    char *name= *((char **)callData);

    // SET NEW NAMES
    iD->setSpecies(name);
    iD->setExperiment(const_cast<char*>(""));
    iD->setProteome(const_cast<char*>(""));

    // UPDATE OUTPUT
    iD->updateARBText();
}


/****************************************************************************
*  CALLBACK - DATA CHANGED CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticExperimentChangedCallback(Widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // GET EXPERIMENT NAME FROM CALLDATA
    char *name= *((char **)callData);

    // SET NEW NAMES
    iD->setExperiment(name);
    iD->setProteome(const_cast<char*>(""));

    // UPDATE OUTPUT
    iD->updateARBText();
}


/****************************************************************************
*  CALLBACK - DATA CHANGED CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticProteomeChangedCallback(Widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // GET PROTEOME NAME FROM CALLDATA
    char *name= *((char **)callData);

    // SET NEW NAMES
    iD->setProteome(name);

    // UPDATE OUTPUT
    iD->updateARBText();
}


/****************************************************************************
*  IMPORT DIALOG - UPDATE THE ARB TEXT FIELD ENTRY
*  (SHOW SELECTED SPECIES/EXPERIMENT/PROTEOME)
****************************************************************************/
void importDialog::updateARBText()
{
    // CREATE BUFFER
    char *buf= (char *)malloc(1024 * sizeof(char));
    char *sp=   const_cast<char*>("no species");
    char *exp=  const_cast<char*>("no experiment");
    char *prot= const_cast<char*>("no proteome");

    // USE REAL STRINGS IF AVAILABLE
    if(m_species && (strlen(m_species))) sp= m_species;
    if(m_experiment && (strlen(m_experiment))) exp= m_experiment;
    if(m_proteome && (strlen(m_proteome))) prot= m_proteome;

    // FILL BUFFER STRING
    sprintf(buf, "[%s] -> [%s] -> [%s]", sp, exp, prot);

    // COPY STRING TO TEXT FIELD
    XtVaSetValues(m_datasetWidget, XmNvalue, buf, NULL);

    // FREE BUFFER
    free(buf);
}


/****************************************************************************
*  IMPORT DIALOG - FILL THE IMPORT FILTER COMBOBOX
****************************************************************************/
void importDialog::fillImportTypeCombobox()
{
    // NUMBER OF ENTRIES COUNTER
    int pos= 0;

    // ADD CSV ENTRY TO THE COMBOBOX
    XmString str_csv= CreateDlgString("CSV");
    XmComboBoxAddItem(m_fileTypeWidget, str_csv, pos, true); pos++;

    // ADD XSLT IMPORT FILTER (FOR XML FILES)
    m_xslt= findXSLTFiles(const_cast<char*>("xslt"));

    if(m_xslt)
    {
        for(int i= 0; i < m_xslt->number; i++)
        {
            XmString str_import= CreateDlgString(m_xslt->importer[i]);
            XmComboBoxAddItem(m_fileTypeWidget, str_import, pos, true); pos++;
        }
    }

    // SET COMBOBOX LIST LENGTH TO <= 5
    int visible= 5;
    if(pos < visible) visible= pos;
    XtVaSetValues(m_fileTypeWidget,
        XmNselectedPosition, (pos - 1),
        XmNvisibleItemCount, visible,
        NULL);
}


/****************************************************************************
*  IMPORT DIALOG - FILL THE ARB ENTRY TYPE COMBOBOX
****************************************************************************/
void importDialog::fillARBTypeCombobox()
{
    // ADD ITEMS
    int pos= 0;
    XmComboBoxAddItem(m_fieldTypeWidget, m_str_int, pos, true); pos++;
    XmComboBoxAddItem(m_fieldTypeWidget, m_str_float, pos, true); pos++;
    XmComboBoxAddItem(m_fieldTypeWidget, m_str_string, pos, true); pos++;

    // SET COMBOBOX LIST LENGTH TO <= 5
    int visible= 5;
    if(pos < visible) visible= pos;
    XtVaSetValues(m_fieldTypeWidget,
        XmNselectedPosition, (pos - 1),
        XmNvisibleItemCount, visible,
        NULL);
}


/****************************************************************************
*  IMPORT DIALOG - CLEAR ENTRIES BUTTON
****************************************************************************/
void importDialog::clearEntriesButtonCallback(Widget, XtPointer)
{
    // RESET FILE NAME
    XtVaSetValues(m_filenameWidget, XmNvalue, "", NULL);

    // RESET SPECIES ENTRIES
    m_species= NULL;
    m_experiment= NULL;
    m_proteome= NULL;
    updateARBText();
}


/****************************************************************************
*  IMPORT DIALOG - CLEAR ENTRIES BUTTON
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticClearEntriesButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->clearEntriesButtonCallback(widget, callData);
}


/****************************************************************************
*  IMPORT DIALOG - OPEN FILE BUTTON
****************************************************************************/
void importDialog::openFileButtonCallback(Widget, XtPointer)
{
    // LOCAL VARIABLES
    char *filename;
    char *filetype;
    XmString *items;
    int item_pos;

    // GET SELECTED FILE TYPE (AS STRING)
    XtVaGetValues(m_fileTypeWidget,
        XmNitems, &items,
        XmNselectedPosition, &item_pos,
        NULL);
    XmStringGetLtoR(items[item_pos], XmFONTLIST_DEFAULT_TAG, &filetype);

    // GET SELECTED FILENAME
    filename= XmTextGetString(m_filenameWidget);

    // IMPORT DATA DEPENDING ON THE FILETYPE
    if(!strcmp(filetype, "CSV")) // TYPE == CSV
    {
        // IMPORT CSV DATA
        m_table= fileopenCSV(filename, ';');

        if(!m_table) return; // THROW ERROR OR WARNING HERE !?!?!
    }

    // COPY FIRST COLUMN ENTRY TO COLUMN HEADER
    copyHeaderEntries();

    // INIT ACTIVE HEADER (COLUMN COUNTER BEGINS WITH 0)
    m_activeHeader= 0;
    m_hasTableData= true;

    // UPDATE THE ENTRIES
    updateHeaderEntries();
}


/****************************************************************************
*  IMPORT DIALOG - OPEN FILE BUTTON
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticOpenFileButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->openFileButtonCallback(widget, callData);
}


/****************************************************************************
*  IMPORT DIALOG - UPDATE HEADER ENTRIES
****************************************************************************/
void importDialog::updateHeaderEntries()
{
    // RETURN, IF NO TABLE DATA IS AVAILABLE
    if(!m_hasTableData) return;

    // LOCAL VARIABLES
    char *buf= (char *)malloc(1024 * sizeof(char));

    // SET ACTUAL COUNTER LABEL
    sprintf(buf, "Column %d of %d", m_activeHeader + 1, m_table->columns);
    XtVaSetValues(m_selectorText, XmNvalue, buf, NULL);

    // UPDATE THE SAMPLE LIST DATA
    updateSampleList();

    // HEADER NAMES
    XtVaSetValues(m_columnHeaderWidget, XmNvalue, m_table->cell[m_activeHeader], NULL);
    XtVaSetValues(m_containerHeaderWidget, XmNvalue, m_table->header[m_activeHeader], NULL);

    // SET THE COLUMN TYPE COMBOBOX
    switch(m_table->columnType[m_activeHeader])
    {
        case DATATYPE_INT:
            XmComboBoxSelectItem(m_fieldTypeWidget, m_str_int);
            break;
        case DATATYPE_FLOAT:
            XmComboBoxSelectItem(m_fieldTypeWidget, m_str_float);
            break;
        case DATATYPE_STRING:
            XmComboBoxSelectItem(m_fieldTypeWidget, m_str_string);
            break;
    }

    // CLEAR BUFFER
    free(buf);
}


/****************************************************************************
*  IMPORT DIALOG - FILL SAMPLE LIST
****************************************************************************/
void importDialog::updateSampleList()
{
    // RETURN, IF NO TABLE DATA IS AVAILABLE
    if(!m_hasTableData) return;

    // HARDCODED - FOR NOW
    int m_samplePosition= 1;

    // FETCH TABLE VARIABLES
    int columns= m_table->columns;
    char **header= m_table->header;

    // CREATE BUFFERS
    char *buf1= (char *)malloc(1025 * sizeof(char));
    buf1[1024]= 0;
    char *buf2= (char *)malloc(1025 * sizeof(char));
    buf2[1024]= 0;

    // CLEAR SAMPLE DATA
    XmListDeleteAllItems(m_sampleList);

    // SET HEADER NAME
    char headerBuffer[33];
    headerBuffer[32]= 0;

    // ADD TITLE LINE TO SAMPLE LIST
    char *title= const_cast<char*>("                  CONTAINER NAME | # | CONTENT...");
    // char *title= "CONTAINER NAME                   | # | CONTENT...";
    XmListAddItem(m_sampleList, CreateDlgString(title), 0);

    //FETCH ALL COLUMN ENTRIES
    for(int i= 0; i < columns; i++)
    {
        strncpy(headerBuffer, header[i], 32);

        // SET FIELD TYPE
        char *type;
        switch(m_table->columnType[i])
        {
            case DATATYPE_INT:
                type= const_cast<char*>("I");
                break;
            case DATATYPE_FLOAT:
                type= const_cast<char*>("F");
                break;
            default:
                type= const_cast<char*>("S");
                break;
        }

        // SET FIELD TEXT (NUMBER OF BYTES: 1024 - THE OTHER ENTRIES LENGTH)
        strncpy(buf1, m_table->cell[(m_samplePosition * columns) + i], (1024 - 32 - 1 - 6));

        // CREATE FORMATED OUTPUT STRING
        sprintf(buf2, "%32s | %s | %s", headerBuffer, type, buf1);

        // ADD STRING TO SAMPLE LIST
        XmListAddItem(m_sampleList, CreateDlgString(buf2), 0);
    }

    // SELECT THE ACTIVE ENTRY
    XmListSelectPos(m_sampleList, m_activeHeader + 2, false);

    // MAKE LIST SCROLL TO THE CORRECT POSITION
    int topItem, visibleItems;
    XtVaGetValues(m_sampleList,
        XmNtopItemPosition, &topItem,
        XmNvisibleItemCount, &visibleItems,
        NULL);

    if((m_activeHeader + 2) < topItem)
        XtVaSetValues(m_sampleList,
            XmNtopItemPosition, m_activeHeader + 2,
            NULL);
    else if((m_activeHeader + 3) > (topItem + visibleItems))
        XtVaSetValues(m_sampleList,
            XmNtopItemPosition, m_activeHeader - visibleItems + 3,
            NULL);

    // FREE BUFFER
    free(buf1);
    free(buf2);
}


/****************************************************************************
*  IMPORT DIALOG - COLUMN DOWN COUNTER
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticColumnDownCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->columnDownCallback(widget, callData);
}


/****************************************************************************
*  IMPORT DIALOG - COLUMN DOWN COUNTER
****************************************************************************/
void importDialog::columnDownCallback(Widget, XtPointer)
{
    // RETURN, IF NO TABLE DATA IS AVAILABLE
    if(!m_hasTableData) return;

    int columns= m_table->columns;

    // UPDATE HEADER COUNTER
    m_activeHeader--;

    // IF COUNTER IS BEYOND UPPER LIMIT, DECREASE IT
    if(m_activeHeader < 0)
        m_activeHeader = m_activeHeader + columns;

    // UPDATE DATA
    updateHeaderEntries();
}


/****************************************************************************
*  IMPORT DIALOG - COLUMN UP COUNTER
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticColumnUpCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->columnUpCallback(widget, callData);
}


/****************************************************************************
*  IMPORT DIALOG - COLUMN UP COUNTER
****************************************************************************/
void importDialog::columnUpCallback(Widget, XtPointer)
{
    // RETURN, IF NO TABLE DATA IS AVAILABLE
    if(!m_hasTableData) return;

    int columns= m_table->columns;

    // UPDATE HEADER COUNTER
    m_activeHeader++;

    // IF COUNTER IS BEYOND UPPER LIMIT, DECREASE IT
    if(m_activeHeader >= columns)
        m_activeHeader= m_activeHeader - columns;

    // UPDATE DATA
    updateHeaderEntries();
}


/****************************************************************************
*  IMPORT DIALOG - COPY COLUMN HEADER TO ARB HEADER
****************************************************************************/
void importDialog::copyHeaderEntries()
{
    int columns= m_table->columns;
    char **header= m_table->header;
    char **cell= m_table->cell;
    // char *entry;

    for(int i= 0; i < columns; i++)
    {
        // CLEAR OLD HEADER, IF AVAILABLE
        if(header[i])
        {
            free(header[i]);
            header[i]= NULL;
        }

        // CLEAN STRING FOR USAGE AS ARB KEY
        header[i]= GBS_string_2_key(cell[i]);
    }

    m_table->hasHeader= true;
}


/****************************************************************************
*  IMPORT DIALOG - RESET THE ARB HEADER ENTRIES
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticResetHeaderButtonCallback(Widget, XtPointer clientData, XtPointer)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->copyHeaderEntries();
    iD->updateHeaderEntries();
}


/****************************************************************************
*  IMPORT DIALOG - TABLE HEADER CHANGED CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticHeaderChangedCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->headerChangedCallback(widget, callData);
}


/****************************************************************************
*  IMPORT DIALOG - TABLE HEADER CHANGED CALLBACK
****************************************************************************/
void importDialog::headerChangedCallback(Widget, XtPointer)
{
    // RETURN, IF NO TABLE DATA IS AVAILABLE
    if(!m_hasTableData) return;

    char **header= m_table->header;

    // GET CHANGED HEADER STRING
    char *str= XmTextGetString(m_containerHeaderWidget);

    // CLEAN STRING FOR USAGE IN ARB
    str= GBS_string_2_key(str);

    // FREE OLD ENTRY IF AVAILABLE
    if(header[m_activeHeader]) free(header[m_activeHeader]);

    // CREATE NEW ENTRY
    char *entry= (char *)malloc((strlen(str) + 1) * sizeof(char));
    strcpy(entry, str);

    header[m_activeHeader]= entry;

    // RETURN THE STRING AS IT MAY HAVE CHANGED...
    XtVaSetValues(m_containerHeaderWidget, XmNvalue, m_table->header[m_activeHeader], NULL);

    // UPDATE THE SAMPLE LIST DATA
    updateSampleList();
}


/****************************************************************************
*  IMPORT DIALOG - IMPORT DATA INTO ARB
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticImportDataCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->importDataCallback(widget, callData);
}


/****************************************************************************
*  IMPORT DIALOG - IMPORT DATA INTO ARB
****************************************************************************/
void importDialog::importDataCallback(Widget, XtPointer)
{
    // CREATE IMPORT DATA ENTRY
    importData data;

    // INITIALIZE DATA ENTRIES
    data.species= m_species;
    data.experiment= m_experiment;
    data.proteome= m_proteome;

    // IMPORT CSV DATA INTO ARB
    importCSV(m_table, &data);
}


/****************************************************************************
*  IMPORT DIALOG - ARB DATATYPE CHANGED CALLBACK
****************************************************************************/
void importDialog::changedDatatypeCallback(Widget, XtPointer callData)
{
    // RETURN, IF NO TABLE DATA IS AVAILABLE
    if(!m_hasTableData) return;

    // GET COMBOBOX SELECTION STRUCT
    XmComboBoxCallbackStruct *cb= (XmComboBoxCallbackStruct *)callData;

    // FETCH SELECTED ENTRY (STRING) FROM STRUCT
    char *type= (char *)XmStringUnparse(cb->item_or_text,
    XmFONTLIST_DEFAULT_TAG,
    XmCHARSET_TEXT, XmCHARSET_TEXT,
    NULL, 0, XmOUTPUT_ALL);

    // IF AN EVENT OCCURED...
    if(cb->event)
    {
        // DEPENDING ON THE SELECTED STRING, SET THE NEW COLUMN DATATYPE
        if(!strcmp(type, "INT"))
            m_table->columnType[m_activeHeader]= DATATYPE_INT;
        else if(!strcmp(type, "FLOAT"))
            m_table->columnType[m_activeHeader]= DATATYPE_FLOAT;
        else
            m_table->columnType[m_activeHeader]= DATATYPE_STRING;
    }

    // UPDATE THE SAMPLE LIST DATA
    updateSampleList();

    // FREE ENTRY STRING
    XtFree(type);
}


/****************************************************************************
*  IMPORT DIALOG - ARB DATATYPE CHANGED CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticChangedDatatypeCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->changedDatatypeCallback(widget, callData);
}


/****************************************************************************
*  IMPORT DIALOG - ANOTHER SAMPLE LIST ENTRY WAS SELECTED
****************************************************************************/
void importDialog::sampleListEntryCallback(Widget, XtPointer callData)
{
    // RETURN, IF NO TABLE DATA IS AVAILABLE
    if(!m_hasTableData) return;

    // GET LIST CALLBACK STRUCT (SELECTED ITEM ETC.)
    XmListCallbackStruct *cb= (XmListCallbackStruct *)callData;

    // GET & SET NEW POSITION
    if(cb->item_position > 1) m_activeHeader= cb->item_position - 2;

    // JUST TO AVOID COMPLICATIONS
    if(m_activeHeader < 0) m_activeHeader= 0;
    else if(m_activeHeader >= m_table->columns) m_activeHeader= m_table->columns - 1;

    // UPDATE THE HEADER ENTRIES...
    updateHeaderEntries();
}


/****************************************************************************
*  IMPORT DIALOG - ANOTHER SAMPLE LIST ENTRY WAS SELECTED
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticSampleListEntryCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->sampleListEntryCallback(widget, callData);
}


/****************************************************************************
*  IMPORT DIALOG - OPEN AN EXTERNAL DIALOG FOR HEADER NAME SELECTION
****************************************************************************/
void importDialog::externHeaderSelectionCallback(Widget, XtPointer)
{
    // RETURN, IF NO TABLE DATA IS AVAILABLE
    if(!m_hasTableData) return;


    static entrySelectionDialog *esD;

//     if(!esD)
//     {
        // CREATE NEW SELECTION DIALOG
        esD= new entrySelectionDialog(this);

        // ADD ENTRY CHANGED CALLBACK
        esD->setListCallback(staticExternHeaderChangedCallback);
//     }
//     else
//     {
//         esD->show();
//     }



//     // GET LIST CALLBACK STRUCT (SELECTED ITEM ETC.)
//     XmListCallbackStruct *cb= (XmListCallbackStruct *)callData;
//
//     // GET & SET NEW POSITION
//     if(cb->item_position > 1) m_activeHeader= cb->item_position - 2;
//
//     // JUST TO AVOID COMPLICATIONS
//     if(m_activeHeader < 0) m_activeHeader= 0;
//     else if(m_activeHeader >= m_table->columns) m_activeHeader= m_table->columns - 1;
//
//     // DEBUG DEBUG DEBUG
//     printf("DEBUG: ITEM= %d, ACTIVE= %d\n", cb->item_position, m_activeHeader);
//     // DEBUG DEBUG DEBUG
//
//     // UPDATE THE HEADER ENTRIES...
//     updateHeaderEntries();
}


/****************************************************************************
*  IMPORT DIALOG - OPEN AN EXTERNAL DIALOG FOR HEADER NAME SELECTION
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticExternHeaderSelectionCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    iD->externHeaderSelectionCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - HEADER NAME CHANGED EXTERNALLY CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticExternHeaderChangedCallback(Widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    importDialog *iD= (importDialog *)clientData;

    // GET SPECIES NAME FROM CALLDATA
    char *name= *((char **)callData);

    // SET NEW HEADER NAME
    iD->setHeaderName(name);

    // UPDATE OUTPUT
    // iD->updateARBText();
}


/****************************************************************************
*  MAIN DIALOG - SET SPECIES NAME
****************************************************************************/
void importDialog::setHeaderName(char *name)
{
    // RETURN, IF NO TABLE DATA IS AVAILABLE
    if(!m_hasTableData) return;

    char **header= m_table->header;

    // FREE OLD ENTRY IF AVAILABLE
    if(header[m_activeHeader]) free(header[m_activeHeader]);

    // CREATE NEW ENTRY
    char *entry= (char *)malloc((strlen(name) + 1) * sizeof(char));
    strcpy(entry, name);

    header[m_activeHeader]= entry;

    // RETURN THE STRING AS IT MAY HAVE CHANGED...
    XtVaSetValues(m_containerHeaderWidget, XmNvalue, m_table->header[m_activeHeader], NULL);

    // UPDATE THE SAMPLE LIST DATA
    updateSampleList();
}


