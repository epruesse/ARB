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
//         void ARBdataButtonCallback(Widget, XtPointer);
//         void TIFFnameButtonCallback(Widget, XtPointer);
//         void setSpecies(char *);
//         void setExperiment(char *);
//         void setProteome(char *);
//         void updateARBText();
//         void imageFileDialogCallback(Widget, XtPointer);
//         void imageRedraw();
    protected:
        void createWindow();
        void createTopToolbar();
        void createLeftToolbar();
//         void updateImage();
//         void setText(char *, int, int);
//         void drawCrosshair(int, int);
//         void displayARBData();
    private:
        Widget m_top;
        Widget m_topToolbar;
        Widget m_leftToolbar;
        Widget m_plotManager;
        Widget m_plotArea;

        Plot *m_myplot; // DEBUG

//         Widget m_drawingArea;
//         Widget m_ARBdata;
//         Widget m_TIFFname;
//         Widget m_fileDialog;
//         //
//         char *m_species;
//         char *m_experiment;
//         char *m_proteome;
//         char *m_x_container;
//         char *m_y_container;
//         char *m_id_container;
//         char *m_vol_container;
//         //
//         bool m_hasFileDialog;
//         bool m_hasTIFFdata;
//         bool m_hasARBdata;
//         //
//         XImage *m_ximage;
//         TIFFimage *m_image;
//         int  m_width;
//         int  m_height;
};


// CALLBACK WRAPPER FUNCTIONS (STATIC)
void staticResizeGnuplot(Widget, XtPointer, XtPointer);

// void staticARBdataButtonCallback(Widget, XtPointer, XtPointer);
// void staticTIFFnameButtonCallback(Widget, XtPointer, XtPointer);
// void staticImageSpeciesCallback(Widget, XtPointer, XtPointer);
// void staticImageExperimentCallback(Widget, XtPointer, XtPointer);
// void staticImageProteomeCallback(Widget, XtPointer, XtPointer);
// void staticImageFileDialogCloseCallback(Widget, XtPointer, XtPointer);
// void staticImageFileDialogCallback(Widget, XtPointer, XtPointer);
// void staticImageRedrawCallback(Widget, XtPointer, XtPointer);


#endif // ANALYZE_WINDOW_HXX
