// ============================================================= //
//                                                               //
//   File      : MP_sonde.cxx                                    //
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

#include <cmath>

Sonde::Sonde(char* bezeichner, int allowed_mis, double outside_mis)
{
    kennung = strdup(bezeichner);
    bitkennung = NULL;
    // fuer Basissonden haben die Bitvektoren noch nicht die Volle laenge, da noch nicht bekannt ist, wieviele Sonden eingetragen werden
    hitliste = NULL;
    length_hitliste = 0;
    minelem = 0;
    maxelem = 0;
    int anzahl_sonden = mp_gl_awars.no_of_probes;

    Allowed_Mismatch = new long[anzahl_sonden];
    Outside_Mismatch = new double[anzahl_sonden];
    for (int i=0; i<anzahl_sonden; i++)
    {
        Allowed_Mismatch[i]=0;
        Outside_Mismatch[i]=0;
    }

    Allowed_Mismatch[0] = allowed_mis;
    Outside_Mismatch[0] = outside_mis;


}

Sonde::~Sonde()
{
    int i;

    free(kennung);

    for (i=0; i<length_hitliste;  i++) {
        delete hitliste[i];             // Hits loeschen
    }
    delete [] hitliste;

    delete [] Allowed_Mismatch;
    delete [] Outside_Mismatch;
    delete bitkennung;
}

void Sonde::print()
{
    printf("\nSonde %s\n------------------------------------------------\n", kennung);
    bitkennung->print();
    printf("Laenge hitliste %ld mit minelem %ld und maxelem %ld\n", length_hitliste, minelem, maxelem);
    printf("Far %ld, Mor %ld, AllMM %ld, OutMM %f\n\n", kombi_far, kombi_mor, *Allowed_Mismatch, *Outside_Mismatch);
}


MO_Mismatch** Sonde::get_matching_species(bool match_kompl, int match_weight, int match_mis, char *match_seq, MO_Liste *convert, long *number_of_species)
{
    MO_Mismatch **ret_list   = NULL;
    const char   *servername = MP_probe_pt_look_for_server();

    if (servername) {
        char           *match_name, *match_mismatches, *match_wmismatches;
        char            toksep[2];
        T_PT_MATCHLIST  match_list;
        char           *probe      = NULL;
        char           *locs_error = 0;
        long            match_list_cnt;
        bytestring      bs;
        int             i          = 0;

        GB_ERROR openerr = NULL;
        mp_pd_gl.link    = aisc_open(servername, mp_pd_gl.com, AISC_MAGIC_NUMBER, &openerr);
        mp_pd_gl.locs.clear();

        if (openerr) {
            aw_message(openerr);
            return NULL;
        }
        if (!mp_pd_gl.link) {
            aw_message("Cannot contact Probe bank server ");
            return NULL;
        }
        if (MP_init_local_com_struct()) {
            aw_message ("Cannot contact Probe bank server (2)");
            return NULL;
        }

        if (aisc_put(mp_pd_gl.link, PT_LOCS, mp_pd_gl.locs,
                     LOCS_MATCH_REVERSED,       match_kompl, // Komplement
                     LOCS_MATCH_SORT_BY,        match_weight, // Weighted
                     LOCS_MATCH_COMPLEMENT,     0,  // ???
                     LOCS_MATCH_MAX_MISMATCHES, match_mis, // Mismatches
                     LOCS_SEARCHMATCH,          match_seq, // Sequence
                     NULL)) {
            free(probe);
            aw_message ("Connection to PT_SERVER lost (4)");
            return NULL;
        }

        bs.data = 0;
        aisc_get(mp_pd_gl.link, PT_LOCS, mp_pd_gl.locs,
                 LOCS_MATCH_LIST,      match_list.as_result_param(),
                 LOCS_MATCH_LIST_CNT,  &match_list_cnt,
                 LOCS_MP_MATCH_STRING, &bs,
                 LOCS_ERROR,           &locs_error,
                 NULL);

        if (locs_error[0]) aw_message(locs_error);
        free(locs_error);

        toksep[0] = 1;
        toksep[1] = 0;
        if (bs.data)
        {
            ret_list = new MO_Mismatch*[match_list_cnt];

            match_name = strtok(bs.data, toksep);
            match_mismatches = strtok(0, toksep);
            match_wmismatches = strtok(0, toksep);

            while (match_name && match_mismatches && match_wmismatches)
            {
                ret_list[i] = new MO_Mismatch;
                ret_list[i]->nummer = convert->get_index_by_entry(match_name);
                if (match_weight == NON_WEIGHTED)
                    ret_list[i]->mismatch = atof(match_mismatches);
                else                            // WEIGHTED und WEIGHTED_PLUS_POS
                    ret_list[i]->mismatch = atof(match_wmismatches);


                match_name = strtok(0, toksep);
                match_mismatches = strtok(0, toksep);
                match_wmismatches = strtok(0, toksep);

                i++;
            }
        }
        else
            aw_message("No matching species found.");

        *number_of_species = match_list_cnt;

        aisc_close(mp_pd_gl.link, mp_pd_gl.com);
        free(bs.data);

    }
    return ret_list;
}


