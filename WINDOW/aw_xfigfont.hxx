#ifndef AW_XFIGFONT_HXX
#define AW_XFIGFONT_HXX

/* -----------------------------------------------------------------
 * Module:                        WINDOW/aw_xfigfont.hxx
 *
 * Global Functions:		  AW_read_xfigfont
 *
 * Description:                   implementing simple vector fonts
 * 
 * -----------------------------------------------------------------
 */

/*  
 * $Header$
 *
 * $Log$
 * Revision 1.2  2005/01/05 11:25:40  westram
 * - changed include wrapper
 *
 * Revision 1.1.1.1  2000/11/23 09:41:17  westram
 * Erster Import
 *
 * Revision 1.7  1995/03/13  16:53:41  jakobi
 * *** empty log message ***
 *
 * Revision 1.6  1995/03/13  12:23:48  jakobi
 * *** empty log message ***
 *
 * Revision 1.6  1995/03/13  12:23:48  jakobi
 * *** empty log message ***
 *
 * Revision 1.5  1995/02/03  17:08:38  jakobi
 * *** empty log message ***
 *
 * Revision 1.4  1995/02/01  13:24:37  jakobi
 * *** empty log message ***
 *
 * Revision 1.3  1995/01/27  17:32:08  jakobi
 * *** empty log message ***
 *
 * Revision 1.2  1995/01/13  20:23:42  jakobi
 * *** empty log message ***
 *
 * Revision 1.1  1994/12/21  11:21:33  jakobi
 * Initial revision
 *
 */

// #ifndef AW_XFIGFONT_HXX
// #define AW_XFIGFONT_HXX 1


/* includes */

/* exported constants */
const int XFIG_FONT_ELEMS = 256;
const int XFIG_FONT_VISIBLE_HEIGHT = 6;
typedef struct AW_xfig_vectorfont{
        short gridsizex,gridsizey;
	// address of the default symbol: NOTE: undefined characters point to this one, too!
        AW_xfig_line *default_symbol;
	AW_xfig_line **lines;
} AW_vectorfont;

/* prototypes */
AW_vectorfont *aw_read_xfigfont(char *filename);
void aw_xfig_font_deletefont(AW_root *aw_root);
void aw_xfig_font_changefont_cb(AW_root *aw_root);
// #endif

#else
#error aw_xfigfont.hxx included twice
#endif
