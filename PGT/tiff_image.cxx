// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#include "tiff_image.hxx"


/****************************************************************************
*  TIFFIMAGE - CONSTRUCTOR
****************************************************************************/
TIFFimage::TIFFimage()
{
    // PREDEFINE THE CLASS VARIABLES
    m_array=  NULL;
    m_width=  0;
    m_height= 0;
    m_size=   0;
    m_ximage_colormap= false;
    m_hasData= false;
}


/****************************************************************************
*  TIFFIMAGE - RETURN THE IMAGE WIDTH
****************************************************************************/
int TIFFimage::width()
{
    if(m_hasData) return (int)m_width;
    return 0;
}


/****************************************************************************
*  TIFFIMAGE - RETURN THE IMAGE HEIGHT
****************************************************************************/
int TIFFimage::height()
{
    if(m_hasData) return (int)m_height;
    return 0;
}


/****************************************************************************
*  TIFFIMAGE - RETURN THE IMAGE SIZE
****************************************************************************/
int TIFFimage::size()
{
    if(m_hasData) return (int)m_size;
    return 0;
}


/****************************************************************************
*  TIFFIMAGE - RETURN BOOLEAN ANSWER IF IMAGE DATA IS AVAILABLE
****************************************************************************/
bool TIFFimage::hasData() { return m_hasData; }


/****************************************************************************
*  TIFFIMAGE - OPEN A TIFF FILE AND IMPORT THE IMAGE DATA
****************************************************************************/
int TIFFimage::open(char *name)
{
    // DEFINE LOCAL VARIABLES
    TIFF *tiff= NULL;
    uint32 *array= NULL;
    uint32 width= 0;
    uint32 height= 0;
    uint32 size= 0;
    uint32 orientation= 0;
    bool error= false;

    // OPEN TIFF FiLENAME
    tiff= TIFFOpen(name, "r");

    // IF A HANDLE WAS RETURNED, CONTINUE...
    if(tiff)
    {
        // GET TIFF IMAGE DIMENSIONS
        TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
        size= width * height;

        // CREATE MEMORY ARRAY FOR THE TIFF IMAGE DATA
        array= (uint32 *)_TIFFmalloc(size * sizeof(uint32));

        // IF ARRAY WAS CREATED SUCCESSFULLY...
        if(array)
        {
            // FIX TIFF IMAGE ORIENTATION
            if(!TIFFGetFieldDefaulted(tiff, TIFFTAG_ORIENTATION, &orientation))
                orientation= 0;

            switch(orientation)
            {
                case ORIENTATION_TOPLEFT:
                case ORIENTATION_TOPRIGHT:
                case ORIENTATION_LEFTTOP:
                case ORIENTATION_RIGHTTOP:   orientation= ORIENTATION_BOTLEFT; break;

                case ORIENTATION_BOTRIGHT:
                case ORIENTATION_BOTLEFT:
                case ORIENTATION_RIGHTBOT:
                case ORIENTATION_LEFTBOT:    orientation= ORIENTATION_TOPLEFT; break;
            }
            TIFFSetField(tiff, TIFFTAG_ORIENTATION, orientation);

            if(!(TIFFReadRGBAImage(tiff, width, height, array, 0)))
                error= true;
        }
        else error= true; // ARRAY INIT FAILED
    }
    else error= true; // OPEN TIFF FILE FAILED (NO HANDLE)

    // CLOSE TIFF FILE
    if(tiff) TIFFClose(tiff);

    // IS THERE AN ERROR ?
    if(error)
    {
        _TIFFfree(array);
        return -1;
    }

    // IF OLD DATA IS AVAILABLE, CLEAN IT UP
    // CHECK, IF AN IMAGE IS AVAILABLE
    if(m_hasData) cleanUp();;

    // FILL LOCAL VARIABLES
    m_array= array;
    m_width= width;
    m_height= height;
    m_size= size;
    m_hasData= true;

    // SAVE FILE NAME
    m_name= (char *)malloc((strlen(name) + 1)* sizeof(char));
    strcpy(m_name, name);

    // CONVERT COLORMAP TO XIMAGE FORMAT
    fixRGB();
    m_ximage_colormap= true;

    return 0;
}


