#include <MultiProbe.hxx>

#include <string.h>
#include <math.h>

extern double MAXMARKEDFACTOR;
extern double MINUNMARKEDFACTOR;
extern double SUMMARKEDFACTOR;
extern double SUMUNMARKEDFACTOR;

probe_combi_statistic::probe_combi_statistic(probe **pc, probe_tabs *ps, double exp, double fit, int life_cnt)
{
    memset( this, 0, sizeof( probe_combi_statistic ) );

    if (ps)
        probe_tab = ps;
    else
        probe_tab = NULL; //new probe_tabs;

    probe_combi = new probe*[mp_gl_awars.no_of_probes];
    if (pc)
    {
        for (int i=0; i < mp_gl_awars.no_of_probes; i++)
            probe_combi[i] = pc[i];
    }
    else
        memset(probe_combi, 0, mp_gl_awars.no_of_probes * sizeof(probe*));


    expected_children = exp;
    fitness = fit;
    life_counter = life_cnt;
}

probe_combi_statistic::~probe_combi_statistic()
{
    delete [] probe_combi;
    delete probe_tab;
}

BOOL probe_combi_statistic::ok_for_next_gen(int &len_roul_wheel)
{
    double exp_child = get_expected_children();

    if (exp_child >= 1.0 || get_random(1, 100) <= 100 * exp_child)      //Behandlung nach Goldberg S.115 bzw. S.121
    {
        if (!is_dead())
            return TRUE;
        else
        {
            len_roul_wheel -= (int) (MULTROULETTEFACTOR * get_expected_children());
            expected_children = 0.0;
        }
    }
    return FALSE;
}

void probe_combi_statistic::init_life_counter()
{
    life_counter = MAXLIFEFORCOMBI;
}

void probe_combi_statistic::sort(long feld_laenge)
{
    if (!feld_laenge)
        return;

    quicksort(0, feld_laenge-1);
}

inline void probe_combi_statistic::swap(probe **a, probe **b)
{
    register probe *help;

    help = *a;
    *a = *b;
    *b = help;
}

void probe_combi_statistic::quicksort(long left, long right)        //Randomized Quicksort !!! wegen effizienz
{                                   // Fuer den Fall, dass Feld sortiert !!!
    register long   i = left,
        j = right;
    int         x,
        help,
        help2;

    if (j>i)
    {
        help = (left + right) / 2;

        // Randomisierung des Quicksort Anfang
        // Falls keine Randomisierung erwuenscht, einfach diesen Teil auskommentieren !!!
        help2 = get_random(left, right);
        swap( & probe_combi[help2], & probe_combi[help]);
        // Randomisierung des Quicksort Ende

        x = probe_combi[help]->probe_index;         // Normale Auswahl des Pivotelements

        do
        {
            while (probe_combi[i]->probe_index < x) i++;
            while (probe_combi[j]->probe_index > x) j--;

            if (i<=j)
            {
                swap ( & probe_combi[i], & probe_combi[j] );

                i++;
                j--;
            }
        } while (i<=j);

        quicksort(left, j);
        quicksort(i, right);
    }
}

probe_combi_statistic *probe_combi_statistic::check_duplicates(GenerationDuplicates *dup_tree)
{
    BOOL result = TRUE;

    sort(mp_gl_awars.no_of_probes);

    if (get_dupl_pos() == -1)
        return this;

    if (dup_tree)           //d.h. dass feld this nur einmal in der Generation vorkommen darf
        if (dup_tree->insert(this, result, mp_gl_awars.no_of_probes))
            return this;

    return NULL;
}


void probe_combi_statistic::print(probe *p)
{
    //    printf("Eine Sonde entspricht : ");
    printf("Idx:%d alMis:%d  ",p->probe_index, p->allowed_mismatches);
}

