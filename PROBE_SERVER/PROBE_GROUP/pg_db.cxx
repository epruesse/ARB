#include <ctype.h>

#ifndef __MAP__
#include <map>
#endif

#ifndef __PG_DB_HXX__
#include <pg_db.hxx>
#endif
#ifndef __PG_DEF_HXX__
#include <pg_def.hxx>
#endif

#include <arbdbt.h>


using namespace std;

// API for Probe-Group-Database format

// --------------------------------------------------------------------------------
// mapping shortname <-> SpeciesID

static map<string, int> species2num_map;
static map<int, string> num2species_map;
static bool             species_maps_initialized = false;

//  ----------------------------------------------------------------------
//      GB_ERROR PG_initSpeciesMaps(GBDATA* gb_main, GBDATA *pb_main)
//  ----------------------------------------------------------------------
GB_ERROR PG_initSpeciesMaps(GBDATA* gb_main, GBDATA *pb_main) {
    GB_transaction gb_dummy(gb_main);
    GB_transaction pb_dummy(pb_main);

    pg_assert(!species_maps_initialized);

    // look for existing mapping in pb-db:
    GBDATA *pb_mapping = GB_find(pb_main, "species_mapping", 0, down_level);
    if (!pb_mapping) {          // none found
        if (!gb_main) return GB_export_error("Need ARB-database!"); // in this case we need arb-db!

        SpeciesID species_count = 0;
        for (GBDATA *gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species         = GBT_next_species(gb_species))
        {
            GBDATA *gb_name = GB_find(gb_species, "name", 0, down_level);
            if (!gb_name) return GB_export_error("species w/o name");

            string name = GB_read_char_pntr(gb_name);
            species_count++;

            species2num_map[name]          = species_count;
            num2species_map[species_count] = name;
        }

        // create string from mapping and save in db
        pb_mapping = GB_create(pb_main, "species_mapping", GB_STRING);
        if (pb_mapping) {
#define PER_ENTRY (8+20)            // estimated length/mapping in string
            string mapping;
            mapping.reserve(species2num_map.size()*PER_ENTRY);

            for (map<string, SpeciesID>::const_iterator m = species2num_map.begin();
                 m != species2num_map.end(); ++m)
            {
                char buffer[PER_ENTRY+1];
                sprintf(buffer, "%s,%i;", m->first.c_str(), m->second);
                mapping = mapping + buffer;
            }

            GB_write_string(pb_mapping, mapping.c_str());
#undef PER_ENTRY
        }
    }
    else {
        // retrieve mapping from string
        const char *mapping = GB_read_char_pntr(pb_mapping);
        if (!mapping) return GB_export_error("Can't read mapping");

        while (mapping[0]) {
            const char *komma     = strchr(mapping, ',');   if (!komma) break;
            const char *semicolon = strchr(komma, ';');     if (!semicolon) break;
            string      name(mapping, komma-mapping);
            komma+=1;
            string idnum(komma,semicolon-komma);
            SpeciesID   id        = atoi(idnum.c_str());

            species2num_map[name] = id;
            num2species_map[id]   = name;

            mapping = semicolon+1;
        }
    }

    species_maps_initialized = true;
    return 0;
}

GB_ERROR PG_transfer_root_string_field(GBDATA *pb_src, GBDATA *pb_dest, const char *field_name) {
    GB_ERROR error = 0;
    GB_push_transaction(pb_src);
    GB_push_transaction(pb_dest);

    GBDATA *pb_src_mapping = GB_find(pb_src, field_name, 0, down_level);
    if (!pb_src_mapping) {
        error = GBS_global_string("no such entry '%s'", field_name);
    }
    else {
        GBDATA *pb_dest_mapping = GB_search(pb_dest, field_name, GB_STRING);
        if (!pb_dest_mapping) {
            error = GB_get_error();
            error = GBS_global_string("can't create '%s' (reason: %s)", field_name, error ? error : "unknown");
        }
        else {
            const char *mapping = GB_read_char_pntr(pb_src_mapping);
            if (!mapping) {
                error = GB_get_error();
                error = GBS_global_string("can't read '%s' (reason: %s)", field_name, error ? error : "unknown");
            }
            else {
                error = GB_write_string(pb_dest_mapping, mapping);
            }
        }
    }

    if (!error) {
        GB_pop_transaction(pb_dest);
        GB_pop_transaction(pb_src);
    }
    else {
        GB_abort_transaction(pb_dest);
        GB_abort_transaction(pb_src);
    }
    return error;
}

//  --------------------------------------------------------------------
//      SpeciesID PG_SpeciesName2SpeciesID(const string& shortname)
//  --------------------------------------------------------------------
SpeciesID PG_SpeciesName2SpeciesID(const string& shortname) {
    pg_assert(species_maps_initialized); // you didn't call PG_initSpeciesMaps
    return species2num_map[shortname];
}

//  --------------------------------------------------------------
//      const string& PG_SpeciesID2SpeciesName(SpeciesID num)
//  --------------------------------------------------------------
const string& PG_SpeciesID2SpeciesName(SpeciesID num) {
    pg_assert(species_maps_initialized); // you didn't call PG_initSpeciesMaps
    return num2species_map[num];
}

