#ifndef MP_PROBE_HXX
#define MP_PROBE_HXX

#ifndef MULTIPROBE_HXX
#include "MultiProbe.hxx"
#endif
#ifndef SOTL_HXX
#include "SoTl.hxx"
#endif

#define NO_GENETIC_ALG      1
#define MIN(x,y)    (x>y) ? y : x
#define MAX(x,y)    (x>y) ? x : y

#define MAXLIFEFORCOMBI 2       // Eine Sondenkombination lebt maximal MAXLIFEFORCOMBI Generationen

class Generation;
class GenerationDuplicates;
class probe_statistic;
class probe_combi_statistic;

typedef struct
{
    int probe_index;
    int allowed_mismatches;
    int e_coli_pos;
}probe;

typedef struct
{
    probe_combi_statistic   *ps;
    char            *view_string;
}result_struct;



class probe_tabs
{
private:
    int             *group_tab;
    int             *non_group_tab;
    int             length_of_group_tabs;
public:
    int             get_non_group_tab(int j) { return non_group_tab[j]; };
    int             get_group_tab(int j) { return group_tab[j]; };
    int             get_len_group_tabs() { return length_of_group_tabs; };

    probe_tabs          *duplicate();
    void            print();
    probe_tabs(int *new_group_field = NULL, int *new_non_group_field = NULL, int len_group = 0);
    ~probe_tabs();
};



class probe_combi_statistic     //die Sondenkombis werden in dieser Klasse gespeichert
{
private:
    probe       **probe_combi;
    probe_tabs      *probe_tab;

    double      expected_children;
    double      fitness;
    int     life_counter;       //Eine Sondenkombination hat nur eine Lebenslaenge von MAXLIFEFORCOMBI

private:
    void        quicksort(long left,
                          long right);  // Randomized Quicksort

    int         get_dupl_pos();     // gibt Index einer Stelle zurueck, die doppelt vorkommt; field
    // muss sortiert sein !!!
    int         modificated_hamming_dist(int one, int two); // pseudo hamming distanz einer Sondenkombi


public:
    void        set_probe_combi(int ind, probe *f) { probe_combi[ind] = f; };
    probe       *get_probe_combi(int j)        { return probe_combi[j]; };
    double      get_fitness()              { return fitness; };
    double      get_expected_children()        { return expected_children; };
    int         sub_expected_children(double val);
    void        init_stats();
    void        scale(double a, double b)      {    //printf("raw fitness : %f**\n",fitness);
        fitness = a * fitness + b;
        //printf("scaled fitness : %f**\n",fitness);
        //printf("*******************\n\n",fitness);
    };

    BOOL        ok_for_next_gen(int &len_roul_wheel);

    void        sub_life_counter()         { life_counter --;};
    void        init_life_counter();
    BOOL        is_dead()              { return (life_counter <= 0) ? TRUE : FALSE; };

    void        sigma_truncation(double average_fit, double dev);       // dient zur Skalierung der Fitness; um zu dominante Kombis zu vermeiden
    double      calc_fitness(int len_of_field);                 // fitness-berechnung einer Sondenkombi
    double      calc_expected_children(double average_fitness);

    void        mutate_Probe();                     // mutiert zufaellig die Sondenkombination nr_of_probe.
    void        crossover_Probes(probe_combi_statistic *pcombi2);   // realisiert den Crossover zwischen Probe1 und Probe2
    void        swap(probe **a, probe **b);

    void        sort(long feld_laenge);     // es wird ein randomized quicksort verwendet
    probe_combi_statistic   *duplicate();       //dupliziert dieses Objekt (z.B. fuer naechste Generation)
    probe_combi_statistic   *check_duplicates(GenerationDuplicates *dup_tree = NULL);
    // rueckgabewert ist NULL, wenn das Feld duplikate enthaelt bzw.
    // es wird sortiertes field zurueckgegeben. Wenn NULL zurueckkommt, dann
    // wurde field jedoch noch nicht deleted
    int         calc_index_system3(int *field);
    void        print();
    void        print(probe *p);
    void        print(probe **arr, int length);


    probe_combi_statistic(probe **pc = NULL, probe_tabs *ps = NULL, double exp = 0, double fit = 0 , int lifec = MAXLIFEFORCOMBI);
    ~probe_combi_statistic();
};