void probe_combi_statistic::print()
{
    for (int i=0; i<mp_gl_awars.no_of_probes; i++)
        print(probe_combi[i]);

    printf("Fit:%f Expchil: %f\n",fitness, expected_children);
    probe_tab->print();
}

void probe_combi_statistic::print(probe **arr, int length)
{
    for (int i=0; i<length; i++)
        print(arr[i]);
}

int probe_combi_statistic::sub_expected_children(double val)
{
    register double result;

    if (expected_children - val > 0)
        result = (double)MULTROULETTEFACTOR * val;
    else
        result = (double)MULTROULETTEFACTOR * expected_children;

    expected_children -= val;

    if (expected_children < 0)
        expected_children = 0;

    return (int) result;
}

probe_combi_statistic *probe_combi_statistic::duplicate()
{
    probe_tabs *new_obje = NULL;

    if (probe_tab)
        new_obje = probe_tab->duplicate();

    return new probe_combi_statistic(probe_combi, new_obje,expected_children, fitness, life_counter);
}

void probe_combi_statistic::mutate_Probe()
{
    int             rand_pool_pos;              // Stelle, an der die Sonde im Pool liegt ( von 0 bis laenge-)


    for (int i=0; i<mp_gl_awars.no_of_probes; i++)              // Jede Posititon der Sondenkombination wird mit einer Wahrscheinlichkeit
        // von 1/MUTATION_WS mutiert.
    {
        if (get_random(1,MUTATION_WS) == 1)
        {
            init_stats();                               // Statistik hat sich geaendert.
            rand_pool_pos   = get_random(0, mp_main->get_p_eval()->get_pool_length() - 1);
            probe_combi[i]  = (mp_main->get_p_eval()->get_probe_pool())[rand_pool_pos];
        }
    }

    while (!check_duplicates())                     // Solange wie in der Sondenkombination noch Duplikate vorhanden sind
        // muessen diese entfernt werden.
    {
        rand_pool_pos   = get_random(0, mp_main->get_p_eval()->get_pool_length() - 1);
        probe_combi[get_dupl_pos()] = (mp_main->get_p_eval()->get_probe_pool())[rand_pool_pos];
    }


}