int PG_NumberSpecies(){
    return num2species_map.size();
}//PG_NumberSpecies

// db-structure of group_tree:
//
//                  <root>
//                  |
//                  |
//                  "group_tree"
//                  |
//                  |
//                  "node" <more nodes...>
//                  | | |
//                  | | |
//                  | | "group" (contains all probes for this group; may be missing)
//                  | |
//                  | "num" (contains species-number (created by PG_SpeciesName2SpeciesID))
//                  |
//                  "node" <more nodes...>
//
//  Notes:  - the "node"s contained in the path from "group_tree" to any "group"
//            describes the members of the group


//  ---------------------------------------------------
//      static GBDATA *group_tree(GBDATA *pb_main)
//  ---------------------------------------------------
// search or create "group_tree"-entry
static GBDATA *group_tree(GBDATA *pb_main) {
    return GB_search(pb_main, "group_tree", GB_CREATE_CONTAINER);
}

//  -------------------------------------------------------------------------------
//      GBDATA *PG_Group::groupEntry(GBDATA *pb_main, bool create, bool& created)
//  -------------------------------------------------------------------------------
// search/create entry for group

GBDATA *PG_Group::groupEntry(GBDATA *pb_main, bool create, bool& created, int* numSpecies) {
    GB_transaction  dummy(pb_main);
    GBDATA         *pb_group_tree   = group_tree(pb_main);
    GBDATA         *pb_current_node = pb_group_tree;

    created = false;

    *numSpecies=0;

    for (set<SpeciesID>::const_iterator i = begin(); i != end(); ++i)
    {
        SpeciesID id = *i;
        char      buffer[20];
        sprintf(buffer, "%i", id);

        GBDATA 	*pb_num  = GB_find(pb_current_node, "num", buffer, down_2_level);
        GBDATA 	*pb_node = 0;

        if (pb_num) {
	    pb_node = GB_get_father(pb_num);
	}
	else {
            if (!create) return 0;	// not found

            pb_node = GB_create_container(pb_current_node, "node");
#if defined(DEBUG)
	    if (!pb_node) fprintf(stderr, "Error: %s\n", GB_get_error());
#endif // DEBUG
	    pg_assert(pb_node);

            pb_num = GB_create(pb_node, "num", GB_STRING);

	    pg_assert(pb_num);

            GB_write_string(pb_num, buffer);

            created = true;
        }
	pg_assert(pb_node);
        pb_current_node = pb_node;
    }

    GBDATA *pb_group = GB_find(pb_current_node, "group", 0, down_level);
    if (!pb_group) {
        pb_group = GB_create_container(pb_current_node, "group");
        created  = true;
	*numSpecies=size();
    }
    return pb_group;
}

//  -----------------------------------------------------
//      GBDATA *PG_get_first_probe(GBDATA *pb_group)
//  -----------------------------------------------------
GBDATA *PG_get_first_probe(GBDATA *pb_group) {
    return GB_find(pb_group, "probe", 0, down_level);
}
//  ----------------------------------------------------
//      GBDATA *PG_get_next_probe(GBDATA *pb_probe)
//  ----------------------------------------------------
GBDATA *PG_get_next_probe(GBDATA *pb_probe) {
    return GB_find(pb_probe, "probe", 0, this_level|search_next);
}

//  ----------------------------------------------------
//      const char *PG_read_probe(GBDATA *pb_probe)
//  ----------------------------------------------------
const char *PG_read_probe(GBDATA *pb_probe) {
    return GB_read_char_pntr(pb_probe);
}

//  --------------------------------------------
//      inline bool is_upper(const char *s)
//  --------------------------------------------
inline bool is_upper(const char *s) {
    for (int i = 0; s[i]; ++i) {
        if (!isupper(s[i])) return false;
    }
    return true;
}


//  -------------------------------------------------------------------
//      GBDATA *PG_find_probe(GBDATA *pb_group, const char *probe)
//  -------------------------------------------------------------------
GBDATA *PG_find_probe(GBDATA *pb_group, const char *probe) {
    pg_assert(is_upper(probe));

    for (GBDATA *pb_probe = PG_get_first_probe(pb_group);
         pb_probe;
         pb_probe = PG_get_next_probe(pb_probe))
    {
        if (strcmp(PG_read_probe(pb_probe), probe) == 0) {
            return pb_probe;
        }
    }
    return 0;
}
//  ------------------------------------------------------------------
//      GBDATA *PG_add_probe(GBDATA *pb_group, const char *probe)
//  ------------------------------------------------------------------
// adds probe to group (if not already existing)

GBDATA *PG_add_probe(GBDATA *pb_group, const char *probe) {
    pg_assert(is_upper(probe));
    GBDATA *pb_probe = PG_find_probe(pb_group, probe);

    if (!pb_probe) {
        pb_probe = GB_create(pb_group, "probe", GB_STRING);
        GB_write_string(pb_probe, probe);
    }
    return pb_probe;
}
