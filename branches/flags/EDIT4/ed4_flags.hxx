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

    const std::string& get_shortname() const { return shortname; }
};

typedef std::list<SpeciesFlag> SpeciesFlagList;

class SpeciesFlags : public SpeciesFlagList {
    static SpeciesFlags *SINGLETON;
    static void create_instance();

    mutable SmartCharPtr header;
    mutable int          header_length;

    SpeciesFlags() :
        header_length(-1)
    {}
    void build_header_text() const;

public:

    // @@@ add config() (=open GUI)

    static const SpeciesFlags& instance() {
        if (!SINGLETON) create_instance();
        e4_assert(SINGLETON);
        return *SINGLETON;
    }

    const char *get_header_text() const {
        if (header.isNull()) build_header_text();
        return &*header;
    }
    int get_header_length() const {
        if (header_length<0) {
            header_length = strlen(get_header_text());
        }
        return header_length;
    }
};



#else
#error ed4_flags.hxx included twice
#endif // ED4_FLAGS_HXX

