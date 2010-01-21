// =============================================================== //
//                                                                 //
//   File      : aw_xfigfont.hxx                                   //
//   Purpose   : implements simple vector fonts                    //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AW_XFIGFONT_HXX
#define AW_XFIGFONT_HXX

const int XFIG_FONT_ELEMS          = 256;
const int XFIG_FONT_VISIBLE_HEIGHT = 6;

struct AW_xfig_vectorfont {
    short gridsizex, gridsizey;
    // address of the default symbol: NOTE: undefined characters point to this one, too!
    AW_xfig_line *default_symbol;
    AW_xfig_line **lines;
};

AW_xfig_vectorfont *aw_read_xfigfont(char *filename);
void aw_xfig_font_deletefont(AW_root *aw_root);
void aw_xfig_font_changefont_cb(AW_root *aw_root);

#else
#error aw_xfigfont.hxx included twice
#endif // AW_XFIGFONT_HXX
