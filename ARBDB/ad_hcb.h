// =============================================================== //
//                                                                 //
//   File      : ad_hcb.h                                          //
//   Purpose   : hierarchical callbacks                            //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2014   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AD_HCB_H
#define AD_HCB_H

#ifndef ARBDB_BASE_H
#include "arbdb_base.h"
#endif
#ifndef GB_LOCAL_H
#include "gb_local.h"
#endif
#ifndef GB_KEY_H
#include "gb_key.h"
#endif

// --------------------------------
//      hierarchical callbacks


/*! Stores path to a specific location in DB hierarchy as list of GBQUARKs
 */
class gb_hierarchy_location {
    static const int MAX_HIERARCHY_DEPTH = 10; // no real limit, just avoids dynamic allocation
    GBQUARK quark[MAX_HIERARCHY_DEPTH];

    void invalidate() { quark[0] = 0; }

public:
    explicit gb_hierarchy_location(GBDATA *gbd) {
        for (int offset = 0; gbd; ++offset) {
            gb_assert(offset<MAX_HIERARCHY_DEPTH); // increase MAX_HIERARCHY_DEPTH (or use dynamic mem)
            quark[offset] = GB_KEY_QUARK(gbd);
            if (!quark[offset]) return;

            gbd = gbd->get_father();
        }
        gb_assert(0); // did not reach DB-root (invalid entry?)
    }
    gb_hierarchy_location(GBDATA *gb_main, const char *db_path);

    bool is_valid() const { return quark[0] != 0; }

    bool matches(GBDATA *gbd) const {
        //! return true if 'gbd' is at 'this' hierarchy location
        if (is_valid()) {
            for (int offset = 0; gbd; ++offset) {
                GBQUARK q = GB_KEY_QUARK(gbd);
                if (!quark[offset]) return !q;
                if (q != quark[offset]) return false;

                gbd = gbd->get_father();
            }
            gb_assert(0); // went beyond root
        }
        return false;
    }

    bool operator == (const gb_hierarchy_location& other) const {
        if (is_valid() && other.is_valid()) {
            int offset;
            for (offset = 0; quark[offset]; ++offset) {
                if (quark[offset] != other.quark[offset]) return false;
            }
            return other.quark[offset] == 0;
        }
        return false;
    }

    char *get_db_path(GBDATA *gb_main) const;
};

class gb_hierarchy_callback : public gb_callback {
    gb_hierarchy_location loc;
public:
    gb_hierarchy_callback(const TypedDatabaseCallback& spec_, const gb_hierarchy_location& loc_)
        : gb_callback(spec_),
          loc(loc_)
    {}
    bool triggered_by(GBDATA *gbd) const { return loc.matches(gbd); }
    const gb_hierarchy_location& get_location() const { return loc; }
};

struct gb_hierarchy_callback_list : public CallbackList<gb_hierarchy_callback> {
    // need forward decl for gb_hierarchy_callback_list, i.e. cant use a simple typedef here
};

#else
#error ad_hcb.h included twice
#endif // AD_HCB_H


