// ================================================================ //
//                                                                  //
//   File      : AWT_canio.cxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "awt_canvas.hxx"

#include <aw_file.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <arbdbt.h>
#include <arb_defs.h>
#include <arb_strbuf.h>
#include <arb_file.h>

using namespace AW;

#define awt_assert(cond) arb_assert(cond)

// --------------------------------------------------------------------------------

#define AWAR_CANIO                 "NT/print/"
#define AWAR_CANIO_LANDSCAPE       AWAR_CANIO "landscape"
#define AWAR_CANIO_CLIP            AWAR_CANIO "clip"
#define AWAR_CANIO_HANDLES         AWAR_CANIO "handles"
#define AWAR_CANIO_COLOR           AWAR_CANIO "color"
#define AWAR_CANIO_DEST            AWAR_CANIO "dest"
#define AWAR_CANIO_PRINTER         AWAR_CANIO "printer"
#define AWAR_CANIO_OVERLAP_WANTED  AWAR_CANIO "overlap"
#define AWAR_CANIO_OVERLAP_PERCENT AWAR_CANIO "operc"
#define AWAR_CANIO_BORDERSIZE      AWAR_CANIO "border"
#define AWAR_CANIO_PAPER           AWAR_CANIO "paper"
#define AWAR_CANIO_PAPER_USABLE    AWAR_CANIO "useable"
#define AWAR_CANIO_F2DBUG          AWAR_CANIO "f2dbug"
#define AWAR_CANIO_PAGES           AWAR_CANIO "pages"
#define AWAR_CANIO_PAGELOCK        AWAR_CANIO "plock"

#define AWAR_CANIO_TMP "tmp/" AWAR_CANIO

#define AWAR_CANIO_MAGNIFICATION AWAR_CANIO_TMP "magnification"
#define AWAR_CANIO_OVERLAP       AWAR_CANIO_TMP "overlap"
#define AWAR_CANIO_GFX_SX        AWAR_CANIO_TMP "gsizex" // graphic size in inch
#define AWAR_CANIO_GFX_SY        AWAR_CANIO_TMP "gsizey"
#define AWAR_CANIO_OUT_SX        AWAR_CANIO_TMP "osizex" // output size in inch
#define AWAR_CANIO_OUT_SY        AWAR_CANIO_TMP "osizey"
#define AWAR_CANIO_PAPER_SX      AWAR_CANIO_TMP "psizex" // paper size in inch
#define AWAR_CANIO_PAPER_SY      AWAR_CANIO_TMP "psizey"
#define AWAR_CANIO_PAGE_SX       AWAR_CANIO_TMP "sizex"  // size in pages
#define AWAR_CANIO_PAGE_SY       AWAR_CANIO_TMP "sizey"

#define AWAR_CANIO_FILE_BASE   AWAR_CANIO_TMP "file"
#define AWAR_CANIO_FILE_NAME   AWAR_CANIO_FILE_BASE "/file_name"
#define AWAR_CANIO_FILE_DIR    AWAR_CANIO_FILE_BASE "/directory"
#define AWAR_CANIO_FILE_FILTER AWAR_CANIO_FILE_BASE "/filter"

// --------------------------------------------------------------------------------

enum PrintDest {
    PDEST_PRINTER    = 0,
    PDEST_POSTSCRIPT = 1,
    PDEST_PREVIEW    = 2,
};

// --------------------------------------------------------------------------------

enum LengthUnit { INCH, MM };

static const double mm_per_inch = 25.4;
inline double inch2mm(double inches) { return inches*mm_per_inch; }
inline double mm2inch(double mms) { return mms/mm_per_inch; }

class PaperFormat {
    const char *description;
    const char *fig2dev_val;
    LengthUnit  unit;
    double      shortSide, longSide;

public:
    PaperFormat(const char *name, LengthUnit lu, double shortSide_, double longSide_)
        : description(name),
          fig2dev_val(name),
          unit(lu),
          shortSide(shortSide_),
          longSide(longSide_)
    {
        awt_assert(shortSide<longSide);
    }
    PaperFormat(const char *aname, const char *fname, LengthUnit lu, double shortSide_, double longSide_)
        : description(aname),
          fig2dev_val(fname),
          unit(lu),
          shortSide(shortSide_),
          longSide(longSide_)
    {
        awt_assert(shortSide<longSide);
    }

    double short_inch() const { return unit == INCH ? shortSide : mm2inch(shortSide); }
    double long_inch() const { return unit == INCH ? longSide : mm2inch(longSide); }

    const char *get_description() const { return description; }
    const char *get_fig2dev_val() const { return fig2dev_val; }
};

// (c&p from fig2dev:)
//  Available paper sizes are:
//      "Letter" (8.5" x 11" also "A"),
//      "Legal" (11" x 14")
//      "Ledger" (11" x 17"),
//      "Tabloid" (17" x 11", really Ledger in Landscape mode),
//      "A" (8.5" x 11" also "Letter"),
//      "B" (11" x 17" also "Ledger"),
//      "C" (17" x 22"),
//      "D" (22" x 34"),
//      "E" (34" x 44"),
//      "A4" (21  cm x  29.7cm),
//      "A3" (29.7cm x  42  cm),
//      "A2" (42  cm x  59.4cm),
//      "A1" (59.4cm x  84.1cm),
//      "A0" (84.1cm x 118.9cm),
//      and "B5" (18.2cm x 25.7cm).

