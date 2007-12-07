// =============================================================== //
//                                                                 //
//   File      : SEC_split.cxx                                     //
//   Purpose   : split/unsplit loops (aka fold/unfold helices)     //
//   Time-stamp: <Fri Sep/07/2007 09:03 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include <map>

#include "SEC_root.hxx"
#include "SEC_drawn_pos.hxx"
#include "SEC_iter.hxx"

using namespace std;

enum AngleBufferMode {
    BUFFER_ABSOLUTE_ANGLES, 
    BUFFER_CENTER_RELATIVE, 
};

class AngleBuffer { // stores the absolute values of some SEC_oriented
    typedef std::map<SEC_helix*, Angle> AngleMap;

    AngleMap        angles;
    AngleBufferMode mode;

    Angle loop2helix(SEC_loop *loop, SEC_helix *helix) {
        return Angle(loop->get_center(), helix->strandAwayFrom(loop)->get_fixpoint());
    }

public:
    AngleBuffer(AngleBufferMode Mode) : mode(Mode) {}

    void store(SEC_helix *helix, SEC_loop *loop) {
        switch (mode) {
            case BUFFER_ABSOLUTE_ANGLES:
                angles[helix] = helix->get_abs_angle();
                break;
            case BUFFER_CENTER_RELATIVE:
                angles[helix] = helix->get_abs_angle()-loop2helix(loop, helix);
                break;
        }
    }

    void set_angle(SEC_helix *helix, const Angle& angle) { angles[helix] = angle; }

    void restore(SEC_helix *helix, SEC_loop *loop) {
        switch (mode) {
            case BUFFER_ABSOLUTE_ANGLES:
                helix->set_abs_angle(angles[helix]);
                break;
            case BUFFER_CENTER_RELATIVE:
                if (helix->hasLoop(loop)) {
                    helix->set_abs_angle(angles[helix]+loop2helix(loop, helix));
                }
                break;
        }
    }

    void restoreAll(SEC_loop *loop) {
        AngleMap::iterator e = angles.end();
        for (AngleMap::iterator a = angles.begin(); a != e; ++a) {
            restore(a->first, loop);
        }
    }

    void remove(SEC_helix *helix) { angles.erase(helix); }

    void storeAllHelices(SEC_loop *loop, SEC_helix *skip) {
        for (SEC_strand_iterator strand(loop); strand; ++strand) {
            SEC_helix *helix = strand->get_helix();
            if (helix != skip) store(helix, loop);
        }
    }
};


// --------------------
//      moving root
// --------------------

void SEC_loop::toggle_root(SEC_loop *old_root) {
    // make this the new root loop
    sec_assert(old_root != this);
    SEC_helix *mid_helix = get_rootside_helix();

    // set root to loop behind mid_helix
    {
        SEC_loop *behind = mid_helix->rootsideLoop();
        if (behind != old_root) {
            behind->toggle_root(old_root);
            old_root = behind;
        }
    }

    // store abs angles of all strands
    Angle midAngle = mid_helix->get_abs_angle();

    AngleBuffer thisOldAbs(BUFFER_ABSOLUTE_ANGLES);
    AngleBuffer otherOldAbs(BUFFER_ABSOLUTE_ANGLES);

    thisOldAbs.storeAllHelices(this, mid_helix);
    otherOldAbs.storeAllHelices(old_root, mid_helix);
    
    // modify structure
    get_root()->set_root_loop(this);
    mid_helix->flip();
    set_fixpoint_strand(mid_helix->strandAwayFrom(this));
    old_root->set_fixpoint_strand(mid_helix->strandAwayFrom(old_root));

    // calculate abs angles of loops and mid_helix
    set_rel_angle(Angle(center, get_fixpoint()));
    mark_angle_absolute();      // root-loop: rel == abs
    mid_helix->set_abs_angle(midAngle.rotate180deg());
    old_root->set_abs_angle(Angle(old_root->get_fixpoint(), old_root->get_center()));

    // restore angles of other helices
    thisOldAbs.restoreAll(this);
    otherOldAbs.restoreAll(old_root);
}

