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

#include <Xm/XmAll.h>

using namespace std;
#include <vector>


class MDialog
{
    public:
        MDialog(Widget);
        MDialog(MDialog *parent= NULL);
        ~MDialog();
        void setDialogTitle(const char*);
        void setDialogSize(int w, int h);
        Widget shellWidget();
        void show();
        void hide();
        void closeDialog();
        bool isVisible();
        //
        void addChild(MDialog *);
        void removeChild(MDialog *);
        //
        XmString CreateDlgString(const char*);
    protected:
        void createShell(const char*);
        void realizeShell();
        //
        MDialog *m_parent_dialog;
        Widget m_parent_widget;
        Widget m_shell;
        bool m_visible;
        vector<MDialog*> m_children;
        vector<XmString> m_strings;
};


// LOAD PIXMAP
Pixmap PGT_LoadPixmap(const char *name, Screen *s, Pixel fg, Pixel bg);

#endif // MOTIF_DIALOG_H
