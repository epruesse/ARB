#include <MultiProbe.hxx>

#include <string.h>
#include <math.h>

extern BOOL check_status(int gen_cnt, double avg_fit, double min_fit, double max_fit);
BOOL    Stop_evaluation = FALSE;

probe_combi_statistic *Generation::single_in_generation(probe_combi_statistic *field)
{
    BOOL result = TRUE;

    if (!dup_tree)
        return NULL;

    dup_tree->insert(field, result);        //dup_tree muss fuer jede Generation neu erstellt werden !!!!
    //dieser Aufruf wird nur zur Vermeidung von doppelten
    //Sondenkombis benoetigt
    if (result)                 //wenn result, dann in generation einmalig
        return field;

    return NULL;                //field ist ein Duplikat
}

void Generation::print()
{
    for (int i=0; i<probe_combi_array_length; i++)
        probe_combi_stat_array[i]->print();
}

void Generation::check_for_results()
{
    for (int i=0; i<probe_combi_array_length; i++)
    {
        if (probe_combi_stat_array[i])
            mp_main->get_p_eval()->insert_in_result_list(probe_combi_stat_array[i]);
    }
}

void Generation::calc_fitness(int flag, double old_avg_fit)     //reoulette_wheel wird DANACH initialisiert
{
    double  fitness = 0;
    double  dummy = 0;

    int     i;

    for (i=0; i<probe_combi_array_length; i++)
    {

        dummy = probe_combi_stat_array[i]->calc_fitness(mp_gl_awars.no_of_probes);
        fitness += dummy;

        if (i==0)
            min_fit = max_fit = dummy;

        if (dummy < min_fit)
            min_fit = dummy;
        else if (dummy > max_fit)
            max_fit = dummy;

        if (!check_status(generation_counter, old_avg_fit, min_fit, max_fit))       //Berechnungen abbrechen
        {
            Stop_evaluation = TRUE;
            probe_combi_array_length = i-1;
            return;
        }
    }

    if (flag == NO_GENETIC_ALG)             //wenn kein gen. ALgorithmus verwendet wird, dann
        return;                     //muss der Rest nicht berechnet werden.

    average_fitness = fitness / (double)probe_combi_array_length;

    //    printf("--------------------------------------------------------------------------------------------------------------------\n");
    //    printf("BEWERTUNG FUER DIE GENERATION:\n");
    //    printf("Generation: %d   max: %f   min: %f   avg: %f\n", generation_counter, max_fit, min_fit, average_fitness);
    //    printf("probe_combi_array_lentgh : %d\n",probe_combi_array_length);
    //    printf("--------------------------------------------------------------------------------------------------------------------\n");


    deviation = 0;


#ifdef USE_LINEARSCALING
    double  dev = 0;
    double  a = 0,
        b = 0;
#ifdef USE_SIGMATRUNCATION
    for (i=0; i<probe_combi_array_length; i++)          // Berechnung der Abweichung nach Goldberg S.124
    {
        dev = probe_combi_stat_array[i]->get_fitness() - average_fitness;
        dev = dev * dev;
        deviation += dev;
    }
    deviation = (1.0 / (double)((double)i - 1.0)) * deviation;
    deviation = sqrt(deviation);

    for (i=0; i<probe_combi_array_length; i++)          //sigma_truncation nur auf schlechte Kombis anwenden ???
        probe_combi_stat_array[i]->sigma_truncation(average_fitness, deviation);
#endif
    //lineare Skalierung auf fitness anwenden !!!
    //Skalierung erfolgt nach der Formel     fitness'= a*fitness + b

    prescale(&a, &b);       //Koeffizienten a und b berechnen
#endif

    for (i=0; i<probe_combi_array_length; i++)
    {
#ifdef USE_LINEARSCALING
        probe_combi_stat_array[i]->scale(a,b);
#endif
        probe_combi_stat_array[i]->calc_expected_children(average_fitness);
    }

    init_roulette_wheel();
}

