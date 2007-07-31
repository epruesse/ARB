// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$


#include <time.h>
#include "arb_interface.hxx"
#include "config_dialog.hxx"


/****************************************************************************
*  CONFIG DIALOG - CONSTRUCTOR
****************************************************************************/
configDialog::configDialog(MDialog *d)
    : MDialog(d)
{
    // PRDEFINE VARIABLES
    m_top= NULL;
    m_crosshairText= NULL;
    m_crosshairArea= NULL;
    m_unmarkedText= NULL;
    m_unmarkedArea= NULL;
    m_markedText= NULL;
    m_markedArea= NULL;
    m_selectedText= NULL;
    m_selectedArea= NULL;
    m_textText= NULL;
    m_textArea= NULL;
    m_ID_ProteinText= NULL;
    m_ID_GeneText= NULL;
    //
    m_crosshairColorString= NULL;
    m_unmarkedColorString= NULL;
    m_markedColorString= NULL;
    m_selectedColorString= NULL;
    m_textColorString= NULL;
    m_id_protein= NULL;
    m_id_gene= NULL;
    m_info_protein= NULL;
    m_info_gene= NULL;

    // CHECK THE PGT AWARS (SET DEFAULT IF NEEDED)
    checkCreateAWARS();

    // CREATE SHELL FOR THIS DIALOG
    createShell("");

    // CALL CREATE WINDOW FUNCTION
    createWindow();

    // SET WINDOW WIDTH
    XtVaSetValues(m_shell,
        XmNwidth, 300,
        XmNheight, 500,
        NULL);

    // REALIZE WINDOW
    realizeShell();

    // SET WINDOW LABEL
    setDialogTitle("PGT - Configuration");

    // DESELECT ALL WINDOW BUTTONS EXCEPT CLOSE AND MOVE
    XtVaSetValues(m_shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE, NULL);

    // FETCH CONFIGURATION (AWARS)
    getCONFIGS();
}


/****************************************************************************
*  CONFIG DIALOG - DESTRUCTOR
****************************************************************************/
configDialog::~configDialog()
{
    // FREE UNNECESSARY STRINGS
    freeStrings();
}


/****************************************************************************
*  CONFIG DIALOG - FREE LOCAL STRINGS (IF NECESSARY)
****************************************************************************/
void configDialog::freeStrings()
{
    if(m_crosshairColorString) free(m_crosshairColorString);
    if(m_unmarkedColorString) free(m_unmarkedColorString);
    if(m_markedColorString) free(m_markedColorString);
    if(m_textColorString) free(m_textColorString);
    if(m_id_protein) free(m_id_protein);
    if(m_id_gene) free(m_id_gene);
    if(m_info_protein) free(m_info_protein);
    if(m_info_gene) free(m_info_gene);
}


