// ============================================================= //
//                                                               //
//   File      : MG_adapt_ali.hxx                                //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef MG_ADAPT_ALI_HXX
#define MG_ADAPT_ALI_HXX

#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif
#ifndef ARB_STRARRAY_H
#include <arb_strarray.h>
#endif

class MG_remap;

struct MG_remaps : virtual Noncopyable {
    int             n_remaps;
    ConstStrArray   alignment_names;
    MG_remap      **remaps;

    MG_remaps(GBDATA *gb_left, GBDATA *gb_right, bool enable, const char *reference_species_names);
    ~MG_remaps();
};


GB_ERROR MG_transfer_all_alignments(MG_remaps *remaps, GBDATA *source_species, GBDATA *destination_species);

#else
#error mg_adapt_ali.hxx included twice
#endif // MG_ADAPT_ALI_HXX
