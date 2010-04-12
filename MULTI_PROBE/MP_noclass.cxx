// =============================================================== //
//                                                                 //
//   File      : MP_noclass.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "MultiProbe.hxx"
#include "mp_proto.hxx"
#include "MP_externs.hxx"
#include "MP_probe.hxx"

#include <awt.hxx>
#include <aw_select.hxx>
#include <TreeCallbacks.hxx>
#include <client.h>
#include <servercntrl.h>

#include <ctime>

bool MP_is_probe(char *seq);
int  get_random(int min, int max); // gibt eine Zufallszahl x mit der Eigenschaft : min <= x <= max
void init_system3_tab();

int        **system3_tab      = NULL;
static int   system3_tab_size = 0;

unsigned char **hamming_tab   = NULL;
bool            new_pt_server = true;
struct Params   P;

long k_aus_n(int k, int n) {
    int a = n, b = 1, i;

    if (k > (n / 2)) k = n - k;
    if (k <= 0) return (k == 0);
    for (i = 2; i <= k; n--, a *= n, b *= i, i++) ;
    return a / b;
}

int get_random(int min, int max) {
    mp_assert(min <= max);
    return GB_random(max-min+1)+min;
}

void MP_new_sequence(AW_window *aww) {
    AWUSE(aww);
    mp_main->get_aw_root()->awar_string(MP_AWAR_SEQUENZEINGABE)->write_string("");

    char *res = aw_input2awar("Enter target sequence", MP_AWAR_SEQUENZEINGABE);
    free(res);

    MP_take_manual_sequence(aww);
}


void MP_close_main(AW_window *aww)
{
    AWT_canvas  *ntw = mp_main->get_ntw();

    if (mp_main->get_mp_window()->get_result_window())
        mp_main->get_mp_window()->get_result_window()->hide();

    GB_transaction dummy(ntw->gb_main);

    AP_tree *ap_tree = AWT_TREE(ntw)->get_root_node();
    if (ap_tree) ap_tree->calc_color();

    if (ntw->gb_main)
        ntw->tree_disp->update(ntw->gb_main);

    ntw->refresh();

    AW_POPDOWN(aww);
    delete mp_main->get_p_eval();
    mp_main->set_p_eval(NULL);
    delete mp_main->get_stc();
    mp_main->set_stc(NULL);

    new_pt_server = true;
}

void MP_gen_quality(AW_root *awr, AW_CL cd1, AW_CL cd2)
{
    bool firsttime = true;

    AWUSE(cd1);
    AWUSE(cd2);

    if (firsttime)
    {
        firsttime = false;
        return;
    }

    char *probe, *new_qual, *ecol_pos;
    char *selected = awr->awar(MP_AWAR_SELECTEDPROBES)->read_string();
    AW_window *aww = mp_main->get_mp_window()->get_window();

    if (!selected || !selected[0])
        return;

    probe = MP_get_probes(selected);
    if (!probe || !probe[0])
        return;

    ecol_pos = MP_get_comment(3, selected);

    aww->delete_selection_from_list(selected_list, selected);

    new_qual = new char[5 + 7 + strlen(probe)];     // 5 = Zahl und Separator und Zahl und Separator und Nullzeichen
    sprintf(new_qual, "%1ld#%1ld#%6d#%s", mp_gl_awars.probe_quality, mp_gl_awars.singlemismatches, atoi(ecol_pos), probe);
    delete probe;

    aww->insert_selection(selected_list, new_qual, new_qual);
    aww->insert_default_selection(selected_list, "", "");
    aww->sort_selection_list(selected_list, 0, 1);
    aww->update_selection_list(selected_list);
    awr->awar(MP_AWAR_SELECTEDPROBES)->write_string(new_qual);
    delete new_qual;
    delete ecol_pos;
}

