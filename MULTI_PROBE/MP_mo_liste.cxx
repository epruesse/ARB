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

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Methoden MO_Liste

GBDATA *MO_Liste::gb_main = NULL;

MO_Liste::MO_Liste() {
    laenge   = 0;
    mo_liste = NULL;
    current  = 0;
    hashptr  = NULL;
    // Nach dem new muss die MO_Liste erst mit set_species gefuellt werden
}

MO_Liste::~MO_Liste() {
    while (laenge) {
        delete mo_liste[laenge-1];
        laenge--;
    }
    delete [] mo_liste;
    if (hashptr) GBS_free_hash(hashptr);
}

#if defined(DEBUG)
#define DUMP_SET_SPECIES
#endif


static void add_to_MO_Liste(const char *species_name, long, void *cl_MO_Liste) {
    MO_Liste *mo = (MO_Liste*)cl_MO_Liste;
    mo->put_entry(species_name);
#if defined(DUMP_SET_SPECIES)
    printf("%s,", species_name);
#endif
}

void MO_Liste::set_species(const GB_HASH *species_hash) {
    mp_assert(species_hash);
    mp_assert(!hashptr); // set_species called twice!

    laenge = GBS_hash_count_elems(species_hash);

#if defined(DUMP_SET_SPECIES)
    printf("set_species:\"");
#endif

    if (laenge) {
        mo_liste = new Bakt_Info*[laenge+2];
        memset(mo_liste, 0, (laenge+2)*sizeof(*mo_liste));
        current  = 1;

        hashptr = GBS_create_hash(laenge, GB_IGNORE_CASE);
        GBS_hash_do_const_loop(species_hash, add_to_MO_Liste, this);
    }
#if defined(DUMP_SET_SPECIES)
    printf("\"\n");
#endif
}

void MO_Liste::get_all_species() { // @@@ rename (gets all species from pt-server)
    const char *servername = NULL;
    char       *match_name = NULL;
    char        toksep[2];
    char       *probe      = NULL;
    char       *locs_error;
    bytestring  bs;
    int         i          = 0;
    long        j          = 0, nr_of_species;

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

    laenge = nr_of_species;
    mo_liste = new Bakt_Info*[laenge+2];
    while (j<laenge+2) {
        mo_liste[j] = NULL;
        j++;
    }
    current = 1;    // ACHTUNG, CURRENT beginnt bei 1, da Hash bei 1 beginnt, d.h. Array[0] ist NULL

    // Initialisieren der Hashtabelle

    hashptr = GBS_create_hash(laenge + 1, GB_IGNORE_CASE);

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

long MO_Liste::get_laenge() { // @@@ inline
    return laenge;
}

long MO_Liste::debug_get_current() { // @@@ inline
    return current;
}

long MO_Liste::put_entry(const char* name) {
    // Pruefe: Gibts den Bakter schon in dieser Liste??
    if (get_index_by_entry(name)) {             // wanns den Bakter scho gibt
        // nicht eintragen
    }
    else {
        mo_liste[current] = new Bakt_Info(name);                    // MEL  koennte mit match_name zusammenhaengen
        GBS_write_hash(hashptr, name, current);
        current++;
    }
    return current;
}

char* MO_Liste::get_entry_by_index(long index) {
    if ((0<index) && (index < current))
        return mo_liste[index]->get_name();
    else
        return NULL;
}

long MO_Liste::get_index_by_entry(const char* key) {
    if (key)
        return (GBS_read_hash(hashptr, key));
    else
        return 0;
}

Bakt_Info* MO_Liste::get_bakt_info_by_index(long index) {
    if ((0<index) && (index < current))
        return mo_liste[index];
    else
        return NULL;
}

