// =============================================================== //
//                                                                 //
//   File      : SEC_layout.cxx                                    //
//   Purpose   : layout size and positions of structure            //
//   Time-stamp: <Mon Sep/10/2007 10:55 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <algorithm>


#include "SEC_root.hxx"
#include "SEC_iter.hxx"

using namespace std;

const Angle& SEC_oriented::calc_abs_angle() const {
    sec_assert(!abs_angle_valid);

    const SEC_base *previous = parent();
    if (previous) {
#if defined(DEBUG)
        static int avoid_deep_recursion = 0;
        avoid_deep_recursion++;
        sec_assert(avoid_deep_recursion<1000); // looks like a bug
#endif // DEBUG

        abs_angle = previous->get_abs_angle()+rel_angle;

#if defined(DEBUG)
        avoid_deep_recursion--;
#endif // DEBUG
    }
    else {                      // no parent = root loop
        abs_angle = rel_angle;
    }
    abs_angle_valid = true;

    return abs_angle;
}
const Angle& SEC_oriented::calc_rel_angle() {
    const SEC_base *previous = parent();
    if (previous) {
        rel_angle = abs_angle - previous->get_abs_angle();
    }
    else {
        rel_angle = abs_angle;
    }
    abs_angle_valid = true;

    return rel_angle;
}

// ------------------------------------
//      calculate size of structure
// ------------------------------------

void SEC_segment::calculate_segment_size() {
    alpha = ((get_region()->get_base_count()+1) / loop->get_circumferance()) * (2*M_PI);
}

void SEC_loop::compute_circumferance(void) {  // Calculates the circumferance of the loop by counting the bases of the loop
    SEC_root *root = get_root();
    double    dbs  = root->display_params().distance_between_strands;

    Circumferance = 0;
    for (SEC_segment_iterator seg(this); seg; ++seg) {
        SEC_region *reg = seg->get_region();
        reg->update_base_count(root);
        Circumferance += reg->get_base_count() + 1 + dbs;
    }
}

void SEC_loop::compute_radius(void) {
    compute_circumferance();
    setStandardSize(Circumferance / (2 * M_PI));
}

void SEC_loop::calculate_loop_size() {
    compute_radius();

    for (SEC_segment_iterator seg(this); seg; ++seg) {
        seg->calculate_segment_size();
        SEC_helix_strand *strand = seg->get_next_strand();
        if (strand->isRootsideFixpoint()) { // for all strands pointing away from loop
            strand->get_helix()->calculate_helix_size();
        }
    }
}

void SEC_helix::calculate_helix_size() {
    SEC_region *reg1 = strandToRoot()->get_region();
    SEC_region *reg2 = strandToOutside()->get_region();
    SEC_root   *root = get_root();

    reg1->update_base_count(root);
    reg2->update_base_count(root);

    reg1->align_helix_strands(root, reg2); // aligns both strands

    base_length = max(reg1->get_base_count(), reg2->get_base_count());
    if (base_length == 0) {
        printf("Helix w/o size faking length=1\n");
        base_length = 1;
    }
    setStandardSize(base_length-1);

    strandToRoot()->get_origin_loop()->calculate_loop_size();
}

void SEC_root::calculate_size() {
    SEC_loop *root_loop = get_root_loop();
    if (root_loop) root_loop->calculate_loop_size();
}

// -------------------------------------------
//      calculate coordinates of structure
// -------------------------------------------

void SEC_segment::calculate_segment_coordinates(const Position& start, const Position& end) {
    // start is rightAttach of previous strand, end is leftAttach of next strand.
    // both strands are already correct.

    const Position& loopCenter = loop->get_center();

    Vector start_center(start, loopCenter);
    Vector end_center(end, loopCenter);

    double radius = loop->drawnSize();

    start_center.normalize() *= radius;
    end_center.normalize()   *= radius;

    center1 = start+start_center;
    center2 = end+end_center;
}

void SEC_loop::calculate_loop_coordinates() {
    // assumes the fixpoint helix and loop-center are correct
    SEC_helix        *fixpoint_helix = get_fixpoint_helix();
    SEC_helix_strand *strand_away    = fixpoint_helix->strandAwayFrom(this);
    const Position&   loop_fixpoint  = strand_away->get_fixpoint();

    Angle current(center, loop_fixpoint);

    double dbs                   = get_root()->display_params().distance_between_strands;
    double angle_between_strands = ( dbs / Circumferance) * (2*M_PI); //angle between two strands

    SEC_segment      *seg     = strand_away->get_next_segment();
    SEC_helix_strand *pstrand = strand_away;

#if defined(DEBUG)
    static int avoid_deep_recursion = 0;
    avoid_deep_recursion++;
    sec_assert(avoid_deep_recursion<500); // structure with more than 500 loops ? Sure ? 
#endif // DEBUG

    while (seg) {
        SEC_helix_strand *strand   = seg->get_next_strand();
        SEC_segment      *next_seg = 0;

        if (strand != strand_away) {
            current += seg->get_alpha()+angle_between_strands;
            strand->setFixpoint(center + current.normal()*drawnSize());
            strand->get_helix()->calculate_helix_coordinates();

            next_seg = strand->get_next_segment();
        }

        seg->calculate_segment_coordinates(pstrand->rightAttachPoint(), strand->leftAttachPoint());

        pstrand = strand;
        seg     = next_seg;
    }

#if defined(DEBUG)
    avoid_deep_recursion--;
#endif // DEBUG

}

