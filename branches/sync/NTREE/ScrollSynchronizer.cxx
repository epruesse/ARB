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
#include <map>

#if defined(DUMP_SYNC)
# define DUMP_ADD
# define DUMP_SCROLL_DETECT
#endif


using namespace AW;
using namespace std;

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
#if defined(DUMP_ADD)
            bool do_insert = species.find(gb_species) == species.end();
#else // NDEBUG
            bool do_insert = true;
#endif
            if (do_insert) {
#if defined(DUMP_ADD)
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

typedef map< RefPtr<GBDATA>, Rectangle> SpeciesPositions; // world-coordinates

class AW_trackPositions_device: public AW_simple_device {
    SpeciesSetPtr    species; // @@@ elim (instead "mark" contained species to improve speed?)
    SpeciesPositions spos;

    void trackPosition(GBDATA *gb_species, const Rectangle& spec_area) {
        SpeciesPositions::iterator tracked = spos.find(gb_species);
        if (tracked == spos.end()) { // first track
            spos[gb_species] = spec_area;
        }
        else {
            tracked->second = bounding_box(tracked->second, spec_area); // combine areas
        }
    }
    void trackPosition(GBDATA *gb_species, const LineVector& spec_vec) {
        trackPosition(gb_species, Rectangle(spec_vec));
    }
    void trackPosition(GBDATA *gb_species, const Position& spec_pos) {
        trackPosition(gb_species, Rectangle(spec_pos, ZeroVector));
    }

public:
    AW_trackPositions_device(AW_common *common_) :
        AW_simple_device(common_)
    {
    }

    void set_species(SpeciesSetPtr species_) { species = species_; }
    void forget_positions() { spos.clear(); }

    AW_DEVICE_TYPE type() OVERRIDE { return AW_DEVICE_SIZE; }
    void specific_reset() OVERRIDE {}
    bool invisible_impl(const Position&, AW_bitset) OVERRIDE { return false; }

    bool line_impl(int, const LineVector& Line, AW_bitset filteri) OVERRIDE {
        bool drawflag = false;
        if (filteri & filter) {
            GBDATA *gb_species = trackable_species(get_click_cd());
            if (gb_species) {
                if (species->find(gb_species) != species->end()) {
                    trackPosition(gb_species, Line);
                }
            }
            drawflag = true;
        }
        return drawflag;
    }
    bool text_impl(int, const char*, const Position& pos, AW_pos, AW_bitset filteri, long int) OVERRIDE {
        bool drawflag = false;
        if (filteri & filter) {
            GBDATA *gb_species = trackable_species(get_click_cd());
            if (gb_species) {
                if (species->find(gb_species) != species->end()) {
                    trackPosition(gb_species, pos);
                }
            }
            drawflag = true;
        }
        return drawflag;
    }

    const SpeciesPositions& get_tracked_positions() const {
        return spos;
    }
};

struct cmp_Rectangles {
    bool operator()(const Rectangle &r1, const Rectangle &r2) const {
    double cmp = r1.top()-r2.top(); // upper first
    if (!cmp) {
        cmp = r1.bottom()-r2.bottom(); // smaller first
        if (!cmp) {
            cmp = r1.left()-r2.left(); // leftmost first
            if (!cmp) {
                cmp = r1.right()-r2.right(); // smaller first
            }
        }
    }
    return cmp<0.0;
  }
};

typedef set<Rectangle, cmp_Rectangles> SortedPositions; // world-coordinates

struct SlaveCanvas::SlaveCanvas_internal {
    SortedPositions sorted_pos;
};

static void sortPositions(const SpeciesPositions& spos, SortedPositions& sorted) {
    sorted.clear();
    for (SpeciesPositions::const_iterator s = spos.begin(); s != spos.end(); ++s) {
        sorted.insert(s->second);
    }
}

static Vector calc_best_scroll_delta(const SortedPositions& pos, const Rectangle& viewport) {
    int       best_count = -1;
    Rectangle best_viewport;

#if defined(DUMP_SCROLL_DETECT)
    AW_DUMP(viewport);
#endif

    const int max_species = int(pos.size());
    int       rest        = max_species;

    SortedPositions::const_iterator end = pos.end();
    for (SortedPositions::const_iterator s1 = pos.begin(); rest>best_count && s1 != end; ++s1) {
        const Rectangle& r1    = *s1;
        Rectangle        testedViewport(r1.upper_left_corner(), viewport.diagonal());
        int              count = 1;

        Rectangle contained_area = r1; // bounding box of all species displayable inside testedViewport

        SortedPositions::const_iterator s2 = s1;
        ++s2;
        for (; s2 != end; ++s2) {
            const Rectangle& r2 = *s2;
            if (r2.overlaps_with(testedViewport)) {
                ++count;
                contained_area = contained_area.bounding_box(r2);
            }
        }

        nt_assert(count>0);

        if (count>best_count) {
            best_count         = count;
            const Vector& diag = testedViewport.diagonal();
            // best_viewport   = Rectangle(contained_area.centroid()-diag/2, diag); // centered (ugly)
            Vector shift(testedViewport.width()*-0.1, (contained_area.height()-testedViewport.height())/2);
            best_viewport = Rectangle(testedViewport.upper_left_corner() + shift, diag);

#if defined(DUMP_SCROLL_DETECT)
            fprintf(stderr, "Found %i species fitting into area\n", count);
            AW_DUMP(testedViewport);
            AW_DUMP(contained_area);
            AW_DUMP(best_viewport);
#endif
        }

        rest--;
    }

    if (best_count<=0) return ZeroVector;

    return best_viewport.upper_left_corner() - viewport.upper_left_corner();
}

void SlaveCanvas::track_display_positions() {
    TREE_canvas *ntw    = get_canvas();
    AW_window   *aww    = ntw->aww;
    AW_common   *common = aww->get_common(AW_MIDDLE_AREA);

    // @@@ use different algo (device) for radial and for other treeviews

    AW_trackPositions_device device(common);

    device.set_species(species); // @@@ move to device-ctor?
    device.forget_positions(); // @@@ not necessary if device is recreated for each tracking

    device.set_filter(AW_TRACK);
    device.reset(); // @@@ really needed?

    {
        GB_transaction ta(ntw->gb_main);

        ntw->init_device(&device);
        ntw->gfx->show(&device);
    }

    const SpeciesPositions& spos = device.get_tracked_positions();
    sortPositions(spos, internal->sorted_pos);
}

void SlaveCanvas::calc_scroll_zoom() {
    TREE_canvas *ntw    = get_canvas();
    AW_window   *aww    = ntw->aww;
    AW_device   *device = aww->get_device(AW_MIDDLE_AREA);

    Rectangle viewport = device->rtransform(Rectangle(ntw->rect, INCLUSIVE_OUTLINE));
    Vector    world_scroll(calc_best_scroll_delta(internal->sorted_pos, viewport));

#if defined(DUMP_SCROLL_DETECT)
    AW_DUMP(viewport);
    AW_DUMP(world_scroll);
#endif

    if (world_scroll.has_length()) { // skip scroll if (nearly) nothing happens
        Vector screen_scroll = device->transform(world_scroll);
#if defined(DUMP_SCROLL_DETECT)
        AW_DUMP(screen_scroll);
#endif
        ntw->scroll(screen_scroll); // @@@ scroll the canvas (should be done by caller, to avoid recalculation on slave-canvas-resize)
    }
}

void SlaveCanvas::refresh() {
#if defined(DUMP_SYNC)
    fprintf(stderr, "DEBUG: SlaveCanvas does refresh (idx=%i)\n", get_index());
#endif
    get_canvas()->refresh();
}

SlaveCanvas::SlaveCanvas() :
    last_master(NULL),
    last_master_change(0),
    need_SetUpdate(true),
    need_PositionTrack(true),
    need_ScrollZoom(true),
    need_Refresh(true)
{
    internal = new SlaveCanvas_internal;
}

SlaveCanvas::~SlaveCanvas() {
    delete internal;
}

