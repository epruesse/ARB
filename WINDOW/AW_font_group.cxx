// ==================================================================== //
//                                                                      //
//   File      : AW_font_group.cxx                                      //
//   Purpose   : Bundles a group of fonts and provides overall maximas  //
//   Time-stamp: <Wed Jan/05/2005 11:02 MET Coder@ReallySoft.de>        //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in December 2004         //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#include "aw_font_group.hxx"


// ____________________________________________________________
// start of implementation of class AW_font_group:

AW_font_group::AW_font_group() {
    unregisterAll();
}

void AW_font_group::unregisterAll()
{
    max_width   = 0;
    max_ascent  = 0;
    max_descent = 0;

    for (int i = 0; i<(AW_FONT_GROUP_MAX_GC+1); ++i) font_info[i] = 0;
}

inline void set_max(int val, int& max) { if (val>max) max = val; }

void AW_font_group::registerFont(AW_device *device, int gc)
{
    aw_assert(gc <= AW_FONT_GROUP_MAX_GC);
    font_info[gc] = device->get_font_information(gc, 0);

    set_max(get_width(gc), max_width);
    set_max(get_ascent(gc), max_ascent);
    set_max(get_descent(gc), max_descent);
}


// -end- of implementation of class AW_font_group.




