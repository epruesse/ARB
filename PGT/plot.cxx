// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$


#include "plot.hxx"
#include <locale.h>


#define GNUPLOT_WINDOW_WIDTH  321
#define GNUPLOT_WINDOW_HEIGHT 123


/****************************************************************************
*  PLOT - CONSTRUCTOR
****************************************************************************/
Plot::Plot(Display *d, Window w)
{
    // SET PARENT WINDOW (WHERE THE GNUPLOT WINDOW WILL BE EMBEDDED)
    m_parent_display= d;
    // m_parent_shell= s;
    m_parent_window= w;

    // RESERVE SPACE FOR THE GNUPLOT WINDOW TITLE
    m_title= (char *)malloc(64*sizeof(char));

    // INITIALIZE THE PIPE VARIABLES
    m_command_pipe= NULL;
    m_has_command_pipe= false;
    m_data_pipe= NULL;
    m_has_data_pipe= false;
    m_temp_name= NULL;
}


/****************************************************************************
*  PLOT - DESTRUCTOR
****************************************************************************/
Plot::~Plot()
{
    // CLOSE COMMAND PIPE (IF NEEDED)
    if(m_has_command_pipe) close_command_pipe();

    // FREE ALLOCATED SPACE
    free(m_title);
}


/****************************************************************************
*  PLOT - SET THE PARENT WINDOW
****************************************************************************/
void Plot::setParentWindow(Window w) { m_parent_window= w; }


/****************************************************************************
*  PLOT - EMBED GNUPLOT X11 WINDOW INTO ANOTHER WINDOW
****************************************************************************/
void Plot::embed()
{
    // DO NOT EMBED IF THE PARENT WINDOW ID IS NULL OR NO DISPLAY IS SET
    if(!m_parent_window || !m_parent_display) return;

    // CREATE AND INIT LOCAL VARIABLES
    int countdown= 100;
    XEvent event;
    char *window_name;
    m_gnuplot_window= 0;

    // SET THE EVENT MASK TO 'SubstructureNotifyMask'
    XSelectInput(m_parent_display, DefaultRootWindow(m_parent_display), SubstructureNotifyMask);

    // LOOP WHILE COUNTDOWN > 0 (AVOIDS INFINITE LOOPS)
    while(countdown)
    {
        // CAUTION:
        // XNEXTEVENT BELOW ALSO CATCHES OTHER EVENTS AND REMOVES THEM FROM
        // THE QUEUE. THIS MAY CAUSE PROBLEMS WITH OTHER WINDOWS/WIDGETS,
        // I.E. FALSE REDRAWN WINDOWS OR LOST SIGNALS FROM WINDOWS/WIDGETS.

        // WAIT FOR AN EVENT TO OCCUR
        XNextEvent(m_parent_display, &event);

        // IS IT A CREATENOTIFY EVENT?
        if(event.type == CreateNotify)
        {
            // FETCH THE CREATEWINDOWEVENT STRUCT
            XCreateWindowEvent cwe= event.xcreatewindow;

            // FETCH THE NAME OF THE WINDOW WHICH THREW THE EVENT (POSSIBLY NULL)
            XFetchName(m_parent_display, cwe.window, &window_name);

            // IF THE NAME MATCHES OUR GNUPLOT WINDOW OR THE SIZE IS RIGHT
            // MARK IT AND EXIT THE LOOP
            if((window_name && (strcmp(window_name, m_title) == 0)) ||
               ((cwe.width == GNUPLOT_WINDOW_WIDTH) && (cwe.height == GNUPLOT_WINDOW_HEIGHT)))
            {
                // GET THE WINDOW ID OF THE NEW GNUPLOT WINDOW
                m_gnuplot_window= cwe.window;

                // SET BORDER WIDTH TO 0 IF NECESSARY
                if(cwe.border_width) XSetWindowBorderWidth(m_parent_display, m_gnuplot_window, 0);

                // SET COUNTDOWN= 0 TO EXIT LOOP
                countdown= 0;
            }
            else countdown--; // DECREASE LOOP COUNTER
        }
        else
        {
            // IF THE EVENT IS NOT OF INTEREST FOR US RETURN IT TO THE QUEUE
            // XPutBackEvent(m_parent_display, &event);

            // DECREASE LOOP COUNTER
            countdown--;
        }

        // THIS XSYNC ERASES THE EVENT QUEUE (ALSO MAKES XPUTBACKEVENT OBSOLETE)
        // XSync(m_parent_display, true);
    }

    // EXIT, IF WE WERE NOT ABLE TO FIND THE GNUPLOT WINDOW
    if (!m_gnuplot_window) return;

    // SYNC AND WITHDRAW THE ORIGINAL WINDOW
    XSync(m_parent_display, false);
    XWithdrawWindow(m_parent_display, m_gnuplot_window, 0);

    // THE REPARENTING MUST BE TRIED MULTIPLE TIMES TO WORK.
    // DONT ASK ME WHY...
    for(int c= 0; c < 100; c++)
    {
        XReparentWindow(m_parent_display, m_gnuplot_window, m_parent_window, 0, 0);
        XSync(m_parent_display, false);
    }

    // SYNC AND MAP THE NEW GNUPLOT WINDOW
    XMapWindow(m_parent_display, m_gnuplot_window);
    XSync(m_parent_display, false);
}


/****************************************************************************
*  PLOT - RESIZE THE GNUPLOT WINDOW
****************************************************************************/
void Plot::resizeWindow(int x, int y, int width, int height)
{
    // RETURN IF NO WINDOW IS VISIBLE
    if(!m_has_command_pipe) return;

    // RESIZE THE GNUPLOT WINDOW
    XMoveResizeWindow(m_parent_display, m_gnuplot_window, x, y, width, height);
    XSync(m_parent_display, false);
}


