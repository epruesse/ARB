// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$


#include "arb_interface.hxx"
#include "selection_dialog.hxx"


bool selectionDialog::m_opened= false;


/****************************************************************************
*  SELECTION DIALOG - CONSTRUCTOR
****************************************************************************/
selectionDialog::selectionDialog(Widget p, MDialog *d, int type)
    : MDialog(p, d)
{
    if(m_opened)
    {
//         this->~selectionDialog();
        return;
    }
    else m_opened= true;

    // DEFAULT VALUES
    m_hasSpeciesCallback= false;
    m_hasExperimentCallback= false;
    m_hasProteomeCallback= false;
    m_ignoreProteomeCallback= false;
    m_species= NULL;
    m_experiment= NULL;
    m_proteome= NULL;
    m_type= type;

    // CREATE SHELL FOR THIS DIALOG
    createShell("");

    // CALL CREATE WINDOW FUNCTION
    createWindow();

    // SET WINDOW WIDTH
    XtVaSetValues(m_shell,
        XmNwidth, 300,
        XmNheight, 500,
        NULL);

    // REALIZE WINDOW
    realizeShell();

    // SET WINDOW LABEL
    switch(m_type)
    {
        case SELECTION_DIALOG_READ:
            setWindowName("PGT - Select Source");
            break;
        case SELECTION_DIALOG_WRITE:
            setWindowName("PGT - New Destination");
            break;
        case SELECTION_DIALOG_RW:
        default:
            setWindowName("PGT - Selection");
            break;
    }

    // DESELECT ALL WINDOW BUTTONS EXCEPT CLOSE AND MOVE
    XtVaSetValues(m_shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE, NULL);

    getSpeciesList(m_speciesList, true);
}


/****************************************************************************
*  SELECTION DIALOG - DESTRUCTOR
****************************************************************************/
selectionDialog::~selectionDialog()
{
    if(m_species) free(m_species);
    if(m_experiment) free(m_experiment);
    if(m_proteome) free(m_proteome);
}


