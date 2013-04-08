// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef IMAGE_DIALOG_H
#define IMAGE_DIALOG_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Xm/XmAll.h>
#include "dialog.hxx"
#include "tiff_image.hxx"
#include "arb_interface.hxx"

using namespace std;
#include <vector>
#include <map>

struct ltstr
{
    bool operator()(const char* s1, const char* s2) const
    {
        return strcmp(s1, s2) < 0;
    }
};

// SPOT STRUCT
typedef struct _SPOT
{
    float x;
    float y;
    int area;
    int diameter;
    char *text;
    char *id;
    bool selected;
    bool marked;
    GBDATA *gbdata;
} SPOT;

// COLOR STRUCT
typedef struct _RGB
{
    int r;
    int g;
    int b;
} RGB;


class imageDialog : public MDialog { // derived from a Noncopyable
    public:
        imageDialog(MDialog*);
        ~imageDialog();
        //
        void ARBdataButtonCallback(Widget, XtPointer);
        void TIFFnameButtonCallback(Widget, XtPointer);
        void setSpecies(const char *);
        void setExperiment(const char *);
        void setProteome(const char *);
        void updateARBText();
        void imageFileDialogCallback(Widget, XtPointer);
        void imageRedrawCallback();
        void imageRedraw();
        void crosshairButtonCallback(Widget, XtPointer);
        void textButtonCallback(Widget, XtPointer);
        void textOnlyButtonCallback(Widget, XtPointer);
        void circleButtonCallback(Widget, XtPointer);
        void markedOnlyButtonCallback(Widget, XtPointer);
        void markAllButtonCallback(Widget, XtPointer);
        void markInvertButtonCallback(Widget, XtPointer);
        void markNoneButtonCallback(Widget, XtPointer);
        bool setAllMarker(int);
        void imageEventCallback(Widget, XtPointer);
        void markSpotAtPos(int, int, int);
        void lockToggleButtonCallback(Widget, XtPointer);
        void ARB_protein_callback();
        void ARB_gene_callback();
        void PGT_config_callback();
        void updateGeneButtonCallback();
        bool mark_Spots2Genes();
        bool mark_Genes2Spots();
        void markWithInfo();
    protected:
        void createWindow();
        void createTopToolbar();
        void createLeftToolbar();
        int updateImage();
        int blankImage();
        int fillBlankImage();
        int getSpotMaxDimensions();
        void setText(const char *, int, int);
        void drawCrosshair(int, int);
        void drawCircle(int, int, int, int, int, int);
        void drawSpots();
        void setColor(Display *, GC, int, int, int);
        char *get_ARB_image_path();
        void set_ARB_image_path(const char *);
        bool getSettings();
        bool createSpotList();
        bool createDescriptions();
        bool updateSelectedGene();
        void updateStatusLabel();
        void genGeneKey(char *, char *, float , float);
    private:
        Widget m_top;
        Widget m_topToolbar;
        Widget m_leftToolbar;
        Widget m_drawingArea;
        Widget m_statusLabel;
        Widget m_ARBdata;
        Widget m_TIFFname;
        Widget m_fileDialog;
        Widget m_LockToggleButton;
        Widget m_UpdateGeneButton;
        //
        vector<SPOT> m_spotList;
        map<char *, char *, ltstr> m_descriptorList;
        map<char *, GBDATA*, ltstr> m_gene_GBDATA_map;
        //
        char *m_filename;
        char *m_species;
        char *m_experiment;
        char *m_proteome;
        //
        // DEBUG -- PRESET -- DEBUG -- PRESET
        const char *m_x_container;
        const char *m_y_container;
        const char *m_id_container;
        const char *m_vol_container;
        const char *m_area_container;
        const char *m_avg_container;
        // DEBUG -- PRESET -- DEBUG -- PRESET
        //
        bool m_changeInProgress;
        bool m_hasFileDialog;
        bool m_hasTIFFdata;
        bool m_hasImagedata;
        bool m_hasARBdata;
        bool m_lockVisualization;
        bool m_updateGene;
        //
        XImage *m_ximage;
        TIFFimage *m_image;
        Pixmap m_pixmap;
        int  m_width;
        int  m_height;
        int  m_numSpots;
        int  m_numMarkedSpots;
        int  m_numSelectedSpots;
        char *m_selectedSpotName;
        //
        bool m_crosshairFlag;
        bool m_circleFlag;
        bool m_labelFlag;
        bool m_linkedOnlyFlag;
        bool m_textOnlyFlag;
        bool m_markedOnlyFlag;
        //
        RGB m_crosshairColor;
        RGB m_unmarkedColor;
        RGB m_markedColor;
        RGB m_selectedColor;
        RGB m_textColor;
};


#endif // IMAGE_DIALOG_H
