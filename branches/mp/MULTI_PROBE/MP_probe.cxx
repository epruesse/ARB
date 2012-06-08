// ============================================================= //
//                                                               //
//   File      : MP_probe.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "MP_probe.hxx"
#include "MP_externs.hxx"

#include <aw_window.hxx>
#include <aw_msg.hxx>
#include <aw_select.hxx>
#include <arb_progress.h>

#include <ctime>

GB_ERROR ProbeValuation::evolution(StrArray& results) {
    long n = 0;
    for (int i=0; i<size_sonden_array; i++) {            // Mismatches (=duplikate) aufsummieren, um Groesse von Pool zu bestimmen.
        n += mismatch_array[i]+1;
    }

    long     moeglichkeiten = k_aus_n(mp_gl_awars.no_of_probes, n);
    double   avg_fit        = 0.0;
    GB_ERROR error          = NULL;
    if (moeglichkeiten <= MAXINITPOPULATION) {
        act_generation->calcFitness(this, false, avg_fit, error);
    }
    else {
        // assumption: genetic algorithm needs about 1/3 of attempts (compared with brute force)
        long max_generation = moeglichkeiten/(3*MAXPOPULATION)-1;
        if (max_generation<1) max_generation = 1;

        arb_progress progress(max_generation);
        MP_aborted(0, 0.0, 0.0, 0.0, progress);

        do { // genetic algorithm loop
            bool aborted = act_generation->calcFitness(this, true, avg_fit, error);
            if (aborted) break;

            avg_fit = act_generation->get_avg_fit();
#if defined(DEBUG)
            printf("Generation %i: avg_fit=%f\n", act_generation->get_generation(), avg_fit);
#endif

            if (avg_fit == 0) {
                error = "Please choose better probes";
                break;
            }
            child_generation = act_generation->create_next_generation(this);
            delete act_generation; act_generation = NULL;

            act_generation = child_generation;
            progress.inc();
        }
        while (act_generation->get_generation() <= max_generation);
        progress.done();
    }
    if (act_generation) act_generation->retrieve_results(this, results);
    return error;
}

void ProbeValuation::insert_in_result_list(probe_combi_statistic *pcs, ProbeValuationResults& pvr) {
    // pcs darf nur eingetragen werden, wenn es nicht schon vorhanden ist

    int probe_len = 0;
    for (int i=0; i<mp_gl_awars.no_of_probes; i++) {
        probe_len += strlen(sondenarray[pcs->get_probe_combi(i)->probe_index]) + 1; // 1 fuer space bzw. 0-Zeichen
    }

    char misms[2*mp_gl_awars.no_of_probes+1];
    char probe_string[probe_len+1];

    probe_string[0] = misms[0] = 0;

    char ecoli_pos[40] = { 0 };

    for (int i=0; i<mp_gl_awars.no_of_probes; i++) {
        if (i>0) {
            strcat(misms, " ");
            strcat(probe_string, " ");
        }

        {
            char temp_misms[3];
            sprintf(temp_misms, "%1d", pcs->get_probe_combi(i)->allowed_mismatches);
            strcat(misms, temp_misms);
        }
        {
            char temp_ecol[10];
            sprintf(temp_ecol, "%6d ", pcs->get_probe_combi(i)->e_coli_pos);
            strcat(ecoli_pos, temp_ecol);
        }

        strcat(probe_string, sondenarray[pcs->get_probe_combi(i)->probe_index]);
    }

    size_t ecoli_len = strlen(ecoli_pos);
    ecoli_pos[--ecoli_len] = 0; // erase last char

    ProbeCombi *rs;
    {
        char new_list_string[21 + probe_len + 2*mp_gl_awars.no_of_probes+2 + 2*SEPARATOR_LEN + ecoli_len + 1];

        char buf[25];
        sprintf(buf, "%f", pcs->get_fitness());
        {
            int buf_len = strlen(buf);
            while (buf_len<20) {
                buf[buf_len++] = ' ';
            }
            buf[buf_len] = 0;
        }

        sprintf(new_list_string, "%20s%s%s%s%s%s%s", buf, SEPARATOR, misms, SEPARATOR, ecoli_pos, SEPARATOR, probe_string);

        rs = new ProbeCombi(pcs, new_list_string);
    }

    List<ProbeCombi>& computation_result_list = pvr.get_list();

    ProbeCombi *elem = computation_result_list.get_first();
    if (!elem) {
        computation_result_list.insert_as_first(rs);
    }
    else {
        while (elem) {                  // Liste ist sortiert von groesster Fitness bis kleinster Fitness
            if (elem->has_same_view_string_as(*rs)) { // do not insert duplicate
                delete rs;
                elem = NULL;
                break;
            }

            if (pcs->get_fitness() > elem->get_fitness()) {
                computation_result_list.insert_before_current(rs);
                break;
            }

            elem = computation_result_list.get_next();
        }

        if (!elem) {
            computation_result_list.insert_as_last(rs);
        }
    }
}

