// ============================================================= //
//                                                               //
//   File      : MP_mo_liste.cxx                                 //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "MP_externs.hxx"
#include "MultiProbe.hxx"

#include <aw_msg.hxx>
#include <client.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Methoden Group


#if defined(DEBUG)
#define DUMP_SET_SPECIES
#endif


static void add_to_MO_Liste(const char *species_name, long, void *cl_MO_Liste) {
    Group *mo = (Group*)cl_MO_Liste;
    mo->put_entry(species_name);
#if defined(DUMP_SET_SPECIES)
    printf("%s,", species_name);
#endif
}

void Group::insert_species(const GB_HASH *species_hash) {
    mp_assert(species_hash);
    mp_assert(!hashptr); // insert_species called twice!

    long count = GBS_hash_count_elems(species_hash);

#if defined(DUMP_SET_SPECIES)
    printf("insert_species:\"");
#endif

    if (count) {
        allocate(count);
        GBS_hash_do_const_loop(species_hash, add_to_MO_Liste, this);
    }
#if defined(DUMP_SET_SPECIES)
    printf("\"\n");
#endif
}

void Group::insert_species_knownBy_PTserver() {
    const char *servername = NULL;
    char       *match_name = NULL;
    char        toksep[2];
    char       *probe      = NULL;
    char       *locs_error;
    bytestring  bs;
    int         i          = 0;
    long        nr_of_species;

    GB_ERROR error = NULL; // @@@ collect all aw_messages here and return as result -> use that result everywhere
    if (!(servername=MP_probe_pt_look_for_server(error))) {
        return;
    }

    mp_pd_gl.link = aisc_open(servername, mp_pd_gl.com, AISC_MAGIC_NUMBER);
    mp_pd_gl.locs.clear();
    servername = 0;

    if (!mp_pd_gl.link) {
        aw_message("Cannot contact Probe bank server ");
        return;
    }
    if (MP_init_local_com_struct()) {
        aw_message("Cannot contact Probe bank server (2)");
        // @@@ missing aisc_close
        return;
    }


    if (aisc_put(mp_pd_gl.link, PT_LOCS, mp_pd_gl.locs, NULL)) {
        free(probe);
        aw_message("Connection to PT_SERVER lost (4)");
        // @@@ missing aisc_close
        return;
    }


    bs.data = 0;
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

    toksep[0] = 1;
    toksep[1] = 0;

    if (bs.data) {
        const GB_HASH *unmarked_species = mp_global->get_unmarked_species();
        const GB_HASH *marked_species   = mp_global->get_marked_species();

        match_name = strtok(bs.data, toksep);
        while (match_name) {
            i++;
            bool species_in_DB = GBS_read_hash(marked_species, match_name) || GBS_read_hash(unmarked_species, match_name);
            if (!species_in_DB) {
                pt_server_different = true;
                // @@@ missing aisc_close
                return;
            }
            put_entry(match_name);
            match_name = strtok(0, toksep);
        }
    }
    else aw_message("DB-query produced no species.\n");

    aisc_close(mp_pd_gl.link, mp_pd_gl.com);
    free(bs.data);

    delete match_name;
}