void MP_modify_selected(AW_root *awr, AW_CL cd1, AW_CL cd2) // setzt den 2.Parameter in selected_list
{
    char    *com1, *com2, *com3, *probes, *ptr2, temp[120];
    AW_window   *aww = mp_main->get_mp_window()->get_window();
    List<char>  *l = new List<char>;

    AWUSE(cd1);
    AWUSE(cd2);

    aww->init_list_entry_iterator(selected_list); // initialisieren

    while ((ptr2 = (char *)aww->get_list_entry_char_value())) {
        aww->iterate_list_entry(1);

        com1 =  MP_get_comment(1, ptr2);
        com2 =  MP_get_comment(2, ptr2);
        com3 =  MP_get_comment(3, ptr2);
        probes = MP_get_probes(ptr2);

        if (!probes || !probes[0])
            break;

        sprintf(temp, "%1d#%1ld#%6d#%s", atoi(com1), mp_gl_awars.no_of_mismatches, atoi(com3), probes);

        l->insert_as_last(strdup(temp));

        delete probes;
        delete com1;
        delete com2;
        delete com3;
    }

    aww->clear_selection_list(selected_list);

    ptr2 = l->get_first();
    while (ptr2)
    {
        l->remove_first();
        aww->insert_selection(selected_list, ptr2, ptr2);
        delete ptr2;
        ptr2 = l->get_first();
    }

    aww->insert_default_selection(selected_list, "", "");
    aww->sort_selection_list(selected_list, 0, 1);
    aww->update_selection_list(selected_list);
    awr->awar(MP_AWAR_SELECTEDPROBES)->write_string("");

    remembered_mismatches = mp_gl_awars.no_of_mismatches;
    delete l;
}

void MP_gen_singleprobe(AW_root *awr, AW_CL cd1, AW_CL cd2)
{
    AWUSE(cd1);
    AWUSE(cd2);

    char *probe, *new_sing;
    char *selected = awr->awar(MP_AWAR_SELECTEDPROBES)->read_string();
    AW_window *aww = mp_main->get_mp_window()->get_window();

    if (!selected || !selected[0])
        return;

    probe = MP_get_probes(selected);
    aww->delete_selection_from_list(selected_list, selected);

    new_sing = new char[5 + 7 + strlen(probe)];     // 5 = Zahl und Separator und Zahl und Separator und Nullzeichen
    sprintf(new_sing, "%1ld#%1ld#%6ld#%s", mp_gl_awars.probe_quality, mp_gl_awars.singlemismatches, mp_gl_awars.ecolipos, probe);
    delete probe;

    aww->insert_selection(selected_list, new_sing, new_sing);
    aww->insert_default_selection(selected_list, "", "");
    aww->sort_selection_list(selected_list, 0, 1);
    aww->update_selection_list(selected_list);
    awr->awar(MP_AWAR_SELECTEDPROBES)->write_string(new_sing);
    delete new_sing;
}

void MP_popup_result_window(AW_window *aww) {
    AWUSE(aww);
    mp_main->get_mp_window()->create_result_window(mp_main->get_aw_root())->activate();
    init_system3_tab();
}

bool MP_check_status(int gen_cnt, double avg_fit, double min_fit, double max_fit) {
    char view_text[150];

    if (gen_cnt == 0)
        sprintf(view_text, "Evaluating first generation");
    else
        sprintf(view_text, "Gen:%d Avg:%5i Min:%5i Max:%5i", gen_cnt, int(avg_fit), int(min_fit), int(max_fit));

    if (aw_status(view_text) == 1)
    {
        aw_closestatus();
        return false;                   // Berechnungen abbrechen !!!!!!
    }

    return true;
}