GB_ERROR ProbeValuation::evaluate(StrArray& results) {
    int    i, j, k, counter = 0;
    probe *temp_probe;

    if (new_pt_server) {
        new_pt_server = false;

        mp_global->reinit_probe_cache(MAXSONDENHASHSIZE);
    }

    if (pt_server_different) {
        mp_global->clear_probe_cache();
        new_pt_server = true;
        return "PT_server does not match dataset (species differ)";
    }

    if (max_init_pop_combis < MAXINITPOPULATION) {
        for (i=0; i<size_sonden_array; i++)         // generierung eines pools, in dem jede Probe nur einmal pro Mismatch
        {                           // vorkommt, damit alle moeglichen Kombinationen deterministisch
            for (j=0; j<=mismatch_array[i]; j++) {      // generiert werden koennen.
                temp_probe = new probe;
                temp_probe->probe_index = i;
                temp_probe->allowed_mismatches = j;
                temp_probe->e_coli_pos = ecolipos_array[i];

                probe_pool[counter++] = temp_probe;
            }
        }
        pool_length = counter;
    }
    else {
        for (i=0; i<size_sonden_array; i++) {                              // Generierung eines Pools, in dem die Wahrscheinlichkeiten fuer die Erfassung
            for (j=0; j<=mismatch_array[i]; j++)        // der Sonden schon eingearbeitet sind. DIe WS werden vom Benutzer fuer jedE
            {                           // einzelne Probe bestimmt
                for (k=0; k < bewertungarray[i]; k++) {
                    temp_probe = new probe;
                    temp_probe->probe_index = i;
                    temp_probe->allowed_mismatches = j;
                    temp_probe->e_coli_pos = ecolipos_array[i];

                    probe_pool[counter++] = temp_probe;
                }
            }
        }
    }

    act_generation->init_valuation(this);
    GB_ERROR error = evolution(results);

    return error;
}



ProbeValuation::ProbeValuation(char**& sonden_array, int no_of_sonden, int*& bewertung, int*& mismatch, int*& ecoli_pos) {
    memset(this, 0, sizeof(ProbeValuation)); 

    size_sonden_array = no_of_sonden;

    sondenarray    = sonden_array; sonden_array = NULL;
    bewertungarray = bewertung;    bewertung    = NULL;
    mismatch_array = mismatch;     mismatch     = NULL;
    ecolipos_array = ecoli_pos;    ecoli_pos    = NULL;

    for (int i=0; i<size_sonden_array; i++) {           // Mismatches (=duplikate) aufsummieren, um Groesse von Pool zu bestimmen.
        max_init_pop_combis += mismatch_array[i]+1;
        pool_length += (mismatch_array[i]+1) * bewertungarray[i];
    }

    max_init_pop_combis = k_aus_n(mp_gl_awars.no_of_probes, max_init_pop_combis);

    if (max_init_pop_combis > MAXINITPOPULATION)        // Ausgangspopulationsgroesse ist limitiert
        max_init_pop_combis = MAXINITPOPULATION;

    probe_pool = new probe*[pool_length];
    memset(probe_pool, 0, pool_length * sizeof(probe*));    // Struktur mit 0 initialisieren.

    set_act_gen(new Generation(get_max_init_for_gen(), 1)); // erste Generation = Ausgangspopulation
}


ProbeValuation::~ProbeValuation() {
    int i;

    for (i=0; i<size_sonden_array; i++) free(sondenarray[i]);
    for (i=0; i<pool_length; i++) delete probe_pool[i];

    if (act_generation == child_generation) delete act_generation;
    else {
        delete act_generation;
        delete child_generation;
    }

    delete [] probe_pool;
    
    delete [] ecolipos_array;
    delete [] mismatch_array;
    delete [] bewertungarray;
    delete [] sondenarray;
}

