#include <MP_probe.hxx>

#include <aw_root.hxx>
#include <aw_window.hxx>
#include <stdlib.h>
#include <time.h>
#include <string.h>

extern BOOL check_status(int gen_cnt, double avg_fit, double min_fit, double max_fit);
extern char     *MP_get_comment(int which, char *str);

void ProbeValuation::evolution()
{
    long n=0;
    long moeglichkeiten;
    double avg_fit = 0;

    for (int i=0; i<size_sonden_array; i++)             // Mismatche (=duplikate) aufsummieren, um Groesse von Pool zu bestimmen.
        n += mismatch_array[i]+1;

    moeglichkeiten = k_aus_n(mp_gl_awars.no_of_probes, n);

    if (moeglichkeiten <= MAXINITPOPULATION)
    {
        act_generation->calc_fitness(NO_GENETIC_ALG);
        act_generation->check_for_results();
        return;
    }

    check_status(0, 0.0, 0.0, 0.0);



    // hier beginnt der genetische Algorithmus
    do
    {
        act_generation->calc_fitness(0, avg_fit);       // hier wird auch init_roulette_wheel gemacht
        avg_fit = act_generation->get_avg_fit();

        if (avg_fit == 0 && !Stop_evaluation)
        {
            aw_message("Please choose better Probes!");
            return;
        }

        if (Stop_evaluation)            //Abgebrochen durch Benutzer
        {
            Stop_evaluation = FALSE;
            act_generation->check_for_results();
            return;
        }

        child_generation = act_generation->create_next_generation();
        delete act_generation;
        act_generation = NULL;


        child_generation->check_for_results();

        act_generation = child_generation;  // zum testen hier nur generierung einer Generation
    }
    while (moeglichkeiten/3 > act_generation->get_generation() * MAXPOPULATION); //hier abbruchbedingung

    //Abbruchbedingung deshalb, weil der genetische Alg. keinesfalls mehr Versuche
    // benoetigt als sequentielles probieren. Hier: Annahme, dass er max.
    // ein drittel der Versuche benoetigt

    if (act_generation)
        act_generation->check_for_results();
}

