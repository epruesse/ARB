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

#include <aw_device.hxx>
#include <arb_strbuf.h>

using namespace std;

SpeciesFlags *SpeciesFlags::SINGLETON = NULL;

void SpeciesFlags::create_instance() {
    e4_assert(!SINGLETON);
    SINGLETON = new SpeciesFlags;

    // @@@ members should be created from setup AWARs
    // @@@ currently we fake some:

#if 0
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

#if 1
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

void SpeciesFlags::calculate_header_dimensions(AW_device *device, int gc) {
    int space_width = device->get_string_size(gc, " ", 1);
    int xpos        = CHARACTEROFFSET; // x-offset of first character of header-text

    const SpeciesFlag *prev_flag = NULL;

    min_flag_distance = INT_MAX;

    for (SpeciesFlagIter i = begin(); i != end(); ++i) {
        SpeciesFlag&  flag       = *i;
        const string& shortname  = flag.get_shortname();
        int           text_width = device->get_string_size(gc, shortname.c_str(), shortname.length());

        flag.set_dimension(xpos, text_width);
        xpos += text_width+space_width;

        if (prev_flag) {
            min_flag_distance = std::min(min_flag_distance, int(floor(flag.center_xpos()-prev_flag->center_xpos())));
        }
        else { // first
            min_flag_distance = std::min(min_flag_distance, int(floor(2*flag.center_xpos())));
        }
        prev_flag = &flag;
    }

    pixel_width = xpos - space_width + 1 + 1;       // +1 (pos->width) +1 (avoid character clipping)

    e4_assert(prev_flag);
    min_flag_distance = std::min(min_flag_distance, int(floor(2*((pixel_width-1)-prev_flag->center_xpos()))));
}



