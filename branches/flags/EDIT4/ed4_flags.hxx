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

    int xpos; // inside terminal
    int width;

public:
    SpeciesFlag(const std::string& shortname_, const std::string& fieldname_) :
        shortname(shortname_),
        fieldname(fieldname_),
        xpos(-1),
        width(-1)
    {
        e4_assert(!shortname.empty()); // not allowed
    }

    const std::string& get_shortname() const { return shortname; }

    void set_dimension(int xpos_, int width_) {
        xpos  = xpos_;
        width = width_;
    }
    int get_xpos() const {
        e4_assert(xpos>=0);
        return xpos;
    }
    int get_width() const {
        e4_assert(width>=1); // empty header not allowed!
        return width;
    }

    double center_xpos() const {
        return get_xpos() + get_width()*0.5;
    }
};

typedef std::list<SpeciesFlag>          SpeciesFlagList;
typedef SpeciesFlagList::iterator       SpeciesFlagIter;
typedef SpeciesFlagList::const_iterator SpeciesFlagCiter;

class SpeciesFlags : public SpeciesFlagList {
    static SpeciesFlags *SINGLETON;
    static void create_instance();

    mutable SmartCharPtr header;
    mutable int          header_length; // characters

    int pixel_width; // width of text (plus a bit extra to avoid char-clipping + CHARACTEROFFSET)
    int min_flag_distance;

    SpeciesFlags() :
        header_length(-1),
        pixel_width(-1),
        min_flag_distance(INT_MAX)
    {}
    void build_header_text() const;

public:

    static const SpeciesFlags& instance() {
        if (!SINGLETON) create_instance();
        e4_assert(SINGLETON);
        return *SINGLETON;
    }
    static SpeciesFlags& mutable_instance() {
        return const_cast<SpeciesFlags&>(instance());
    }
    static void forget() {
        delete SINGLETON;
        SINGLETON = NULL;
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
    int get_pixel_width() const {
        e4_assert(pixel_width>0);
        return pixel_width;
    }
    int get_min_flag_distance() const {
        e4_assert(min_flag_distance != INT_MAX);
        return min_flag_distance;
    }

    void calculate_header_dimensions(AW_device *device, int gc);
};

AW_window *ED4_configure_species_flags(AW_root *root, GBDATA *gb_main);

#else
#error ed4_flags.hxx included twice
#endif // ED4_FLAGS_HXX

