// ==================================================================== //
//                                                                      //
//   File      : aw_font_group.hxx                                      //
//   Purpose   : Bundles a group of fonts and provides overall maximas  //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in December 2004         //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#ifndef AW_FONT_GROUP_HXX
#define AW_FONT_GROUP_HXX

#ifndef AW_DEVICE_HXX
#include <aw_device.hxx>
#endif

#define AW_FONT_GROUP_MAX_GC 10

class AW_font_group {
    AW_font_limits gc_limits[AW_FONT_GROUP_MAX_GC+1];
    AW_font_limits any_limits; // maxima of all registered fonts

#if defined(ASSERTION_USED)
    bool font_registered(int gc) const { return gc_limits[gc].was_notified(); }
    bool any_font_registered()   const { return any_limits.was_notified(); }
#endif

public:
    void unregisterAll() { *this = AW_font_group(); }
    void registerFont(AW_device *device_, int gc, const char *chars = 0); // if no 'chars' specified => use complete ASCII-range

    const AW_font_limits& get_limits(int gc) const {
        aw_assert(font_registered(gc));
        return gc_limits[gc];
    }
    const AW_font_limits& get_max_limits() const {
        aw_assert(any_font_registered());
        return any_limits;
    }

    int get_width  (int gc) const { return get_limits(gc).width; }
    int get_ascent (int gc) const { return get_limits(gc).ascent; }
    int get_descent(int gc) const { return get_limits(gc).descent; }
    int get_height (int gc) const { return get_limits(gc).get_height(); }

    // maxima of all registered fonts:
    int get_max_width  () const { return get_max_limits().width; }
    int get_max_ascent () const { return get_max_limits().ascent; }
    int get_max_descent() const { return get_max_limits().descent; }
    int get_max_height () const { return get_max_limits().get_height(); }
};


#else
#error aw_font_group.hxx included twice
#endif // AW_FONT_GROUP_HXX