void SEC_root::set_root(SEC_loop *loop) {
    SEC_loop *old_root = get_root_loop();
    if (loop != old_root) {
        Vector new2old(loop->get_center(), old_root->get_center());
        add_autoscroll(new2old);
        loop->toggle_root(old_root);
        recalc();
    }
}


// ---------------------------------
//      search segment by abspos
// ---------------------------------

SEC_base_part *SEC_root::find(int pos) {
    SEC_helix_strand *start_strand = root_loop->get_fixpoint_strand();
    SEC_helix_strand *strand       = start_strand;
    do {
        SEC_region *reg = strand->get_region();
        if (reg->contains_seq_position(pos)) return strand;

        SEC_helix_strand *other_strand = strand->get_other_strand();
        SEC_region       *oreg         = other_strand->get_region();

        SEC_segment *seg;
        if (SEC_region(reg->get_sequence_end(), oreg->get_sequence_start()).contains_seq_position(pos)) {
            seg = other_strand->get_next_segment();
        }
        else {
            if (oreg->contains_seq_position(pos)) return other_strand;
            seg = strand->get_next_segment();
        }

        if (seg->get_region()->contains_seq_position(pos)) return seg;
        strand = seg->get_next_strand();
    }
    while (strand != start_strand);

    return 0;
}

inline SEC_segment *findSegmentContaining(SEC_root *root, int pos, GB_ERROR& error) {
    SEC_segment *result = 0;
    error               = 0;

    SEC_base_part *found  = root->find(pos);
    if (found) {
        if (found->parent()->getType() == SEC_LOOP) {
            result = static_cast<SEC_segment*>(found);
        }
        else {
            error = GBS_global_string("Position %i not in a segment", pos);
        }
    }
    else {
        error = GBS_global_string("Position %i is outside allowed range", pos);
    }
    return result;
}

inline SEC_segment *findSegmentContaining(SEC_root *root, int start, int end, GB_ERROR& error) {
    // end is position behind questionable position
    error = 0;

    SEC_segment *start_segment = findSegmentContaining(root, start, error);
    if (start_segment) {
        SEC_segment *end_segment;
        if (end == start+1) {
            end_segment = start_segment;;
        }
        else {
            end_segment = findSegmentContaining(root, end-1, error);
        }
        
        if (end_segment) {
            if (end_segment != start_segment) {
                error         = GBS_global_string("Positions %i and %i are in different segments", start, end);
                start_segment = 0;
            }
        }
    }
    sec_assert(!start_segment != !error); // either start_segment or error
    return start_segment;
}

// -------------------
//      split loop
// -------------------

