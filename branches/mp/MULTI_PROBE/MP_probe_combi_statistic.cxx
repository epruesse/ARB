// ============================================================= //
//                                                               //
//   File      : MP_probe_combi_statistic.cxx                    //
//   Purpose   :                                                 //
//                                                               //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "MP_probe.hxx"
#include <cmath>

probe_combi_statistic::probe_combi_statistic(const ProbeRef **pc, probe_tabs *ps, double exp, double fit, int life_cnt) {
    memset(this, 0, sizeof(probe_combi_statistic));

    if (ps)
        probe_tab = ps;
    else
        probe_tab = NULL; // new probe_tabs

    probe_combi = new const ProbeRef*[mp_gl_awars.no_of_probes];
    if (pc) {
        for (int i=0; i < mp_gl_awars.no_of_probes; i++)
            probe_combi[i] = pc[i];
    }
    else
        memset(probe_combi, 0, mp_gl_awars.no_of_probes * sizeof(ProbeRef*));


    expected_children = exp;
    fitness = fit;
    life_counter = life_cnt;
}

probe_combi_statistic::~probe_combi_statistic() {
    delete [] probe_combi;
    delete probe_tab;
}

bool probe_combi_statistic::ok_for_next_gen(int &len_roul_wheel) {
    double exp_child = get_expected_children();

    if (exp_child >= 1.0 || get_random(1, 100) <= 100 * exp_child) {    // Behandlung nach Goldberg S.115 bzw. S.121
        if (!is_dead())
            return true;
        else {
            len_roul_wheel -= (int)(MULTROULETTEFACTOR * get_expected_children());
            expected_children = 0.0;
        }
    }
    return false;
}

void probe_combi_statistic::randomized_quicksort(long left, long right) {
    long i = left, j = right;
    int  x, help, help2;

    if (j>i) {
        help = (left + right) / 2;

        // Randomisierung des Quicksort Anfang
        // Falls keine Randomisierung erwuenscht, einfach diesen Teil auskommentieren !!!
        help2 = get_random(left, right);
        swap(probe_combi[help2], probe_combi[help]);
        // Randomisierung des Quicksort Ende

        x = probe_combi[help]->probe_index;         // Normale Auswahl des Pivotelements

        do {
            while (probe_combi[i]->probe_index < x) i++;
            while (probe_combi[j]->probe_index > x) j--;

            if (i<=j) {
                swap(probe_combi[i], probe_combi[j]);

                i++;
                j--;
            }
        }
        while (i<=j) ;

        randomized_quicksort(left, j);
        randomized_quicksort(i, right);
    }
}

probe_combi_statistic *probe_combi_statistic::check_duplicates() {
    sort(mp_gl_awars.no_of_probes);
    return (get_dupl_pos() == -1) ? this : NULL;
}


void probe_combi_statistic::print(const ProbeRef *p) {
    printf("Idx:%d alMis:%d  ", p->probe_index, p->allowed_mismatches);
}

void probe_combi_statistic::print() {
    for (int i=0; i<mp_gl_awars.no_of_probes; i++)
        print(probe_combi[i]);

    printf("Fit:%f Expchil: %f\n", fitness, expected_children);
    probe_tab->print();
}

void probe_combi_statistic::print(const ProbeRef **arr, int length) {
    for (int i=0; i<length; i++)
        print(arr[i]);
}

int probe_combi_statistic::sub_expected_children(double val) {
    double result;

    if (expected_children - val > 0)
        result = (double)MULTROULETTEFACTOR * val;
    else
        result = (double)MULTROULETTEFACTOR * expected_children;

    expected_children -= val;

    if (expected_children < 0)
        expected_children = 0;

    return (int) result;
}

probe_combi_statistic *probe_combi_statistic::duplicate() const {
    probe_tabs *new_obje = NULL;
    if (probe_tab) {
        new_obje = probe_tab->duplicate();
    }
    return new probe_combi_statistic(probe_combi, new_obje, expected_children, fitness, life_counter);
}