static PaperFormat knownPaperFormat[] = {
    PaperFormat("A4", MM, 210, 297), // first is the default
    PaperFormat("A3", MM, 297, 420),
    PaperFormat("A2", MM, 420, 594),
    PaperFormat("A1", MM, 594, 841),
    PaperFormat("A0", MM, 841, 1189),
    PaperFormat("B5", MM, 182, 257),
    
    PaperFormat("A (Letter)", "A", INCH, 8.5, 11),
    PaperFormat("Legal",           INCH, 11, 14),
    PaperFormat("B (Ledger)", "B", INCH, 11, 17),
    PaperFormat("C",          "C", INCH, 17, 22),
    PaperFormat("D",          "D", INCH, 22, 34),
    PaperFormat("E",          "E", INCH, 34, 44),
};

// --------------------------------------------------------------------------------

static Rectangle get_drawsize(AWT_canvas *scr, bool draw_all) {
    // returns size of drawn graphic in screen-coordinates
    
    Rectangle       drawsize;
    GB_transaction  ta(scr->gb_main);
    AW_device_size *size_device = scr->aww->get_size_device(AW_MIDDLE_AREA);

    if (draw_all) {
        size_device->reset();
        size_device->zoom(scr->trans_to_fit);
        size_device->set_filter(AW_PRINTER|AW_PRINTER_EXT);
        scr->gfx->show(size_device);
        drawsize = size_device->get_size_information();
    }
    else {
        drawsize = Rectangle(size_device->get_area_size(), INCLUSIVE_OUTLINE);
    }

    return drawsize;
}

static Rectangle add_border_to_drawsize(const Rectangle& drawsize, double border_percent) {
    double borderratio     = border_percent*.01;
    double draw2bordersize = -borderratio/(borderratio-1.0);
    Vector bordersize      = drawsize.diagonal() * (draw2bordersize*0.5);

    return Rectangle(drawsize.upper_left_corner()-bordersize,
                     drawsize.lower_right_corner()+bordersize);
}

static void awt_print_tree_check_size(void *, AW_CL cl_canvas) {
    AWT_canvas *scr         = (AWT_canvas*)cl_canvas;
    AW_root    *awr         = scr->awr;
    long        draw_all    = awr->awar(AWAR_CANIO_CLIP)->read_int();
    double      border      = awr->awar(AWAR_CANIO_BORDERSIZE)->read_float();
    Rectangle   drawsize    = get_drawsize(scr, draw_all);
    Rectangle   with_border = add_border_to_drawsize(drawsize, border);

    awr->awar(AWAR_CANIO_GFX_SX)->write_float((with_border.width())/DPI_SCREEN);
    awr->awar(AWAR_CANIO_GFX_SY)->write_float((with_border.height())/DPI_SCREEN);
}

inline int xy2pages(double sx, double sy) {
    return (int(sx+0.99)*int(sy+0.99));
}

static bool allow_page_size_check_cb = true;
static bool allow_overlap_toggled_cb = true;

inline void set_paper_size_xy(AW_root *awr, double px, double py) {
    // modify papersize but perform only one callback

    bool old_allow           = allow_page_size_check_cb;
    allow_page_size_check_cb = false;
    awr->awar(AWAR_CANIO_PAPER_SX)->write_float(px);
    allow_page_size_check_cb = old_allow;
    awr->awar(AWAR_CANIO_PAPER_SY)->write_float(py);
}

static void overlap_toggled_cb(AW_root *awr) {
    if (allow_overlap_toggled_cb) {
        int new_val = awr->awar(AWAR_CANIO_OVERLAP)->read_int();
        awr->awar(AWAR_CANIO_OVERLAP_WANTED)->write_int(new_val);
    }
}

static long calc_mag_from_psize(AW_root *awr, double papersize, double gfxsize, double wantedpages, bool use_x) {
    bool   wantOverlap = awr->awar(AWAR_CANIO_OVERLAP_WANTED)->read_int();
    double usableSize = 0;

    if (wantOverlap && wantedpages>1) {
        double overlapPercent = awr->awar(AWAR_CANIO_OVERLAP_PERCENT)->read_float();
        double usableRatio    = (100.0-overlapPercent)/100.0;

        // See also fig2devbug in page_size_check_cb() 
        bool fig2devbug = !use_x && awr->awar(AWAR_CANIO_F2DBUG)->read_int();
        if (fig2devbug) {
            bool landscape = awr->awar(AWAR_CANIO_LANDSCAPE)->read_int();
            fig2devbug     = fig2devbug && !landscape; // only occurs in portrait mode
        }

        if (fig2devbug) {
            usableSize = wantedpages*papersize*usableRatio;
        }
        else {
            usableSize = (papersize*usableRatio)*(wantedpages-1)+papersize;
        }
    }
    else {
        usableSize = wantedpages*papersize;
    }

    return usableSize*100/gfxsize; // always "round" to floor
}

