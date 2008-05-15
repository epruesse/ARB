// =============================================================== //
//                                                                 //
//   File      : SEC_structure.cxx                                 //
//   Purpose   : general implementation of classes in SEC_root.hxx //
//   Time-stamp: <Mon Sep/17/2007 11:07 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "SEC_root.hxx"
#include "SEC_iter.hxx"

using namespace std;

// ---------------------
//      Constructors
// ---------------------

SEC_segment::SEC_segment()
    : alpha(0)
    , center1(Origin)
    , center2(Origin)
    , next_helix_strand(0)
    , loop(0)
{}

SEC_helix_strand::SEC_helix_strand()
    : origin_loop(0)
    , other_strand(0)
    , helix_info(0)
    , next_segment(0)
    , fixpoint(Origin)
    , rightAttach(Origin)
    , leftAttach(Origin)
{}

SEC_loop::SEC_loop(SEC_root *root_)
    : SEC_base(root_)
    , Circumferance(0)
    , center(0, 0)
    , primary_strand(0)
{}

SEC_helix::SEC_helix(SEC_root *root_, SEC_helix_strand *to_root, SEC_helix_strand *from_root)
    : SEC_base(root_)
    , strand_to_root(to_root)
    , base_length(0)
{
    sec_assert(to_root->get_helix()     == 0);
    sec_assert(from_root->get_helix()   == 0);
    sec_assert(to_root->get_other_strand()   == 0);
    sec_assert(from_root->get_other_strand() == 0);

    to_root->set_other_strand(from_root);
    from_root->set_other_strand(to_root);

    to_root->set_helix_info(this);
    from_root->set_helix_info(this);
}


SEC_root::SEC_root()
    : root_loop(0)
    , cursorAbsPos(-1)
    , xString(0)
    , constructing(false)
    , db(0)
    , bg_color(0)
    , autoscroll(0)
    , nailedAbsPos(-1)
    , drawnPositions(0)
    , cursor_line(LineVector(Origin, ZeroVector))
    , show_constraints(SEC_NO_TYPE)
{
}


void SEC_root::init(SEC_graphic *gfx, AWT_canvas *ntw) {
    // canvas   = ntw;
    db = new SEC_db_interface(gfx, ntw);
}

// --------------------
//      Destructors
// --------------------

SEC_region::SEC_region(int start, int end)
    : sequence_start(start)
    , sequence_end(end)
    , baseCount(-1)
    , abspos_array(NULL)
#if defined(DEBUG)
    , abspos_array_size(0)
#endif // DEBUG
{
    sec_assert((start == -1 && end == -1) || start != end);
}

SEC_region::~SEC_region() {
    invalidate_base_count(); // frees abspos_array
}

SEC_segment::~SEC_segment() {
}


SEC_helix_strand::~SEC_helix_strand() {
    if(next_segment != NULL) {
        next_segment->delete_pointer_2(this);
    }

    delete helix_info;

    if (other_strand != NULL) {
        if (other_strand->next_segment != NULL) {
            other_strand->next_segment->delete_pointer_2(other_strand);
        }
        other_strand->helix_info = NULL;
        other_strand->other_strand = NULL;
        other_strand->next_segment = NULL;
        delete other_strand;
    }

    delete origin_loop;
}


#define SEG_MAX 20

SEC_loop::~SEC_loop() {
    if (primary_strand) {
        sec_assert(get_root()->get_root_loop() != this);

        // collect all segments in an array
        SEC_segment *segment[SEG_MAX];
        int          i = 0;

        for (SEC_strand_iterator strand(this); strand; ++strand) {
            SEC_segment *seg = strand->get_next_segment();
            if (seg) {
                sec_assert(i < SEG_MAX);
                sec_assert(seg->get_loop() == this);

                segment[i++] = seg;
            }
        }

        set_fixpoint_strand(NULL); // disconnect from loop

        // delete all strands connected to loop
        int j;
        for (j=0; j<i; j++) {
            SEC_helix_strand *strand = segment[j]->get_next_strand();
            if (strand) {
                sec_assert(strand->get_origin_loop() == this);
                strand->set_origin_loop(NULL);
                delete strand;
            }
        }
        // delete all segments connected to loop
        for (j=0; j<i; j++) delete segment[j];        
    }
}

