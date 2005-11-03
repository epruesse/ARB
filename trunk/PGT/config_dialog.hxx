// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef CONFIG_DIALOG_H
#define CONFIG_DIALOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Xm/XmAll.h>
#include "dialog.hxx"
#include "arb_interface.hxx"


class configDialog : public MDialog
{
    public:
        configDialog(Widget, MDialog*);
        ~configDialog();
        //
        bool setDefault(bool);
        void getCONFIGS();
        void setCONFIGS();
        void textChangedCallback(Widget);
    protected:
        void createWindow();
        void updateColors();
        void drawColoredSpot(Widget, int, int, int);
        void setColor(Display *, GC, int, int, int);

    private:
        char *m_crosshairColorString;
        char *m_unmarkedColorString;
        char *m_markedColorString;
        char *m_selectedColorString;
        char *m_textColorString;
        char *m_id_protein;
        char *m_id_gene;
        char *m_info_protein;
        char *m_info_gene;
        //
        Widget m_top;
        Widget m_crosshairText;
        Widget m_crosshairArea;
        Widget m_unmarkedText;
        Widget m_unmarkedArea;
        Widget m_markedText;
        Widget m_markedArea;
        Widget m_selectedText;
        Widget m_selectedArea;
        Widget m_textText;
        Widget m_textArea;
        Widget m_ID_ProteinText;
        Widget m_ID_GeneText;
        Widget m_cancelButton;
        Widget m_defaultButton;
        Widget m_okButton;
        Widget m_info_ProteinText;
        Widget m_info_GeneText;
};


// HELPER CONVERSION BETWEEN STRING AND RGB COLORS
char *rgb2hex(int, int, int);
bool hex2rgb(int *, int *, int *, char *);


// CALLBACK WRAPPER FUNCTIONS (STATIC)
void staticCancelButtonCallback(Widget, XtPointer, XtPointer);
void staticDefaultButtonCallback(Widget, XtPointer, XtPointer);
void staticOkButtonCallback(Widget, XtPointer, XtPointer);
void staticTextChangedCallback(Widget, XtPointer, XtPointer);


#endif // CONFIG_DIALOG_H

