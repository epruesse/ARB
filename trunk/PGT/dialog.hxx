// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef MOTIF_DIALOG_H
#define MOTIF_DIALOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Xm/XmAll.h>


class MDialog
{
    public:
        MDialog(Widget, MDialog*);
        ~MDialog();
        void setWindowName(char*);
        Widget shellWidget();
        Widget parentWidget();
        void show();
        void hide();
        void closeDialog();
        bool isEnabled();
        void windowCloseCallback(Widget, XtPointer);
    protected:
        void createShell(char*);
        void realizeShell();
        //
        MDialog *m_parent_dialog;
        Widget m_parent;
        Widget m_shell;
        //
        bool m_modal;
        bool m_enabled;
    private:
};


// STATIC CALLBACK
void staticWindowCloseCallback(Widget, XtPointer, XtPointer);

#endif // MOTIF_DIALOG_HXX