/****************************************************************************
*  MAIN DIALOG - CREATE WINDOW
****************************************************************************/
void selectionDialog::createWindow()
{
    // CREATE TOP LEVEL WIDGET
    m_top= XtVaCreateManagedWidget("top",
        xmFormWidgetClass, m_shell,
        XmNorientation, XmVERTICAL,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        NULL);

    // CREATE A SIMPLE LABEL
    Widget label1= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, PGT_XmStringCreateLocalized("Select Species:"),
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE SPECIES LIST WIDGET
    m_speciesList= XmCreateScrolledList(m_top, const_cast<char*>("speciesList"), NULL, 0);
    XtVaSetValues(m_speciesList,
        XmNallowResize, true,
        XmNvisibleItemCount, 5,
        // XmNselectionPolicy, XmSINGLE_SELECT,
        NULL);
    XtVaSetValues(XtParent(m_speciesList),
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label1,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtManageChild(m_speciesList);
    XtAddCallback(m_speciesList, XmNbrowseSelectionCallback, staticSpeciesCallback, this);

    // CREATE A SIMPLE LABEL
    Widget label2= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, PGT_XmStringCreateLocalized("Select Experiment:"),
        // XmNheight, 30,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_speciesList,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE EXPERIMENT LIST WIDGET
    m_experimentList= XmCreateScrolledList(m_top, const_cast<char*>("experimentList"), NULL, 0);
    XtVaSetValues(m_experimentList,
        XmNallowResize, true,
        XmNvisibleItemCount, 5,
        // XmNselectionPolicy, XmSINGLE_SELECT,
        NULL);
    XtVaSetValues(XtParent(m_experimentList),
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label2,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtManageChild(m_experimentList);
    XtAddCallback(m_experimentList, XmNbrowseSelectionCallback, staticExperimentCallback, this);

    // CREATE A SIMPLE LABEL
    Widget label3= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, PGT_XmStringCreateLocalized("Select Proteome:"),
        // XmNheight, 30,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_experimentList,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE PROTEOME LIST WIDGET
    m_proteomeList= XmCreateScrolledList(m_top, const_cast<char*>("proteomeList"), NULL, 0);
    XtVaSetValues(m_proteomeList,
        XmNallowResize, true,
        XmNvisibleItemCount, 5,
        // XmNselectionPolicy, XmSINGLE_SELECT,
        NULL);
    XtVaSetValues(XtParent(m_proteomeList),
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label3,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtManageChild(m_proteomeList);
    XtAddCallback(m_proteomeList, XmNbrowseSelectionCallback, staticProteomeListCallback, this);

    // CREATE A SIMPLE LABEL
    Widget label01= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, PGT_XmStringCreateLocalized("Enter a new proteome name here:"),
        XmNheight, 30,
        // XmNalignment, XmALIGNMENT_BEGINNING,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_proteomeList,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE NEW PROTEOME TEXT FIELD
    m_proteomeText= XtVaCreateManagedWidget("proteomeTextField",
        xmTextWidgetClass, m_top,
        // XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label01,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    if(m_type == SELECTION_DIALOG_WRITE || m_type == SELECTION_DIALOG_RW)
    {
        // ADD CALLBACK ONLY IF WE ARE A WRITEABLE TYPE
        XtAddCallback(m_proteomeText, XmNvalueChangedCallback, staticProteomeTextCallback, this);
    }
    else
    {
        // OTHERWISE DEACTIVATE EDIT FUNCTIONS
        XtVaSetValues(m_proteomeText,
            XmNeditable, false,
            NULL);

        // AND CHANGE LABEL TEXT
        XtVaSetValues(label01,
            XmNlabelString, PGT_XmStringCreateLocalized("---"),
            NULL);
    }

    m_warning_label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, PGT_XmStringCreateLocalized(""),
        // XmNheight, 30,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_proteomeText,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    Widget exitButton= XtVaCreateManagedWidget("exitButton",
        xmPushButtonWidgetClass, m_top,
        XmNlabelString, PGT_XmStringCreateLocalized("Close"),
        XmNwidth, 100,
        XmNheight, 30,
        XmNrightAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        // XmNtopAttachment, XmATTACH_WIDGET,
        // XmNtopWidget, m_proteomeText,
        NULL);
    XtAddCallback(exitButton, XmNactivateCallback, staticExitButtonCallback, this);
}


/****************************************************************************
*  CALLBACK - EXIT BUTTON CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticExitButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    selectionDialog *sD= (selectionDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    sD->exitButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - EXIT BUTTON CALLBACK
****************************************************************************/
void selectionDialog::exitButtonCallback(Widget, XtPointer)
{
    m_opened= false;
    this->~selectionDialog();
}


/****************************************************************************
*  CALLBACK - SPECIES CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticSpeciesCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    selectionDialog *sD= (selectionDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    sD->speciesCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - SPECIES CALLBACK
****************************************************************************/
void selectionDialog::speciesCallback(Widget, XtPointer callData)
{
    // GET CALLBACK DATA
    XmListCallbackStruct *cbs= (XmListCallbackStruct *)callData;
    XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &m_species);

    // FILL EXPERIMENT LIST
    getExperimentList(m_experimentList, m_species, true);

    // CLEAR PROTEOME ENTRIES
    XmListDeleteAllItems(m_proteomeList);
    XtVaSetValues(m_proteomeText, XmNvalue, "", NULL);

    // TRIGGER ENTRY CHANGED CALLBACK
    triggerSpeciesChange();
}


/****************************************************************************
*  CALLBACK - EXPERIMENT CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticExperimentCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    selectionDialog *sD= (selectionDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    sD->experimentCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - EXPERIMENT CALLBACK
****************************************************************************/
void selectionDialog::experimentCallback(Widget, XtPointer callData)
{
    // GET CALLBACK DATA
    XmListCallbackStruct *cbs= (XmListCallbackStruct *)callData;
    XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &m_experiment);

    // FILL PROTEOME LIST
    getProteomeList(m_proteomeList, m_species, m_experiment, true);

    // CLEAR PROTEOME TEXT FIELD
    XtVaSetValues(m_proteomeText, XmNvalue, "", NULL);

    // TRIGGER ENTRY CHANGED CALLBACK
    triggerExperimentChange();
}


/****************************************************************************
*  CALLBACK - PROTEOME LIST CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticProteomeListCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    selectionDialog *sD= (selectionDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    sD->proteomeListCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - PROTEOME LIST CALLBACK
****************************************************************************/
void selectionDialog::proteomeListCallback(Widget, XtPointer callData)
{
    // GET CALLBACK DATA
    XmListCallbackStruct *cbs= (XmListCallbackStruct *)callData;
    XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &m_proteome);

    // SET TEXT FIELD ENTRY
    XtVaSetValues(m_proteomeText, XmNvalue, m_proteome, NULL);

    // TEXT FIELD TEXT -> M_PROTEOME
    m_proteome= XmTextGetString(m_proteomeText);

    // TRIGGER ENTRY CHANGED CALLBACK
    triggerProteomeChange();
}


