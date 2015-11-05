// =========================================================== //
//                                                             //
//   File      : island_hopping.h                              //
//   Purpose   :                                               //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de)               //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef ISLAND_HOPPING_H
#define ISLAND_HOPPING_H

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef ARB_CORE_H
#include <arb_core.h>
#endif
#ifndef POS_RANGE_H
#include <pos_range.h>
#endif

class IslandHopping;

class IslandHoppingParameter {
    int    use_user_freqs;
    double fT;
    double fC;
    double fA;
    double fG;

    double rTC;
    double rTA;
    double rTG;
    double rCA;
    double rCG;
    double rAG;

    double dist;
    double supp;
    double gapA;
    double gapB;
    double gapC;
    double thres;

    friend class IslandHopping;

public:
    IslandHoppingParameter(bool use_user_freqs_,
                           double fT_, double fC_, double fA_, double fG_,
                           double rTC_, double rTA_, double rTG_, double rCA_, double rCG_, double rAG_,
                           double dist_, double supp_, double gapA_, double gapB_, double gapC_, double thres_);

    virtual ~IslandHoppingParameter();
};

class IslandHopping : virtual Noncopyable {
    static IslandHoppingParameter *para;

    int alignment_length;

    int firstColumn; // @@@ go PosRange
    int lastColumn;

    const char *ref_sequence;   // with gaps

    const char *toAlign_sequence; // with gaps

    const char *ref_helix; // with gaps
    const char *toAlign_helix; // with gaps

    char *aligned_ref_sequence; //aligned (ref_sequence)
    char *output_sequence;      // aligned (toAlign_sequence)

    int output_alignment_length;


public:

    IslandHopping() {

        alignment_length = 0;

        firstColumn = 0;
        lastColumn = -1;

        ref_sequence = 0;

        toAlign_sequence = 0;

        ref_helix = 0;
        toAlign_helix = 0;

        output_sequence         = 0;
        aligned_ref_sequence    = 0;
        output_alignment_length = 0;
    }

    void set_parameters(bool use_user_freqs,
                        double fT, double fC, double fA, double fG,
                        double rTC, double rTA, double rTG, double rCA, double rCG, double rAG,
                        double dist, double supp, double gapA, double gapB, double gapC, double thres)
    {
        delete para;
        para = new IslandHoppingParameter(use_user_freqs, fT, fC , fA, fG, rTC, rTA, rTG, rCA, rCG, rAG , dist, supp, gapA, gapB, gapC, thres);
    }

    virtual ~IslandHopping() {
        delete output_sequence;
        delete aligned_ref_sequence;
    }

    void set_alignment_length(int len) { alignment_length = len; }

    void set_ref_sequence(const char *ref_seq) { ref_sequence = ref_seq; }

    void set_toAlign_sequence(const char *toAlign_seq) { toAlign_sequence = toAlign_seq; }

    void set_helix(const char *hel) {
        ref_helix     = hel;
        toAlign_helix = hel;
    }

    void set_range(ExplicitRange range) {
        firstColumn = range.start();
        lastColumn  = range.end();
    }

    const char *get_result() const { return output_sequence; }
    const char *get_result_ref() const { return aligned_ref_sequence; }
    int get_result_length() const { return output_alignment_length; }

    bool was_aligned() const { return output_sequence && aligned_ref_sequence; }

    GB_ERROR do_align();

};

#else
#error island_hopping.h included twice
#endif // ISLAND_HOPPING_H