void init_system3_tab()
{
    int i, j, k,
        size_hamming_tab,
        hamm_dist;
    int **dummy_int;
    int counter, wert;

    if (system3_tab) {
        for (j=0; j< system3_tab_size; j++)
            delete [] system3_tab[j];

        delete [] system3_tab;
    }
    system3_tab      = new int*[mp_gl_awars.no_of_probes];
    system3_tab_size = mp_gl_awars.no_of_probes;
    for (j=0; j<mp_gl_awars.no_of_probes; j++)
    {
        system3_tab[j] = new int[3];
        memset(system3_tab[j], 0, sizeof(int) * 3);
    }

    for (j=0; j< mp_gl_awars.no_of_probes; j++)
    {
        for (k=0; k<3; k++)
        {
            if (!j) system3_tab[j][k] = k;
            else    system3_tab[j][k] = system3_tab[j-1][k] * 3;
        }
    }


    // ** hamming_tab
        if (hamming_tab)
            delete [] hamming_tab;

        size_hamming_tab = (int)pow(3.0, (double) mp_gl_awars.no_of_probes);
        hamming_tab = new unsigned char*[size_hamming_tab];
        dummy_int = new int*[size_hamming_tab];

        for (i=0; i<size_hamming_tab; i++)
        {
            hamming_tab[i] = new unsigned char[size_hamming_tab];
            memset(hamming_tab[i], 0, sizeof(unsigned char) * size_hamming_tab);

            dummy_int[i] = new int[mp_gl_awars.no_of_probes];
            memset(dummy_int[i], 0, sizeof(int) * mp_gl_awars.no_of_probes);
        }

        counter = 1;
        for (i=0; i< mp_gl_awars.no_of_probes; i++)
        {
            for (j=0; j<size_hamming_tab; j++)
            {
                for (wert = 0; wert < 3; wert++)
                {
                    for (k=0; k<counter; k++)
                    {
                        dummy_int[j++][i] = wert;
                    }
                }
                j--;
            }
            counter *= 3;
        }

        for (i=0; i<size_hamming_tab; i++)
        {
            for (j=0; j<size_hamming_tab; j++)
            {
                hamm_dist = 0;
                for (k=0; k<mp_gl_awars.no_of_probes; k++)
                {
                    if ((dummy_int[i][k] == 2 && dummy_int[j][k] == 0) ||
                        (dummy_int[i][k] == 0 && dummy_int[j][k] == 2))
                        hamm_dist++;
                }
                hamming_tab[i][j] = hamm_dist;
            }
        }

        for (i=0; i<size_hamming_tab; i++)
        {
            delete [] dummy_int[i];
        }
        delete [] dummy_int;



}

void MP_compute(AW_window *aww, AW_CL cl_gb_main) {
    AW_root         *aw_root = mp_main->get_aw_root();
    AW_window       *aww2;
    int              i       = 0;
    int             *bew_array;
    char            *ptr, *ptr2, *qual;
    char           **probe_field;
    int             *single_mismatch;
    ProbeValuation  *p_eval  = NULL;
    GBDATA          *gb_main = (GBDATA*)cl_gb_main;

    MO_Liste::set_gb_main(gb_main);

    size_t selected_probes_count = selected_list->size();
    if ((int)selected_probes_count < mp_gl_awars.no_of_probes) {
        aw_message("Not enough probes selected for computation !!!");
        return;
    }

    if (mp_main->get_stc())
    {
        delete mp_main->get_stc();
        mp_main->set_stc(NULL);
        new_pt_server = true;
    }

    init_system3_tab();

    aww2 = mp_main->get_mp_window()->create_result_window(aw_root);

    aww2->clear_selection_list(result_probes_list);
    aww2->insert_default_selection(result_probes_list, "", "");
    aww2->update_selection_list(result_probes_list);

    aww->init_list_entry_iterator(selected_list);   // initialisieren
    probe_field     = new char*[selected_probes_count];
    bew_array       = new int[selected_probes_count];
    single_mismatch = new int[selected_probes_count];

    aw_openstatus("Computing multiprobes");

    while ((ptr2 = (char *)aww->get_list_entry_char_value()))
    {
        aww->iterate_list_entry(1);
        ptr = MP_get_probes(ptr2);      // hier sind es einfachsonden
        if (ptr && ptr[0] != ' ' && ptr[0] != '\t' && ptr[0] != '\0') {
            qual         = MP_get_comment(1, ptr2);
            bew_array[i] = atoi(qual);
            free(qual);

            qual               = MP_get_comment(2, ptr2);
            single_mismatch[i] = atoi(qual); // single mismatch kann zwar eingestellt werden, aber wird noch nicht uebergeben
            free(qual);

            probe_field[i++] = ptr;
        }
    }

    p_eval = mp_main->new_probe_eval(probe_field, i, bew_array, single_mismatch);
    p_eval->init_valuation();

    if (pt_server_different)
    {
        pt_server_different = false;
        aw_message("There are species in the tree which are\nnot included in the PT-Server");
    }

    mp_main->destroy_probe_eval();

    aw_closestatus();
    aww2->activate();

}