/****************************************************************************
*  CONFIG DIALOG - CREATE WINDOW
****************************************************************************/
void configDialog::createWindow()
{
    // TOP LEVEL WIDGET
    m_top= XtVaCreateManagedWidget("top",
        xmFormWidgetClass, m_shell,
        XmNorientation, XmVERTICAL,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        NULL);

    // LABEL WIDGET
    Widget label_01= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, CreateDlgString("VISUALIZATION COLORS (#RGB)"),
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // LABEL WIDGET (COLOR ROW 01 -- CROSSHAIR COLOR)
    Widget label_02= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, CreateDlgString("Crosshair color:"),
        XmNheight, 30,
        XmNwidth, 150,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_01,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    // TEXT FIELD - CROSSHAIR COLOR
    m_crosshairText= XtVaCreateManagedWidget("textField",
        xmTextWidgetClass, m_top,
        XmNheight, 30,
        XmNwidth, 100,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_01,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, label_02,
        NULL);
    XtAddCallback(m_crosshairText, XmNvalueChangedCallback, staticTextChangedCallback, this);

    // DRAWING AREA WIDGET - CROSSHAIR COLOR
    m_crosshairArea= XtVaCreateManagedWidget("drawingArea",
        xmDrawingAreaWidgetClass, m_top,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_01,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, m_crosshairText,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_crosshairArea, XmNexposeCallback, staticTextChangedCallback, this);

    // LABEL WIDGET (COLOR ROW 02 -- UNMARKED  COLOR)
    Widget label_03= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, CreateDlgString("Unmarked protein color:"),
        XmNheight, 30,
        XmNwidth, 150,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_02,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    // TEXT FIELD - UNMARKED COLOR
    m_unmarkedText= XtVaCreateManagedWidget("textField",
        xmTextWidgetClass, m_top,
        XmNheight, 30,
        XmNwidth, 100,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_crosshairText,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, label_03,
        NULL);
    XtAddCallback(m_unmarkedText, XmNvalueChangedCallback, staticTextChangedCallback, this);

    // DRAWING AREA WIDGET - UNMARKED COLOR
    m_unmarkedArea= XtVaCreateManagedWidget("drawingArea",
        xmDrawingAreaWidgetClass, m_top,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_crosshairArea,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, m_unmarkedText,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_unmarkedArea, XmNexposeCallback, staticTextChangedCallback, this);

    // LABEL WIDGET (COLOR ROW 03 -- MARKED COLOR)
    Widget label_04= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, CreateDlgString("Marked protein color:"),
        XmNheight, 30,
        XmNwidth, 150,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_03,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    // TEXT FIELD - MARKED COLOR
    m_markedText= XtVaCreateManagedWidget("textField",
        xmTextWidgetClass, m_top,
        XmNheight, 30,
        XmNwidth, 100,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_unmarkedText,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, label_04,
        NULL);
    XtAddCallback(m_markedText, XmNvalueChangedCallback, staticTextChangedCallback, this);

    // DRAWING AREA WIDGET - MARKED COLOR
    m_markedArea= XtVaCreateManagedWidget("drawingArea",
        xmDrawingAreaWidgetClass, m_top,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_unmarkedArea,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, m_markedText,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_markedArea, XmNexposeCallback, staticTextChangedCallback, this);

    // LABEL WIDGET (COLOR ROW 04 -- PROTEIN COLOR)
    Widget label_05= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, CreateDlgString("Selected protein color:"),
        XmNheight, 30,
        XmNwidth, 150,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_04,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    // TEXT FIELD - CROSSHAIR COLOR
    m_selectedText= XtVaCreateManagedWidget("textField",
        xmTextWidgetClass, m_top,
        XmNheight, 30,
        XmNwidth, 100,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_markedText,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, label_05,
        NULL);
    XtAddCallback(m_selectedText, XmNvalueChangedCallback, staticTextChangedCallback, this);

    // DRAWING AREA WIDGET - CROSSHAIR COLOR
    m_selectedArea= XtVaCreateManagedWidget("drawingArea",
        xmDrawingAreaWidgetClass, m_top,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_markedArea,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, m_selectedText,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_selectedArea, XmNexposeCallback, staticTextChangedCallback, this);

    // LABEL WIDGET (COLOR ROW 05 -- TEXT COLOR)
    Widget label_0a= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, CreateDlgString("Text color:"),
        XmNheight, 30,
        XmNwidth, 150,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_05,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    // TEXT FIELD - TEXT COLOR
    m_textText= XtVaCreateManagedWidget("textField",
        xmTextWidgetClass, m_top,
        XmNheight, 30,
        XmNwidth, 100,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_selectedText,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, label_0a,
        NULL);
    XtAddCallback(m_textText, XmNvalueChangedCallback, staticTextChangedCallback, this);

    // DRAWING AREA WIDGET - TEXT COLOR
    m_textArea= XtVaCreateManagedWidget("drawingArea",
        xmDrawingAreaWidgetClass, m_top,
        XmNheight, 30,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_selectedArea,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, m_textText,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_textArea, XmNexposeCallback, staticTextChangedCallback, this);

    // CREATE HORIZONTAL SEPARATOR
    Widget separator_01= XtVaCreateManagedWidget("separator",
        xmSeparatorWidgetClass, m_top,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_0a,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNorientation, XmHORIZONTAL,
        NULL);

    // LABEL WIDGET (PROTEIN ID TEXT)
    Widget label_06= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, CreateDlgString("PROTEIN IDENTIFIER"),
        XmNheight, 30,
        XmNwidth, 150,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, separator_01,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);

    // LABEL WIDGET (GENE ID TEXT)
    Widget label_07= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, CreateDlgString("GENE IDENTIFIER"),
        XmNheight, 30,
        XmNwidth, 150,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, separator_01,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, label_06,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // TEXT FIELD - PROTEIN IDENTIFIER
    m_ID_ProteinText= XtVaCreateManagedWidget("textField",
        xmTextWidgetClass, m_top,
        XmNheight, 30,
        XmNwidth, 150,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_06,
        XmNleftAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_ID_ProteinText, XmNvalueChangedCallback, staticTextChangedCallback, this);

    // TEXT FIELD - GENE IDENTIFIER
    m_ID_GeneText= XtVaCreateManagedWidget("textField",
        xmTextWidgetClass, m_top,
        XmNheight, 30,
        XmNwidth, 150,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_07,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, m_ID_ProteinText,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_ID_GeneText, XmNvalueChangedCallback, staticTextChangedCallback, this);

    // CREATE HORIZONTAL SEPARATOR
    Widget separator_02= XtVaCreateManagedWidget("separator",
        xmSeparatorWidgetClass, m_top,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_ID_ProteinText,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        XmNorientation, XmHORIZONTAL,
        NULL);

    // LABEL WIDGET (PROTEIN INFOS)
    Widget label_08= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, CreateDlgString("DISPLAYED PROTEIN INFO"),
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, separator_02,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // TEXT FIELD - PROTEIN INFOS
    m_info_ProteinText= XtVaCreateManagedWidget("textField",
        xmTextWidgetClass, m_top,
        XmNheight, 80,
        XmNeditMode, XmMULTI_LINE_EDIT,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_08,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_info_ProteinText, XmNvalueChangedCallback, staticTextChangedCallback, this);

    // LABEL WIDGET (GENE INFOS)
    Widget label_09= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, m_top,
        XmNlabelString, CreateDlgString("DISPLAYED GENE INFO"),
        XmNheight, 30,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, m_info_ProteinText,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // TEXT FIELD - GENE INFOS
    m_info_GeneText= XtVaCreateManagedWidget("textField",
        xmTextWidgetClass, m_top,
        XmNheight, 80,
        XmNeditMode, XmMULTI_LINE_EDIT,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, label_09,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_info_GeneText, XmNvalueChangedCallback, staticTextChangedCallback, this);

    m_cancelButton= XtVaCreateManagedWidget("button",
        xmPushButtonWidgetClass, m_top,
        XmNlabelString, CreateDlgString("Cancel"),
        XmNwidth, 100,
        XmNheight, 30,
        XmNleftAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_cancelButton, XmNactivateCallback, staticCancelButtonCallback, this);

    m_okButton= XtVaCreateManagedWidget("button",
        xmPushButtonWidgetClass, m_top,
        XmNlabelString, CreateDlgString("Ok"),
        XmNwidth, 100,
        XmNheight, 30,
        XmNrightAttachment, XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_okButton, XmNactivateCallback, staticOkButtonCallback, this);

    m_defaultButton= XtVaCreateManagedWidget("button",
        xmPushButtonWidgetClass, m_top,
        XmNlabelString, CreateDlgString("Default"),
        XmNwidth, 100,
        XmNheight, 30,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, m_cancelButton,
        XmNrightAttachment, XmATTACH_WIDGET,
        XmNrightWidget, m_okButton,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);
    XtAddCallback(m_defaultButton, XmNactivateCallback, staticDefaultButtonCallback, this);

}


