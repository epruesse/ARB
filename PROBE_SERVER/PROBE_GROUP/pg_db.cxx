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
        pb_mapping = GB_search(pb_main, "species_mapping", GB_STRING);
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

SpeciesID PG_SpeciesName2SpeciesID(const string& shortname) {
    pg_assert(species_maps_initialized); // you didn't call PG_initSpeciesMaps
    return species2num_map[shortname];
}

const string& PG_SpeciesID2SpeciesName(SpeciesID num) {
    pg_assert(species_maps_initialized); // you didn't call PG_initSpeciesMaps
    return num2species_map[num];
}

int PG_NumberSpecies(){
    return num2species_map.size();
}

// // db-structure of group_tree (old outdated structure):
// //
// //                  <root>
// //                  |
// //                  |
// //                  "group_tree"
// //                  |
// //                  |
// //                  "node" <more nodes...>
// //                  | | |
// //                  | | |
// //                  | | "group" (contains all probes for this group; may be missing)
// //                  | |
// //                  | "num" (contains species-number (created by PG_SpeciesName2SpeciesID))
// //                  |
// //                  "node" <more nodes...>
// //
// //  Notes:  - the "node"s contained in the path from "group_tree" to any "group"
// //            describes the members of the group
// // 
// // end of old structure
// 
// db-structure of group_tree (current structure): 
// 
//              <root>
//              |
//              |
//              "group_tree" ---- "path" ---- "members" (string: comma-separated list of SpeciesID's)
//                             |           |        
//                             |           |- "probes" (string: comma-separated list of probes)        
//                             |           |        
//                             |           |- [optional more] "path" (continued path)        
//                             |
//                             |
//                             |- [optional more] "path" (other path)
// 
// 
// Note: - the sum of all "members"-entries of all
//         parent "path"-nodes forms the group of 
//         species matched by the probes in "probes"


// search or create "group_tree"-entry
static GBDATA *group_tree(GBDATA *pb_main) {
    return GB_search(pb_main, "group_tree", GB_CREATE_CONTAINER);
}

// typedef set<SpeciesID>::const_iterator SpeciesIter;

size_t PG_match_path(const char *members, SpeciesBagIter start, SpeciesBagIter end, SpeciesBagIter& lastMatch, const char *&mismatch) {
    SpeciesID first       = atoi(members);
    size_t    match_count = 0;

    lastMatch = end;
    mismatch  = 0;

    while (start != end) {
        if (*start == first) {
            ++match_count;
            lastMatch = start;

            const char *comma = strchr(members, ',');
            if (!comma) {
                members = 0;
                break;          // end of members in path
            }

            members = comma+1;
            first   = atoi(members);

            ++start;
        }
        else {                  // mismatch
            break;
        }
    }

    if (members) mismatch = members; // not all members matched 

    return match_count;
}

bool PG_match_path_with_mismatches(const char *members, SpeciesBagIter start, SpeciesBagIter end, int size, int allowed_mismatches,
                                   SpeciesBagIter& nextToMatch, int& used_mismatches, int& matched_members)
{
#warning currently unused
    bool            path_matched = false;
    const char     *mismatch;
    SpeciesBagIter  lastMatch;

    used_mismatches = 0;
    matched_members = 0;
    nextToMatch     = start;

    while (1) {
        size_t matched = PG_match_path(members, nextToMatch, end, lastMatch, mismatch);
        if (matched) {
            matched_members += matched;

            if (matched == (size_t)size) { // complete (rest of) path matched
                nextToMatch  = end;
                path_matched = true;
                break;
            }

            // partial match
            nextToMatch = lastMatch;
            ++nextToMatch;

            if (!mismatch) {
                // all "members" were matched
                path_matched = true;
                break;
            }

            if (used_mismatches >= allowed_mismatches) break; // no (more) mismatches allowed

            ++used_mismatches; // count mismatches

            const char *comma = strchr(members, ',');
            if (!comma) { // mismatch on last element of "members"
                path_matched = true;
                break;
            }

            members  = comma+1;
            size    -= matched; // calculate size of rest of SpeciesBag
        }
        else {
            if (used_mismatches >= allowed_mismatches) break; // no (more) mismatches allowed

            ++used_mismatches;  // count mismatches

            const char *comma = strchr(members, ',');
            if (!comma) { // "members" only contain 1 element
                path_matched = true;
                break;
            }
            members = comma+1;
        }
    }

    pg_assert(used_mismatches <= allowed_mismatches);
    return path_matched;
}

static string generate_path_string(SpeciesBagIter start, SpeciesBagIter end, size_t size) {
    string result;
    result.reserve((5+1)*size);

    for (; start != end; ++start) {
        char buffer[30];
        sprintf(buffer, "%i,", *start);
        result += buffer;
    }

    result.resize(result.length()-1, 'x'); // remove trailing comma

    return result;
}

