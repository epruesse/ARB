// =============================================================== //
//                                                                 //
//   File      : pt_probepart.h                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2013   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef PT_PROBEPART_H
#define PT_PROBEPART_H

#ifndef _GLIBCXX_LIST
#include <list>
#endif
#ifndef _GLIBCXX_MAP
#include <map>
#endif
#ifndef SMARTPTR_H
#include <smartptr.h>
#endif
#ifndef PT_PROTOTYPES_H
#include "pt_prototypes.h"
#endif

#if defined(DEBUG)
// #define DUMP_PROBEPARTS
#endif

class ProbePart {
    const SmartCharPtr  sequence; // sequence of part
    PT_tprobes         *source;
    int                 start;    // the number of characters cut off at the start of the 'source'

    double dt;                   // add this temperature to the result
    double sum_bonds;            // the sum of bonds of the longest non mismatch string

    int cmp_seq(const ProbePart& other) const { return strcmp(&*sequence, &*other.sequence); }

public:

    ProbePart(const SmartCharPtr seq, PT_tprobes& src, int pos)
        : sequence(seq),
          source(&src),
          start(pos),
          dt(0.0),
          sum_bonds(0.0)
    {}
    ProbePart(const SmartCharPtr seq, const ProbePart& ref)
        : sequence(seq),
          source(&ref.get_source()),
          start(ref.get_start()),
          dt(0.0),
          sum_bonds(0.0)
    {}
    ProbePart(const ProbePart& other)
        : sequence(other.sequence),
          source(other.source),
          start(other.start),
          dt(other.dt),
          sum_bonds(other.sum_bonds)
    {}
    DECLARE_ASSIGNMENT_OPERATOR(ProbePart);

    const SmartCharPtr get_sequence() const { return sequence; }
    PT_tprobes& get_source() const { return *source; }
    int get_start() const { return start; }

    double get_dt() const { return dt; }
    double get_sum_bonds() const { return sum_bonds; }

    void set_dt_and_bonds(double dt_, double sum_bonds_) {
        dt        = dt_;
        sum_bonds = sum_bonds_;
    }

    bool is_same_part_as(const ProbePart& other) const { return &source == &other.source && cmp_seq(other) == 0; }
    bool seq_less(const ProbePart& other) const { return cmp_seq(other)<0; }

#if defined(DUMP_PROBEPARTS)
    SmartCharPtr get_readable_sequence() const {
        const char *seq      = &*get_sequence();
        return readable_probe(seq, strlen(seq), 'T');
    }
#endif
};

typedef std::list<ProbePart> Parts;
typedef Parts::iterator      PartsIter;

#else
#error pt_probepart.h included twice
#endif // PT_PROBEPART_H
