// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef MAIN_DIALOG_H
#define MAIN_DIALOG_H

#include <Xm/XmAll.h>
#include "dialog.hxx"
#include "import_dialog.hxx"
#include "image_dialog.hxx"


class mainDialog : public MDialog
{
    public:
        mainDialog(Widget);
        void ARB_callback();
        void openImportCallback(Widget, XtPointer);
        void openImageCallback(Widget, XtPointer);
        void openAnalyzeCallback(Widget, XtPointer);
        void configCallback(Widget, XtPointer);
        void infoCallback(Widget, XtPointer);
        void exitCallback(Widget, XtPointer);
        void PGTinfoCallback(Widget, XtPointer);
    protected:
        void createWindow();
        void createToolbar();
        void createMainArea();
        void updateListEntries();
    private:
        Widget m_top;
        Widget m_selection_area;
        Widget m_speciesText;
        Widget m_experimentText;
        Widget m_proteomeText;
        Widget m_proteinText;
        importDialog *m_importDialog;
};


// CALLBACK WRAPPER FUNCTIONS (STATIC)
void static_main_ARB_callback(GBDATA *, mainDialog *mD, GB_CB_TYPE);
// void static_main_ARB_callback(GBDATA *, int *, GB_CB_TYPE);
void staticOpenImportCallback(Widget, XtPointer, XtPointer);
void staticOpenImageCallback(Widget, XtPointer, XtPointer);
void staticOpenAnalyzeCallback(Widget, XtPointer, XtPointer);
void staticConfigCallback(Widget, XtPointer, XtPointer);
void staticInfoCallback(Widget, XtPointer, XtPointer);
void staticExitCallback(Widget, XtPointer, XtPointer);
void staticPGTInfoCallback(Widget, XtPointer, XtPointer);

#endif // MAIN_DIALOG_HXX