bool configDialog::setDefault(bool global)
{
    // SET DEFAULT VALUES
    m_crosshairColorString= const_cast<char*>(DEFAULT_COLOR_CROSSHAIR);
    m_unmarkedColorString=  const_cast<char*>(DEFAULT_COLOR_UNMARKED);
    m_markedColorString=    const_cast<char*>(DEFAULT_COLOR_MARKED);
    m_selectedColorString=  const_cast<char*>(DEFAULT_COLOR_SELECTED);
    m_textColorString=      const_cast<char*>(DEFAULT_COLOR_TEXT);
    m_id_protein=           const_cast<char*>(DEFAULT_ID_PROTEIN);
    m_id_gene=              const_cast<char*>(DEFAULT_ID_GENE);
    m_info_protein=         const_cast<char*>(DEFAULT_INFO_PROTEIN);
    m_info_gene=            const_cast<char*>(DEFAULT_INFO_GENE);

    // SET GLOBAL SETTINGS ALSO TO DEFAULT?
    if(global)
    {
        set_CONFIG(CONFIG_PGT_COLOR_CROSSHAIR, m_crosshairColorString);
        set_CONFIG(CONFIG_PGT_COLOR_UNMARKED, m_unmarkedColorString);
        set_CONFIG(CONFIG_PGT_COLOR_MARKED, m_markedColorString);
        set_CONFIG(CONFIG_PGT_COLOR_SELECTED, m_selectedColorString);
        set_CONFIG(CONFIG_PGT_COLOR_TEXT, m_textColorString);
        set_CONFIG(CONFIG_PGT_ID_PROTEIN, m_id_protein);
        set_CONFIG(CONFIG_PGT_ID_GENE, m_id_gene);
        set_CONFIG(CONFIG_PGT_INFO_PROTEIN, m_info_protein);
        set_CONFIG(CONFIG_PGT_INFO_GENE, m_info_gene);
    }

    // UPDATE TEXT FIELD ENTRIES
    if(m_crosshairText) XtVaSetValues(m_crosshairText, XmNvalue, m_crosshairColorString, NULL);
    if(m_unmarkedText) XtVaSetValues(m_unmarkedText, XmNvalue, m_unmarkedColorString, NULL);
    if(m_markedText) XtVaSetValues(m_markedText, XmNvalue, m_markedColorString, NULL);
    if(m_selectedText) XtVaSetValues(m_selectedText, XmNvalue, m_selectedColorString, NULL);
    if(m_textText) XtVaSetValues(m_textText, XmNvalue, m_textColorString, NULL);
    if(m_ID_ProteinText) XtVaSetValues(m_ID_ProteinText, XmNvalue, m_id_protein, NULL);
    if(m_ID_GeneText) XtVaSetValues(m_ID_GeneText, XmNvalue, m_id_gene, NULL);
    if(m_info_ProteinText) XtVaSetValues(m_info_ProteinText, XmNvalue, m_info_protein, NULL);
    if(m_info_GeneText) XtVaSetValues(m_info_GeneText, XmNvalue, m_info_gene, NULL);

    // UPDATE THE DRAWING AREA COLORS
    updateColors();

    return true;
}


