// ============================================================== //
//                                                                //
//   File      : ED4_flags.cxx                                    //
//   Purpose   : species flag implementation                      //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2016   //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include "ed4_flags.hxx"

using namespace std;

SpeciesFlags *SpeciesFlags::SINGLETON = NULL;

void SpeciesFlags::create_instance() {
    e4_assert(!SINGLETON);
    SINGLETON = new SpeciesFlags;

    // @@@ members should be created from AWARs
    // @@@ currently we fake some:

    SINGLETON->push_back(SpeciesFlag("cur", "curated"));
    SINGLETON->push_back(SpeciesFlag("bad", "bad_data"));
    SINGLETON->push_back(SpeciesFlag("new", "new_species"));
}