static GBDATA *groupEntry_recursive(GBDATA *pb_father, SpeciesBagIter start, SpeciesBagIter end, size_t size, bool& created) {
    GBDATA   *pb_probes = 0;    // result
    GB_ERROR  error     = 0;

    for (GBDATA *pb_path = GB_find(pb_father, "path", 0, down_level);
         pb_path && !error;
         pb_path = GB_find(pb_path, "path", 0, this_level|search_next))
    {
        GBDATA     *pb_members = GB_find(pb_path, "members", 0, down_level);
        const char *members    = GB_read_char_pntr(pb_members);

        SpeciesBagIter  last_match;
        const char  *mismatch;
        size_t       same = PG_match_path(members, start, end, last_match, mismatch);

        if (same) {             // at least one member of path matched
            GBDATA *pb_dest_path = 0; // father of result probe entry

            if (same == size && !mismatch) { // complete match -> existing group
                pg_assert(last_match != end);

                pb_dest_path = pb_path; // "probes" entry here
            }
            else {
                pg_assert(last_match != end);
                SpeciesBagIter  first_of_rest    = last_match;
                ++first_of_rest;
                GBDATA         *pb_restpath_root = pb_path;

                if (mismatch) { // partial match
                    // 1. split existing path
                    string common_path(members, mismatch-1);

                    GBDATA *pb_sub         = GB_create_container(pb_path, "path");
                    GBDATA *pb_sub_members = GB_create(pb_sub, "members", GB_STRING);
                    GB_write_string(pb_sub_members, mismatch);
                    GB_write_string(pb_members, common_path.c_str());

                    GBDATA *pb_old_probes = GB_find(pb_path, "probes", 0, down_level);
                    if (pb_old_probes) {
                        GBDATA *pb_sub_probes = GB_create(pb_sub, "probes", GB_STRING);

                        if (!error) error = GB_copy(pb_sub_probes, pb_old_probes);
                        if (!error) error = GB_delete(pb_old_probes); // delete splitted probe_group
                    }
#if defined(DEBUG)
                    else {
                        // group w/o "probes" entry should have at least two "path" entries
                        GBDATA *pb_first_path = GB_find(pb_path, "path", 0, down_level);

                        pg_assert(pb_first_path != 0);
                        pg_assert(GB_find(pb_first_path, "path", 0, this_level|search_next) != 0);
                    }
#endif // DEBUG

                    // copy all other path-entries

                    for (GBDATA *pb_sub_path = GB_find(pb_path, "path", 0, down_level), *pb_next_sub = 0;
                         pb_sub_path && !error;
                         pb_sub_path = pb_next_sub)
                    {
                        pb_next_sub = GB_find(pb_sub_path, "path", 0, this_level|search_next);

                        if (pb_sub_path != pb_sub) { // do not copy NEW sub-path
                            GBDATA *pb_sub_path_copy = GB_create_container(pb_sub, "path");
                            error                    = GB_copy(pb_sub_path_copy, pb_sub_path);
                            if (!error) error        = GB_delete(pb_sub_path);
                        }
                    }
                }
                else {
                    // complete match, but SpeciesBag has additional members
                    pg_assert(first_of_rest != end);
                }

                // 2. generate distinct path for tail of set

                if (first_of_rest != end) {
                    pb_probes = groupEntry_recursive(pb_restpath_root, first_of_rest, end, size-same, created);
                    pg_assert(pb_probes);
                }
                else {
                    pb_dest_path = pb_restpath_root;
                }
            }

            if (pb_dest_path) {
                pg_assert(!pb_probes);

                pb_probes    = GB_find(pb_dest_path, "probes", 0, down_level);
                if (!pb_probes) {
                    pb_probes = GB_create(pb_dest_path, "probes", GB_STRING);
                    pg_assert(pb_probes);
                    GB_write_string(pb_probes, "");
                    created = true;
                }
            }

            pg_assert(pb_probes);

            break;              // break main loop (only 1 path does match partially)
        }
    }

    if (!pb_probes && !error) { // no match at all -> create distinct group
        GBDATA *pb_path    = GB_create_container(pb_father, "path");
        GBDATA *pb_members = GB_create(pb_path, "members", GB_STRING);
        string  path       = generate_path_string(start, end, size);

        GB_write_string(pb_members, path.c_str());
        pb_probes = GB_create(pb_path, "probes", GB_STRING);
        error     = GB_write_string(pb_probes, "");
        created   = true;
    }

    if (error) {
        fprintf(stderr, "Error in groupEntry_recursive: '%s'\n", error);
        return 0;
    }

    return pb_probes;
}

// search/create entry for group
GBDATA *PG_Group::groupEntry(GBDATA *pb_main, bool create, bool& created) {
    GB_transaction  dummy(pb_main);
    GBDATA         *pb_group_tree   = group_tree(pb_main);
    GBDATA         *pb_current_node = pb_group_tree;

    created = false;

    pg_assert(create == true); // groupEntry_recursive always creates missing entries
    GBDATA *pb_probes = groupEntry_recursive(pb_current_node, begin(), end(), size(), created);

    return pb_probes;
}

#if defined(DEBUG)
inline bool is_upper(const char *s) {
    for (int i = 0; s[i]; ++i) {
        if (!isupper(s[i])) return false;
    }
    return true;
}
#endif // DEBUG

// adds probe to group (if not already existing)
bool PG_add_probe(GBDATA *pb_group, const char *probe) {
    GB_ERROR error = 0;
    pg_assert(is_upper(probe));

    const char *existing = GB_read_char_pntr(pb_group);
    const char *exists   = strstr(existing, probe);
    bool        inserted = false;

    if (exists) {
        if (exists != existing) { // not first
            pg_assert(exists[-1] == ',');
        }

        pg_assert(exists[strlen(probe)] == ',' || exists[strlen(probe)] == '\0');
    }
    else {
        string newProbes(existing);
        if (existing[0]) newProbes += ',';
        newProbes += probe;

        error    = GB_write_string(pb_group, newProbes.c_str());
        inserted = true;
    }

    if (error) {
        fprintf(stderr, "error in PG_add_probe: '%s'\n", error);
        pg_assert(0);
    }

    return inserted;
}
