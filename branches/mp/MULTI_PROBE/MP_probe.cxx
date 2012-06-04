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
#include "MultiProbe.hxx"

#include <aw_window.hxx>
#include <aw_msg.hxx>
#include <aw_select.hxx>
#include <arb_progress.h>
#include <arb_strarray.h>

#include <ctime>

GB_ERROR ProbeValuation::evolution(ConstStrArray& results) {
    long n = 0;
    for (int i=0; i<size_sonden_array; i++) {            // Mismatche (=duplikate) aufsummieren, um Groesse von Pool zu bestimmen.
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

            // child_generation->check_for_results(this);  

            act_generation = child_generation;
            progress.inc();
        }
        while (act_generation->get_generation() <= max_generation);
        progress.done();
    }
    if (act_generation) act_generation->check_for_results(this, results);
    return error;
}

void ProbeValuation::insert_in_result_list(probe_combi_statistic *pcs, ConstStrArray& results) {
    // pcs darf nur eingetragen werden, wenn es nicht schon vorhanden ist

    char *new_list_string;
    char *misms;
    char *probe_string;
    char  temp_misms[3];
    int   probe_len = 0;
    char  buf[25];
    char  ecoli_pos[40], temp_ecol[10];
    int   buf_len, i;

    result_struct *rs = new result_struct;
    result_struct *elem;

    memset(rs, 0, sizeof(result_struct));
    memset(ecoli_pos, 0, 40);

    for (i=0; i<mp_gl_awars.no_of_probes; i++)
        probe_len += strlen(sondenarray[pcs->get_probe_combi(i)->probe_index]) + 1; // 1 fuer space bzw. 0-Zeichen

    misms = new char[2*mp_gl_awars.no_of_probes+1];
    probe_string = new char[probe_len+1];

    probe_string[0] = misms[0] = 0;
    for (i=0; i<mp_gl_awars.no_of_probes; i++)
    {
        if (i>0)
        {
            strcat(misms, " ");
            strcat(probe_string, " ");
        }


        sprintf(temp_misms, "%1d", pcs->get_probe_combi(i)->allowed_mismatches);
        strcat(misms, temp_misms);
        sprintf(temp_ecol, "%6d ", pcs->get_probe_combi(i)->e_coli_pos);
        strcat(ecoli_pos, temp_ecol);
        strcat(probe_string, sondenarray[pcs->get_probe_combi(i)->probe_index]);
    }

    ecoli_pos[strlen(ecoli_pos)-1] = 0;

    new_list_string = new char[21+
                               probe_len+
                               2*mp_gl_awars.no_of_probes+2+
                               2*strlen(SEPARATOR) + strlen(ecoli_pos)+
                               1];  // 1 fuer 0-Zeichen

    sprintf(buf, "%f", pcs->get_fitness());
    buf_len = strlen(buf);
    for (i=0; i<20-buf_len; i++)
        strcat(buf, " ");

    sprintf(new_list_string, "%20s%s%s%s%s%s%s", buf, SEPARATOR, misms, SEPARATOR, ecoli_pos, SEPARATOR, probe_string);
    delete [] misms;
    delete [] probe_string;

    rs->ps = pcs->duplicate();
    rs->view_string = new_list_string;
    elem = computation_result_list->get_first();
    if (! elem)
        computation_result_list->insert_as_first(rs);
    else
    {
        while (elem)                    // Liste ist sortiert von groesster Fitness bis kleinster Fitness
        {
            if (strcmp(elem->view_string, new_list_string) == 0)
            {
                delete [] new_list_string;
                delete rs->ps;
                delete rs;
                return;
            }

            if (pcs->get_fitness() > elem->ps->get_fitness())
            {
                computation_result_list->insert_before_current(rs);
                break;
            }

            elem = computation_result_list->get_next();
        }

        if (!elem)
            computation_result_list->insert_as_last(rs);
    }


    // @@@ done for each added element
    // instead just add elements and let caller sort 'results' 

    results.erase();
    elem = computation_result_list->get_first();
    while (elem) {
        results.put(elem->view_string);
        elem = computation_result_list->get_next();
    }
}

GB_ERROR ProbeValuation::initValuation(ConstStrArray& results) {
    int    i, j, k, counter = 0;
    probe *temp_probe;

    if (new_pt_server)
    {
        new_pt_server = false;

        mp_global->reinit_stc(MAXSONDENHASHSIZE);
    }

    if (pt_server_different)
    {
        mp_global->clear_stc();
        new_pt_server = true;
        return "PT_server does not match dataset (species differ)";
    }

    if (max_init_pop_combis < MAXINITPOPULATION) {
        for (i=0; i<size_sonden_array; i++)         // generierung eines pools, in dem jede Sonde nur einmal pro Mismatch
        {                           // vorkommt, damit alle moeglichen Kombinationen deterministisch
            for (j=0; j<=mismatch_array[i]; j++)        // generiert werden koennen.
            {
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
        for (i=0; i<size_sonden_array; i++)
        {                               // Generierung eines Pools, in dem die Wahrscheinlichkeiten fuer die Erfassung
            for (j=0; j<=mismatch_array[i]; j++)        // der Sonden schon eingearbeitet sind. DIe WS werden vom Benutzer fuer jedE
            {                           // einzelne Sonde bestimmt
                for (k=0; k < bewertungarray[i]; k++)
                {
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



ProbeValuation::ProbeValuation(char **sonden_array, int no_of_sonden, int *bewertung, int *mismatch, int *ecoli_pos) {
    memset(this, 0, sizeof(ProbeValuation));

    size_sonden_array = no_of_sonden;

    sondenarray    = sonden_array;
    bewertungarray = bewertung;
    mismatch_array = mismatch;
    ecolipos_array = ecoli_pos;

    computation_result_list = new List<result_struct>;

    for (int i=0; i<size_sonden_array; i++)             // Mismatche (=duplikate) aufsummieren, um Groesse von Pool zu bestimmen.
    {
        max_init_pop_combis += mismatch[i]+1;
        pool_length += (mismatch_array[i]+1) * bewertungarray[i];
    }

    max_init_pop_combis = k_aus_n(mp_gl_awars.no_of_probes, max_init_pop_combis);

    if (max_init_pop_combis > MAXINITPOPULATION)        // Ausgangspopulationsgroesse ist limitiert
        max_init_pop_combis = MAXINITPOPULATION;

    probe_pool = new probe*[pool_length];
    memset(probe_pool, 0, pool_length * sizeof(probe*));    // Struktur mit 0 initialisieren.

}


ProbeValuation::~ProbeValuation()
{
    int i;
    result_struct *elem;

    for (i=0; i<size_sonden_array; i++) free(sondenarray[i]);
    for (i=0; i<pool_length; i++) delete probe_pool[i];

    elem = computation_result_list->get_first();
    while (elem) {
        computation_result_list->remove_first();
        delete [] elem->view_string;
        delete elem;
        elem = computation_result_list->get_first();
    }

    delete computation_result_list;

    if (act_generation == child_generation) delete act_generation;
    else {
        delete act_generation;
        delete child_generation;
    }

    delete [] sondenarray;
    delete [] bewertungarray;
    delete [] mismatch_array;
    delete [] probe_pool;
}

