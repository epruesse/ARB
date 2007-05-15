#include <string.h>
#include <stdlib.h>
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_global.hxx>
#include <awt.hxx>
#include <awt_sel_boxes.hxx>

#include "MultiProbe.hxx"
#include <mp_proto.hxx>

struct mp_gl_struct mp_pd_gl;


//**************************************************************************

AW_selection_list *selected_list;   //globale id's fuer
AW_selection_list *probelist;       //Identifizierung der Listen
AW_selection_list *result_probes_list;


AW_window_simple *MP_Window::create_result_window(AW_root *aw_root)
{
    if (result_window) return result_window;

    result_window = new AW_window_simple;
    result_window->init( aw_root, "MULTIPROBE_RESULTS", "MultiProbe combination results");
    result_window->load_xfig("mp_results.fig");

    result_window->button_length(8);
    result_window->at("close");
    result_window->callback(AW_POPDOWN);
    result_window->create_button("CLOSE", "CLOSE");

    result_window->at("DeleteAll");
    result_window->callback(MP_del_all_result);
    result_window->create_button("DELETE_ALL", "DELETE");

    result_window->at("DeleteSel");
    result_window->callback(MP_del_sel_result);
    result_window->create_button("DELETE_SEELECTED", "DELETE");

    result_window->at("box");
    result_window->callback(MP_result_chosen);

    result_probes_list                      = result_window->create_selection_list( MP_AWAR_RESULTPROBES,"ResultProbes","R" );
    result_probes_list->value_equal_display = true; // plain load/save (no # interpretation)

    result_window->set_selection_list_suffix(result_probes_list,"mpr");

    result_window->insert_default_selection(result_probes_list, "","");


    result_window->at("Load");
    result_window->callback(AW_POPUP, (AW_CL)create_load_box_for_selection_lists, (AW_CL)result_probes_list );
    result_window->create_button("LOAD_RPL", "LOAD");

    result_window->at("Save");
    result_window->callback( AW_POPUP, (AW_CL)create_save_box_for_selection_lists, (AW_CL)result_probes_list);
    result_window->create_button("SAVE_RPL", "SAVE");

    result_window->button_length(7);
    result_window->at("Help");
    result_window->callback(AW_POPUP_HELP,(AW_CL)"multiproberesults.hlp");
    result_window->create_button("HELP","HELP");

    // change comment :

    result_window->button_length(8);

    result_window->at("Trash");
    result_window->callback(MP_Comment, (AW_CL) "Bad");
    result_window->create_button("MARK_AS_BAD", "BAD");

    result_window->at("Good");
    result_window->callback(MP_Comment, (AW_CL) "???");
    result_window->create_button("MARK_AS_GOOD", "???");

    result_window->at("Best");
    result_window->callback(MP_Comment, (AW_CL) "Good");
    result_window->create_button("MARK_AS_BEST", "Good");

    result_window->at("Comment");
    result_window->callback(MP_Comment, (AW_CL) NULL);
    result_window->create_input_field(MP_AWAR_RESULTPROBESCOMMENT);

    result_window->at("auto");
    result_window->create_toggle(MP_AWAR_AUTOADVANCE);

    // tree actions :

    result_window->button_length(3);

    result_window->at("ct_back");
    result_window->callback(MP_show_probes_in_tree_move, (AW_CL)1, (AW_CL)result_probes_list);
    result_window->create_button("COLOR_TREE_BACKWARD", "#rightleft_small.bitmap");

    result_window->at("ct_fwd");
    result_window->callback(MP_show_probes_in_tree_move, (AW_CL)0, (AW_CL)result_probes_list);
    result_window->create_button("COLOR_TREE_FORWARD", "#leftright_small.bitmap");

    result_window->button_length(8);

    result_window->at("ColorTree");
    result_window->button_length(4);
    result_window->callback(MP_show_probes_in_tree);
    result_window->create_button("COLOR_TREE", "GO");

    result_window->at("MarkTree");
    result_window->callback(MP_mark_probes_in_tree);
    result_window->create_button("MARK_TREE", "GO");

    result_window->at("GroupAll");
    result_window->callback(MP_group_all_except_marked);
    result_window->create_button("GROUP_UNMARKED", "GO");

    result_window->at("StandardColor");
    result_window->callback(MP_normal_colors_in_tree);
    result_window->create_button("RESET_COLORS", "GO");

    return result_window;
}

