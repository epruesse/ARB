// OpenGl Header files

#define OPEN_GL_WAY_TO_INCLUDE 3
// ------------------------------
#if (OPEN_GL_WAY_TO_INCLUDE == 1)
// working on waltz:
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glpng.h>
#ifdef __GLX_glx_h__
#error include wrapper define for glx.h already set -- next include will do nothing
#endif
// i have not included this file in my version. I replaced glxew.h with glx.h when
// you mentioned that it was not compiling on your machine and checked in to the repository. --yadhu 
//#include <GL/glx.h>
#endif // OPEN_GL_WAY_TO_INCLUDE == 1
// ------------------------------
#if (OPEN_GL_WAY_TO_INCLUDE == 2)
// working on ralfs machine
// not workint on waltz : Previous declaration errors because
// GLwMDrawA.h includes glx.h and/or glxew.h files
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glpng.h>
#include <GL/glxew.h>
#endif // OPEN_GL_WAY_TO_INCLUDE == 2
// ------------------------------
#if (OPEN_GL_WAY_TO_INCLUDE == 3)
// working on ralfs machine
// working on waltz too
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glpng.h>

// @@@ HACK @@@
// including glew.h does define the include wrapper used by glx.h
// so including glx.h does includes nothing and symbols are missing.
// Undefining it here does the job: 
#undef __GLX_glx_h__
#include <GL/glx.h>
#endif // OPEN_GL_WAY_TO_INCLUDE == 3
// ------------------------------

#include <GL/GLwMDrawA.h>