void    MP_take_manual_sequence(AW_window *aww)
{
    AW_window_simple    *aws = mp_main->get_mp_window()->get_window();
    char        *seq = mp_gl_awars.manual_sequence,
        *new_seq;

    AWUSE(aww);

    if (! MP_is_probe(seq)) {
        aw_message("This is not a valid probe !!!");
        return;
    }

    new_seq = new char[strlen(seq)+5+7];
    sprintf(new_seq, "%1ld#%1ld#%6d#%s", mp_gl_awars.probe_quality, mp_gl_awars.singlemismatches, 0, seq);

    aws->insert_selection(selected_list, new_seq, new_seq);

    aws->insert_default_selection(selected_list, "", "");
    aws->sort_selection_list(selected_list, 0, 1);
    aws->update_selection_list(selected_list);
    mp_main->get_aw_root()->awar(MP_AWAR_SEQUENZEINGABE)->write_string("");
    mp_main->get_aw_root()->awar(MP_AWAR_SELECTEDPROBES)->write_string(new_seq);
    delete [] new_seq;
}


void MP_cache_sonden(AW_window *) { new_pt_server = true; }
void MP_cache_sonden2(AW_root *) { new_pt_server = true; }

void MP_show_probes_in_tree_move(AW_window *aww, AW_CL cl_backward, AW_CL cl_result_probes_list) {
    bool               backward           = bool(cl_backward);
    AW_selection_list *resultProbesList = (AW_selection_list*)cl_result_probes_list;

    aww->move_selection(resultProbesList, backward ? -1 : 1);
    MP_show_probes_in_tree(aww);
}

void MP_show_probes_in_tree(AW_window *aww)
{
    AWT_canvas *ntw                = mp_main->get_ntw();
    char       *mism, *mism_temp;
    char       *a_probe, *another_probe, *the_probe, *mism_temp2;
    int         i, how_many_probes = 0;

    AWUSE(aww);

    {
        char *sel = mp_main->get_aw_root()->awar(MP_AWAR_RESULTPROBES)->read_string();
        a_probe   = MP_get_probes(sel); // haelt jetzt Sondenstring
        if (! a_probe || ! a_probe[0]) {
            free(a_probe);
            free(sel);
            return;
        }

        mism_temp2 = MP_get_comment(2, sel);
        mism_temp  = mism_temp2;
        free(sel);
    }

    char **probe_field = new char*[MAXMISMATCHES];
    int   *mismatches  = new int[MAXMISMATCHES];

    for (i=0; i<MAXMISMATCHES; i++)
    {
        probe_field[i] = NULL;
        mismatches[i] = 0;
    }

    another_probe = a_probe;
    for (i=0; i< MAXPROBECOMBIS; i++)       // generiert  probe_field und mismatches array
    {
        mism = strchr(mism_temp, ' ');
        if (mism)
            *(mism++) = 0;

        mismatches[i] = atoi(mism_temp);
        mism_temp = mism;


        probe_field[i] = NULL;
        the_probe = another_probe;
        another_probe = strchr(another_probe, ' ');
        if (another_probe)
        {
            *(another_probe++) = 0;
            while (*another_probe == ' ' || *another_probe == '\t')
                another_probe++;
        }
        else
        {
            probe_field[i] = strdup(the_probe);
            how_many_probes ++;
            break;
        }

        if (the_probe && the_probe[0])
        {
            probe_field[i] = strdup(the_probe);
            how_many_probes ++;
        }
    }

    free(a_probe);
    free(mism_temp2);

    if (new_pt_server)
    {
        new_pt_server = false;

        if (mp_main->get_stc())
            delete mp_main->get_stc();

        mp_main->set_stc(new ST_Container(MAXSONDENHASHSIZE));
        if (pt_server_different)
        {
            mp_main->set_stc(NULL);
            new_pt_server = true;
            aw_message("There are species in the tree which are\nnot included in the PT-Server");
            pt_server_different = false;
            return;
        }
    }

    delete mp_main->get_stc()->sondentopf;
    mp_main->get_stc()->sondentopf = new Sondentopf(mp_main->get_stc()->Bakterienliste, mp_main->get_stc()->Auswahlliste);

    for (i=0; i<MAXMISMATCHES; i++) {
        if (probe_field[i]) {
            mp_main->get_stc()->sondentopf->put_Sonde(probe_field[i], mismatches[i], mismatches[i] + mp_gl_awars.outside_mismatches_difference);
        }
    }

    mp_main->get_stc()->sondentopf->gen_color_hash(mp_gl_awars.no_of_probes);

    GB_transaction dummy(ntw->gb_main);
    AWT_TREE(ntw)->get_root_node()->calc_color_probes(mp_main->get_stc()->sondentopf->get_color_hash());

    if (ntw->gb_main)
        ntw->tree_disp->update(ntw->gb_main);

    ntw->refresh();

    for (i=0; i<MAXMISMATCHES; i++)
        free(probe_field[i]);

    delete [] probe_field;
    delete [] mismatches;
}

