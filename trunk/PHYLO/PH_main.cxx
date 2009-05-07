#include "phylo.hxx"
#include "phwin.hxx"
#include "PH_display.hxx"

#include <awt_sel_boxes.hxx>
#include <aw_preset.hxx>

#include <awt.hxx>

#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace std;

extern void ph_view_matrix_cb(AW_window *);
extern void ph_view_species_cb(AW_window *,AW_CL,AW_CL);
extern void ph_view_filter_cb(AW_window *,AW_CL,AW_CL);
extern void ph_calculate_matrix_cb(AW_window *,AW_CL,AW_CL);
extern void PH_create_save_matrix_window(AW_root *,char *);
extern void display_status(AW_window *,AW_CL,AW_CL);

AW_HEADER_MAIN
AW_window *create_tree_window(AW_root *aw_root);

GBDATA *GLOBAL_gb_main; // global gb_main for arb_phylo
char **filter_text;

GB_ERROR ph_check_initialized() {
    if (!PHDATA::ROOT) return "Please select alignment and press DONE";
    return 0;
}

void create_filter_text()
{
    filter_text = (char **) calloc(FILTER_MODES,sizeof (char *));
    for(int i=0;i<FILTER_MODES;i++) filter_text[i] = new char[100];
    
    strcpy(filter_text[DONT_COUNT],           "don't count (ignore)                              ");
    strcpy(filter_text[SKIP_COLUMN_IF_MAX],   "if occurs most often => forget whole column       ");
    strcpy(filter_text[SKIP_COLUMN_IF_OCCUR], "if occurs => forget whole column                  ");
    strcpy(filter_text[COUNT_DONT_USE_MAX],   "count, but do NOT use as maximum                  ");
    strcpy(filter_text[TREAT_AS_UPPERCASE],   "treat as uppercase character                      ");
    strcpy(filter_text[TREAT_AS_REGULAR],     "treat as regular character                        ");
}

void startup_sequence_cb(AW_window *aww,AW_CL cd1, AW_CL cl_aww)
{
    PHDATA *phd;
    char *use,*load_what;
    AW_root *aw_root;

    if(aww) aww->hide();
    aw_root=(AW_root *) cd1;
    // loading database
    GB_push_transaction(GLOBAL_gb_main);
    if(GBT_get_alignment_len(GLOBAL_gb_main,aw_root->awar("phyl/alignment")->read_string())<1) {
        aw_root->awar("phyl/alignment")->write_string(GBT_get_default_alignment(GLOBAL_gb_main));
    }
    GB_pop_transaction(GLOBAL_gb_main);

    use = aw_root->awar("phyl/alignment")->read_string();
    load_what = aw_root->awar("phyl/which_species")->read_string();   // all, marked ...
    phd=new PHDATA(aw_root);
    GB_set_cache_size(GLOBAL_gb_main, PH_DB_CACHE_SIZE);
    phd->load(use);
    phd->ROOT = phd;
    long len = PHDATA::ROOT->get_seq_len();
    aw_root->awar("phyl/filter/stopcol")->write_int(len );
    aw_root->awar("phyl/filter/startcol")->set_minmax(0,len);
    aw_root->awar("phyl/filter/stopcol")->set_minmax(0,len);

    ((AW_window*)cl_aww)->show(); // pop up main window
    ph_view_species_cb(0,0,0);
}



ATTRIBUTED(__ATTR__NORETURN, static void ph_exit(AW_window *aw_window, PH_root *ph_root))
{
    AWUSE(aw_window);
    if (ph_root->gb_main) {
        GB_close(ph_root->gb_main);
    }
    exit(0);
}


void expose_callb(AW_window *aw,AW_CL cd1,AW_CL cd2)
{
    AWUSE(aw);AWUSE(cd1);AWUSE(cd2);

    if(PH_display::ph_display->displayed()!=NONE)
    {
        PH_display::ph_display->clear_window();
        PH_display::ph_display->display();
    }
    else    // startup
    {
        //startup_sequence_cb(0,aw->get_root());
    }
} // expose_callb


void resize_callb(AW_window *aw,AW_CL cd1,AW_CL cd2)
{
    AWUSE(aw);AWUSE(cd1);AWUSE(cd2);
    if(PH_display::ph_display)
    {
        PH_display::ph_display->resized();
        PH_display::ph_display->display();
    }
}
//                           total_cells_horiz=PHDATA::ROOT->get_seq_len();
//                            { if(PHDATA::ROOT->markerline[x]>=0.0)

