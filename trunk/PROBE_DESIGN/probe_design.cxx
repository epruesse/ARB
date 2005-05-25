#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>
#include <ctype.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <awt.hxx>
#include <awt_iupac.hxx>

#include <awt_canvas.hxx>
#include <awt_tree.hxx>
#include <awt_dtree.hxx>
#include <awt_tree_cb.hxx>
#include <awt_config_manager.hxx>
#include <awt_canvas.hxx>
#include <awt_sel_boxes.hxx>

#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>
#include <probe_design.hxx>

#include <GEN.hxx>
#include "SaiProbeVisualization.hxx"
#include "probe_match_parser.hxx"

void NT_group_not_marked_cb(void *dummy, AWT_canvas *ntw); // real prototype is in awt_tree_cb.hxx

// general awars

#define AWAR_PROBE_CREATE_GENE_SERVER "tmp/probe_admin/gene_server" // whether to create a gene pt server

// probe match awars

// #define AWAR_PD_MATCH_ITEM     AWAR_SPECIES_NAME
#define AWAR_PD_SELECTED_MATCH "tmp/probe_design/match"
#define AWAR_PD_MATCH_RESOLVE  "tmp/probe_design/match_resolve" // for IUPAC resolution

#define AWAR_PD_MATCH_SORTBY     "probe_match/sort_by" // weighted mismatches
#define AWAR_PD_MATCH_COMPLEMENT "probe_match/complement" // check reverse complement too
#define AWAR_PD_MATCH_MARKHITS   "probe_match/mark_hits" // mark hitten species in database
#define AWAR_PD_MATCH_WRITE2TMP  "probe_match/write_2_tmp" // write result to field tmp
#define AWAR_PD_MATCH_AUTOMATCH  "probe_match/auto_match" // auto match probes when target string changes

#define AWAR_PD_MATCH_CLIPHITS "probe_match/clip_hits"
#define AWAR_PD_MATCH_NHITS    "tmp/probe_match/nhits" // display the 'number of hits'

// probe design awars

#define AWAR_PD_DESIGN_CLIPRESULT "probe_design/CLIPRESULT" // 'length of output' (how many probes will get designed)
#define AWAR_PD_DESIGN_MISHIT     "probe_design/MISHIT" // 'non group hits'
#define AWAR_PD_DESIGN_MAXBOND    "probe_design/MAXBOND" // max hairpinbonds ?
#define AWAR_PD_DESIGN_MINTARGETS "probe_design/MINTARGETS" // 'min. group hits (%)'

#define AWAR_PD_DESIGN_PROBELENGTH  "probe_design/PROBELENGTH" // length of probe
#define AWAR_PD_DESIGN_MIN_TEMP     "probe_design/MINTEMP" // temperature (min)
#define AWAR_PD_DESIGN_MAX_TEMP     "probe_design/MAXTEMP" // temperature (max)
#define AWAR_PD_DESIGN_MIN_GC       "probe_design/MINGC" // GC content (min)
#define AWAR_PD_DESIGN_MAX_GC       "probe_design/MAXGC" // GC content (max)
#define AWAR_PD_DESIGN_MIN_ECOLIPOS "probe_design/MINPOS" // ecolipos (min)
#define AWAR_PD_DESIGN_MAX_ECOLIPOS "probe_design/MAXPOS" // ecolipos (max)

#define AWAR_PD_DESIGN_GENE "probe_design/gene" // generate probes for genes ?

// probe design/match (expert window)
#define AWAR_PD_DESIGN_EXP_BONDS  "probe_design/bonds/pos" // prefix for bonds
#define AWAR_PD_DESIGN_EXP_SPLIT  "probe_design/SPLIT"
#define AWAR_PD_DESIGN_EXP_DTEDGE "probe_design/DTEDGE"
#define AWAR_PD_DESIGN_EXP_DT     "probe_design/DT"

// ----------------------------------------

saiProbeData *g_spd = 0;

extern GBDATA *gb_main;     /* must exist */

struct gl_struct {
    aisc_com *link;
    T_PT_LOCS locs;
    T_PT_MAIN com;
    AW_window_simple *pd_design;
    AW_selection_list *pd_design_id;
} pd_gl;

struct probe_match_event_para {
    AW_window *aww;
    AW_CL      sel_id;
    bool       disable;
};
static probe_match_event_para auto_match_cb_settings = { 0, 0, true };
void probe_match_event(AW_window *aww, AW_CL cl_selection_id, AW_CL cl_count_ptr); // prototype

static void auto_match_cb(AW_root *root) {
    if (!auto_match_cb_settings.disable) {
        char *ts = root->awar(AWAR_TARGET_STRING)->read_string();
        if (strlen(ts) > 0) {
            probe_match_event(auto_match_cb_settings.aww, auto_match_cb_settings.sel_id, 0);
        }
        free(ts);
    }
}

static const char *auto_match_sensitive_awars[] = {
    AWAR_TARGET_STRING,
    AWAR_PT_SERVER,
    AWAR_PD_MATCH_COMPLEMENT,
    AWAR_MAX_MISMATCHES,
    AWAR_PD_MATCH_SORTBY,
    0
};

static void auto_match_changed(AW_root *root) {
    static bool callback_active = false;
    int         autoMatch       = root->awar(AWAR_PD_MATCH_AUTOMATCH)->read_int();

    if (autoMatch) {
        if (!callback_active) {
            for (int i = 0; auto_match_sensitive_awars[i]; ++i) {
                root->awar(auto_match_sensitive_awars[i])->add_callback(auto_match_cb);
            }
        }
    }
    else {
        if (callback_active) {
            for (int i = 0; auto_match_sensitive_awars[i]; ++i) {
                root->awar(auto_match_sensitive_awars[i])->remove_callback(auto_match_cb);
            }
        }
    }
    callback_active = bool(autoMatch);
}

static void enable_auto_match_cb(AW_root *root, AW_window *aww, AW_CL cl_sel_id) {
    if (cl_sel_id == 0 && // small enable (w/o selection list)
        auto_match_cb_settings.aww != 0) return; // is only done when not enabled yet

    auto_match_cb_settings.aww     = aww;
    auto_match_cb_settings.sel_id  = cl_sel_id;
    auto_match_cb_settings.disable = false;
    auto_match_changed(root);
}

static void popup_match_window_cb(AW_window *aww) {
    AW_root   *root         = aww->get_root();
    AW_window *match_window = create_probe_match_window(root, 0);
    match_window->show();
    root->awar(AWAR_TARGET_STRING)->touch(); // force re-match
}

// --------------------------------------------------------------------------------

void probe_design_create_result_window(AW_window *aww)
{
    if (pd_gl.pd_design){
        pd_gl.pd_design->show();
        return;
    }
    AW_root *root = aww->get_root();

    pd_gl.pd_design = new AW_window_simple;
    pd_gl.pd_design->init(root, "PD_RESULT", "PD RESULT");
    pd_gl.pd_design->load_xfig("pd_reslt.fig");

    pd_gl.pd_design->at("close");
    pd_gl.pd_design->callback((AW_CB0)AW_POPDOWN);
    pd_gl.pd_design->create_button("CLOSE", "CLOSE", "C");

    pd_gl.pd_design->at("help");
    pd_gl.pd_design->callback(AW_POPUP_HELP, (AW_CL)"probedesignresult.hlp");
    pd_gl.pd_design->create_button("HELP", "HELP", "H");

    pd_gl.pd_design->at("auto");
    pd_gl.pd_design->label("Auto match");
    pd_gl.pd_design->create_toggle(AWAR_PD_MATCH_AUTOMATCH );

    enable_auto_match_cb(root, pd_gl.pd_design, 0);

    pd_gl.pd_design->at( "result" );
    pd_gl.pd_design_id = pd_gl.pd_design->create_selection_list( AWAR_TARGET_STRING, NULL, "", 40, 5 );
    pd_gl.pd_design->set_selection_list_suffix(pd_gl.pd_design_id, "prb");

    pd_gl.pd_design->at("save");
    pd_gl.pd_design->callback( AW_POPUP, (AW_CL)create_save_box_for_selection_lists, (AW_CL)pd_gl.pd_design_id );
    pd_gl.pd_design->create_button("SAVE", "SAVE", "S");

    pd_gl.pd_design->at("print");
    pd_gl.pd_design->callback( create_print_box_for_selection_lists, (AW_CL)pd_gl.pd_design_id );
    pd_gl.pd_design->create_button("PRINT", "PRINT", "P");

    pd_gl.pd_design->at("match");
    pd_gl.pd_design->callback( popup_match_window_cb);
    pd_gl.pd_design->create_button("MATCH", "MATCH", "M");

    pd_gl.pd_design->show();
}

int init_local_com_struct()
{
    const char *user = GB_getenvUSER();

    if( aisc_create(pd_gl.link, PT_MAIN, pd_gl.com,
                    MAIN_LOCS, PT_LOCS, &pd_gl.locs,
                    LOCS_USER, user,
                    NULL)){
        return 1;
    }
    return 0;
}

char *probe_pt_look_for_server(AW_root *root)
{
    char choice[256];
    sprintf(choice, "ARB_PT_SERVER%li", root->awar(AWAR_PT_SERVER)->read_int());
    GB_ERROR error;
    error = arb_look_and_start_server(AISC_MAGIC_NUMBER, choice, gb_main);
    if (error) {
        aw_message(error);
        return 0;
    }
    return GBS_read_arb_tcp(choice);
}

static GB_ERROR species_requires(GBDATA *gb_species, const char *whats_required) {
    GBDATA     *gb_species_name = GB_find(gb_species, "name", 0, down_level);
    const char *name            = "unnamed (which is a dangerous error)";
    if (gb_species_name) name   = GB_read_char_pntr(gb_species_name);

    return GBS_global_string("Species '%s' needs %s", name, whats_required);
}

static GB_ERROR gene_requires(GBDATA *gb_gene, const char *whats_required) {
    GBDATA     *gb_gene_name    = GB_find(gb_gene, "name", 0, down_level);
    const char *gene_name       = "unnamed (which is a dangerous error)";
    if (gb_gene_name) gene_name = GB_read_char_pntr(gb_gene_name);

    GBDATA *gb_species = GB_get_father(GB_get_father(gb_gene));
    pd_assert(gb_species);

    GBDATA     *gb_species_name       = GB_find(gb_species, "name", 0, down_level);
    const char *species_name          = "unnamed (which is a dangerous error)";
    if (gb_species_name) species_name = GB_read_char_pntr(gb_species_name);

    return GBS_global_string("Gene '%s' of organism '%s' needs %s", gene_name, species_name, whats_required);
}

