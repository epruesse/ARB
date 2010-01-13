// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef ANALYZE_WINDOW_HXX
#define ANALYZE_WINDOW_HXX

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Xm/XmAll.h>
#include "dialog.hxx"
#include "plot.hxx"


class analyzeWindow : public MDialog
{
    public:
        analyzeWindow(MDialog*);
        ~analyzeWindow();
        //
        void resizeGnuplot();
    protected:
        void createWindow();
        void createTopToolbar();
        void createLeftToolbar();
    private:
        Widget m_top;
        Widget m_topToolbar;
        Widget m_leftToolbar;
        Widget m_plotManager;
        Widget m_plotArea;

        Plot *m_myplot; // DEBUG

};


// CALLBACK WRAPPER FUNCTIONS (STATIC)
void staticResizeGnuplot(Widget, XtPointer, XtPointer);

#endif // ANALYZE_WINDOW_HXX
