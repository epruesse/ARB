// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$


#include "help_dialog.hxx"


/****************************************************************************
*  HELP DIALOG - CONSTRUCTOR
****************************************************************************/
helpDialog::helpDialog(Widget p, MDialog *d)
    : MDialog(p, d)
{
    // CREATE SHELL FOR THIS DIALOG
    createShell("");

    // CALL CREATE WINDOW FUNCTION
    createWindow();

    // SET WINDOW WIDTH
    XtVaSetValues(m_shell,
        XmNwidth, 300,
        XmNheight, 450,
        NULL);

    // REALIZE WINDOW
    realizeShell();

    // SET WINDOW LABEL
    setWindowName("PGT - Help Dialog");

    // DESELECT ALL WINDOW BUTTONS EXCEPT CLOSE AND MOVE
    XtVaSetValues(m_shell, XmNmwmFunctions, MWM_FUNC_MOVE | MWM_FUNC_CLOSE, NULL);
}


/****************************************************************************
*  HELP DIALOG - DESTRUCTOR
****************************************************************************/
helpDialog::~helpDialog()
{

}


/****************************************************************************
*  HELP DIALOG - CREATE WINDOW
****************************************************************************/
void helpDialog::createWindow()
{
    // CREATE TOP LEVEL WIDGET
    m_top= XtVaCreateManagedWidget("top",
        xmFormWidgetClass, m_shell,
        XmNorientation, XmVERTICAL,
        XmNmarginHeight, 0,
        XmNmarginWidth, 0,
        NULL);

    // CREATE A SIMPLE LABEL
    createVisualizationHelp(m_top);
}


/****************************************************************************
*  HELP DIALOG - CREATE VISUALIZATION HELP
****************************************************************************/
void helpDialog::createVisualizationHelp(Widget parent)
{
    // GET FOREGROUND AND BACKGROUND PIXEL COLORS
    Pixel fg, bg;
    XtVaGetValues(parent, XmNforeground, &fg, XmNbackground, &bg, NULL);

    // USED PIXMAPS (BUTTON LOGOS)
    Pixmap circle22_xpm, cross22_xpm, text22_xpm, markonly22_xpm, textonly22_xpm,
           markall22_xpm, marknone22_xpm, arb2mark_xpm, mark2arb_xpm, help_xpm,
           markinfo22_xpm, markinvert22_xpm;

    // OPEN THE PIXMAP FILES
    circle22_xpm=     XmGetPixmap(XtScreen(parent), "pics/circle22.xpm", fg, bg);
    cross22_xpm=      XmGetPixmap(XtScreen(parent), "pics/cross22.xpm", fg, bg);
    text22_xpm=       XmGetPixmap(XtScreen(parent), "pics/text22.xpm", fg, bg);
    markonly22_xpm=   XmGetPixmap(XtScreen(parent), "pics/markonly22.xpm", fg, bg);
    textonly22_xpm=   XmGetPixmap(XtScreen(parent), "pics/textonly22.xpm", fg, bg);
    markall22_xpm=    XmGetPixmap(XtScreen(parent), "pics/markall22.xpm", fg, bg);
    markinvert22_xpm= XmGetPixmap(XtScreen(parent), "pics/markinvert22.xpm", fg, bg);
    marknone22_xpm=   XmGetPixmap(XtScreen(parent), "pics/marknone22.xpm", fg, bg);
    markinfo22_xpm=   XmGetPixmap(XtScreen(parent), "pics/markinfo22.xpm", fg, bg);
    arb2mark_xpm=     XmGetPixmap(XtScreen(parent), "pics/arb2mark22.xpm", fg, bg);
    mark2arb_xpm=     XmGetPixmap(XtScreen(parent), "pics/mark2arb22.xpm", fg, bg);
    help_xpm=         XmGetPixmap(XtScreen(parent), "pics/help22.xpm", fg, bg);

    // CREATE A SIMPLE LABEL
    Widget top_label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("Visualization Buttons:"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_CENTER,
        XmNtopAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_FORM,
        XmNrightAttachment, XmATTACH_FORM,
        NULL);

    // CIRCLE DESCRIPTION
    Widget pos01_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, circle22_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, top_label,
        NULL);
    Widget pos01_Label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("Show/hide virtual spots"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos01_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, top_label,
        NULL);

    // CROSS DESCRIPTION
    Widget pos02_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, cross22_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos01_Button,
        NULL);
    Widget pos02_Label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("Show/hide spot crosshair"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos02_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos01_Label,
        NULL);

    // TEXT DESCRIPTION
    Widget pos03_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, text22_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos02_Button,
        NULL);
    Widget pos03_Label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("Show/hide spot information"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos03_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos02_Label,
        NULL);

    // MARKONLY DESCRIPTION
    Widget pos04_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, markonly22_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos03_Button,
        NULL);
    Widget pos04_Label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("show only marked spots"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos04_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos03_Label,
        NULL);

    // TEXTONLY DESCRIPTION
    Widget pos05_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, textonly22_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos04_Button,
        NULL);
    Widget pos05_Label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("show only spots with an ID"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos05_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos04_Label,
        NULL);

    // MARKALL DESCRIPTION
    Widget pos06_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, markall22_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos05_Button,
        NULL);
    Widget pos06_Label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("mark all spots"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos06_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos05_Label,
        NULL);

    // MARKNONE DESCRIPTION
    Widget pos07_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, marknone22_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos06_Button,
        NULL);
    Widget pos07_Label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("unmark all spots"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos07_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos06_Label,
        NULL);

    // ARB2MARK DESCRIPTION
    Widget pos08_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, arb2mark_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos07_Button,
        NULL);
    Widget pos08_Label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("transfer marker to arb gene map"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos08_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos07_Label,
        NULL);

    // ARB2MARK DESCRIPTION
    Widget pos09_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, mark2arb_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos08_Button,
        NULL);
    Widget pos09_Label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("fetch marker from arb gene map"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos09_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos08_Label,
        NULL);

    // INVERT MARK DESCRIPTION
    Widget pos10_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, markinvert22_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos09_Button,
        NULL);
    Widget pos10_Label= XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("invert all spot marker"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos10_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos09_Label,
        NULL);

    // MARK INFO ONLY DESCRIPTION
    Widget pos11_Button= XtVaCreateManagedWidget("spotsbtn",
        xmPushButtonWidgetClass, parent,
        XmNlabelType, XmPIXMAP,
        XmNlabelPixmap, markinfo22_xpm,
        XmNheight, 32,
        XmNleftAttachment, XmATTACH_FORM,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos10_Button,
        NULL);
    XtVaCreateManagedWidget("label",
        xmLabelWidgetClass, parent,
        XmNlabelString, XmStringCreateLocalized("mark only spots with an ID"),
        XmNheight, 32,
        XmNalignment, XmALIGNMENT_BEGINNING,
        XmNrightAttachment, XmATTACH_FORM,
        XmNleftAttachment, XmATTACH_WIDGET,
        XmNleftWidget, pos11_Button,
        XmNtopAttachment, XmATTACH_WIDGET,
        XmNtopWidget, pos10_Label,
        NULL);
}