void Generation::prescale(double *a, double *b) //berechnet Koeffizienten fuer lineare Skalierung
{
    double delta = 0;

    if ((min_fit > C_MULT * average_fitness - max_fit) / (C_MULT - 1.0))    //nach Goldberg S.79
    {
        delta = max_fit - average_fitness;              // Normale Skalierung
        *a = (C_MULT - 1.0) * average_fitness / delta;
        *b = average_fitness * (max_fit - C_MULT * average_fitness) / delta;
    }
    else                                //Skalieren soweit moeglich
    {
        delta = average_fitness - min_fit;
        *a = average_fitness / delta;
        *b = -min_fit * average_fitness / delta;
    }
}

void Generation::init_roulette_wheel()
{
    int     i=0;

    len_roulette_wheel = 0;
    while (i<probe_combi_array_length)
        len_roulette_wheel += (int)(MULTROULETTEFACTOR * (probe_combi_stat_array[i++]->get_expected_children()));   //um aus z.B. 4,2 42 zu machen
}

probe_combi_statistic *Generation::choose_combi_for_next_generation()
{
    int random_help = get_random(0,len_roulette_wheel-1),
        i;

    for (i=0; i<probe_combi_array_length; i++)              //in einer Schleife bis zu den betreffenden Elementen
    {                                   // vordringen (Rouletterad !!!)
        random_help -= (int) (MULTROULETTEFACTOR * probe_combi_stat_array[i]->get_expected_children());

        if (random_help <= 0)
        {
            if (probe_combi_stat_array[i]->ok_for_next_gen(len_roulette_wheel))
                return probe_combi_stat_array[i];
            else
            {
                random_help = get_random(0,len_roulette_wheel-1);
                i = -1;
            }
        }
    }

    return NULL;
}

Generation *Generation::create_next_generation()
{
    Generation      *child_generation = new Generation(MAXPOPULATION, generation_counter+1);
    probe_combi_statistic   *first_child_pcs = NULL,
        *second_child_pcs = NULL,
        *orig1 = NULL,
        *orig2 = NULL;
    int cnt = 0;
#ifdef USE_DUP_TREE
    BOOL res;
#endif

    while (len_roulette_wheel > 1)                  // kann kleiner sein, wenn Population kleiner als MAXPOPULATION
    {
        cnt++;
        orig1 = choose_combi_for_next_generation();
        orig2 = choose_combi_for_next_generation();

        if (! orig1 && ! orig2)
            break;
        else if (!orig1 && orig2)
        {
            orig1 = orig2;
            orig2 = NULL;
        }

        delete first_child_pcs;
        delete second_child_pcs;
        first_child_pcs = second_child_pcs = NULL;

        first_child_pcs = orig1->duplicate();
        if (orig2)
            second_child_pcs = orig2->duplicate();

        if (orig2 && get_random(1,100) <= CROSSOVER_WS)     //Crossover durchfueheren
        {
            first_child_pcs->crossover_Probes(second_child_pcs);
            first_child_pcs->init_life_counter();       //wenn  Crossover durchgefuehrt wird, dann Lebensdauer wieder initialisieren, da
            second_child_pcs->init_life_counter();      //sich die Gene veraendert haben
            len_roulette_wheel -= orig1->sub_expected_children(0.5);    //Verfahren nach Goldberg S.115
            len_roulette_wheel -= orig2->sub_expected_children(0.5);
        }
        else
        {
            first_child_pcs->sub_life_counter();                //Gene gleich geblieben => Lebensdauer verkuerzen
            len_roulette_wheel -= orig1->sub_expected_children(1.0);    //nur tatsaechlich subtrahierte Zahl abziehen !!!

            if (orig2)
            {
                second_child_pcs->sub_life_counter();
                len_roulette_wheel -= orig2->sub_expected_children(1.0);
            }
        }

        first_child_pcs->mutate_Probe();    //fuer jede Position wird mit 1/MUTATION_WS eine Mutation durchgefuehrt.
        if (orig2)                  //Mutationen durchfuehren
            second_child_pcs->mutate_Probe();

#ifdef USE_DUP_TREE

        res = TRUE;
        if (child_generation->get_dup_tree()->insert(first_child_pcs, res, 0))
        {
            if (!child_generation->insert(first_child_pcs))     //Population schon auf MAXPOPULATION
                break;
        }

        res = TRUE;
        if (child_generation->get_dup_tree()->insert(second_child_pcs, res, 0))
        {
            if (orig2)
                if (!child_generation->insert(second_child_pcs))
                    break;
        }

#else
        if (!child_generation->insert(first_child_pcs))     //Population schon auf MAXPOPULATION
            break;

        if (orig2)
            if (!child_generation->insert(second_child_pcs))
                break;

#endif
    }

    delete first_child_pcs;
    delete second_child_pcs;

    if (len_roulette_wheel <= 1)
        child_generation->set_length();             //probe_combi_array_length muss andere laenge bekommen

    //  printf("Generationenschleife : %d\n",cnt);
    return child_generation;
}

