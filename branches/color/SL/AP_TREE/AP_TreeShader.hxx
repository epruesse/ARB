// ============================================================ //
//                                                              //
//   File      : AP_TreeShader.hxx                              //
//   Purpose   : separate GC calculation from AP_TREE library   //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2016   //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#ifndef AP_TREESHADER_HXX
#define AP_TREESHADER_HXX

#ifndef AP_TREECOLORS_HXX
#include <AP_TreeColors.hxx>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#define ap_assert(cond) arb_assert(cond)

class GBDATA;

class AP_TreeShader {

public:
    AP_TreeShader() {}
    virtual ~AP_TreeShader() {}

    int calc_leaf_GC(GBDATA *gb_node, bool is_marked) const {
        int gc;
        if (gb_node) {
            if (is_marked) {
                gc = AWT_GC_ALL_MARKED;
            }
            else {
                // check for user color
                long color_group = AW_find_active_color_group(gb_node);
                if (color_group == 0) {
                    gc = AWT_GC_NONE_MARKED;
                }
                else {
                    gc = AWT_GC_FIRST_COLOR_GROUP+color_group-1;
                }
            }
        }
        else {
            gc = AWT_GC_ONLY_ZOMBIES;
        }
        return gc;
    }

    int calc_inner_GC(int left_gc, int right_gc) const {
        int gc;

        if (left_gc == right_gc) gc = left_gc;

        else if (left_gc == AWT_GC_ALL_MARKED || right_gc == AWT_GC_ALL_MARKED) gc = AWT_GC_SOME_MARKED;

        else if (left_gc  == AWT_GC_ONLY_ZOMBIES) gc = right_gc;
        else if (right_gc == AWT_GC_ONLY_ZOMBIES) gc = left_gc;

        else if (left_gc == AWT_GC_SOME_MARKED || right_gc == AWT_GC_SOME_MARKED) gc = AWT_GC_SOME_MARKED;

        else {
            ap_assert(left_gc != AWT_GC_ALL_MARKED  && right_gc != AWT_GC_ALL_MARKED);
            ap_assert(left_gc != AWT_GC_SOME_MARKED && right_gc != AWT_GC_SOME_MARKED);
            gc = AWT_GC_NONE_MARKED;
        }

        return gc;
    }
};


#else
#error AP_TreeShader.hxx included twice
#endif // AP_TREESHADER_HXX