void configDialog::updateColors()
{
    if(!m_crosshairArea || !m_unmarkedArea ||
       !m_markedArea || !m_selectedArea || !m_textArea) return;

    int r, g, b;

    if(hex2rgb(&r, &g, &b, m_crosshairColorString))
        drawColoredSpot(m_crosshairArea, r, g, b);

    if(hex2rgb(&r, &g, &b, m_unmarkedColorString))
        drawColoredSpot(m_unmarkedArea, r, g, b);

    if(hex2rgb(&r, &g, &b, m_markedColorString))
        drawColoredSpot(m_markedArea, r, g, b);

    if(hex2rgb(&r, &g, &b, m_selectedColorString))
        drawColoredSpot(m_selectedArea, r, g, b);

    if(hex2rgb(&r, &g, &b, m_textColorString))
        drawColoredSpot(m_textArea, r, g, b);
}


/****************************************************************************
*  IMAGE DIALOG - SET FOREGROUND COLOR
****************************************************************************/
void configDialog::setColor(Display *display, GC gc, int r, int g, int b)
{
    XColor xc;

    xc.red= r << 8; xc.green= g << 8; xc.blue= b << 8;
    xc.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(display, DefaultColormap(display, DefaultScreen(display)), &xc);
    XSetForeground(display, gc, xc.pixel);
}


