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
#include "entry_sel_dialog.hxx"


bool entrySelectionDialog::m_opened= false;


/****************************************************************************
*  ENTRY SELECTION DIALOG - CONSTRUCTOR
****************************************************************************/
entrySelectionDialog::entrySelectionDialog(Widget p, MDialog *d)
    : MDialog(p, d)
{
    if(m_opened)
    {
//         this->~entrySelectionDialog();
        return;
    }
    else m_opened= true;

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
    setWindowName("PGT - Selection");

    // DESELECT ALL WINDOW BUTTONS EXCEPT CLOSE AND MOVE
    XtVaSetValues(m_shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE, NULL);

    // FILL THE LIST WITH ENTRIES...
    getEntryNamesList(m_list, true);
}


/****************************************************************************
*  ENTRY SELECTION DIALOG - DESTRUCTOR
****************************************************************************/
entrySelectionDialog::~entrySelectionDialog()
{
}


/****************************************************************************
*  MAIN DIALOG - CREATE WINDOW
****************************************************************************/
void entrySelectionDialog::createWindow()
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
        XmNlabelString, PGT_XmStringCreateLocalized("Select entry from the list:"),
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE SPECIES LIST WIDGET
    m_list= XmCreateScrolledList(m_top, "selectionList", NULL, 0);
    XtVaSetValues(m_list,
        XmNallowResize, true,
        XmNvisibleItemCount, 25,
        NULL);
    XtVaSetValues(XtParent(m_list),
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label1,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtManageChild(m_list);
    XtAddCallback(m_list, XmNbrowseSelectionCallback, staticListCallback, this);

    // CREATE AN EXIT BUTTON
    Widget exitButton= XtVaCreateManagedWidget("exitButton",
        xmPushButtonWidgetClass, m_top,
        XmNlabelString, PGT_XmStringCreateLocalized("Close"),
        XmNwidth, 100,
        XmNheight, 30,
        XmNrightAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_list,
        // XmNbottomAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(exitButton, XmNactivateCallback, staticEntrySelExitButtonCallback, this);
}


/****************************************************************************
*  CALLBACK - EXIT BUTTON CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticEntrySelExitButtonCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    entrySelectionDialog *esD= (entrySelectionDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    esD->exitButtonCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - EXIT BUTTON CALLBACK
****************************************************************************/
void entrySelectionDialog::exitButtonCallback(Widget, XtPointer)
{
    m_opened= false;
    this->~entrySelectionDialog();
}


/****************************************************************************
*  CALLBACK - SPECIES CALLBACK
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticListCallback(Widget widget, XtPointer clientData, XtPointer callData)
{
    // GET POINTER OF THE ORIGINAL CALLER
    entrySelectionDialog *esD= (entrySelectionDialog *)clientData;

    // CALL CLASS MEMBER FUNCTION
    esD->listCallback(widget, callData);
}


/****************************************************************************
*  CALLBACK - SPECIES CALLBACK
****************************************************************************/
void entrySelectionDialog::listCallback(Widget, XtPointer callData)
{
    // GET CALLBACK DATA
    XmListCallbackStruct *cbs= (XmListCallbackStruct *)callData;
    XmStringGetLtoR(cbs->item, XmFONTLIST_DEFAULT_TAG, &m_entry);

    // TRIGGER ENTRY CHANGED CALLBACK
    triggerListChange();
}


/****************************************************************************
*  SET SPECIES DATA CHANGED CALLBACK
****************************************************************************/
void entrySelectionDialog::setListCallback(XtCallbackProc callback)
{
    m_listCallback= callback;
    m_hasListCallback= true;
}


/****************************************************************************
*  TRIGGER SPECIES DATA CHANGED CALLBACK
****************************************************************************/
void entrySelectionDialog::triggerListChange()
{
    if(m_hasListCallback)
        m_listCallback(m_parent, (XtPointer)m_parent_dialog, (XtPointer)&m_entry);
}

