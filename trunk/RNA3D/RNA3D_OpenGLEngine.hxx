#include <Xm/Xm.h>

#include "GlobalHeader.hxx"

#define CREATED     1
#define NOT_CREATED 0

void ShowVendorInformation();
void ReshapeOpenGLWindow( GLint width, GLint height );
void InitializeOpenGLEngine(GLint width, GLint height );
void drawObjects();
void RenderOpenGLScene(Widget w);
void InitializeOpenGLWindow( Widget w );
void rotate(int x, int y);
