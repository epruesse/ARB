// =========================================================== //
//                                                             //
//   File      : aw_common_xm.hxx                              //
//   Purpose   :                                               //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2011   //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AW_COMMON_XM_HXX
#define AW_COMMON_XM_HXX

#ifndef AW_COMMON_HXX
#include "aw_common.hxx"
#endif
#ifndef X_H
#include <X11/X.h>
#endif
#ifndef _XLIB_H_
#include <X11/Xlib.h>
#endif


class AW_common_Xm;

// @@@ misleading name (not motif dependent)
class AW_GC_Xm : public AW_GC { // derived from Noncopyable
    GC          gc;
    XFontStruct curfont;

    virtual void wm_set_foreground_color(AW_rgb col);
    virtual void wm_set_function(AW_function mode);
    virtual void wm_set_lineattributes(short lwidth, AW_linestyle lstyle);
    virtual void wm_set_font(AW_font font_nr, int size, int *found_size);

public:

    AW_GC_Xm(AW_common *common);
    ~AW_GC_Xm();

    // AW_GC interface (uses motif call)
    virtual int get_available_fontsizes(AW_font font_nr, int *available_sizes) const;

    inline AW_common_Xm *get_common() const;
    
    GC get_gc() const { return gc; }
    const XFontStruct *get_xfont() const { return &curfont; }
};

// @@@ misleading name (not motif dependent)
class AW_common_Xm: public AW_common { // derived from Noncopyable
    Display *display;
    XID      window_id;

    void install_common_extends_cb(AW_window *aww, AW_area area); 

public:
    AW_common_Xm(Display   *display_in,
                 XID        window_id_in,
                 AW_rgb*&   fcolors,
                 AW_rgb*&   dcolors,
                 long&      dcolors_count,
                 AW_window *aww,
                 AW_area    area)
        : AW_common(fcolors, dcolors, dcolors_count),
          display(display_in),
          window_id(window_id_in)
    {
        install_common_extends_cb(aww, area);
    }

    virtual AW_GC *create_gc();

    const AW_GC_Xm *map_gc(int gc) const { return DOWNCAST(const AW_GC_Xm*, AW_common::map_gc(gc)); }
    AW_GC_Xm *map_mod_gc(int gc) { return DOWNCAST(AW_GC_Xm*, AW_common::map_mod_gc(gc)); }

    Display *get_display() const { return display; }
    XID get_window_id() const { return window_id; }

    GC get_GC(int gc) const { return map_gc(gc)->get_gc(); }
    const XFontStruct *get_xfont(int gc) const { return map_gc(gc)->get_xfont(); }
};

inline AW_common_Xm *AW_GC_Xm::get_common() const {
    return DOWNCAST(AW_common_Xm*, AW_GC::get_common());
}

#else
#error aw_common_xm.hxx included twice
#endif // AW_COMMON_XM_HXX
