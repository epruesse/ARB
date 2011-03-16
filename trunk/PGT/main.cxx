// COPYRIGHT (C) 2004 - 2005
//         KAI BADER <BADERK@IN.TUM.DE>
//         DEPARTMENT OF MICROBIOLOGY (TECHNICAL UNIVERSITY MUNICH)
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// CVS REVISION TAG  --  $Revision$


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Xm/XmAll.h>
#include "arb_interface.hxx"
#include "main_dialog.hxx"


/****************************************************************************
*  MAIN EVENT HANDLER CLASS
****************************************************************************/
class CMain
{
    GB_shell shell;
    
    public:
        CMain();
        ~CMain();
        //
        bool DB_Connect();
        void DB_Disconnect();
        int Run(int, char **);
    protected:
        void MainLoop();
        //
        bool m_connected;
        XEvent m_event;
        XDestroyWindowEvent m_dw_event;
        XtAppContext m_xapp;
        mainDialog *m_maindialog;
        Widget m_topwidget;
        Window m_topwindow;
};


/****************************************************************************
*  MAIN EVENT HANDLER CLASS -- CONSTRUCTOR
****************************************************************************/
CMain::CMain()
{
    m_connected= false;
    m_maindialog= NULL;
}


/****************************************************************************
*  MAIN EVENT HANDLER CLASS -- DESTRUCTOR
****************************************************************************/
CMain::~CMain()
{
    // DESTRUCT MAIN DIALOG CLASS (IF NOT ALREADY HAPPENED!?)
    if(m_maindialog) delete m_maindialog;

    // CLOSE CONNECTIONS IF THEY ARE STILL OPEN
    DB_Disconnect();

    // DESTROY TOP WIDGET
    if(m_topwidget) XtDestroyWidget(m_topwidget);
}


/****************************************************************************
*  MAIN EVENT HANDLER CLASS -- CONNECT ALL DATABASES
*
*  RETURNVALUE:
*       TRUE = AN ARB CONNECTION IS ESTABLISHED
*       FALSE = AN ARB CONNECTION COULD NOT BE ESTABLISHED
****************************************************************************/
bool CMain::DB_Connect()
{
    // CHECK CONNECTION FLAG (ALREADY CONNECTED?)
    if(!m_connected)
    {
        // TRY TO CONNECT TO THE DEFAULT ARB DATABASE; EXIT IF AN ERROR OCCURS
        if(ARB_connect(NULL))
        {
            // IF A CONNECTION IS ESTABLISHED: ALSO CONNECT TO THE CONFIG DB
            // (NO 'ACCESS-FAILED' CHECK HERE AT THE MOMENT)
            CONFIG_connect();

            m_connected= true;
        }
    }

    return m_connected;
}


/****************************************************************************
*  MAIN EVENT HANDLER CLASS -- DISCONNECT ALL DATABASES
****************************************************************************/
void CMain::DB_Disconnect()
{
    // IS THERE AN OPEN CONNECTION?
    if(!m_connected) return;

    // DISCONNECT ALL OPENED DATABASES (AVOIDS UNSTABLE ARB CONDITIONS)
    CONFIG_disconnect();
    ARB_disconnect();

    // SET CONNECTION FLAG -> FALSE
    m_connected= false;
}


/****************************************************************************
*  MAIN EVENT HANDLER CLASS -- MAIN EVENT LOOP (CATCHES X11 EXIT EVENTS)
****************************************************************************/
void CMain::MainLoop()
{
    // EVENT LOOP (BREAKS ON A WINDOW DESTROY EVENT)
    for(;;)
    {
        XtAppNextEvent(m_xapp, &m_event);

        if(m_event.type == DestroyNotify)
        {
            m_dw_event= m_event.xdestroywindow;
            if(m_dw_event.window == m_topwindow) break;
        }

        XtDispatchEvent(&m_event);
    }
}


/****************************************************************************
*  MAIN EVENT HANDLER CLASS -- INITS THE CLASS AND RUN EVENT LOOP
*
*  RETURNVALUE:
*        0 = EVERYTHING IS FINE
        -1 = AN ERROR OCCURRED (UNABLE TO ESTABLISH DB CONNECTION)
****************************************************************************/
int CMain::Run(int argc, char **argv)
{
    // TRY TO ESTABLISH THE DATABASE CONNECTIONS
    if(!m_connected)
        if(!DB_Connect())
        {
            printf("PGT-ERROR: Unable to connect to the database. Make sure ARB is already running.\n");
            return -1;
        }

    // CALL DEFAULT LANGUAGE PROCEDURE
    XtSetLanguageProc(NULL, NULL, NULL);

    // CREATE THE TOP LEVEL WIDGET (APPLICATION WIDGET)
    m_topwidget= XtVaOpenApplication(&m_xapp, "PGTApp", NULL, 0,
        &argc, argv, NULL,
        sessionShellWidgetClass,
        NULL);

    // CREATE THE PGT MAIN DIALOG WINDOW
    m_maindialog= new mainDialog(m_topwidget);

    // FETCH THE TOP WINDOW
    m_topwindow= XtWindow(m_maindialog->shellWidget());

    // ENTER THE MAIN APPLICATION LOOP (WAIT FOR EVENTS)
    MainLoop();

    // // DISCONNECT THE ARB CONNECTION
    // DB_Disconnect(); // @@@ done by dtor only (pending callbacks!)

    return 0;
}


/****************************************************************************
*  MAIN FUNCTION - NO FURTHER COMMENT NECESSARY... ;-)
****************************************************************************/
int main(int argc, char **argv)
{
    // CREATE THE PGT MAIN EVENT HANDLER
    CMain cmain;

    // RUN PGT MAIN EVENT HANDLER
    int retVal= cmain.Run(argc, argv);

    // EXIT WITH RETURN VALUE 'retVal'
    return retVal;
}