void MP_mark_probes_in_tree(AW_window *aww)
{
    AWT_canvas  *ntw                = mp_main->get_ntw();
    char        *mism, *mism_temp;
    char        *a_probe, *another_probe, *the_probe, *mism_temp2;
    int          i, how_many_probes = 0;
    GBDATA      *gb_species;

    AWUSE(aww);

    {
        char *sel = mp_main->get_aw_root()->awar(MP_AWAR_RESULTPROBES)->read_string();
        a_probe   = MP_get_probes(sel);             // haelt jetzt Sondenstring

        if (! a_probe || ! a_probe[0]) {
            free(a_probe);
            free(sel);
            return;
        }

        mism_temp2 = MP_get_comment(2, sel);
        mism_temp  = mism_temp2;
        free(sel);
    }

    char **probe_field = new char*[MAXMISMATCHES];
    int   *mismatches  = new int[MAXMISMATCHES];

    for (i=0; i<MAXMISMATCHES; i++)
    {
        probe_field[i] = NULL;
        mismatches[i] = 0;
    }

    another_probe = a_probe;
    for (i=0; i< MAXPROBECOMBIS; i++)       // generiert  probe_field und mismatches array
    {
        mism = strchr(mism_temp, ' ');
        if (mism)
            *(mism++) = 0;

        mismatches[i] = atoi(mism_temp);
        mism_temp = mism;

        probe_field[i] = NULL;
        the_probe = another_probe;
        another_probe = strchr(another_probe, ' ');
        if (another_probe) {
            *(another_probe++) = 0;
            while (*another_probe == ' ' || *another_probe == '\t')
                another_probe++;
        }
        else {
            probe_field[i] = strdup(the_probe);
            how_many_probes ++;
            break;
        }

        if (the_probe && the_probe[0]) {
            probe_field[i] = strdup(the_probe);
            how_many_probes ++;
        }
    }

    free(a_probe);
    free(mism_temp2);

    if (new_pt_server) {
        new_pt_server = false;

        if (mp_main->get_stc())
            delete mp_main->get_stc();

        mp_main->set_stc(new ST_Container(MAXSONDENHASHSIZE));
        if (pt_server_different)
        {
            mp_main->set_stc(NULL);
            new_pt_server = true;
            aw_message("There are species in the tree which are\nnot included in the PT-Server");
            pt_server_different = false;
            return;
        }
    }

    delete mp_main->get_stc()->sondentopf;
    mp_main->get_stc()->sondentopf = new Sondentopf(mp_main->get_stc()->Bakterienliste, mp_main->get_stc()->Auswahlliste);

    for (i=0; i<MAXMISMATCHES; i++) {
        if (probe_field[i]) {
            mp_main->get_stc()->sondentopf->put_Sonde(probe_field[i], mismatches[i], mismatches[i] + mp_gl_awars.outside_mismatches_difference);
        }
    }
    mp_main->get_stc()->sondentopf->gen_color_hash(mp_gl_awars.no_of_probes);

    {
        GB_push_transaction(ntw->gb_main);
        GB_HASH *col_hash = mp_main->get_stc()->sondentopf->get_color_hash();
        for (gb_species = GBT_first_species(ntw->gb_main); gb_species; gb_species = GBT_next_species(gb_species)) {
            GB_write_flag(gb_species, GBS_read_hash(col_hash, GBT_read_name(gb_species)) > AWT_GC_BLACK);
        }
    }
    GB_pop_transaction(ntw->gb_main);

    GB_transaction dummy(ntw->gb_main);

    if (ntw->gb_main)
        ntw->tree_disp->update(ntw->gb_main);

    ntw->refresh();

    for (i=0; i<MAXMISMATCHES; i++)
        free(probe_field[i]);

    delete [] probe_field;
    delete [] mismatches;

    MP_normal_colors_in_tree(aww);
}