static GB_ERROR PH_create_ml_multiline_SAI(GB_CSTR sai_name, int nr, GBDATA **gb_sai_ptr) {
    GBDATA   *gb_sai = GBT_find_or_create_SAI(GLOBAL_gb_main, sai_name);
    GBDATA   *gbd, *gb2;
    GB_ERROR  error  = ph_check_initialized();

    if (!error) {
        for (gbd = GB_child(gb_sai); gbd; gbd = gb2) {
            gb2 = GB_nextChild(gbd);

            const char *key = GB_read_key_pntr(gbd);
            if (!strcmp(key,"name")) continue;
            if (!strncmp(key,"ali_",4)) continue;
            
            error = GB_delete(gbd);
            if (error) break;
        }
    }

    if (!error) {
        GBDATA *gb_ali = GB_search(gb_sai,PHDATA::ROOT->use,GB_FIND);
        if (gb_ali) {
            for (gbd = GB_child(gb_ali); gbd; gbd = gb2) {
                gb2 = GB_nextChild(gbd);

                const char *key = GB_read_key_pntr(gbd);
                if (!strcmp(key,"data")) continue;
                if (!strcmp(key,"_TYPE")) continue;
                
                error = GB_delete(gbd);
                if (error) break;
            }
        }
    }

    GBDATA *gb_data = 0, *gb_TYPE = 0;

    if (!error) {
        gb_data             = GBT_add_data(gb_sai,PHDATA::ROOT->use,"data",GB_STRING);
        if (!gb_data) error = GB_await_error();
    }

    if (!error) {
        gb_TYPE             = GBT_add_data(gb_sai,PHDATA::ROOT->use,"_TYPE",GB_STRING);
        if (!gb_TYPE) error = GB_await_error();
    }

    if (!error && !PHDATA::ROOT->markerline) {
        error = "Nothing calculated yet";
    }

    if (!error) {
        char *full_save_name = 0;
        FILE *saveResults    = 0;
        if (nr == 2) {
            char *save_name = GB_unique_filename("conservationProfile", "gnu");
            saveResults     = GB_fopen_tempfile(save_name, "w+", &full_save_name);
            GB_remove_on_exit(full_save_name);
            free(save_name);

            if (!saveResults) error = GB_await_error();
        }

        if (!error) {
            AW_window *main_win   = PH_used_windows::windowList->phylo_main_window;
            long       minhom     = main_win->get_root()->awar("phyl/filter/minhom")->read_int();
            long       maxhom     = main_win->get_root()->awar("phyl/filter/maxhom")->read_int();
            long       startcol   = main_win->get_root()->awar("phyl/filter/startcol")->read_int();
            long       stopcol    = main_win->get_root()->awar("phyl/filter/stopcol")->read_int();
            float     *markerline = PHDATA::ROOT->markerline;
            long       len        = PHDATA::ROOT->get_seq_len();

            char *data = (char *)calloc(sizeof(char),(int)len+1);
            int   cnt  = 0;


            for (int x=0;x<len;x++) {
                char c;

                if (x<startcol || x>stopcol) {
                    c = '.';
                }
                else  {
                    float ml = markerline[x];
                    if (nr==2 && ml>0.0) {
                        ph_assert(saveResults);
                        fprintf(saveResults, "%i\t%.2f\n", cnt, ml);
                        cnt++;
                    }

                    if (ml>=0.0 && ml>=minhom && ml<=maxhom) {
                        int digit = -1;
                        switch (nr) {
                            case 0: // hundred
                                if (ml>=100.0) digit = 1;
                                break;
                            case 1: // ten
                                if (ml>=10.0) digit = int(ml/10);
                                break;
                            case 2: // one
                                digit = int(ml);
                                break;
                            default:
                                gb_assert(0);
                                break;
                        }
                        
                        if (digit<0) c = '-';
                        else         c = '0' + digit%10;
                    }
                    else {
                        c = '-';
                    }
                }

                data[x] = c;
            }
            data[len] = 0;

            if (saveResults) {
                fclose(saveResults);
                fprintf(stderr, "Note: Frequencies as well saved to '%s'\n", full_save_name);
            }

            error = GB_write_string(gb_data, data);
            if (!error) {
                const char *buffer = GBS_global_string("FMX: Filter by Maximum Frequency: "
                                                       "Start %li; Stop %li; Minhom %li%%; Maxhom %li%%",
                                                       startcol, stopcol, minhom, maxhom);
                error = GB_write_string(gb_TYPE, buffer);
            }
            free(data);
        }
        free(full_save_name);
    }

    if (!error) *gb_sai_ptr = gb_sai;
    return error;
}