static const char *parse_word(const char *& line, int& wordlen) {
    gb_assert(line);
    
    while (line[0] == ' ') ++line; // eat whitespace
    if (line[0] == 0) return 0; // at EOL

    const char *behind_word       = strchr(line, ' ');
    if (!behind_word) behind_word = strchr(line, 0); // get EOL
    gb_assert(behind_word);

    wordlen = behind_word-line;
    gb_assert(wordlen);

    static char *word_buffer = 0;
    if (word_buffer) free(word_buffer);
    word_buffer              = (char*)malloc(wordlen+1);
    memcpy(word_buffer, line, wordlen);
    word_buffer[wordlen]     = 0;

    line = behind_word;

    return word_buffer;
}

static GB_ERROR parse_probe_list_entry(const char *one_line, char*& probe_string, int& ecoli_position) {
    const char *start  = one_line;
    const char *reason = "more tokens expected";
    int         wordlen;
    const char *word   = parse_word(one_line, wordlen);
    probe_string       = 0;

    if (word) {
        int target_length = wordlen;
        word              = parse_word(one_line, wordlen);
        if (word) {
            int length = atoi(word);

            if (length != target_length) {
                reason = "Length mismatch between 'Target' and 'le'";
            }
            else {
                word = parse_word(one_line, wordlen);
                if (word && wordlen == 2) { // spaces between 'A=' and number
                    word = parse_word(one_line, wordlen); // parse number
                }
                if (word) word = parse_word(one_line, wordlen); // parse ecoli
                if (word) {
                    ecoli_position = atoi(word);
                    if (word) word = parse_word(one_line, wordlen); // parse 'grps'
                    if (word) word = parse_word(one_line, wordlen); // parse 'G+C'
                    if (word) word = parse_word(one_line, wordlen); // parse '4GC+2AT'
                    if (word) word = parse_word(one_line, wordlen); // parse 'Probe sequence'
                    if (word) {
                        if (wordlen != target_length) {
                            reason = "Length mismatch between 'Target' and 'Probe sequence'";
                        }
                        else {
                            probe_string = strdup(word);
                            return 0; // success
                        }
                    }
                }
            }
        }
    }

    return GBS_global_string("can't parse line '%s' (Reason: %s)", start, reason);
}


