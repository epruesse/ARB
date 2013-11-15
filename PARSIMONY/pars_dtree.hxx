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

class AWT_graphic_parsimony : public AWT_graphic_tree, virtual Noncopyable {
    ArbParsimony& parsimony;

    AW_gc_manager init_devices(AW_window *, AW_device *, AWT_canvas *ntw) OVERRIDE;

    void show(AW_device *device) OVERRIDE;

    void handle_command(AW_device *device, AWT_graphic_event& event) OVERRIDE;

public:
    AWT_graphic_parsimony(ArbParsimony& parsimony_, GBDATA *gb_main_, AD_map_viewer_cb map_viewer_cb_);
};

void PARS_tree_init(AWT_graphic_tree *agt);

#else
#error pars_dtree.hxx included twice
#endif // PARS_DTREE_HXX
