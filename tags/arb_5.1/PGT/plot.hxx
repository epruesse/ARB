// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef PLOT_H
#define PLOT_H


#include <X11/Xlib.h>
#include <Xm/XmAll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>


class Plot
{
    public:
        Plot(Display *, Window);
        ~Plot();
        void demo();
        void setParentWindow(Window);
        void resizeWindow(int, int, int, int);
    protected:
        FILE *open_command_pipe();
        void close_command_pipe();
        FILE *open_data_pipe();
        void close_data_pipe();
        void send_command_pipe(char *);
        void send_data_pipe(char *);
    private:
        void embed();
        //
        FILE *m_command_pipe;
        bool m_has_command_pipe;
        FILE *m_data_pipe;
        bool m_has_data_pipe;
        Window m_gnuplot_window;
        Window m_parent_window;
        Display *m_parent_display;
        char *m_title;
        char *m_temp_name;
};

#endif // PLOT_HXX