void probe_combi_statistic::mutate_Probe(ProbeValuation *p_eval) {
    int             rand_pool_pos;              // Stelle, an der die Probe im Pool liegt ( von 0 bis laenge-)


    for (int i=0; i<mp_gl_awars.no_of_probes; i++)              // Jede Posititon der Sondenkombination wird mit einer Wahrscheinlichkeit
        // von 1/MUTATION_WS mutiert.
    {
        if (get_random(1, MUTATION_WS) == 1) {
            init_stats();                               // Statistik hat sich geaendert.
            rand_pool_pos  = get_random(0, p_eval->get_pool_length() - 1);
            probe_combi[i] = (p_eval->get_probe_pool())[rand_pool_pos];
        }
    }

    while (!check_duplicates())                     // Solange wie in der Sondenkombination noch Duplikate vorhanden sind
        // muessen diese entfernt werden.
    {
        rand_pool_pos = get_random(0, p_eval->get_pool_length() - 1);
        probe_combi[get_dupl_pos()] = (p_eval->get_probe_pool())[rand_pool_pos];
    }


}

int probe_combi_statistic::get_dupl_pos() {
    int length = mp_gl_awars.no_of_probes;

    for (int i=0; i<length-1; i++)      // auf duplikate in der Sondenkombi. pruefen
        if (probe_combi[i]->probe_index == probe_combi[i+1]->probe_index)
            return i;

    return -1;
}


void probe_combi_statistic::sigma_truncation(double average_fit, double deviation)  // vor allem dafuer gedacht, dass wenn wenige schlecht und
{                                           // sehr viele gute vorhanden sind, dann werden die schlechten
    // 'herausskaliert'
    fitness = (fitness - average_fit) + ((double)SIGMATRUNCATION_CONST * deviation);
    if (fitness < 0)
        fitness = 0;
}

void probe_combi_statistic::crossover_Probes(probe_combi_statistic *pcombi2) {
    // An bis zu no_of_probes werden Gene zwischen pcombi1 und pcombi2 ausgetauscht.

    int rand_cross_pos1,                        // Position an der der Crossover aufgefuehrt wird.
        rand_cross_pos2,
        rand_no_of_cross,                       // Anzahl der Austauschaktionen. (mind. 1)
        random_interval = mp_gl_awars.no_of_probes - 1;
    probe_combi_statistic *f1, *f2;

    rand_no_of_cross = random_interval ? get_random(1, random_interval) : 0;

    for (int i = 0; i < rand_no_of_cross; i++) {            // eigentliche Crossover Schleife
        rand_cross_pos1 = get_random(0, random_interval);
        rand_cross_pos2 = get_random(0, random_interval);

        swap(probe_combi[rand_cross_pos1],          pcombi2->probe_combi[rand_cross_pos2]);
        swap(probe_combi[rand_cross_pos1],          probe_combi[random_interval]);       // um keine Listen zu verwenden
        swap(pcombi2->probe_combi[rand_cross_pos2], pcombi2->probe_combi[random_interval]);         // wird im Array getauscht

        random_interval--;
    }

    while (true) {      // Crossovernachbehandlung, um duplikate in Kombinationen zu vermeiden
        int change1, change2;

        f1 = check_duplicates();
        f2 = pcombi2->check_duplicates();

        if (f1 && f2)
            break;

        if (f1)                         // in f1 kein Duplikat
            change1 = get_random(0, mp_gl_awars.no_of_probes - 1);
        else
            change1 = get_dupl_pos();

        if (f2)                         // in f2 kein Duplikat
            change2 = get_random(0, mp_gl_awars.no_of_probes - 1);
        else
            change2 = pcombi2->get_dupl_pos();

        swap(probe_combi[change1], pcombi2->probe_combi[change2]);        // worst case = die Felder sehen genauso aus, wie vor
        // dem Crossover
    }

    init_stats();
    pcombi2->init_stats();
}

void probe_combi_statistic::init_stats() {
    memset(&probe_tab, 0, sizeof(probe_tabs));                      // bisherige Statistiken fuer diese Sondenkombination zuruecksetzten
    life_counter = MAXLIFEFORCOMBI;
    fitness = 0;
    expected_children = 0;
}

// ----------------------------------------