void MP_Comment(AW_window *aww, AW_CL cl_com) {
    // Comment fuer Auswahl eintragen

    AW_root *awr       = mp_main->get_aw_root();
    char    *selprobes = awr->awar(MP_AWAR_RESULTPROBES)->read_string();
    char    *new_val   = MP_get_probes(selprobes);

    if (new_val && new_val[0]) {
        char *new_list_string;
        {
            char       *edited_comment = NULL;
            const char *comment        = NULL;

            if (cl_com) {
                comment = (char*)cl_com;
            }
            else {
                edited_comment = awr->awar(MP_AWAR_RESULTPROBESCOMMENT)->read_string();
                for (int i = 0; edited_comment[i]; ++i) { // remove all '#' from new comment
                    if (edited_comment[i] == SEPARATOR[0]) {
                        edited_comment[i] = '|';
                    }
                }
            }

            char *misms = MP_get_comment(2, selprobes);
            char *ecol  = MP_get_comment(3, selprobes);

            new_list_string = GBS_global_string_copy("%-20s" SEPARATOR "%s" SEPARATOR "%s" SEPARATOR "%s",
                                                     comment, misms, ecol, new_val);
            free(ecol);
            free(misms);
            free(edited_comment);
        }

        {
            bool autoadvance = awr->awar(MP_AWAR_AUTOADVANCE)->read_int();

            if (autoadvance) aww->move_selection(result_probes_list, 1);

            aww->delete_selection_from_list(result_probes_list, selprobes);
            aww->insert_selection(result_probes_list, new_list_string, new_list_string);
            aww->update_selection_list(result_probes_list);

            if (!autoadvance) awr->awar(MP_AWAR_RESULTPROBES)->write_string(new_list_string);
        }
        free(new_list_string);
    }

    free(new_val);
    free(selprobes);
}

void MP_leftright(AW_window *aww) {
    char *sel = mp_main->get_aw_root()->awar(MP_AWAR_PROBELIST)->read_string();

    if (sel && sel[0]) {
        aww->insert_selection(selected_list, sel, sel);
        aww->delete_selection_from_list(probelist, sel);
        aww->insert_default_selection(probelist, "", "");
        mp_main->get_aw_root()->awar(MP_AWAR_PROBELIST)->write_string("");
        aww->sort_selection_list(selected_list, 0, 1);
        aww->update_selection_list(selected_list);
        aww->update_selection_list(probelist);
    }
    free(sel);
}

void    MP_rightleft(AW_window *aww)    // von rechts nach links
{
    char *sel = mp_main->get_aw_root()->awar(MP_AWAR_SELECTEDPROBES)->read_string();

    if (!sel ||  !sel[0])
        return;

    aww->insert_selection(probelist, sel, sel);
    aww->delete_selection_from_list(selected_list, sel);
    aww->insert_default_selection(selected_list, "", "");
    mp_main->get_aw_root()->awar(MP_AWAR_SELECTEDPROBES)->write_string("");
    aww->update_selection_list(selected_list);
    aww->sort_selection_list(probelist, 0, 1);
    aww->update_selection_list(probelist);
}

bool MP_is_probe(char *seq)
{
    bool    result=true;
    char    *s,
        *seq2;

    if (! seq)
        return false;

    seq2 = MP_get_probes(seq);
    if (!seq2 || ! seq2[0])
        return false;

    s = seq2;
    while (*s && result)
    {
        result = result && MP_probe_tab[(unsigned char)*s];
        s++;
    }
    free(seq2);
    return result;
}

void MP_selected_chosen(AW_window *aww)
{
    char *selected = mp_main->get_aw_root()->awar(MP_AWAR_SELECTEDPROBES)->read_string();
    char *probe;

    AWUSE(aww);

    if (!selected || !selected[0])
        return;

    probe = MP_get_comment(3, selected);
    mp_main->get_aw_root()->awar(MP_AWAR_ECOLIPOS)->write_int(atoi(probe));
    free(probe);

    probe = MP_get_comment(1, selected);
    mp_main->get_aw_root()->awar(MP_AWAR_QUALITY)->write_int(atoi(probe));
    free(probe);

    probe = MP_get_comment(2, selected);
    mp_main->get_aw_root()->awar(MP_AWAR_SINGLEMISMATCHES)->write_int(atoi(probe));
    free(probe);

    probe = MP_get_probes(selected);
    mp_main->get_aw_root()->awar_string(MP_AWAR_SEQUENZEINGABE)->write_string(MP_get_probes(probe));
    free(probe);
}

