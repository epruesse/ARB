// =============================================================== //
//                                                                 //
//   File      : AP_sequence.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "AP_sequence.hxx"
#include <arbdbt.h>

long AP_sequence::global_combineCount;

AP_sequence::AP_sequence(const AliView *aliview)
    : cached_wbc(-1.0)
    , ali(aliview)
    , gb_sequence(NULL)
    , has_sequence(false)
    , update(0)
{
}

GB_ERROR AP_sequence::bind_to_species(GBDATA *gb_species) {
    GB_ERROR error = NULL;

    ap_assert(!gb_sequence); // already bound to species!
    if (!gb_sequence) {
        GB_transaction ta(ali->get_gb_main());
        gb_sequence = GBT_read_sequence(gb_species, ali->get_aliname());
        if (!gb_sequence) {
            error = GBS_global_string("Species '%s' has no data in alignment '%s'",
                                      GBT_get_name(gb_species), ali->get_aliname());
        }
        unset();
    }
    return error;
}

void AP_sequence::unbind_from_species() {
    ap_assert(gb_sequence);
    gb_sequence = NULL;
    unset();
}

void AP_sequence::do_lazy_load() const {
    ap_assert(gb_sequence);
    ap_assert(!has_sequence);

    GB_transaction ta(gb_sequence);
    const char *seq = GB_read_char_pntr(gb_sequence);
    if (!seq) {
        GB_ERROR    error      = GB_await_error();
        GBDATA     *gb_species = GB_get_grandfather(gb_sequence);
        const char *name       = GBT_get_name(gb_species);

        GB_warningf("Failed to load sequence of '%s'\n(Reason: %s)", name, error);
        seq = "";                                   // fake empty sequence
    }

    // this is no modification, this is lazy initialization
    const_cast<AP_sequence*>(this)->set(seq);
}