void ProbeValuation::insert_in_result_list(probe_combi_statistic *pcs)      //pcs darf nur eingetragen werden, wenn es nicht schon vorhanden ist
{
    char    *new_list_string,
        *misms,
        *probe_string,
        temp_misms[3];
    int     probe_len = 0;
    char      buf[25];
    char    ecoli_pos[40],temp_ecol[10];
    int buf_len, i;
    result_struct *rs = new result_struct,
        *elem;
    AW_window   *aww = mp_main->get_mp_window()->get_result_window();

    memset(rs, 0, sizeof(result_struct));
    memset(ecoli_pos,0, 40);

    for (i=0; i<mp_gl_awars.no_of_probes; i++)
        probe_len += strlen(sondenarray[pcs->get_probe_combi(i)->probe_index]) + 1; //1 fuer space bzw. 0-Zeichen

    misms = new char[2*mp_gl_awars.no_of_probes+1];
    probe_string = new char[probe_len+1];

    probe_string[0] = misms[0] = 0;
    for (i=0; i<mp_gl_awars.no_of_probes; i++)
    {
        if (i>0)
        {
            strcat(misms," ");
            strcat(probe_string," ");
        }


        sprintf(temp_misms,"%1d",pcs->get_probe_combi(i)->allowed_mismatches);
        strcat(misms, temp_misms);
        sprintf(temp_ecol,"%6d ",pcs->get_probe_combi(i)->e_coli_pos);
        strcat(ecoli_pos,temp_ecol);
        strcat(probe_string, sondenarray[pcs->get_probe_combi(i)->probe_index]);
    }

    ecoli_pos[strlen(ecoli_pos)-1] = 0;

    new_list_string = new char[21+
                               probe_len+
                               2*mp_gl_awars.no_of_probes+2+
                               2*strlen(SEPARATOR)+ strlen(ecoli_pos)+
                               1];  //1 fuer 0-Zeichen

    sprintf(buf,"%f",pcs->get_fitness());
    buf_len= strlen(buf);
    for(i=0;i<20-buf_len;i++)
        strcat(buf," ");

    sprintf(new_list_string,"%20s%s%s%s%s%s%s",buf,SEPARATOR,misms,SEPARATOR,ecoli_pos,SEPARATOR,probe_string);
    delete [] misms;
    delete [] probe_string;

    rs->ps = pcs->duplicate();
    rs->view_string = new_list_string;
    elem = computation_result_list->get_first();
    if (! elem)
        computation_result_list->insert_as_first(rs);
    else
    {
        while (elem)                    //Liste ist sortiert von groesster Fitness bis kleinster Fitness
        {
            if (strcmp(elem->view_string, new_list_string) == 0)
            {
                delete new_list_string;
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



    aww->clear_selection_list( result_probes_list );

    elem = computation_result_list->get_first();
    while (elem)
    {
        aww->insert_selection( result_probes_list, elem->view_string, elem->view_string );
        elem = computation_result_list->get_next();
    }

    aww->insert_default_selection( result_probes_list, "", "");
    aww->update_selection_list( result_probes_list );
}

void ProbeValuation::init_valuation()
{
    int         i, j, k, counter=0;
    probe       *temp_probe;
    ST_Container    *stc;
    AW_window       *aww;
    char        *ptr, *ptr2;

    if (new_pt_server)
    {
        new_pt_server = FALSE;

        if (mp_main->get_stc())
            delete mp_main->get_stc();

        aw_status("Starting computation");
        mp_main->set_stc(new ST_Container(MAXSONDENHASHSIZE));
    }

    if (pt_server_different)
    {
        mp_main->set_stc(NULL);
        new_pt_server = TRUE;
        return;
    }

    stc = mp_main->get_stc();
    aww = mp_main->get_mp_window()->get_window();
    aww->init_list_entry_iterator(selected_list); //initialisieren

    if (max_init_pop_combis < MAXINITPOPULATION)
    {
        for (i=0; i<size_sonden_array; i++)         // generierung eines pools, in dem jede Sonde nur einmal pro Mismatch
        {                           // vorkommt, damit alle moeglichen Kombinationen deterministisch
            ptr2 = (char *)aww->get_list_entry_char_value();
            aww->iterate_list_entry(1);

            for(j=0; j<=mismatch_array[i]; j++)         // generiert werden koennen.
            {
                temp_probe = new probe;
                temp_probe->probe_index = i;
                temp_probe->allowed_mismatches = j;
                temp_probe->e_coli_pos = atoi(ptr = MP_get_comment(3,ptr2));
                free(ptr);

                probe_pool[counter++] = temp_probe;
            }
        }
        pool_length = counter;

        act_generation->init_valuation();

        evolution();
        aww = mp_main->get_mp_window()->get_result_window();
        aww->show();
        return;
    }

    for (i=0; i<size_sonden_array; i++)
    {                               // Generierung eines Pools, in dem die Wahrscheinlichkeiten fuer die Erfassung
        ptr2 = (char *)aww->get_list_entry_char_value();
        aww->iterate_list_entry(1);

        for(j=0; j<=mismatch_array[i]; j++)         // der Sonden schon eingearbeitet sind. DIe WS werden vom Benutzer fuer jedE
        {                           // einzelne Sonde bestimmt
            for (k=0; k < bewertungarray[i]; k++)
            {
                temp_probe = new probe;
                temp_probe->probe_index = i;
                temp_probe->allowed_mismatches = j;
                temp_probe->e_coli_pos = atoi(ptr = MP_get_comment(3,ptr2));
                delete ptr;

                probe_pool[counter++] = temp_probe;
            }
        }
    }


    act_generation->init_valuation();

    evolution();
    aww = mp_main->get_mp_window()->get_result_window();
    aww->show();
}



ProbeValuation::ProbeValuation(char **sonden_array, int no_of_sonden, int *bewertung, int *mismatch)
{

    memset(this, 0, sizeof(ProbeValuation));

    sondenarray     = sonden_array;
    bewertungarray  = bewertung;
    size_sonden_array   = no_of_sonden;
    mismatch_array  = mismatch;

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

    for (i=0; i<size_sonden_array; i++)
        free(sondenarray[i]);

    for (i=0; i<pool_length; i++)
        delete probe_pool[i];

    elem = computation_result_list->get_first();
    while (elem)
    {
        computation_result_list->remove_first();
        delete [] elem->view_string;
        delete elem;
        elem = computation_result_list->get_first();
    }

    delete computation_result_list;

    if (act_generation == child_generation)
        delete act_generation;
    else
    {
        delete act_generation;
        delete child_generation;
    }

    delete [] sondenarray;
    delete [] bewertungarray;
    delete [] mismatch_array;
    delete [] probe_pool;
}

