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
#ifndef AD_COLORSET_H
#include <ad_colorset.h>
#endif
#ifndef ITEM_SHADER_H
#include <item_shader.h>
#endif

#define ap_assert(cond) arb_assert(cond)

class GBDATA;

class AP_TreeShader {
protected:
    bool colorize_marked;
    bool colorize_groups;
    bool shade_species;
public:
    AP_TreeShader() :
        colorize_marked(false),
        colorize_groups(false),
        shade_species(false)
    {}
    virtual ~AP_TreeShader() {}
    virtual void init() = 0; // called by AP_tree::set_tree_shader when installed

    bool does_shade() const { return shade_species; }

    virtual void update_settings() = 0;
    virtual ShadedValue calc_shaded_leaf_GC(GBDATA *gb_node) const = 0;
    virtual ShadedValue calc_shaded_inner_GC(const ShadedValue& left, float left_ratio, const ShadedValue& right) const = 0;
    virtual int to_GC(const ShadedValue& val) const = 0;

    int calc_leaf_GC(GBDATA *gb_node, bool is_marked) const {
        int gc = AWT_GC_NONE_MARKED;
        if (gb_node) {
            if (is_marked && colorize_marked) {
                gc = AWT_GC_ALL_MARKED;
            }
            else if (colorize_groups) { // check for user color
                long color_group = GBT_get_color_group(gb_node);
                if (color_group) gc = AWT_GC_FIRST_COLOR_GROUP+color_group-1;
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
