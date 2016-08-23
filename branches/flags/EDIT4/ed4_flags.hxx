// ============================================================== //
//                                                                //
//   File      : ed4_flags.hxx                                    //
//   Purpose   : Representation of species-flag setup             //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2016   //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef ED4_FLAGS_HXX
#define ED4_FLAGS_HXX

#ifndef _GLIBCXX_STRING
#include <string>
#endif
#ifndef _GLIBCXX_LIST
#include <list>
#endif
#ifndef ED4_DEFS_HXX
#include "ed4_defs.hxx"
#endif

class SpeciesFlag {
    std::string shortname; // abbreviation used in flag-header
    std::string fieldname; // name of species field

    // @@@ store textsize here?

public:
    SpeciesFlag(const std::string& shortname_, const std::string& fieldname_) :
        shortname(shortname_),
        fieldname(fieldname_)
    {}
};

class SpeciesFlags : std::list<SpeciesFlag> {
    static SpeciesFlags *SINGLETON;

    SpeciesFlags() {}
    static void create_instance();
public:

    // @@@ add config() (=open GUI)

    static const SpeciesFlags& get() {
        if (!SINGLETON) create_instance();
        e4_assert(SINGLETON);
        return *SINGLETON;
    }
};



#else
#error ed4_flags.hxx included twice
#endif // ED4_FLAGS_HXX