GB_ERROR pd_get_the_names(bytestring &bs, bytestring &checksum) {
    void     *names     = GBS_stropen(1024);
    void     *checksums = GBS_stropen(1024);
    GB_ERROR  error     = 0;

    GB_begin_transaction(gb_main);

    char *use = GBT_get_default_alignment(gb_main);

    for (GBDATA *gb_species = GBT_first_marked_species(gb_main); gb_species; gb_species = GBT_next_marked_species(gb_species)) {
        GBDATA *gb_name = GB_find(gb_species, "name", 0, down_level);
        if (!gb_name) { error = species_requires(gb_species, "name"); break; }

        GBDATA *gb_data = GBT_read_sequence(gb_species, use);
        if (!gb_data) { error = species_requires(gb_species, GBS_global_string("data in '%s'", use)); break; }

        GBS_intcat(checksums, GBS_checksum(GB_read_char_pntr(gb_data), 1, ".-"));
        GBS_strcat(names, GB_read_char_pntr(gb_name));
        GBS_chrcat(checksums, '#');
        GBS_chrcat(names, '#');
    }

    GBS_str_cut_tail(names, 1); // remove trailing '#'
    GBS_str_cut_tail(checksums, 1); // remove trailing '#'

    free(use);

    bs.data = GBS_strclose(names);
    bs.size = strlen(bs.data)+1;

    checksum.data = GBS_strclose(checksums);
    checksum.size = strlen(checksum.data)+1;

    GB_commit_transaction(gb_main);

    return error;
}

GB_ERROR pd_get_the_gene_names(bytestring &bs, bytestring &checksum){
    void     *names     = GBS_stropen(1024);
    void     *checksums = GBS_stropen(1024);
    GB_ERROR  error     = 0;

    GB_begin_transaction(gb_main);
    const char *use = GENOM_ALIGNMENT; // gene pt server is always build on 'ali_genom'

    for (GBDATA *gb_species = GEN_first_organism(gb_main); gb_species && !error; gb_species = GEN_next_organism(gb_species)) {
        const char *sequence     = 0;
        const char *species_name = 0;
        {
            GBDATA *gb_data = GBT_read_sequence(gb_species, use);
            if (!gb_data) { error = species_requires(gb_species, GBS_global_string("data in '%s'", use)); break; }
            sequence = GB_read_char_pntr(gb_data);

            GBDATA *gb_name = GB_search(gb_species, "name", GB_FIND);
            if (!gb_name) { error = species_requires(gb_species, "name"); break; }
            species_name = GB_read_char_pntr(gb_name);
        }

        for (GBDATA *gb_gene = GEN_first_marked_gene(gb_species); gb_gene; gb_gene = GEN_next_marked_gene(gb_gene)) {
            const char *gene_name = 0;
            {
                GBDATA *gb_gene_name = GB_find(gb_gene, "name", 0, down_level);
                if (!gb_gene_name) { error = gene_requires(gb_gene, "name"); break; }
                gene_name = GB_read_char_pntr(gb_gene_name);
            }

            long checksum;
            {
                GBDATA *gb_gene_pos_begin = GB_find(gb_gene, "pos_begin", 0, down_level);
                if (!gb_gene_pos_begin) { error = gene_requires(gb_gene, "pos_begin"); break; }

                GBDATA *gb_gene_pos_end = GB_find(gb_gene,"pos_end",0,down_level);
                if (!gb_gene_pos_end) { error = gene_requires(gb_gene, "pos_end"); break; }

                int pos_begin = GB_read_int(gb_gene_pos_begin)-1;
                int pos_end   = GB_read_int(gb_gene_pos_end)-1;

                int   len      = pos_end-pos_begin+1;
                char *gene_seq = new char[len+1];
                strncpy(gene_seq, sequence+pos_begin, len);
                gene_seq[len]  = 0;

                checksum = GBS_checksum(gene_seq, 1, ".-");

                // @@@ FIXME: what to do with splitted genes ?
            }

            const char *id = GBS_global_string("%s/%s", species_name, gene_name);

            GBS_intcat(checksums, checksum);
            GBS_strcat(names, id);
            GBS_chrcat(checksums, '#');
            GBS_chrcat(names, '#');
        }
    }

    GBS_str_cut_tail(names, 1); // remove trailing '#'
    GBS_str_cut_tail(checksums, 1); // remove trailing '#'

    bs.data = GBS_strclose(names);
    bs.size = strlen(bs.data)+1;

    checksum.data = GBS_strclose(checksums);
    checksum.size = strlen(checksum.data)+1;

    GB_commit_transaction(gb_main);
    return error;
}

int probe_design_send_data(AW_root *root, T_PT_PDC  pdc)
{
    int i;
    char buffer[256];
    if (aisc_put(pd_gl.link, PT_PDC, pdc,
                 PDC_DTEDGE,    (double)root->awar(AWAR_PD_DESIGN_EXP_DTEDGE)->read_float()*100.0,
                 PDC_DT,        (double)root->awar(AWAR_PD_DESIGN_EXP_DT)->read_float()*100.0,
                 PDC_SPLIT, (double)root->awar(AWAR_PD_DESIGN_EXP_SPLIT)->read_float(),
                 PDC_CLIPRESULT,    root->awar(AWAR_PD_DESIGN_CLIPRESULT)->read_int(),
                 0)) return 1;
    for (i=0;i<16;i++) {
        sprintf(buffer,AWAR_PD_DESIGN_EXP_BONDS "%i",i);
        if (aisc_put(pd_gl.link,PT_PDC, pdc,
                     PT_INDEX,  i,
                     PDC_BONDVAL,   (double)root->awar(buffer)->read_float(),
                     0) ) return 1;
    }
#if 0
    T_PT_SPECIALS specials;
    for (i=0;i<PROBE_DESIGN_EXCEPTION_MAX;i++){
        char pdcmode[256],pdcll[256],pdcl[256],pdcc[256],pdcr[256],pdcrr[256];

        sprintf(pdcmode,"probe_design/exceptions/nr%i/MODE",i);
        if (root->awar(pdcmode)->read_int()<0) break;
        sprintf(pdcll,"probe_design/exceptions/nr%i/LLEFT",i);
        sprintf(pdcl,"probe_design/exceptions/nr%i/LEFT",i);
        sprintf(pdcc,"probe_design/exceptions/nr%i/CENTER",i);
        sprintf(pdcr,"probe_design/exceptions/nr%i/RIGHT",i);
        sprintf(pdcrr,"probe_design/exceptions/nr%i/RRIGHT",i);
        if (aisc_create(pd_gl.link,PT_PDC, pdc,
                        PDC_SPECIALS,   PT_SPECIALS, &specials,
                        SPECIALS_MODE, root->awar(pdcmode)->read_int(),
                        SPECIALS_LLEFT, (double)root->awar(pdcll)->read_float(),
                        SPECIALS_LEFT, (double)root->awar(pdcl)->read_float(),
                        SPECIALS_CENTER, (double)root->awar(pdcc)->read_float(),
                        SPECIALS_RIGHT, (double)root->awar(pdcr)->read_float(),
                        SPECIALS_RRIGHT, (double)root->awar(pdcrr)->read_float(),
                        0)) return 1;
    }
#endif
    return 0;
}

