/*********************************************************************************
 *  Coded by Tina Lai/Ralf Westram (coder@reallysoft.de) 2001-2003               *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef PG_DB_HXX
#define PG_DB_HXX

#ifndef arbdb_h_included
#include "arbdb.h"
#endif
#ifndef __STRING__
#include <string>
#endif
#ifndef __SET__
#include <set>
#endif


// conventions for database-pointer-names:
//
//  - arb-db            is referenced by    gb_...
//  - probe-group-db    is referenced by    pb_...
//

typedef int SpeciesID;

GB_ERROR PG_initSpeciesMaps(GBDATA* gb_main, GBDATA *pb_main); // call this at startup
GB_ERROR PG_transfer_root_string_field(GBDATA *pb_src, GBDATA *pb_dest, const char *field_name);

SpeciesID     PG_SpeciesName2SpeciesID(const std::string& shortname);
const std::string& PG_SpeciesID2SpeciesName(SpeciesID id);

int PG_NumberSpecies();

//  --------------------
//      class PG_Group
//  --------------------
class PG_Group {
private:
    std::set<SpeciesID> species;

public:
    PG_Group() {}
    virtual ~PG_Group() {}

    void add(SpeciesID id) { species.insert(id); }
    void add(const std::string& species_name) { add(PG_SpeciesName2SpeciesID(species_name)); }

    std::set<SpeciesID>::const_iterator 	begin() const { return species.begin(); }
    std::set<SpeciesID>::const_iterator 	end() const { return species.end(); }

    size_t 	size() const { return species.size(); }
    size_t 	empty() const { return species.empty(); }

    GBDATA *groupEntry(GBDATA *pb_main, bool create, bool& created, int *numSpecies); // searches or creates a group-entry in pb-db

    bool operator == (const PG_Group& other) const { return species == other.species; }
};

GBDATA *PG_get_first_probe(GBDATA *pb_group);
GBDATA *PG_get_next_probe(GBDATA *pb_probe);

const char *PG_read_probe(GBDATA *pb_probe);

GBDATA *PG_find_probe(GBDATA *pb_group, const char *probe);
GBDATA *PG_add_probe(GBDATA *pb_group, const char *probe);


#else
#error pg_db.hxx included twice
#endif // PG_DB_HXX
