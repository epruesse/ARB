// =============================================================== //
//                                                                 //
//   File      : SEC_iter.hxx                                      //
//   Purpose   : secondary structure iterators                     //
//   Time-stamp: <Fri Sep/07/2007 09:01 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2007    //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SEC_ITER_HXX
#define SEC_ITER_HXX

// iterates over all parts of the structure (segments/strands)
class SEC_base_part_iterator {
    SEC_base_part *start;
    SEC_base_part *curr;

public:
    SEC_base_part_iterator(SEC_root *root)
        : start(root->get_root_loop()->get_fixpoint_strand()), curr(start)
    {}
    
    SEC_base_part& operator*() { return *curr; }
    SEC_base_part* operator->() { return curr; }

    SEC_base_part_iterator& operator++() {
        if (curr) {
            curr = curr->next();
            if (curr == start) curr = 0;
        }
        return *this;
    }

    operator bool() const { return curr != 0; }

};

// iterates over all bases of the structure (loops/helices)
class SEC_base_iterator {
    SEC_base_part_iterator part; // always points to an outgoing strand
    bool                   do_loop; // true -> curr is origin loop, false -> curr is helix

    SEC_base *curr() {
        SEC_helix_strand& strand = static_cast<SEC_helix_strand&>(*part);
        return do_loop
            ? static_cast<SEC_base*>(strand.get_origin_loop())
            : static_cast<SEC_base*>(strand.get_helix());
    }

public:
    SEC_base_iterator(SEC_root *root) : part(root), do_loop(true) {}

    SEC_base& operator*() { return *curr(); }
    SEC_base* operator->() { return curr(); }

    SEC_base_iterator& operator++() {
        if (part) {
            if (do_loop) do_loop = false;
            else {
                // skip over all strands pointing to root
                int steps = 0;
                do {
                    ++++part;
                    ++steps;
                }
                while (part && static_cast<SEC_helix_strand&>(*part).pointsToRoot());
                do_loop = (steps == 1);
            }
        }
        return *this;
    }
    
    operator bool() const { return part; }
};


// iterates over all strands in one loop (starting with fixpoint strand)
class SEC_strand_iterator { 
    SEC_helix_strand *start;
    SEC_helix_strand *curr;
public:
    SEC_strand_iterator(SEC_loop *loop) : start(loop->get_fixpoint_strand()), curr(start) {}

    SEC_helix_strand& operator*() { return *curr; }
    SEC_helix_strand* operator->() { return curr; }

    SEC_strand_iterator& operator++() {
        sec_assert(curr);
        SEC_segment *seg = curr->get_next_segment();

        if (seg) {
            curr = seg->get_next_strand();
            if (curr == start) curr = 0;
        }
        else {
            curr = 0;
        }
        return *this;
    }

    operator bool() const { return curr != 0; }
};

// iterates over all segments in one loop (starting with segment behind fixpoint strand)
class SEC_segment_iterator : private SEC_strand_iterator {
    const SEC_strand_iterator& strand_iter() const { return static_cast<const SEC_strand_iterator&>(*this); }
    SEC_strand_iterator& strand_iter() { return static_cast<SEC_strand_iterator&>(*this); }
public:
    SEC_segment_iterator(SEC_loop *loop) : SEC_strand_iterator(loop) {}

    SEC_segment& operator*() { return *strand_iter()->get_next_segment(); }
    SEC_segment* operator->() { return strand_iter()->get_next_segment(); }

    SEC_segment_iterator& operator++() {
        SEC_strand_iterator& si = strand_iter();
        ++si;
        return *this;
    }

    operator bool() const { return strand_iter(); }
    
    SEC_helix_strand *get_previous_strand() { return &*strand_iter(); }
};


// const versions

class SEC_strand_const_iterator : private SEC_strand_iterator {
    const SEC_strand_iterator& strand_iter() const { return static_cast<const SEC_strand_iterator&>(*this); }
    SEC_strand_iterator& strand_iter() { return static_cast<SEC_strand_iterator&>(*this); }
public:
    SEC_strand_const_iterator(const SEC_loop *loop) : SEC_strand_iterator(const_cast<SEC_loop*>(loop)) {}
    const SEC_helix_strand& operator*() { return *strand_iter(); }
    const SEC_helix_strand* operator->() { return strand_iter().operator->(); }
    SEC_strand_const_iterator& operator++() { return static_cast<SEC_strand_const_iterator&>(++strand_iter()); }
    operator bool() const { return bool(strand_iter()); }
};

#else
#error SEC_iter.hxx included twice
#endif // SEC_ITER_HXX
