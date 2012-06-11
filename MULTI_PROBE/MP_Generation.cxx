// ============================================================= //
//                                                               //
//   File      : MP_Generation.cxx                               //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "MP_probe.hxx"
#include "MP_externs.hxx"
#include <arb_progress.h>
#include <arb_strarray.h>

void Generation::print() {
    for (int i=0; i<inserted; i++) {
        mp_assert(valid_probe_combi_index(i));
        probe_combi_stat_array[i]->print();
    }
}

void Generation::retrieve_results(ProbeValuation *p_eval, StrArray& results) {
    ProbeValuationResults pvr;
    for (int i=0; i<inserted; i++) {
        mp_assert(valid_probe_combi_index(i));
        if (probe_combi_stat_array[i]) {
            p_eval->insert_in_result_list(probe_combi_stat_array[i], pvr);
        }
    }

    results.erase();

    List<ProbeCombi>& computation_result_list = pvr.get_list();
    
    ProbeCombi *elem = computation_result_list.get_first();
    while (elem) {
        results.put(strdup(elem->get_viewString()));
        elem = computation_result_list.get_next();
    }
    
}

bool Generation::calcFitness(ProbeValuation *p_eval, bool use_genetic_algo, double old_avg_fit, GB_ERROR& error) {
    // returns true if aborted
    //
    // (roulette_wheel wird am Ende neu initialisiert)

    mp_assert(!error);

    arb_progress progress(inserted);
    double       fitness = 0;

    bool aborted = false;

    for (int i=0; i<inserted && !error && !aborted; i++) {
        mp_assert(valid_probe_combi_index(i));
        double dummy = probe_combi_stat_array[i]->calc_fitness(p_eval, mp_gl_awars.no_of_probes, error);
        fitness += dummy;

        if (i==0)
            min_fit = max_fit = dummy;

        if (dummy < min_fit)
            min_fit = dummy;
        else if (dummy > max_fit)
            max_fit = dummy;

        MP_set_progress_subtitle(generation_counter, old_avg_fit, min_fit, max_fit, progress);
        aborted = progress.aborted();
        progress.inc();
    }

    progress.done();

    if (use_genetic_algo && !error && !aborted) {
        average_fitness = fitness / (double)inserted;

        deviation = 0;


#ifdef USE_LINEARSCALING
        double dev = 0;
        double a   = 0;
        double b   = 0;
#ifdef USE_SIGMATRUNCATION
        for (i=0; i<probe_combi_array_length; i++) {        // Berechnung der Abweichung nach Goldberg S.124
            mp_assert(valid_probe_combi_index(i));
            dev = probe_combi_stat_array[i]->get_fitness() - average_fitness;
            dev = dev * dev;
            deviation += dev;
        }
        deviation = (1.0 / (double)((double)i - 1.0)) * deviation;
        deviation = sqrt(deviation);

        for (i=0; i<probe_combi_array_length; i++) {         // sigma_truncation nur auf schlechte Kombis anwenden ???
            mp_assert(valid_probe_combi_index(i));
            probe_combi_stat_array[i]->sigma_truncation(average_fitness, deviation);
        }
#endif
        // lineare Skalierung auf fitness anwenden !!!
        // Skalierung erfolgt nach der Formel     fitness'= a*fitness + b

        prescale(&a, &b);       // Koeffizienten a und b berechnen
#endif

        for (int i=0; i<inserted; i++) {
            mp_assert(valid_probe_combi_index(i));
#ifdef USE_LINEARSCALING
            probe_combi_stat_array[i]->scale(a, b);
#endif
            probe_combi_stat_array[i]->calc_expected_children(average_fitness);
        }

        init_roulette_wheel();
    }
    return aborted;
}

void Generation::prescale(double *a, double *b) { // berechnet Koeffizienten fuer lineare Skalierung
    double delta = 0;

    if ((min_fit > C_MULT * average_fitness - max_fit) / (C_MULT - 1.0)) {  // nach Goldberg S.79
        delta = max_fit - average_fitness;              // Normale Skalierung
        *a = (C_MULT - 1.0) * average_fitness / delta;
        *b = average_fitness * (max_fit - C_MULT * average_fitness) / delta;
    }
    else {                              // Skalieren soweit moeglich
        delta = average_fitness - min_fit;
        *a = average_fitness / delta;
        *b = -min_fit * average_fitness / delta;
    }
}

void Generation::init_roulette_wheel() {
    int     i=0;

    len_roulette_wheel = 0;
    while (i<inserted) {
        mp_assert(valid_probe_combi_index(i));
        len_roulette_wheel += (int)(MULTROULETTEFACTOR * (probe_combi_stat_array[i++]->get_expected_children()));   // um aus z.B. 4,2 42 zu machen
    }
}

probe_combi_statistic *Generation::choose_combi_for_next_generation() {
    int random_help = get_random(0, len_roulette_wheel-1),
                      i;

    for (i=0; i<inserted; i++) {            // in einer Schleife bis zu den betreffenden Elementen vordringen (Rouletterad !!!)
        mp_assert(valid_probe_combi_index(i));
        random_help -= (int)(MULTROULETTEFACTOR * probe_combi_stat_array[i]->get_expected_children());

        if (random_help <= 0) {
            if (probe_combi_stat_array[i]->ok_for_next_gen(len_roulette_wheel))
                return probe_combi_stat_array[i];
            else {
                random_help = get_random(0, len_roulette_wheel-1);
                i = -1;
            }
        }
    }

    return NULL;
}