void MP_group_all_except_marked(AW_window * /* aww */) {
    AWT_canvas  *ntw = mp_main->get_ntw();
    NT_group_not_marked_cb(0, ntw);
}

void MP_normal_colors_in_tree(AW_window *aww)
{
    AWT_canvas  *ntw = mp_main->get_ntw();

    AWUSE(aww);

    GB_transaction dummy(ntw->gb_main);

    AWT_TREE(ntw)->get_root_node()->calc_color();

    if (ntw->gb_main)
        ntw->tree_disp->update(ntw->gb_main);

    ntw->refresh();
}

void MP_all_right(AW_window *aww)
{
    aww->conc_list(probelist, selected_list);
    mp_main->get_aw_root()->awar(MP_AWAR_PROBELIST)->write_string("");
    aww->sort_selection_list(selected_list, 0, 1);
    aww->update_selection_list(selected_list);
    aww->update_selection_list(probelist);
}

void MP_del_all_sel_probes(AW_window *aww)
{
    mp_main->get_aw_root()->awar(MP_AWAR_SELECTEDPROBES)->write_string("");
    aww->clear_selection_list(selected_list);
    aww->insert_default_selection(selected_list, "", "");
    aww->update_selection_list(selected_list);
}

void MP_del_all_probes(AW_window *aww)
{
    mp_main->get_aw_root()->awar(MP_AWAR_PROBELIST)->write_string("");
    aww->clear_selection_list(probelist);
    aww->insert_default_selection(probelist, "", "");
    aww->update_selection_list(probelist);
}

void MP_del_all_result(AW_window *aww)
{
    mp_main->get_aw_root()->awar(MP_AWAR_RESULTPROBES)->write_string("");
    aww->clear_selection_list(result_probes_list);
    aww->insert_default_selection(result_probes_list, "", "");
    aww->update_selection_list(result_probes_list);
}

void MP_del_sel_result(AW_window *aww)
{
    char *val = mp_main->get_aw_root()->awar(MP_AWAR_RESULTPROBES)->read_string();
    aww->delete_selection_from_list(result_probes_list, val);
    aww->insert_default_selection(result_probes_list, "", "");
    mp_main->get_aw_root()->awar(MP_AWAR_RESULTPROBES)->write_string("");
    aww->update_selection_list(result_probes_list);
}

void MP_del_sel_probes(AW_window *aww)
{
    char *val = mp_main->get_aw_root()->awar(MP_AWAR_SELECTEDPROBES)->read_string();
    aww->delete_selection_from_list(selected_list, val);
    aww->insert_default_selection(selected_list, "", "");
    mp_main->get_aw_root()->awar(MP_AWAR_SELECTEDPROBES)->write_string("");
    aww->update_selection_list(selected_list);
}

void MP_del_probes(AW_window *aww)
{
    char *val = mp_main->get_aw_root()->awar(MP_AWAR_PROBELIST)->read_string();
    aww->delete_selection_from_list(probelist, val);
    aww->insert_default_selection(probelist, "", "");
    mp_main->get_aw_root()->awar(MP_AWAR_PROBELIST)->write_string("");
    aww->update_selection_list(probelist);
}

char *MP_get_comment(int which, const char *str) {
    // Parses information from strings like "val1#val2#val3#probes".
    // 'which' == 1 -> 'val1'
    // 'which' == 2 -> 'val2'
    // ...
    // Values may be present or not (i.e. 'probes', 'val1#probes', ... are also valid as 'str')

    char       *result  = NULL;
    const char *numsign = strchr(str, '#');

    mp_assert(which >= 1);

    if (numsign) {
        if (which == 1) {
            result = GB_strpartdup(str, numsign-1);
        }
        else {
            result = MP_get_comment(which-1, numsign+1);
        }
    }

    return result;
}

char *MP_remove_comment(char *old_str)
{
    char *result = strrchr(old_str, '#');
    char *help;
    char *ret_res;

    if (!result)
        return strdup(old_str);

    *result = 0;
    help = old_str;
    while (*help == ' ' || *help == '\t')
        help ++;

    ret_res = strdup(help);
    *result = '#';

    return ret_res;
}