HammingDistance::HammingDistance(int noOfProbes) {
    size = (int)pow(3.0, (double)noOfProbes);
    dist = new unsigned char*[size];

    int **help = new int*[size];
    for (int i=0; i<size; i++) {
        dist[i] = new unsigned char[size];
        memset(dist[i], 0, sizeof(unsigned char) * size);

        help[i] = new int[noOfProbes];
        memset(help[i], 0, sizeof(int) * noOfProbes);
    }

    {
        int counter = 1;
        for (int i=0; i< noOfProbes; i++) {
            for (int j=0; j<size; j++) {
                for (int wert = 0; wert < 3; wert++) {
                    for (int k=0; k<counter; k++) {
                        help[j++][i] = wert;
                    }
                }
                j--;
            }
            counter *= 3;
        }
    }

    for (int i=0; i<size; i++) {
        for (int j=0; j<size; j++) {
            for (int k=0; k<noOfProbes; k++) {
                if ((help[i][k] == 2 && help[j][k] == 0) ||
                    (help[i][k] == 0 && help[j][k] == 2))
                {
                    dist[i][j]++;
                }
            }
        }
    }

    for (int i=0; i<size; i++) delete [] help[i];
    delete [] help;
}

HammingDistance::~HammingDistance() {
    for (int i = 0; i<size; i++) {
        delete [] dist[i];
    }
    delete [] dist;
}

double probe_combi_statistic::calc_fitness(ProbeValuation *p_eval, int len_of_field, GB_ERROR& error) {
    // fitness-bewertung einer Sondenkombi

    mp_assert(!error);

    int     i, j, k, mod_ham_dist;
    long   *hammingarray;
    double  tolerated_non_group_hits, ham_dist;

    const ProbeCache *stc = mp_global->get_probe_cache(error);

    if (!error) {
        MultiProbeCombinations *combis = new MultiProbeCombinations(stc->get_TargetGroup());

        for (i=0; i<len_of_field && !error; i++) {
            combis->add_probe((p_eval->get_sondenarray())[probe_combi[i]->probe_index],
                              probe_combi[i]->allowed_mismatches,
                              probe_combi[i]->allowed_mismatches + mp_gl_awars.outside_mismatches_difference, error);
        }

        if (!error) probe_tab = combis->fill_Stat_Arrays();
        delete combis;
    }

    fitness = 0.0;

    if (!error) {
        hammingarray = new long[mp_gl_awars.no_of_probes+1];
        
        const HammingDistance& hamming_distance = p_eval->get_hamming_distance();

        for (i=0; i< probe_tab->get_len_group_tabs()-1; i++) {
            memset(hammingarray, 0,  sizeof(long) *(mp_gl_awars.no_of_probes + 1));
            for (j=0; j<probe_tab->get_len_group_tabs(); j++) {
                mod_ham_dist = hamming_distance.get(i, j);
                hammingarray[mod_ham_dist] +=  probe_tab->get_non_group_tab(j);
            }

            tolerated_non_group_hits = (double) mp_gl_awars.qualityborder_best;

            for (k=0; k < mp_gl_awars.no_of_probes + 1 && tolerated_non_group_hits >= 0.0; k++) {
                for (j=0; j<FITNESSSCALEFACTOR && tolerated_non_group_hits >= 0.0; j++) {
                    tolerated_non_group_hits -= (double)((double)hammingarray[k] / (double)FITNESSSCALEFACTOR);
                }

            }

            if (tolerated_non_group_hits<0.0) {
                if (j)
                    ham_dist = (double)k - 1.0 + ((double)(((double)j - 1.0)/(double)FITNESSSCALEFACTOR));
                else
                    ham_dist = (double)k - 1.0;
            }
            else if (tolerated_non_group_hits > 0.0)
                ham_dist = (double) mp_gl_awars.no_of_probes;
            else
                ham_dist = 0.0;

            fitness += ham_dist * ((double) probe_tab->get_group_tab(i));
        }

        delete [] hammingarray;
    }
    return fitness;
}

double probe_combi_statistic::calc_expected_children(double average_fitness) {
    expected_children = fitness / average_fitness;
    return expected_children;
}
