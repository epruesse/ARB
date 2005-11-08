#include "RNA3D_GlobalHeader.hxx"
#include "RNA3D_Global.hxx"
#include "RNA3D_OpenGLGraphics.hxx"
#include "RNA3D_Renderer.hxx"
#include "RNA3D_Textures.hxx"
#include "RNA3D_StructureData.hxx"

#include <math.h>

// The following includes are needed to use AW_window_Motif 
// and access Application's colormap
#include "../WINDOW/aw_awar.hxx"
#include "../WINDOW/aw_Xm.hxx"
#include "../WINDOW/aw_click.hxx"
#include "../WINDOW/aw_size.hxx"
#include "../WINDOW/aw_print.hxx"
#include "../WINDOW/aw_window_Xm.hxx"
#include "../WINDOW/aw_commn.hxx"

using namespace std;

OpenGLGraphics::OpenGLGraphics(void)
    : displayGrid(false)
    , ApplicationBGColor(0, 0, 0)
{
}

OpenGLGraphics::~OpenGLGraphics(void){
}

// Sets the Background Color for the OpenGL Window
void OpenGLGraphics::SetOpenGLBackGroundColor() {

	unsigned long bgColor;
	XtVaGetValues( RNA3D->glw, XmNbackground, &bgColor, NULL );

    Widget w = RNA3D->gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area;

    XColor xcolor;
    xcolor.pixel = bgColor;
    Colormap colormap = DefaultColormapOfScreen( XtScreen( w ) );
    XQueryColor( XtDisplay( w ), colormap, &xcolor );

    float r, g, b; r = g = b = 0.0;
    r = (float) xcolor.red / 65535.0;
    g = (float) xcolor.green / 65535.0;
    b = (float) xcolor.blue / 65535.0;

    // set OpenGL Backgroud Color to the widget's backgroud     
    glClearColor(r, g, b, 1);

    // extern ColorRGBf ApplicationBGColor;
    ApplicationBGColor = ColorRGBf(r, g, b);
}

// Converts the GC into RGB values 
ColorRGBf OpenGLGraphics::ConvertGCtoRGB(int gc) {
    ColorRGBf clr = ColorRGBf(0,0,0);
    float r, g, b; r = g = b = 0.0;
   
    Widget w = RNA3D->gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->area;

    AW_common *common = RNA3D->gl_Canvas->aww->p_w->areas[ AW_MIDDLE_AREA ]->common;
    register class AW_GC_Xm *gcm = AW_MAP_GC( gc );
 
    XGCValues xGCValues;
    XGetGCValues( XtDisplay( w ), gcm->gc, GCForeground, &xGCValues );
    unsigned long color = xGCValues.foreground;

    XColor xcolor;
    xcolor.pixel = color;

    Colormap colormap = DefaultColormapOfScreen( XtScreen( w ) );
	XQueryColor( XtDisplay( w ), colormap, &xcolor );
	
	r = (float) xcolor.red / 65535.0;
	g = (float) xcolor.green / 65535.0;
	b = (float) xcolor.blue / 65535.0;

    clr = ColorRGBf(r,g,b);
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

//=========== Functions related to rendering FONTS ========================//

static GLuint font_base;

void OpenGLGraphics::init_font(GLuint base, char* f)
{
   Display* display;
   XFontStruct* font_info;
   int first;
   int last;

   /* Need an X Display before calling any Xlib routines. */
   display = XOpenDisplay(0);
   if (display == 0) {
      fprintf(stderr, "XOpenDisplay() failed.  Exiting.\n");
      exit(-1);
   } 
   else {
      /* Load the font. */
      font_info = XLoadQueryFont(display, f);
      if (!font_info) {
         fprintf(stderr, "XLoadQueryFont() failed - Exiting.\n");
         exit(-1);
      }
      else {
         /* Tell GLX which font & glyphs to use. */
         first = font_info->min_char_or_byte2;
         last  = font_info->max_char_or_byte2;
         glXUseXFont(font_info->fid, first, last-first+1, base+first);
      }
      XCloseDisplay(display);
      display = 0;
   }
}

void OpenGLGraphics::print_string(GLuint base, char* s)
{
   if (!glIsList(font_base)) {
      fprintf(stderr, "print_string(): Bad display list. - Exiting.\n");
      exit (-1);
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
      exit (-1);
   }
   else {
      init_font(font_base, f);
   }
}

void OpenGLGraphics::PrintString(float x, float y, float z, char *s, void *font) {
    glRasterPos3f(x, y, z);
    print_string(font_base, s);
//         for (unsigned int i = 0; i < strlen(s); i++) {
//             glutBitmapCharacter(font, s[i]);
//         }
}

void OpenGLGraphics::PrintComment(float x, float y, float z, char *s){
    // if comments are longer break them and display in next line
//     int col  = 35;

//         glRasterPos3f(x, y, z);
//         for (unsigned int i = 0; i < strlen(s); i++) {
//             if(i%col==0) {
//                 y -= 10;
//                 glRasterPos3f(x, y, z);
//             }
//             glutBitmapCharacter(GLUT_BITMAP_8_BY_13, s[i]);
//         }
}

//===============================================================================

void  OpenGLGraphics::DrawBox(float x, float y,  float w, float h)
{
	glBegin(GL_QUADS);		
        glVertex2f(x-w/2, y-h/2);
        glVertex2f(x+w/2, y-h/2);
        glVertex2f(x+w/2, y+h/2);
        glVertex2f(x-w/2, y+h/2);
    glEnd();
}