SEC_root::~SEC_root() {
    delete db;
    delete autoscroll;
    delete (get_root_loop());
    delete [] bg_color;
}

// -------------------------
//      integrity checks
// -------------------------

#if defined(CHECK_INTEGRITY)

void SEC_region::check_integrity(const SEC_root *root, SEC_CHECK_TYPE what) const {
    if (what&CHECK_SIZE) {
        sec_assert(baseCount >= 0);
        if (baseCount>0) {
            sec_assert(abspos_array || !root->get_db()->canDisplay());
        }
    }
}

void SEC_segment::check_integrity(SEC_CHECK_TYPE what) const {
    if (what&CHECK_STRUCTURE) {
        sec_assert(next_helix_strand != 0);
        sec_assert(loop == parent());
    }
    if (what&CHECK_SIZE) {
        sec_assert(alpha == alpha);
        sec_assert(alpha != 0);
    }
    if (what&CHECK_POSITIONS) {
        sec_assert(center1.valid());
        sec_assert(center2.valid());
    }
    get_region()->check_integrity(get_root(), what);
}

void SEC_helix_strand::check_integrity(SEC_CHECK_TYPE what) const {
    if (what&CHECK_STRUCTURE) {
        sec_assert(other_strand != this);
        sec_assert(other_strand->other_strand == this);
        sec_assert(helix_info);
        sec_assert(helix_info == other_strand->helix_info);
        sec_assert(parent() == helix_info);
    }
    if (what&CHECK_SIZE) {
        get_region()->get_base_count(); // asserts base count is up-to-date
    }
    if (what&CHECK_POSITIONS) {
        sec_assert(fixpoint.valid());
        sec_assert(rightAttach.valid());
        sec_assert(leftAttach.valid());
    }
    get_region()->check_integrity(get_root(), what);
}

void SEC_helix::check_integrity(SEC_CHECK_TYPE what) const {
    sec_assert(strand_to_root);
    
    SEC_helix_strand *other_strand = strand_to_root->get_other_strand();
    sec_assert(other_strand);

    if (what&CHECK_STRUCTURE) {
        sec_assert(strand_to_root->get_helix()      == this);
        sec_assert(other_strand->get_other_strand() == strand_to_root);
        sec_assert(get_rel_angle().valid());
        sec_assert(parent()                         == rootsideLoop());
    }

    if (what&CHECK_SIZE) {
        sec_assert(base_length >= 1);
        sec_assert(drawnSize() >= 0);
    }
    
    strand_to_root->check_integrity(what);
    other_strand->check_integrity(what);
}

void SEC_loop::check_integrity(SEC_CHECK_TYPE what) const {
    int count       = 0;
    int rootStrands = 0;

    if (what&CHECK_STRUCTURE) {
        sec_assert(primary_strand);
        sec_assert(primary_strand->get_origin_loop() == this);
        sec_assert(get_rel_angle().valid());
    }

    for (SEC_strand_const_iterator strand(this); strand; ++strand) {
        if (what&CHECK_STRUCTURE) {
            sec_assert(this == strand->get_origin_loop());
        }
        
        const SEC_helix *helix = strand->get_helix();
        if (this == helix->rootsideLoop()) { // test outgoing helixes
            helix->check_integrity(what);
        }
        else {
            rootStrands++;
        }
        
        const SEC_segment *seg = strand->get_next_segment();

        if (what&CHECK_STRUCTURE) {
            sec_assert(this == seg->get_loop());
        }
        seg->check_integrity(what);
        count++;
        sec_assert(count<100);  // more than 100 segments in one loop! assume error in structure!
    }

    if (what&CHECK_STRUCTURE) {
        if (is_root_loop()) {
            sec_assert(!rootStrands);
            sec_assert(primary_strand->pointsToOutside());
            sec_assert(parent() == 0);
        }
        else {
            sec_assert(rootStrands);
            sec_assert(primary_strand->pointsToRoot());
            sec_assert(parent() == get_rootside_helix());
        }
    }

    if (what&CHECK_SIZE) {
        sec_assert(Circumferance>0);
        sec_assert(drawnSize()>0);
    }
    if (what&CHECK_POSITIONS) {
        sec_assert(center.valid());
        if (is_root_loop()) {
            sec_assert(isOrigin(center));
        }
    }

    // now recurse downwards
    for (SEC_strand_const_iterator strand(this); strand; ++strand) {
        SEC_loop *outsideLoop = strand->get_helix()->outsideLoop();
        if (outsideLoop != this) outsideLoop->check_integrity(what);
    }
}

