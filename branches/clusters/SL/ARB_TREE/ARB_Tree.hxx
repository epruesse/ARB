// =============================================================== //
//                                                                 //
//   File      : ARB_Tree.hxx                                      //
//   Purpose   : Minimal interface between GBT_TREE and C++        //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2009   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef ARB_TREE_HXX
#define ARB_TREE_HXX

#ifndef ARBDBT_H
#include <arbdbt.h>
#endif
#ifndef AP_FILTER_HXX
#include <AP_filter.hxx>
#endif


#define at_assert(cond) arb_assert(cond)

class ARB_Tree_root {
    AP_filter  *filter;
    AP_weights *weights;
    
protected:
    GBDATA *gb_main;

public:
    ARB_Tree_root(GBDATA *gb_main_)
        : filter(0)
        , weights(0)
        , gb_main(gb_main_)
    {
    }

    virtual ~ARB_Tree_root() {
        delete weights;
        delete filter;
    }

    void set_filter(AP_filter *filter_) { delete filter; filter = filter_; }
    void set_weights(AP_weights *weights_) { delete weights; weights = weights_; }

    const AP_filter *get_filter() const { return filter; }
    const AP_weights *get_weights() const { return weights; }

    GBDATA *get_gb_main() const { return gb_main; }
};



class ARB_Tree {
    GBT_TREE_ELEMENTS(ARB_Tree);

    void unlink_son(ARB_Tree *son) {
        if (son == leftson) {
            leftson = NULL;
        }
        else {
            at_assert(son == rightson);
            rightson = NULL;
        }
    }

    void unlink_from_father() {
        if (father) {
            father->unlink_son(this);
            father = NULL;
        }
    }

    void move_gbt_info(GBT_TREE *tree);
public:
    ARB_Tree();
    virtual ~ARB_Tree();                            // needed (see FAKE_VTAB_PTR)

    virtual ARB_Tree *dup() const = 0; // create new ARB_Tree element from prototype

    GBT_TREE *get_gbt_tree() { return (GBT_TREE*)this; }

    GB_ERROR load(GBDATA *gb_main, const char *tree_name, bool link_to_database, bool show_status);
    GB_ERROR save(GBDATA *gb_main, const char *tree_name);
};



#else
#error ARB_Tree.hxx included twice
#endif // ARB_TREE_HXX