void Generation::gen_determ_combis(int beg,
                                   int len,
                                   int &pos_counter,
                                   probe_combi_statistic *p)
{
    int     i, j;
    probe_combi_statistic   *bastel_probe_combi;

    if (len == 0)
    {
        probe_combi_stat_array[pos_counter++] = p;
        return;
    }

    for (i=beg; i <= mp_main->get_p_eval()->get_pool_length() - len; i++)
    {
        bastel_probe_combi = new probe_combi_statistic();

        for (j=0; j < mp_gl_awars.no_of_probes - len; j++)
            bastel_probe_combi->set_probe_combi(j, p->get_probe_combi(j));

        if (len == mp_gl_awars.no_of_probes ||
            (mp_main->
             get_p_eval()->
             get_probe_pool())[i]->
            probe_index !=
            bastel_probe_combi->
            get_probe_combi(mp_gl_awars.no_of_probes - len - 1)->probe_index)
        {
            bastel_probe_combi->set_probe_combi(    mp_gl_awars.no_of_probes - len,
                                                    (mp_main->get_p_eval()->get_probe_pool())[i]);
            gen_determ_combis(i+1, len-1, pos_counter, bastel_probe_combi);
        }

        if (len !=1)
            delete bastel_probe_combi;
    }
}

BOOL Generation::insert(probe_combi_statistic *pcs)
{
    if (last_elem == MAXPOPULATION)
        return FALSE;

    probe_combi_stat_array[last_elem++] = pcs->duplicate();
    probe_combi_array_length = last_elem;

    return TRUE;
}

void Generation::init_valuation()
{
    int         i, counter=0;
    probe           *random_probe;
    int         zw_erg;
    probe_combi_statistic *pcs;
    int         pos = 0;


    if (probe_combi_array_length < MAXINITPOPULATION)
    {
        gen_determ_combis(0,mp_gl_awars.no_of_probes, pos, NULL);   //probe_combi_stat_array ist danach gefuellt !!!

        probe_combi_array_length = pos;

        return;         //aufruf der funktion fuer die letzte Generation
    }

    counter = 0;
    pcs = new probe_combi_statistic();

    while (counter < probe_combi_array_length)      //Hier erfolgt die Generierung des probe_combi_stat_array
    {
        for (i=0; i<mp_gl_awars.no_of_probes; i++)
        {
            zw_erg = get_random(0, mp_main->get_p_eval()->get_pool_length()-1);
            random_probe = (mp_main->get_p_eval()->get_probe_pool())[zw_erg];
            pcs->set_probe_combi(i, random_probe);
        }

        if (pcs->check_duplicates(dup_tree))            //2 gleiche Sonden in der Kombination => nicht verwendbar
        {
            probe_combi_stat_array[counter++] = pcs;
            if (counter < probe_combi_array_length)
                pcs = new probe_combi_statistic();
        }
    }
}

Generation::Generation(int len, int gen_nr)
{
    memset( (char *)this, 0, sizeof(Generation) );

    probe_combi_array_length  = len;
    probe_combi_stat_array = new probe_combi_statistic*[probe_combi_array_length];  //probe_combi_array_length entspricht
    // der Groesse der Ausgangspopulation
    memset(probe_combi_stat_array, 0, probe_combi_array_length * sizeof(probe_combi_statistic*));   // Struktur mit 0 initialisieren.
    generation_counter = gen_nr;

#ifdef USE_DUP_TREE
    dup_tree = new GenerationDuplicates(mp_main->get_p_eval()->get_size_sondenarray());     //nur wenn sondenkombis nur einmal
    // in der Generation vorkommen duerfen
#endif

}

Generation::~Generation()
{
    int i;

    for (i=0; i<probe_combi_array_length; i++)
        delete probe_combi_stat_array[i];

    delete [] probe_combi_stat_array;
    delete dup_tree;
}