void SEC_root::check_integrity(SEC_CHECK_TYPE what) const {
    if (root_loop) {
        sec_assert(!under_construction()); // cannot check integrity, when structure is under construction
    
        root_loop->check_integrity(what);

        // check whether structure is a ring and whether regions are correct

        const SEC_base_part *start_part = root_loop->get_fixpoint_strand();
        const SEC_base_part *part       = start_part;
        const SEC_region    *region     = part->get_region();

        int count = 0;
        do {
            const SEC_base_part *next_part   = part->next();
            const SEC_region    *next_region = next_part->get_region();

            sec_assert(are_adjacent_regions(region, next_region));

            part   = next_part;
            region = next_region;

            count++;
            sec_assert(count<10000); // structure does not seem to be a ring
        }
        while (part != start_part);
    }
}
#endif // CHECK_INTEGRITY

// --------------------------------
//      unlink strands/segments
// --------------------------------

void SEC_helix_strand::unlink(bool fromOtherStrandAsWell) {
    // if called with fromOtherStrandAsWell == false,
    // the strand-pair remains deletable 
    
    next_segment = NULL;
    origin_loop  = NULL;

    if (fromOtherStrandAsWell) other_strand = NULL;
}

void SEC_segment::unlink(void) {
    next_helix_strand = NULL;
}

// -----------------------------
//      split/merge segments
// -----------------------------

SEC_helix_strand *SEC_segment::split(size_t start, size_t end, SEC_segment **segment2_ptr) {
    // split segment into 'segment1 - strand - segment2'
    // segment2 still points to same loop as 'this' (must be corrected by caller)
    // strand is a single strand (must be connected by caller)

    sec_assert(get_region()->contains_seq_position(start));
    sec_assert(get_region()->contains_seq_position(end-1));

    SEC_helix_strand *strand   = new SEC_helix_strand;
    SEC_segment      *segment2 = new SEC_segment;

    segment2->set_sequence_portion(end, get_region()->get_sequence_end());
    strand->set_sequence_portion(start, end);
    set_sequence_portion(get_region()->get_sequence_start(), start);

    segment2->set_loop(get_loop()); // set to same loop as this (must be corrected later)
    strand->set_origin_loop(get_loop());

    segment2->set_next_strand(get_next_strand());
    set_next_strand(strand);

    *segment2_ptr = segment2;

    return strand;
}

void SEC_segment::mergeWith(SEC_segment *other, SEC_loop *target_loop) {
    set_sequence_portion(get_region()->get_sequence_start(),
                         other->get_region()->get_sequence_end());

    set_next_strand(other->get_next_strand());
    set_loop(target_loop);
    delete other;
}

// ---------------------
//      Reset angles
// ---------------------

void SEC_loop::reset_angles() {
    for (SEC_strand_iterator strand(this); strand; ++strand) {
        if (strand->pointsToOutside()) {
            Angle abs(center, strand->get_fixpoint());
            strand->get_helix()->set_abs_angle(abs);
        }
    }
    set_rel_angle(0); 
}