class Generation
{
private:
    probe_combi_statistic   **probe_combi_stat_array;       // Liste von Sondenkombinationen, auf denen der genetische Algorithmus aus-
    // gefuehrt wird.
    int             probe_combi_array_length;
    GenerationDuplicates    *dup_tree;
    double          average_fitness;
    double          min_fit;
    double          max_fit;
    double          deviation;              //Abweichung

    int             len_roulette_wheel;         //wichtig zur Erstellung der naechsten Generation
    int             generation_counter;


    //***intern variables
    int         last_elem;

private:
    void        prescale(double *a, double *b);         //berechnet Koeffizienten fuer lineare Skalierung
    probe_combi_statistic *choose_combi_for_next_generation();

public:
    double      get_avg_fit() { return average_fitness; };
    int     get_generation() { return generation_counter; };
    GenerationDuplicates *get_dup_tree()    { return dup_tree; };
    void        set_length() { probe_combi_array_length = last_elem; };     // nur verwenden, wenn man weiss was man tut !!!!
    void        check_for_results();                //traegt eventuelle. resultate in Ergebnisfenster ein

    BOOL        insert(probe_combi_statistic *pcs);     //FALSE wenn Generation schon MAXPOPULATION Eintraege hat

    void        init_valuation();
    void        gen_determ_combis(int beg,          //wo faengt der Alg. an
                                  int len,          //wieviele probes muessen noch drangehaengt werden
                                  int &pos_counter,     //zaehler fuer probe_combi_stat_array
                                  probe_combi_statistic *p);    //bisher zusammengestellte probe

    void        calc_fitness(int flag = 0, double old_avg_fit = 0);// fitness-berechnung aller Sondenkombis im Feld; und average_fitness
    // und deviation
    void        init_roulette_wheel();
    Generation      *create_next_generation();          // die Kindergeneration wird zurueckgegeben

    probe_combi_statistic *single_in_generation(probe_combi_statistic *field);  // Nach der Funktion ist sichergestellt, dass dieses field in der
    //Generation nur einmal vorkommt. Achtung: wenn Population sehr klein
    //=> Endlosschleifengefahr ( nicht in dieser Funktion, sondern u.U. in der
    // aufrufenden

    void        print();

    Generation(int len, int gen_nr);
    ~Generation();
};

class ProbeValuation
{

private:
    char            **sondenarray;
    int             *bewertungarray;
    int             *mismatch_array;
    int             size_sonden_array;
    probe           **probe_pool;               // Generierung eines Pools, in dem die Wahrscheinlichkeiten fuer die Erfassung
    // der Sonden schon eingearbeitet sind. DIe WS werden vom Benutzer fuer jede
    // einzelne Sonde bestimmt.
    int             pool_length,
        max_init_pop_combis;
    Generation          *act_generation,
        *child_generation;
    List<result_struct>     *computation_result_list;

public:
    void        set_act_gen(Generation *g) { act_generation = g; };
    int         get_max_init_for_gen() { return max_init_pop_combis; };
    int         get_pool_length()   { return pool_length; };
    probe       **get_probe_pool()  { return probe_pool; };
    int         get_size_sondenarray()  { return size_sonden_array; };
    char        **get_sondenarray() { return sondenarray; };

    void        insert_in_result_list(probe_combi_statistic *pcs);

    void        init_valuation();               // Zufaellige Auswahl einer Grundmenge von  Sondenkombinationen
    void        evolution();                    // Evolution

    ProbeValuation(char **sonden_array, int no_of_sonden, int *bewertung,int *single_mismatch);
    ~ProbeValuation();
};

class GenerationDuplicates      // Fuer eine Generation muss ueberprueft werden, ob es doppelte Sondenkombinationen
{                   // gibt.
private:
    int             intern_size;    //enthaelt size aus dem Konstruktor (size entspricht muss der groesse von sondenarray entsprechen
    GenerationDuplicates    **next;     //die laenge dieses arrays entspricht der laenge des sondenarrays in ProbeValuation
    int             *next_mism; //zu jedem next eintrag merkt man sich wieviele Mismatche schon aufgetreten sind
    //laenge von next_mism ist die maximale anzahl der Mismatche

public:
    BOOL insert(probe_combi_statistic *sondenkombi, BOOL &result, int depth = 0);   //fuegt sondenkombination ein, wenn es Sie in dieser Struktur noch nicht gibt(=> TRUE).
    //Wenn es Sie schon gibt, dann FALSE. depth ist nur fuer interne Zwecke.

    GenerationDuplicates(int size);
    ~GenerationDuplicates();        //loescht rekursiv nach unten alles.
};

#else
#error MP_probe.hxx included twice
#endif