static void set_mag_from_psize(AW_root *awr, bool use_x) {
    const char *papersize_name   = use_x ? AWAR_CANIO_PAPER_SX : AWAR_CANIO_PAPER_SY;
    const char *gfxsize_name     = use_x ? AWAR_CANIO_GFX_SX : AWAR_CANIO_GFX_SY;
    const char *wantedpages_name = use_x ? AWAR_CANIO_PAGE_SX : AWAR_CANIO_PAGE_SY;

    double   papersize   = awr->awar(papersize_name)->read_float();
    double   gfxsize     = awr->awar(gfxsize_name)->read_float();
    double   wantedpages = awr->awar(wantedpages_name)->read_float();
    long     mag         = calc_mag_from_psize(awr, papersize, gfxsize, wantedpages, use_x);

    awr->awar(AWAR_CANIO_MAGNIFICATION)->write_int(mag);
}

static void fit_pages(AW_root *awr, int wanted_pages, bool allow_orientation_change) {
    int         best_orientation    = -1;
    const char *best_zoom_awar_name = 0;
    float       best_zoom           = 0;
    int         best_magnification  = 0;
    int         best_pages          = 0;

    bool lockpages = awr->awar(AWAR_CANIO_PAGELOCK)->read_int();
    awr->awar(AWAR_CANIO_PAGELOCK)->write_int(0);

    if (!allow_orientation_change) {
        best_orientation = awr->awar(AWAR_CANIO_LANDSCAPE)->read_int();
    }

    for (int o = 0; o <= 1; ++o) {
        if (!allow_orientation_change && o != best_orientation) continue;

        awr->awar(AWAR_CANIO_LANDSCAPE)->write_int(o); // set orientation (calls page_size_check_cb)

        for (int xy = 0; xy <= 1; ++xy) {
            const char *awar_name = xy == 0 ? AWAR_CANIO_PAGE_SX : AWAR_CANIO_PAGE_SY;

            for (int pcount = 1; pcount <= wanted_pages; pcount++) {
                double zoom = pcount*1.0;
                awr->awar(awar_name)->write_float(zoom); // set zoom (x or y)

                set_mag_from_psize(awr, xy == 0);

                double sx            = awr->awar(AWAR_CANIO_PAGE_SX)->read_float();
                double sy            = awr->awar(AWAR_CANIO_PAGE_SY)->read_float();
                int    pages         = xy2pages(sx, sy);

                if (pages>wanted_pages) break; // pcount-loop

                if (pages <= wanted_pages && pages >= best_pages) {
                    int magnification = awr->awar(AWAR_CANIO_MAGNIFICATION)->read_int();    // read calculated magnification
                    if (magnification>best_magnification) {
                        // fits on wanted_pages and is best result yet
                        best_magnification  = magnification;
                        best_orientation    = o;
                        best_zoom_awar_name = awar_name;
                        best_zoom           = zoom;
                        best_pages          = pages;
                    }
                }
            }
        }
    }

    if (best_zoom_awar_name) {
        // take the best found values :
        awr->awar(AWAR_CANIO_LANDSCAPE)->write_int(best_orientation);
        awr->awar(best_zoom_awar_name)->write_float(best_zoom);
        awr->awar(AWAR_CANIO_PAGES)->write_int(best_pages);
        awr->awar(AWAR_CANIO_MAGNIFICATION)->write_int(best_magnification); 

        if (best_pages != wanted_pages) {
            aw_message(GBS_global_string("That didn't fit on %i page(s) - using %i page(s)",
                                         wanted_pages, best_pages));
        }
    }
    else {
        aw_message(GBS_global_string("That didn't fit on %i page(s) - impossible settings", wanted_pages));
    }
    
    awr->awar(AWAR_CANIO_PAGELOCK)->write_int(lockpages);
}

