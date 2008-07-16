#ifndef awt_asciiprint_hxx_included
#define awt_asciiprint_hxx_included


#define AWAR_APRINT               "tmp/aprint/"
#define AWAR_APRINT_TITLE         AWAR_APRINT "title"
#define AWAR_APRINT_TEXT          AWAR_APRINT "text"
#define AWAR_APRINT_PAPER_SIZE    AWAR_APRINT "paper_size"
#define AWAR_APRINT_MAGNIFICATION AWAR_APRINT "magnification"
#define AWAR_APRINT_SX            AWAR_APRINT "source_coloumns"
#define AWAR_APRINT_SY            AWAR_APRINT "source_rows"
#define AWAR_APRINT_DX            AWAR_APRINT "dest_cols"
#define AWAR_APRINT_DY            AWAR_APRINT "dest_rows"
#define AWAR_APRINT_ORIENTATION   AWAR_APRINT "orientation"
#define AWAR_APRINT_PAGES         AWAR_APRINT "pages"
#define AWAR_APRINT_PRINTTO       AWAR_APRINT "printto"
#define AWAR_APRINT_PRINTER       AWAR_APRINT "printer"
#define AWAR_APRINT_FILE          AWAR_APRINT "file"


enum AWT_asciiprint_orientation {
    AWT_APRINT_ORIENTATION_PORTRAIT,
    AWT_APRINT_ORIENTATION_LANDSCAPE,
    AWT_APRINT_ORIENTATION_DOUBLE_PORTRAIT
};


enum AWT_asciiprint_paper_size {
    AWT_APRINT_PAPERSIZE_A4,
    AWT_APRINT_PAPERSIZE_US
};

enum AWT_asciiprint_destination {
    AWT_APRINT_DEST_PRINTER,
    AWT_APRINT_DEST_FILE,
    AWT_APRINT_DEST_AFILE,
    AWT_APRINT_DEST_PREVIEW
};


#endif