void probe_design_event(AW_window *aww)
{
    AW_root     *root  = aww->get_root();
    char        *servername;
    T_PT_PDC     pdc;
    T_PT_TPROBE  tprobe;
    bytestring   bs;
    bytestring   check;
    char        *match_info;
    GB_ERROR     error = 0;

    aw_openstatus("Probe Design");
    aw_status("Search a free running server");

    if( !(servername=(char *)probe_pt_look_for_server(root)) ){
        aw_closestatus();
        return;
    }

    pd_gl.link = (aisc_com *)aisc_open(servername, &pd_gl.com,AISC_MAGIC_NUMBER);
    free (servername); servername = 0;

    if (!pd_gl.link) {
        aw_message ("Cannot contact Probe bank server ");
        aw_closestatus();
        return;
    }
    if (init_local_com_struct() ) {
        error = "Cannot contact to probe server: Connection Refused";
    }

    bool design_gene_probes = root->awar(AWAR_PD_DESIGN_GENE)->read_int();
    if (design_gene_probes) {
        GB_transaction ta(gb_main);
        if (!GEN_is_genome_db(gb_main, -1)) design_gene_probes = false;
    }

    if (!error){
        if (design_gene_probes) { // design probes for genes
            error = pd_get_the_gene_names(bs,check);
        }
        else {
            error = pd_get_the_names(bs,check);
        }
    }

    if (error){
        aw_message (error);
        aw_closestatus();
        return;
    }

    aw_status("Start probe design (Cannot be stopped)");

    aisc_create(pd_gl.link,PT_LOCS, pd_gl.locs,
                LOCS_PROBE_DESIGN_CONFIG, PT_PDC,   &pdc,
                PDC_PROBELENGTH,    root->awar(AWAR_PD_DESIGN_PROBELENGTH)->read_int(),
                PDC_MINTEMP,    (double)root->awar(AWAR_PD_DESIGN_MIN_TEMP)->read_float(),
                PDC_MAXTEMP,    (double)root->awar(AWAR_PD_DESIGN_MAX_TEMP)->read_float(),
                PDC_MINGC,          (double)root->awar(AWAR_PD_DESIGN_MIN_GC)->read_float()/100.0,
                PDC_MAXGC,          (double)root->awar(AWAR_PD_DESIGN_MAX_GC)->read_float()/100.0,
                PDC_MAXBOND,    (double)root->awar(AWAR_PD_DESIGN_MAXBOND)->read_int(),
                0);
    aisc_put(pd_gl.link,PT_PDC, pdc,
             PDC_MINPOS,    root->awar(AWAR_PD_DESIGN_MIN_ECOLIPOS)->read_int(),
             PDC_MAXPOS,    root->awar(AWAR_PD_DESIGN_MAX_ECOLIPOS)->read_int(),
             PDC_MISHIT,    root->awar(AWAR_PD_DESIGN_MISHIT)->read_int(),
             PDC_MINTARGETS,    (double)root->awar(AWAR_PD_DESIGN_MINTARGETS)->read_float()/100.0,
             0);

    if (probe_design_send_data(root,pdc)) {
        aw_message ("Connection to PT_SERVER lost (1)");
        aw_closestatus();
        return;
    }

    aisc_put(pd_gl.link,PT_PDC, pdc,
             PDC_NAMES, &bs,
             PDC_CHECKSUMS, &check,
             0);


    /* Get the unknown names */
    bytestring unknown_names;
    if (aisc_get(pd_gl.link,PT_PDC, pdc,
                 PDC_UNKNOWN_NAMES, &unknown_names,
                 0)){
        aw_message ("Connection to PT_SERVER lost (1)");
        aw_closestatus();
        return;
    }

    char *unames = unknown_names.data;
    bool  abort  = false;

    if (unknown_names.size>1) {
        if (design_gene_probes) { // updating sequences of missing genes is not possible with gene PT server
            aw_message(GBS_global_string("Your PT server is not up to date or wrongly chosen.\n"
                                         "The following genes are new to it: %s\n"
                                         "You'll have to update the PT server.", unames));
            abort = true;
        }
        else if (aw_message(GBS_global_string(
                                              "Your PT server is not up to date or wrongly chosen\n"
                                              "  The following names are new to it:\n"
                                              "  %s\n"
                                              "  This version allows you to quickly add the unknown sequences\n"
                                              "  to the pt_server\n"
                                              ,unames),"Add and Continue,Abort"))
        {
            abort = true;
        }
        else {
            GB_transaction dummy(gb_main);

            char *h;
            char *ali_name = GBT_get_default_alignment(gb_main);

            for (; unames && !abort; unames = h) {
                h = strchr(unames,'#');
                if (h) *(h++) = 0;
                GBDATA *gb_species = GBT_find_species(gb_main,unames);
                if (!gb_species) {
                    aw_message(GBS_global_string("Species '%s' not found", unames));
                    abort = true;
                }
                else {
                    GBDATA *data = GBT_read_sequence(gb_species,ali_name);
                    if (!data) {
                        aw_message( GB_export_error("Species '%s' has no sequence belonging to alignment '%s'", unames, ali_name));
                        abort = true;
                    }
                    else {
                        T_PT_SEQUENCE pts;
                        bytestring    bs_seq;
                        bs_seq.data = GB_read_char_pntr(data);
                        bs_seq.size = GB_read_string_count(data)+1;
                        aisc_create(pd_gl.link, PT_PDC, pdc, PDC_SEQUENCE,
                                    PT_SEQUENCE, &pts,
                                    SEQUENCE_SEQUENCE, &bs_seq,
                                    0);
                    }
                }
            }
        }
        free(unknown_names.data);
    }

    if (!abort) {
        aisc_put(pd_gl.link,PT_PDC, pdc,
                 PDC_GO,0,
                 0);

        aw_status("Read the results from the server");
        {
            char *locs_error = 0;
            if (aisc_get( pd_gl.link, PT_LOCS, pd_gl.locs, LOCS_ERROR, &locs_error, 0)) {
                aw_message ("Connection to PT_SERVER lost (1)");
                abort = true;
            }
            else if (*locs_error) {
                aw_message(locs_error);
                abort = true;
            }
            else {
                free(locs_error);
            }
        }
    }

    if (!abort) {
        aisc_get( pd_gl.link, PT_PDC, pdc,
                  PDC_TPROBE, &tprobe,
                  0);

        probe_design_create_result_window(aww);
        pd_gl.pd_design->clear_selection_list(pd_gl.pd_design_id);

        if (tprobe) {
            aisc_get( pd_gl.link, PT_TPROBE, tprobe,
                      TPROBE_INFO_HEADER,   &match_info,
                      0);
            char *s = strtok(match_info,"\n");
            while (s) {
                pd_gl.pd_design->insert_selection( pd_gl.pd_design_id, s, "" );
                s = strtok(0,"\n");
            }
            free(match_info);
        }else{
            pd_gl.pd_design->insert_selection( pd_gl.pd_design_id, "There are no results", "" );
        }

        //#define TEST_PD
#if defined(TEST_PD)
        int my_TPROBE_KEY;
        char *my_TPROBE_KEYSTRING;
        int my_TPROBE_CNT;
        int my_TPROBE_PARENT;
        int my_TPROBE_LAST;
        char *my_TPROBE_IDENT;
        char *my_TPROBE_SEQUENCE;
        double my_TPROBE_QUALITY;
        int my_TPROBE_GROUPSIZE;
        int my_TPROBE_HAIRPIN;
        int my_TPROBE_WHAIRPIN;
        //    int my_TPROBE_PERC;
        double my_TPROBE_TEMPERATURE;
        int my_TPROBE_MISHIT;
        int my_TPROBE_APOS;
        int my_TPROBE_ECOLI_POS;

#endif // TEST_PD

        while ( tprobe ){
            long tprobe_next;
            if (aisc_get( pd_gl.link, PT_TPROBE, tprobe,
                          TPROBE_NEXT,      &tprobe_next,
                          TPROBE_INFO,      &match_info,
#if defined(TEST_PD)
                          TPROBE_KEY, &my_TPROBE_KEY,
                          TPROBE_KEYSTRING, &my_TPROBE_KEYSTRING,
                          TPROBE_CNT, &my_TPROBE_CNT,
                          TPROBE_PARENT, &my_TPROBE_PARENT,
                          TPROBE_LAST, &my_TPROBE_LAST,
                          TPROBE_IDENT, &my_TPROBE_IDENT,
                          TPROBE_SEQUENCE, &my_TPROBE_SEQUENCE,  // encoded probe sequence (2=A 3=C 4=G 5=U)
                          TPROBE_QUALITY, &my_TPROBE_QUALITY, // quality of probe ?
#endif // TEST_PD
                          0)) break;


#if defined(TEST_PD)
            if (aisc_get( pd_gl.link, PT_TPROBE, tprobe,
                          TPROBE_GROUPSIZE, &my_TPROBE_GROUPSIZE, // groesse der Gruppe,  die von Sonde getroffen wird?
                          TPROBE_HAIRPIN, &my_TPROBE_HAIRPIN,
                          TPROBE_WHAIRPIN, &my_TPROBE_WHAIRPIN,
                          //                      TPROBE_PERC, &my_TPROBE_PERC,
                          TPROBE_TEMPERATURE, &my_TPROBE_TEMPERATURE,
                          TPROBE_MISHIT, &my_TPROBE_MISHIT, // Treffer ausserhalb von Gruppe ?
                          TPROBE_APOS, &my_TPROBE_APOS, // Alignment-Position
                          TPROBE_ECOLI_POS, &my_TPROBE_ECOLI_POS,
                          0)) break;
#endif // TEST_PD
            tprobe = tprobe_next;

            char *probe,*space;
            probe = strpbrk(match_info,"acgtuACGTU");
            if (probe) space = strchr(probe,' ');
            if (probe && space) {
                *space = 0; probe = strdup(probe);*space=' ';
            }else{
                probe = strdup("");
            }
            pd_gl.pd_design->insert_selection( pd_gl.pd_design_id, match_info, probe );
            free(probe);
            free(match_info);
        }
        pd_gl.pd_design->insert_default_selection( pd_gl.pd_design_id, "default", "" );
        pd_gl.pd_design->update_selection_list( pd_gl.pd_design_id );
    }

    aisc_close(pd_gl.link); pd_gl.link = 0;
    aw_closestatus();
    return;
}

static bool allow_probe_match_event = true;