void PH_save_ml_multiline_cb(AW_window *aww) {
    GB_begin_transaction(GLOBAL_gb_main);
    GB_ERROR error = 0;
    char *fname = aww->get_root()->awar("tmp/phylo/markerlinename")->read_string();
    int fname_len = strlen(fname);
    {
        char *digit_appended = (char*)malloc(fname_len+2);
        memcpy(digit_appended, fname, fname_len);
        strcpy(digit_appended+fname_len, "0");

        freeset(fname, digit_appended);
    }
    GBDATA *gb_sai[3];
    int i;
    for (i=0; !error && i<3; i++) {
        fname[fname_len] = '0'+i;
        error = PH_create_ml_multiline_SAI(fname, i, &gb_sai[i]);
    }

    delete fname;

    if (error) {
        GB_abort_transaction(GLOBAL_gb_main);
        aw_message(error);
    }else{
        GB_commit_transaction(GLOBAL_gb_main);
    }
}

void PH_save_ml_cb(AW_window *aww) {
    GB_begin_transaction(GLOBAL_gb_main);
    GB_ERROR error = 0;

    char   *fname  = aww->get_root()->awar("tmp/phylo/markerlinename")->read_string();
    GBDATA *gb_sai = GBT_find_or_create_SAI(GLOBAL_gb_main,fname);
    GBDATA *gbd,*gb2;

    error = ph_check_initialized();

    if (!error) {
        for (gbd = GB_child(gb_sai); gbd; gbd = gb2) {
            gb2 = GB_nextChild(gbd);

            const char *key = GB_read_key_pntr(gbd);
            if (!strcmp(key,"name")) continue;
            if (!strncmp(key,"ali_",4)) continue;
            
            error = GB_delete(gbd);
            if (error) break;
        }
    }

    if (!error) {
        GBDATA *gb_ali = GB_search(gb_sai,PHDATA::ROOT->use,GB_FIND);
        if (gb_ali) {
            for (gbd = GB_child(gb_ali); gbd; gbd = gb2) {
                gb2 = GB_nextChild(gbd);

                const char *key = GB_read_key_pntr(gbd);
                if (!strcmp(key,"bits")) continue;
                if (!strcmp(key,"_TYPE")) continue;
                
                error = GB_delete(gbd);
                if (error) break;
            }
        }
    }

    GBDATA *gb_bits = 0, *gb_TYPE = 0;

    if (!error) {
        gb_bits             = GBT_add_data(gb_sai,PHDATA::ROOT->use,"bits",GB_BITS);
        if (!gb_bits) error = GB_await_error();
    }

    if (!error) {
        gb_TYPE             = GBT_add_data(gb_sai,PHDATA::ROOT->use,"_TYPE",GB_STRING);
        if (!gb_TYPE) error = GB_await_error();
    }

    if (!error && !PHDATA::ROOT->markerline) {
        error = "Nothing calculated yet";
    }

    if (!error) {
        AW_window *main_win   = PH_used_windows::windowList->phylo_main_window;
        long       minhom     = main_win->get_root()->awar("phyl/filter/minhom")->read_int();
        long       maxhom     = main_win->get_root()->awar("phyl/filter/maxhom")->read_int();
        long       startcol   = main_win->get_root()->awar("phyl/filter/startcol")->read_int();
        long       stopcol    = main_win->get_root()->awar("phyl/filter/stopcol")->read_int();
        long       len        = PHDATA::ROOT->get_seq_len();
        char      *bits       = (char *)calloc(sizeof(char),(int)len+1);
        int        x;
        float     *markerline = PHDATA::ROOT->markerline;

        for (x=0;x<len;x++) {
            int bit;

            if (x < startcol || x>stopcol) {
                bit = 0;
            }
            else {
                float ml = markerline[x];

                if (ml>=0.0 && ml>=minhom && ml<=maxhom) bit = 1;
                else                     bit = 0;

            }
            bits[x] = '0'+bit;
        }

        GB_write_bits(gb_bits,bits,len, "0");
        char buffer[1024];
        sprintf(buffer,"FMX: Filter by Maximum Frequency: "
                "Start %li; Stop %li; Minhom %li%%; Maxhom %li%%",
                startcol, stopcol, minhom, maxhom);

        GB_write_string(gb_TYPE,buffer);

        free(bits);
    }
    free(fname);

    if (error) {
        GB_abort_transaction(GLOBAL_gb_main);
        aw_message(error);
    }else{
        GB_commit_transaction(GLOBAL_gb_main);
    }
}