Generation *Generation::create_next_generation(ProbeValuation *p_eval) {
    Generation *child_generation = new Generation(MAXPOPULATION, generation_counter+1);

    probe_combi_statistic *first_child_pcs  = NULL;
    probe_combi_statistic *second_child_pcs = NULL;

    int cnt = 0;

    while (len_roulette_wheel > 1) {                // kann kleiner sein, wenn Population kleiner als MAXPOPULATION
        cnt++;
        probe_combi_statistic *orig1 = choose_combi_for_next_generation();
        probe_combi_statistic *orig2 = choose_combi_for_next_generation();

        if (! orig1 && ! orig2) break;
        else if (!orig1 && orig2) {
            orig1 = orig2;
            orig2 = NULL;
        }

        delete first_child_pcs;
        delete second_child_pcs;
        first_child_pcs = second_child_pcs = NULL;

        first_child_pcs = orig1->duplicate();
        if (orig2) second_child_pcs = orig2->duplicate();

        if (orig2 && get_random(1, 100) <= CROSSOVER_WS) {  // Crossover durchfueheren
            first_child_pcs->crossover_Probes(second_child_pcs);
            first_child_pcs->init_life_counter();       // wenn  Crossover durchgefuehrt wird, dann Lebensdauer wieder initialisieren, da
            second_child_pcs->init_life_counter();      // sich die Gene veraendert haben
            len_roulette_wheel -= orig1->sub_expected_children(0.5);    // Verfahren nach Goldberg S.115
            len_roulette_wheel -= orig2->sub_expected_children(0.5);
        }
        else {
            first_child_pcs->dec_life_counter();                // Gene gleich geblieben => Lebensdauer verkuerzen
            len_roulette_wheel -= orig1->sub_expected_children(1.0);    // nur tatsaechlich subtrahierte Zahl abziehen !!!

            if (orig2) {
                second_child_pcs->dec_life_counter();
                len_roulette_wheel -= orig2->sub_expected_children(1.0);
            }
        }

        first_child_pcs->mutate_Probe(p_eval);    // fuer jede Position wird mit 1/MUTATION_WS eine Mutation durchgefuehrt.
        if (orig2) second_child_pcs->mutate_Probe(p_eval); // Mutationen durchfuehren

        if (!child_generation->insert(first_child_pcs)) break; // Population schon auf MAXPOPULATION
        if (orig2 && !child_generation->insert(second_child_pcs)) break;
    }

    delete first_child_pcs;
    delete second_child_pcs;

    return child_generation;
}

void Generation::gen_determ_combis(ProbeValuation *p_eval, int beg, int len, int &pos_counter, probe_combi_statistic *p) {
    int     i, j;
    probe_combi_statistic   *bastel_probe_combi;

    if (len == 0) {
        mp_assert(valid_probe_combi_index(pos_counter));
        probe_combi_stat_array[pos_counter++] = p;
        return;
    }

    for (i=beg; i <= p_eval->get_pool_length() - len; i++) {
        bastel_probe_combi = new probe_combi_statistic();

        for (j=0; j < mp_gl_awars.no_of_probes - len; j++)
            bastel_probe_combi->set_probe_combi(j, p->get_probe_combi(j));

        if (len == mp_gl_awars.no_of_probes ||
            (p_eval->get_probe_pool())[i]->probe_index
            !=
            bastel_probe_combi->get_probe_combi(mp_gl_awars.no_of_probes - len - 1)->probe_index) {
            bastel_probe_combi->set_probe_combi(mp_gl_awars.no_of_probes - len, (p_eval->get_probe_pool())[i]);
            gen_determ_combis(p_eval, i+1, len-1, pos_counter, bastel_probe_combi);
        }

        if (len != 1)
            delete bastel_probe_combi;
    }
}

bool Generation::insert(probe_combi_statistic *pcs) {
    if (inserted == MAXPOPULATION)
        return false;

    mp_assert(valid_probe_combi_index(inserted));
    probe_combi_stat_array[inserted++] = pcs->duplicate();

    return true;
}

void Generation::init_valuation(ProbeValuation *p_eval) {
    if (allocated < MAXINITPOPULATION) {
        int pos = 0;
        gen_determ_combis(p_eval, 0, mp_gl_awars.no_of_probes, pos, NULL);  // probe_combi_stat_array ist danach gefuellt !!!
        inserted = pos;
    }
    else {
        int counter = 0;
        probe_combi_statistic *pcs = new probe_combi_statistic();

        while (counter < allocated) {    // Hier erfolgt die Generierung des probe_combi_stat_array
            for (int i=0; i<mp_gl_awars.no_of_probes; i++) {
                int             zw_erg       = get_random(0, p_eval->get_pool_length()-1);
                const ProbeRef *random_probe = (p_eval->get_probe_pool())[zw_erg];
                pcs->set_probe_combi(i, random_probe);
            }

            if (pcs->check_duplicates()) {          // 2 gleiche Sonden in der Kombination => nicht verwendbar
                mp_assert(valid_probe_combi_index(counter));
                probe_combi_stat_array[counter++] = pcs;
                if (counter < allocated) pcs = new probe_combi_statistic();
            }
        }

        inserted = counter;
    }
}

Generation::Generation(int len, int gen_nr)
    : probe_combi_stat_array(new probe_combi_statistic*[len]),
      allocated(len),
      inserted(0),
      average_fitness(0.0), 
      min_fit(0.0), 
      max_fit(0.0), 
      deviation(0.0), 
      len_roulette_wheel(0), 
      generation_counter(gen_nr) 
{
    memset(probe_combi_stat_array, 0, allocated*sizeof(*probe_combi_stat_array));
}

Generation::~Generation() {
    for (int i=0; i<allocated; i++) {
        mp_assert(valid_probe_combi_index(i));
        delete probe_combi_stat_array[i];
    }

    delete [] probe_combi_stat_array;
}
