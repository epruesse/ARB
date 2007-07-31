/*********************************************************************************
 *  Coded by Tina Lai/Ralf Westram (coder@reallysoft.de) 2001-2004               *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/

#ifndef PG_DB_HXX
#define PG_DB_HXX

#ifndef __CSTDIO__
#include <cstdio>
#endif
#ifndef ARBDB_H
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
typedef std::set<SpeciesID> SpeciesBag;
typedef SpeciesBag::const_iterator SpeciesBagIter;

GB_ERROR PG_initSpeciesMaps(GBDATA* gb_main, GBDATA *pb_main); // call this at startup
GB_ERROR PG_transfer_root_string_field(GBDATA *pb_src, GBDATA *pb_dest, const char *field_name);

SpeciesID     PG_SpeciesName2SpeciesID(const std::string& shortname);
const std::string& PG_SpeciesID2SpeciesName(SpeciesID id);

int PG_NumberSpecies();

size_t PG_match_path(const char *members, SpeciesBagIter start, SpeciesBagIter end, SpeciesBagIter& lastMatch, const char *&mismatch);
bool   PG_match_path_with_mismatches(const char *members, SpeciesBagIter start, SpeciesBagIter end, int size, int allowed_mismatches,
                                     /*result parameters : */ SpeciesBagIter& nextToMatch, int& used_mismatches, int& matched_members);

//  --------------------
//      class PG_Group
//  --------------------
class PG_Group {
private:
    SpeciesBag species;

public:
    PG_Group() {}
    virtual ~PG_Group() {}

    void add(SpeciesID id) { species.insert(id); }
    void add(const std::string& species_name) { add(PG_SpeciesName2SpeciesID(species_name)); }

    SpeciesBagIter begin() const { return species.begin(); }
    SpeciesBagIter end() const { return species.end(); }
    const SpeciesBag& bag() const { return species; }

    size_t size() const { return species.size(); }
    size_t empty() const { return species.empty(); }

    GBDATA *groupEntry(GBDATA *pb_main, bool create, bool& created); // searches or creates a group-entry in pb-db

    bool operator == (const PG_Group& other) const { return species == other.species; }
};

bool PG_add_probe(GBDATA *pb_group, const char *probe);

#else
#error pg_db.hxx included twice
#endif // PG_DB_HXX
