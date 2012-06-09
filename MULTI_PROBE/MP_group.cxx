// ============================================================ //
//                                                              //
//   File      : MP_group.cxx                                   //
//   Purpose   :                                                //
//                                                              //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "MP_externs.hxx"
#include "MultiProbe.hxx"

#include <aw_msg.hxx>
#include <client.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Methoden Group


#if defined(DEBUG)
#define DUMP_SET_SPECIES
#endif

template <typename ID>
static void add_to_MO_Liste(const char *species_name, long, void *cl_MO_Liste) {
    Group<ID> *mo = (Group<ID>*)cl_MO_Liste;
    mo->put_entry(species_name);
#if defined(DUMP_SET_SPECIES)
    printf("%s,", species_name);
#endif
}

template <typename ID>
void Group<ID>::insert_species(const GB_HASH *species_hash) {
    mp_assert(species_hash);
    mp_assert(!hashptr); // insert_species called twice!

    long count = GBS_hash_count_elems(species_hash);

#if defined(DUMP_SET_SPECIES)
    printf("insert_species:\"");
#endif

    if (count) {
        allocate(count);
        GBS_hash_do_const_loop(species_hash, add_to_MO_Liste<ID>, this);
    }
#if defined(DUMP_SET_SPECIES)
    printf("\"\n");
#endif
}

template <typename ID>
GB_ERROR Group<ID>::insert_species_knownBy_PTserver() {
    GB_ERROR error = NULL;

    const char *servername = MP_probe_pt_look_for_server(error);
    if (servername) {
        mp_pd_gl.link = aisc_open(servername, mp_pd_gl.com, AISC_MAGIC_NUMBER);
        mp_pd_gl.locs.clear();

        if (!mp_pd_gl.link) {
            error = "Cannot contact PT-server [1]";
        }
        else {
            if (MP_init_local_com_struct()) {
                error = "Cannot contact PT-server [2]";
            }
            else if (aisc_put(mp_pd_gl.link, PT_LOCS, mp_pd_gl.locs, NULL)) {
                error = "Connection to PT_SERVER lost [1]";
            }
            else {
                bytestring  bs; bs.data = NULL;
                long        nr_of_species;
                char       *locs_error;

                aisc_get(mp_pd_gl.link, PT_LOCS, mp_pd_gl.locs,
                         LOCS_MP_ALL_SPECIES_STRING, &bs,
                         LOCS_MP_COUNT_ALL_SPECIES,  &nr_of_species,
                         LOCS_ERROR,                 &locs_error,
                         NULL);

                if (*locs_error) {
                    aw_message(locs_error);
                }

                free(locs_error);

                allocate(nr_of_species);

                if (bs.data) {
                    const GB_HASH *unmarked_species = mp_global->get_unmarked_species();
                    const GB_HASH *marked_species   = mp_global->get_marked_species();

                    char toksep[2] = {1,0};
                    const char *match_name = strtok(bs.data, toksep);
                    while (match_name) {
                        bool species_in_DB = GBS_read_hash(marked_species, match_name) || GBS_read_hash(unmarked_species, match_name);
                        if (!species_in_DB) {
                            error = "PT-server contains species which are not in database";
                            break;
                        }
                        put_entry(match_name);
                        match_name = strtok(0, toksep);
                    }

                    long species_in_DB_count = GBS_hash_count_elems(unmarked_species)+GBS_hash_count_elems(marked_species);
                    if (species_in_DB_count>nr_of_species) {
                        error = "database contains species which are not in PT-server";
                    }
                }
                else {
                    error = "PT-server query reported no species";
                }

                free(bs.data);
            }
            aisc_close(mp_pd_gl.link, mp_pd_gl.com);
        }
    }
    return error;
}

TargetGroup::TargetGroup(const GB_HASH *targeted_species, GB_ERROR& error) {
    error = known.insert_species_knownBy_PTserver();
    targeted.insert_species(targeted_species);
}
