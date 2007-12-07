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
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <arbdb.h>
#include <arbdbt.h>
// #include <X11/ShellP.h>


/****************************************************************************
*  CALLBACK FUNCTION (FROM THE WINDOW CLOSE CALLBACK)
****************************************************************************/
void staticWindowCloseCallback(Widget, XtPointer clientData, XtPointer)
{
    // GET POINTER OF THE ORIGINAL CALLER
    MDialog *md= (MDialog *)clientData;

    // CLOSE THE DIALOG
    md->closeDialog();
}


/****************************************************************************
*  MDIALOG - CONSTRUCTOR #1
****************************************************************************/
MDialog::MDialog(Widget w)
{
    // PREDEFINE VARIABLES
    m_shell= NULL;
    m_visible= false;
    m_parent_dialog= NULL;
    m_parent_widget= w;
}


/****************************************************************************
*  MDIALOG - CONSTRUCTOR #2
****************************************************************************/
MDialog::MDialog(MDialog *parent)
{
    // PREDEFINE VARIABLES
    m_shell= NULL;
    m_visible= false;

    // FETCH THE POINTER TO THE PARENT DIALOG
    m_parent_dialog= parent;

    if(m_parent_dialog)
    {
        // ADD NEW DIALOG TO THE PARENTS LIST
        m_parent_dialog->addChild(this);

        // FETCH THE PARENT WIDGET (IF POSSIBLE)
        m_parent_widget= m_parent_dialog->shellWidget();
    }
    else
        m_parent_widget= NULL;
}


/****************************************************************************
*  MDIALOG - DESTRUCTOR
****************************************************************************/
MDialog::~MDialog()
{
    // CLOSE THIS DIALOG AND ALL CHILDREN
    closeDialog();
}


/****************************************************************************
*  MDIALOG - CREATEMYWIDGET
*  CREATES A NEW TOP LEVEL SHELL
****************************************************************************/
void MDialog::createShell(const char *name)
{
    // CREATE TOP LEVEL APPLICATION SHELL
    m_shell= XtVaAppCreateShell(NULL, name,
        applicationShellWidgetClass, XtDisplay(m_parent_widget),
        XmNdeleteResponse, XmDO_NOTHING, // WINDOW CLOSE SHOULD BE CALLED MANUALLY...
        NULL);

    // CREATE AN ATOM TO CATCH WM_DELETE_WINDOW
    Atom atom = XInternAtom(XtDisplay(m_shell), "WM_DELETE_WINDOW", false);

    // ADD THE CALLBACK TO THE SHELL WIDGET
    XmAddWMProtocolCallback(m_shell, atom, staticWindowCloseCallback, this);
}


/****************************************************************************
*  MDIALOG - CLOSE / UNMANAGE DIALOG
****************************************************************************/
void MDialog::closeDialog()
{
    // KILL ALL CHILDREN
    while(m_children.size())
    {
        // for(vector<MDialog*>::iterator i = m_children.begin(); i != m_children.end(); i++)
        for(vector<MDialog*>::iterator i = m_children.begin(); i != m_children.end(); ++i)
        {
            delete *i;
            m_children.erase(i);
            break; // NECESSARY, BECAUSE VECTOR ITERATOR MUST NOW BE REBUILT
        }
    }
    m_children.clear();

    // DESTROY OUR SHELL AND ALL OUR WIDGETS
    if(m_shell != NULL) XtDestroyWidget(m_shell);
    m_shell= NULL;
    m_visible= false;

    // FREE ALL STRINGS PREVIOUSLY USED BY THE DIALOG
    // for(vector<XmString>::iterator s = m_strings.begin(); s != m_strings.end(); s++)
    for(vector<XmString>::iterator s = m_strings.begin(); s != m_strings.end(); ++s)
    {
        // printf("FREEING STRING \"%s\"\n", XmCvtXmStringToCT((XmString)*s)); // DEBUG

        XmStringFree((XmString)*s);
    }
    m_strings.clear();
}


/****************************************************************************
*  MDIALOG - CLOSE / UNMANAGE DIALOG
****************************************************************************/
bool MDialog::isVisible()
{
    return m_visible;
}


/****************************************************************************
*  MDIALOG - CREATEMYWIDGET
*  CREATES A NEW TOP LEVEL SHELL
****************************************************************************/
void MDialog::realizeShell()
{
    // REALIZE MAIN AND TOPLEVEL WIDGET
    if(m_shell) XtRealizeWidget(m_shell);

    // DIALOG IS ENABLED (WE HAVE WIDGETS)
    m_visible= true;

    // SHOULD RETURN ERROR IF M_SHELL IS UNDEFINED!
}