double Sonde::check_for_min(long k, MO_Mismatch** probebacts, long laenge)
{
    long    i = k+1;
    double  min;

    min = probebacts[k]->mismatch;                  // min ist gleich mismatch des ersten MOs
    while ((i<laenge) && (probebacts[k]->nummer == probebacts[i]->nummer))
    {
        if (min > probebacts[i]->mismatch)              // wenn min groesser ist als mismatch des naechsten MOs
            min = probebacts[i]->mismatch;              // setze min uf groesse des naechsten
        i++;                                // checke naechsten MO
    }
    return min;
}



int Sonde::gen_Hitliste(MO_Liste *Bakterienliste)
    // Angewandt auf eine frische Sonde generiert diese Methode die Hitliste durch eine Anfrage an die Datenbank, wobei
    // der Name der Sonde uebergeben wird
{
    MO_Mismatch**   probebacts;
    long        i, k;           // Zaehlervariable
    long        laenge = 0;
    double      mm_to_search = 0;
    int         mm_int_to_search = 0;


    // DATENBANKAUFRUF
    mm_to_search = mp_gl_awars.greyzone + mp_gl_awars.outside_mismatches_difference + get_Allowed_Mismatch_no(0);
    if (mm_to_search > (int) mm_to_search)
        mm_int_to_search = (int) mm_to_search + 1;
    else
        mm_int_to_search = (int) mm_to_search;

    probebacts = get_matching_species(mp_gl_awars.complement,
                                      mp_gl_awars.weightedmismatches,
                                      mm_int_to_search,
                                      kennung,
                                      Bakterienliste,
                                      &laenge);


    // ACHTUNG probebacts mit laenge enthaelt nur laenge-1 Eintraege von 0 bis laenge -2
    if (!laenge || !probebacts)
    {
        if (!laenge)
            aw_message("This probe matches no species!");
        if (!probebacts)
        {
            aw_message("This probe matches no species!");
            return 11;
        }
        return 1;
    }

    // Ptrliste ist Nullterminiert
    // Sortieren des Baktnummernfeldes:

    heapsort(laenge, probebacts);

    double min_mm;          // Minimaler Mismatch
    // laenge ist die Anzahl der Eintraege in probebact
    // Korrekturschleife, um Mehrfachtreffer auf das gleiche Bakterium abzufangen

    for (k=0;  k < laenge-1;  k++)
    {
        if (probebacts[k]->nummer == probebacts[k+1]->nummer)
        {
            min_mm = check_for_min(k, probebacts, laenge);
            probebacts[k]->mismatch = min_mm;
            while ((k<laenge-1) && (probebacts[k]->nummer == probebacts[k+1]->nummer))
            {
                probebacts[k+1]->mismatch = min_mm;
                k++;
            }
        }
    }

    // Das hier funktioniert, da Liste sortiert ist
    minelem = probebacts[0]->nummer;
    maxelem = probebacts[laenge-1]->nummer;

    // Probebacts besteht aus eintraegen der Art (Nummer, Mismatch)
    hitliste = new Hit*[laenge+1];
    for (i=0; i<laenge+1; i++)
        hitliste[i]=NULL;

    for (i=0; i<laenge; i++)
    {
        hitliste[i] = new Hit(probebacts[i]->nummer);
        hitliste[i]->set_mismatch_at_pos(0, probebacts[i]->mismatch);
    }
    length_hitliste = laenge;

    // Loesche hitflags wieder
    long bl_index = 0;
    Bakt_Info** baktliste = Bakterienliste->get_mo_liste();
    Bakt_Info** bl_elem = baktliste+1;
    while (bl_elem[bl_index])
    {
        bl_elem[bl_index]->kill_flag();
        bl_index++;
    }
    // Loeschen der Temps
    for (i=0; i<laenge; i++)
    {
        delete probebacts[i];
    }
    delete [] probebacts;
    return 0;
}



