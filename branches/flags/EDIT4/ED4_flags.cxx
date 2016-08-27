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

#include <arb_strbuf.h>

using namespace std;

SpeciesFlags *SpeciesFlags::SINGLETON = NULL;

typedef SpeciesFlagList::iterator       SpeciesFlagIter;
typedef SpeciesFlagList::const_iterator SpeciesFlagCiter;

void SpeciesFlags::create_instance() {
    e4_assert(!SINGLETON);
    SINGLETON = new SpeciesFlags;

    // @@@ members should be created from setup AWARs
    // @@@ currently we fake some:

#if 1
    SINGLETON->push_back(SpeciesFlag("cur", "curated"));
    SINGLETON->push_back(SpeciesFlag("bad", "bad_data"));
    SINGLETON->push_back(SpeciesFlag("new", "new_species"));
#endif

#if 0
    SINGLETON->push_back(SpeciesFlag("cr", "curated"));
    SINGLETON->push_back(SpeciesFlag("bd", "bad_data"));
    SINGLETON->push_back(SpeciesFlag("nw", "new"));
    SINGLETON->push_back(SpeciesFlag("xt", "extended"));
    SINGLETON->push_back(SpeciesFlag("fl", "flag"));
#endif

#if 0
    SINGLETON->push_back(SpeciesFlag("c", "curated"));
    SINGLETON->push_back(SpeciesFlag("b", "bad_data"));
    SINGLETON->push_back(SpeciesFlag("n", "new"));
    SINGLETON->push_back(SpeciesFlag("x", "extended"));
    SINGLETON->push_back(SpeciesFlag("f", "flag"));
#endif

#if 0
    SINGLETON->push_back(SpeciesFlag("curated", "curated"));
    SINGLETON->push_back(SpeciesFlag("bad",     "bad_data"));
#endif
}

void SpeciesFlags::build_header_text() const {
    GBS_strstruct buf(30);

    bool first = true;
    for (SpeciesFlagCiter i = begin(); i != end(); ++i) {
        if (!first) buf.put(' ');
        const SpeciesFlag& flag = *i;
        buf.cat(flag.get_shortname().c_str());
        first = false;
    }

    header = buf.release();
}

