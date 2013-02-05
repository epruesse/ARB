/* PNG loader library for OpenGL v1.45 (10/07/00)
 * by Ben Wyatt ben@wyatt100.freeserve.co.uk
 * Using LibPNG 1.0.2 and ZLib 1.1.3
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the author be held liable for any damages arising from the
 * use of this software.
 *
 * Permission is hereby granted to use, copy, modify, and distribute this
 * source code, or portions hereof, for any purpose, without fee, subject to
 * the following restrictions:
 *
 * 1. The origin of this source code must not be misrepresented. You must not
 *    claim that you wrote the original software. If you use this software in
 *    a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered versions must be plainly marked as such and must not be
 *    misrepresented as being the original source.
 * 3. This notice must not be removed or altered from any source distribution.
 *
 * ---------------------------------------------------
 * This version has been modified for usage inside ARB
 * http://arb-home.de/
 */

#ifndef _GLPNG_H_
#define _GLPNG_H_

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#ifdef _DEBUG
#pragma comment (lib, "glpngd.lib")
#else
#pragma comment (lib, "glpng.lib")
#endif
#endif

    // XXX This is from Win32's <windef.h>
#ifndef APIENTRY
#if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define APIENTRY    __stdcall
#else
#define APIENTRY
#endif
#endif

    // Mipmapping parameters
#define GLPNG_NOMIPMAPS      0 // No mipmapping
#define GLPNG_BUILDMIPMAPS  -1 // Calls a clone of gluBuild2DMipmaps()
#define GLPNG_SIMPLEMIPMAPS -2 // Generates mipmaps without filtering

    // Who needs an "S" anyway?
#define GLPNG_NOMIPMAP     GLPNG_NOMIPMAPS
#define GLPNG_BUILDMIPMAP  GLPNG_BUILDMIPMAPS
#define GLPNG_SIMPLEMIPMAP GLPNG_SIMPLEMIPMAPS

    // Transparency parameters
#define GLPNG_CALLBACK  -3 // Call the callback function to generate alpha
#define GLPNG_ALPHA     -2 // Use alpha channel in PNG file, if there is one
#define GLPNG_SOLID     -1 // No transparency
#define GLPNG_STENCIL    0 // Sets alpha to 0 for r=g=b=0, 1 otherwise
#define GLPNG_BLEND1     1 // a = r+g+b
#define GLPNG_BLEND2     2 // a = (r+g+b)/2
#define GLPNG_BLEND3     3 // a = (r+g+b)/3
#define GLPNG_BLEND4     4 // a = r*r+g*g+b*b
#define GLPNG_BLEND5     5 // a = (r*r+g*g+b*b)/2
#define GLPNG_BLEND6     6 // a = (r*r+g*g+b*b)/3
#define GLPNG_BLEND7     7 // a = (r*r+g*g+b*b)/4
#define GLPNG_BLEND8     8 // a = sqrt(r*r+g*g+b*b)

    typedef struct {
        unsigned int Width;
        unsigned int Height;
        unsigned int Depth;
        unsigned int Alpha;
    } pngInfo;

    typedef struct {
        unsigned int Width;
        unsigned int Height;
        unsigned int Depth;
        unsigned int Alpha;

        unsigned int Components;
        unsigned char *Data;
        unsigned char *Palette;
    } pngRawInfo;

    extern unsigned int APIENTRY pngBind(const char *filename, int mipmap, int trans, pngInfo *info, int wrapst, int minfilter, int magfilter);

#ifdef __cplusplus
}
#endif

#endif