void probe_match_event(AW_window *aww, AW_CL cl_selection_id, AW_CL cl_count_ptr)
{
    if (allow_probe_match_event) {
        AW_selection_list *selection_id = (AW_selection_list*)cl_selection_id;
        int               *counter      = (int*)cl_count_ptr;
        AW_root           *root         = aww->get_root();
        T_PT_PDC           pdc;
        int                show_status  = 0;
        int                extras       = 1; // mark species and write to temp fields,
        GB_ERROR           error        = 0;

        if (!gb_main) {
            error = "No database";
        }

        if (!error) {
            char *servername = probe_pt_look_for_server(root);
            if (!servername) error = GB_get_error();

            if (!error) {
                if (selection_id) {
                    aww->clear_selection_list(selection_id);
                    pd_assert(!counter);
                    show_status = 1;
                }
                else if (counter) {
                    extras = 0;
                }

                if (show_status) {
                    aw_openstatus("Probe Match");
                    aw_status("Open Connection");
                }

                pd_gl.link = (aisc_com *)aisc_open(servername, &pd_gl.com,AISC_MAGIC_NUMBER);
                free(servername);
            }
        }

        if (!error && !pd_gl.link) {
            error = "Cannot contact PT-server";
        }

        if (!error) {
            if (show_status) aw_status("Initialize Server");
            if (init_local_com_struct() ) error = "Cannot contact PT-server (2)";

            if (!error) {
                aisc_create(pd_gl.link,PT_LOCS, pd_gl.locs, LOCS_PROBE_DESIGN_CONFIG, PT_PDC, &pdc, 0);
                if (probe_design_send_data(root,pdc)) error = "Connection to PT_SERVER lost (2)";
            }
        }

        char *probe = root->awar(AWAR_TARGET_STRING)->read_string();

        if (!error) {
            if (show_status) aw_status("Start Probe Match");

            if (aisc_nput(pd_gl.link,PT_LOCS, pd_gl.locs,
                          LOCS_MATCH_REVERSED,       root->awar(AWAR_PD_MATCH_COMPLEMENT)->read_int(),
                          LOCS_MATCH_SORT_BY,        root->awar(AWAR_PD_MATCH_SORTBY)->read_int(),
                          LOCS_MATCH_COMPLEMENT,     0,
                          LOCS_MATCH_MAX_MISMATCHES, root->awar(AWAR_MAX_MISMATCHES)->read_int(),
                          LOCS_MATCH_MAX_SPECIES,    root->awar(AWAR_PD_MATCH_CLIPHITS)->read_int(),
                          LOCS_SEARCHMATCH,          probe,
                          0))
            {
                error = "Connection to PT_SERVER lost (2)";
            }
            else {
                delete(g_spd);              // delete previous probe data
                g_spd = new saiProbeData;
                transferProbeData(g_spd);

                if (selection_id) {
                    g_spd->setProbeTarget(probe);
                }
            }
        }

        bytestring bs;
        bs.data = 0;

        long matches_truncated = 0;
        if (!error) {
            if (show_status) aw_status("Read the Results");

            T_PT_MATCHLIST  match_list;
            long            match_list_cnt    = 0;
            char           *locs_error        = 0;

            if (aisc_get( pd_gl.link, PT_LOCS, pd_gl.locs,
                          LOCS_MATCH_LIST,        &match_list,
                          LOCS_MATCH_LIST_CNT,    &match_list_cnt,
                          LOCS_MATCH_STRING,      &bs,
                          LOCS_MATCHES_TRUNCATED, &matches_truncated,
                          LOCS_ERROR,             &locs_error,
                          0))
            {
                error = "Connection to PT_SERVER lost (3)";
            }

            if (*locs_error) error = GBS_global_string("%s", locs_error);
            free(locs_error);

            root->awar(AWAR_PD_MATCH_NHITS)->write_string(GBS_global_string(matches_truncated ? "> %li" : "%li", match_list_cnt));
            if (matches_truncated) {
                aw_message("Too many matches - list does not necessarily contain best matches.");
            }
        }

        long mcount                = 0;
        long unknown_species_count = 0;
        long unknown_gene_count    = 0;

        if (!error) {
            if (show_status) aw_status("Parse the Results");

            char        toksep[2]  = { 1, 0 };
            char       *strtok_ptr = 0; // stores strtok position
            const char *hinfo      = strtok_r(bs.data,toksep, &strtok_ptr);

            bool              gene_flag = false;
            ProbeMatchParser *parser    = 0;
            char             *result    = (char*)malloc(1024);


            if (hinfo) {
                g_spd->setHeadline(hinfo);
                parser = new ProbeMatchParser(probe, hinfo);
                error  = parser->get_error();
                if (!error) gene_flag = parser->is_gene_result();
            }

            if (selection_id) {
                int width         = 0;
                if (parser && !error) width = parser->get_probe_region_offset()+2+10; // 2 cause headline is shorter and 10 for match prefix region

                const char *searched = GBS_global_string("%-*s%s", width, "Searched for ", probe);
                aww->insert_selection( selection_id, searched, probe );
                if (hinfo) aww->insert_selection( selection_id, hinfo, "" );
            }


            // clear all marks and delete all 'tmp' entries

            int mark        = extras && (int)root->awar(AWAR_PD_MATCH_MARKHITS)->read_int();
            int write_2_tmp = extras && (int)root->awar(AWAR_PD_MATCH_WRITE2TMP)->read_int();

            GB_push_transaction(gb_main);

            GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
            if (mark && !error) {
                if (show_status) aw_status(gene_flag ? "Unmarking all species and genes" : "Unmarking all species");
                for (GBDATA *gb_species = GBT_first_marked_species_rel_species_data(gb_species_data);
                     gb_species;
                     gb_species = GBT_next_marked_species(gb_species))
                {
                    GB_write_flag(gb_species,0);
                }

                if (gene_flag) {
                    // unmark genes of ALL species
                    for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);
                         gb_species;
                         gb_species = GBT_next_species(gb_species))
                    {
                        for (GBDATA *gb_gene = GEN_first_marked_gene(gb_species);
                             gb_gene;
                             gb_gene = GEN_next_marked_gene(gb_gene))
                        {
                            GB_write_flag(gb_gene,0);
                        }
                    }
                }
            }
            if (write_2_tmp && !error) {
                if (show_status) aw_status("Deleting old 'tmp' fields");
                for (GBDATA *gb_species = GBT_first_species_rel_species_data(gb_species_data);
                     gb_species;
                     gb_species = GBT_next_species(gb_species) )
                {
                    GBDATA *gb_tmp = GB_find(gb_species,"tmp",0,down_level);
                    if (gb_tmp) GB_delete(gb_tmp);
                    if (gene_flag) {
                        for (GBDATA *gb_gene = GEN_first_gene(gb_species);
                             gb_gene;
                             gb_gene = GEN_next_gene(gb_species))
                        {
                            gb_tmp = GB_find(gb_gene, "tmp", 0, down_level);
                            if (gb_tmp) GB_delete(gb_tmp);
                        }
                    }
                }
            }

            // read results from pt-server :

            if (!error) {
                if (show_status) aw_status("Parsing results..");

                g_spd->probeSpecies.clear();
                g_spd->probeSeq.clear();

                if (gene_flag) {
                    if (!GEN_is_genome_db(gb_main, -1)) {
                        error = "Wrong PT-Server chosen (selected one is built for genes)";
                    }
                }
            }

            const char *match_name = 0;
            while (hinfo && !error && (match_name = strtok_r(0,toksep, &strtok_ptr)) ) {
                const char *match_info = strtok_r(0,toksep, &strtok_ptr);
                if (!match_info) break;

                pd_assert(parser);
                ParsedProbeMatch  ppm(match_info, *parser);
                char             *gene_str = 0;

                if (gene_flag) {
                    gene_str = ppm.get_column_content("genename", true);

                    // #error change parse method for gene name
                    // // parse the gene name:
                    // int off = 0;
                    // while (match_info[off] == ' ') ++off; // search first non-space ( = start of species name)
                    // char *space1       = strchr(match_info+off, ' '); // space between species and gene name
                    // char *space2       = 0;
                    // if (space1) space2 = strchr(space1+1, ' '); // space after gene name
                    // if (space1 && space2) { // ok - parsing sucessful
                    //     int len       = space2-space1-1;
                    //     // delete [] gene_str;
                    //     gene_str      = new char[len+1];
                    //     memcpy(gene_str, space1+1, len);
                    //     gene_str[len] = 0;
                    // }
                    // else {
                    //     error = "cannot parse gene name from match result";
                    // }
                }

                if (!error) {
                    char flags[]       = "xx";
                    GBDATA *gb_species = GBT_find_species_rel_species_data(gb_species_data, match_name);

                    if (gb_species) {
                        GBDATA *gb_gene = 0;
                        if (gene_flag && strncmp(gene_str,"intergene_", 10) != 0) { // real gene
                            gb_gene = GEN_find_gene(gb_species,gene_str);
                            if (!gb_gene) {
                                aw_message(GBS_global_string("Gene '%s' not found in organism '%s'", gene_str, match_name));
                            }
                        }

                        if (mark) {
                            GB_write_flag(gb_species, 1);
                            flags[0] = '*';
                            if (gb_gene) {
                                GB_write_flag(gb_gene, 1);
                                flags[1] = '*';
                            }
                            else {
                                flags[1] = '?'; // no gene
                            }
                        }
                        else {
                            flags[0] = " *"[GB_read_flag(gb_species)];
                            flags[1] = " *?"[gb_gene ? GB_read_flag(gb_gene) : 2];
                        }

                        if (write_2_tmp) {
                            GBDATA   *gb_tmp = 0;
                            GB_ERROR  error  = 0;
                            {
                                GBDATA *gb_parent = gene_flag ? gb_gene : gb_species;
                                gb_tmp            = GB_search(gb_parent, "tmp", GB_FIND);
                                if (gb_tmp) {
                                    const char *name    = "<unknown>";
                                    GBDATA     *gb_name = GB_search(gb_parent, "name", GB_FIND);
                                    if (gb_name) name   = GB_read_char_pntr(gb_name);
                                    error               = GBS_global_string("field 'tmp' already exists for %s '%s'", gene_flag ? "gene" : "species", name);
                                    gb_tmp              = 0; // don't overwrite
                                }
                                else {
                                    gb_tmp = GB_search(gb_species, "tmp", GB_STRING);
                                }
                            }

                            if (!error) {
                                pd_assert(gb_tmp);
                                error = GB_write_string(gb_tmp, match_info);
                            }

                            if (error) aw_message(error);
                        }
                    }
                    else {
                        flags[0] = flags[1] = '?'; // species does not exist
                        unknown_species_count++;
                    }


                    if (gene_flag) {
                        sprintf(result, "%s %s", flags, match_info+1); // both flags (skip 1 space from match info to keep alignment)
                        char *gene_match_name = new char[strlen(match_name) + strlen(gene_str)+2];
                        sprintf(gene_match_name,"%s/%s",match_name,gene_str);
                        if (selection_id) aww->insert_selection( selection_id, result, gene_match_name ); // @@@ wert fuer awar eintragen
                    }
                    else {
                        sprintf(result, "%c %s", flags[0], match_info); // only first flag ( = species related)
                        if (selection_id)  aww->insert_selection( selection_id, result, match_name ); // @@@ wert fuer awar eintragen

                        if (selection_id) {  // storing probe data into linked lists
                            g_spd->probeSeq.push_back(strdup(match_info));
                            g_spd->probeSpecies.push_back(strdup(match_name));
                        }
                    }
                    mcount++;
                }

                free(gene_str);
            }

            if (error) error = GBS_global_string("%s", error); // make static copy (error may be free'd by delete parser)
            delete parser;
            free(result);

            GB_pop_transaction(gb_main);
        }

        if (error) {
            root->awar(AWAR_PD_MATCH_NHITS)->write_string("0"); // clear hits
            aw_message(error);
        }
        else {
            if (unknown_species_count>0) {
                aw_message(GBS_global_string("%li matches hit unknown species -- PT-server is out-of-date or build upon a different database", unknown_species_count));
            }
            if (unknown_gene_count>0) {
                aw_message(GBS_global_string("%li matches hit unknown genes -- PT-server is out-of-date or build upon a different database", unknown_gene_count));
            }

            if (selection_id) {     // if !selection_id then probe match window is not opened
                pd_assert(g_spd);
                root->awar(AWAR_SPV_DB_FIELD_NAME)->touch(); // force refresh of SAI/Probe window
            }
        }

        if (counter) *counter = mcount;

        aisc_close(pd_gl.link);
        pd_gl.link = 0;

        if (selection_id) {
            const char *last_line                 = 0;
            if (error) last_line                  = GBS_global_string("****** Error: %s *******", error);
            else if (matches_truncated) last_line = "****** List is truncated *******";
            else        last_line                 = "****** End of List *******";

            aww->insert_default_selection( selection_id, last_line, "" );

            if (show_status) aw_status("Formatting output");
            aww->update_selection_list( selection_id );
        }

        if (show_status) aw_closestatus();

        free(bs.data);
        free(probe);

        allow_probe_match_event = false;
        root->awar(AWAR_TREE_REFRESH)->touch();
        allow_probe_match_event = true;
    }

    return;
}

void probe_match_all_event(AW_window *aww, AW_CL cl_iselection_id) {
    auto_match_cb_settings.disable = true;

    AW_selection_list *iselection_id = (AW_selection_list*)cl_iselection_id;
    AW_root           *root          = aww->get_root();
    char              *target_string = root->awar(AWAR_TARGET_STRING)->read_string();

    aww->init_list_entry_iterator(iselection_id); // init
    aw_openstatus("Matching all resolved strings");

    int string_count = aww->get_no_of_entries(iselection_id);
    int local_count = 0;

    for (;;) {
        const char *entry = aww->get_list_entry_char_value();
        if (!entry) break;

        double percent = local_count++/double(string_count);
        aw_status(GBS_global_string("Match string %i of %i", local_count, string_count));
        aw_status(percent);

        root->awar(AWAR_TARGET_STRING)->write_string(entry); // probe match
        int counter = -1;
        probe_match_event(aww, AW_CL(0), AW_CL(&counter));
        if (counter==-1) break;

        char *buffer = new char[strlen(entry)+10]; // write # of matched to list entries
        sprintf(buffer, "%5i %s", counter, entry);
        aww->set_list_entry_displayed(buffer);
        delete buffer;

        aww->iterate_list_entry(1); // iterate
    }

    aw_closestatus();

    if (local_count) {
        aww->sort_selection_list(iselection_id, 1, 1);
        aww->update_selection_list(iselection_id);
        root->awar(AWAR_TARGET_STRING)->write_string(target_string);
    }

    auto_match_cb_settings.disable = false;
}