void SEC_helix::calculate_helix_coordinates() {
    // assumes the rootside fixpoint and the rootside loop-center are correct
    SEC_helix_strand *strand1 = strandToOutside();
    const Position&   fix1    = strand1->get_fixpoint();

    const Angle& loopAngle   = strand1->get_origin_loop()->get_abs_angle();
    Angle        strandAngle = loopAngle+get_rel_angle();

    Position          fix2    = fix1 + strandAngle.normal()*drawnSize();
    SEC_helix_strand *strand2 = strand1->get_other_strand();
    strand2->setFixpoint(fix2);

    // calculate attachment points
    double dbs          = get_root()->display_params().distance_between_strands;
    Vector fix1_rAttach = strandAngle.normal() * (dbs * 0.5);
    fix1_rAttach.rotate90deg();

    strand1->setAttachPoints(fix1-fix1_rAttach, fix1+fix1_rAttach);
    strand2->setAttachPoints(fix2+fix1_rAttach, fix2-fix1_rAttach);

    // calculate loop-center of outside loop
    SEC_loop *nextLoop    = outsideLoop();
    Angle     fix2_center = strandAngle + nextLoop->get_rel_angle();
    Position  loopCenter  = fix2 + fix2_center.normal()*nextLoop->drawnSize();

    nextLoop->set_center(loopCenter);
    nextLoop->calculate_loop_coordinates();
}

void SEC_root::calculate_coordinates() {
    SEC_loop *root_loop = get_root_loop();
    
    if (root_loop) {
        root_loop->set_center(Origin);
        root_loop->mark_angle_absolute(); // mark angle as absolute

        SEC_helix *primary_helix = root_loop->get_fixpoint_helix();

        // calculate the coordinates of the primary helix
        const Angle& loopAngle = root_loop->get_abs_angle();
        Position     rootside  = Origin + loopAngle.normal() * root_loop->drawnSize();
        Position     outside   = rootside + Angle(loopAngle + primary_helix->get_abs_angle()).normal() * primary_helix->drawnSize();

        primary_helix->setFixpoints(rootside, outside);
        primary_helix->calculate_helix_coordinates();

        root_loop->calculate_loop_coordinates(); // does not calculate for the primary helix
    }
}

// ---------------------------
//      angle invalidation
// ---------------------------

void SEC_oriented::invalidate() {
    if (abs_angle_valid) { // skip recursion if already invalidated
        invalidate_sub_angles();
        abs_angle_valid = false;
    }
}

void SEC_helix::invalidate_sub_angles() {
    SEC_loop *outLoop = outsideLoop(); // does not exist during read
    if (outLoop) {
        outLoop->invalidate();
    }
    else {
        sec_assert(get_root()->under_construction()); // loop missing and structure 
    }
}

void SEC_loop::invalidate_sub_angles() {
    for (SEC_strand_iterator strand(this); strand; ++strand) {
        if (strand->isRootsideFixpoint()) { // outgoing strand
            strand->get_helix()->invalidate();
        }
    }
}

// --------------------
//      count bases
// --------------------

void SEC_region::invalidate_base_count() {
    delete [] abspos_array;
    abspos_array      = 0;
#if defined(DEBUG)
    abspos_array_size = 0;
#endif // DEBUG
    
    baseCount = -1;
}

void SEC_region::create_abspos_array(const int *static_array) {
    sec_assert(abspos_array == 0);
    sec_assert(baseCount >= 0);

    if (baseCount>0) {
        abspos_array = new int[baseCount];
        memcpy(abspos_array, static_array, baseCount*sizeof(*static_array));
    }
#if defined(DEBUG)
    abspos_array_size = baseCount;
#endif // DEBUG
}

void SEC_region::count_bases(SEC_root *root) {
    invalidate_base_count();

    bool is_endings_seg = false;
    int  max_index      = root->max_index();

    sec_assert(sequence_start <= max_index);
    sec_assert(sequence_end <= (max_index+1));

    int size;
    int last;
    
    if (sequence_end < sequence_start) { // if this is the "endings-segment"
        size           = (max_index - sequence_start + 1) + sequence_end;
        last           = max_index;
        is_endings_seg = true;
    }
    else {
        size = sequence_end - sequence_start;
        last = sequence_end-1;
    }

    sec_assert(root->get_db()->canDisplay());
    static int *static_array        = NULL;
    static int  sizeof_static_array = 0;

    if (size > sizeof_static_array) {
        delete [] static_array;
        static_array = new int[size];
        sizeof_static_array = size;
    }

    baseCount = 0;

    int i;
    for (i = sequence_start; i <= last; ++i) {
        if (root->shallDisplayPosition(i)) {
            sec_assert(baseCount < size);
            static_array[baseCount++] = i;
        }
    }

    if (is_endings_seg) {
        for (i = 0; i < sequence_end; ++i) {
            if (root->shallDisplayPosition(i)) {
                sec_assert(baseCount < size);
                static_array[baseCount++] = i;
            }
        }
    }

    sec_assert(baseCount <= size);
    create_abspos_array(static_array);

    sec_assert(baseCount <= size);
}



