// ============================================================= //
//                                                               //
//   File      : aw_window_ogl.hxx                               //
//   Purpose   : open gl window                                  //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef AW_WINDOW_OGL_HXX
#define AW_WINDOW_OGL_HXX

#ifndef AW_WINDOW_HXX
#include <aw_window.hxx>
#endif

// --------------------------------------------------------------------------------
// For Applications Using OpenGL Windows
// Variable "AW_alpha_Size_Supported" says whether the hardware (Graphics Card)
// supports alpha channel or not. Alpha channel is used for shading/ multi textures
// in OpenGL applications.

extern bool AW_alpha_Size_Supported;


// Extended by Daniel Koitzsch & Christian Becker 19-05-04
struct AW_window_menu_modes_opengl : public AW_window_menu_modes { // derived from a Noncopyable
    AW_window_menu_modes_opengl();
    ~AW_window_menu_modes_opengl();
    virtual void init(AW_root *root, const char *wid, const char *windowname, int width, int height);
};



#else
#error aw_window_ogl.hxx included twice
#endif // AW_WINDOW_OGL_HXX
