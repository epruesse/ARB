// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef IMPORT_DIALOG_H
#define IMPORT_DIALOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Xm/XmAll.h>
#include "dialog.hxx"
#include "file_import.hxx"


class importDialog : public MDialog
{
    public:
        importDialog(MDialog*);
        ~importDialog();
        void getFilenameCallback(Widget, XtPointer);
        void fileDialogCallback(Widget, XtPointer);
        void ARBdestinationCallback(Widget, XtPointer);
//         void dataChangedCallback(Widget, XtPointer);
        void setSpecies(char *);
        void setExperiment(char *);
        void setProteome(char *);
        void updateARBText();
        void clearEntriesButtonCallback(Widget, XtPointer);
        void openFileButtonCallback(Widget, XtPointer);
        void columnDownCallback(Widget, XtPointer);
        void columnUpCallback(Widget, XtPointer);
        void headerChangedCallback(Widget, XtPointer);
        void updateHeaderEntries();
        void updateSampleList();
        void copyHeaderEntries();
        void importDataCallback(Widget, XtPointer);
        void changedDatatypeCallback(Widget, XtPointer);
        void sampleListEntryCallback(Widget, XtPointer);
        void externHeaderSelectionCallback(Widget, XtPointer);
        void setHeaderName(char *);
    protected:
        void createWindow();
        void createSelectionArea();
        void createImportArea();
        void fillImportTypeCombobox();
        void fillARBTypeCombobox();
    private:
        Widget m_top;
        Widget m_fileDialog;
        Widget m_fileTypeWidget;
        Widget m_filenameWidget;
        Widget m_datasetWidget;
        Widget m_selectorText;
        Widget m_columnHeaderWidget;
        Widget m_containerHeaderWidget;
        Widget m_sampleList;
        Widget m_fieldTypeWidget;
        Widget m_sampleScale;
        //
        bool m_hasFileDialog;
        bool m_hasSelectionDialog;
        //
        char *m_species;
        char *m_experiment;
        char *m_proteome;
        //
        importTable *m_table;
        bool m_hasTableData;
        int m_activeHeader;
        //char **m_ARBheader;
        //
        XSLTimporter *m_xslt;
        //
        XmString m_str_int;
        XmString m_str_float;
        XmString m_str_string;
};

#endif // IMPORT_DIALOG_H