static void resolved_probe_chosen(AW_root *root) {
    char *string = root->awar(AWAR_PD_MATCH_RESOLVE)->read_string();
    root->awar(AWAR_TARGET_STRING)->write_string(string);
}

static void selected_match_changed_cb(AW_root *root) {
    // this gets called when ever the selected probe match changes
    char *temp;
    char *selected_match = root->awar(AWAR_PD_SELECTED_MATCH)->read_string();

    if (strchr(selected_match, '/')) { // "organism/gene"
        temp = strtok(selected_match,"/");
        root->awar(AWAR_SPECIES_NAME)->write_string(temp);
        temp = strtok(NULL," /\n");
        root->awar(AWAR_GENE_NAME)->write_string(temp);
    }
    else {
        root->awar(AWAR_SPECIES_NAME)->write_string(selected_match);
    }

    {
        bool prev               = allow_probe_match_event;
        allow_probe_match_event = false; // avoid recursion
        root->awar(AWAR_TARGET_STRING)->touch(); // forces editor to jump to probe match in gene
        allow_probe_match_event = prev;
    }

    free(selected_match);
}

void create_probe_design_variables(AW_root *root,AW_default db1, AW_default global)
{
    char buffer[256]; memset(buffer,0,256);
    int  i;
    pd_gl.pd_design = 0;        /* design result window not created */
    root->awar_string(AWAR_SPECIES_NAME,         "", db1);
    root->awar_string(AWAR_PD_SELECTED_MATCH,    "", db1)->add_callback(selected_match_changed_cb);
    root->awar_float (AWAR_PD_DESIGN_EXP_DTEDGE, .5, db1);
    root->awar_float (AWAR_PD_DESIGN_EXP_DT,     .5, db1);


    double default_bonds[16] = {
        0.0, 0.0, 0.5, 1.1,
        0.0, 0.0, 1.5, 0.0,
        0.5, 1.5, 0.4, 0.9,
        1.1, 0.0, 0.9, 0.0
    };

    for (i=0;i<16;i++) {
        sprintf(buffer,AWAR_PD_DESIGN_EXP_BONDS "%i",i);
        root->awar_float( buffer, default_bonds[i], db1);
        root->awar(buffer)->set_minmax(0,3.0);
    }
#if 0
    for (i=0;i<PROBE_DESIGN_EXCEPTION_MAX;i++){
        sprintf(buffer, "probe_design/exceptions/nr%i/MODE",   i); root->awar_int  (buffer, -1,  db1);
        sprintf(buffer, "probe_design/exceptions/nr%i/LLEFT",  i); root->awar_float(buffer, 0.0, db1);
        sprintf(buffer, "probe_design/exceptions/nr%i/LEFT",   i); root->awar_float(buffer, 0.0, db1);
        sprintf(buffer, "probe_design/exceptions/nr%i/CENTER", i); root->awar_float(buffer, 0.0, db1);
        sprintf(buffer, "probe_design/exceptions/nr%i/RIGHT",  i); root->awar_float(buffer, 0.0, db1);
        sprintf(buffer, "probe_design/exceptions/nr%i/RRIGHT", i); root->awar_float(buffer, 0.0, db1);
    }
#endif
    root->awar_float(AWAR_PD_DESIGN_EXP_SPLIT,  .5, db1);
    root->awar_float(AWAR_PD_DESIGN_EXP_DTEDGE, .5, db1);
    root->awar_float(AWAR_PD_DESIGN_EXP_DT,     .5, db1);

    root->awar_int  (AWAR_PD_DESIGN_CLIPRESULT, 50,   db1)->set_minmax(0, 1000  );
    root->awar_int  (AWAR_PD_DESIGN_MISHIT,     0,    db1)->set_minmax(0, 100000);
    root->awar_int  (AWAR_PD_DESIGN_MAXBOND,    4,    db1)->set_minmax(0, 20    );
    root->awar_float(AWAR_PD_DESIGN_MINTARGETS, 50.0, db1)->set_minmax(0, 100   );

    root->awar_int  (AWAR_PD_DESIGN_PROBELENGTH,  18,     db1)->set_minmax(10, 100    );
    root->awar_float(AWAR_PD_DESIGN_MIN_TEMP,     50.0,   db1)->set_minmax(0,  1000   );
    root->awar_float(AWAR_PD_DESIGN_MAX_TEMP,     100.0,  db1)->set_minmax(0,  1000   );
    root->awar_float(AWAR_PD_DESIGN_MIN_GC,       50.0,   db1)->set_minmax(0,  100    );
    root->awar_float(AWAR_PD_DESIGN_MAX_GC,       100.0,  db1)->set_minmax(0,  100    );
    root->awar_int  (AWAR_PD_DESIGN_MIN_ECOLIPOS, 0,      db1)->set_minmax(0,  1000000);
    root->awar_int  (AWAR_PD_DESIGN_MAX_ECOLIPOS, 100000, db1)->set_minmax(0,  1000000);

    root->awar_int(AWAR_PT_SERVER,      0, db1);
    root->awar_int(AWAR_PD_DESIGN_GENE, 0, db1);

    root->awar_int   (AWAR_PD_MATCH_MARKHITS,   1,    db1   );
    root->awar_int   (AWAR_PD_MATCH_SORTBY,     0,    db1   );
    root->awar_int   (AWAR_PD_MATCH_WRITE2TMP,  0,    db1   );
    root->awar_int   (AWAR_PD_MATCH_COMPLEMENT, 0,    db1   );
    root->awar_int   (AWAR_MIN_MISMATCHES,      0,    global);
    root->awar_int   (AWAR_MAX_MISMATCHES,      0,    global);
    root->awar_int   (AWAR_PD_MATCH_CLIPHITS,   1000, db1   );
    root->awar_string(AWAR_TARGET_STRING,       "",   global);
    root->awar_string(AWAR_PD_MATCH_NHITS,      0,    db1   );
    root->awar_int   (AWAR_PD_MATCH_AUTOMATCH,  0,    db1   )->add_callback(auto_match_changed);

    root->awar_string(AWAR_PD_MATCH_RESOLVE, "", db1)->add_callback(resolved_probe_chosen);
    root->awar_string(AWAR_ITARGET_STRING, "", global);

    root->awar_int   (AWAR_PROBE_ADMIN_PT_SERVER,    0,  db1   );
    root->awar_int   (AWAR_PROBE_CREATE_GENE_SERVER, 0,  db1   );
    
    root->awar_string(AWAR_SPV_SAI_2_PROBE,    "",     global); // name of SAI selected in list
    root->awar_string(AWAR_SPV_DB_FIELD_NAME,  "name", global); // name of displayed species field
    root->awar_int   (AWAR_SPV_DB_FIELD_WIDTH, 10,     global); // width of displayed species field
    root->awar_string(AWAR_SPV_ACI_COMMAND,    "",     global); // User defined or pre-defined ACI command to display
    root->awar_string(AWAR_SPV_SELECTED_PROBE, "",     global); // For highlighting the selected PROBE
}

AW_window *create_probe_design_expert_window( AW_root *root)  {
    int i;
    char buffer[256];
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "PD_EXPERT","PD-SPECIALS");

    aws->load_xfig("pd_spec.fig");
    aws->label_length(30);
    aws->button_length(10);

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP, (AW_CL)"pd_spec_param.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","C");

    aws->at("dt");
    aws->create_input_field(AWAR_PD_DESIGN_EXP_DT,6);

    aws->at("dt_edge");
    aws->create_input_field(AWAR_PD_DESIGN_EXP_DTEDGE,6);

    aws->at("split");
    aws->create_input_field(AWAR_PD_DESIGN_EXP_SPLIT,6);


    for (i=0;i<16;i++) {
        sprintf(buffer,"%i",i);
        aws->at(buffer);
        sprintf(buffer,AWAR_PD_DESIGN_EXP_BONDS "%i",i);
        aws->create_input_field(buffer,4);
    }

#if 0
    for (i=0;i<PROBE_DESIGN_EXCEPTION_MAX;i++) {
        sprintf(buffer,"m%i",i);
        aws->at(buffer);
        sprintf(buffer,"probe_design/exceptions/nr%i/MODE",i);
        aws->create_option_menu( buffer, NULL , "" );
        aws->insert_option( "dont use", "d", -1 );
        aws->insert_option( "dont split", "d", 0 );
        aws->insert_option( "split", "d", 1 );
        aws->update_option_menu();

        sprintf(buffer,"ll%i",i);
        aws->at(buffer);
        sprintf(buffer,"probe_design/exceptions/nr%i/LLEFT",i);
        aws->create_input_field(buffer,4);

        sprintf(buffer,"l%i",i);
        aws->at(buffer);
        sprintf(buffer,"probe_design/exceptions/nr%i/LEFT",i);
        aws->create_input_field(buffer,5);

        sprintf(buffer,"c%i",i);
        aws->at(buffer);
        sprintf(buffer,"probe_design/exceptions/nr%i/CENTER",i);
        aws->create_input_field(buffer,5);

        sprintf(buffer,"r%i",i);
        aws->at(buffer);
        sprintf(buffer,"probe_design/exceptions/nr%i/RIGHT",i);
        aws->create_input_field(buffer,5);

        sprintf(buffer,"rr%i",i);
        aws->at(buffer);
        sprintf(buffer,"probe_design/exceptions/nr%i/RRIGHT",i);
        aws->create_input_field(buffer,5);
    }
#endif
    return aws;
}

static AWT_config_mapping_def probe_design_mapping_def[] = {
    { AWAR_PD_DESIGN_CLIPRESULT,   "clip" }, 
    { AWAR_PD_DESIGN_MISHIT,       "mishit" }, 
    { AWAR_PD_DESIGN_MAXBOND,      "maxbond" }, 
    { AWAR_PD_DESIGN_MINTARGETS,   "mintarget"}, 
    { AWAR_PD_DESIGN_PROBELENGTH,  "probelen" }, 
    { AWAR_PD_DESIGN_MIN_TEMP,     "mintemp" }, 
    { AWAR_PD_DESIGN_MAX_TEMP,     "maxtemp" }, 
    { AWAR_PD_DESIGN_MIN_GC,       "mingc" }, 
    { AWAR_PD_DESIGN_MAX_GC,       "maxgc" }, 
    { AWAR_PD_DESIGN_MIN_ECOLIPOS, "minecoli" }, 
    { AWAR_PD_DESIGN_MAX_ECOLIPOS, "maxecoli" }, 
    { AWAR_PD_DESIGN_GENE,         "gene" }, 
    { AWAR_PD_DESIGN_EXP_SPLIT,    "split" }, 
    { AWAR_PD_DESIGN_EXP_DTEDGE,   "dtedge" }, 
    { AWAR_PD_DESIGN_EXP_DT,       "dt" },
    { 0, 0 }
};

