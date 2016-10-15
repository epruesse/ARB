// =============================================================== //
//                                                                 //
//   File      : pars_dtree.hxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PARS_DTREE_HXX
#define PARS_DTREE_HXX

#ifndef TREEDISPLAY_HXX
#include <TreeDisplay.hxx>
#endif

class arb_progress;
class ArbParsimony;
class AP_tree_nlen;
class AP_pars_root;

class AWT_graphic_parsimony : public AWT_graphic_tree, virtual Noncopyable {
    ArbParsimony& parsimony;

    AW_gc_manager *init_devices(AW_window *, AW_device *, AWT_canvas *ntw) OVERRIDE;

    void show(AW_device *device) OVERRIDE;

    void handle_command(AW_device *device, AWT_graphic_event& event) OVERRIDE;
    AP_tree_root *create_tree_root(AliView *aliview, AP_sequence *seq_prototype, bool insert_delete_cbs) OVERRIDE;

public:
    AWT_graphic_parsimony(ArbParsimony& parsimony_, GBDATA *gb_main_, AD_map_viewer_cb map_viewer_cb_);

    AP_tree_nlen *get_root_node() {
        return DOWNCAST(AP_tree_nlen*, AWT_graphic_tree::get_root_node());
    }
    AP_pars_root *get_tree_root() { return DOWNCAST(AP_pars_root*, AWT_graphic_tree::get_tree_root()); }
    ArbParsimony& get_parsimony() { return parsimony; }
};

void PARS_tree_init(AWT_graphic_parsimony *agt);

#else
#error pars_dtree.hxx included twice
#endif // PARS_DTREE_HXX
