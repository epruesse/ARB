// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef ENTRY_SELECTION_DIALOG_H
#define ENTRY_SELECTION_DIALOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Xm/XmAll.h>
#include "dialog.hxx"


class entrySelectionDialog : public MDialog
{
    public:
        entrySelectionDialog(MDialog*);
        ~entrySelectionDialog();
        //
        void setListCallback(XtCallbackProc);
        void triggerListChange();
        //
        void exitButtonCallback(Widget, XtPointer);
        void listCallback(Widget, XtPointer);
    protected:
        void createWindow();

    private:
        char *m_entry;
        Widget m_top;
        Widget m_list;
        XtCallbackProc m_listCallback;
        bool m_hasListCallback;
        static bool m_opened;
};


// CALLBACK WRAPPER FUNCTIONS (STATIC)
void staticEntrySelExitButtonCallback(Widget, XtPointer, XtPointer);
void staticListCallback(Widget, XtPointer, XtPointer);


#endif // ENTRY_SELECTION_DIALOG_H
