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
#include <client.h>

#include <algorithm>

Probe::Probe(char* bezeichner, int allowed_mis, double outside_mis)
    : minelem(0), 
      maxelem(0)
{
    kennung = strdup(bezeichner);
    bitkennung = NULL;
    // fuer Basissonden haben die Bitvektoren noch nicht die Volle laenge, da noch nicht bekannt ist, wieviele Sonden eingetragen werden
    hitliste = NULL;
    length_hitliste = 0;
    int anzahl_sonden = mp_gl_awars.no_of_probes;

    Allowed_Mismatch = new long[anzahl_sonden];
    Outside_Mismatch = new double[anzahl_sonden];
    for (int i=0; i<anzahl_sonden; i++) {
        Allowed_Mismatch[i]=0;
        Outside_Mismatch[i]=0;
    }

    Allowed_Mismatch[0] = allowed_mis;
    Outside_Mismatch[0] = outside_mis;


}

Probe::~Probe() {
    free(kennung);

    for (int i=0; i<length_hitliste; i++) {
        delete hitliste[i];
    }
    delete [] hitliste;

    delete [] Allowed_Mismatch;
    delete [] Outside_Mismatch;
    delete bitkennung;
}

void Probe::print() {
    printf("\nSonde %s\n------------------------------------------------\n", kennung);
    bitkennung->print();
    printf("Laenge hitliste %ld mit minelem %d und maxelem %d\n", length_hitliste, minelem.base(), maxelem.base());
    printf("Far %ld, Mor %ld, AllMM %ld, OutMM %f\n\n", kombi_far, kombi_mor, *Allowed_Mismatch, *Outside_Mismatch);
}


MO_Mismatch** Probe::get_matching_species(bool match_kompl, int match_weight, int match_mis, char *match_seq, const TargetGroup& targetGroup, int *number_of_species, GB_ERROR& error) {
    mp_assert(!error);

    MO_Mismatch **ret_list   = NULL;
    const char   *servername = MP_probe_pt_look_for_server(error);

    if (servername) {
        char           *match_name, *match_mismatches, *match_wmismatches;
        char            toksep[2];
        T_PT_MATCHLIST  match_list;
        char           *locs_error;
        long            match_list_cnt;
        bytestring      bs;
        int             i        = 0;

        mp_pd_gl.link = aisc_open(servername, mp_pd_gl.com, AISC_MAGIC_NUMBER);
        mp_pd_gl.locs.clear();

        if (!mp_pd_gl.link) {
            error = "cannot contact PT-Server";
        }
        if (!error && MP_init_local_com_struct()) {
            error = "Connection to PT_SERVER lost (1)";
        }

        if (!error &&
            aisc_put(mp_pd_gl.link, PT_LOCS, mp_pd_gl.locs,
                     LOCS_MATCH_REVERSED,       match_kompl, // Komplement
                     LOCS_MATCH_SORT_BY,        match_weight, // Weighted
                     LOCS_MATCH_COMPLEMENT,     0,  // ???
                     LOCS_MATCH_MAX_MISMATCHES, match_mis, // Mismatches
                     LOCS_SEARCHMATCH,          match_seq, // Sequence
                     NULL)) {
            error = "Connection to PT_SERVER lost (2)";
        }

        if (!error) {
            bs.data = 0;
            aisc_get(mp_pd_gl.link, PT_LOCS, mp_pd_gl.locs,
                     LOCS_MATCH_LIST,      match_list.as_result_param(),
                     LOCS_MATCH_LIST_CNT,  &match_list_cnt,
                     LOCS_MP_MATCH_STRING, &bs,
                     LOCS_ERROR,           &locs_error,
                     NULL);

            if (*locs_error) {
                error = GBS_global_string("%s", locs_error);
            }
            free(locs_error);
        }

        if (!error) {
            toksep[0] = 1;
            toksep[1] = 0;
            if (bs.data) {
                ret_list = new MO_Mismatch*[match_list_cnt];

                match_name        = strtok(bs.data, toksep);
                match_mismatches  = strtok(0, toksep);
                match_wmismatches = strtok(0, toksep);

                while (match_name && match_mismatches && match_wmismatches) {
                    ret_list[i] = new MO_Mismatch;
                    ret_list[i]->nummer = targetGroup.name2id(match_name);
                    if (match_weight == NON_WEIGHTED) {
                        ret_list[i]->mismatch = atof(match_mismatches);
                    }
                    else { // WEIGHTED und WEIGHTED_PLUS_POS
                        ret_list[i]->mismatch = atof(match_wmismatches);
                    }

                    match_name        = strtok(0, toksep);
                    match_mismatches  = strtok(0, toksep);
                    match_wmismatches = strtok(0, toksep);

                    i++;
                }
            }
            else {
                error = "No matching species found"; // @@@ more details ?
            }
        }

        *number_of_species = match_list_cnt;

        aisc_close(mp_pd_gl.link, mp_pd_gl.com);
        free(bs.data);

    }

    return ret_list;
}


