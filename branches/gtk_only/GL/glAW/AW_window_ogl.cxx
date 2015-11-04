// ============================================================= //
//                                                               //
//   File      : AW_window_ogl.cxx                               //
//   Purpose   : open gl window                                  //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_window_ogl.hxx"

// Extended by Daniel Koitzsch & Christian Becker 19-05-04

#ifndef ARB_OPENGL
#error only compiles in ARB_OPENGL mode
#endif // ARB_OPENGL

#define GLX_GLXEXT_PROTOTYPES

#include <aw_window_Xm.hxx>
#include <aw_root.hxx>
#include <arbdb.h>

#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#include <Xm/DrawingA.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/MainW.h>
#include <Xm/CascadeB.h>
#include <Xm/MenuShell.h>
#include <Xm/ScrollBar.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include "GLwMDrawA.h" //* Provides a special motif widget class

// defined here by Yadhu in order to make it more General
bool AW_alpha_Size_Supported = false;


AW_window_menu_modes_opengl::AW_window_menu_modes_opengl() {
}

AW_window_menu_modes_opengl::~AW_window_menu_modes_opengl() {
}

void AW_window_menu_modes_opengl::init(AW_root *root_in, const char *wid,
                                       const char *windowname, int width, int height) {

    Widget      main_window;
    Widget      help_popup;
    Widget      help_label;
    Widget      separator;
    Widget      form1;
    Widget      form2;
    const char *help_button   = "HELP";
    const char *help_mnemonic = "H";

#if defined(DUMP_MENU_LIST)
    initMenuListing(windowname);
#endif // DUMP_MENU_LIST
    root = root_in; // for macro
    window_name = strdup(windowname);
    window_defaults_name = GBS_string_2_key(wid);

    int posx = 50;
    int posy = 50;

    p_w->shell = aw_create_shell(this, true, true, width, height, posx, posy);

    main_window = XtVaCreateManagedWidget("mainWindow1",
                                          xmMainWindowWidgetClass, p_w->shell,
                                          NULL);

    p_w->menu_bar[0] = XtVaCreateManagedWidget("menu1", xmRowColumnWidgetClass,
                                               main_window,
                                               XmNrowColumnType, XmMENU_BAR,
                                               NULL);

    // create shell for help-cascade
    help_popup = XtVaCreatePopupShell("menu_shell", xmMenuShellWidgetClass,
                                      p_w->menu_bar[0],
                                      XmNwidth, 1,
                                      XmNheight, 1,
                                      XmNallowShellResize, true,
                                      XmNoverrideRedirect, true,
                                      NULL);

    // create row column in Pull-Down shell
    p_w->help_pull_down = XtVaCreateWidget("menu_row_column",
                                           xmRowColumnWidgetClass, help_popup,
                                           XmNrowColumnType, XmMENU_PULLDOWN,
                                           NULL);

                                                    // create HELP-label in menu bar
    help_label = XtVaCreateManagedWidget("menu1_top_b1",
                                         xmCascadeButtonWidgetClass, p_w->menu_bar[0],
                                         RES_CONVERT(XmNlabelString, help_button),
                                         RES_CONVERT(XmNmnemonic, help_mnemonic),
                                         XmNsubMenuId, p_w->help_pull_down, NULL);
    XtVaSetValues(p_w->menu_bar[0], XmNmenuHelpWidget, help_label, NULL);
    // insert help_label to button_list
    root->register_widget(help_label, AWM_ALL);

    form1 = XtVaCreateManagedWidget("form1",
                                     xmFormWidgetClass,
                                     main_window,
                                     XmNresizePolicy, XmRESIZE_NONE,
                                     NULL);

    p_w->mode_area = XtVaCreateManagedWidget("mode area",
                                              xmDrawingAreaWidgetClass,
                                              form1,
                                              XmNresizePolicy, XmRESIZE_NONE,
                                              XmNwidth, 38,
                                              XmNheight, height,
                                              XmNx, 0,
                                              XmNy, 0,
                                              XmNleftOffset, 0,
                                              XmNtopOffset, 0,
                                              XmNbottomAttachment, XmATTACH_FORM,
                                              XmNleftAttachment, XmATTACH_POSITION,
                                              XmNtopAttachment, XmATTACH_POSITION,
                                              XmNmarginHeight, 2,
                                              XmNmarginWidth, 1,
                                              NULL);

    separator = XtVaCreateManagedWidget("separator",
                                         xmSeparatorWidgetClass,
                                         form1,
                                         XmNx, 37,
                                         XmNshadowThickness, 4,
                                         XmNorientation, XmVERTICAL,
                                         XmNbottomAttachment, XmATTACH_FORM,
                                         XmNtopAttachment, XmATTACH_FORM,
                                         XmNleftAttachment, XmATTACH_NONE,
                                         XmNleftWidget, NULL,
                                         XmNrightAttachment, XmATTACH_NONE,
                                         XmNleftOffset, 70,
                                         XmNleftPosition, 0,
                                         NULL);

    form2 = XtVaCreateManagedWidget("form2",
                                     xmFormWidgetClass,
                                     form1,
                                     XmNwidth, width,
                                     XmNheight, height,
                                     XmNtopOffset, 0,
                                     XmNbottomOffset, 0,
                                     XmNleftOffset, 0,
                                     XmNrightOffset, 0,
                                     XmNrightAttachment, XmATTACH_FORM,
                                     XmNbottomAttachment, XmATTACH_FORM,
                                     XmNleftAttachment, XmATTACH_WIDGET,
                                     XmNleftWidget, separator,
                                     XmNtopAttachment, XmATTACH_POSITION,
                                     XmNresizePolicy, XmRESIZE_NONE,
                                     XmNx, 0,
                                     XmNy, 0,
                                     NULL);
    p_w->areas[AW_INFO_AREA] =
        new AW_area_management(form2, XtVaCreateManagedWidget("info_area",
                                                              xmDrawingAreaWidgetClass,
                                                              form2,
                                                              XmNheight, 0,
                                                              XmNbottomAttachment, XmATTACH_NONE,
                                                              XmNtopAttachment, XmATTACH_FORM,
                                                              XmNleftAttachment, XmATTACH_FORM,
                                                              XmNrightAttachment, XmATTACH_FORM,
                                                              XmNmarginHeight, 2,
                                                              XmNmarginWidth, 2,
                                                              NULL));

    p_w->areas[AW_BOTTOM_AREA] =
        new AW_area_management(form2, XtVaCreateManagedWidget("bottom_area",
                                                              xmDrawingAreaWidgetClass,
                                                              form2,
                                                              XmNheight, 0,
                                                              XmNbottomAttachment, XmATTACH_FORM,
                                                              XmNtopAttachment, XmATTACH_NONE,
                                                              XmNleftAttachment, XmATTACH_FORM,
                                                              XmNrightAttachment, XmATTACH_FORM,
                                                              NULL));

    p_w->scroll_bar_horizontal = XtVaCreateWidget("scroll_bar_horizontal",
                                                   xmScrollBarWidgetClass,
                                                   form2,
                                                   XmNheight, 15,
                                                   XmNminimum, 0,
                                                   XmNmaximum, AW_SCROLL_MAX,
                                                   XmNincrement, 10,
                                                   XmNsliderSize, AW_SCROLL_MAX,
                                                   XmNrightAttachment, XmATTACH_FORM,
                                                   XmNbottomAttachment, XmATTACH_FORM,
                                                   XmNbottomOffset, 0,
                                                   XmNleftAttachment, XmATTACH_FORM,
                                                   XmNtopAttachment, XmATTACH_NONE,
                                                   XmNorientation, XmHORIZONTAL,
                                                   XmNrightOffset, 18,
                                                   NULL);

    p_w->scroll_bar_vertical = XtVaCreateWidget("scroll_bar_vertical",
                                                 xmScrollBarWidgetClass,
                                                 form2,
                                                 XmNwidth, 15,
                                                 XmNminimum, 0,
                                                 XmNmaximum, AW_SCROLL_MAX,
                                                 XmNincrement, 10,
                                                 XmNsliderSize, AW_SCROLL_MAX,
                                                 XmNrightAttachment, XmATTACH_FORM,
                                                 XmNbottomAttachment, XmATTACH_WIDGET,
                                                 XmNbottomWidget, p_w->scroll_bar_horizontal,
                                                 XmNbottomOffset, 3,
                                                 XmNleftOffset, 3,
                                                 XmNrightOffset, 3,
                                                 XmNleftAttachment, XmATTACH_NONE,
                                                 XmNtopAttachment, XmATTACH_WIDGET,
                                                 XmNtopWidget, INFO_WIDGET,
                                                 NULL);

    p_w->frame = XtVaCreateManagedWidget("draw_area",
                                          xmFrameWidgetClass,
                                          form2,
                                          XmNshadowType, XmSHADOW_IN,
                                          XmNshadowThickness, 2,
                                          XmNleftOffset, 3,
                                          XmNtopOffset, 3,
                                          XmNbottomOffset, 3,
                                          XmNrightOffset, 3,
                                          XmNtopOffset, 0,
                                          NULL);

    Arg args[20];
    int n;
    Widget glw;
    n = 0;

    XtSetArg(args[n], (char *) GLwNrgba, True); n++;
    XtSetArg(args[n], (char *) GLwNallocateBackground, True); n++;
    XtSetArg(args[n], (char *) GLwNallocateOtherColors, True); n++;
    XtSetArg(args[n], (char *) GLwNdoublebuffer, True); n++;
    XtSetArg(args[n], (char *) GLwNdepthSize, True); n++;
    XtSetArg(args[n], (char *) GLwNredSize, 4); n++;
    XtSetArg(args[n], (char *) GLwNgreenSize, 4); n++;
    XtSetArg(args[n], (char *) GLwNblueSize, 4); n++;

    static int alpha_Attributes[] = { GLX_RGBA,
                                     GLX_DEPTH_SIZE, 12,
                                     GLX_RED_SIZE, 4,
                                     GLX_GREEN_SIZE, 4,
                                     GLX_BLUE_SIZE, 4,
                                     GLX_ALPHA_SIZE, 4,
                                     None };

    Widget tmp = XtCreateWidget("glw", glwMDrawingAreaWidgetClass,
                                form2, args, n);

    XVisualInfo *vi;
    Display *dpy;
    dpy = XtDisplay(tmp);
    vi = glXChooseVisual(dpy, DefaultScreen(dpy), alpha_Attributes);
    if (vi) {
        XtSetArg(args[n], (char *) GLwNalphaSize, 4); n++;
        AW_alpha_Size_Supported = true;
        printf("Alpha channel supported\n");
    }
    else {
        AW_alpha_Size_Supported = false;
        printf("Alpha channel NOT supported\n");
    }

    XtSetArg(args[n], XmNmarginHeight, 0); n++;
    XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET); n++;
    XtSetArg(args[n], XmNtopWidget, p_w->areas[AW_INFO_AREA]->get_area()); n++;
    XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
    XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;

    glw = XtCreateManagedWidget("glw", glwMDrawingAreaWidgetClass,
                                form2, args, n);

    p_w->areas[AW_MIDDLE_AREA] = new AW_area_management(p_w->frame, glw);

    XmMainWindowSetAreas(main_window, p_w->menu_bar[0], (Widget) NULL, (Widget) NULL, (Widget) NULL, form1);

    aw_realize_widget(this);

    create_devices();
    aw_insert_default_help_entries(this);
    create_window_variables();
}



