// ================================================================ //
//                                                                  //
//   File      : MostLikelySeq.hxx                                  //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef MOSTLIKELYSEQ_HXX
#define MOSTLIKELYSEQ_HXX

#ifndef AP_SEQUENCE_HXX
#include <AP_sequence.hxx>
#endif
#ifndef DOWNCAST_H
#include <downcast.h>
#endif

extern class DNA_Table {
    char char_to_enum_table[256];
public:
    DNA_Base char_to_enum(char i) {
        return (DNA_Base)char_to_enum_table[(unsigned char)i];
    }
    DNA_Table();
} dna_table;

const size_t ST_MAX_SEQ_PART = 256;                 /* should be greater than the editor width (otherwise extrem performance penalties)
                                                     * (Please note: this value has as well a small influence on the calculated results)
                                                     */

const int ST_BUCKET_SIZE = 16;                      // at minimum ST_BUCKET_SIZE characters are calculated per call
const int LD_BUCKET_SIZE = 4;                       // log dualis of ST_BUCKET_SIZE

class ST_ML;

class MostLikelySeq : public AP_sequence { // derived from a Noncopyable
    /*! contains existing sequence or ancestor sequence
     * as max. likelihood vectors
     */
public:
    static ST_base_vector *tmp_out;                 // len = alignment length (@@@ could be member of ST_ML ? )

private:
    ST_ML          *st_ml;                     // link to a global ST object (@@@ could be static)
    ST_base_vector *sequence;                       // A part of the sequence
    bool            up_to_date;
public:
    // @@@ move the 2 following members into one new class and put one pointer here
    ST_ML_Color    *color_out;
    int            *color_out_valid_till;           // color_out is valid up to

private:
    AP_FLOAT count_weighted_bases() const OVERRIDE;

    void set(const char *sequence) OVERRIDE;
    void unset() OVERRIDE;

public:

    MostLikelySeq(const AliView *aliview, ST_ML *st_ml_);
    ~MostLikelySeq() OVERRIDE;

    bool is_up_to_date() const { return up_to_date; }

    AP_sequence *dup() const OVERRIDE;
    AP_FLOAT     combine(const AP_sequence* lefts, const AP_sequence *rights, char *mutation_per_site = 0) OVERRIDE;
    void partial_match(const AP_sequence* part, long *overlap, long *penalty) const OVERRIDE;
    uint32_t checksum() const OVERRIDE;

    bool equals(const MostLikelySeq */*other*/) const { arb_assert(0); return false; } // unused
    bool equals(const AP_sequence *other) const OVERRIDE { return equals(DOWNCAST(const MostLikelySeq*, other)); }

    GB_ERROR bind_to_species(GBDATA *gb_species);
    void     unbind_from_species(bool remove_callbacks);
    GBDATA *get_bound_species_data() const { return AP_sequence::get_bound_species_data(); }

    void sequence_change();                         // sequence has changed in db
    void set_sequence();                            // start at st_ml->base

    void calculate_ancestor(const MostLikelySeq *lefts, double leftl, const MostLikelySeq *rights, double rightl);
    void forget_sequence() { up_to_date = false; }

    void calc_out(const MostLikelySeq *sequence_of_brother, double dist);
    void print();
};


#else
#error MostLikelySeq.hxx included twice
#endif // MOSTLIKELYSEQ_HXX

