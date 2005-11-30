// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$


#include "analyze_window.hxx"


/****************************************************************************
*  ANALYZE WINDOW - CONSTRUCTOR
****************************************************************************/
analyzeWindow::analyzeWindow(MDialog *d)
    : MDialog(d)
{
    // CREATE WINDOW SHELL
    createShell("");

    // SET A NEW WIDTH AND HEIGHT OF THE SHELL
    XtVaSetValues(m_shell,
        XmNwidth, 800,
        XmNheight, 600,
        NULL);

    // CREATE MAIN WINDOW WIDGETS
    createWindow();

    // REALIZE SHELL & WIDGETS
    realizeShell();

    // SET WINDOW LABEL
    setDialogTitle("PGT - Data Analyzer");

    // DEBUG...

    m_myplot= new Plot(XtDisplay(m_plotArea), XtWindow(m_plotArea));

    // m_myplot->demo();

    XtAddCallback(m_plotArea, XmNresizeCallback, staticResizeGnuplot, this);

    resizeGnuplot();

    // ...DEBUG
}


/****************************************************************************
*  ANALYZE WINDOW - DESTRUCTOR
****************************************************************************/
analyzeWindow::~analyzeWindow()
{

}


/****************************************************************************
*  ANALYZE WINDOW - CREATE WINDOW
****************************************************************************/
void analyzeWindow::createWindow()
{
    // CREATE THIS WINDOWS TOP LEVEL WIDGET
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
        XmNheight, 30, // DEBUG
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CREATE TOP TOOLBAR WIDGET
    m_leftToolbar= XtVaCreateManagedWidget("topToolbar",
        xmRowColumnWidgetClass, m_top,
        XmNorientation, XmVERTICAL,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        XmNwidth, 30, // DEBUG
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_topToolbar,
        XmNleftAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);

    m_plotManager= XtVaCreateManagedWidget("plotManager",
        xmFormWidgetClass, m_top,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_topToolbar,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, m_leftToolbar,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    m_plotArea= XtVaCreateManagedWidget("area",
        xmDrawingAreaWidgetClass, m_plotManager,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

//     m_plotArea= XtVaCreateManagedWidget("area",
//         xmFormWidgetClass, m_plotManager,
//         XmNtopAttachment, XmATTACH_FORM,
//         XmNleftAttachment, XmATTACH_FORM,
//         XmNbottomAttachment, XmATTACH_FORM,
//         XmNrightAttachment, XmATTACH_FORM,
//         NULL);

    // FILL TOOLBARS WITH WIDGETS
    createTopToolbar();
    createLeftToolbar();
}


/****************************************************************************
*  ANALYZE WINDOW - CREATE TOP TOOLBAR
****************************************************************************/
void analyzeWindow::createTopToolbar()
{
    // DEBUG LABEL (PLACE HOLDER)
    XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_topToolbar,
        XmNlabelString, CreateDlgString("TOP"),
        NULL);
}


/****************************************************************************
*  ANALYZE WINDOW - CREATE LEFT TOOLBAR
****************************************************************************/
void analyzeWindow::createLeftToolbar()
{
    // DEBUG LABEL (PLACE HOLDER)
    XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_leftToolbar,
        XmNlabelString, CreateDlgString("LEFT"),
        NULL);
}



/****************************************************************************
*  ANALYZE WINDOW - RESIZE THE GNUPLOT WINDOW
****************************************************************************/
void analyzeWindow::resizeGnuplot()
{
    Dimension w, h;
    // unsigned int w, h;

    // GET DIMENSIONS
    XtVaGetValues(m_plotArea,
        XmNwidth, &w,
        XmNheight, &h,
        NULL);

    // SET PLOT WINDOW DIMENSIONS
    if(m_myplot) m_myplot->resizeWindow(0, 0, (int)w, (int)h);
}


/****************************************************************************
*  ANALYZE WINDOW - RESIZE THE GNUPLOT WINDOW
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticResizeGnuplot(Widget, XtPointer clientData, XtPointer)
{
    // GET POINTER OF THE ORIGINAL CALLER
    analyzeWindow *aW= (analyzeWindow *)clientData;

    // CALL CLASS MEMBER FUNCTION
    aW->resizeGnuplot();
}