AW_window *PH_save_markerline(AW_root *root, AW_CL cl_multi_line)
{
    int multi_line = int(cl_multi_line); // ==1 -> save three SAI's usable as column percentage

    root->awar_string("tmp/phylo/markerlinename","markerline",AW_ROOT_DEFAULT);

    AW_window_simple *aws = new AW_window_simple;

    if (multi_line) {
        aws->init( root, "EXPORT_FREQUENCY_LINES", "Export Frequency Lines");
    }
    else {
        aws->init( root, "EXPORT_MARKER_LINE", "Export Marker Line");
    }

    aws->load_xfig("phylo/save_markerline.fig");

    aws->callback( AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"ph_export_markerline.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("name");
    aws->create_input_field("tmp/phylo/markerlinename");

    aws->at("box");
    awt_create_selection_list_on_extendeds(GLOBAL_gb_main,aws,"tmp/phylo/markerlinename");

    aws->at("save");
    if (multi_line)     aws->callback(PH_save_ml_multiline_cb);
    else        aws->callback(PH_save_ml_cb);
    aws->create_button("EXPORT","EXPORT","E");

    return aws;
}

AW_window *create_phyl_main_window(AW_root *aw_root,PH_root *ph_root,AWT_graphic * awd)
{
    AWUSE(awd);
    AW_window_menu_modes *awm = new AW_window_menu_modes();
    awm->init(aw_root,"ARB_PHYLO", "ARB_PHYLO", 830,630);

    // create menues and menu inserts with callbacks
    //

    AW_gc_manager gcmiddle = AW_manage_GC( awm,
                                           awm->get_device (AW_MIDDLE_AREA),
                                           PH_GC_0, PH_GC_0_DRAG, AW_GCM_DATA_AREA,
                                           resize_callb, 0,0,
                                           false, // no color groups
                                           "#CC9AF8",
                                           "#SEQUENCE$#000000",
                                           "#MARKER$#FF0000",
                                           "NOT_MARKER$#A270C0",
                                           NULL);
    AW_gc_manager gcbottom = AW_manage_GC( awm,
                                           awm->get_device (AW_BOTTOM_AREA),
                                           PH_GC_0, PH_GC_0_DRAG, AW_GCM_WINDOW_AREA,
                                           resize_callb, 0,0,
                                           false, // no color groups
                                           "pink",
                                           "#FOOTER",NULL);

    GBUSE(gcbottom);

    // File menu
    awm->create_menu(0,"File","F");
#if defined(DEBUG)
    awm->insert_menu_topic("db_browser", "Browse loaded database(s)", "", "db_browser.hlp", AWM_ALL, AW_POPUP, (AW_CL)AWT_create_db_browser, 0);
    awm->insert_separator();
#endif // DEBUG
    awm->insert_menu_topic("export_filter","Export Filter", "E",    "ph_export_markerline.hlp",AWM_ALL, (AW_CB)AW_POPUP,(AW_CL) PH_save_markerline, 0 );
    awm->insert_menu_topic("export_freq","Export Frequencies",  "F",    "ph_export_markerline.hlp",AWM_ALL, (AW_CB)AW_POPUP,(AW_CL) PH_save_markerline, 1 );
    awm->insert_menu_topic("quit","QUIT",       "q",    "quit.hlp", AWM_ALL,        (AW_CB)ph_exit ,(AW_CL)ph_root,0);

    // Calculate menu
    awm->create_menu(0,"Calculate","C" );
    awm->insert_menu_topic("calc_column_filter",    "Column Filter",    "F","no help",AWM_ALL,(AW_CB2)ph_view_filter_cb,(AW_CL) 0,(AW_CL) 0);

    // Config menu
    awm->create_menu(0,"Config","o");
    awm->insert_menu_topic("config_column_filter","Column Filter",  "F","no help",  AWM_ALL,    AW_POPUP,(AW_CL)PH_create_filter_window,0);

    // Properties menu
    awm->create_menu("","Properties","P");
    awm->insert_menu_topic("props_menu",    "Menu: Colors and Fonts ...",   "M","props_frame.hlp",  AWM_ALL, AW_POPUP, (AW_CL)AW_preset_window, 0 );
    awm->insert_menu_topic("props_data",    "Data: Colors and Fonts ...",   "D","ph_props_data.hlp",AWM_ALL, AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)gcmiddle );
    awm->insert_menu_topic("save_props",    "Save Properties (in ~/.arb_prop/phylo.arb)","S","savedef.hlp",AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );


    // set window areas
    awm->set_info_area_height( 30 );
    awm->at(5,2);
    awm->auto_space(5,-2);

    awm->callback( (AW_CB1)ph_exit ,(AW_CL)ph_root);
    awm->button_length(0);
    awm->help_text("quit.hlp");
    awm->create_button("QUIT","QUIT");

    awm->callback(AW_POPUP_HELP,(AW_CL)"phylo.hlp");
    awm->button_length(0);
    awm->create_button("HELP","HELP","H");


    awm->set_bottom_area_height( 120 );

    awm->set_expose_callback (AW_MIDDLE_AREA,   expose_callb,   (AW_CL)awm,0);
    awm->set_resize_callback (AW_MIDDLE_AREA,   resize_callb,   (AW_CL)awm,0);
    awm->set_expose_callback (AW_BOTTOM_AREA,   display_status, (AW_CL)aw_root,0);
    awm->set_resize_callback (AW_BOTTOM_AREA,   display_status, (AW_CL)aw_root,0);

    return (AW_window *)awm;
}