static void page_size_check_cb(AW_root *awr) {
    if (!allow_page_size_check_cb) return;

    bool landscape    = awr->awar(AWAR_CANIO_LANDSCAPE)->read_int();
    bool lockpages    = awr->awar(AWAR_CANIO_PAGELOCK)->read_int();
    int  locked_pages = awr->awar(AWAR_CANIO_PAGES)->read_int();

    double px = awr->awar(AWAR_CANIO_PAPER_SX)->read_float(); // paper-size
    double py = awr->awar(AWAR_CANIO_PAPER_SY)->read_float();

    if (landscape != (px>py)) {
        set_paper_size_xy(awr, py, px); // recalls this function
        return;
    }

    long magnification = awr->awar(AWAR_CANIO_MAGNIFICATION)->read_int();
    
    double gx = awr->awar(AWAR_CANIO_GFX_SX)->read_float(); // graphic size
    double gy = awr->awar(AWAR_CANIO_GFX_SY)->read_float();

    // output size
    double ox = (gx*magnification)/100; // resulting size of output in inches
    double oy = (gy*magnification)/100;
    awr->awar(AWAR_CANIO_OUT_SX)->write_float(ox);
    awr->awar(AWAR_CANIO_OUT_SY)->write_float(oy);

    bool wantOverlap = awr->awar(AWAR_CANIO_OVERLAP_WANTED)->read_int();
    bool useOverlap  = awr->awar(AWAR_CANIO_OVERLAP)->read_int();

    double sx = 0.0; // resulting pages
    double sy = 0.0;

    bool fits_on_one_page = (ox <= px && oy <= py);

    if (wantOverlap && !fits_on_one_page) {
        double overlapPercent = awr->awar(AWAR_CANIO_OVERLAP_PERCENT)->read_float();
        double pageRatio      = (100.0-overlapPercent)/100.0;

        double rpx = px*pageRatio; // usable page-size (due to overlap)
        double rpy = py*pageRatio;

        while (ox>px) { ox -= rpx; sx += 1.0; }
        sx += ox/px;

        bool fig2devbug = !landscape;
        if (fig2devbug) fig2devbug = awr->awar(AWAR_CANIO_F2DBUG)->read_int();

        if (fig2devbug) {
            // fig2dev has a bug with printing multiple portrait pages with overlap:
            // Regardless of whether there is a needed overlapping in y-direction
            // it prints a empty border at top of all top-row-pages and then clips
            // the fig in the bottom-row (even if there is only one row).
            // See also fig2devbug in calc_mag_from_psize() 
            sy += oy/rpy;
        }
        else {
            while (oy>py) { oy -= rpy; sy += 1.0; }
            sy += oy/py;
        }
    }
    else {
        sx = ox/px;
        sy = oy/py;
    }

    // write amount of pages needed
    awr->awar(AWAR_CANIO_PAGE_SX)->write_float(sx);
    awr->awar(AWAR_CANIO_PAGE_SY)->write_float(sy);

    int pages = xy2pages(sx, sy);
    if (lockpages) {
        fit_pages(awr, locked_pages, false); // Note: recurses with !lockpages
        return;
    }

    awr->awar(AWAR_CANIO_PAGES)->write_int(pages);

    // correct DISPLAYED overlapping..
    bool willUseOverlap = wantOverlap && (pages != 1);
    if (willUseOverlap != useOverlap) {
        bool old_allow           = allow_overlap_toggled_cb;
        allow_overlap_toggled_cb = false; // ..but do not modify wanted overlapping

        awr->awar(AWAR_CANIO_OVERLAP)->write_int(willUseOverlap);

        allow_overlap_toggled_cb = old_allow;
    }
}

inline double round_psize(double psize) { return AW_INT(psize*10)/10.0; }

static void paper_changed_cb(AW_root *awr) {
    int                paper     = awr->awar(AWAR_CANIO_PAPER)->read_int();
    const PaperFormat& format    = knownPaperFormat[paper];
    bool               landscape = awr->awar(AWAR_CANIO_LANDSCAPE)->read_int();
    double             useable   = awr->awar(AWAR_CANIO_PAPER_USABLE)->read_float()/100.0;

    if (landscape) {
        set_paper_size_xy(awr, useable*format.long_inch(), useable*format.short_inch());
    }
    else {
        set_paper_size_xy(awr, useable*format.short_inch(), useable*format.long_inch());
    }
}

// --------------------------------------------------------------------------------

static bool export_awars_created = false;
static bool print_awars_created = false;

static void create_export_awars(AW_root *awr) {
    if (!export_awars_created) {
        AW_default def = AW_ROOT_DEFAULT;

        awr->awar_int(AWAR_CANIO_CLIP, 0, def);
        awr->awar_int(AWAR_CANIO_HANDLES, 1, def);
        awr->awar_int(AWAR_CANIO_COLOR, 1, def);

        awr->awar_string(AWAR_CANIO_FILE_NAME, "print.fig", def);
        awr->awar_string(AWAR_CANIO_FILE_DIR, "", def);
        awr->awar_string(AWAR_CANIO_FILE_FILTER, "fig", def);

        awr->awar_int(AWAR_CANIO_LANDSCAPE, 0, def);
        awr->awar_int(AWAR_CANIO_MAGNIFICATION, 100, def);

        // constraints:

        awr->awar(AWAR_CANIO_MAGNIFICATION)->set_minmax(1, 10000);

        export_awars_created = true;
    }
}

static void resetFiletype(AW_root *awr, const char *filter, const char *defaultFilename) {
    AW_awar *awar_filter    = awr->awar(AWAR_CANIO_FILE_FILTER);
    char    *current_filter = awar_filter->read_string();

    if (strcmp(current_filter, filter) != 0) {
        awar_filter->write_string(filter);
        awr->awar(AWAR_CANIO_FILE_NAME)->write_string(defaultFilename);
    }
    free(current_filter);
}