void mp_load_list( AW_window *aww, AW_selection_list *selection_list, char *base_name)
{
    aww->clear_selection_list(selection_list);
    char *data;
    {
        const char *awar_file = GBS_global_string("%s/file_name", base_name);
        char *filename  = aww->get_root()->awar(awar_file)->read_string();
        data            = GB_read_file(filename);
        free(filename);
        if (!data){
            aw_message(GB_get_error());
            return;
        }
    }

    if (strstr(data,"Probe design Parameters:")) { // designliste nach Sonden filtern
        char     *next_line   = 0;
        int       target_seen = 0;
        GB_ERROR  error       = 0;

        for (char *line = data; line; line = next_line) {
            {
                char *nl = strchr(line, '\n');
                if (nl) {
                    nl[0]     = 0;
                    next_line = nl+1;
                }
                else {
                    next_line = 0;
                }
            }

            if (!target_seen) {
                if (strncmp(line, "Target ", 7) == 0) {
                    target_seen = 1;
                }
            }
            else {
                char *probe_string;
                int   ecoli_position;
                error = parse_probe_list_entry(line, probe_string, ecoli_position);

                if (error) {
                    next_line = 0;
                }
                else {
                    const char *real_disp = GBS_global_string("%1d#%1d#%6d#%s", QUALITYDEFAULT, 0, ecoli_position, probe_string);
                    aww->insert_selection(selection_list, real_disp, real_disp);
                }

                free(probe_string);
            }
        }
    }
    else {
        char *next_word = 0;
        for (char *pl = data; pl && pl[0]; pl = next_word)
        {
            char *ko = strchr(pl,'\n');
            if (ko)
            {
                *(ko++) = 0;
                next_word = ko;
            }
            else
                next_word = ko;

            ko = strchr(pl,',');

            char *real_disp = 0;
            if (ko)
            {
                *(ko++) = 0;
                if (selection_list == selected_list || selection_list == probelist) //in ausgewaehltenliste laden
                {
                    real_disp = new char[3+strlen(ko)];
                    sprintf(real_disp,"%1d#%s",atoi(pl),ko);
                }
                else
                {
                    real_disp = new char[21+strlen(SEPARATOR)+strlen(ko)+1];
                    sprintf(real_disp,"%20s%s%s",pl,SEPARATOR,ko);
                }
            }
            else            //kein Komma
            {
                if (selection_list == selected_list || selection_list == probelist)
                {
                    real_disp = new char[5+7+strlen(pl)];
                    sprintf(real_disp,"%1d#%1d#%6d#%s",QUALITYDEFAULT,0,0,pl);
                }
                else
                {
                    real_disp = new char[21+strlen(SEPARATOR)+strlen(pl)+1];
                    sprintf(real_disp,"%20s%s%s"," ",SEPARATOR,pl);
                }
            }

            aww->insert_selection(selection_list,real_disp,real_disp);
            delete real_disp;
        }
    }

    free(data);

    aww->insert_default_selection(selection_list,"","");
    //  aww->sort_selection_list( selection_list );
    aww->update_selection_list(selection_list);
}

AW_window *mp_create_load_box_for_selection_lists(AW_root *aw_root, AW_CL selid)
{
    char *base_name = GBS_global_string_copy("tmp/load_box_sel_%li",(long)selid);

    aw_create_selection_box_awars(aw_root, base_name, ".", ".list", "");

    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "LOAD", "Load");
    aws->load_xfig("sl_l_box.fig");

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("load");
    aws->highlight();
    aws->callback((AW_CB)mp_load_list,(AW_CL)selid,(AW_CL)base_name); // transfers ownership of base_name
    aws->create_button("LOAD","LOAD","L");

    awt_create_selection_box((AW_window *)aws,base_name);
    return (AW_window*) aws;
}

void MP_Window::build_pt_server_list()
{
    int     i;
    char    *choice;

    aws->at("PTServer");
    aws->callback(MP_cache_sonden);
    aws->create_option_menu( MP_AWAR_PTSERVER, NULL, "");

    for (i=0; ; i++) {
        choice = GBS_ptserver_id_to_choice(i, 1);
        if (! choice) break;

        aws->insert_option( choice, "", i);
        delete choice;
    }

    aws->update_option_menu();
}