int probe_combi_statistic::get_dupl_pos()
{
    register int length = mp_gl_awars.no_of_probes;

    for (int i=0; i<length-1; i++)      //auf duplikate in der Sondenkombi. pruefen
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

void probe_combi_statistic::crossover_Probes(probe_combi_statistic *pcombi2)    // An bis zu no_of_probes werden Gene zwischen pcombi1
    // und pcombi2 ausgetauscht.
{

    int rand_cross_pos1,                        // Position an der der Crossover aufgefuehrt wird.
        rand_cross_pos2,
        rand_no_of_cross,                       // Anzahl der Austauschaktionen. (mind. 1)
        random_intervall = mp_gl_awars.no_of_probes - 1;
    probe_combi_statistic *f1, *f2;

    rand_no_of_cross = random_intervall ? get_random(1, random_intervall) : 0;

    for (int i = 0; i < rand_no_of_cross; i++)              //eigentliche Crossover Schleife
    {
        rand_cross_pos1 = get_random(0, random_intervall);
        rand_cross_pos2 = get_random(0, random_intervall);

        swap( & probe_combi[rand_cross_pos1], & pcombi2->probe_combi[rand_cross_pos2]);

        swap(  & probe_combi[rand_cross_pos1],  & probe_combi[random_intervall] );      //um keine Listen zu verwenden
        swap(  & pcombi2->probe_combi[rand_cross_pos2],  & pcombi2->probe_combi[random_intervall] );        //wird im Array getauscht

        random_intervall--;
    }

    while (TRUE)        //Crossovernachbehandlung, um duplikate in Kombinationen zu vermeiden
    {
        int change1, change2;

        f1 = check_duplicates();
        f2 = pcombi2->check_duplicates();

        if (f1 && f2 )
            break;

        if (f1)                         //in f1 kein Duplikat
            change1 = get_random(0,mp_gl_awars.no_of_probes - 1);
        else
            change1 = get_dupl_pos();

        if (f2)                         //in f2 kein Duplikat
            change2 = get_random(0,mp_gl_awars.no_of_probes - 1);
        else
            change2 = pcombi2->get_dupl_pos();

        swap( & probe_combi[change1],  & pcombi2->probe_combi[change2]);        //worst case = die Felder sehen genauso aus, wie vor
        //dem Crossover
    }

    init_stats();
    pcombi2->init_stats();
}

void probe_combi_statistic::init_stats()
{
    memset(&probe_tab, 0, sizeof(probe_tabs));                      // bisherige Statistiken fuer diese Sondenkombination zuruecksetzten
    life_counter = MAXLIFEFORCOMBI;
    fitness = 0;
    expected_children = 0;
}

int probe_combi_statistic::calc_index_system3(int *field)
{
    register int i, result = 0;

    for (i=0; i<mp_gl_awars.no_of_probes; i++)
        result += system3_tab[field[i]][i];     //Ergebnis von : (3^i) * field[i];

    return result;
}


inline int probe_combi_statistic::modificated_hamming_dist(int one, int two) // pseudo hamming distanz einer Sondenkombi
{
    return hamming_tab[one][two];
}

double  probe_combi_statistic::calc_fitness(int len_of_field)       // fitness-bewertung einer Sondenkombi
{
    int     i, j, k, mod_ham_dist;
    long        *hammingarray;
    double  tolerated_non_group_hits, ham_dist;


    Sondentopf  *sondentopf = new Sondentopf(mp_main->get_stc()->Bakterienliste,
                                             mp_main->get_stc()->Auswahlliste);

    for (i=0; i<len_of_field; i++)
        sondentopf->put_Sonde((mp_main->get_p_eval()->get_sondenarray())[probe_combi[i]->probe_index],
                              probe_combi[i]->allowed_mismatches,
                              probe_combi[i]->allowed_mismatches + mp_gl_awars.outside_mismatches_difference);

    probe_tab = sondentopf->fill_Stat_Arrays();
    delete sondentopf;

    fitness = 0.0;
    hammingarray = new long[mp_gl_awars.no_of_probes+1];

    for (i=0; i< probe_tab->get_len_group_tabs()-1; i++)
    {
        memset(hammingarray, 0 , sizeof(long) * (mp_gl_awars.no_of_probes + 1));
        for (j=0; j<probe_tab->get_len_group_tabs(); j++)
        {
            mod_ham_dist = modificated_hamming_dist(i, j);
            hammingarray[mod_ham_dist] +=  probe_tab->get_non_group_tab(j);
        }

        tolerated_non_group_hits = (double) mp_gl_awars.qualityborder_best;

        for (k=0; k < mp_gl_awars.no_of_probes + 1 && tolerated_non_group_hits >= 0.0; k++)
        {
            for (j=0; j<FITNESSSCALEFACTOR && tolerated_non_group_hits >= 0.0; j++)
            {
                tolerated_non_group_hits -= (double) ((double)hammingarray[k] / (double)FITNESSSCALEFACTOR);
            }

        }

        if (tolerated_non_group_hits<0.0)
        {
            if (j)
                ham_dist = (double)k - 1.0 + ((double)(((double)j - 1.0)/(double)FITNESSSCALEFACTOR));
            else
                ham_dist = (double)k - 1.0;
        }
        else if(tolerated_non_group_hits > 0.0)
            ham_dist = (double) mp_gl_awars.no_of_probes;
        else
            ham_dist = 0.0;

        fitness += ham_dist * ((double) probe_tab->get_group_tab(i));
    }

    delete [] hammingarray;
    return fitness;
}

double probe_combi_statistic::calc_expected_children(double average_fitness)
{
    expected_children = fitness / average_fitness;
    return expected_children;
}


