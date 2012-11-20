// ============================================================= //
//                                                               //
//   File      : AW_common_gtk.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 9, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //


#include "aw_common_gtk.hxx"
#include "aw_gtk_migration_helpers.hxx"
#include "aw_window.hxx"
#include <gdk/gdkgc.h>
#include <gdk/gdkpixmap.h>
#include <gtk/gtkwidget.h>
#include "aw_xfont.hxx"
//TODO remove if xfont is gone
#include <gdk/gdkx.h>
#include <string>
#include <algorithm>




/*
 * Is called upon window resize
 * @param cl_common_gtk Pointer to the common_gtk instance that registered this callback.
 */
static void AW_window_resize_cb(AW_window *, AW_CL cl_common_gtk, AW_CL) {
   AW_common_gtk *common = (AW_common_gtk*)cl_common_gtk;
//    Window        root;
//    unsigned int  width, height;
//    unsigned int  depth, borderwidth; // unused
//    int           x_offset, y_offset; // unused
//
//    XGetGeometry(common->get_display(), common->get_window_id(),
//                 &root,
//                 &x_offset,
//                 &y_offset,
//                 &width,
//                 &height,
//                 &borderwidth,  // border width
//                 &depth);       // depth of display
//
//    common->set_screen_size(width, height);


    gint width = common->get_window()->allocation.width;
    gint height = common->get_window()->allocation.height;
    common->set_screen_size(width, height);

}

AW_common_gtk::AW_common_gtk(GdkDisplay *display_in,
             GtkWidget    *window_in,
             AW_rgb*&   fcolors,
             AW_rgb*&   dcolors,
             long&      dcolors_count,
             AW_window *window,
             AW_area    area)
    : AW_common(fcolors, dcolors, dcolors_count),
      display(display_in),
      window(window_in)
{

    window->set_resize_callback(area, AW_window_resize_cb, (AW_CL)this);
    AW_window_resize_cb(window, (AW_CL)this, 0);//call the resize cb once in the beginning to get the size
}

AW_GC *AW_common_gtk::create_gc() {
    return new AW_GC_gtk(this);
}

void AW_GC::set_font(const AW_font font_nr, const int size, int *found_size) {
    font_limits.reset();
    wm_set_font(font_nr, size, found_size);
    font_limits.calc_height();
    fontnr   = font_nr;
    fontsize = size;
}

//FIXME initialize gc
AW_GC_gtk::AW_GC_gtk(AW_common *common) : AW_GC(common) {

    FIXME("Use real drawable to create gc");
    //It is not possible to create a gc without a drawable.
    //The gc can only be used to draw on drawables which use the same colormap and depth
    GdkPixmap *temp = gdk_pixmap_new(0, 3000, 3000, 24); // memory leak
    gc = gdk_gc_new(temp);

}
AW_GC_gtk::~AW_GC_gtk(){};

void AW_GC_gtk::wm_set_foreground_color(AW_rgb col){

    GdkColor color;
    color.pixel = col;
    gdk_gc_set_foreground(gc, &color);
}

void AW_GC_gtk::wm_set_function(AW_function mode){
    switch (mode) {
        case AW_XOR:
            gdk_gc_set_function(gc, GDK_XOR);
            //XSetFunction(get_common()->get_display(), gc, GXxor);
            break;
        case AW_COPY:
            gdk_gc_set_function(gc, GDK_COPY);
            //XSetFunction(get_common()->get_display(), gc, GXcopy);
            break;
    }
}

void AW_GC_gtk::wm_set_lineattributes(short lwidth, AW_linestyle lstyle){
    GdkLineStyle lineStyle;

    switch(lstyle) {
        case AW_SOLID:
            lineStyle = GDK_LINE_SOLID;
            break;
        case AW_DASHED:
        case AW_DOTTED:
            FIXME("DOTTED lines are converted to DASHED lines");
            lineStyle = GDK_LINE_ON_OFF_DASH;
            break;
        default:
            lineStyle = GDK_LINE_SOLID;
            break;
    }

    gdk_gc_set_line_attributes(gc, lwidth, lineStyle, GDK_CAP_BUTT, GDK_JOIN_BEVEL);
}

void AW_GC_gtk::wm_set_font(const AW_font font_nr, const int size, int *found_size) {
    // if found_size != 0 -> return value for used font size

    XFontStruct *xfs;
    {
        int  found_font_size;
        arb_assert(lookfont(GDK_DISPLAY_XDISPLAY(get_common()->get_display()), font_nr, size, found_font_size, true, false, &xfs)); // lookfont should do fallback
        if (found_size) *found_size = found_font_size;
    }


    FIXME("Font conversion hack");
    //extract XLFD name from XFontStruct

    XFontProp *xfp;
    char *name;
    int i;
    for (i = 0, xfp = xfs->properties; i < xfs->n_properties; i++, xfp++) {
        if (xfp->name == XA_FONT) {
            /*
             * Set the font name but don't add it to the list in the font.
             */
            name = XGetAtomName(GDK_DISPLAY_XDISPLAY(get_common()->get_display()), (Atom) xfp->card32);
            break;
        }
    }

    FIXME("Gtk seems unable to load unicode fonts");
    //workaround
    //for some reason gtk cannot load fonts that contain ISO10646. ISO10646 is unicode.
    //However it works fine if the "ISO10646" is replaced with a wildcard.
    //This is most likely due to the very old gtk version on this system :)
    std::string fontname(name);
    std::string iso = "ISO10646";
    size_t start = fontname.find(iso);
    fontname.replace(start, iso.length(), "*");


    //load a gdk font corresponding to the XLDF name
    GdkFont *font = gdk_font_load(fontname.c_str());
    ASSERT_FALSE(font == NULL);
    gdk_gc_set_font(gc, font);

    //set char size for each ascii char
    for (unsigned int j = AW_FONTINFO_CHAR_ASCII_MIN; j <= AW_FONTINFO_CHAR_ASCII_MAX; j++) {
        gchar c = (gchar)j;
        gint lbearing = 0;
        gint rbearing = 0;
        gint width = 0;
        gint ascent = 0;
        gint descent = 0;
        gdk_text_extents(font, &c, 1, &lbearing, &rbearing, &width, &ascent, &descent);

        set_char_size(j, ascent, descent, width);
        //else    set_no_char_size(i);
    }

}

int AW_GC_gtk::get_available_fontsizes(AW_font font_nr, int *available_sizes) const {
    GTK_NOT_IMPLEMENTED;
    return 0;
}