/****************************************************************************
*  CALLBACK - PROTEOME TEXT CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticProteomeTextCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    selectionDialog *sD= (selectionDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    sD->proteomeTextCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - PROTEOME TEXT CALLBACK
****************************************************************************/
void selectionDialog::proteomeTextCallback(Widget, XtPointer)
{
    if(m_ignoreProteomeCallback) return;

    // TEXT FIELD TEXT -> M_PROTEOME
    char *proteome= XmTextGetString(m_proteomeText);

    // DISABLE CALLBACK (OTHERWISE A INFINITE LOOP WOULD OCCUR)
    m_ignoreProteomeCallback= true;

    // FIX PROTEOME TEXT FOR ARB
    m_proteome= GBS_string_2_key(proteome);

    // IF THE STRING DIFFERS - THROW WARNING
    if(strlen(m_proteome) < 3)
    {
        XtVaSetValues(m_warning_label,
            XmNlabelString, PGT_XmStringCreateLocalized("WARNING - NAME IS TOO SHORT"),
            NULL);
    }
    else if(strcmp(m_proteome, proteome))
    {
        XtVaSetValues(m_warning_label,
            XmNlabelString, PGT_XmStringCreateLocalized("WARNING - CONTAINS ILLEGAL CHARACTER(S)"),
            NULL);
    }
    else
    {
        XtVaSetValues(m_warning_label,
            XmNlabelString, PGT_XmStringCreateLocalized(""),
            NULL);
    }

    // ENABLE CALLBACK
    m_ignoreProteomeCallback= false;

    // TRIGGER ENTRY CHANGED CALLBACK
    triggerProteomeChange();
}


/****************************************************************************
*  SET SPECIES DATA CHANGED CALLBACK
****************************************************************************/
void selectionDialog::setSpeciesCallback(XtCallbackProc callback)
{
    m_speciesCallback= callback;
    m_hasSpeciesCallback= true;
}


/****************************************************************************
*  TRIGGER SPECIES DATA CHANGED CALLBACK
****************************************************************************/
void selectionDialog::triggerSpeciesChange()
{
    if(m_hasSpeciesCallback)
        m_speciesCallback(m_parent, (XtPointer)m_parent_dialog, (XtPointer)&m_species);
}


/****************************************************************************
*  SET EXPERIMENT DATA CHANGED CALLBACK
****************************************************************************/
void selectionDialog::setExperimentCallback(XtCallbackProc callback)
{
    m_experimentCallback= callback;
    m_hasExperimentCallback= true;
}


/****************************************************************************
*  TRIGGER EXPERIMENT DATA CHANGED CALLBACK
****************************************************************************/
void selectionDialog::triggerExperimentChange()
{
    if(m_hasExperimentCallback)
        m_experimentCallback(m_parent, (XtPointer)m_parent_dialog, (XtPointer)&m_experiment);
}


/****************************************************************************
*  SET PROTEOME DATA CHANGED CALLBACK
****************************************************************************/
void selectionDialog::setProteomeCallback(XtCallbackProc callback)
{
    m_proteomeCallback= callback;
    m_hasProteomeCallback= true;
}


/****************************************************************************
*  TRIGGER PROTEOME DATA CHANGED CALLBACK
****************************************************************************/
void selectionDialog::triggerProteomeChange()
{
    // TEXT FIELD TEXT -> M_PROTEOME
    char *proteome= XmTextGetString(m_proteomeText);

    // FIX PROTEOME TEXT FOR ARB
    m_proteome= GBS_string_2_key(proteome);

    if(m_hasProteomeCallback)
        m_proteomeCallback(m_parent, (XtPointer)m_parent_dialog, (XtPointer)&m_proteome);
}
