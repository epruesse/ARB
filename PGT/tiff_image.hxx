// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef TIFF_IMAGE_H
#define TIFF_IMAGE_H

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <tiffio.h>
//
#include <Xm/XmAll.h>
//
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


// RGBA COLOR MASKS (Hex: 0x00RRGGBB)
#define MASK_RED   0x00FF0000
#define MASK_GREEN 0x0000FF00
#define MASK_BLUE  0x000000FF


class TIFFimage
{
    public:
        TIFFimage();
        int width();
        int height();
        int size();
        bool hasData();
        int open(char *);
        XImage *createXImage(Widget);
        void colorFilter(uint32);
    protected:
        void fixRGB();
        void cleanUp();
    private:
        uint32 *m_array; // POINTER TO IMAGE ARRAY IN MEMORY
        uint32 m_width;  // IMAGE WIDTH (PIXELS)
        uint32 m_height; // IMAGE HEIGHT (PIXELS)
        uint32 m_size;   // IMAGE SIZE (WIDTH * HEIGHT)
        bool m_ximage_colormap; // IS THE DATA USING XIMAGE COLORMAP?
        bool m_hasData;
        char *m_name;
};

#endif // TIFF_IMAGE_H
