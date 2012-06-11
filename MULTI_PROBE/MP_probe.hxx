#ifndef MP_PROBE_HXX
#define MP_PROBE_HXX

#ifndef SOTL_HXX
#include "SoTl.hxx"
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef ARB_CORE_H
#include <arb_core.h>
#endif
#ifndef MULTIPROBE_HXX
#include "MultiProbe.hxx"
#endif

#define MAXLIFEFORCOMBI 2       // Eine Sondenkombination lebt maximal MAXLIFEFORCOMBI Generationen

class StrArray;
class ConstStrArray;

struct ProbeRef {
    int probe_index;
    int allowed_mismatches;
    int e_coli_pos;
};

class probe_tabs : virtual Noncopyable {
    int *group_tab;
    int *non_group_tab;
    int  length_of_group_tabs;

public:
    
    int get_non_group_tab(int j) { return non_group_tab[j]; }
    int get_group_tab(int j) { return group_tab[j]; }
    int get_len_group_tabs() { return length_of_group_tabs; }

    probe_tabs *duplicate();
    void        print();

    probe_tabs(int *new_group_field = NULL, int *new_non_group_field = NULL, int len_group = 0);
    ~probe_tabs();
};

class probe_combi_statistic : virtual Noncopyable {
    // die Sondenkombis werden in dieser Klasse gespeichert
    const ProbeRef **probe_combi;
    probe_tabs      *probe_tab;

    double expected_children;
    double fitness;
    int    life_counter;        // Eine Sondenkombination hat nur eine Lebenslaenge von MAXLIFEFORCOMBI

    void randomized_quicksort(long left, long right);

    int get_dupl_pos(); // gibt Index einer Stelle zurueck, die doppelt vorkommt; field muss sortiert sein !!!

public:
    void set_probe_combi(int j, const ProbeRef *f) { probe_combi[j] = f; }
    const ProbeRef *get_probe_combi(int j) { return probe_combi[j]; }

    double      get_fitness()              { return fitness; }
    double      get_expected_children()        { return expected_children; }
    int         sub_expected_children(double val);
    void        init_stats();
    void        scale(double a, double b)      { fitness = a * fitness + b; }

    bool        ok_for_next_gen(int &len_roul_wheel);

    void        init_life_counter() { life_counter = MAXLIFEFORCOMBI; }
    void        dec_life_counter() { life_counter--; }
    bool        is_dead() { return life_counter <= 0; }

    void        sigma_truncation(double average_fit, double dev);       // dient zur Skalierung der Fitness; um zu dominante Kombis zu vermeiden
    double      calc_fitness(class ProbeValuation *p_eval, int len_of_field, GB_ERROR& error);                 // fitness-berechnung einer Sondenkombi
    double      calc_expected_children(double average_fitness);

    void mutate_Probe(ProbeValuation *p_eval);                            // mutiert zufaellig die Sondenkombination nr_of_probe.
    void crossover_Probes(probe_combi_statistic *pcombi2);                // realisiert den Crossover zwischen Probe1 und Probe2
    
    void swap(const ProbeRef*& a, const ProbeRef*& b) {
        const ProbeRef *help = a;
        a = b;
        b = help;
    }

    void sort(long feld_laenge) { if (feld_laenge>1) randomized_quicksort(0, feld_laenge-1); }

    probe_combi_statistic   *duplicate() const;       // dupliziert dieses Objekt (z.B. fuer naechste Generation)
    probe_combi_statistic *check_duplicates();

    void print();
    void print(const ProbeRef *p);
    void print(const ProbeRef **arr, int length);


    probe_combi_statistic(const ProbeRef **pc = NULL, probe_tabs *ps = NULL, double exp = 0, double fit = 0,  int lifec = MAXLIFEFORCOMBI);
    ~probe_combi_statistic();
};

class Generation : virtual Noncopyable {
    probe_combi_statistic **probe_combi_stat_array;         // Liste von Sondenkombinationen, auf denen der genetische Algorithmus ausgefuehrt wird.

    int allocated;
    int inserted;

    double average_fitness;
    double min_fit;
    double max_fit;
    double deviation; // Abweichung

    int len_roulette_wheel; // wichtig zur Erstellung der naechsten Generation
    int generation_counter;