MP_Window::MP_Window(AW_root *aw_root)
{
    int max_seq_col = 35,
        max_seq_hgt = 15;

    result_window = NULL;

    aws = new AW_window_simple;
    aws->init( aw_root, "MULTI_PROBE", "MULTI_PROBE");
    aws->load_xfig("multiprobe.fig");

    aws->at("close");
    aws->callback(MP_cache_sonden);
    aws->callback(MP_close_main);
    aws->create_button("CLOSE","CLOSE");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"multiprobe.hlp");
    aws->create_button("HELP","HELP");

    //      aws->at("Sequenzeingabe");
    //  aws->callback(MP_take_manual_sequence);
    //  aws->create_input_field(MP_AWAR_SEQUENZEINGABE, max_seq_col);

    aws->at("Border1");
    aws->callback(MP_cache_sonden);
    aws->create_input_field(MP_AWAR_QUALITYBORDER1, 4);

    //     aws->at("EcoliPos");
    //     aws->create_input_field(MP_AWAR_ECOLIPOS, 6);

    aws->button_length(5);
    aws->at("New");
    aws->callback(MP_new_sequence);
    aws->create_button("CREATE_NEW_SEQUENCE", "ADD");

    aws->button_length(10);
    aws->at("Compute");
    aws->callback(MP_compute);
    aws->highlight();
    aws->help_text("Compute possible Solutions");
    aws->create_button("GO","GO");

    aws->button_length(20);
    aws->at("Results");
    aws->callback(MP_result_window);
    aws->create_button("OPEN_RESULT_WIN", "Open result window");

    aws->button_length(5);
    aws->at("RightLeft");
    aws->callback(MP_rightleft);
    aws->create_button("MOVE_RIGHT", "#rightleft.bitmap");      //rightleft.bitmap

    aws->at("LeftRight");
    aws->callback(MP_leftright);
    aws->create_button("MOVE_LEFT", "#leftright.bitmap");       //leftright.bitmap

    aws->at("AllRight");
    aws->callback(MP_all_right);
    aws->create_button("MOVE_ALL_RIGHT", "#allright.bitmap");

    aws->at("Quality");
    aws->callback(MP_cache_sonden);
    aws->create_option_menu( MP_AWAR_QUALITY, NULL, "");
    aws->insert_option( "High Priority", "", 5 );
    aws->insert_option( "       4", "", 4 );
    aws->insert_option( "Normal 3", "", 3 );
    aws->insert_option( "       2", "", 2 );
    aws->insert_option( "Low Prio. 1", "", 1 );
    aws->update_option_menu();

    if (1){
        aws->at("OutsideMismatches");
        aws->callback(MP_cache_sonden);
        aws->create_option_menu( MP_AWAR_OUTSIDEMISMATCHES, NULL, "");
        aws->insert_option( "3.0", "", (float)3.0 );
        aws->insert_option( "2.5", "", (float)2.5 );
        aws->insert_option( "2.0", "", (float)2.0 );
        aws->insert_option( "1.5", "", (float)1.5 );
        aws->insert_option( "1.0", "", (float)1.0 );
#ifdef DEBUG
        aws->insert_option( "0", "", (float)1.0 );
#endif

        aws->update_option_menu();
    }


    aws->button_length(7);
    aws->at("Selectedprobes");
    aws->callback(MP_selected_chosen);
    selected_list = aws->create_selection_list( MP_AWAR_SELECTEDPROBES,
                                                "Selected Probes",
                                                "Selected Probes",
                                                max_seq_col,
                                                max_seq_hgt );
    aws->set_selection_list_suffix(selected_list,"prb");

    aws->insert_default_selection(selected_list, "","");

    aws->at("Probelist");
    probelist = aws->create_selection_list( MP_AWAR_PROBELIST,
                                            "Probelist",
                                            "P" );
    aws->set_selection_list_suffix(probelist,"prb");
    aws->insert_default_selection(probelist, "","");

    aws->at("LoadProbes");
    aws->callback(AW_POPUP, (AW_CL)mp_create_load_box_for_selection_lists, (AW_CL)probelist);
    aws->create_button("LOAD_PROBES","LOAD");

    aws->at("SaveProbes");
    aws->callback( AW_POPUP, (AW_CL)create_save_box_for_selection_lists, (AW_CL)probelist);
    aws->create_button("SAVE_PROBES", "SAVE");

    aws->at("LoadSelProbes");
    aws->callback(AW_POPUP, (AW_CL)mp_create_load_box_for_selection_lists, (AW_CL)selected_list);
    aws->create_button("LOAD_SELECTED_PROBES", "LOAD");

    aws->at("SaveSelProbes");
    aws->callback( AW_POPUP, (AW_CL)create_save_box_for_selection_lists, (AW_CL)selected_list );
    aws->create_button("SAVE_SELECTED_PROBES", "SAVE");

    aws->at("DeleteAllPr");
    aws->callback(MP_del_all_probes);
    aws->create_button("DELETE_ALL_PROBES", "DELETE");

    aws->at("DeleteAllSelPr");
    aws->callback(MP_del_all_sel_probes);
    aws->create_button("DELETE_ALL_SELECTED_PROBES", "DELETE");

    aws->at("DeleteSel");
    aws->callback(MP_del_sel_probes);
    aws->create_button("DELETE_SELECTED_PROBE", "DELETE");

    aws->at("DeletePr");
    aws->callback(MP_del_probes);
    aws->create_button("DELETE_PROBES", "DELETE");

    aws->at("WeightedMismatches");
    aws->callback(MP_cache_sonden);
    aws->create_toggle(MP_AWAR_WEIGHTEDMISMATCHES);

    //  if (0){
    //      aws->create_toggle_field(MP_AWAR_WEIGHTEDMISMATCHES,1);
    //      aws->insert_toggle("#weighted1.bitmap","0",0);
    //      aws->insert_toggle("#weighted3.bitmap","1",1);
    //      aws->insert_toggle("#weighted2.bitmap","2",2);
    //      aws->update_toggle_field();
    //  }

    aws->at("Komplement");
    aws->callback(MP_cache_sonden);
    aws->create_toggle(MP_AWAR_COMPLEMENT);

    //  if (0){
    //      aws->at("Mismatches");
    //      aws->create_option_menu( MP_AWAR_MISMATCHES, NULL, "");
    //      aws->callback(MP_cache_sonden);
    //      aws->insert_option( "0 mismatches", "", 0 );
    //      aws->insert_option( "1 mismatches", "", 1 );
    //      aws->insert_option( "2 mismatches", "", 2 );
    //      aws->insert_option( "3 mismatches", "", 3 );
    //      aws->insert_option( "4 mismatches", "", 4 );
    //      aws->insert_option( "5 mismatches", "", 5 );
    //      aws->update_option_menu();
    //  }
    //  if (0){
    //      aws->at("SingleMismatches");
    //      aws->create_option_menu( MP_AWAR_SINGLEMISMATCHES, NULL, "");
    //      aws->insert_option( "0 mismatches", "", 0 );
    //      aws->insert_option( "1 mismatches", "", 1 );
    //      aws->insert_option( "2 mismatches", "", 2 );
    //      aws->insert_option( "3 mismatches", "", 3 );
    //      aws->insert_option( "4 mismatches", "", 4 );
    //      aws->insert_option( "5 mismatches", "", 5 );
    //      aws->update_option_menu();
    //  }

    aws->at("Greyzone");
    aws->callback(MP_cache_sonden);
    aws->create_option_menu( MP_AWAR_GREYZONE, NULL, "");

    aws->insert_default_option( "0.0", "", (float)0.0 );
    for (float lauf=0.1; lauf<(float)1.0; lauf+=0.1){
        char strs[20];
        sprintf(strs,"%.1f",lauf);
        aws->insert_option( strs, "", lauf );
    }
    aws->update_option_menu();


    aws->at("NoOfProbes");
    aws->create_option_menu( MP_AWAR_NOOFPROBES, NULL, "");
    aws->callback(MP_cache_sonden);

    aws->insert_option( "Compute  1 probe ", "", 1 );
    char str[50];
    for (int i=2; i<=MAXPROBECOMBIS; i++){
        sprintf(str,"%2d-probe-combinations",i);
        aws->insert_option( str, "", i );
    }

    aws->update_option_menu();

    aws->at("PTServer");
    awt_create_selection_list_on_pt_servers(aws, MP_AWAR_PTSERVER, AW_TRUE);

    aw_root->awar(MP_AWAR_PTSERVER)->add_callback(MP_cache_sonden2); // remove cached probes when changing pt-server

    //     build_pt_server_list();
}


MP_Window::~MP_Window()
{
    if (result_window)
        result_window->hide();

    if (aws)    aws->hide();

    delete result_window;
    delete aws;
}
