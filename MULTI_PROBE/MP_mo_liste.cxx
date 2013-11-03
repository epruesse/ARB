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
#include <arbdbt.h>
#include <client.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Methoden MO_Liste

GBDATA *MO_Liste::gb_main = NULL;

MO_Liste::MO_Liste() {
    mp_assert(gb_main);
    
    laenge   = 0;
    mo_liste = NULL;
    current  = 0;
    hashptr  = NULL;
    // Nach dem new muss die MO_Liste erst mit fill_all_bakts bzw fill_marked_bakts gefuellt werden
}

MO_Liste::~MO_Liste()
{
    while (laenge)
    {
        delete mo_liste[laenge-1];
        laenge--;
    }
    delete [] mo_liste;
    if (hashptr) GBS_free_hash(hashptr);
}


void MO_Liste::get_all_species() {
    const char *servername = NULL;
    char       *match_name = NULL;
    char        toksep[2];
    char       *probe      = NULL;
    char       *locs_error = 0;
    bytestring  bs;
    int         i          = 0;
    long        j          = 0, nr_of_species;

    if (!(servername=MP_probe_pt_look_for_server())) {
        return;
    }

    GB_ERROR openerr = NULL;
    mp_pd_gl.link    = aisc_open(servername, mp_pd_gl.com, AISC_MAGIC_NUMBER, &openerr);
    mp_pd_gl.locs.clear();
    servername       = 0;

    if (openerr) {
        aw_message(openerr);
        return;
    }

    if (!mp_pd_gl.link) {
        aw_message ("Cannot contact Probe bank server ");
        return;
    }
    if (MP_init_local_com_struct()) {
        aw_message ("Cannot contact Probe bank server (2)");
        // @@@ missing aisc_close
        return;
    }


    if (aisc_put(mp_pd_gl.link, PT_LOCS, mp_pd_gl.locs, NULL))
    {
        free(probe);
        aw_message ("Connection to PT_SERVER lost (4)");
        // @@@ missing aisc_close
        return;
    }


    bs.data = 0;
    aisc_get(mp_pd_gl.link, PT_LOCS, mp_pd_gl.locs,
             LOCS_MP_ALL_SPECIES_STRING, &bs,
             LOCS_MP_COUNT_ALL_SPECIES,  &nr_of_species,
             LOCS_ERROR,                 &locs_error,
             NULL);

    if (*locs_error)
    {
        aw_message(locs_error);
    }

    free(locs_error);

    laenge = nr_of_species;
    mo_liste = new Bakt_Info*[laenge+2];
    while (j<laenge+2)
    {
        mo_liste[j] = NULL;
        j++;
    }
    current = 1;    // ACHTUNG, CURRENT beginnt bei 1, da Hash bei 1 beginnt, d.h. Array[0] ist NULL

    // Initialisieren der Hashtabelle

    hashptr = GBS_create_hash(laenge + 1, GB_IGNORE_CASE);


    toksep[0] = 1;
    toksep[1] = 0;
    if (bs.data)
    {

        match_name = strtok(bs.data, toksep);
        GB_push_transaction(gb_main);
        while (match_name)
        {
            i++;
            if (!GBT_find_species(gb_main, match_name))
            {                               // Testen, ob Bakterium auch im Baum existiert, um
                pt_server_different = true;
                // @@@ missing aisc_close
                return;
            }
            put_entry(match_name);
            match_name = strtok(0, toksep);
        }
        GB_pop_transaction(gb_main);
    }
    else
        aw_message("DB-query produced no species.\n");


    aisc_close(mp_pd_gl.link, mp_pd_gl.com);
    free(bs.data);

    delete match_name;
}



positiontype MO_Liste::fill_marked_bakts()
{
    long    j = 0;
    GBDATA *gb_species;


    GB_push_transaction(gb_main);
    laenge = GBT_count_marked_species(gb_main);     // laenge ist immer zuviel oder gleich der Anzahl wirklick markierter. weil pT-Server nur
    // die Bakterien mit Sequenz zurueckliefert.

    if (!laenge) {
        aw_message("Please mark some species!");
        laenge = 1;
    }
    mo_liste = new Bakt_Info*[laenge+2];

    while (j<laenge+2)
    {
        mo_liste[j] = NULL;
        j++;
    }
    current = 1;    // ACHTUNG, CURRENT beginnt bei 1, da Hash bei 1 beginnt, d.h. Array[0] ist NULL

    hashptr = GBS_create_hash(laenge, GB_IGNORE_CASE);


    for (gb_species = GBT_first_marked_species(gb_main);
          gb_species;
          gb_species = GBT_next_marked_species(gb_species))
    {
        put_entry(GBT_read_name(gb_species));
    }

    GB_pop_transaction(gb_main);

    anz_elem_marked = laenge;

    return laenge;
}



long MO_Liste::get_laenge()
{
    return laenge;
}

long MO_Liste::debug_get_current()
{
    return current;
}

long MO_Liste::put_entry(const char* name) {
    // Pruefe: Gibts den Bakter schon in dieser Liste??
    if (get_index_by_entry(name))               // wanns den Bakter scho gibt
    {
        // nicht eintragen
    }
    else
    {
        mo_liste[current] = new Bakt_Info(name);                    // MEL  koennte mit match_name zusammenhaengen
        GBS_write_hash(hashptr, name, current);
        current++;
    }
    return current;
}

char* MO_Liste::get_entry_by_index(long index)
{
    if ((0<index) && (index < current))
        return mo_liste[index]->get_name();
    else
        return NULL;
}

long MO_Liste::get_index_by_entry(const char* key)
{
    if (key)
        return (GBS_read_hash(hashptr, key));
    else
        return 0;
}

Bakt_Info* MO_Liste::get_bakt_info_by_index(long index)
{
    if ((0<index) && (index < current))
        return mo_liste[index];
    else
        return NULL;
}