GB_ERROR SEC_root::split_loop(int start1, int end1, int start2, int end2) {
    // end1/end2 are positions behind the helix-positions!
    sec_assert(start1<end1);
    sec_assert(start2<end2);

    if (start1>start2) {
        return split_loop(start2, end2, start1, end1);
    }

    GB_ERROR error = 0;
    if (start2<end1) {
        error = GBS_global_string("Helices overlap (%i-%i and %i-%i)", start1, end1, start2, end2);
    }

    if (!error) {
        SEC_segment *seg1 = findSegmentContaining(this, start1, end1, error);
        SEC_segment *seg2 = 0;

        if (!error) seg2 = findSegmentContaining(this, start2, end2, error);

        if (!error) {
            sec_assert(seg1 && seg2);
            SEC_loop *old_loop = seg1->get_loop();
            if (old_loop != seg2->get_loop()) {
                error = "Positions are in different loops (no tertiary structures possible)";
            }
            else {
                SEC_loop *setRootTo = 0; // set root back afterwards?

                if (old_loop->is_root_loop()) {
                    set_root(seg1->get_next_strand()->get_destination_loop()); // another loop
                    setRootTo = old_loop;
                }

                AngleBuffer oldAngles(BUFFER_CENTER_RELATIVE);
                oldAngles.storeAllHelices(old_loop, 0);

                SEC_helix *new_helix = 0;
                SEC_loop  *new_loop  = 0;
                
                if (seg1 == seg2) { // split one segment
                    //                           \                                                   .
                    //                            \ seg1     >>>                                     .
                    //     seg1                    \       strand1     ....
                    // ______________     =>        \_________________.    .   seg2
                    //                               _________________.    .
                    //                              /                  ....
                    //                             /       strand2
                    //                            / seg3     <<<
                    //                           /
                    //
                    // seg1 is the old segment

                    SEC_helix_strand *strand1 = seg1->split(start1, end1, &seg2);
                    SEC_helix_strand *strand2 = 0;
                    SEC_segment      *seg3    = 0;

                    if (seg1->get_region()->contains_seq_position(start2)) {
                        seg3    = seg2;
                        strand2 = strand1;
                        strand1 = seg1->split(start2, end2, &seg2);
                    }
                    else {
                        sec_assert(seg2->get_region()->contains_seq_position(start2));
                        strand2 = seg2->split(start2, end2, &seg3);
                    }

                    sec_assert(are_adjacent_regions(seg1->get_region(), strand1->get_region()));
                    sec_assert(are_adjacent_regions(strand1->get_region(), seg2->get_region()));
                    sec_assert(are_adjacent_regions(seg2->get_region(), strand2->get_region()));
                    sec_assert(are_adjacent_regions(strand2->get_region(), seg3->get_region()));

                    new_helix = new SEC_helix(this, strand2, strand1); // strands are responsible for memory

                    strand1->set_next_segment(seg3);
                    strand2->set_next_segment(seg2);

                    new_loop = new SEC_loop(this);

                    seg2->set_loop(new_loop);
                    
                    strand2->set_origin_loop(new_loop);
                    new_loop->set_fixpoint_strand(strand2);
                }
                else { // split two segments
                    //                                   \                       /
                    //     seg1                           \ seg1     >>>        / 
                    // ______________                      \       strand1     / seg3
                    //                                      \_________________/      
                    //                    =>     old_loop    _________________        new_loop
                    // ______________                       /                 \                                     .   
                    //     seg2                            /       strand2     \ seg2
                    //                                    / seg4     <<<        \                                   .
                    //                                   /                       \                                  .
                    //
                    // seg1 and seg2 are the old segments

                    // maybe swap seg1/seg2 (to ensure fixpoint-strand stays in old loop)
                    for (SEC_segment *s = seg1; s != seg2; ) {
                        SEC_helix_strand *hs = s->get_next_strand();
                        if (!hs->isRootsideFixpoint()) { // fixpoint-strand is between seg1 -> seg2
                            // swap seg1<->seg2
                            swap(seg1, seg2);
                            swap(start1, start2);
                            swap(end1, end2);
                            break;
                        }
                        s = hs->get_next_segment();
                    }

                    SEC_segment *seg3;
                    SEC_segment *seg4;
                    SEC_helix_strand *strand1 = seg1->split(start1, end1, &seg3);
                    SEC_helix_strand *strand2 = seg2->split(start2, end2, &seg4);

                    new_helix = new SEC_helix(this, strand2, strand1);
                    
                    strand1->set_next_segment(seg4);
                    strand2->set_next_segment(seg3);

                    new_loop = new SEC_loop(this);

                    for (SEC_segment *s = seg3; ; ) {
                        s->set_loop(new_loop);
                        SEC_helix_strand *h = s->get_next_strand();
                        h->set_origin_loop(new_loop);
                        if (s == seg2) break;
                        s = h->get_next_segment();
                    }

                    new_loop->set_fixpoint_strand(strand2);
                }

                // set angles of new helix and new loop
                new_helix->set_rel_angle(0); // wrong, but relayout fails otherwise
                new_loop->set_rel_angle(0);

                relayout(); 

                // correct angles of other helices
                oldAngles.restoreAll(new_loop);
                oldAngles.set_angle(new_helix, Angle(0));
                oldAngles.restoreAll(old_loop);

                recalc();      
                
                if (setRootTo) {
                    set_root(setRootTo); // restore root loop
                }
            }
        }
    }
    return error;
}

// ----------------------
//      fold a strand
// ----------------------

