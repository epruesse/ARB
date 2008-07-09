//  ==================================================================== //
//                                                                       //
//    File      : mapping.h                                              //
//    Purpose   : simple species mapping                                 //
//    Time-stamp: <Wed Jul/09/2008 12:52 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2003        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//  ==================================================================== //

// This is not really a header - it's included only once per executable!

#ifndef _CPP_MAP
#include <map>
#endif
#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#ifndef _CPP_CSTDLIB
#include <cstdlib>
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
        GBDATA *pb_mapping = GB_entry(pb_main, "species_mapping");

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

SpeciesID PM_name2ID(const string& name, GB_ERROR& error) {
    pm_assert(PM_species_maps_initialized);

    map<string, SpeciesID>::const_iterator found = PM_species2num_map.find(name);
    if (found != PM_species2num_map.end()) {
        return found->second;
    }

    error = GBS_global_string("Unknown species '%s'", name.c_str());
    return -1;
}

const string& PM_ID2name(SpeciesID id, GB_ERROR& error) {
    pm_assert(PM_species_maps_initialized);

    map<SpeciesID, string>::const_iterator found = PM_num2species_map.find(id);
    if (found != PM_num2species_map.end()) {
        return found->second;
    }

    error = GBS_global_string("Unknown SpeciesID %i", id);
    static string illegal = "<illegal>";
    return illegal;
}

