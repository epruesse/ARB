// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef SELECTION_DIALOG_H
#define SELECTION_DIALOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Xm/XmAll.h>
#include "dialog.hxx"

#define SELECTION_DIALOG_READ  1
#define SELECTION_DIALOG_WRITE 2
#define SELECTION_DIALOG_RW    3


class selectionDialog : public MDialog
{
    public:
        selectionDialog(Widget, MDialog*, int);
        ~selectionDialog();
        //
        void setSpeciesCallback(XtCallbackProc);
        void triggerSpeciesChange();
        void setExperimentCallback(XtCallbackProc);
        void triggerExperimentChange();
        void setProteomeCallback(XtCallbackProc);
        void triggerProteomeChange();
        //
        void exitButtonCallback(Widget, XtPointer);
        void speciesCallback(Widget, XtPointer);
        void experimentCallback(Widget, XtPointer);
        void proteomeListCallback(Widget, XtPointer);
        void proteomeTextCallback(Widget, XtPointer);
    protected:
        void createWindow();

    private:
        char *m_species;
        char *m_experiment;
        char *m_proteome;
        //
        Widget m_top;
        Widget m_speciesList;
        Widget m_experimentList;
        Widget m_proteomeList;
        Widget m_proteomeText;
        Widget m_warning_label;
        //
        XtCallbackProc m_speciesCallback;
        XtCallbackProc m_experimentCallback;
        XtCallbackProc m_proteomeCallback;
        //
        int m_type;
        //
        bool m_hasSpeciesCallback;
        bool m_hasExperimentCallback;
        bool m_hasProteomeCallback;
        bool m_ignoreProteomeCallback;
        //
        static bool m_opened;
};


// CALLBACK WRAPPER FUNCTIONS (STATIC)
void staticExitButtonCallback(Widget, XtPointer, XtPointer);
void staticSpeciesCallback(Widget, XtPointer, XtPointer);
void staticExperimentCallback(Widget, XtPointer, XtPointer);
void staticProteomeListCallback(Widget, XtPointer, XtPointer);
void staticProteomeTextCallback(Widget, XtPointer, XtPointer);


#endif // SELECTION_DIALOG_H