GB_ERROR SEC_root::unsplit_loop(SEC_helix_strand *remove_strand) {
    // 
    //          \ before[0]               / after[1]
    //           \                       /
    //            \        >>>>         /
    //             \      strand[0]    /
    //              \_________________/
    //   loop[0]     _________________           loop[1]
    //              /     strand[1]   \                             . 
    //             /       <<<<        \                            .
    //            /                     \                           .
    //           /                       \                          .
    //          / after[0]                \ before[1]
    //
    // The strands are removed and segments get connected.
    // One loop is deleted.

    GB_ERROR error = 0;
    SEC_helix_strand *strand[2] = { remove_strand, remove_strand->get_other_strand() };
    SEC_segment *before[2], *after[2];
    SEC_loop    *loop[2];

#if defined(CHECK_INTEGRITY)
    check_integrity(CHECK_STRUCTURE);
#endif // CHECK_INTEGRITY
    
    int s;
    for (s = 0; s<2; s++) {
        after[s]  = strand[s]->get_next_segment();
        before[s] = strand[s]->get_previous_segment();
        loop[s]   = strand[s]->get_origin_loop();

        sec_assert(before[s]->get_loop() == loop[s]);
        sec_assert(after[s]->get_loop() == loop[s]);
    }

    bool is_terminal_loop[2] = { before[0] == after[0], before[1] == after[1] };
    int  i0      = -1;          // index of terminal loop (or -1)
    bool unsplit = true;

    if (is_terminal_loop[0]) {
        if (is_terminal_loop[1]) {
            error   = "You cannot delete the last helix";
            unsplit = false;
        }
        else i0 = 0;
    }
    else {
        if (is_terminal_loop[1]) i0 = 1;
    }

    if (unsplit) {
        int del = i0 >= 0 ? i0 : 1; // index of loop which will be deleted

        SEC_loop *setRootTo = 0; // set root back to afterwards? 

        {
            // move away root-loop to make things easy
            SEC_loop *rootLoop = get_root_loop();
            if (loop[0] == rootLoop || loop[1] == rootLoop) {
                SEC_loop         *termLoop    = is_terminal_loop[0] ? loop[0] : loop[1];
                SEC_helix_strand *toTerm      = strand[0]->get_helix()->strandTowards(termLoop);
                SEC_helix_strand *toNextLoop  = toTerm->get_next_segment()->get_next_strand();
                SEC_loop         *anotherLoop = toNextLoop->get_destination_loop();

                sec_assert(anotherLoop != loop[0] && anotherLoop != loop[1]);

                set_root(anotherLoop);
                setRootTo = loop[1-del]; // afterwards set root back to non-deleted loop

                // recalc();
            }
        }

        SEC_helix   *removed_helix = strand[0]->get_helix();
        AngleBuffer  oldAngles(BUFFER_CENTER_RELATIVE);
        oldAngles.storeAllHelices(loop[0], removed_helix);
        oldAngles.storeAllHelices(loop[1], removed_helix);

        if (i0 >= 0) { // one loop is terminal
            // i0 and i1 are indexes 0 and 1 in picture above.
            // The left loop (loop[i0]) will be removed.
            int i1 = 1-i0; // index of non-terminal loop

            before[i1]->mergeWith(after[i1], loop[i1]);

            sec_assert(after[i0] == before[i0]);
            delete after[i0];       // delete the segment of the terminal-loop 
        }
        else { // none of the loops is terminal
            // keep loop[0], delete loop[1]

            SEC_helix_strand *rootsideStrand = strand[0]->get_helix()->rootsideLoop()->get_rootside_strand();

            before[1]->mergeWith(after[0], loop[0]);
            before[0]->mergeWith(after[1], loop[0]);
            // after[] segments are invalid now!

            // loop over all segments in loop[1] and relink them to loop[0]
            SEC_segment *seg = before[0];
            while (seg != before[1]) {
                SEC_helix_strand *loop1_strand = seg->get_next_strand();
                loop1_strand->set_origin_loop(loop[0]);

                seg = loop1_strand->get_next_segment();
                seg->set_loop(loop[0]);
            }

            loop[0]->set_fixpoint_strand(rootsideStrand);
        }

        loop[del]->set_fixpoint_strand(NULL);
        delete loop[del];

        for (s = 0; s<2; s++) strand[s]->unlink(false);
        delete strand[0];       // delete both strands

        relayout();
        
        oldAngles.restoreAll(loop[1-del]);
        recalc();  

        if (setRootTo) set_root(setRootTo);
    }

    return error;
}