double Probe::check_for_min(long k, MO_Mismatch** probebacts, long laenge) {
    long    i = k+1;
    double  min;

    min = probebacts[k]->mismatch;                  // min ist gleich mismatch des ersten MOs
    while ((i<laenge) && (probebacts[k]->nummer == probebacts[i]->nummer)) {
        if (min > probebacts[i]->mismatch)              // wenn min groesser ist als mismatch des naechsten MOs
            min = probebacts[i]->mismatch;              // setze min uf groesse des naechsten
        i++;                                // checke naechsten MO
    }
    return min;
}

GB_ERROR Probe::gen_Hitliste(const TargetGroup& targetGroup) {
    // Angewandt auf eine frische Probe generiert diese Methode die Hitliste durch eine Anfrage an die Datenbank, wobei
    // der Name der Probe uebergeben wird

    int mm_int_to_search;
    {
        double mm_to_search = mp_gl_awars.greyzone + mp_gl_awars.outside_mismatches_difference + get_Allowed_Mismatch_no(0);
        mm_int_to_search    = int(mm_to_search);
        if (mm_to_search>mm_int_to_search) mm_int_to_search++;
    }

    GB_ERROR error  = NULL;
    int      laenge = 0;

    MO_Mismatch** probebacts =
        get_matching_species(mp_gl_awars.complement,
                             mp_gl_awars.weightedmismatches,
                             mm_int_to_search,
                             kennung,
                             targetGroup,
                             &laenge,
                             error);

    mp_assert(contradicted(error, laenge && probebacts));

    if (!error) {
        // Ptrliste ist Nullterminiert
        // Sortieren des Baktnummernfeldes:

        heapsort(laenge, probebacts);

        double min_mm;          // Minimaler Mismatch
        // laenge ist die Anzahl der Eintraege in probebact
        // Korrekturschleife, um Mehrfachtreffer auf das gleiche Bakterium abzufangen

        for (int k=0; k<laenge-1; k++) {
            if (probebacts[k]->nummer == probebacts[k+1]->nummer) {
                min_mm = check_for_min(k, probebacts, laenge);
                probebacts[k]->mismatch = min_mm;
                while ((k<laenge-1) && (probebacts[k]->nummer == probebacts[k+1]->nummer)) {
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

        for (int i=0; i<laenge; i++) {
            hitliste[i] = new Hit(probebacts[i]->nummer);
            hitliste[i]->set_mismatch_at_pos(0, probebacts[i]->mismatch);
        }
        hitliste[laenge] = NULL;
        length_hitliste  = laenge;

        // Loeschen der Temps
        for (int i=0; i<laenge; i++) delete probebacts[i];
        delete [] probebacts;
    }
    return error;
}

Hit *Probe::hit(long index) {
    mp_assert(hitliste);
    mp_assert(index < length_hitliste);
    return hitliste[index];
}

void Probe::heapsort(long feldlaenge, MO_Mismatch** Nr_Mm_Feld) {
    // Heapsortfunktion, benutzt sink(), sortiert Feld von longs

    long max_index = feldlaenge-1;

    for (long i=max_index/2; i>-1; i--) {
        sink(i, max_index, Nr_Mm_Feld);
    }
    for (long m=max_index; m>0; m--) {
        std::swap(Nr_Mm_Feld[0], Nr_Mm_Feld[m]);
        sink(0, m-1, Nr_Mm_Feld);
    }
}

void Probe::sink(long i, long t, MO_Mismatch** A) {
    // Algorithmus fuer den Heapsort

    long j = 2*i;
    long k = j+1;
    if (j <= t) {
        if (A[i]->nummer >= A[j]->nummer) j = i;
        if (k <= t && A[k]->nummer > A[j]->nummer) j = k;
        if (i != j) {
            std::swap(A[i], A[j]);
            sink(j, t, A);
        }
    }
}

void Probe::set_bitkennung(Bitvector* bv) {
    bitkennung = bv;
}

Hit::Hit(SpeciesID id_)
    : id(id_)
{
    // Mismatch Array mit Laenge = anzahl Sonden in Experiment
    mismatch = new double[mp_gl_awars.no_of_probes+1];
    for (int i=0; i<mp_gl_awars.no_of_probes+1; i++) mismatch[i]=101;
}

Hit::~Hit() {
    delete [] mismatch;
}