static void create_print_awars(AW_root *awr, AWT_canvas *scr) {
    create_export_awars(awr);

    if (!print_awars_created) {
        AW_default def = AW_ROOT_DEFAULT;

        awr->awar_int(AWAR_CANIO_PAGES, 1, def);
        awr->awar_int(AWAR_CANIO_PAGELOCK, 1, def);

        awr->awar_int(AWAR_CANIO_OVERLAP_WANTED, 1, def);
        awr->awar_int(AWAR_CANIO_OVERLAP, 1, def)->add_callback(overlap_toggled_cb);
        awr->awar_float(AWAR_CANIO_OVERLAP_PERCENT, 13.0, def); // determined

        awr->awar_int(AWAR_CANIO_F2DBUG, 1, def); // bug still exists in most recent version (3.2 5d from July 23, 2010)

        awr->awar_float(AWAR_CANIO_BORDERSIZE, 0.0, def);

        awr->awar_float(AWAR_CANIO_GFX_SX);
        awr->awar_float(AWAR_CANIO_GFX_SY);

        awr->awar_float(AWAR_CANIO_OUT_SX);
        awr->awar_float(AWAR_CANIO_OUT_SY);

        awr->awar_float(AWAR_CANIO_PAPER_SX);
        awr->awar_float(AWAR_CANIO_PAPER_SY);

        awr->awar_int(AWAR_CANIO_PAPER, 0)->add_callback(paper_changed_cb); // sets first format (A4)
        awr->awar_float(AWAR_CANIO_PAPER_USABLE, 95)->add_callback(paper_changed_cb); // 95% of paper are usable

        awr->awar_float(AWAR_CANIO_PAGE_SX);
        awr->awar_float(AWAR_CANIO_PAGE_SY);

        awr->awar_int(AWAR_CANIO_DEST);

        {
            char *print_command;
            if (getenv("PRINTER")) {
                print_command = GBS_eval_env("lpr -h -P$(PRINTER)");
            } else   print_command = strdup("lpr -h");

            awr->awar_string(AWAR_CANIO_PRINTER, print_command, def);
            free(print_command);
        }

        // constraints and automatics:

        awr->awar(AWAR_CANIO_PAPER_SX)->set_minmax(0.1, 100);
        awr->awar(AWAR_CANIO_PAPER_SY)->set_minmax(0.1, 100);

        awt_print_tree_check_size(0, (AW_CL)scr);

        awr->awar(AWAR_CANIO_CLIP)->add_callback((AW_RCB1)awt_print_tree_check_size, (AW_CL)scr);
        awr->awar(AWAR_CANIO_BORDERSIZE)->add_callback((AW_RCB1)awt_print_tree_check_size, (AW_CL)scr);

        { // add callbacks for page recalculation
            const char *checked_awars[] = {
                AWAR_CANIO_MAGNIFICATION,
                AWAR_CANIO_LANDSCAPE,
                AWAR_CANIO_BORDERSIZE,
                AWAR_CANIO_OVERLAP_WANTED, AWAR_CANIO_OVERLAP_PERCENT,
                AWAR_CANIO_F2DBUG, 
                AWAR_CANIO_PAPER_SX,         AWAR_CANIO_PAPER_SY,
                AWAR_CANIO_GFX_SX,         AWAR_CANIO_GFX_SY,
                0
            };
            for (int ca = 0; checked_awars[ca]; ca++) {
                awr->awar(checked_awars[ca])->add_callback(page_size_check_cb);
            }
        }

        paper_changed_cb(awr); // also calls page_size_check_cb
        print_awars_created = true;
    }
}

// --------------------------------------------------------------------------------

static GB_ERROR canvas_to_xfig(AWT_canvas *scr, const char *xfig_name, bool add_invisibles, double border) {
    // if 'add_invisibles' is true => print 2 invisible dots to make fig2dev center correctly

    GB_transaction  ta(scr->gb_main);
    AW_root        *awr = scr->awr;

    bool draw_all  = awr->awar(AWAR_CANIO_CLIP)->read_int();
    bool handles   = awr->awar(AWAR_CANIO_HANDLES)->read_int();
    bool use_color = awr->awar(AWAR_CANIO_COLOR)->read_int();

    AW_device_print *device = scr->aww->get_print_device(AW_MIDDLE_AREA);

    device->reset();
    device->set_color_mode(use_color);
#ifdef ARB_GTK
    GB_ERROR error = NULL;
#else
    GB_ERROR error = device->open(xfig_name);
#endif

    if (!error) {
        Rectangle drawsize = get_drawsize(scr, draw_all);
        Rectangle world_drawsize;

        if (draw_all) {
            Rectangle with_border = add_border_to_drawsize(drawsize, border);

            double zoom = scr->trans_to_fit;
            device->zoom(zoom); // same zoom as used by get_drawsize above

            world_drawsize = device->rtransform(drawsize);

            Vector ulc2origin = Origin-with_border.upper_left_corner();
            Vector offset     = device->rtransform(ulc2origin)*device->get_unscale();

            device->set_offset(offset);

            device->set_bottom_clip_border((int)(with_border.height()+1), true);
            device->set_right_clip_border((int)(with_border.width()+1), true);
        }
        else {
            scr->init_device(device);
            world_drawsize = device->rtransform(drawsize);
        }

        AW_bitset filter       = AW_PRINTER;
        if (handles) filter   |= AW_PRINTER_EXT;
        if (!draw_all) filter |= AW_PRINTER_CLIP;

        device->set_filter(filter);
#ifdef ARB_GTK
        error = device->open(xfig_name);
        if (error) return error;
#endif     
        scr->gfx->show(device);

        if (add_invisibles) {
            Position ul = world_drawsize.upper_left_corner();
            Position lr = world_drawsize.lower_right_corner();

            // move invisible points towards center (in case they are clipped away)
            Vector stepInside = device->rtransform(Vector(1, 1));
            int maxloop = 10;

            bool drawnUL = false;
            bool drawnLR = false;

            while (!(drawnLR && drawnUL) && maxloop-->0) {
                if (!drawnUL) {
                    drawnUL  = device->invisible(ul);
                    ul      += stepInside;
                }
                if (!drawnLR) {
                    drawnLR  = device->invisible(lr);
                    lr      -= stepInside;
                }
            }
            awt_assert(drawnLR && drawnUL);
        }

        device->close();
    }
    return error;
}

