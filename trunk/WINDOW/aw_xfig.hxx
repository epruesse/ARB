#ifndef aw_xfig_hxx_included
#define aw_xfig_hxx_included

/* -----------------------------------------------------------------
 * Module:                        WINDOW/aw_xfig.hxx
 *
 * Exported Classes:              xfig
 *
 * Description: xfig stuff
 *
 * -----------------------------------------------------------------
 */

/*
 * $Header$
 *
 * $Log$
 * Revision 1.2  2001/08/17 19:56:47  westram
 * * aw_xfig.hxx: - new method AW_xfig::add_line
 *            - new constructor (constructing empty xfig)
 *
 * Revision 1.1.1.1  2000/11/23 09:41:17  westram
 * Erster Import
 *
 * Revision 1.6  1995/03/13  16:53:41  jakobi
 * *** empty log message ***
 *
 * Revision 1.5  1995/03/13  12:23:48  jakobi
 * *** empty log message ***
 *
 */


const int XFIG_DEFAULT_FONT_SIZE = 13;
const int MAX_LINE_WIDTH = 20;
const int MAX_XFIG_LENGTH = 100000;

char *aw_get_font_from_xfig(int fontnr);

struct AW_xfig_line {
	struct AW_xfig_line *next;
	short	x0,y0;
	short	x1,y1;
	short	color;
	int	gc;
};

struct AW_xfig_text {
	struct AW_xfig_text *next;

	short    x,y;
	short    pix_len;
	char	*text;
	AW_font  font;
	short	 fontsize;
	int	     center;
	short	 color;
	int	     gc;
};

struct AW_xfig_pos {
	short x,y;
	int	center;
};

class AW_xfig {
public:
	struct AW_xfig_text *text;
	struct AW_xfig_line *line[MAX_LINE_WIDTH];

	GB_HASH *hash;	            // hash table for buttons

	int	minx,miny;
	int	maxx,maxy;
    int	size_x,size_y;
	int	centerx,centery;

	double font_scale;
	double dpi_scale;

	AW_xfig(const char *filename, int fontsize);
	AW_xfig(int fontsize);      // creates an empty drawing area

	~AW_xfig(void);
	void print(AW_device *device); // you can scale it
	void create_gcs(AW_device *device, int screen_depth); // create the gcs

    void add_line(int x1, int y2, int x2, int y2, int width); // add a line to xfig
};


/* xfig format ::

text::

4 0 font(-1== default) fontsize depth color  ?? angle justified(4=left) flags width x y text^A

lines::

2 art depth width color ....
	x y x y 9999 9999

*/


#endif