static void probe_design_init_config(AWT_config_definition& cdef) {
    cdef.add(probe_design_mapping_def);
    for (int i = 0; i<16; ++i) {
        cdef.add(GBS_global_string(AWAR_PD_DESIGN_EXP_BONDS "%i", i), "bond", i);
    }
}

static char *probe_design_store_config(AW_window *aww, AW_CL, AW_CL) {
    AWT_config_definition cdef(aww->get_root());
    probe_design_init_config(cdef);
    return cdef.read();
}
static void probe_design_restore_config(AW_window *aww, const char *stored_string, AW_CL, AW_CL) {
    AWT_config_definition cdef(aww->get_root());
    probe_design_init_config(cdef);
    cdef.write(stored_string);
}

void probe_design_save_default(AW_window *aw,AW_default aw_def)
{
    AW_root *aw_root = aw->get_root();
    aw_root->save_default(aw_def,0);
}


AW_window *create_probe_design_window( AW_root *root, AW_CL cl_genome_db)  {
    AW_window_simple *aws         = new AW_window_simple;
    int               is_genom_db = (int)cl_genome_db;

    aws->init( root, "PROBE_DESIGN","PROBE DESIGN");

    aws->load_xfig("pd_main.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"probedesign.hlp");
    aws->create_button("HELP","HELP","H");

    aws->callback( probe_design_event);
    aws->at("design");
    aws->highlight();
    aws->create_button("GO","GO","G");

    //  aws->callback( (AW_CB1)AW_save_defaults, (int) def);
    //  aws->at("save_default");
    //  aws->create_button("SAVE DEFAULTS","S");

    aws->callback( probe_design_create_result_window);
    aws->at("result");
    aws->create_button("RESULT","RESULT","S");

    //  aws->callback( AW_POPUP, (AW_CL)create_probe_design_window, (int)def);
    //  aws->at("open");
    //  aws->create_button("REOPEN","REOPEN","S");

    aws->callback( (AW_CB1)AW_POPUP,(AW_CL)create_probe_design_expert_window);
    aws->at("expert");
    aws->create_button("EXPERT","EXPERT","S");

    aws->at( "pt_server" );
    aws->label("PT-Server:");
    awt_create_selection_list_on_pt_servers(aws,AWAR_PT_SERVER,AW_TRUE);

    aws->at("lenout"  ); aws->create_input_field(AWAR_PD_DESIGN_CLIPRESULT, 6);
    aws->at("mishit"  ); aws->create_input_field(AWAR_PD_DESIGN_MISHIT,     6);
    aws->at("maxbonds"); aws->create_input_field(AWAR_PD_DESIGN_MAXBOND,    6);
    aws->at("minhits" ); aws->create_input_field(AWAR_PD_DESIGN_MINTARGETS, 6);

    aws->at("minlen"); aws->create_input_field(AWAR_PD_DESIGN_PROBELENGTH,  5);
    aws->at("mint"  ); aws->create_input_field(AWAR_PD_DESIGN_MIN_TEMP,     5);
    aws->at("maxt"  ); aws->create_input_field(AWAR_PD_DESIGN_MAX_TEMP,     5);
    aws->at("mingc" ); aws->create_input_field(AWAR_PD_DESIGN_MIN_GC,       5);
    aws->at("maxgc" ); aws->create_input_field(AWAR_PD_DESIGN_MAX_GC,       5);
    aws->at("minpos"); aws->create_input_field(AWAR_PD_DESIGN_MIN_ECOLIPOS, 5);
    aws->at("maxpos"); aws->create_input_field(AWAR_PD_DESIGN_MAX_ECOLIPOS, 5);

    if (is_genom_db) {
        aws->at("gene");
        aws->label("Gene probes?");
        aws->create_toggle(AWAR_PD_DESIGN_GENE);
    }

    aws->at("save");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "probe_design", probe_design_store_config, probe_design_restore_config, 0, 0);

    return aws;
}

void print_event( AW_window *aww, AW_CL selection_id, AW_CL name ) {
    char *filename = (char *)name;
    aww->save_selection_list( (AW_selection_list *)selection_id, filename );
}

// -------------------------------------------------------------------


inline void my_strupr(char *s) {
    pd_assert(s);
    for (int i=0; s[i]; i++) {
        s[i] = toupper(s[i]);
    }
}

static GB_alignment_type ali_used_for_resolvement = GB_AT_UNKNOWN;

static void resolve_IUPAC_target_string(AW_root *, AW_CL cl_aww, AW_CL cl_selid) {
    AW_window         *aww          = (AW_window*)cl_aww;
    AW_selection_list *selection_id = (AW_selection_list*)cl_selid;

    aww->clear_selection_list(selection_id);

    if (ali_used_for_resolvement != GB_AT_RNA && ali_used_for_resolvement!=GB_AT_DNA) {
        aww->insert_default_selection(selection_id, "Wrong alignment type!", "");
        aww->update_selection_list(selection_id);
        return;
    }

    int       index   = ali_used_for_resolvement==GB_AT_RNA ? 1 : 0;
    AW_root  *root    = aww->get_root();
    char     *istring = root->awar(AWAR_ITARGET_STRING)->read_string();
    GB_ERROR  err     = 0;

    if (istring && istring[0]) { // contains sth?
        my_strupr(istring);

        int bases_to_resolve = 0;
        char *istr = istring;
        int istring_length = strlen(istring);

        for (;;) {
            char i = *istr++;
            if (!i) break;
            if (i=='?') continue; // ignore '?'

            int idx = i-'A';
            if (idx<0 || idx>=26 || AWT_iupac_code[idx][index].iupac==0) {
                err = GB_export_error("Illegal character '%c' in IUPAC-String", i);
                break;
            }

            if (AWT_iupac_code[idx][index].count>1) {
                bases_to_resolve++;
            }
        }

        if (!err) {
            int *offsets_to_resolve = new int[bases_to_resolve];
            int resolutions = 1;
            {
                istr = istring;
                int offset = 0;
                int offset_count = 0;
                for (;;) {
                    char i = *istr++;
                    if (!i) break;

                    if (i!='?') {
                        int idx = AWT_iupac2index(i);
                        pd_assert(AWT_iupac_code[idx][index].iupac);

                        if (AWT_iupac_code[idx][index].count>1) {
                            offsets_to_resolve[offset_count++] = offset; // store string offsets of non-unique base-codes
                            resolutions *= AWT_iupac_code[idx][index].count; // calc # of resolutions
                        }
                    }
                    offset++;
                }
            }

            int cont = 1;
            if (resolutions>5000) {
                const char *warning = GBS_global_string("Resolution of this string will result in %i single strings", resolutions);
                cont = aw_message(warning, "Abort,Continue");
            }

            if (cont) { // continue with resolution?
                int *resolution_idx = new int[bases_to_resolve];
                int *resolution_max_idx = new int[bases_to_resolve];
                {
                    int i;
                    for (i=0; i<bases_to_resolve; i++) {
                        resolution_idx[i] = 0;
                        int idx = AWT_iupac2index(istring[offsets_to_resolve[i]]);
                        resolution_max_idx[i] = AWT_iupac_code[idx][index].count-1;
                    }
                }

                char *buffer = new char[istring_length+1];
                int not_last = resolutions-1;

                for (;;) {
                    // create string according to resolution_idx[]:
                    int i;

                    memcpy(buffer, istring, istring_length+1);
                    for (i=0; i<bases_to_resolve; i++) {
                        int off = offsets_to_resolve[i];
                        int idx = AWT_iupac2index(istring[off]);

                        pd_assert(AWT_iupac_code[idx][index].iupac);
                        buffer[off] = AWT_iupac_code[idx][index].iupac[resolution_idx[i]];
                    }

                    /*if (not_last) */  aww->insert_selection        (selection_id, buffer, buffer);
                    /*else      aww->insert_default_selection(selection_id, buffer, buffer); */
                    not_last--;

                    // permutatate indices:
                    int nidx = bases_to_resolve-1;
                    int done = 0;
                    while (!done && nidx>=0) {
                        if (resolution_idx[nidx]<resolution_max_idx[nidx]) {
                            resolution_idx[nidx]++;
                            done = 1;
                            break;
                        }
                        nidx--;
                    }
                    if (!done) break; // we did all permutations!

                    nidx++; // do not touch latest incremented index
                    while (nidx<bases_to_resolve) resolution_idx[nidx++] = 0; // zero all other indices
                }

                delete buffer;
                delete resolution_max_idx;
                delete resolution_idx;

                aww->insert_default_selection(selection_id, "", "");
                aww->update_selection_list(selection_id);
            }

            delete offsets_to_resolve;
        }
    }

    if (err) {
        aw_message(err);
    }
}

enum ModMode { TS_MOD_CLEAR, TS_MOD_REV_COMPL, TS_MOD_COMPL };

static void modify_target_string(AW_window *aww, AW_CL cl_mod_mode) {
    ModMode   mod_mode      = ModMode(cl_mod_mode);
    AW_root  *root          = aww->get_root();
    char     *target_string = root->awar(AWAR_TARGET_STRING)->read_string();
    GB_ERROR  error         = 0;

    if (mod_mode == TS_MOD_CLEAR) {
        target_string[0] = 0;
    }
    else {
        GB_transaction dummy(gb_main);
        GB_alignment_type ali_type = GBT_get_alignment_type(gb_main, GBT_get_default_alignment(gb_main));
        if (mod_mode == TS_MOD_REV_COMPL) {
            char T_or_U;
            error = GBT_determine_T_or_U(ali_type, &T_or_U, "reverse-complement");
            if (!error) GBT_reverseComplementNucSequence(target_string, strlen(target_string), T_or_U);
        }
        else if (mod_mode == TS_MOD_COMPL) {
            char T_or_U;
            error = GBT_determine_T_or_U(ali_type, &T_or_U, "complement");
            if (!error) {
                char *new_target_string = GBT_complementNucSequence(target_string, strlen(target_string), T_or_U);
                free(target_string);
                target_string           = new_target_string;
            }
        }
    }

    if (error) {
        aw_message(error);
    }
    else {
        root->awar(AWAR_TARGET_STRING)->write_string(target_string);
    }
    free(target_string);
}

