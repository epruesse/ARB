
#ifndef awtc_island_hopping_h_included
#define awtc_island_hopping_h_included

typedef const char *GB_ERROR;

class IslandHopping;

class IslandHoppingParameter {
private:
    int    freqs;
    double fT;
    double fC;
    double fA;
    double fG;

    int rates;
    double rTC;
    double rTA;
    double rTG;
    double rCA;
    double rCG;
    double rAG;

    double dist;
    double supp;
    double gap;
    double thres;

    friend class IslandHopping;

public:
    IslandHoppingParameter(int    freqs_,
                           double fT_, double fC_, double fA_, double fG_,
                           int    rates_,
                           double rTC_, double rTA_, double rTG_, double rCA_, double rCG_, double rAG_,
                           double dist_, double supp_, double gap_, double thres_);

    virtual ~IslandHoppingParameter();

};


class IslandHopping {

private:
    static IslandHoppingParameter *para;

    int alignment_length;

    int firstColumn;
    int lastColumn;

    const char *ref_sequence;   // with gaps

    const char *toAlign_sequence; // with gaps

    const char *helix; // with gaps

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

        helix = 0;

        output_sequence         = 0;
        aligned_ref_sequence    = 0;
        output_alignment_length = 0;
    }

    void set_parameters(int    freqs,
                        double fT, double fC, double fA, double fG,
                        int    rates,
                        double rTC, double rTA, double rTG, double rCA, double rCG, double rAG,
                        double dist, double supp, double gap, double thres)
    {
        delete para;
        para = new IslandHoppingParameter(freqs, fT, fC , fA, fG, rates, rTC, rTA, rTG, rCA, rCG, rAG , dist, supp, gap, thres);
    }

    virtual ~IslandHopping() {
        delete output_sequence;
        delete aligned_ref_sequence;
    }

    void set_alignment_length(int len) { alignment_length = len; }

    void set_ref_sequence(const char *ref_seq) { ref_sequence = ref_seq; }

    void set_toAlign_sequence(const char *toAlign_seq) { toAlign_sequence = toAlign_seq; }

    void set_helix(const char *hel) { helix = hel; }

    void set_range(int first_col,int last_col) {
        firstColumn=first_col;
        lastColumn=last_col;
    }

    const char *get_result() const { return output_sequence; }
    const char *get_result_ref() const { return aligned_ref_sequence; }
    int get_result_length() const { return output_alignment_length; }

    bool was_aligned() const { return output_sequence && aligned_ref_sequence; }

    GB_ERROR do_align();

};

#endif
