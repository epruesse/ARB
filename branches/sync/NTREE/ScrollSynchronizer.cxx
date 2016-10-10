// =============================================================== //
//                                                                 //
//   File      : ScrollSynchronizer.cxx                            //
//   Purpose   : synchronize TREE_canvas scrolling                 //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2016   //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ScrollSynchronizer.h"

#include <TreeDisplay.hxx>
#include <awt_canvas.hxx>

class AW_trackSpecies_device: public AW_simple_device {
    SpeciesSet& species;

    bool track(AW_bitset filteri) {
        bool drawflag = false;
        if (filteri & filter) {
            const AW_click_cd *clickable = get_click_cd();
            if (clickable) {
                ClickedType  clicked    = (ClickedType)clickable->get_cd2();
                GBDATA      *gb_species = NULL;

                if (clicked == CL_SPECIES) {
                    gb_species = (GBDATA*)clickable->get_cd1();
                }
                else if (clicked == CL_NODE) {
                    TreeNode *node = (TreeNode*)clickable->get_cd1();
                    if (node->is_leaf) {
                        gb_species = node->gb_node;
                    }
                }

                if (gb_species) {
#if defined(DEBUG) && 0
                    const char *name = GBT_get_name(gb_species);
                    fprintf(stderr, " - adding species '%s'\n", name);
#endif
                    species.insert(gb_species);
                }
            }
        }
        return drawflag;
    }

public:
    AW_trackSpecies_device(AW_common *common_, SpeciesSet& species_) :
        AW_simple_device(common_),
        species(species_)
    {}

    AW_DEVICE_TYPE type() OVERRIDE { return AW_DEVICE_CLICK; }
    void specific_reset() OVERRIDE {}

    bool line_impl(int, const AW::LineVector&, AW_bitset filteri) OVERRIDE { return track(filteri); }
    bool text_impl(int, const char*, const AW::Position&, AW_pos, AW_bitset filteri, long int) OVERRIDE { return track(filteri); }
    bool invisible_impl(const AW::Position&, AW_bitset) OVERRIDE { return false; }
};



SpeciesSetPtr MasterCanvas::track_displayed_species() {
    // clip_expose

    TREE_canvas *ntw    = get_canvas();
    AW_window   *aww    = ntw->aww;
    AW_common   *common = aww->get_common(AW_MIDDLE_AREA);

    SpeciesSetPtr tracked = new SpeciesSet;

    AW_trackSpecies_device device(common, *tracked);

    device.set_filter(AW_CLICK);
    device.reset();

    const AW_screen_area& rect = ntw->rect; // @@@ has wrong size (tracks a few species below bottom of screen)
                                            // maybe top-area-size isn't subtracted from area??

    device.set_top_clip_border(rect.t);
    device.set_bottom_clip_border(rect.b);
    device.set_left_clip_border(rect.l);
    device.set_right_clip_border(rect.r);

    {
        GB_transaction ta(ntw->gb_main);

        ntw->init_device(&device);
        ntw->gfx->show(&device);
    }

    return tracked;
}