// --------------------------------------------------------------------------------

static void canvas_to_xfig_and_run_xfig(AW_window *aww, AWT_canvas *scr) {
    AW_root *awr  = aww->get_root();
    char    *xfig = AW_get_selected_fullname(awr, AWAR_CANIO_FILE_BASE);

    GB_ERROR error = NULL;
    if (xfig[0] == 0) {
        error = "Please enter a file name";
    }
    else {
        error = canvas_to_xfig(scr, xfig, true, 0.0);
        if (!error) {
            awr->awar(AWAR_CANIO_FILE_DIR)->touch(); // reload dir to show created xfig
#if defined(ARB_GTK)
            error = GBK_system(GBS_global_string("evince %s &", xfig));
#else
            error = GBK_system(GBS_global_string("xfig %s &", xfig));
#endif
        }
    }
    if (error) aw_message(error);
    free(xfig);
}

static void canvas_to_printer(AW_window *aww, AW_CL cl_canvas) {
    AWT_canvas     *scr       = (AWT_canvas*)cl_canvas;
    GB_transaction  ta(scr->gb_main);
    AW_root        *awr       = aww->get_root();
    GB_ERROR        error     = 0;
    char           *dest      = 0;
    PrintDest       printdest = (PrintDest)awr->awar(AWAR_CANIO_DEST)->read_int();

    switch (printdest) {
        case PDEST_POSTSCRIPT: {
            dest = AW_get_selected_fullname(awr, AWAR_CANIO_FILE_BASE);
            FILE *out = fopen(dest, "w");
            if (!out) error = GB_export_IO_error("writing", dest);
            else fclose(out);
            break;
        }
        default: {
            char *name = GB_unique_filename("arb_print", "ps");
            dest       = GB_create_tempfile(name);
            free(name);

            if (!dest) error = GB_await_error();
            break;
        }
    }

    if (!error) {
        char *xfig;
        {
            char *name = GB_unique_filename("arb_print", "xfig");
            xfig       = GB_create_tempfile(name);
            free(name);
        }

        arb_progress progress("Printing");
        progress.subtitle("Exporting Data");

        if (!xfig) error = GB_await_error();
        if (!error) {
            double border = awr->awar(AWAR_CANIO_BORDERSIZE)->read_float();
            error         = canvas_to_xfig(scr, xfig, true, border);
        }

        if (!error) {
            awt_assert(GB_is_privatefile(xfig, true));

            // ----------------------------------------
            progress.subtitle("Converting to Postscript");

            {
                bool   landscape     = awr->awar(AWAR_CANIO_LANDSCAPE)->read_int();
                bool   useOverlap    = awr->awar(AWAR_CANIO_OVERLAP)->read_int();
                double magnification = awr->awar(AWAR_CANIO_MAGNIFICATION)->read_int() * 0.01;
                int    paper         = awr->awar(AWAR_CANIO_PAPER)->read_int();

                const PaperFormat& format = knownPaperFormat[paper];

                GBS_strstruct cmd(500);
                cmd.cat("fig2dev");

                cmd.cat(" -L ps"); // postscript output
                if (useOverlap) {
                    cmd.cat(" -M"); // generate multiple pages if needed
                    cmd.cat(" -O"); // generate overlap (if printing to multiple pages)
                }
                cmd.nprintf(20, " -m %f", magnification);
                cmd.cat(landscape ? " -l 0" : " -p 0");              // landscape or portrait
                cmd.nprintf(30, " -z %s", format.get_fig2dev_val()); // paperformat

                cmd.put(' '); cmd.cat(xfig); // input
                cmd.put(' '); cmd.cat(dest); // output

                error = GBK_system(cmd.get_data());
            }

            // if user saves to .ps -> no special file permissions are required
            awt_assert(printdest == PDEST_POSTSCRIPT || GB_is_privatefile(dest, false));

            if (!error) {
                progress.subtitle("Printing");

                switch (printdest) {
                    case PDEST_PREVIEW: {
                        GB_CSTR gs      = GB_getenvARB_GS();
                        GB_CSTR command = GBS_global_string("(%s %s;rm -f %s) &", gs, dest, dest);
                        error           = GBK_system(command);
                        break;
                    }
                    case PDEST_POSTSCRIPT:
                        break;

                    case PDEST_PRINTER: {
                        char *prt = awr->awar(AWAR_CANIO_PRINTER)->read_string();
                        error     = GBK_system(GBS_global_string("%s %s", prt, dest));
                        free(prt);
                        GB_unlink_or_warn(dest, &error);
                        break;
                    }
                }
            }
        }
        if (xfig) {
#if defined(DEBUG) && 0
            // show intermediate xfig and unlink it
            GBK_system(GBS_global_string("( xfig %s; rm %s) &", xfig, xfig));
#else // !defined(DEBUG)
            GB_unlink_or_warn(xfig, &error);
#endif
            free(xfig);
        }
    }

    free(dest);

    if (error) aw_message(error);
}

