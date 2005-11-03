// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
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
#include <Xm/XmAll.h>

#include "arb_interface.hxx"
#include "main_dialog.hxx"


/****************************************************************************
*  PRINT_HELP
*  OUTPUTS THE PROGRAMME USAGE TO STDOUT
****************************************************************************/
int print_help(char *name)
{
    printf("USAGE: %s [option]\n"
            "\t-h, --help      -  display this programme help\n"
            "\t-i, --import    -  show the import dialog    (DISABLED)\n"
            "\t-a, --analyze   -  show the analyze dialog   (DISABLED)\n"
            "\t-v, --visualize -  show the visualize dialog (DISABLED)\n"
            "If no parameters are given the selection dialog with all options is shown.\n",
    name
          );

    return 0;
}


/****************************************************************************
*  MAIN DIALOG - CLEANUP
*  THIS FUNCTION IS CALLED WHENEVER A WM_DELETE_WINDOW EVENT OCCURS
*  (WHEN THE WINDOW IS TO BE CLOSED)
****************************************************************************/
void myCleanup(Widget, XtPointer, XtPointer)
{
    // DISCONNECT FROM ARB DATABASE (AVOIDS UNSTABLE ARB CONDITIONS)
    CONFIG_disconnect();
    ARB_disconnect();
}


/****************************************************************************
*  MAIN DIALOG - XTAPPMAINDIALOG
*  THIS IS MY OWN VERSION OF THE MOTIF EVENT HANDLER - NECESSARY TO CATCH
*  EXIT EVENTS BEFORE X11 GETS THEM...
****************************************************************************/
void myXtAppMainLoop(XtAppContext app, Window w)
{
    XEvent event;
    XDestroyWindowEvent dwe;

    for(;;)
    {
        XtAppNextEvent(app, &event);

        if(event.type == DestroyNotify)
        {
            dwe= event.xdestroywindow;
            if(dwe.window == w) break;
        }

        XtDispatchEvent(&event);
    }
}


/****************************************************************************
*  MAIN FUNCTION
*  DOES A C++ MAIN FUNCTION NEED AN EXPLANATION?
****************************************************************************/
int main(int argc, char **argv)
{
    // PREDEFINE LOCAL VARIABLES
    int returnValue= 0;

    // CHECK IF PARAMETERS WERE GIVEN AND PROCESS THEM...
    if(argc != 1)
    {
        print_help(argv[0]);
        exit(returnValue);
    }

    // TRY TO CONNECT TO THE ARB DATABASE, EXIT IF AN ERROR OCCURS
    if(!ARB_connect(NULL))
    {
        printf("%s ERROR: failed to establish an ARB connection.\n", argv[0]);
        exit(returnValue);
    }

    CONFIG_connect();

    // CREATE APPLICATION CONTEXT
    XtAppContext app;

    // DEFAULT LANGUAGE PROCEDURE
    XtSetLanguageProc(NULL, NULL, NULL);

    // CREATE TOP LEVEL WIDGET (APPLICATION WIDGET)
    Widget top= XtVaOpenApplication(&app, "Application", NULL, 0, &argc, argv, NULL,
        sessionShellWidgetClass,
        NULL);

    // CREATE THE MAIN DIALOG WINDOW
    mainDialog *mD= new mainDialog(top);

    // ENTER MAIN APPLICATION LOOP (WAIT FOR EVENTS)
    // THE ORIGINAL VERSION OF THE HANDLER WAS EXCHANGED BY MY MODIFIED VERSION
    myXtAppMainLoop(app, XtWindow(mD->shellWidget())); // XtAppMainLoop(app);

    // DISCONNECT THE ARB CONNECTION
    CONFIG_disconnect();
    ARB_disconnect();

    // EXIT MAIN FUNCTION WITH RETURN VALUE
    return returnValue;
}