AW_window *create_select_alignment_window(AW_root *aw_root, AW_CL cl_aww)
{
    AW_window_simple *aws = new AW_window_simple;

    aws->init( aw_root,"SELECT_ALIGNMENT", "SELECT ALIGNMENT");
    aws->load_xfig("phylo/select_ali.fig");
    aws->button_length(10);

    aws->at("done");
    aws->callback(startup_sequence_cb,(AW_CL) aw_root, cl_aww);
    aws->create_button("DONE","DONE","D");

    aws->at("which_alignment");
    awt_create_selection_list_on_ad(GLOBAL_gb_main,(AW_window *)aws,"phyl/alignment","*=");
    return (AW_window *) aws;
}



PH_used_windows::PH_used_windows(void)
{
    memset((char *) this,0,sizeof(PH_used_windows));
}

// initialize 'globals'
PH_used_windows *PH_used_windows::windowList = 0;
PH_display *PH_display::ph_display=0;
PHDATA *PHDATA::ROOT = 0;


int
main(int argc, char **argv)
{
    if (argc >= 2 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
        fprintf(stderr, "Usage: arb_phylo [database]\n");
        return EXIT_FAILURE;
    }

    PH_root          *ph_root;
    AW_root          *aw_root;
    char            **alignment_names;
    GB_ERROR          error     = 0;
    AW_default        aw_default;
    PH_used_windows  *puw       = new PH_used_windows;
    PH_display       *phd       = new PH_display;
    int               num_alignments;
    const char       *db_server = ":";

    aw_initstatus();
    aw_root = new AW_root;
    aw_default = aw_root->open_default(".arb_prop/phylo.arb");
    aw_root->init_variables(aw_default);
    aw_root->init_root("ARB_PHYLO", AW_FALSE);

    if (argc == 2) {
        db_server = argv[1];
    }
    ph_root = new PH_root;
    if ((error = ph_root->open(db_server))) { // initializes global 'GLOBAL_gb_main'
        aw_message(error);
        exit(-1);
    }

    // create arb_phylo awars :
    PH_create_filter_variables(aw_root, aw_default);
    PH_create_matrix_variables(aw_root, aw_default);
    ARB_init_global_awars(aw_root, aw_default, GLOBAL_gb_main);
    AWT_create_db_browser_awars(aw_root, aw_default);

    AWT_announce_db_to_browser(GLOBAL_gb_main, GBS_global_string("ARB-database (%s)", db_server));

    create_filter_text();

    // create main window :

    puw->phylo_main_window = create_phyl_main_window(aw_root, ph_root, 0);
    puw->windowList        = puw;

    // exposing the main window is now done from inside startup_sequence_cb()
    // (otherwise it crashes during resize because the resize callback gets
    // called before startup_sequence_cb initialized the necessary data)
    //
    // puw->phylo_main_window->show();

    phd->ph_display = phd;

    //loading database
    GB_push_transaction(GLOBAL_gb_main);
    alignment_names = GBT_get_alignment_names(GLOBAL_gb_main);

    for (num_alignments = 0; alignment_names[num_alignments] != 0; num_alignments++) ;
    if (num_alignments > 1) {
        AW_window *sel_ali_aww = create_select_alignment_window(aw_root, (AW_CL)puw->phylo_main_window);
        sel_ali_aww->show();
    }
    else {
        aw_root->awar("phyl/alignment")->write_string( alignment_names[0]);
        startup_sequence_cb(0, (AW_CL)aw_root, (AW_CL)puw->phylo_main_window);
    }
    GBT_free_names(alignment_names);
    GB_pop_transaction(GLOBAL_gb_main);

    aw_root->main_loop();
    return EXIT_SUCCESS;
}



void AD_map_viewer(GBDATA *dummy,AD_MAP_VIEWER_TYPE)
    {
    AWUSE(dummy);
}