AW_window *create_IUPAC_resolve_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "PROBE_MATCH_RESOLVE_IUPAC","Resolve IUPAC for Probe Match");
    aws->load_xfig("pd_match_iupac.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP, (AW_CL)"pd_match_iupac.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","C");

    AW_selection_list *iselection_id;
    aws->at( "iresult" );
    iselection_id = aws->create_selection_list( AWAR_PD_MATCH_RESOLVE, NULL, "", 32, 15 );
    aws->insert_default_selection( iselection_id, "---empty---", "" );

    aws->at("istring");
    aws->create_input_field(AWAR_ITARGET_STRING,32);

    // automatically resolve AWAR_ITARGET_STRING:
    {
        GB_transaction dummy(gb_main);
        ali_used_for_resolvement = GBT_get_alignment_type(gb_main, GBT_get_default_alignment(gb_main));
    }
    root->awar(AWAR_ITARGET_STRING)->add_callback(resolve_IUPAC_target_string, AW_CL(aws), AW_CL(iselection_id));

    aws->callback(probe_match_all_event, (AW_CL)iselection_id);
    aws->at("match_all");
    aws->create_button("MATCH_ALL", "MATCH ALL", "A");

    return aws;
}

static void matchSaiProbe(AW_window *aw) {
    AW_root *awr = aw->get_root();
    static AW_window *awExists = 0;

    if (!awExists) {
        awExists = createSaiProbeMatchWindow(awr);
    }
    if(g_spd) transferProbeData(g_spd); //transferring probe data to saiProbeMatch function

    awExists->show();
}