void configDialog::drawColoredSpot(Widget area, int r , int g , int b)
{
    if(!area) return;

    // GET DISPLAY + GC
    Display *display= XtDisplay(area);
    Window win = XtWindow(area);
    GC gc= XCreateGC(display, win, 0, 0);

    // GET BACKGROUND COLOR
    Pixel bg;
    XtVaGetValues(m_top, XmNbackground, &bg, NULL);

    // FILL BACKGROUND WITH COLOR BG
    XtVaSetValues(area, XmNbackground, bg, NULL);

    // DRAW THE FILLED CIRCLE
    setColor(display, gc, r, g, b);
    XFillArc(display, win, gc, 4, 4, 20, 20, 0, 360*64);

    // DRAW A BLACK BORDER AROUND THE CIRCLE
    setColor(display, gc, 0, 0, 0);
    XDrawArc(display, win, gc, 4, 4, 20, 20, 0, 360*64);

    XFlush(display);

    // FREE OUR GC AS WE DONT NEED IT ANYMORE
    XFreeGC(display, gc);
}


void configDialog::getCONFIGS()
{
    // FREE OLD STRING ENTRIES BEFORE ADDING NEW ONES
    freeStrings();

    // FETCH AWARS
    m_crosshairColorString= get_CONFIG(CONFIG_PGT_COLOR_CROSSHAIR);
    m_unmarkedColorString=  get_CONFIG(CONFIG_PGT_COLOR_UNMARKED);
    m_markedColorString=    get_CONFIG(CONFIG_PGT_COLOR_MARKED);
    m_selectedColorString=  get_CONFIG(CONFIG_PGT_COLOR_SELECTED);
    m_textColorString=      get_CONFIG(CONFIG_PGT_COLOR_TEXT);
    m_id_protein=           get_CONFIG(CONFIG_PGT_ID_PROTEIN);
    m_id_gene=              get_CONFIG(CONFIG_PGT_ID_GENE);
    m_info_protein=         get_CONFIG(CONFIG_PGT_INFO_PROTEIN);
    m_info_gene=            get_CONFIG(CONFIG_PGT_INFO_GENE);

    // UPDATE TEXT FIELD ENTRIES
    if(m_crosshairText) XtVaSetValues(m_crosshairText, XmNvalue, m_crosshairColorString, NULL);
    if(m_unmarkedText) XtVaSetValues(m_unmarkedText, XmNvalue, m_unmarkedColorString, NULL);
    if(m_markedText) XtVaSetValues(m_markedText, XmNvalue, m_markedColorString, NULL);
    if(m_selectedText) XtVaSetValues(m_selectedText, XmNvalue, m_selectedColorString, NULL);
    if(m_textText) XtVaSetValues(m_textText, XmNvalue, m_textColorString, NULL);
    if(m_ID_ProteinText) XtVaSetValues(m_ID_ProteinText, XmNvalue, m_id_protein, NULL);
    if(m_ID_GeneText) XtVaSetValues(m_ID_GeneText, XmNvalue, m_id_gene, NULL);
    if(m_info_ProteinText) XtVaSetValues(m_info_ProteinText, XmNvalue, m_info_protein, NULL);
    if(m_info_GeneText) XtVaSetValues(m_info_GeneText, XmNvalue, m_info_gene, NULL);

    // UPDATE THE DRAWING AREA COLORS
    updateColors();
}


void configDialog::setCONFIGS()
{
    // CONFIGS - WRITE BACK
    set_CONFIG(CONFIG_PGT_COLOR_CROSSHAIR, m_crosshairColorString);
    set_CONFIG(CONFIG_PGT_COLOR_UNMARKED, m_unmarkedColorString);
    set_CONFIG(CONFIG_PGT_COLOR_MARKED, m_markedColorString);
    set_CONFIG(CONFIG_PGT_COLOR_SELECTED, m_selectedColorString);
    set_CONFIG(CONFIG_PGT_COLOR_TEXT, m_textColorString);
    set_CONFIG(CONFIG_PGT_ID_PROTEIN, m_id_protein);
    set_CONFIG(CONFIG_PGT_ID_GENE, m_id_gene);
    set_CONFIG(CONFIG_PGT_INFO_PROTEIN, m_info_protein);
    set_CONFIG(CONFIG_PGT_INFO_GENE, m_info_gene);

    // GET LOCAL TIME CODE (-> SET TIME OF LAST CHANGE)
    time_t ttime= time(NULL);
    char *stime=  asctime(localtime(&ttime));

    // TRIGGER SIGNAL: CONFIG HAS CHANGED...
    set_config_AWAR(stime);
}


