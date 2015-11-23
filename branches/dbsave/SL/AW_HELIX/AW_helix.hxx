// ==================================================================== //
//                                                                      //
//   File      : AW_helix.hxx                                           //
//   Purpose   : Wrapper for BI_helix + AW-specific functions           //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in December 2004         //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //
#ifndef AW_HELIX_HXX
#define AW_HELIX_HXX

#ifndef BI_HELIX_HXX
#include <BI_helix.hxx>
#endif
#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef CB_H
#include <cb.h>
#endif

class AW_cb;

class AW_helix : public BI_helix {
    long enabled; // draw or not

public:
    AW_helix(AW_root *awroot);

    char *seq_2_helix(char *sequence, char undefsymbol = ' ');
    char get_symbol(char left, char right, BI_PAIR_TYPE pair_type);
    int show_helix(void *device, int gc1, const char *sequence, AW_pos x, AW_pos y, AW_bitset filter);
    bool is_enabled() const { return (enabled != 0) && (size()>0); }
};

AW_window *create_helix_props_window(AW_root *awr, const WindowCallback *refreshCallback);

#else
#error AW_helix.hxx included twice
#endif // AW_HELIX_HXX