void SEC_helix::reset_angles() {
    outsideLoop()->set_rel_angle(0);
    SEC_loop *rloop = rootsideLoop();

    Angle toFix(rloop->get_center(), strandToOutside()->get_fixpoint());
    set_abs_angle(toFix);
}


// --------------
//      other
// --------------

size_t SEC_base_part::getNextAbspos() const {
    // returns the next valid abspos
    const SEC_region *reg   = get_region();
    int               start = reg->get_sequence_start();

    if (start != reg->get_sequence_end()) {
        return start;
    }
    return next()->getNextAbspos();
}


SEC_segment *SEC_helix_strand::get_previous_segment(void) {
    SEC_segment *segment_before;
    SEC_helix_strand *strand_pointer = next_segment->get_next_strand();

    if (strand_pointer == this) {
        segment_before = next_segment;   //we are in a loop with only one segment
    }
    else {
        while (strand_pointer != this) {
            segment_before = strand_pointer->get_next_segment();
            strand_pointer = segment_before->get_next_strand();
        }
    }
    return segment_before;
}

static void findLongestHelix(const BI_helix *helix, size_t& start1, size_t& end1, size_t& start2, size_t& end2) {
    const char *longestHelixNr = 0;
    size_t      longestLength  = 0;

    const char *lastHelixNr  = 0;
    size_t      lastHelixLen = 0;
    for (long pos = helix->first_pair_position(); pos != -1; pos = helix->next_pair_position(pos)) {
        const char *currHelixNr = helix->helixNr(size_t(pos));
        if (currHelixNr != lastHelixNr) {
            if (lastHelixLen>longestLength) {
                longestLength  = lastHelixLen;
                longestHelixNr = lastHelixNr;
            }
            lastHelixNr  = currHelixNr;
            lastHelixLen = 1;
        }
        else {
            lastHelixLen++;
        }
    }
    
    if (lastHelixLen>longestLength) {
        longestLength  = lastHelixLen;
        longestHelixNr = lastHelixNr;
    }

    sec_assert(longestHelixNr);
    start1 = helix->first_position(longestHelixNr);
    end1   = helix->last_position(longestHelixNr);
    start2 = helix->opposite_position(end1);
    end2   = helix->opposite_position(start1);
}

void SEC_root::create_default_bone() {
    // create default structure

    set_under_construction(true);
    
    SEC_loop *loop1 = new SEC_loop(this);
    SEC_loop *loop2 = new SEC_loop(this);

    set_root_loop(loop1);

    SEC_segment *segment1 = new SEC_segment;
    SEC_segment *segment2 = new SEC_segment;

    segment1->set_loop(loop1);
    segment2->set_loop(loop2);

    SEC_helix_strand *strand1 = new SEC_helix_strand;
    SEC_helix_strand *strand2 = new SEC_helix_strand;

    loop1->set_fixpoint_strand(strand1);
    loop2->set_fixpoint_strand(strand2);

    segment1->set_next_strand(strand1);
    segment2->set_next_strand(strand2);

    strand1->set_origin_loop(loop1);
    strand2->set_origin_loop(loop2);

    SEC_helix *helix = new SEC_helix(this, strand1, strand2);

    strand1->set_next_segment(segment1);
    strand2->set_next_segment(segment2);

    size_t start1, end1, start2, end2;
    findLongestHelix(get_helix(), start1, end1, start2, end2);

    strand1->set_sequence_portion(start1, end1+1); segment2->set_sequence_portion(end1+1, start2);
    strand2->set_sequence_portion(start2, end2+1); segment1->set_sequence_portion(end2+1, start1);
    
    root_loop = helix->rootsideLoop();
    
    loop1->set_rel_angle(0);
    loop2->set_rel_angle(0);
    helix->set_rel_angle(0);

    root_loop->mark_angle_absolute();
    root_loop->set_center(Origin);

    set_under_construction(false);

    delete xString;
    xString = 0;
    generate_x_string();

    relayout();
}

