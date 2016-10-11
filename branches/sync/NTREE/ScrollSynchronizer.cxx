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

using namespace AW;

inline GBDATA *trackable_species(const AW_click_cd *clickable) {
    if (!clickable) return NULL;

    ClickedType clicked = (ClickedType)clickable->get_cd2();
    if (clicked == CL_SPECIES) return (GBDATA*)clickable->get_cd1(); // NDS list

    nt_assert(clicked == CL_NODE || clicked == CL_BRANCH); // unexpected clickable tracked!
    TreeNode *node = (TreeNode*)clickable->get_cd1();
    return (node && node->is_leaf) ? node->gb_node :  NULL;
}

class AW_trackSpecies_device: public AW_simple_device {
    SpeciesSet& species;

    void track() {
        GBDATA *gb_species = trackable_species(get_click_cd());

        if (gb_species) {
#if defined(DEBUG)
            bool do_insert = species.find(gb_species) == species.end();
#else // NDEBUG
            bool do_insert = true;
#endif
            if (do_insert) {
#if defined(DUMP_SYNC) && 1
                const char *name = GBT_get_name(gb_species);
                fprintf(stderr, " - adding species #%zu '%s'\n", species.size(), name);
#endif
                species.insert(gb_species);
            }
        }
    }

public:
    AW_trackSpecies_device(AW_common *common_, SpeciesSet& species_) :
        AW_simple_device(common_),
        species(species_)
    {}

    AW_DEVICE_TYPE type() OVERRIDE { return AW_DEVICE_CLICK; }
    void specific_reset() OVERRIDE {}
    bool invisible_impl(const Position&, AW_bitset) OVERRIDE { return false; }

    bool line_impl(int, const LineVector& Line, AW_bitset filteri) OVERRIDE {
        bool drawflag = false;
        if (filteri & filter) {
            LineVector transLine = transform(Line);
            LineVector clippedLine;
            drawflag = clip(transLine, clippedLine);
            if (drawflag) track();
        }
        return drawflag;
    }
    bool text_impl(int, const char*, const Position& pos, AW_pos, AW_bitset filteri, long int) OVERRIDE {
        bool drawflag = false;
        if (filteri & filter) {
            if (!is_outside_clip(transform(pos))) track();
        }
        return drawflag;
    }
};



SpeciesSetPtr MasterCanvas::track_displayed_species() {
    // clip_expose

    TREE_canvas *ntw    = get_canvas();
    AW_window   *aww    = ntw->aww;
    AW_common   *common = aww->get_common(AW_MIDDLE_AREA);

    SpeciesSetPtr tracked = new SpeciesSet;

    AW_trackSpecies_device device(common, *tracked);

    device.set_filter(AW_TRACK);
    device.reset();

    const AW_screen_area& rect = ntw->rect;

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

void SlaveCanvas::refresh() {
#if defined(DUMP_SYNC)
    fprintf(stderr, "DEBUG: SlaveCanvas does refresh (idx=%i)\n", get_index());
#endif
    get_canvas()->refresh();
}