char *MP_get_probes(const char *str) {
    const char *result = strrchr(str, '#');

    if (!result) {
        result = str;
    }
    else {
        ++result;
        result += strspn(result, " \t"); // skip over whitespace
    }
    return strdup(result);
}

void MP_result_chosen(AW_window *aww)
{
    AW_root     *aw_root    = mp_main->get_aw_root();
    char    *str        = aw_root->awar(MP_AWAR_RESULTPROBES)->read_as_string(),
        *new_str;

    AWUSE(aww);

    new_str = MP_get_comment(1, str);

    aw_root->awar(MP_AWAR_RESULTPROBESCOMMENT)->write_string(new_str);
    free(str);
    free(new_str);
}



//
// functions concerning the server
//

int MP_init_local_com_struct()
{
    const char *user = GB_getenvUSER();

    if (aisc_create(mp_pd_gl.link, PT_MAIN, mp_pd_gl.com,
                    MAIN_LOCS, PT_LOCS, &mp_pd_gl.locs,
                    LOCS_USER, user,
                    NULL)) {
        return 1;
    }
    return 0;
}

const char *MP_probe_pt_look_for_server()
{
    char choice[256];
    sprintf(choice, "ARB_PT_SERVER%ld", mp_gl_awars.ptserver);
#if defined(DEVEL_RALF)
#warning use GBS_ptserver_id_to_choice here ?
#endif // DEVEL_RALF
    GB_ERROR error;
    error = arb_look_and_start_server(AISC_MAGIC_NUMBER, choice, 0);
    if (error) {
        aw_message((char *)error);
        return 0;
    }
    return GBS_read_arb_tcp(choice);
}


int MP_probe_design_send_data(T_PT_PDC  pdc)
{
    if (aisc_put(mp_pd_gl.link, PT_PDC, pdc,
                 PDC_CLIPRESULT,    P.DESIGNCPLIPOUTPUT,
                 NULL)) return 1;
    return 0;
}

// --------------------------------------------------------------------------------

#if (UNIT_TESTS == 1)

#include <test_unit.h>

#define MP_GET_COMMENT_EQUAL(which, str, expect)  \
    do {                                          \
        char *got = MP_get_comment(which, str);   \
        TEST_ASSERT_EQUAL(got, expect);           \
        free(got);                                \
    } while(0)                                    \
        
#define MP_GET_PROBES_EQUAL(str, expect)          \
    do {                                          \
        char *got = MP_get_probes(str);           \
        TEST_ASSERT_EQUAL(got, expect);           \
        free(got);                                \
    } while(0)                                    \

void TEST_MP_get_comment_and_probes() {
    char *probes_only    = strdup("ACGT TGCA ATCG");
    char *probes_comment = strdup("val1#val2#val3#ACGT TGCA ATCG");
    char *probes_1 = strdup("one#     \t    ACGT TGCA ATCG");
    char *probes_2 = strdup("one#two#\tACGT TGCA ATCG");

    MP_GET_PROBES_EQUAL(probes_only,    "ACGT TGCA ATCG");
    MP_GET_PROBES_EQUAL(probes_comment, "ACGT TGCA ATCG");
    MP_GET_PROBES_EQUAL(probes_1,       "ACGT TGCA ATCG");
    MP_GET_PROBES_EQUAL(probes_2,       "ACGT TGCA ATCG");
    
    MP_GET_COMMENT_EQUAL(1, probes_comment, "val1");
    MP_GET_COMMENT_EQUAL(2, probes_comment, "val2");
    MP_GET_COMMENT_EQUAL(3, probes_comment, "val3");

    MP_GET_COMMENT_EQUAL(1, probes_only, NULL);
    MP_GET_COMMENT_EQUAL(2, probes_only, NULL);
    MP_GET_COMMENT_EQUAL(3, probes_only, NULL);

    MP_GET_COMMENT_EQUAL(1, probes_1, "one");
    MP_GET_COMMENT_EQUAL(2, probes_1, NULL);
    MP_GET_COMMENT_EQUAL(3, probes_1, NULL);
    
    MP_GET_COMMENT_EQUAL(1, probes_2, "one");
    MP_GET_COMMENT_EQUAL(2, probes_2, "two");
    MP_GET_COMMENT_EQUAL(3, probes_2, NULL);
}

#endif