Hit* Sonde::get_hitdata_by_number(long index)
{
    // Gibt Zeiger auf ein Hit Element zurueck, welches an Stelle index steht, vorerst nur zur Ausgabe gedacht
    if (hitliste && (index < length_hitliste))
        return hitliste[index];
    else
        return NULL;
}




void Sonde::heapsort(long feldlaenge, MO_Mismatch** Nr_Mm_Feld)
    // Heapsortfunktion, benutzt sink(), sortiert Feld von longs
{
    long        m=0, i=0;
    MO_Mismatch*    tmpmm;

    for (i=(feldlaenge-1)/2; i>-1; i--)
        sink(i, feldlaenge-1, Nr_Mm_Feld);
    for (m=feldlaenge-1; m>0; m--)
    {
        tmpmm =  Nr_Mm_Feld[0];
        Nr_Mm_Feld[0] =  Nr_Mm_Feld[m];
        Nr_Mm_Feld[m] = tmpmm;

        sink(0, m-1, Nr_Mm_Feld);
    }
}

void Sonde::sink(long i, long t, MO_Mismatch** A)
    // Algorithmus fuer den Heapsort
{
    long        j, k;
    MO_Mismatch*    tmpmm;

    j = 2*i;
    k = j+1;
    if (j <= t)
    {
        if (A[i]->nummer >= A[j]->nummer)
            j = i;
        if (k <= t)
            if (A[k]->nummer > A[j]->nummer)
                j = k;
        if (i != j)
        {
            tmpmm = A[i]; A[i] = A[j]; A[j] = tmpmm;
            sink(j, t, A);
        }
    }
}

void Sonde::set_bitkennung(Bitvector* bv)
{
    bitkennung = bv;
}



// ########################################################################################################
/* Bakt_Info haengt in der MO_Liste drinnen. Hier werden u.a. die Hitflags gespeichert
 */
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Methoden Bakt_Info~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Bakt_Info::Bakt_Info(const char* n)
{
    name = strdup(n);                       // MEL  (match_name in mo_liste)
    hit_flag = 0;
}

Bakt_Info::~Bakt_Info()
{
    free(name);
    hit_flag = 0;
}


// ##########################################################################################################
// Hit speichert die  Trefferinformation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~Methoden Hit

Hit::Hit(long baktnummer)
{
    // Mismatch Array mit Laenge = anzahl Sonden in Experiment
    int i=0;
    mismatch = new double[mp_gl_awars.no_of_probes+1];
    for (i=0; i<mp_gl_awars.no_of_probes+1; i++)
        mismatch[i]=101;

    baktid = baktnummer;
}

Hit::~Hit()
{
    delete [] mismatch;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