// --------------------------------------------------------------------------------

void AWT_popup_tree_export_window(AW_window *parent_win, AWT_canvas *scr) {
    static AW_window_simple *aws = 0;

    AW_root *awr = parent_win->get_root();
    create_export_awars(awr);
#if defined(ARB_GTK)
    resetFiletype(awr, "pdf", "print.pdf");
#else
    resetFiletype(awr, "fig", "print.fig");
#endif

    if (!aws) {
        aws = new AW_window_simple;
        aws->init(awr, "EXPORT_TREE_AS_XFIG", "EXPORT TREE TO XFIG");
        aws->load_xfig("awt/export.fig");

        aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help"); aws->callback(makeHelpCallback("tree2file.hlp"));
        aws->create_button("HELP", "HELP", "H");

        aws->label_length(15);

        AW_create_standard_fileselection(aws, AWAR_CANIO_FILE_BASE);

        aws->at("what");
        aws->label("Clip at Screen");
        aws->create_toggle_field(AWAR_CANIO_CLIP, 1);
        aws->insert_toggle("#print/clipscreen.xpm", "S", 0);
        aws->insert_toggle("#print/clipall.xpm", "A", 1);
        aws->update_toggle_field();

        aws->at("remove_root");
        aws->label("Show Handles");
        aws->create_toggle_field(AWAR_CANIO_HANDLES, 1);
        aws->insert_toggle("#print/nohandles.xpm", "S", 0);
        aws->insert_toggle("#print/handles.xpm", "A", 1);
        aws->update_toggle_field();

        aws->at("color");
        aws->label("Export colors");
        aws->create_toggle(AWAR_CANIO_COLOR);

        aws->at("xfig"); aws->callback(makeWindowCallback(canvas_to_xfig_and_run_xfig, scr));
        aws->create_autosize_button("START_XFIG", "EXPORT to XFIG", "X");
    }

    aws->activate();
}

void AWT_popup_sec_export_window(AW_window *parent_win, AWT_canvas *scr) {
    static AW_window_simple *aws = 0;

    AW_root *awr = parent_win->get_root();
    create_export_awars(awr);
    resetFiletype(awr, "fig", "print.fig");

    if (!aws) {
        aws = new AW_window_simple;
        aws->init(awr, "EXPORT_TREE_AS_XFIG", "EXPORT STRUCTURE TO XFIG");
        aws->load_xfig("awt/secExport.fig");

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("tree2file.hlp"));
        aws->create_button("HELP", "HELP", "H");

        aws->label_length(15);
        AW_create_standard_fileselection(aws, AWAR_CANIO_FILE_BASE);

        aws->at("what");
        aws->label("Clip Options");
        aws->create_option_menu(AWAR_CANIO_CLIP, true);
        aws->insert_option("Export screen only", "s", 0);
        aws->insert_default_option("Export complete structure", "c", 1);
        aws->update_option_menu();

        aws->at("color");
        aws->label("Export colors");
        aws->create_toggle(AWAR_CANIO_COLOR);

        aws->at("xfig");
        aws->callback(makeWindowCallback(canvas_to_xfig_and_run_xfig, scr));
        aws->create_autosize_button("START_XFIG", "EXPORT to XFIG", "X");
    }

    aws->activate();
}

static void columns_changed_cb(AW_window *aww) { set_mag_from_psize(aww->get_root(), true); }
static void rows_changed_cb(AW_window *aww) { set_mag_from_psize(aww->get_root(), false); }

static void fit_pages_cb(AW_window *aww, AW_CL cl_pages) {
    AW_root *awr = aww->get_root();
    int      wanted_pages;

    if (cl_pages>0)
        wanted_pages = cl_pages;
    else {
        wanted_pages = awr->awar(AWAR_CANIO_PAGES)->read_int();
    }
    fit_pages(awr, wanted_pages, true);
}