/****************************************************************************
*  MDIALOG - SETS THE WINDOW TITLE OF THE DIALOG
****************************************************************************/
void MDialog::setDialogTitle(const char *name)
{
    if(m_shell)
        XStoreName(XtDisplay(m_shell), XtWindow(m_shell), const_cast<char*>(name));
}


/****************************************************************************
*  MDIALOG - SETS THE WINDOW WIDTH AND HEIGHT (IN PIXEL)
****************************************************************************/
void MDialog::setDialogSize(int w, int h)
{
    if(m_shell)
        XtVaSetValues(m_shell, XmNwidth, w, XmNheight, h, NULL);
}


/****************************************************************************
*  MDIALOG - RETURN SHELL WIDGET
****************************************************************************/
Widget MDialog::shellWidget()
{
    return m_shell;
}


/****************************************************************************
*  MDIALOG - SHOW WIDGET
****************************************************************************/
void MDialog::show()
{
    // RARELY TESTED!
    if(m_shell)
    {
        XtMapWidget(m_shell);
        m_visible= true;
    }

//    ShellWidget shell_widget= NULL;
//
//    if(m_shell != NULL && XtIsSubclass(m_shell, shellWidgetClass))
//    {
//       shell_widget= (ShellWidget)m_shell;
//
//       if(XtParent(shell_widget) == NULL ||
//         (XtIsTopLevelShell(shell_widget) &&
//         XtIsManaged((Widget)shell_widget)) ||
//         shell_widget->shell.popped_up)
//       {
//          XtMapWidget (m_shell);
//       }
//    }
}


/****************************************************************************
*  MDIALOG - HIDE WIDGET
****************************************************************************/
void MDialog::hide()
{
    // RARELY TESTED!
    if(m_shell)
    {
        XtUnmapWidget(m_shell);
        m_visible= false;
    }

//    ShellWidget shell_widget= NULL;
//
//    if(m_shell != NULL && XtIsSubclass(m_shell, shellWidgetClass))
//    {
//       shell_widget= (ShellWidget)m_shell;
//
//       if(XtIsTopLevelShell(shell_widget) ||
//         shell_widget->shell.popped_up)
//         {
// #ifdef USE_WITHDRAW_WINDOW
//             XWithdrawWindow(XtDisplay(m_shell),
//                 XtWindow(m_shell),
//                 XScreenNumberOfScreen(XtScreen(m_shell)));
// #else
//             XtUnmapWidget(m_shell);
// #endif
//         }
//     }
}


/****************************************************************************
*  MDIALOG - ADD A CHILD WIDGET TO THE PARENTS LIST
****************************************************************************/
void MDialog::addChild(MDialog *child)
{
    // ADD AN ERROR CHECK
    m_children.push_back(child);

    // SHOULD RETURN THE ADDED CHILD!?
}


/****************************************************************************
*  MDIALOG - REMOVE A CHILD WIDGET FROM THE PARENTS LIST
****************************************************************************/
void MDialog::removeChild(MDialog *child)
{
    for(vector<MDialog*>::iterator i = m_children.begin(); i != m_children.end(); ++i)
    {
        if((MDialog*)*i == child) m_children.erase(i);
        break;
    }

    // SHOULD RETURN THE REMOVED CHILD!?
}


/****************************************************************************
*  MDIALOG - CREATE A STRING (DIALOG WILL TAKE CARE OF ALLOC/FREE)
****************************************************************************/
XmString MDialog::CreateDlgString(const char *c_str)
{
    // CREATE A XMSTRING
    XmString xm_str= XmStringCreateLocalized(const_cast<char*>(c_str));

    // ADD STRING TO THE DIALOGS STRINGS
    m_strings.push_back(xm_str);

    // RETURN THE STRING
    return xm_str;

    // CAUTION: THE XMSTRING WILL BE AUTOMATICALLY
    // FREED BY THE DIALOG CLASS (IN THE DESTRUCTOR).
}


/****************************************************************************
*  RETURNS THE REAL PATH OF TO A PGT-PIXMAP
*  --> THIS IS NOT A MDIALOG MEMBER FUNCTION!
****************************************************************************/
static char *pixmapPath(const char *pixmapName)
{
    return GBS_global_string_copy("%s/lib/pixmaps/pgt/%s", GB_getenvARBHOME(), pixmapName);
}


/****************************************************************************
*  EXTENDED LOAD-PIXMAP FUNCTION
*  --> THIS IS NOT A MDIALOG MEMBER FUNCTION!
****************************************************************************/
Pixmap PGT_LoadPixmap(const char *name, Screen *s, Pixel fg, Pixel bg)
{
    char *fullname= pixmapPath(name);

    Pixmap pixmap= XmGetPixmap(s, fullname, fg, bg);

    free(fullname);

    return pixmap;
}