/****************************************************************************
*  PLOT - OPEN/INIT THE COMMAND PIPE
****************************************************************************/
FILE *Plot::open_command_pipe()
{
    // LOCAL GNUPLOT WINDOW ID (INCREMENTED BY EACH CALL TO MAKE IT UNIQUE)
    static int m_local_ID= 0;

    char *buf= (char *)malloc(1024*sizeof(char));

    // CREATE GNUPLOT TITLE (NEEDED FOR LATER RECOGNITION)
    // m_local_ID++;
    sprintf(m_title, "plot%dclient%d", getpid(), ++m_local_ID);
    sprintf(buf, "gnuplot -geometry %dx%d -title '%s'",
            GNUPLOT_WINDOW_WIDTH, GNUPLOT_WINDOW_HEIGHT, m_title);

    // CREATE A TEMPORARY FILENAME (WARNING: TMPNAM() IS DEPRECATED)
    // if((m_temp_name= tmpnam(NULL)) == NULL) return NULL;

    // CREATE A TEMPORARY FILENAME (USING MKSTEMP)
    m_temp_name= (char *)malloc(17 * sizeof(char));
    strcpy(m_temp_name, "/tmp/plot_XXXXXX");
        if((mkstemp(m_temp_name)) == -1) return NULL;

    // AS MKTEMP ALSO CREATES THE FILE ITSELF (WHAT WE DONT WANT) REMOVE IT
    // RIGHT AFTER ITS CREATION FOR FURTHER USE AS A PIPE
    unlink(m_temp_name);

    // CREATE A FIFO PIPE
    if(mkfifo(m_temp_name, S_IRUSR | S_IWUSR) != 0) return NULL;
    m_command_pipe= popen(buf, "w");

    m_has_command_pipe= true;

    free(buf);

    return m_command_pipe;
}


/****************************************************************************
*  PLOT - CLOSE THE COMMAND PIPE
****************************************************************************/
void Plot::close_command_pipe()
{
    // CLOSE DATA PIPE (IF NEEDED)
    if(m_has_data_pipe) close_data_pipe();

    // CLOSE DATA PIPE
    pclose(m_command_pipe);

    // REMOVE TEMP FILE (WHICH WAS USED AS A PIPE)
    unlink(m_temp_name);

    if(m_temp_name) free(m_temp_name);
    m_temp_name= NULL;

    // SET COMMAND PIPE FLAG TO FALSE
    m_has_command_pipe= false;
}


/****************************************************************************
*  PLOT - OPEN THE DATA PIPE
****************************************************************************/
FILE *Plot::open_data_pipe()
{
    if(!m_has_command_pipe) return NULL;

    // fprintf(m_command_pipe, "plot \"%s\" with lines\n", m_temp_name);
    fprintf(m_command_pipe, "plot \"%s\" with linespoints\n", m_temp_name);

    fflush(m_command_pipe);
    m_data_pipe= fopen(m_temp_name, "w");

    // SET DATA PIPE FLAG TO TRUE
    m_has_data_pipe= true;

    return m_data_pipe;
}


/****************************************************************************
*  PLOT - CLOSE THE DATA PIPE
****************************************************************************/
void Plot::close_data_pipe()
{
    if(!m_has_data_pipe) return;

    // CLOSE DATA PIPE
    fclose(m_data_pipe);

    // SET DATA PIPE FLAG TO FALSE
    m_has_data_pipe= false;
}


/****************************************************************************
*  PLOT - SEND A STRING TO THE COMMAND PIPE
****************************************************************************/
void Plot::send_command_pipe(char *s)
{
    // RETURN, IF THERE IS NO OPEN COMMAND PIPE
    if(!m_has_command_pipe) return;

    // WRITE THE STRING S TO THE DATA PIPE
    fputs(s, m_command_pipe);
}


/****************************************************************************
*  PLOT - SEND A STRING TO THE DATA PIPE
****************************************************************************/
void Plot::send_data_pipe(char *s)
{
    // RETURN, IF THERE IS NO OPEN DATA PIPE
    if(!m_has_data_pipe) return;

    // WRITE THE STRING S TO THE DATA PIPE
    fputs(s, m_data_pipe);
}


/****************************************************************************
*  PLOT - CREATE A DEMO PLOT (FOR TESTING PURPOSES ONLY)
****************************************************************************/
void Plot::demo()
{
    // AVOID WRONG FLOAT OUTPUT DUE TO GERMAN LOCALES ( ',' -> '.')
    setlocale(LC_ALL, "C");

    if(!open_command_pipe()) return;

    send_command_pipe("set title \"Überschrift - Proteomdaten\"\n");
    send_command_pipe("set xlabel \"Protein\"\n");
    send_command_pipe("set ylabel \"Abweichung\"\n");

    if(!open_data_pipe()) return;

    double a, b;

    // FIRST DATASET
    for (int i= 1; i < 20; i++)
    {
        a= i / 10.0;
        b= cos(a);

        fprintf(m_data_pipe, "%f %f\n", a, b);
    }

    // EMPTY LINE (SEPARATOR BETWEEN DATASETS)
    fprintf(m_data_pipe, "\n");

    // SECOND DATASET
    for (int i= 1; i < 20; i++)
    {
        a= i / 10.0;
        b= (double)(rand()%10);

        fprintf(m_data_pipe, "%f %f\n", a, b);
    }

    close_data_pipe();
    // close_command_pipe();

    // RESET LOCALES
    setlocale(LC_ALL, "");

    // DEBUG
    embed();
    // DEBUG
}

