// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$


#include "dialog.hxx"
#include <arbdb.h>
#include <arbdbt.h>


/****************************************************************************
*  CALLBACK FUNCTION (FROM THE WINDOW CLOSE CALLBACK)
****************************************************************************/
void staticWindowCloseCallback(Widget, XtPointer clientData, XtPointer)
{
    // GET POINTER OF THE ORIGINAL CALLER
    MDialog *md= (MDialog *)clientData;

    // CLOSE THE DIALOG
    md->closeDialog();

    // DESTROY THE DIALOG OBJECT
    // (POSSIBLY DANGEROUS IF A PARENT KEEPS A POINTER TO THE OBJECT)
    delete md;
}


/****************************************************************************
*  MDIALOG - CONSTRUCTOR
****************************************************************************/
MDialog::MDialog(Widget p, MDialog *d= NULL)
{
    // AT THIS TIME WE HAVE NO WIDGETS
    m_shell= NULL;
    m_enabled= false;

    // OUR PARENT WIDGET IS P
    m_parent= p;

    // OUT PARENT DIALOG IS D
    m_parent_dialog= d;

    // AT FIRST, THIS DIALOG IS NON-BLOCKING
    m_modal= false;
}


/****************************************************************************
*  MDIALOG - DESTRUCTOR
****************************************************************************/
MDialog::~MDialog()
{
    // DESTROY ALL OUR WIDGETS
    if(m_shell != NULL) XtDestroyWidget(m_shell);
}


/****************************************************************************
*  MDIALOG - CREATEMYWIDGET
*  CREATES A NEW TOP LEVEL SHELL
****************************************************************************/
void MDialog::createShell(const char *name)
{
    if(m_modal)
    {
        // CREATE TOP LEVEL DIALOG SHELL
        m_shell= XmCreateDialogShell(m_parent, const_cast<char*>(name), 0, 0);

        XtVaSetValues(m_shell, XmNdialogStyle, XmDIALOG_APPLICATION_MODAL, NULL);
    }
    else
    {
        // CREATE TOP LEVEL APPLICATION SHELL
        m_shell= XtVaAppCreateShell(NULL, name,
            applicationShellWidgetClass, XtDisplay(m_parent),
            XmNdeleteResponse, XmDO_NOTHING, // WINDOW CLOSE SHOULD BE CALLED MANUALLY...
            NULL);
    }

    // CREATE AN ATOM TO CATCH WM_DELETE_WINDOW
    Atom atom = XInternAtom(XtDisplay(m_shell), "WM_DELETE_WINDOW", false) ;

    // ADD THE CALLBACK TO THE SHELL WIDGET
    XmAddWMProtocolCallback(m_shell, atom, staticWindowCloseCallback, this) ;

    // DIALOG IS ENABLED (WE HAVE WIDGETS)
    m_enabled= true;
}


/****************************************************************************
*  MDIALOG - CLOSE / UNMANAGE DIALOG
****************************************************************************/
void MDialog::closeDialog()
{
    // XtUnmanageChild(m_shell);
    XtDestroyWidget(m_shell);
    m_shell= NULL;

    m_enabled= false;
}


/****************************************************************************
*  MDIALOG - CLOSE / UNMANAGE DIALOG
****************************************************************************/
bool MDialog::isEnabled()
{
    return m_enabled;
}


/****************************************************************************
*  MDIALOG - CREATEMYWIDGET
*  CREATES A NEW TOP LEVEL SHELL
****************************************************************************/
void MDialog::realizeShell()
{
    // REALIZE MAIN AND TOPLEVEL WIDGET
    XtRealizeWidget(m_shell);
}


/****************************************************************************
*  MDIALOG - SETWINDOWNAME
****************************************************************************/
void MDialog::setWindowName(const char *name)
{
    XStoreName(XtDisplay(m_shell), XtWindow(m_shell), const_cast<char*>(name));
}


/****************************************************************************
*  MDIALOG - RETURN SHELL WIDGET
****************************************************************************/
Widget MDialog::shellWidget()
{
    return m_shell;
}


/****************************************************************************
*  MDIALOG - RETURN PARENT WIDGET
****************************************************************************/
Widget MDialog::parentWidget()
{
    return m_parent;
}


/****************************************************************************
*  MDIALOG - SHOW WIDGET
****************************************************************************/
void MDialog::show()
{
    // UNSUPPORTED SO FAR...
}


/****************************************************************************
*  MDIALOG - HIDE WIDGET
****************************************************************************/
void MDialog::hide()
{
    // UNSUPPORTED SO FAR...
    // XUNMAPWINDOW ???
}


/****************************************************************************
*  RETURNS THE REAL PATH OF TO A PGT-PIXMAP
****************************************************************************/
static char *pixmapPath(const char *pixmapName)
{
    return GBS_global_string_copy("%s/lib/pixmaps/pgt/%s", GB_getenvARBHOME(), pixmapName);
}


/****************************************************************************
*  EXTENDED LOAD-PIXMAP FUNCTION
****************************************************************************/
Pixmap PGT_LoadPixmap(const char *name, Screen *s, Pixel fg, Pixel bg)
{
    char *fullname= pixmapPath(name);

    Pixmap pixmap= XmGetPixmap(s, fullname, fg, bg);

    free(fullname);

    return pixmap;
}