AW_window *create_probe_match_window( AW_root *root,AW_default)  {
    static AW_window_simple *aws = 0; // the one and only probe match window
    if (aws) return aws;

    aws = new AW_window_simple;

    aws->init( root, "PROBE_MATCH", "PROBE MATCH");

    aws->load_xfig("pd_match.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"probematch.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->at("string");
    aws->create_input_field(AWAR_TARGET_STRING,32);

    AW_selection_list *selection_id;
    aws->at( "result" );
    selection_id = aws->create_selection_list( AWAR_PD_SELECTED_MATCH, NULL, "", 110, 15 );
    aws->insert_default_selection( selection_id, "****** No results yet *******", "" ); // if list is empty -> crashed if new species was selected in ARB_EDIT4

    aws->callback( create_print_box_for_selection_lists, (AW_CL)selection_id );
    aws->at("print");
    aws->create_button("PRINT","PRINT","P");

    aws->at("matchSai");
    aws->callback(matchSaiProbe);
    aws->create_button("MATCH_SAI","Match SAI","S");

    aws->callback( (AW_CB1)AW_POPUP,(AW_CL)create_probe_design_expert_window);
    aws->at("expert");
    aws->create_button("EXPERT","EXPERT","X");

    aws->at( "pt_server" );
    awt_create_selection_list_on_pt_servers(aws,AWAR_PT_SERVER,AW_TRUE);

    aws->at( "complement" );
    aws->create_toggle( AWAR_PD_MATCH_COMPLEMENT );

    aws->at( "mark" );
    aws->create_toggle( AWAR_PD_MATCH_MARKHITS );

    aws->at( "weighted" );
    aws->create_toggle( AWAR_PD_MATCH_SORTBY );

    aws->at( "mismatches" );
    aws->create_option_menu(AWAR_MAX_MISMATCHES, NULL , "" );
    aws->insert_default_option( "Search up to zero mismatches", "", 0 );
    for (int mm = 1; mm <= 20; ++mm) {
        aws->insert_option(GBS_global_string("Search up to %i mismatches", mm), "", mm);
    }
    aws->update_option_menu();

    aws->at("tmp");
    aws->create_toggle( AWAR_PD_MATCH_WRITE2TMP );

    aws->at("nhits");
    aws->create_button(0,AWAR_PD_MATCH_NHITS);

    aws->callback(probe_match_event,(AW_CL)selection_id, (AW_CL)0);
    aws->at("match");
    aws->create_button("MATCH","MATCH","D");

    aws->at("auto");
    aws->label("Auto");
    aws->create_toggle( AWAR_PD_MATCH_AUTOMATCH );
    enable_auto_match_cb(root, aws, (AW_CL)selection_id);

    aws->callback(modify_target_string, (AW_CL)TS_MOD_CLEAR);
    aws->at("clear");
    aws->create_button("CLEAR","Clear","0");

    aws->callback(modify_target_string, (AW_CL)TS_MOD_REV_COMPL);
    aws->at("revcompl");
    aws->create_button("REVCOMPL","RevCompl","R");

    aws->callback(modify_target_string, (AW_CL)TS_MOD_COMPL);
    aws->at("compl");
    aws->create_button("COMPL","Compl","C");

    aws->callback( (AW_CB1)AW_POPUP,(AW_CL)create_IUPAC_resolve_window);
    aws->at("iupac");
    aws->create_button("IUPAC","IUPAC","I");

    return aws;
}

void pd_start_pt_server(AW_window *aww)
{
    AW_root *awr = aww->get_root();
    char pt_server[256];
    sprintf(pt_server,"ARB_PT_SERVER%li",awr->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int());
    GB_ERROR error;
    aw_openstatus("Start a server");
    aw_status("Look for server or start one");
    error = arb_look_and_start_server(AISC_MAGIC_NUMBER,pt_server,gb_main);
    if (error) {
        aw_message(error);
    }
    aw_closestatus();
}

void pd_kill_pt_server(AW_window *aww, AW_CL kill_all)
{
    AW_root *awr = aww->get_root();
    char pt_server[256];
    long min = 0;
    long max = 1000;
    long i;
    if (!kill_all) min = max = awr->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int();
    if (!aw_message("Are you sure to kill a server","YES,CANCEL")){
        aw_openstatus("Kill a server");
        GB_ERROR error;
        for (i= min ; i <=max ; i++) {
            char *choice = GBS_ptserver_id_to_choice((int)i);
            if (!choice) break;
            sprintf(AW_ERROR_BUFFER,"Try to call '%s'",choice);
            aw_status(AW_ERROR_BUFFER);
            sprintf(pt_server,"ARB_PT_SERVER%li",i);
            error = arb_look_and_kill_server(AISC_MAGIC_NUMBER,pt_server);
            if (error) {
                sprintf(AW_ERROR_BUFFER,"NOP    '%s' -- %s",choice,error);
                aw_message();
            }else{
                sprintf(AW_ERROR_BUFFER,"Killed '%s' killed",choice);
                aw_message();
            }
            free(choice);
        }
        aw_closestatus();
    }
}

void pd_query_pt_server(AW_window *aww)
{
    AW_root *awr = aww->get_root();
    char *server;
    char pt_server[256];
    sprintf(pt_server,"ARB_PT_SERVER%li",awr->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int());
    server = GBS_read_arb_tcp(pt_server);
    char *ssh = 0;
    if ( server[0] && strchr(server+1,':') ) {
        strchr(server+1,':') [0] = 0;
        ssh = (char *)calloc(1, 4+10+strlen(server) );
        sprintf(ssh,"ssh %s ",server);
    }else{
        ssh = strdup("");
    }
    void *strstruct = GBS_stropen(1024);
    GBS_strcat(strstruct,   "echo Contents of directory ARBHOME/lib/pts:;echo;"
               "(cd $ARBHOME/lib/pts; ls -l);"
               "echo; echo Disk Space for PT_server files:; echo;"
               "df $ARBHOME/lib/pts;");
    GBS_strcat(strstruct,"echo;echo Running ARB Programms:;");
    GBS_strcat(strstruct,ssh);
    GBS_strcat(strstruct,"$ARBHOME/bin/arb_who");
    char *sys = GBS_strclose(strstruct);
    GB_xcmd(sys,GB_TRUE, GB_FALSE);
    free(sys);
    free(ssh);
}

void pd_export_pt_server(AW_window *aww)
{
    AW_root  *awr                = aww->get_root();
    char      pt_server[256];
    char     *server;
    char     *file;
    GB_ERROR  error              = 0;

    bool create_gene_server = awr->awar(AWAR_PROBE_CREATE_GENE_SERVER)->read_int();
    {
        GB_transaction ta(gb_main);
        if (create_gene_server && !GEN_is_genome_db(gb_main, -1)) create_gene_server = false;

        // check alignment first
        if (create_gene_server) {
            GBDATA *gb_ali = GBT_get_alignment(gb_main, GENOM_ALIGNMENT);
            if (!gb_ali) {
                error             = GB_get_error();
                if (!error) error = "cannot find alignment '" GENOM_ALIGNMENT "'";
            }
        }
        else { // normal pt server
            char              *use     = GBT_get_default_alignment(gb_main);
            GB_alignment_type  alitype = GBT_get_alignment_type(gb_main, use);
            if (alitype == GB_AT_AA) error = "The PT-server does only support RNA/DNA sequence data";
            free(use);
        }
    }

    sprintf(pt_server,"ARB_PT_SERVER%li",awr->awar(AWAR_PROBE_ADMIN_PT_SERVER)->read_int());
    
    if (!error &&
        aw_message("This function will send your currently loaded data as the new data to the pt_server !!!\n"
                   "The server will need a long time (up to several hours) to analyse the data.\n"
                   "Until the new server has analyzed all data, no server functions are available.\n\n"
                   "Note 1: You must have the write permissions to do that ($ARBHOME/lib/pts/xxx))\n"
                   "Note 2: The server will do the job in background,\n"
                   "        quitting this program won't affect the server","Cancel,Do it"))
    {
        aw_openstatus("Export db to server");
        aw_status("Search server to kill");
        arb_look_and_kill_server(AISC_MAGIC_NUMBER,pt_server);
        server = GBS_read_arb_tcp(pt_server);
        file = server;              /* i got the machine name of the server */
        if (*file) file += strlen(file)+1;  /* now i got the command string */
        if (*file) file += strlen(file)+1;  /* now i got the file */
        if (*file == '-') file += 2;

        aw_status("Exporting the database");
        {
            const char *mode = "bfm"; // save PT-server database with Fastload file

            if (create_gene_server) {
                char *temp_server_name = GBS_string_eval(file, "*.arb=*_temp.arb", 0);
                error = GB_save_as(gb_main, temp_server_name, mode);

                if (!error) {
                    // convert database (genes -> species)
                    aw_status("Preparing database for gene PT server");
                    char *command = GBS_global_string_copy("$ARBHOME/bin/arb_gene_probe %s %s", temp_server_name, file);
                    printf("Executing '%s'\n", command);
                    int result = system(command);
                    if (result != 0) {
                        error = GBS_global_string("Couldn't convert database for gene pt server (arb_gene_probe failed, see console for reason)");
                    }
                    free(command);
                }
                free(temp_server_name);
            }
            else { // normal pt_server
                error = GB_save_as(gb_main, file, mode);
            }
        }

        if (!error) { // set pt-server database file to same permissions as pts directory
            char *dir = strrchr(file,'/');
            if (dir) {
                *dir = 0;
                long modi = GB_mode_of_file(file);
                *dir = '/';
                modi &= 0666;
                error = GB_set_mode_of_file(file,modi);
            }
        }

        if (!error ) {
            aw_status("Start new server");
            error = arb_start_server(pt_server,gb_main, 1);
        }
        aw_closestatus();
    }
    if (error) aw_message(error);
}

void pd_edit_arb_tcp(AW_window *aww){
    awt_edit(aww->get_root(),"$(ARBHOME)/lib/arb_tcp.dat",900,400);
}
AW_window *create_probe_admin_window( AW_root *root, AW_CL cl_genome_db)  {
    AW_window_simple *aws         = new AW_window_simple;
    bool              is_genom_db = (bool)cl_genome_db;

    aws->init( root, "PT_SERVER_ADMIN", "PT_SERVER ADMIN");

    aws->load_xfig("pd_admin.fig");


    aws->callback( AW_POPUP_HELP,(AW_CL)"probeadmin.hlp");
    aws->at("help");
    aws->create_button("HELP","HELP","H");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->button_length(18);

    aws->at( "pt_server" );
    awt_create_selection_list_on_pt_servers(aws, AWAR_PROBE_ADMIN_PT_SERVER, AW_FALSE);

    aws->at( "start" );
    aws->callback(pd_start_pt_server);
    aws->create_button("START_SERVER","Start server");

    aws->at( "kill" );
    aws->callback(pd_kill_pt_server,0);
    aws->create_button("KILL_SERVER","Kill server");

    aws->at( "query" );
    aws->callback(pd_query_pt_server);
    aws->create_button("CHECK_SERVER","Check server");

    aws->at( "kill_all" );
    aws->callback(pd_kill_pt_server,1);
    aws->create_button("KILL_ALL_SERVERS","Kill all servers");

    aws->at( "edit" );
    aws->callback(pd_edit_arb_tcp);
    aws->create_button("CREATE_TEMPLATE","Configure");

    aws->at( "export" );
    aws->callback(pd_export_pt_server);
    aws->create_button("UPDATE_SERVER","Update server");

    if (is_genom_db) {
        aws->at( "gene_server" );
        aws->label("Gene server?");
        aws->create_toggle(AWAR_PROBE_CREATE_GENE_SERVER);
    }

    return aws;
}

// --------------------------------------------------------------------------------
// Probe group result visualization

#define AWAR_PG_SELECTED_RESULT "tmp/pg_result/selected"

static struct pg_global_struct {
    char *awar_pg_result_filename; // awar containing result filename
    AW_window_simple *pg_groups_window; // window showing groups
    AW_selection_list *selList; // list containing groups
    AW_root *aw_root;
    AWT_canvas *ntw; // canvas of main window

    char *pg_filename;  // database containing results (from one result file)
    GBDATA *pg_main;    // root-handle of database

} pg_global;



static void pg_result_selected(AW_window */*aww*/) {

    AW_root *aw_root = pg_global.aw_root;
    long position = aw_root->awar(AWAR_PG_SELECTED_RESULT)->read_int();

    if (position) { // ignore headline
        long i = 1;

        GB_transaction dummy(pg_global.pg_main);
        GB_transaction dummy2(gb_main);

        GBT_mark_all(gb_main, 0); // unmark all species

        GBDATA *gb_species_data = GB_search(gb_main, "species_data", GB_FIND);
        gb_assert(gb_species_data);
        GBDATA *pg_group = GB_search(pg_global.pg_main, "probe_groups/group", GB_FIND);
        long count = 0;
        long marked = 0;
        for (; pg_group;pg_group=GB_find(pg_group, 0, 0, this_level+search_next), ++i) {
            if (i==position) {
                GBDATA *pg_species = GB_search(pg_group, "species/name", GB_FIND);
                for (; pg_species; pg_species=GB_find(pg_species, 0, 0, this_level+search_next), count++)  {
                    const char *name = GB_read_char_pntr(pg_species);
                    GBDATA *gb_speciesname = GB_find(gb_species_data, "name", name, down_2_level);
                    if (gb_speciesname) {
                        GBDATA *gb_species = GB_get_father(gb_speciesname);
                        gb_assert(gb_species);
                        GB_write_flag(gb_species, 1); // mark species
                        if (!marked) { // select first species
                            aw_root->awar(AWAR_SPECIES_NAME)->write_string(name);
                        }
                        marked++;
                    }
                    else {
                        aw_message(GBS_global_string("No species '%s' in current DB", name));
                    }
                }
                break;
            }
        }

        if (marked!=count) {
            aw_message(GBS_global_string("%li species were in group - %li species marked in database", count, marked));
        }

        NT_group_not_marked_cb(0, pg_global.ntw);
    }
}


// --------------------------------------------------------------------------------
//     static void create_probe_group_result_awars(AW_root *aw_root)
// --------------------------------------------------------------------------------
static void create_probe_group_result_awars(AW_root *aw_root) {
    aw_root->awar_int(AWAR_PG_SELECTED_RESULT, 0);
}

// --------------------------------------------------------------------------------
//     static void create_probe_group_result_sel_box(AW_root *aw_root, AW_window *aws)
// --------------------------------------------------------------------------------
static void create_probe_group_result_sel_box(AW_root *aw_root, AW_window *aws) {
    char *file_name = aw_root->awar(pg_global.awar_pg_result_filename)->read_string();

    AW_selection_list *selList = pg_global.selList;

    static bool awars_created;
    if (!awars_created) {
        create_probe_group_result_awars(aw_root);
        awars_created = 1;
    }
    aw_root->awar(AWAR_PG_SELECTED_RESULT)->write_int(0); // reset to element #0

    if (selList==0) {
        aws->at("box");
        aws->callback(pg_result_selected);
        selList = pg_global.selList = aws->create_selection_list(AWAR_PG_SELECTED_RESULT, 0, "", 2, 2);
    }
    else {
        aws->clear_selection_list(selList);
    }

    GB_ERROR error = 0;

    if (pg_global.pg_main) {
        GB_close(pg_global.pg_main);
        free(pg_global.pg_filename);

        pg_global.pg_main = 0;
        pg_global.pg_filename = 0;
    }

    pg_global.aw_root = aw_root;
    pg_global.pg_filename = strdup(file_name);
    pg_global.pg_main = GB_open(file_name, "r");

    if (pg_global.pg_main) {
        const char *reason = 0;
        long i = 0;
        aws->insert_selection(selList, "members | probes | fitness | quality | mintarget | mishit | probelen | birth", i++);

        GB_transaction dummy(pg_global.pg_main);
        GBDATA *pg_group = GB_search(pg_global.pg_main, "probe_groups/group", GB_FIND);
        if (pg_group) {
            for (; pg_group;pg_group=GB_find(pg_group, 0, 0, this_level+search_next), ++i) {

                double fitness=-1, quality=-1, min_targets=-1;
                int mishit=-1, probelength=-1, birth=-1;
                int member_count = 0;
                int probe_count = 0;

                GBDATA *field;

                field = GB_find(pg_group, "fitness", 0, down_level);        if (field) fitness = GB_read_float(field);
                field = GB_find(pg_group, "quality", 0, down_level);        if (field) quality = GB_read_float(field);
                field = GB_find(pg_group, "min_targets", 0, down_level);    if (field) min_targets = GB_read_float(field);
                field = GB_find(pg_group, "mishit", 0, down_level);         if (field) mishit = GB_read_int(field);
                field = GB_find(pg_group, "probelength", 0, down_level);    if (field) probelength = GB_read_int(field);
                field = GB_find(pg_group, "birth", 0, down_level);          if (field) birth = GB_read_int(field);

                GBDATA *species = GB_search(pg_group, "species/name", GB_FIND);
                for (; species; species=GB_find(species, 0, 0, this_level+search_next)) member_count++;

                GBDATA *probe = GB_search(pg_group, "probes/data", GB_FIND);
                for (; probe; probe=GB_find(probe, 0, 0, this_level+search_next)) probe_count++;

                char entry[200];

                sprintf(entry, "%7i | %6i | %7.5f | %7.1f | %9.2f | %6i | %8i | %5i",
                        member_count,
                        probe_count,
                        fitness,
                        quality,
                        min_targets,
                        mishit,
                        probelength,
                        birth);

                aws->insert_selection(selList, entry, i);
            }
        }
        else {
            reason = "probe_group/group - no such entry";
        }

        if (reason) {
            error = GB_export_error("Error in database format (reason: %s)", reason);
            aws->insert_selection(selList, error, (long)0);
        }

        aws->update_selection_list(selList);
    }
    else {
        error = GB_export_error("Can't open database '%s'", file_name);
        free(pg_global.pg_filename);
        pg_global.pg_filename = 0;
    }

    free(file_name);

    if (error) {
        aw_message(error);
    }
}

// --------------------------------------------------------------------------------
//     static void create_probe_group_groups_window(AW_window *aww)
// --------------------------------------------------------------------------------
static void create_probe_group_groups_window(AW_window *aww) {
    AW_root *aw_root = aww->get_root();

    if (pg_global.pg_groups_window==0) { // window was not created yet
        AW_window_simple *aws = new AW_window_simple;

        aws->init(aw_root, "PG_RESULT_SELECTION", "Probe group results");
        aws->load_xfig("pg_select.fig");

        aws->at("close");
        aws->callback((AW_CB0)AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE","C");

        aws->at("help");
        aws->callback(AW_POPUP_HELP,(AW_CL)"");
        aws->create_button("HELP","HELP");

        create_probe_group_result_sel_box(aw_root, aws);

        pg_global.pg_groups_window = aws;
    }
    else { // window already created -> refresh selection box
        create_probe_group_result_sel_box(aw_root, pg_global.pg_groups_window);
    }

    pg_global.pg_groups_window->show();
}

// --------------------------------------------------------------------------------
//     AW_window *create_probe_group_result_window(AW_root *awr, AW_default cl_AW_canvas_ntw)
// --------------------------------------------------------------------------------
AW_window *create_probe_group_result_window(AW_root *awr, AW_default cl_AW_canvas_ntw){
    if (pg_global.awar_pg_result_filename) {
        free(pg_global.awar_pg_result_filename);
        pg_global.awar_pg_result_filename = 0;
    }

    pg_global.ntw = (AWT_canvas*)cl_AW_canvas_ntw;

    return awt_create_load_box(awr, "arb_probe_group result", "arb", &pg_global.awar_pg_result_filename,
                               create_probe_group_groups_window,
                               0);
}