/****************************************************************************
*  CALLBACK - CANCEL BUTTON PRESSED
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticCancelButtonCallback(Widget, XtPointer clientData, XtPointer)
{
    // GET POINTER OF THE ORIGINAL CALLER
    configDialog *cD= (configDialog *)clientData;

    // DESTROY OBJECT
    cD->~configDialog();
}


/****************************************************************************
*  CALLBACK - DEFAULT BUTTON PRESSED
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticDefaultButtonCallback(Widget, XtPointer clientData, XtPointer)
{
    // GET POINTER OF THE ORIGINAL CALLER
    configDialog *cD= (configDialog *)clientData;

    // SET VALUES TO DEFAULT
    cD->setDefault(false);
}


/****************************************************************************
*  CALLBACK - COLOR CHANGED
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticOkButtonCallback(Widget, XtPointer clientData, XtPointer)
{
    // GET POINTER OF THE ORIGINAL CALLER
    configDialog *cD= (configDialog *)clientData;

    // SAVE AWARS TO ARB
    cD->setCONFIGS();

    // DESTROY OBJECT
    cD->~configDialog();
}


/****************************************************************************
*  CALLBACK - COLOR/TEXT CHANGED
*  !!! CAUTION: THIS IS A WRAPPER FUNCTION !!!
****************************************************************************/
void staticTextChangedCallback(Widget widget, XtPointer clientData, XtPointer)
{
    // GET POINTER OF THE ORIGINAL CALLER
    configDialog *cD= (configDialog *)clientData;

    // CALL TEXT CHANGED FUNCTION
    cD->textChangedCallback(widget);
}


