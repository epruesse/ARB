//  ==================================================================== //
//                                                                       //
//    File      : mapping.cxx                                            //
//    Purpose   : simple species mapping                                 //
//    Time-stamp: <Mon Oct/06/2003 17:17 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2003        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef __MAP__
#include <map>
#endif

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define pm_assert(cond) arb_assert(cond)

using namespace std;

typedef int SpeciesID;

static map<string, SpeciesID> PM_species2num_map;
static map<SpeciesID, string> PM_num2species_map;
static bool                   PM_species_maps_initialized = false;

GB_ERROR PM_initSpeciesMaps(GBDATA *pb_main) {
    GB_transaction dummy(pb_main);
    GB_ERROR       error = 0;

    if (PM_species_maps_initialized) {
        error = "initSpeciesMaps called twice";
    }
    else {
        GBDATA *pb_mapping = GB_find(pb_main, "species_mapping", 0, down_level);

        if (!pb_mapping) {
            error = GB_get_error();
        }
        else {
            const char *mapping = GB_read_char_pntr(pb_mapping);
            if (!mapping) return "Can't read mapping";

            while (mapping[0]) {
                const char *komma     = strchr(mapping, ',');   if (!komma) break;
                const char *semicolon = strchr(komma, ';');     if (!semicolon) break;

                string name(mapping, komma-mapping);
                komma += 1;

                string    idnum(komma,semicolon-komma);
                SpeciesID id = atoi(idnum.c_str());

                PM_species2num_map[name] = id;
                PM_num2species_map[id]   = name;

                mapping = semicolon+1;
            }

            PM_species_maps_initialized = true;
        }
    }

    return error;
}

SpeciesID PM_name2ID(const string& name) {
    pm_assert(PM_species_maps_initialized);
    return PM_species2num_map[name];
}
const string& PM_ID2name(SpeciesID id) {
    pm_assert(PM_species_maps_initialized);
    return PM_num2species_map[id];
}

