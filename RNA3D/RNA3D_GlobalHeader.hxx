// C/C++ Header files

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <iostream>
#include <fstream>

// ARB Header files
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <awt_canvas.hxx>
#include <awt.hxx>
#include <awt_tree.hxx>
#include <awt_dtree.hxx>
#include <awt_tree_cb.hxx>
#include <BI_helix.hxx>

// Xt/Motif Header files
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/StringDefs.h>
#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#include <Xm/MwmUtil.h>
#include <Xm/MainW.h>

// OpenGl Header files

// @yadhu : please don't remove any code here until it works on all machines

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
#include <GL/glx.h>
#endif // OPEN_GL_WAY_TO_INCLUDE == 1
// ------------------------------
#if (OPEN_GL_WAY_TO_INCLUDE == 2)
// working on ralfs machine
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glpng.h>
#include <GL/glxew.h>
#endif // OPEN_GL_WAY_TO_INCLUDE == 2
// ------------------------------
#if (OPEN_GL_WAY_TO_INCLUDE == 3)
// working on ralfs machine
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