    bool valid_probe_combi_index(int i) const { return i >= 0 && i<allocated; }

private:
    void prescale(double *a, double *b); // berechnet Koeffizienten fuer lineare Skalierung
    probe_combi_statistic *choose_combi_for_next_generation();

public:
    double get_avg_fit() { return average_fitness; }
    int get_generation() { return generation_counter; }
    
    void retrieve_results(ProbeValuation *p_eval, StrArray& results);

    bool insert(probe_combi_statistic *pcs); // false wenn Generation schon MAXPOPULATION Eintraege hat

    void init_valuation(ProbeValuation *p_eval);

    void gen_determ_combis(ProbeValuation        *p_eval,
                           int                    beg,         // wo faengt der Alg. an
                           int                    len,         // wieviele probes muessen noch drangehaengt werden
                           int                   &pos_counter, // zaehler fuer probe_combi_stat_array
                           probe_combi_statistic *p);          // bisher zusammengestellte probe

    bool calcFitness(ProbeValuation *p_eval, bool use_genetic_algo, double old_avg_fit, GB_ERROR& error); // fitness-berechnung aller Sondenkombis im Feld; und average_fitness und deviation

    void init_roulette_wheel();
    Generation *create_next_generation(ProbeValuation *p_eval); // die Kindergeneration wird zurueckgegeben

    void print();

    Generation(int len, int gen_nr);
    ~Generation();
};

class ProbeCombi : virtual Noncopyable {
    probe_combi_statistic *ps;
    char                  *view_string;

public:
    ProbeCombi(const probe_combi_statistic *pcs, const char *vs)
        : ps(pcs->duplicate()),
          view_string(strdup(vs))
    {}

    ~ProbeCombi() {
        delete ps;
        free(view_string);
    }

    bool has_same_view_string_as(const ProbeCombi& other) const { return strcmp(view_string, other.view_string) == 0; }
    double get_fitness() const { return ps->get_fitness(); }
    const char *get_viewString() const { return view_string; }
};

class ProbeValuationResults : virtual Noncopyable {
    List<ProbeCombi> computation_result_list; // @@@ replace by std container

public:

    ~ProbeValuationResults() {
        ProbeCombi *elem = computation_result_list.get_first();
        while (elem) {
            computation_result_list.remove_first();
            delete elem;
            elem = computation_result_list.get_first();
        }
    }

    List<ProbeCombi>& get_list() { return computation_result_list; }
};

class HammingDistance : virtual Noncopyable {
    unsigned char **dist;
    int             size;

public:
    HammingDistance(int noOfProbes_);
    ~HammingDistance();
    int get(int i, int j) const { return dist[i][j]; }
};

class ProbeValuation : virtual Noncopyable {
    HammingDistance hamdist;
    
    // @@@ merge elements of these 4 arrays into one class and replace by one array of instances of that class
    char **sondenarray;
    int   *bewertungarray;
    int   *mismatch_array;
    int   *ecolipos_array;
    int    size_sonden_array;

    ProbeRef **probe_pool; // Generierung eines Pools, in dem die Wahrscheinlichkeiten fuer die Erfassung
    // der Sonden schon eingearbeitet sind. DIe WS werden vom Benutzer fuer jede einzelne Probe bestimmt.

    int pool_length;
    
    int max_init_pop_combis;

    Generation *act_generation;

    GB_ERROR evolution(StrArray& results) __ATTR__USERESULT;

public:
    void set_act_gen(Generation *g) { act_generation = g; }
    int get_max_init_for_gen() { return max_init_pop_combis; }

    int get_pool_length()   { return pool_length; }
    const ProbeRef *const *get_probe_pool() { return probe_pool; }
    
    int get_size_sondenarray() { return size_sonden_array; }
    char **get_sondenarray() { return sondenarray; } // @@@ rename

    void insert_in_result_list(probe_combi_statistic *pcs, ProbeValuationResults& pvr);

    const HammingDistance& get_hamming_distance() const { return hamdist; }

    GB_ERROR evaluate(StrArray& results) __ATTR__USERESULT; // Zufaellige Auswahl einer Grundmenge von Sondenkombinationen

    ProbeValuation(char**& sonden_array, int no_of_sonden, int*& bewertung, int*& mismatch, int*& ecoli_pos);
    ~ProbeValuation();
};

#else
#error MP_probe.hxx included twice
#endif

