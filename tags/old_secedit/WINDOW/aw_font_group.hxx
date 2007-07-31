// ==================================================================== //
//                                                                      //
//   File      : aw_font_group.hxx                                      //
//   Purpose   : Bundles a group of fonts and provides overall maximas  //
//   Time-stamp: <Wed Feb/02/2005 22:27 MET Coder@ReallySoft.de>        //
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
    const AW_font_information *font_info[AW_FONT_GROUP_MAX_GC+1];
    
    int max_width; // maximas of all registered fonts
    int max_ascent;
    int max_descent;
    int max_height;

public:
    AW_font_group();

    void unregisterAll();
    void registerFont(AW_device *device_, int gc);

    int get_width  (int gc) const { return font_info[gc]->max_letter.width; }
    int get_ascent (int gc) const { return font_info[gc]->max_letter.ascent; }
    int get_descent(int gc) const { return font_info[gc]->max_letter.descent; }
    int get_height (int gc) const { return font_info[gc]->max_letter.height; }

    int get_max_width  () const { return max_width; }
    int get_max_ascent () const { return max_ascent; }
    int get_max_descent() const { return max_descent; }
    int get_max_height () const { return max_height; }
};


#else
#error aw_font_group.hxx included twice
#endif // AW_FONT_GROUP_HXX