/****************************************************************************
*  TIFFIMAGE -  CORRECTS THE COLORMAP OF EACH PIXEL
*  FOR THE USE IN AN XIMAGE (SWITCHES R <-> B BYTE)
****************************************************************************/
void TIFFimage::fixRGB()
{
    // CHECK, IF AN IMAGE IS AVAILABLE
    if(!m_hasData) return;

    // SOME LOCAL VARIABLES
    uint32 i, p;

    // THE FOLLOWING LOOP SWITCHES THE R<->B COLOR BYTES FOR EACH PIXEL
    for(i= 0; i < m_size; i++)
    {
        p= m_array[i] & 0xFF00FF00;
        p= p | ((m_array[i] & 0x000000FF) << 16);
        p= p | ((m_array[i] & 0x00FF0000) >> 16);
        m_array[i]= p;
    }

    // INVERT COLORMAP FLAG
    m_ximage_colormap= ~m_ximage_colormap;
}


/****************************************************************************
*  TIFFIMAGE - RESET ALL ENTRIES, FREE ALLOCATED MEM IF NEEDED
****************************************************************************/
void TIFFimage::cleanUp()
{
    // CHECK, IF AN IMAGE IS AVAILABLE
    if(!m_hasData) return;

    // FREE ALLOCATED NAME MEMORY
    free(m_name);
    m_name= NULL;

    // FREE ALLOCATED IMAGE MEMORY
    _TIFFfree(m_array);
    m_array= NULL;

    // RESET CLASS VARIABLES
    m_hasData= false;
    m_width= 0;
    m_height= 0;
    m_size= 0;
    m_ximage_colormap= false;
}


/****************************************************************************
*  TIFFIMAGE - RESET ALL ENTRIES, FREE ALLOCATED MEM IF NEEDED
****************************************************************************/
XImage *TIFFimage::createXImage(Widget widget)
{
    // CHECK, IF AN IMAGE IS AVAILABLE
    if(!m_hasData) return NULL;

    // CREATE XIMAGE VARIABLE
    XImage *ximage;

    // FETCH ENVIRONMENT SETTINGS
    Display *display= XtDisplay(widget);
    Visual *visual= XDefaultVisual(display, 0);

    // INVERT COLORMAP IF NOT ALREADY DONE
    if(!m_ximage_colormap) fixRGB();

    // CREATE XIMAGE FROM ARRAY DATA
    ximage= XCreateImage(display, visual, 24, ZPixmap, 0,
        (char *)m_array, m_width, m_height, 32, 0);

    return ximage;
}


/****************************************************************************
*  TIFFIMAGE - MASK IMAGE COLOURS
****************************************************************************/
void TIFFimage::colorFilter(uint32 mask)
{
    // TODO:  MASKING IS INFLUENCED BY THE POSITION OF THE R<->B BYTES
    // SEE: RGBFIX FUNCTION !!!


    // CHECK, IF AN IMAGE IS AVAILABLE
    if(!m_hasData) return;

    // INVERT COLORMAP IF NOT ALREADY DONE
    if(!m_ximage_colormap) fixRGB();

    // SOME LOCAL VARIABLES
    uint32 i, p;

    // IN THIS LOOP EVERY PIXEL IN THE ARRAY IS MASKED
    // WITH THE PREDEFINED COLOR
    for(i= 0; i < m_size; i++)
    {
        // GET ARRAY ENTRY
        p= m_array[i];

        // DO SOME MASKING HERE...
        p= p | mask;

        // WRITE BACK ARRAY ENTRY
        m_array[i]= p;
    }
}