void AWT_popup_print_window(AW_window *parent_win, AWT_canvas *scr) {
    AW_root                 *awr = parent_win->get_root();
    static AW_window_simple *aws = 0;

    create_print_awars(awr, scr);
    resetFiletype(awr, "ps", "print.ps");

    if (!aws) {
        aws = new AW_window_simple;
        aws->init(awr, "PRINT_CANVAS", "PRINT GRAPHIC");
        aws->load_xfig("awt/print.fig");

        aws->at("close"); aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help"); aws->callback(makeHelpCallback("tree2prt.hlp"));
        aws->create_button("HELP", "HELP", "H");

        // -----------------------------
        //      select what to print

        aws->label_length(15);

        aws->at("what");
        aws->label("Clip at Screen");
        aws->create_toggle_field(AWAR_CANIO_CLIP, 1);
        aws->insert_toggle("#print/clipscreen.xpm", "S", 0);
        aws->insert_toggle("#print/clipall.xpm", "A", 1);
        aws->update_toggle_field();

        aws->at("remove_root");
        aws->label("Show Handles");
        aws->create_toggle_field(AWAR_CANIO_HANDLES, 1);
        aws->insert_toggle("#print/nohandles.xpm", "S", 0);
        aws->insert_toggle("#print/handles.xpm", "A", 1);
        aws->update_toggle_field();

        aws->at("color");
        aws->label("Export colors");
        aws->create_toggle(AWAR_CANIO_COLOR);

        // --------------------
        //      page layout

        aws->at("getsize");
        aws->callback((AW_CB1)awt_print_tree_check_size, (AW_CL)scr);
        aws->create_autosize_button(0, "Get Graphic Size");

        aws->button_length(6);
        aws->at("gsizex"); aws->create_button(0, AWAR_CANIO_GFX_SX);
        aws->at("gsizey"); aws->create_button(0, AWAR_CANIO_GFX_SY);
        aws->at("osizex"); aws->create_button(0, AWAR_CANIO_OUT_SX);
        aws->at("osizey"); aws->create_button(0, AWAR_CANIO_OUT_SY);

        aws->at("magnification");
        aws->create_input_field(AWAR_CANIO_MAGNIFICATION, 4);

        aws->label_length(0);
        aws->at("orientation");
        aws->create_toggle_field(AWAR_CANIO_LANDSCAPE, 1);
        aws->insert_toggle("#print/landscape.xpm", "L", 1);
        aws->insert_toggle("#print/portrait.xpm",  "P", 0);
        aws->update_toggle_field();

        aws->at("bsize"); aws->create_input_field(AWAR_CANIO_BORDERSIZE, 4);

        // -------------------
        //      paper size

        aws->at("psizex"); aws->create_input_field(AWAR_CANIO_PAPER_SX, 5);
        aws->at("psizey"); aws->create_input_field(AWAR_CANIO_PAPER_SY, 5);

        aws->at("paper");
        aws->create_option_menu(AWAR_CANIO_PAPER, true);
        aws->insert_default_option(knownPaperFormat[0].get_description(), "", 0);
        for (int f = 1; f<int(ARRAY_ELEMS(knownPaperFormat)); ++f) {
            const PaperFormat& format = knownPaperFormat[f];
            aws->insert_option(format.get_description(), "", f);
        }
        aws->update_option_menu();

        aws->at("usize"); aws->create_input_field(AWAR_CANIO_PAPER_USABLE, 4);


        // -----------------------
        //      multiple pages

        aws->at("sizex");
        aws->callback(columns_changed_cb);
        aws->create_input_field(AWAR_CANIO_PAGE_SX, 4);
        aws->at("sizey");
        aws->callback(rows_changed_cb);
        aws->create_input_field(AWAR_CANIO_PAGE_SY, 4);

        aws->at("best_fit");
        aws->callback(fit_pages_cb, 0);
        aws->create_autosize_button("fit_on", "Fit on");

        aws->at("pages");
        aws->create_input_field(AWAR_CANIO_PAGES, 3);

        aws->at("plock");
        aws->create_toggle(AWAR_CANIO_PAGELOCK);
        
        aws->button_length(1);
        {
            char name[] = "p0";
            for (int p = 1; p <= 9; ++p) {
                name[1] = '0'+p; // at-label, macro-name and button-text
                if (aws->at_ifdef(name)) {
                    aws->at(name);
                    aws->callback(fit_pages_cb, p);
                    aws->create_button(name, name+1);
                }
            }
        }

        aws->at("overlap");
        aws->create_toggle(AWAR_CANIO_OVERLAP);
        aws->at("amount");
        aws->create_input_field(AWAR_CANIO_OVERLAP_PERCENT, 4);

        aws->at("f2dbug");
        aws->create_toggle(AWAR_CANIO_F2DBUG);

        // --------------------
        //      destination

        aws->at("printto");
        aws->label_length(12);
        aws->label("Destination");
        aws->create_toggle_field(AWAR_CANIO_DEST);
        aws->insert_toggle("Printer",           "P", PDEST_PRINTER);
        aws->insert_toggle("File (Postscript)", "F", PDEST_POSTSCRIPT);
        aws->insert_toggle("Preview",           "V", PDEST_PREVIEW);
        aws->update_toggle_field();

        aws->at("printer");
        aws->create_input_field(AWAR_CANIO_PRINTER, 16);

        aws->at("filename");
        aws->create_input_field(AWAR_CANIO_FILE_NAME, 16);

        aws->at("go");
        aws->highlight();
        aws->callback(canvas_to_printer, (AW_CL)scr);
        aws->create_autosize_button("PRINT", "PRINT", "P");
    }

    awt_print_tree_check_size(0, (AW_CL)scr);
    aws->activate();
}
