#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_Renderer.hxx"
#include "RNA3D_Textures.hxx"
#include "RNA3D_StructureData.hxx"

#include <awt_canvas.hxx>
#include <aw_window_Xm_interface.hxx>

using namespace std;

OpenGLGraphics::OpenGLGraphics()
    : displayGrid(false)
    , ApplicationBGColor(0, 0, 0)
{
}

OpenGLGraphics::~OpenGLGraphics() {
}

// Sets the Background Color for the OpenGL Window
void OpenGLGraphics::SetOpenGLBackGroundColor() {
    unsigned long bgColor;
    XtVaGetValues(RNA3D->glw, XmNbackground, &bgColor, NULL);

    Widget w = AW_get_AreaWidget(RNA3D->gl_Canvas->aww, AW_MIDDLE_AREA);

    XColor xcolor;
    xcolor.pixel = bgColor;
    Colormap colormap = DefaultColormapOfScreen(XtScreen(w));
    XQueryColor(XtDisplay(w), colormap, &xcolor);

    float r = xcolor.red / 65535.0;
    float g = xcolor.green / 65535.0;
    float b = xcolor.blue / 65535.0;

    // set OpenGL background color to the widget's background
    glClearColor(r, g, b, 1);

    ApplicationBGColor = ColorRGBf(r, g, b);
}

// Converts the GC into RGB values
ColorRGBf OpenGLGraphics::ConvertGCtoRGB(int gc) {
    ColorRGBf clr = ColorRGBf(0, 0, 0);
    Widget    w   = AW_get_AreaWidget(RNA3D->gl_Canvas->aww, AW_MIDDLE_AREA);
    GC        xgc = AW_map_AreaGC(RNA3D->gl_Canvas->aww, AW_MIDDLE_AREA, gc);

    XGCValues     xGCValues;
    XGetGCValues(XtDisplay(w), xgc, GCForeground, &xGCValues);
    unsigned long color = xGCValues.foreground;

    XColor xcolor;
    xcolor.pixel = color;

    Colormap colormap = DefaultColormapOfScreen(XtScreen(w));
    XQueryColor(XtDisplay(w), colormap, &xcolor);

    float r = xcolor.red / 65535.0;
    float g = xcolor.green / 65535.0;
    float b = xcolor.blue / 65535.0;

    clr = ColorRGBf(r, g, b);
    return clr;
}

// Converts the GC into RGB values and sets the glColor
void OpenGLGraphics::SetColor(int gc) {
    ColorRGBf color = ConvertGCtoRGB(gc);
    glColor4f(color.red, color.green, color.blue, 1);
}

// Converts the GC into RGB values and returns them
ColorRGBf OpenGLGraphics::GetColor(int gc) {
    ColorRGBf color = ConvertGCtoRGB(gc);
    return color;
}

void OpenGLGraphics::WinToScreenCoordinates(int x, int y, GLdouble *screenPos) {
    // project window coords to gl coord
    glLoadIdentity();
    GLdouble modelMatrix[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
    GLdouble projMatrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    gluUnProject(x, y, 0,
                 modelMatrix,
                 projMatrix,
                 viewport,
                 // the next 3 parameters are the pointers to the final object coordinates.(double)
                 &screenPos[0], &screenPos[1], &screenPos[2]
                 );
}

// =========== Functions related to rendering FONTS ========================//

static GLuint font_base;

void OpenGLGraphics::init_font(GLuint base, char* f)
{
    Display* display;
    XFontStruct* font_info;
    int first;
    int last;

    // Need an X Display before calling any Xlib routines.
    display = glXGetCurrentDisplay();
    if (display == 0) {
        fprintf(stderr, "XOpenDisplay() failed.  Exiting.\n");
        exit(EXIT_FAILURE);
    }
    else {
        // Load the font.
        font_info = XLoadQueryFont(display, f);
        if (!font_info) {
            fprintf(stderr, "XLoadQueryFont() failed - Exiting.\n");
            exit(EXIT_FAILURE);
        }
        else {
            // Tell GLX which font & glyphs to use.
            first = font_info->min_char_or_byte2;
            last  = font_info->max_char_or_byte2;
            glXUseXFont(font_info->fid, first, last-first+1, base+first);
        }
        display = 0;
    }
}

void OpenGLGraphics::print_string(GLuint base, char* s)
{
    if (!glIsList(font_base)) {
        fprintf(stderr, "print_string(): Bad display list. - Exiting.\n");
        exit(EXIT_FAILURE);
    }
    else if (s && strlen(s)) {
        glPushAttrib(GL_LIST_BIT);
        glListBase(base);
        glCallLists(strlen(s), GL_UNSIGNED_BYTE, (GLubyte *)s);
        glPopAttrib();
    }
}

void OpenGLGraphics::InitMainFont(char* f)
{
    font_base = glGenLists(256);
    if (!glIsList(font_base)) {
        fprintf(stderr, "InitMainFont(): Out of display lists. - Exiting.\n");
        exit(EXIT_FAILURE);
    }
    else {
        init_font(font_base, f);
    }
}

void OpenGLGraphics::PrintString(float x, float y, float z, char *s, void * /* font */) {
    glRasterPos3f(x, y, z);
    print_string(font_base, s);
}

void  OpenGLGraphics::DrawBox(float x, float y,  float w, float h)
{
    glBegin(GL_QUADS);
    glVertex2f(x-w/2, y-h/2);
    glVertex2f(x+w/2, y-h/2);
    glVertex2f(x+w/2, y+h/2);
    glVertex2f(x-w/2, y+h/2);
    glEnd();
}