/****************************************************************************
*  CALLBACK - COLOR/TEXT CHANGED
****************************************************************************/
void configDialog::textChangedCallback(Widget widget)
{
    int r, g, b; // DUMMY RGB VALUES

    // CROSSHAIR TEXT-WIDGET CHANGED?
    if(widget == m_crosshairText)
    {
        char *crosshairColorString= XmTextGetString(m_crosshairText);

        if(hex2rgb(&r, &g, &b, crosshairColorString))
        {
            if(m_crosshairColorString) free(m_crosshairColorString);
            m_crosshairColorString= crosshairColorString;
        }
        else free(crosshairColorString);
    }
    // UNMARKED TEXT-WIDGET CHANGED?
    else if(widget == m_unmarkedText)
    {
        char *unmarkedColorString= XmTextGetString(m_unmarkedText);

        if(hex2rgb(&r, &g, &b, unmarkedColorString))
        {
            if(m_unmarkedColorString) free(m_unmarkedColorString);
            m_unmarkedColorString= unmarkedColorString;
        }
        else free(unmarkedColorString);
    }
    // MARKED TEXT-WIDGET CHANGED?
    else if(widget == m_markedText)
    {
        char *markedColorString= XmTextGetString(m_markedText);

        if(hex2rgb(&r, &g, &b, markedColorString))
        {
            if(m_markedColorString) free(m_markedColorString);
            m_markedColorString= markedColorString;
        }
        else free(markedColorString);
    }
    // SELECTED TEXT-WIDGET CHANGED?
    else if(widget == m_selectedText)
    {
        char *selectedColorString= XmTextGetString(m_selectedText);

        if(hex2rgb(&r, &g, &b, selectedColorString))
        {
            if(m_selectedColorString) free(m_selectedColorString);
            m_selectedColorString= selectedColorString;
        }
        else free(selectedColorString);
    }
    // INFOTEXT TEXT-WIDGET CHANGED?
    else if(widget == m_textText)
    {
        char *textColorString= XmTextGetString(m_textText);

        if(hex2rgb(&r, &g, &b, textColorString))
        {
            if(m_textColorString) free(m_textColorString);
            m_textColorString= textColorString;
        }
        else free(textColorString);
    }
    // PROTEIN-ID TEXT-WIDGET CHANGED?
    else if(widget == m_ID_ProteinText)
    {
        if(m_id_protein) free(m_id_protein);
        m_id_protein= XmTextGetString(m_ID_ProteinText);
    }
    // GENE-ID TEXT-WIDGET CHANGED?
    else if(widget == m_ID_GeneText)
    {
        if(m_id_gene) free(m_id_gene);
        m_id_gene= XmTextGetString(m_ID_GeneText);
    }
    // PROTEIN-TEXT TEXT-WIDGET CHANGED?
    else if(widget == m_info_ProteinText)
    {
        if(m_info_protein) free(m_info_protein);
        m_info_protein= XmTextGetString(m_info_ProteinText);
    }
    // GENE-TEXT TEXT-WIDGET CHANGED?
    else if(widget == m_info_GeneText)
    {
        if(m_info_gene) free(m_info_gene);
        m_info_gene= XmTextGetString(m_info_GeneText);
    }



//     // GET TEXT FIELD ENTRIES
//     char *crosshairText= XmTextGetString(m_crosshairText);
//     char *unmarkedText= XmTextGetString(m_unmarkedText);
//     char *markedText= XmTextGetString(m_markedText);
//     char *selectedText= XmTextGetString(m_selectedText);
//     char *textText= XmTextGetString(m_textText);
//
//     char *id_proteinText= XmTextGetString(m_ID_ProteinText);
//     char *id_geneText= XmTextGetString(m_ID_GeneText);
//     char *info_proteinText= XmTextGetString(m_info_ProteinText);
//     char *info_geneText= XmTextGetString(m_info_GeneText);
//
//     // UPDATE RELATED WIDGET
//     if((widget == m_crosshairText)
//         && (hex2rgb(&r, &g, &b, crosshairText))) m_crosshairColorString= crosshairText;
//
//     else if((widget == m_unmarkedText)
//         && (hex2rgb(&r, &g, &b, unmarkedText))) m_unmarkedColorString= unmarkedText;
//
//     else if((widget == m_markedText)
//         && (hex2rgb(&r, &g, &b, markedText))) m_markedColorString= markedText;
//
//     else if((widget == m_selectedText)
//         && (hex2rgb(&r, &g, &b, selectedText))) m_selectedColorString= selectedText;
//
//     else if((widget == m_textText)
//              && (hex2rgb(&r, &g, &b, textText))) m_textColorString= textText;
//
//     else if(widget == m_ID_ProteinText) m_id_protein= id_proteinText;
//     else if(widget == m_ID_GeneText) m_id_gene= id_geneText;
//     else if(widget == m_info_ProteinText) m_info_protein= info_proteinText;
//     else if(widget == m_info_GeneText) m_info_gene= info_geneText;

    // UPDATE THE COLORS
    updateColors();
}


/****************************************************************************
*  CONVERT INT RGB -> HEX COLOR STRING
****************************************************************************/
char *rgb2hex(int r, int g, int b)
{
    // ALL INPUT VALUES IN RANGE?
    if((r < 0) || (g < 0) || (b < 0) ||
       (r > 255) || (g > 255) || (b > 255)) return NULL;

    // ALLOCATE MEM FOR HEX COLOR STRING
    char *hex= (char *)malloc(8 * sizeof(char));
    if(!hex) return NULL;

    // WRITE COLORS INTO MEM
    if(sprintf(hex, "#%2X%2X%2X", r, g, b) != 3) return NULL;

    // RETURN POINTER TO HEX STRING
    return hex;
}


/****************************************************************************
*  CONVERT HEX STRING -> INT RGB
****************************************************************************/
bool hex2rgb(int *r, int *g, int *b, char *hex)
{
    // SPLIT THE THREE COLORS
    if(sscanf(hex, "#%2X%2X%2X", r, g, b) != 3)  return false;

    // RETURN TRUE IF SUCCESSFUL
    return true;
}
